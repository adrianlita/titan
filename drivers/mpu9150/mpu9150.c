#include "mpu9150.h"
#include <assert.h>
#include <error.h>
#include <task.h>

TITAN_DEBUG_FILE_MARK;

#define MPU6000_I2C_ADDRESS                     0xD0
#define AK8975A_I2C_ADDRESS                     0x18

#define MPU6000_REG_XGOFFS_TC                   0x00
#define MPU6000_REG_YGOFFS_TC                   0x01
#define MPU6000_REG_ZGOFFS_TC                   0x02
#define MPU6000_REG_X_FINE_GAIN                 0x03
#define MPU6000_REG_Y_FINE_GAIN                 0x04
#define MPU6000_REG_Z_FINE_GAIN                 0x05
#define MPU6000_REG_XA_OFFSET_H                 0x06
#define MPU6000_REG_XA_OFFSET_L_TC              0x07
#define MPU6000_REG_YA_OFFSET_H                 0x08
#define MPU6000_REG_YA_OFFSET_L_TC              0x09
#define MPU6000_REG_ZA_OFFSET_H                 0x0A
#define MPU6000_REG_ZA_OFFSET_L_TC              0x0B
#define MPU6000_REG_SELF_TEST_X                 0x0D
#define MPU6000_REG_SELF_TEST_Y                 0x0E
#define MPU6000_REG_SELF_TEST_Z                 0x0F
#define MPU6000_REG_SELF_TEST_A                 0x10
#define MPU6000_REG_XG_OFFS_USRH                0x13
#define MPU6000_REG_XG_OFFS_USRL                0x14
#define MPU6000_REG_YG_OFFS_USRH                0x15
#define MPU6000_REG_YG_OFFS_USRL                0x16
#define MPU6000_REG_ZG_OFFS_USRH                0x17
#define MPU6000_REG_ZG_OFFS_USRL                0x18
#define MPU6000_REG_SMPLRT_DIV                  0x19
#define MPU6000_REG_CONFIG                      0x1A
#define MPU6000_REG_GYRO_CONFIG                 0x1B
#define MPU6000_REG_ACCEL_CONFIG                0x1C
#define MPU6000_REG_FF_THR                      0x1D
#define MPU6000_REG_FF_DUR                      0x1E
#define MPU6000_REG_MOT_THR                     0x1F
#define MPU6000_REG_MOT_DUR                     0x20
#define MPU6000_REG_ZMOT_THR                    0x21
#define MPU6000_REG_ZRMOT_DUR                   0x22
#define MPU6000_REG_FIFO_EN                     0x23
#define MPU6000_REG_I2C_MST_CTRL                0x24
#define MPU6000_REG_I2C_SLV0_ADDR               0x25
#define MPU6000_REG_I2C_SLV0_REG                0x26
#define MPU6000_REG_I2C_SLV0_CTRL               0x27
#define MPU6000_REG_I2C_SLV1_ADDR               0x28
#define MPU6000_REG_I2C_SLV1_REG                0x29
#define MPU6000_REG_I2C_SLV1_CTRL               0x2A
#define MPU6000_REG_I2C_SLV2_ADDR               0x2B
#define MPU6000_REG_I2C_SLV2_REG                0x2C
#define MPU6000_REG_I2C_SLV2_CTRL               0x2D
#define MPU6000_REG_I2C_SLV3_ADDR               0x2E
#define MPU6000_REG_I2C_SLV3_REG                0x2F
#define MPU6000_REG_I2C_SLV3_CTRL               0x30
#define MPU6000_REG_I2C_SLV4_ADDR               0x31
#define MPU6000_REG_I2C_SLV4_REG                0x32
#define MPU6000_REG_I2C_SLV4_DO                 0x33
#define MPU6000_REG_I2C_SLV4_CTRL               0x34
#define MPU6000_REG_I2C_SLV4_DI                 0x35
#define MPU6000_REG_I2C_MST_STATUS              0x36
#define MPU6000_REG_INT_PIN_CFG                 0x37
#define MPU6000_REG_INT_ENABLE                  0x38
#define MPU6000_REG_DMP_INT_STATUS              0x39
#define MPU6000_REG_INT_STATUS                  0x3A
#define MPU6000_REG_ACCEL_XOUT_H                0x3B
#define MPU6000_REG_ACCEL_XOUT_L                0x3C
#define MPU6000_REG_ACCEL_YOUT_H                0x3D
#define MPU6000_REG_ACCEL_YOUT_L                0x3E
#define MPU6000_REG_ACCEL_ZOUT_H                0x3F
#define MPU6000_REG_ACCEL_ZOUT_L                0x40
#define MPU6000_REG_TEMP_OUT_H                  0x41
#define MPU6000_REG_TEMP_OUT_L                  0x42
#define MPU6000_REG_GYRO_XOUT_H                 0x43
#define MPU6000_REG_GYRO_XOUT_L                 0x44
#define MPU6000_REG_GYRO_YOUT_H                 0x45
#define MPU6000_REG_GYRO_YOUT_L                 0x46
#define MPU6000_REG_GYRO_ZOUT_H                 0x47
#define MPU6000_REG_GYRO_ZOUT_L                 0x48
#define MPU6000_REG_EXT_SENS_DATA_00            0x49
#define MPU6000_REG_EXT_SENS_DATA_01            0x4A
#define MPU6000_REG_EXT_SENS_DATA_02            0x4B
#define MPU6000_REG_EXT_SENS_DATA_03            0x4C
#define MPU6000_REG_EXT_SENS_DATA_04            0x4D
#define MPU6000_REG_EXT_SENS_DATA_05            0x4E
#define MPU6000_REG_EXT_SENS_DATA_06            0x4F
#define MPU6000_REG_EXT_SENS_DATA_07            0x50
#define MPU6000_REG_EXT_SENS_DATA_08            0x51
#define MPU6000_REG_EXT_SENS_DATA_09            0x52
#define MPU6000_REG_EXT_SENS_DATA_10            0x53
#define MPU6000_REG_EXT_SENS_DATA_11            0x54
#define MPU6000_REG_EXT_SENS_DATA_12            0x55
#define MPU6000_REG_EXT_SENS_DATA_13            0x56
#define MPU6000_REG_EXT_SENS_DATA_14            0x57
#define MPU6000_REG_EXT_SENS_DATA_15            0x58
#define MPU6000_REG_EXT_SENS_DATA_16            0x59
#define MPU6000_REG_EXT_SENS_DATA_17            0x5A
#define MPU6000_REG_EXT_SENS_DATA_18            0x5B
#define MPU6000_REG_EXT_SENS_DATA_19            0x5C
#define MPU6000_REG_EXT_SENS_DATA_20            0x5D
#define MPU6000_REG_EXT_SENS_DATA_21            0x5E
#define MPU6000_REG_EXT_SENS_DATA_22            0x5F
#define MPU6000_REG_EXT_SENS_DATA_23            0x60
#define MPU6000_REG_MOT_DETECT_STATUS           0x61
#define MPU6000_REG_I2C_SLV0_DO                 0x63
#define MPU6000_REG_I2C_SLV1_DO                 0x64
#define MPU6000_REG_I2C_SLV2_DO                 0x65
#define MPU6000_REG_I2C_SLV3_DO                 0x66
#define MPU6000_REG_I2C_MST_DELAY_CTRL          0x67
#define MPU6000_REG_SIGNAL_PATH_RESET           0x68
#define MPU6000_REG_MOT_DETECT_CTRL             0x69
#define MPU6000_REG_USER_CTRL                   0x6A
#define MPU6000_REG_PWR_MGMT_1                  0x6B
#define MPU6000_REG_PWR_MGMT_2                  0x6C
#define MPU6000_REG_DMP_BANK                    0x6D
#define MPU6000_REG_DMP_RW_PNT                  0x6E
#define MPU6000_REG_DMP_REG                     0x6F
#define MPU6000_REG_DMP_REG_1                   0x70
#define MPU6000_REG_DMP_REG_2                   0x71
#define MPU6000_REG_FIFO_COUNTH                 0x72
#define MPU6000_REG_FIFO_COUNTL                 0x73
#define MPU6000_REG_FIFO_R_W                    0x74
#define MPU6000_REG_WHO_AM_I                    0x75

#define AK8975A_REG_WHO_AM_I                    0x00
#define AK8975A_REG_INFO                        0x01
#define AK8975A_REG_ST1                         0x02
#define AK8975A_REG_XOUT_L                      0x03
#define AK8975A_REG_XOUT_H                      0x04
#define AK8975A_REG_YOUT_L                      0x05
#define AK8975A_REG_YOUT_H                      0x06
#define AK8975A_REG_ZOUT_L                      0x07
#define AK8975A_REG_ZOUT_H                      0x08
#define AK8975A_REG_ST2                         0x09
#define AK8975A_REG_CNTL                        0x0A
#define AK8975A_REG_ASTC                        0x0C
#define AK8975A_REG_ASAX                        0x10
#define AK8975A_REG_ASAY                        0x11
#define AK8975A_REG_ASAZ                        0x12

#define MPU6000_WHO_AM_I                        0x68
#define AK8975A_WHO_AM_I                        0x48

uint32_t mpu9150_init(mpu9150_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t ad0_pin_value) {
    assert(dev != 0);

    uint8_t buffer = 0;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    dev->i2c_address = MPU6000_I2C_ADDRESS;
    if(ad0_pin_value) {
        dev->i2c_address += 0x02;
    }

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_WHO_AM_I, 1, &buffer, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if(buffer != MPU6000_WHO_AM_I) {
        ret = ENODEV;
    }
    else {
        buffer = 0x00;
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_I2C_MST_CTRL, 1, &buffer, 1);

        buffer = 0x32;  //enable bypass to secondary i2c, and enable interrupt latch and clear on any read
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_INT_PIN_CFG, 1, &buffer, 1);

        ret += i2c_master_read_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_WHO_AM_I, 1, &buffer, 1);
        if(ret != 0) {
            ret = EIO;
        }
        else
        if(buffer != AK8975A_WHO_AM_I) {
            ret = ENODEV;
        }
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t mpu9150_turn_on(mpu9150_t *dev, mpu9150_accelerometer_range_t acc_range, mpu9150_gyroscope_range_t gyro_range, uint8_t sample_rate_divider, mpu9150_dlpf_rate_t dlpf_rate) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;
    uint8_t buffer2[6];
    uint8_t status;

    dev->acc_range = acc_range;
    dev->gyro_range = gyro_range;
    dev->sample_rate_divider = sample_rate_divider;
    dev->dlpf_rate = dlpf_rate;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x80;  //reset
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_PWR_MGMT_1, 1, &buffer, 1);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    task_sleep(10);   //checkAL

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x07;  //reset signal path
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_SIGNAL_PATH_RESET, 1, &buffer, 1);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    task_sleep(10);   //checkAL

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x01;  //turn on
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_PWR_MGMT_1, 1, &buffer, 1);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    task_sleep(10);   //checkAL

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0xE0 | (acc_range << 3); //self test
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_ACCEL_CONFIG, 1, &buffer, 1);

    buffer = 0xE0 | (gyro_range << 3); //self test
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_GYRO_CONFIG, 1, &buffer, 1);
    
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    task_sleep(25);   //checkAL

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_SELF_TEST_X, 1, &dev->self_test_x, 1);

    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_SELF_TEST_Y, 1, &dev->self_test_y, 1);

    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_SELF_TEST_Z, 1, &dev->self_test_z, 1);

    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_SELF_TEST_A, 1, &dev->self_test_a, 1);

    buffer = 0x00 + sample_rate_divider;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_SMPLRT_DIV, 1, &buffer, 1);

    buffer = 0x00 + dlpf_rate;   //filter
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_CONFIG, 1, &buffer, 1);

    buffer = gyro_range << 3;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_GYRO_CONFIG, 1, &buffer, 1);

    buffer = acc_range << 3;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_ACCEL_CONFIG, 1, &buffer, 1);

    buffer = 0x00;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_I2C_MST_CTRL, 1, &buffer, 1);

    buffer = 0x32;  //enable bypass to secondary i2c, and enable interrupt latch and clear on any read
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_INT_PIN_CFG, 1, &buffer, 1);

    buffer = 0x01;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_INT_ENABLE, 1, &buffer, 1);

    buffer = 0x00;  //turn off
    ret += i2c_master_write_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_CNTL, 1, &buffer, 1);

    buffer = 0x0F;  //fuse ROM
    ret += i2c_master_write_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_CNTL, 1, &buffer, 1);

    ret += i2c_master_read_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_ASAX, 1, &dev->mag_coeff_x, 1);
    ret += i2c_master_read_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_ASAY, 1, &dev->mag_coeff_y, 1);
    ret += i2c_master_read_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_ASAZ, 1, &dev->mag_coeff_z, 1);

    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_INT_STATUS, 1, &status, 1);
    
    buffer = 0x00;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_USER_CTRL, 1, &buffer, 1);
    
    if(status & 0x01) {
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_ACCEL_XOUT_H, 1, buffer2, 6);
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_GYRO_XOUT_H, 1, buffer2, 6);
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_TEMP_OUT_H, 1, buffer2, 2);
    }

    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t mpu9150_turn_off(mpu9150_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = 0x00;  //turn off
    ret += i2c_master_write_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_CNTL, 1, &buffer, 1);

    buffer = 0x4F; //turn off
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, MPU6000_REG_PWR_MGMT_1, 1, &buffer, 1);
    
    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}


uint32_t mpu9150_read_mpu6000(mpu9150_t *dev, mpu6000_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[6];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_INT_STATUS, 1, buffer, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else if(buffer[0] & 0x01) {
        //has data
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_ACCEL_XOUT_H, 1, buffer, 6);
        data->acc_x = (int16_t)(((int16_t)buffer[0] << 8) + buffer[1]);
        data->acc_y = (int16_t)(((int16_t)buffer[2] << 8) + buffer[3]);
        data->acc_z = (int16_t)(((int16_t)buffer[4] << 8) + buffer[5]);

        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_GYRO_XOUT_H, 1, buffer, 6);
        data->gyro_x = (int16_t)(((int16_t)buffer[0] << 8) + buffer[1]);
        data->gyro_y = (int16_t)(((int16_t)buffer[2] << 8) + buffer[3]);
        data->gyro_z = (int16_t)(((int16_t)buffer[4] << 8) + buffer[5]);

        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MPU6000_REG_TEMP_OUT_H, 1, buffer, 2);
        //don't care for temperature from here, but:
        /*
            temp = (int16_t)(((int16_t)buffer[0] << 8) + buffer[1]);
            temp = temp + 11900;
            temp = temp / 3.4;
        */
    }
    else {
        ret = EAGAIN;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t mpu9150_read_ak8975(mpu9150_t *dev, ak8975_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[6];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer[0] = 0x01;  //single meas
    ret += i2c_master_write_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_CNTL, 1, buffer, 1);

    buffer[0] = 0;
    while((buffer[0] & 0x01) == 0x00) {
        ret += i2c_master_read_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_ST1, 1, buffer, 1);
    }

    ret += i2c_master_read_mem(dev->i2c, AK8975A_I2C_ADDRESS, AK8975A_REG_XOUT_L, 1, buffer, 6);
    if(ret != 0) {
        ret = EIO;
    }
    else {
        data->mag_x = (int16_t)(((int16_t)buffer[1] << 8) + buffer[0]);
        data->mag_y = (int16_t)(((int16_t)buffer[3] << 8) + buffer[2]);
        data->mag_z = (int16_t)(((int16_t)buffer[5] << 8) + buffer[4]);
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
