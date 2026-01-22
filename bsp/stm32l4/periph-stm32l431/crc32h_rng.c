#include <periph/crc32h.h>
#include <periph/rng.h>

#include <stm32l4xx_ll_crc.h>
#include <stm32l4xx_ll_rng.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_system.h>

#include <semaphore.h>
#include <task.h>
#include <kernel.h>
#include <assert.h>

TITAN_PDEBUG_FILE_MARK;

static semaphore_t crc_semaphore;
static semaphore_t rng_semaphore;
static uint8_t crc_semaphore_created = 0;
static uint8_t rng_semaphore_created = 0;

static uint32_t *rng_pool;
static uint32_t rng_pool_count;
static uint32_t rng_pool_length;
static task_t *rng_calling_task;


void crc32_hard_init(void) {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
    LL_CRC_ResetCRCCalculationUnit(CRC);
    LL_CRC_SetPolynomialCoef(CRC, LL_CRC_DEFAULT_CRC32_POLY);
    LL_CRC_SetPolynomialSize(CRC, LL_CRC_POLYLENGTH_32B);
    LL_CRC_SetInitialData(CRC, LL_CRC_DEFAULT_CRC_INITVALUE);  
    LL_CRC_SetInputDataReverseMode(CRC, LL_CRC_INDATA_REVERSE_WORD);
    LL_CRC_SetOutputDataReverseMode(CRC, LL_CRC_OUTDATA_REVERSE_BIT);

    if(crc_semaphore_created == 0) {
        semaphore_create(&crc_semaphore, 1);
        crc_semaphore_created = 1;
    }
}

void crc32_hard_deinit(void) {
    LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_CRC);
}

uint32_t crc32_hard(const uint8_t *buffer, const uint32_t length) {
    passert(buffer);
    passert(length);

    semaphore_lock(&crc_semaphore);

    register uint32_t data = 0;
    register uint32_t index = 0;

    LL_CRC_ResetCRCCalculationUnit(CRC);

    /* Compute the CRC of Data Buffer array*/
    for (index = 0; index < (length / 4); index++) {
        data = (uint32_t)((buffer[4 * index + 3] << 24) | (buffer[4 * index + 2] << 16) | (buffer[4 * index + 1] << 8) | buffer[4 * index]);
        LL_CRC_FeedData32(CRC, data);
    }

    /* Last bytes specific handling */
    if((length % 4) != 0) {
        if (length % 4 == 1) {
            LL_CRC_FeedData8(CRC, buffer[4 * index]);
        }
        else if(length % 4 == 2) {
            LL_CRC_FeedData16(CRC, (uint16_t)((buffer[4 * index + 1] << 8) | buffer[4 * index]));
        }
        else if(length % 4 == 3) {
            LL_CRC_FeedData16(CRC, (uint16_t)((buffer[4 * index + 1] << 8) | buffer[4 * index]));
            LL_CRC_FeedData8(CRC, buffer[4 * index + 2]);
        }
    }

    uint32_t result = LL_CRC_ReadData32(CRC) ^ 0xFFFFFFFF;
    semaphore_unlock(&crc_semaphore);

    return result;
}

void rng_init(void) {
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_RNG);
    LL_RCC_SetRNGClockSource(LL_RCC_RNG_CLKSOURCE_MSI);

    NVIC_SetPriority(RNG_IRQn, 0);  
    NVIC_EnableIRQ(RNG_IRQn);

    if(rng_semaphore_created == 0) {
        semaphore_create(&rng_semaphore, 1);
        rng_semaphore_created = 1;
    }
}

void rng_deinit(void) {
    LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_RNG);
}

void rng_random(uint32_t *pool, uint32_t pool_length) {
    passert(pool);
    passert(pool_length);

    semaphore_lock(&rng_semaphore);

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    rng_calling_task = (task_t*)kernel_current_task;
    
    rng_pool = pool;
    rng_pool_length = pool_length;
    rng_pool_count = 0;
    LL_RNG_Enable(RNG);
    LL_RNG_EnableIT(RNG);

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);

    LL_RNG_Disable(RNG);
    semaphore_unlock(&rng_semaphore);
}

void RNG_IRQHandler(void) {
    if((LL_RNG_IsActiveFlag_CECS(RNG)) || (LL_RNG_IsActiveFlag_SECS(RNG))) {
        /* Call Error function */
    }

    if(LL_RNG_IsActiveFlag_DRDY(RNG)) {
        rng_pool[rng_pool_count++] = LL_RNG_ReadRandData32(RNG);
        if(rng_pool_count >= rng_pool_length) {
            LL_RNG_DisableIT(RNG);
            kernel_scheduler_io_wait_finished(rng_calling_task);
            kernel_scheduler_trigger();
        }
    }
}
