#pragma once

#include <periph/i2c.h>
#include <mutex.h>

//MPU9250 = MPU6500 + AK8963

typedef enum {
    MPU9250_ACC_RANGE_2G = 0,
    MPU9250_ACC_RANGE_4G = 1,
    MPU9250_ACC_RANGE_8G = 2,
    MPU9250_ACC_RANGE_16G = 3,
} mpu9250_accelerometer_range_t;

typedef enum {
    MPU9250_GYRO_RANGE_250DPS = 0,
    MPU9250_GYRO_RANGE_500DPS = 1,
    MPU9250_GYRO_RANGE_1000DPS = 2,
    MPU9250_GYRO_RANGE_2000DPS = 3,
} mpu9250_gyroscope_range_t;

typedef enum {
    MPU9250_DLPF_RATE_250_HZ = 0,  //sample rate = 8KHz
    MPU9250_DLPF_RATE_184_HZ = 1,  //sample rate = 1KHz
    MPU9250_DLPF_RATE_92_HZ = 2,   //sample rate = 1KHz
    MPU9250_DLPF_RATE_41_HZ = 3,   //sample rate = 1KHz
    MPU9250_DLPF_RATE_20_HZ = 4,   //sample rate = 1KHz
    MPU9250_DLPF_RATE_10_HZ = 5,   //sample rate = 1KHz
    MPU9250_DLPF_RATE_5_HZ = 6,    //sample rate = 1KHz
} mpu9250_dlpf_rate_t;

typedef struct __mpu6500_data {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} mpu6500_data_t;

typedef struct __ak8963_data {
    int16_t mag_x;
    int16_t mag_y;
    int16_t mag_z;
} ak8963_data_t;

typedef struct __mpu9250 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    mpu9250_accelerometer_range_t acc_range;
    mpu9250_gyroscope_range_t gyro_range;
    uint8_t sample_rate_divider;
    mpu9250_dlpf_rate_t dlpf_rate;

    /*
    use coeff to adjust sensitivity:
    mag_adj = mag * (((coeff - 128) * 0.5) / 128 + 1)
    */
    uint8_t mag_coeff_x;
    uint8_t mag_coeff_y;
    uint8_t mag_coeff_z;
} mpu9250_t;

uint32_t mpu9250_init(mpu9250_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t ad0_pin_value);
uint32_t mpu9250_turn_on(mpu9250_t *dev, mpu9250_accelerometer_range_t acc_range, mpu9250_gyroscope_range_t gyro_range, uint8_t sample_rate_divider, mpu9250_dlpf_rate_t dlpf_rate);
uint32_t mpu9250_turn_off(mpu9250_t *dev);

uint32_t mpu9250_read_mpu6500(mpu9250_t *dev, mpu6500_data_t *data);
uint32_t mpu9250_read_ak8963(mpu9250_t *dev, ak8963_data_t *data);
/*
    usage:

    #include <mpu9250/mpu9250.h>

    mpu9250_t mpu9250;
    mpu6500_data_t mpu_data;
    ak8963_data_t ak_data;

    ret = mpu9250_init(&mpu9250, (i2c_t)I2C3, &i2c3_mutex, 0);

    mpu9250_turn_on(&mpu9250, MPU9250_ACC_RANGE_2G, MPU9250_GYRO_RANGE_250DPS, 10, MPU9250_DLPF_RATE_20_HZ);
    gpio_init_interrupt(MPU9250_INT, GPIO_MODE_INTERRUPT_REDGE, GPIO_PULL_NOPULL, mpu9250_on_int);

    on interrupt:
    mpu9250_read_mpu6500(&mpu9250, &mpu_data);
    mpu9250_read_ak8963(&mpu9250, &ak_data);
    explorer_plot16(0, mpu_data.acc_x);
    explorer_plot16(1, mpu_data.acc_y);
    explorer_plot16(2, mpu_data.acc_z);

    explorer_plot16(3, mpu_data.gyro_x);
    explorer_plot16(4, mpu_data.gyro_y);
    explorer_plot16(5, mpu_data.gyro_z);

    explorer_plot16(6, ak_data.mag_x);
    explorer_plot16(7, ak_data.mag_y);
    explorer_plot16(8, ak_data.mag_z);
*/
