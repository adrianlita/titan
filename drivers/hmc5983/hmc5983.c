#include "hmc5983.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define HMC5983_ADDRESS             0x3C

#define HMC5983_REG_CONF_A          0x00
#define HMC5983_REG_CONF_B          0x01
#define HMC5983_REG_MODE            0x02
#define HMC5983_REG_OUT_X_MSB       0x03
#define HMC5983_REG_OUT_X_LSB       0x04
#define HMC5983_REG_OUT_Z_MSB       0x05
#define HMC5983_REG_OUT_Z_LSB       0x06
#define HMC5983_REG_OUT_Y_MSB       0x07
#define HMC5983_REG_OUT_Y_LSB       0x08
#define HMC5983_REG_STATUS          0x09
#define HMC5983_REG_ID_A            0x0A
#define HMC5983_REG_ID_B            0x0B
#define HMC5983_REG_ID_C            0x0C
#define HMC5983_REG_TEMP_OUT_MSB    0x31
#define HMC5983_REG_TEMP_OUT_LSB    0x32

#define HMC5983_ID_A                0x48
#define HMC5983_ID_B                0x34
#define HMC5983_ID_C                0x33


uint32_t hmc5983_init(hmc5983_t *dev, i2c_t i2c, mutex_t *i2c_mutex) {
    assert(dev != 0);

    uint8_t ida, idb, idc;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_ID_A, 1, (void*)&ida, 1);
    ret += i2c_master_read_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_ID_B, 1, (void*)&idb, 1);
    ret += i2c_master_read_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_ID_C, 1, (void*)&idc, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if((ida != HMC5983_ID_A) || (idb != HMC5983_ID_B) || (idc != HMC5983_ID_C) ) {
        ret = ENODEV;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t hmc5983_turn_on(hmc5983_t *dev, hmc5983_data_rate_t data_rate, hmc5983_gain_t gain) {
    assert(dev != 0);
    
    uint32_t ret = 0;
    uint8_t buffer;

    dev->data_rate = data_rate;
    dev->gain = gain;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x80 | (data_rate << 2);   //set data rate. enable temperature sensor to compensate the measurement. no average
    ret = i2c_master_write_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_CONF_A, 1, &buffer, 1);

    buffer = (gain << 5);
    ret += i2c_master_write_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_CONF_B, 1, &buffer, 1);

    buffer = 0x00;
    ret += i2c_master_write_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_MODE, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t hmc5983_turn_off(hmc5983_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    dev->data_rate = HMC5983_DATA_RATE_OFF;
    buffer = 0x02;
    ret += i2c_master_write_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_MODE, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t hmc5983_read(hmc5983_t *dev, hmc5983_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[6];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret += i2c_master_read_mem(dev->i2c, HMC5983_ADDRESS, HMC5983_REG_OUT_X_MSB, 1, buffer, 6);
    data->x = (int16_t)(((int16_t)buffer[1] << 8) + buffer[0]);
    data->z = (int16_t)(((int16_t)buffer[3] << 8) + buffer[2]);
    data->y = (int16_t)(((int16_t)buffer[5] << 8) + buffer[4]);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
