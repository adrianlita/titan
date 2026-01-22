#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum __pac1934_sample_rate {
    PAC1934_SAMPLE_RATE_0HZ_SLEEP = 5,
    PAC1934_SAMPLE_RATE_0HZ_ONE_SHOT = 4,
    PAC1934_SAMPLE_RATE_8HZ = 3,
    PAC1934_SAMPLE_RATE_64HZ = 2,
    PAC1934_SAMPLE_RATE_256HZ = 1,
    PAC1934_SAMPLE_RATE_1024HZ = 0,
} pac1934_sample_rate_t;

typedef enum {
    PAC1934_CHANNEL_OFF = 0,
    PAC1934_CHANNEL_UNI_VI = 1,   //unidirectional voltage and current
    PAC1934_CHANNEL_BIDI_V = 2,   //bidirectional voltage, unidirectional current
    PAC1934_CHANNEL_BIDI_I = 3,   //unidirectional voltage, bidirectional current
    PAC1934_CHANNEL_BIDI_VI = 4,  //bidirectional voltage and current
} pac1934_channel_config_t;

typedef struct __pac1934_channel_data {
    int64_t vpower_acc;
    int32_t vbus;
    int32_t vsense;
    int32_t vbus_avg;
    int32_t vsense_avg;
    int32_t vpower;
} pac1934_channel_data_t;

typedef enum {
    PAC1934_ADDR_RES0 = 0,
    PAC1934_ADDR_RES499 = 499,
    PAC1934_ADDR_RES806 = 806,
    PAC1934_ADDR_RES1270 = 1270,
    PAC1934_ADDR_RES2050 = 2050,
    PAC1934_ADDR_RES3240 = 3240,
    PAC1934_ADDR_RES5230 = 5230,
    PAC1934_ADDR_RES8450 = 8450,
    PAC1934_ADDR_RES13300 = 13300,
    PAC1934_ADDR_RES21500 = 21500,
    PAC1934_ADDR_RES34000 = 34000,
    PAC1934_ADDR_RES54900 = 54900,
    PAC1934_ADDR_RES88700 = 88700,
    PAC1934_ADDR_RES140000 = 140000,
    PAC1934_ADDR_RES226000 = 226000,
    PAC1934_ADDR_RESNONE = 1000000,
} pac1934_addr_res_t;

typedef enum {
    PAC1934_ALERT_TYPE_DISABLE = 0,
    PAC1934_ALERT_TYPE_CYCLE_COMPLETE = 1,
    PAC1934_ALERT_TYPE_ACC_OVF = 2,
} pac1934_alert_type_t;

typedef struct __pac1934 {
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    uint8_t revision;

    pac1934_alert_type_t alert_type;
    pac1934_sample_rate_t sample_rate;
    pac1934_channel_config_t channel_config[4];
} pac1934_t;

uint32_t pac1934_init(pac1934_t *dev, i2c_t i2c, mutex_t *i2c_mutex, pac1934_addr_res_t addr_res);
uint32_t pac1934_turn_on(pac1934_t *dev, pac1934_alert_type_t alert_type, pac1934_sample_rate_t sample_rate, pac1934_channel_config_t channel_config[4]);
uint32_t pac1934_turn_off(pac1934_t *dev);

uint32_t pac1934_refresh(pac1934_t *dev);           //resets and updates registers
uint32_t pac1934_refresh_v(pac1934_t *dev);         //updates registers without resetting
uint32_t pac1934_read(pac1934_t *dev, uint32_t *accumulator_count, pac1934_channel_data_t channel[4]);  //read all data

/*
    usage:

    #include <pac1934/pac1934.h>

    pac1934_t pac1934;
    pac1934_channel_config_t pac_config[4] = {PAC1934_CHANNEL_BIDI_VI, PAC1934_CHANNEL_BIDI_VI, PAC1934_CHANNEL_BIDI_VI, PAC1934_CHANNEL_BIDI_VI};
    uint32_t acc_count;
    pac1934_channel_data_t pac_data[4];
    
    gpio_init_digital(PAC1934_nPWRDN, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    gpio_init_interrupt(PAC1934_INT, GPIO_MODE_INTERRUPT_FEDGE, GPIO_PULL_NOPULL, pac_on_int);

    gpio_digital_write(PAC1934_nPWRDN, 1);  //turn pac1934 on
    task_sleep(50);   //allow it to power up

    ret = pac1934_init(&pac1934, (i2c_t)I2C3, &i2c3_mutex, PAC1934_ADDR_RES0);
    pac1934_turn_on(&pac1934, PAC1934_SAMPLE_RATE_8HZ, pac_config);

    on int: pac1934_read(&pac1934, &acc_count, pac_data);
*/
