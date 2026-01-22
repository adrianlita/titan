#include <periph/spi.h>
#include <periph/cpu.h>
#include <kernel.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_spi.h>

#include <assert.h>

TITAN_PDEBUG_FILE_MARK;

#define TITAN_SPI_TOTAL     3

typedef enum {
    SPI_UNUSED = 0,
    SPI_MASTER,
    SPI_SLAVE,
} titan_spi_type_t;

typedef enum {
    SPI_MODE_READ     = 0x01,
    SPI_MODE_WRITE    = 0x02,
    SPI_MODE_TRANSFER = 0x03,
} titan_spi_mode_t;

typedef struct __titan_spi_setting {
    titan_spi_type_t type;

    task_t *task;

    titan_spi_mode_t mode;

    uint8_t *write_buffer;
    uint32_t write_length;
    uint32_t write_count;

    uint8_t *read_buffer;
    uint32_t read_length;
    uint32_t read_count;

    uint8_t transfer_error;
} titan_spi_setting_t;

static titan_spi_setting_t titan_spi_settings[TITAN_SPI_TOTAL] = {0};

static uint8_t _spi_to_spi_no(spi_t spi) {
    switch(spi) {
        case (spi_t)SPI1:
            return 0;
            break;

        case (spi_t)SPI2:
            return 1;
            break;

        case (spi_t)SPI3:
            return 2;
            break;

        default:
            passert(0);
            break;
    }
    return 255;
}

static void _spi_irq(spi_t spi, uint8_t spi_no) {
    if(LL_SPI_IsActiveFlag_RXNE((SPI_TypeDef*)spi)) {
        if(titan_spi_settings[spi_no].read_count < titan_spi_settings[spi_no].read_length) {
            if(titan_spi_settings[spi_no].mode & SPI_MODE_READ) {
                titan_spi_settings[spi_no].read_buffer[titan_spi_settings[spi_no].read_count] = LL_SPI_ReceiveData8((SPI_TypeDef*)spi);
            }
            else {
                volatile uint8_t unused = LL_SPI_ReceiveData8((SPI_TypeDef*)spi);
                (void)unused;
            }

            titan_spi_settings[spi_no].read_count++;
            if(titan_spi_settings[spi_no].read_count == titan_spi_settings[spi_no].read_length) {
                LL_SPI_DisableIT_TXE((SPI_TypeDef*)spi);
                LL_SPI_DisableIT_RXNE((SPI_TypeDef*)spi);
                LL_SPI_DisableIT_ERR((SPI_TypeDef*)spi);

                kernel_scheduler_io_wait_finished(titan_spi_settings[spi_no].task);
                kernel_scheduler_trigger();
            }
        }
    }
    else if(LL_SPI_IsActiveFlag_TXE((SPI_TypeDef*)spi)) {
        while((titan_spi_settings[spi_no].write_count < titan_spi_settings[spi_no].write_length) && (LL_SPI_GetTxFIFOLevel((SPI_TypeDef*)spi) != LL_SPI_TX_FIFO_FULL)) {
            if(titan_spi_settings[spi_no].mode & SPI_MODE_WRITE) {
                LL_SPI_TransmitData8((SPI_TypeDef*)spi, titan_spi_settings[spi_no].write_buffer[titan_spi_settings[spi_no].write_count]);
            }
            else {
                LL_SPI_TransmitData8((SPI_TypeDef*)spi, 0x00);
            }
            titan_spi_settings[spi_no].write_count++;
        }
    }
    else if(LL_SPI_IsActiveFlag_OVR((SPI_TypeDef*)spi)) {
        titan_spi_settings[spi_no].transfer_error = 1;
        
        LL_SPI_DisableIT_RXNE((SPI_TypeDef*)spi);
        LL_SPI_DisableIT_TXE((SPI_TypeDef*)spi);
        LL_SPI_DisableIT_ERR((SPI_TypeDef*)spi);

        kernel_scheduler_io_wait_finished(titan_spi_settings[spi_no].task);
        kernel_scheduler_trigger();
    }
}

static void _spi_common_init(spi_t spi) {
    switch(spi) {
        case (spi_t)SPI1:
            LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
            NVIC_SetPriority(SPI1_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(SPI1_IRQn);
            break;

        case (spi_t)SPI2:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);
            NVIC_SetPriority(SPI2_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(SPI2_IRQn);
            break;

        case (spi_t)SPI3:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3);
            NVIC_SetPriority(SPI3_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(SPI3_IRQn);
            break;

        default:
            passert(0);
            break;
    }
}

void spi_master_init(spi_t spi, spi_mode_t mode, uint8_t data_width, spi_bit_order_t bit_order, uint32_t frequency) {
    uint8_t spi_no = _spi_to_spi_no(spi);

    passert(titan_spi_settings[spi_no].type == SPI_UNUSED);

    _spi_common_init(spi);
    
    uint32_t prescaler = cpu_clock_speed() / frequency;
    if(prescaler > 192) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV256;
    }
    else if(prescaler > 96) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV128;
    }
    else if(prescaler > 48) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV64;
    }
    else if(prescaler > 24) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV32;
    }
    else if(prescaler > 12) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV16;
    }
    else if(prescaler > 6) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV8;
    }
    else if(prescaler > 3) {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV4;
    }
    else {
        prescaler = LL_SPI_BAUDRATEPRESCALER_DIV2;
    }
    LL_SPI_SetBaudRatePrescaler((SPI_TypeDef*)spi, prescaler);

    uint32_t phase;
    uint32_t polarity;
    switch(mode) {
        case SPI_MODE_0:         //polarity == 0, phase == 0
            polarity = LL_SPI_POLARITY_LOW;
            phase = LL_SPI_PHASE_1EDGE;
            break;

        case SPI_MODE_1:         //polarity == 0, phase == 1
            polarity = LL_SPI_POLARITY_LOW;
            phase = LL_SPI_PHASE_2EDGE;
            break;

        case SPI_MODE_2:         //polarity == 1, phase == 0
            polarity = LL_SPI_POLARITY_HIGH;
            phase = LL_SPI_PHASE_1EDGE;
            break;

        case SPI_MODE_3:         //polarity == 1, phase == 1
            polarity = LL_SPI_POLARITY_HIGH;
            phase = LL_SPI_PHASE_2EDGE;
            break;
    }

    uint32_t calc_data_width;
    switch(data_width) {
        case 4:
            calc_data_width = LL_SPI_DATAWIDTH_4BIT;
            break;

        case 5:
            calc_data_width = LL_SPI_DATAWIDTH_5BIT;
            break;

        case 6:
            calc_data_width = LL_SPI_DATAWIDTH_6BIT;
            break;

        case 7:
            calc_data_width = LL_SPI_DATAWIDTH_7BIT;
            break;

        case 8:
            calc_data_width = LL_SPI_DATAWIDTH_8BIT;
            break;

        case 9:
            calc_data_width = LL_SPI_DATAWIDTH_9BIT;
            break;

        case 10:
            calc_data_width = LL_SPI_DATAWIDTH_10BIT;
            break;

        case 11:
            calc_data_width = LL_SPI_DATAWIDTH_11BIT;
            break;

        case 12:
            calc_data_width = LL_SPI_DATAWIDTH_12BIT;
            break;

        case 13:
            calc_data_width = LL_SPI_DATAWIDTH_13BIT;
            break;

        case 14:
            calc_data_width = LL_SPI_DATAWIDTH_14BIT;
            break;

        case 15:
            calc_data_width = LL_SPI_DATAWIDTH_15BIT;
            break;

        case 16:
            calc_data_width = LL_SPI_DATAWIDTH_16BIT;
            break;

        default:
            passert(0);
            break;
    }

    uint32_t calc_bit_order = LL_SPI_MSB_FIRST;
    if(bit_order == SPI_BIT_ORDER_LSB_FIRST) {
        calc_bit_order = LL_SPI_LSB_FIRST;
    }

    LL_SPI_SetClockPhase((SPI_TypeDef*)spi, phase);
    LL_SPI_SetClockPolarity((SPI_TypeDef*)spi, polarity);
    LL_SPI_SetDataWidth((SPI_TypeDef*)spi, calc_data_width);
    LL_SPI_SetTransferBitOrder((SPI_TypeDef*)spi, calc_bit_order);

    LL_SPI_SetNSSMode((SPI_TypeDef*)spi, LL_SPI_NSS_SOFT);
    LL_SPI_SetRxFIFOThreshold((SPI_TypeDef*)spi, LL_SPI_RX_FIFO_TH_QUARTER);
    LL_SPI_SetMode((SPI_TypeDef*)spi, LL_SPI_MODE_MASTER);

    titan_spi_settings[spi_no].type = SPI_MASTER;

    LL_SPI_SetTransferDirection((SPI_TypeDef*)spi, LL_SPI_FULL_DUPLEX);

    LL_SPI_Enable((SPI_TypeDef*)spi);
}

void spi_deinit(spi_t spi) {
    uint8_t spi_no = _spi_to_spi_no(spi);
    passert(titan_spi_settings[spi_no].type != SPI_UNUSED);

    switch(titan_spi_settings[spi_no].type) {
        case SPI_MASTER:
            LL_SPI_Disable((SPI_TypeDef*)spi);

            LL_SPI_DisableIT_RXNE((SPI_TypeDef*)spi);
            LL_SPI_DisableIT_TXE((SPI_TypeDef*)spi);
            LL_SPI_DisableIT_ERR((SPI_TypeDef*)spi);

            switch(spi) {
                case (spi_t)SPI1:
                    LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_SPI1);
                    LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_SPI1);
                    LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_SPI1);
                    NVIC_DisableIRQ(SPI1_IRQn);
                    break;

                case (spi_t)SPI2:
                    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_SPI2);
                    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_SPI2);
                    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_SPI2);
                    NVIC_DisableIRQ(SPI2_IRQn);
                    break;

                case (spi_t)SPI3:
                    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_SPI3);
                    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_SPI3);
                    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_SPI3);
                    NVIC_DisableIRQ(SPI3_IRQn);
                    break;
            }
            break;

        case SPI_SLAVE:
            passert(0); //unsupported yet
            break;

        default:
            passert(0);
            break;
    }

    titan_spi_settings[spi_no].type = SPI_UNUSED;
}


uint32_t spi_master_read(spi_t spi, void *data, uint32_t length) {
    uint8_t spi_no = _spi_to_spi_no(spi);
    passert(data != 0);
    passert(length != 0);

    titan_spi_settings[spi_no].mode = SPI_MODE_READ;

    titan_spi_settings[spi_no].read_buffer = data;
    titan_spi_settings[spi_no].read_length = length;
    titan_spi_settings[spi_no].read_count = 0;
    titan_spi_settings[spi_no].write_length = length;
    titan_spi_settings[spi_no].write_count = 0;
    titan_spi_settings[spi_no].transfer_error = 0;


    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_spi_settings[spi_no].task = (task_t*)kernel_current_task;
    
    LL_SPI_EnableIT_TXE((SPI_TypeDef*)spi);
    LL_SPI_EnableIT_RXNE((SPI_TypeDef*)spi);
    LL_SPI_EnableIT_ERR((SPI_TypeDef*)spi);

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_spi_settings[spi_no].transfer_error;
}

uint32_t spi_master_write(spi_t spi, const void *data, uint32_t length) {
    uint8_t spi_no = _spi_to_spi_no(spi);

    titan_spi_settings[spi_no].mode = SPI_MODE_WRITE;

    titan_spi_settings[spi_no].write_buffer = (uint8_t*)data;
    titan_spi_settings[spi_no].write_length = length;
    titan_spi_settings[spi_no].write_count = 0;
    titan_spi_settings[spi_no].read_length = length;
    titan_spi_settings[spi_no].read_count = 0;
    titan_spi_settings[spi_no].transfer_error = 0;


    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_spi_settings[spi_no].task = (task_t*)kernel_current_task;
    
    LL_SPI_EnableIT_RXNE((SPI_TypeDef*)spi);
    LL_SPI_EnableIT_TXE((SPI_TypeDef*)spi);
    LL_SPI_EnableIT_ERR((SPI_TypeDef*)spi);


    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_spi_settings[spi_no].transfer_error;
}

uint32_t spi_master_transfer(spi_t spi, const void *write_data, void *read_data, uint32_t length) {
    uint8_t spi_no = _spi_to_spi_no(spi);
    passert(write_data != 0);
    passert(read_data != 0);
    passert(length != 0);
    
    titan_spi_settings[spi_no].mode = SPI_MODE_TRANSFER;

    titan_spi_settings[spi_no].write_buffer = (uint8_t*)write_data;
    titan_spi_settings[spi_no].write_length = length;
    titan_spi_settings[spi_no].write_count = 0;
    
    titan_spi_settings[spi_no].read_buffer = read_data;
    titan_spi_settings[spi_no].read_length = length;
    titan_spi_settings[spi_no].read_count = 0;

    titan_spi_settings[spi_no].transfer_error = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_spi_settings[spi_no].task = (task_t*)kernel_current_task;

    
    LL_SPI_EnableIT_RXNE((SPI_TypeDef*)spi);
    LL_SPI_EnableIT_TXE((SPI_TypeDef*)spi);
    LL_SPI_EnableIT_ERR((SPI_TypeDef*)spi);

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_spi_settings[spi_no].transfer_error;
}

void SPI1_IRQHandler(void) {
    _spi_irq((spi_t)SPI1, 0);
}

void SPI2_IRQHandler(void) {
    _spi_irq((spi_t)SPI2, 1);
}

void SPI3_IRQHandler(void) {
    _spi_irq((spi_t)SPI3, 2);
}
