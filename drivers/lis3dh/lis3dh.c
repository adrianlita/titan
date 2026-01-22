#include "lis3dh.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define LIS3DH_ADDRESS_SA0_0     0x30
#define LIS3DH_ADDRESS_SA0_1     0x32

#define LIS3DH_REG_OUTADC1_L     0x08
#define LIS3DH_REG_OUTADC1_H     0x09
#define LIS3DH_REG_OUTADC2_L     0x0A
#define LIS3DH_REG_OUTADC2_H     0x0B
#define LIS3DH_REG_OUTADC3_L     0x0C
#define LIS3DH_REG_OUTADC3_H     0x0D
#define LIS3DH_REG_INTCOUNT      0x0E
#define LIS3DH_REG_WHOAMI        0x0F
#define LIS3DH_REG_TEMPCFG       0x1F
#define LIS3DH_REG_CTRL1         0x20
#define LIS3DH_REG_CTRL2         0x21
#define LIS3DH_REG_CTRL3         0x22
#define LIS3DH_REG_CTRL4         0x23
#define LIS3DH_REG_CTRL5         0x24
#define LIS3DH_REG_CTRL6         0x25
#define LIS3DH_REG_REFERENCE     0x26
#define LIS3DH_REG_STATUS2       0x27
#define LIS3DH_REG_OUT_X_L       0x28
#define LIS3DH_REG_OUT_X_H       0x29
#define LIS3DH_REG_OUT_Y_L       0x2A
#define LIS3DH_REG_OUT_Y_H       0x2B
#define LIS3DH_REG_OUT_Z_L       0x2C
#define LIS3DH_REG_OUT_Z_H       0x2D
#define LIS3DH_REG_FIFOCTRL      0x2E
#define LIS3DH_REG_FIFOSRC       0x2F
#define LIS3DH_REG_INT1CFG       0x30
#define LIS3DH_REG_INT1SRC       0x31
#define LIS3DH_REG_INT1THS       0x32
#define LIS3DH_REG_INT1DUR       0x33
#define LIS3DH_REG_CLICKCFG      0x38
#define LIS3DH_REG_CLICKSRC      0x39
#define LIS3DH_REG_CLICKTHS      0x3A
#define LIS3DH_REG_TIMELIMIT     0x3B
#define LIS3DH_REG_TIMELATENCY   0x3C
#define LIS3DH_REG_TIMEWINDOW    0x3D

#define LIS3DH_WHOAMI            0x33

uint32_t lis3dh_init(lis3dh_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t sdo_pin_value) {
    assert(dev != 0);

    uint8_t whoami = 0;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    if(sdo_pin_value) {
        dev->i2c_address = LIS3DH_ADDRESS_SA0_1;
    }
    else {
        dev->i2c_address = LIS3DH_ADDRESS_SA0_0;
    }

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_WHOAMI, 1, (void*)&whoami, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if(whoami != LIS3DH_WHOAMI) {
        ret = ENODEV;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t lis3dh_turn_on(lis3dh_t *dev, lis3dh_data_rate_t data_rate, lis3dh_range_t range, uint8_t fifo_store) {
    assert(dev != 0);
    assert(fifo_store <= 32);

    uint32_t ret = 0;
    uint8_t buffer;

    dev->data_rate = data_rate;
    dev->range = range;
    dev->fifo_store = fifo_store;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }


    buffer = 0x07 | (data_rate << 4);
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_CTRL1, 1, &buffer, 1);

    if(fifo_store) {
        buffer = 0x04;  // configure ctrl3 for watermark on INT1
    }
    else {
        buffer = 0x10;  // configure ctrl3 for DRDY1 interrupt
    }
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_CTRL3, 1, &buffer, 1);

    buffer = 0x88 + (range << 4);  // configure ctrl4 to allow block data update and high resolution
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_CTRL4, 1, &buffer, 1);

    if(fifo_store) {
        buffer = fifo_store | 0x80; //stream mode, if fifo is enabled
    }
    else {
        buffer = 0x00;  //skip fifo
    }
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_FIFOCTRL, 1, &buffer, 1);


    if(fifo_store) {
        buffer = 0x40;  // configure ctrl5 to enable FIFO
    }
    else {
        buffer = 0x00;  // configure ctrl5 to disable FIFO
    }
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_CTRL5, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t lis3dh_turn_off(lis3dh_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    dev->data_rate = LIS3DH_DATARATE_POWERDOWN;
    buffer = 0x07 | (dev->data_rate << 4);
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_CTRL1, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}


uint32_t lis3dh_read(lis3dh_t *dev, lis3dh_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[6];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    uint8_t sample = 0;
    while(sample <= dev->fifo_store) {
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, LIS3DH_REG_OUT_X_L | 0x80, 1, buffer, 6);
        data[sample].x = (int16_t)(((int16_t)buffer[1] << 8) + buffer[0]);
        data[sample].y = (int16_t)(((int16_t)buffer[3] << 8) + buffer[2]);
        data[sample].z = (int16_t)(((int16_t)buffer[5] << 8) + buffer[4]);
        sample++;
    }

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
