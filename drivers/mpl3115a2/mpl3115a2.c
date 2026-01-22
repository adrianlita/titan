#include "mpl3115a2.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define MPL3115A2_ADDRESS                       0xC0
#define MPL3115A2_WHOAMI                        0xC4
#define MPL3115A2_REGISTER_STATUS               0x00
#define MPL3115A2_REGISTER_STATUS_TDR           0x02
#define MPL3115A2_REGISTER_STATUS_PDR           0x04
#define MPL3115A2_REGISTER_STATUS_PTDR          0x08
#define MPL3115A2_REGISTER_PRESSURE_MSB         0x01
#define MPL3115A2_REGISTER_PRESSURE_CSB         0x02
#define MPL3115A2_REGISTER_PRESSURE_LSB         0x03
#define MPL3115A2_REGISTER_TEMP_MSB             0x04
#define MPL3115A2_REGISTER_TEMP_LSB             0x05
#define MPL3115A2_REGISTER_DR_STATUS            0x06
#define MPL3115A2_OUT_P_DELTA_MSB               0x07
#define MPL3115A2_OUT_P_DELTA_CSB               0x08
#define MPL3115A2_OUT_P_DELTA_LSB               0x09
#define MPL3115A2_OUT_T_DELTA_MSB               0x0A
#define MPL3115A2_OUT_T_DELTA_LSB               0x0B
#define MPL3115A2_PT_DATA_CFG                   0x13
#define MPL3115A2_PT_DATA_CFG_TDEFE             0x01
#define MPL3115A2_PT_DATA_CFG_PDEFE             0x02
#define MPL3115A2_PT_DATA_CFG_DREM              0x04
#define MPL3115A2_CTRL_REG1                     0x26
#define MPL3115A2_CTRL_REG1_SBYB                0x01
#define MPL3115A2_CTRL_REG1_OST                 0x02
#define MPL3115A2_CTRL_REG1_RST                 0x04
#define MPL3115A2_CTRL_REG1_OS1                 0x00
#define MPL3115A2_CTRL_REG1_OS2                 0x08
#define MPL3115A2_CTRL_REG1_OS4                 0x10
#define MPL3115A2_CTRL_REG1_OS8                 0x18
#define MPL3115A2_CTRL_REG1_OS16                0x20
#define MPL3115A2_CTRL_REG1_OS32                0x28
#define MPL3115A2_CTRL_REG1_OS64                0x30
#define MPL3115A2_CTRL_REG1_OS128               0x38
#define MPL3115A2_CTRL_REG1_RAW                 0x40
#define MPL3115A2_CTRL_REG1_ALT                 0x80
#define MPL3115A2_CTRL_REG1_BAR                 0x00
#define MPL3115A2_CTRL_REG2                     0x27
#define MPL3115A2_CTRL_REG3                     0x28
#define MPL3115A2_CTRL_REG3_INT1_ACTIVE_HIGH    0x20
#define MPL3115A2_CTRL_REG4                     0x29
#define MPL3115A2_CTRL_REG4_DRDY                0x80
#define MPL3115A2_CTRL_REG4_PCHG                0x02
#define MPL3115A2_CTRL_REG4_TCHG                0x01
#define MPL3115A2_CTRL_REG5                     0x2A
#define MPL3115A2_REGISTER_STARTCONVERSION      0x12
#define MPL3115A2_REGISTER_WHOAMI               0x0C

uint32_t mpl3115a2_init(mpl3115a2_t *dev, i2c_t i2c, mutex_t *i2c_mutex) {
    assert(dev != 0);

    uint8_t whoami = 0;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    dev->ctrl1 = 0;
    dev->ctrl_ptdata = 0;
    dev->ctrl2 = 0;
    dev->ctrl3 = 0;
    dev->ctrl4 = 0;
    dev->ctrl5  = 0;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_REGISTER_WHOAMI, 1, (void*)&whoami, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if(whoami != MPL3115A2_WHOAMI) {
        ret = ENODEV;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t mpl3115a2_turn_on(mpl3115a2_t *dev, mpl3115a2_period_t period) {
    assert(dev != 0);

    uint32_t ret = 0;

    dev->ctrl1 = MPL3115A2_CTRL_REG1_SBYB | MPL3115A2_CTRL_REG1_OS128 | MPL3115A2_CTRL_REG1_BAR; //configure ctrl1 for oversampling at 128 samples and active mode. mpl3115a2 or altimeter by selection
    dev->ctrl_ptdata = MPL3115A2_PT_DATA_CFG_TDEFE | MPL3115A2_PT_DATA_CFG_PDEFE | MPL3115A2_PT_DATA_CFG_DREM; //configure pressure/time for events when new data on: temperature, pressure/altitude.
    dev->ctrl2 = (uint8_t)period;
    dev->ctrl3 = MPL3115A2_CTRL_REG3_INT1_ACTIVE_HIGH; //configure INT1 to signal dataready
    dev->ctrl4 = MPL3115A2_CTRL_REG4_DRDY;
    dev->ctrl5  = MPL3115A2_CTRL_REG4_DRDY;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret += i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_CTRL_REG1, 1, &dev->ctrl1, 1);
    ret += i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_PT_DATA_CFG, 1, &dev->ctrl_ptdata, 1);
    ret += i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_CTRL_REG2, 1, &dev->ctrl2, 1);
    ret += i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_CTRL_REG3, 1, &dev->ctrl3, 1);
    ret += i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_CTRL_REG4, 1, &dev->ctrl4, 1);
    ret += i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_CTRL_REG5, 1, &dev->ctrl5, 1);

    if(ret != 0) {
        ret = EIO;
    }
    
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t mpl3115a2_turn_off(mpl3115a2_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    dev->ctrl1 = MPL3115A2_CTRL_REG1_OS128 | MPL3115A2_CTRL_REG1_BAR;  //SBYB is zero
    ret = i2c_master_write_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_CTRL_REG1, 1, &dev->ctrl1, 1);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    if(ret != 0) {
        ret = EIO;
    }

    return ret;
}

uint32_t mpl3115a2_read(mpl3115a2_t *dev, mpl3115a2_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint8_t current_register_raw[6];
    uint32_t ret = 0;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, MPL3115A2_ADDRESS, MPL3115A2_REGISTER_STATUS, 1, (void*)current_register_raw, 6);
    if(ret != 0) {
        ret = EIO;
    }
    else {
        if(current_register_raw[0] & MPL3115A2_REGISTER_STATUS_PDR) {
            data->pressure = 0;
            data->pressure = (int8_t)current_register_raw[1];
            data->pressure = data->pressure << 8;
            data->pressure += current_register_raw[2];
            data->pressure = data->pressure << 8;
            data->pressure += current_register_raw[3];
        }

        if(current_register_raw[0] & MPL3115A2_REGISTER_STATUS_TDR) {
            data->temperature = (int16_t)(current_register_raw[4] << 8) + current_register_raw[5];
        }
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
