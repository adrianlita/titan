#include "apds9301.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define APDS9301_ADDRESS_GND            0x52
#define APDS9301_ADDRESS_FLOAT          0x72
#define APDS9301_ADDRESS_VDD            0x92

#define APDS9301_REG_CONTROL            0x80
#define APDS9301_REG_TIMING             0x81
#define APDS9301_REG_THRESHLOWLOW       0x82
#define APDS9301_REG_THRESHLOWHIGH      0x83
#define APDS9301_REG_THRESHHIGHLOW      0x84
#define APDS9301_REG_THRESHHIGHIGH      0x85
#define APDS9301_REG_INTERRUPT          0x86
#define APDS9301_REG_ID                 0x8A
#define APDS9301_REG_DATA0LOW           0x8C
#define APDS9301_REG_DATA0HIGH          0x8D
#define APDS9301_REG_DATA1LOW           0x8E
#define APDS9301_REG_DATA1HIGH          0x8F

uint32_t apds9301_init(apds9301_t *dev, i2c_t i2c, mutex_t *i2c_mutex, apds9301_addr_sel_t addr_sel_value) {
    assert(dev != 0);

    uint8_t whoami = 0;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    switch(addr_sel_value) {
        case APDS9301_ADDR_SEL_GND:
            dev->i2c_address = APDS9301_ADDRESS_GND;
            break;

        case APDS9301_ADDR_SEL_FLOAT:
            dev->i2c_address = APDS9301_ADDRESS_FLOAT;
            break;

        case APDS9301_ADDR_SEL_VDD:
            dev->i2c_address = APDS9301_ADDRESS_VDD;
            break;

        default:
            assert(0);
            break;
    }

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, APDS9301_REG_ID, 1, (void*)&whoami, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if((whoami & 0xF0) != 0x50) {
        ret = ENODEV;
    }

    dev->revision = whoami & 0x0F;

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t apds9301_turn_on(apds9301_t *dev, apds9301_gain_t gain, apds9301_integration_time_t integration_time) {
    assert(dev != 0);
    
    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    dev->gain = gain;
    dev->integration_time = integration_time;


    buffer = 0x03;
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, APDS9301_REG_CONTROL, 1, &buffer, 1);

    buffer = (gain << 4) | integration_time;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, APDS9301_REG_TIMING, 1, &buffer, 1);

    buffer = (0x01 << 4);
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, APDS9301_REG_INTERRUPT, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t apds9301_turn_off(apds9301_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x00;
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, APDS9301_REG_CONTROL, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}


uint32_t apds9301_read(apds9301_t *dev, apds9301_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[6];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }


    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, APDS9301_REG_DATA0LOW | 0x20, 1, buffer, 2);
    data->ch0 = (uint16_t)(((uint16_t)buffer[1] << 8) + buffer[0]);

    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, APDS9301_REG_DATA1LOW | 0x20 | 0x40, 1, buffer, 2); //also clear interrupt
    data->ch1 = (uint16_t)(((uint16_t)buffer[1] << 8) + buffer[0]);
    
    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
