#include <periph/i2c.h>
#include <periph/cpu.h>
#include <kernel.h>

#include <stm32l4xx_ll_bus.h>
#include <stm32l4xx_ll_rcc.h>
#include <stm32l4xx_ll_i2c.h>

#include <assert.h>

TITAN_PDEBUG_FILE_MARK;

#define TITAN_I2C_TOTAL     4

typedef enum {
    I2C_UNUSED = 0,
    I2C_MASTER,
    I2C_SLAVE,
} titan_i2c_type_t;

typedef enum {
    I2C_MODE_READ,
    I2C_MODE_WRITE,
    I2C_MODE_READ_MEM,
    I2C_MODE_WRITE_MEM,
} titan_i2c_mode_t;

typedef struct __titan_i2c_setting {
    titan_i2c_type_t type;

    task_t *task;

    titan_i2c_mode_t mode;
    uint8_t slave_address;
    uint8_t reg[4];
    uint8_t reg_size;

    uint8_t *buffer;
    uint32_t length;
    uint32_t count;

    uint8_t nack_error;
} titan_i2c_setting_t;

static titan_i2c_setting_t titan_i2c_settings[TITAN_I2C_TOTAL] = {0};

static uint8_t _i2c_to_i2c_no(i2c_t i2c) {
    switch(i2c) {
        case (i2c_t)I2C1:
            return 0;
            break;

        case (i2c_t)I2C2:
            return 1;
            break;

        case (i2c_t)I2C3:
            return 2;
            break;

        case (i2c_t)I2C4:
            return 3;
            break;

        default:
            passert(0);
            break;
    }
    return 255;
}

static void _i2c_irq(i2c_t i2c, uint8_t i2c_no) {

    if(LL_I2C_IsActiveFlag_NACK((I2C_TypeDef*)i2c)) {
        LL_I2C_ClearFlag_NACK((I2C_TypeDef*)i2c);

        if(titan_i2c_settings[i2c_no].count < titan_i2c_settings[i2c_no].length) {
            titan_i2c_settings[i2c_no].nack_error = 1;
        }
    }    
    else if(LL_I2C_IsActiveFlag_RXNE((I2C_TypeDef*)i2c)) {
        titan_i2c_settings[i2c_no].buffer[titan_i2c_settings[i2c_no].count] = LL_I2C_ReceiveData8((I2C_TypeDef*)i2c);
        titan_i2c_settings[i2c_no].count++;
    }
    
    else if(LL_I2C_IsActiveFlag_TXIS((I2C_TypeDef*)i2c)) {
        if(titan_i2c_settings[i2c_no].mode != I2C_MODE_WRITE) {
            LL_I2C_TransmitData8((I2C_TypeDef*)i2c, titan_i2c_settings[i2c_no].reg[titan_i2c_settings[i2c_no].reg_size - 1]);
            titan_i2c_settings[i2c_no].reg_size--;
        }
        else {
            LL_I2C_TransmitData8((I2C_TypeDef*)i2c, titan_i2c_settings[i2c_no].buffer[titan_i2c_settings[i2c_no].count]);
            titan_i2c_settings[i2c_no].count++;
        }
    }

    else if(LL_I2C_IsActiveFlag_TCR((I2C_TypeDef*)i2c)) {
        if(titan_i2c_settings[i2c_no].mode == I2C_MODE_WRITE_MEM) {
            titan_i2c_settings[i2c_no].mode = I2C_MODE_WRITE;
        }

        if(titan_i2c_settings[i2c_no].count < titan_i2c_settings[i2c_no].length) {
            uint32_t nlength = titan_i2c_settings[i2c_no].length - titan_i2c_settings[i2c_no].count;

            if(nlength > 255) {
                LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, titan_i2c_settings[i2c_no].slave_address, LL_I2C_ADDRSLAVE_7BIT, 255, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_NOSTARTSTOP);
            }
            else {
                LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, titan_i2c_settings[i2c_no].slave_address, LL_I2C_ADDRSLAVE_7BIT, nlength, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_NOSTARTSTOP);
            }
        }
    }
    else if(LL_I2C_IsActiveFlag_TC((I2C_TypeDef*)i2c)) {
        if(titan_i2c_settings[i2c_no].mode == I2C_MODE_READ_MEM) {
            titan_i2c_settings[i2c_no].mode = I2C_MODE_READ;

            if(titan_i2c_settings[i2c_no].length > 255) {
                LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, titan_i2c_settings[i2c_no].slave_address, LL_I2C_ADDRSLAVE_7BIT, 255, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_READ);
            }
            else {
                LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, titan_i2c_settings[i2c_no].slave_address, LL_I2C_ADDRSLAVE_7BIT, titan_i2c_settings[i2c_no].length, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
            }
        }
    }

    if(LL_I2C_IsActiveFlag_STOP((I2C_TypeDef*)i2c)) {
        LL_I2C_ClearFlag_STOP((I2C_TypeDef*)i2c);

        kernel_scheduler_io_wait_finished(titan_i2c_settings[i2c_no].task);
        kernel_scheduler_trigger();
    }
}

static void _i2c_common_init(i2c_t i2c) {
    switch(i2c) {
        case (i2c_t)I2C1:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
            LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_SYSCLK);
            NVIC_SetPriority(I2C1_EV_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C1_EV_IRQn);
            NVIC_SetPriority(I2C1_ER_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C1_ER_IRQn);
            break;

        case (i2c_t)I2C2:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);
            LL_RCC_SetI2CClockSource(LL_RCC_I2C2_CLKSOURCE_SYSCLK);
            NVIC_SetPriority(I2C2_EV_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C2_EV_IRQn);
            NVIC_SetPriority(I2C2_ER_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C2_ER_IRQn);
            break;

        case (i2c_t)I2C3:
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);
            LL_RCC_SetI2CClockSource(LL_RCC_I2C3_CLKSOURCE_SYSCLK);
            NVIC_SetPriority(I2C3_EV_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C3_EV_IRQn);
            NVIC_SetPriority(I2C3_ER_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C3_ER_IRQn);
            break;

        case (i2c_t)I2C4:
            LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_I2C4);
            LL_RCC_SetI2CClockSource(LL_RCC_I2C4_CLKSOURCE_SYSCLK);
            NVIC_SetPriority(I2C4_EV_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C4_EV_IRQn);
            NVIC_SetPriority(I2C4_ER_IRQn, 0);      //checkAL
            NVIC_EnableIRQ(I2C4_ER_IRQn);
            break;

        default:
            passert(0);
            break;
    }
}

static uint32_t _i2c_timing(i2c_baudrate_t baudrate) {
    uint32_t timing = 0;
    uint32_t i2c_clock = cpu_clock_speed();
    uint32_t req_freq = 0;

    uint8_t presc = 1;
    uint16_t sclh;
    uint16_t scll;
    uint8_t scldel;
    uint8_t sdadel;
    uint8_t sclratio = 4;

    sdadel = 0;
    scldel = 0;
    
    switch (baudrate) {
        case I2C_BAUDRATE_100KHZ:
            req_freq = 100000;
            sclratio = 2;
            break;

        case I2C_BAUDRATE_400KHZ:
            req_freq = 400000;
            sclratio = 3;
            break;

        case I2C_BAUDRATE_1000KHZ:
            req_freq = 1000000;
            sclratio = 4;
            break;

        case I2C_BAUDRATE_3400KHZ:
            passert(0); //not supported
            break;

        default:
            passert(0);
            break;
    }

    uint32_t scl_tim = i2c_clock / presc / req_freq;
    while (scl_tim >= 510) {
        presc++;
        scl_tim = i2c_clock / presc / req_freq;
    }

    scldel = scl_tim / 16;
    if (scldel > 0x0A) {
        scldel /= 2;
    }

    scl_tim -= scldel;
    sclh = scl_tim / sclratio;
    scll = scl_tim - sclh;

    if (scll > 255) {
        sclh += scll - 255;
        scll = 255;
    }

    presc--;
    sclh--;
    scll--;

    timing |= ((presc & 0x0F) << 28);
    timing |= ((scldel & 0x0F) << 20);
    timing |= ((sdadel & 0x0F) << 16);
    timing |= ((sclh & 0xFF) << 8);
    timing |= ((scll & 0xFF) << 0);

    //checkAL -- still not perfect
    return timing;
}


void i2c_master_init(i2c_t i2c, i2c_baudrate_t baudrate) {
    uint8_t i2c_no = _i2c_to_i2c_no(i2c);
    uint32_t timing = 0;

    passert(titan_i2c_settings[i2c_no].type == I2C_UNUSED);

    timing = _i2c_timing(baudrate);

    _i2c_common_init(i2c);

    //checkAL  LL_SYSCFG_EnableFastModePlus   in _system.h

    LL_I2C_Disable((I2C_TypeDef*)i2c);
    LL_I2C_SetTiming((I2C_TypeDef*)i2c, timing);
    LL_I2C_SetOwnAddress1((I2C_TypeDef*)i2c, 0x00, LL_I2C_OWNADDRESS1_7BIT);
    LL_I2C_DisableOwnAddress1((I2C_TypeDef*)i2c);
    LL_I2C_EnableClockStretching((I2C_TypeDef*)i2c);
    LL_I2C_SetDigitalFilter((I2C_TypeDef*)i2c, 0x00);
    LL_I2C_EnableAnalogFilter((I2C_TypeDef*)i2c);
    LL_I2C_DisableGeneralCall((I2C_TypeDef*)i2c);
    LL_I2C_SetOwnAddress2((I2C_TypeDef*)i2c, 0x00, LL_I2C_OWNADDRESS2_NOMASK);
    LL_I2C_DisableOwnAddress2((I2C_TypeDef*)i2c);
    LL_I2C_SetMasterAddressingMode((I2C_TypeDef*)i2c, LL_I2C_ADDRESSING_MODE_7BIT);
    LL_I2C_SetMode((I2C_TypeDef*)i2c, LL_I2C_MODE_I2C);
    LL_I2C_Enable((I2C_TypeDef*)i2c);
    LL_I2C_EnableIT_TX((I2C_TypeDef*)i2c);
    LL_I2C_EnableIT_RX((I2C_TypeDef*)i2c);
    LL_I2C_EnableIT_TC((I2C_TypeDef*)i2c);
    LL_I2C_EnableIT_NACK((I2C_TypeDef*)i2c);
    //LL_I2C_EnableIT_ERR((I2C_TypeDef*)i2c);   //checkAL
    LL_I2C_EnableIT_STOP((I2C_TypeDef*)i2c);

    titan_i2c_settings[i2c_no].type = I2C_MASTER;
}

void i2c_deinit(i2c_t i2c) {
    uint8_t i2c_no = _i2c_to_i2c_no(i2c);
    passert(titan_i2c_settings[i2c_no].type != I2C_UNUSED);

    switch(titan_i2c_settings[i2c_no].type) {
        case I2C_MASTER:
            LL_I2C_DisableIT_TX((I2C_TypeDef*)i2c);
            LL_I2C_DisableIT_RX((I2C_TypeDef*)i2c);
            LL_I2C_DisableIT_TC((I2C_TypeDef*)i2c);
            LL_I2C_DisableIT_NACK((I2C_TypeDef*)i2c);
            //LL_I2C_DisableIT_ERR((I2C_TypeDef*)i2c);  //checkAL
            LL_I2C_DisableIT_STOP((I2C_TypeDef*)i2c);

            switch(i2c) {
                case (i2c_t)I2C1:
                    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C1);
                    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C1);
                    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C1);
                    NVIC_DisableIRQ(I2C1_EV_IRQn);
                    NVIC_DisableIRQ(I2C1_ER_IRQn);
                    break;

                case (i2c_t)I2C2:
                    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C2);
                    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C2);
                    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C2);
                    NVIC_DisableIRQ(I2C2_EV_IRQn);
                    NVIC_DisableIRQ(I2C2_ER_IRQn);
                    break;

                case (i2c_t)I2C3:
                    LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_I2C3);
                    LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_I2C3);
                    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_I2C3);
                    NVIC_DisableIRQ(I2C3_EV_IRQn);
                    NVIC_DisableIRQ(I2C3_ER_IRQn);
                    break;

                case (i2c_t)I2C4:
                    LL_APB1_GRP2_ForceReset(LL_APB1_GRP2_PERIPH_I2C4);
                    LL_APB1_GRP2_ReleaseReset(LL_APB1_GRP2_PERIPH_I2C4);
                    LL_APB1_GRP2_DisableClock(LL_APB1_GRP2_PERIPH_I2C4);
                    NVIC_DisableIRQ(I2C4_EV_IRQn);
                    NVIC_DisableIRQ(I2C4_ER_IRQn);
                    break;
            }
            break;

        case I2C_SLAVE:
            passert(0); //unsupported yet
            break;

        default:
            passert(0);
            break;
    }

    titan_i2c_settings[i2c_no].type = I2C_UNUSED;
}


uint32_t i2c_master_read(i2c_t i2c, uint8_t slave_address, void *data, uint32_t length) {
    uint8_t i2c_no = _i2c_to_i2c_no(i2c);
    passert(data != 0);
    passert(length != 0);

    titan_i2c_settings[i2c_no].mode = I2C_MODE_READ;
    titan_i2c_settings[i2c_no].slave_address = slave_address;

    titan_i2c_settings[i2c_no].buffer = data;
    titan_i2c_settings[i2c_no].length = length;
    titan_i2c_settings[i2c_no].count = 0;
    titan_i2c_settings[i2c_no].nack_error = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_i2c_settings[i2c_no].task = (task_t*)kernel_current_task;
    
    if(length > 255) {
        LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, slave_address, LL_I2C_ADDRSLAVE_7BIT, 255, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_READ);
    }
    else {
        LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, slave_address, LL_I2C_ADDRSLAVE_7BIT, length, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    }

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_i2c_settings[i2c_no].nack_error;
}

uint32_t i2c_master_write(i2c_t i2c, uint8_t slave_address, const void *data, uint32_t length) {
    uint8_t i2c_no = _i2c_to_i2c_no(i2c);

    titan_i2c_settings[i2c_no].mode = I2C_MODE_WRITE;
    titan_i2c_settings[i2c_no].slave_address = slave_address;

    titan_i2c_settings[i2c_no].buffer = (uint8_t*)data;
    titan_i2c_settings[i2c_no].length = length;
    titan_i2c_settings[i2c_no].count = 0;
    titan_i2c_settings[i2c_no].nack_error = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_i2c_settings[i2c_no].task = (task_t*)kernel_current_task;
    
    if(length > 255) {
        LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, slave_address, LL_I2C_ADDRSLAVE_7BIT, 255, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_WRITE);
    }
    else {
        LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, slave_address, LL_I2C_ADDRSLAVE_7BIT, length, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    }

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_i2c_settings[i2c_no].nack_error;
}


uint32_t i2c_master_read_mem(i2c_t i2c, uint8_t slave_address, uint32_t reg, uint8_t reg_size, void *data, uint32_t length) {
    uint8_t i2c_no = _i2c_to_i2c_no(i2c);
    uint8_t i = 0;

    titan_i2c_settings[i2c_no].mode = I2C_MODE_READ_MEM;
    titan_i2c_settings[i2c_no].slave_address = slave_address;
    titan_i2c_settings[i2c_no].reg_size = reg_size;
    while(i < reg_size) {
        titan_i2c_settings[i2c_no].reg[i] = reg % 256;
        reg /= 256;
        i++;
    }
    
    titan_i2c_settings[i2c_no].buffer = data;
    titan_i2c_settings[i2c_no].length = length;
    titan_i2c_settings[i2c_no].count = 0;
    titan_i2c_settings[i2c_no].nack_error = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_i2c_settings[i2c_no].task = (task_t*)kernel_current_task;
    
    LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, slave_address, LL_I2C_ADDRSLAVE_7BIT, titan_i2c_settings[i2c_no].reg_size, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_i2c_settings[i2c_no].nack_error;
}

uint32_t i2c_master_write_mem(i2c_t i2c, uint8_t slave_address, uint32_t reg, uint8_t reg_size, const void *data, uint32_t length) {
    uint8_t i2c_no = _i2c_to_i2c_no(i2c);
    uint8_t i = 0;

    titan_i2c_settings[i2c_no].mode = I2C_MODE_WRITE_MEM;
    titan_i2c_settings[i2c_no].slave_address = slave_address;
    titan_i2c_settings[i2c_no].reg_size = reg_size;
    while(i < reg_size) {
        titan_i2c_settings[i2c_no].reg[i] = reg % 256;
        reg /= 256;
        i++;
    }

    titan_i2c_settings[i2c_no].buffer = (uint8_t*)data;
    titan_i2c_settings[i2c_no].length = length;
    titan_i2c_settings[i2c_no].count = 0;
    titan_i2c_settings[i2c_no].nack_error = 0;

    volatile kernel_atomic_t __atomic;
    kernel_begin_critical(&__atomic);
    titan_i2c_settings[i2c_no].task = (task_t*)kernel_current_task;
    
    LL_I2C_HandleTransfer((I2C_TypeDef*)i2c, slave_address, LL_I2C_ADDRSLAVE_7BIT, titan_i2c_settings[i2c_no].reg_size, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_WRITE);

    kernel_scheduler_io_wait_start();
    kernel_scheduler_trigger();
    kernel_end_critical(&__atomic);
    return titan_i2c_settings[i2c_no].nack_error;
}


void I2C1_EV_IRQHandler(void) {
    _i2c_irq((i2c_t)I2C1, 0);
}

void I2C2_EV_IRQHandler(void) {
    _i2c_irq((i2c_t)I2C2, 1);
}

void I2C3_EV_IRQHandler(void) {
    _i2c_irq((i2c_t)I2C3, 2);
}

void I2C4_EV_IRQHandler(void) {
    _i2c_irq((i2c_t)I2C4, 3);
}
