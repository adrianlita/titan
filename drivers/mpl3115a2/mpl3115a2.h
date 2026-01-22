#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
  MPL3115A2_PERIOD_1S = 0,
  MPL3115A2_PERIOD_2S = 1,
  MPL3115A2_PERIOD_4S = 2,
  MPL3115A2_PERIOD_8S = 3,
  MPL3115A2_PERIOD_16S = 4,
  MPL3115A2_PERIOD_32S = 5,
  MPL3115A2_PERIOD_64S = 6,
  MPL3115A2_PERIOD_128S = 7,
  MPL3115A2_PERIOD_256S = 8,
} mpl3115a2_period_t;

typedef struct __mpl3115a2_data {
    uint32_t pressure;
    int32_t temperature;
} mpl3115a2_data_t;

typedef struct __mpl3115a2 {
    i2c_t i2c;
    mutex_t *i2c_mutex;

    mpl3115a2_period_t period;

    uint8_t ctrl1;
    uint8_t ctrl2;
    uint8_t ctrl3;
    uint8_t ctrl4;
    uint8_t ctrl5;  
    uint8_t ctrl_ptdata;
} mpl3115a2_t;

uint32_t mpl3115a2_init(mpl3115a2_t *dev, i2c_t i2c, mutex_t *i2c_mutex);
uint32_t mpl3115a2_turn_on(mpl3115a2_t *dev, mpl3115a2_period_t period);
uint32_t mpl3115a2_turn_off(mpl3115a2_t *dev);

uint32_t mpl3115a2_read(mpl3115a2_t *dev, mpl3115a2_data_t *data);

/*
    usage:

    #include <mpl3115a2/mpl3115a2.h>

    mpl3115a2_t mpl3115a2;
    mpl3115a2_data_t baro_data;

    ret = mpl3115a2_init(&mpl3115a2, (i2c_t)I2C3, &i2c3_mutex);
    
    mpl3115a2_turn_on(&mpl3115a2, MPL3115A2_PERIOD_1S);
    gpio_init_interrupt(MPL3115A2_INT1, GPIO_MODE_INTERRUPT_REDGE, GPIO_PULL_NOPULL, mpl3115a2_on_int);
    if(gpio_digital_read(MPL3115A2_INT1) == 1) {
        mpl3115a2_read(&mpl3115a2, &baro_data);
        explorer_log("MPL3115A2 Data: %u kPa | %u deg C\r\n", baro_data.pressure / 64, baro_data.temperature / 256);
    }
*/
