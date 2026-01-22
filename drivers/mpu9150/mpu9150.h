#pragma once

#include <periph/i2c.h>
#include <mutex.h>

//MPU9150 = MPU6000 + AK8975A

typedef enum {
  MPU9150_ACC_RANGE_2G = 0,
  MPU9150_ACC_RANGE_4G = 1,
  MPU9150_ACC_RANGE_8G = 2,
  MPU9150_ACC_RANGE_16G = 3,
} mpu9150_accelerometer_range_t;

typedef enum {
  MPU9150_GYRO_RANGE_250DPS = 0,
  MPU9150_GYRO_RANGE_500DPS = 1,
  MPU9150_GYRO_RANGE_1000DPS = 2,
  MPU9150_GYRO_RANGE_2000DPS = 3,
} mpu9150_gyroscope_range_t;

typedef enum {
  MPU9150_DLPF_RATE_256_HZ = 0,  //sample rate = 8KHz
  MPU9150_DLPF_RATE_188_HZ = 1,  //sample rate = 1KHz
  MPU9150_DLPF_RATE_98_HZ = 2,   //sample rate = 1KHz
  MPU9150_DLPF_RATE_42_HZ = 3,   //sample rate = 1KHz
  MPU9150_DLPF_RATE_20_HZ = 4,   //sample rate = 1KHz
  MPU9150_DLPF_RATE_10_HZ = 5,   //sample rate = 1KHz
  MPU9150_DLPF_RATE_5_HZ = 6,    //sample rate = 1KHz
} mpu9150_dlpf_rate_t;

typedef struct __mpu6000_data {
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;

    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} mpu6000_data_t;

typedef struct __ak8975_data {
    int16_t mag_x;
    int16_t mag_y;
    int16_t mag_z;
} ak8975_data_t;

typedef struct __mpu9150 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    mpu9150_accelerometer_range_t acc_range;
    mpu9150_gyroscope_range_t gyro_range;
    uint8_t sample_rate_divider;
    mpu9150_dlpf_rate_t dlpf_rate;

    uint8_t self_test_x;
    uint8_t self_test_y;
    uint8_t self_test_z;
    uint8_t self_test_a;

    /*
    use coeff to adjust sensitivity:
    mag_adj = mag * (((coeff - 128) * 0.5) / 128 + 1)
    */
    uint8_t mag_coeff_x;
    uint8_t mag_coeff_y;
    uint8_t mag_coeff_z;
} mpu9150_t;

uint32_t mpu9150_init(mpu9150_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t ad0_pin_value);
uint32_t mpu9150_turn_on(mpu9150_t *dev, mpu9150_accelerometer_range_t acc_range, mpu9150_gyroscope_range_t gyro_range, uint8_t sample_rate_divider, mpu9150_dlpf_rate_t dlpf_rate);
uint32_t mpu9150_turn_off(mpu9150_t *dev);

uint32_t mpu9150_read_mpu6000(mpu9150_t *dev, mpu6000_data_t *data);
uint32_t mpu9150_read_ak8975(mpu9150_t *dev, ak8975_data_t *data);

/*
    usage:

    #include <mpu9150/mpu9150.h>

    mpu9150_t mpu9150;
    mpu6000_data_t mpu_data;
    ak8975_data_t ak_data;

    ret = mpu9150_init(&mpu9150, (i2c_t)I2C3, &i2c3_mutex, 1);

    mpu9150_turn_on(&mpu9150, MPU9150_ACC_RANGE_2G, MPU9150_GYRO_RANGE_250DPS, 10, MPU9150_DLPF_RATE_10_HZ);
    gpio_init_interrupt(MPU9150_INT, GPIO_MODE_INTERRUPT_REDGE, GPIO_PULL_NOPULL, mpu9150_on_int);

    on interrupt:
    mpu9150_read_mpu6000(&mpu9150, &mpu_data);
    mpu9150_read_ak8975(&mpu9150, &ak_data);
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
