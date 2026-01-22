#include "adxl375.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define ADXL375_I2C_ADDRESS_SDO_0       0xA6
#define ADXL375_I2C_ADDRESS_SDO_1       0x3A


#define ADXL375_REG_WHO_AM_I            0x00    // Device ID

#define ADXL375_REG_THRESH_SHOCK        0x1D    // Shock threshold
#define ADXL375_REG_OFSX                0x1E    // X-axis offset
#define ADXL375_REG_OFSY                0x1F    // Y-axis offset
#define ADXL375_REG_OFSZ                0x20    // Z-axis offset
#define ADXL375_REG_DUR                 0x21    // Shock duration
#define ADXL375_REG_LATENT              0x22    // Shock latency
#define ADXL375_REG_WINDOW              0x23    // Tap window
#define ADXL375_REG_THRESH_ACT          0x24    // Activity threshold
#define ADXL375_REG_THRESH_INACT        0x25    // Inactivity threshold
#define ADXL375_REG_TIME_INACT          0x26    // Inactivity time
#define ADXL375_REG_ACT_INACT_CTL       0x27    // Axis enable control for activity and inactivity detection
#define ADXL375_REG_SHOCK_AXES          0x2A    // Axis control for single/double tap
#define ADXL375_REG_ACT_SHOCK_STATUS    0x2B    // Source for single/double tap
#define ADXL375_REG_BW_RATE             0x2C    // Data rate and power mode control
#define ADXL375_REG_POWER_CTL           0x2D    // Power-saving features control
#define ADXL375_REG_INT_ENABLE          0x2E    // Interrupt enable control
#define ADXL375_REG_INT_MAP             0x2F    // Interrupt mapping control
#define ADXL375_REG_INT_SOURCE          0x30    // Source of interrupts
#define ADXL375_REG_DATA_FORMAT         0x31    // Data format control
#define ADXL375_REG_DATAX0              0x32    // X-axis data 0
#define ADXL375_REG_DATAX1              0x33    // X-axis data 1
#define ADXL375_REG_DATAY0              0x34    // Y-axis data 0
#define ADXL375_REG_DATAY1              0x35    // Y-axis data 1
#define ADXL375_REG_DATAZ0              0x36    // Z-axis data 0
#define ADXL375_REG_DATAZ1              0x37    // Z-axis data 1
#define ADXL375_REG_FIFO_CTL            0x38    // FIFO control
#define ADXL375_REG_FIFO_STATUS         0x39    // FIFO status

#define ADXL375_WHO_AM_I                0xE5

uint32_t adxl375_init(adxl375_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t sdo_pin_value) {
    assert(dev != 0);

    uint8_t whoami = 0;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    if(sdo_pin_value) {
        dev->i2c_address = ADXL375_I2C_ADDRESS_SDO_1;
    }
    else {
        dev->i2c_address = ADXL375_I2C_ADDRESS_SDO_0;
    }

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, ADXL375_REG_WHO_AM_I, 1, (void*)&whoami, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if(whoami != ADXL375_WHO_AM_I) {
        ret = ENODEV;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t adxl375_turn_on(adxl375_t *dev, adxl375_data_rate_t data_rate, uint8_t fifo_store) {
    assert(dev != 0);
    assert(fifo_store <= 32);

    uint32_t ret = 0;
    uint8_t buffer;

    dev->data_rate = data_rate;
    dev->fifo_store = fifo_store;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x0B; //left justified, no self test, no SPI, interrupt active-high
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_DATA_FORMAT, 1, &buffer, 1);

    if(fifo_store) {
        buffer = fifo_store | 0x80; //stream mode, if fifo is enabled
    }
    else {
        buffer = 0x00;  //skip fifo
    }
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_FIFO_CTL, 1, &buffer, 1);

    buffer = 0x00; //map all interrupts on INT1
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_INT_MAP, 1, &buffer, 1);

    if(fifo_store) {
        buffer = 0x02;  //enable int for watermark
    }
    else {
        buffer = 0x80; //enable int for data ready
    }
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_INT_ENABLE, 1, &buffer, 1);

    buffer = 0x00 + data_rate; //set data rate
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_BW_RATE, 1, &buffer, 1);

    buffer = 0x08; //turn on measure; no sleep
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_POWER_CTL, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t adxl375_turn_off(adxl375_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x04; //turn off measure; sleep
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, ADXL375_REG_POWER_CTL, 1, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}


uint32_t adxl375_read(adxl375_t *dev, adxl375_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[6];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    uint8_t sample = 0;
    while(sample <= dev->fifo_store) {
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, ADXL375_REG_DATAX0, 1, buffer, 6);
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
