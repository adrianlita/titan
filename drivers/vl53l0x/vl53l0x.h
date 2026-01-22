#pragma once

#include <periph/i2c.h>
#include <mutex.h>

typedef enum {
    VL53L0X_MODE_OFF = 0,
    VL53L0X_MODE_SINGLE = 1,
    VL53L0X_MODE_CONTINUOUS = 2,
} vl53l0x_mode_t;

typedef struct __vl53l0x {
    i2c_t i2c;
    mutex_t *i2c_mutex;

    vl53l0x_mode_t mode;

    uint8_t revision;

    uint8_t stop_variable;
    uint32_t measurement_timing_budget_us;
} vl53l0x_t;

//checkAL - returns might not be concludent. driver needs to be 100% reviewed

uint32_t vl53l0x_init(vl53l0x_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t voltage2v8);
uint32_t vl53l0x_turn_on(vl53l0x_t *dev, vl53l0x_mode_t mode, uint32_t cont_period_ms, uint16_t limit_Mcps, uint32_t timing_budget, uint8_t prerange_period_pclks, uint8_t finalrange_period_pclks);
uint32_t vl53l0x_turn_on_default(vl53l0x_t *dev, vl53l0x_mode_t mode, uint32_t cont_period_ms);
uint32_t vl53l0x_turn_off(vl53l0x_t *dev);

uint32_t vl53l0x_single_read_trigger(vl53l0x_t *dev);
uint32_t vl53l0x_read(vl53l0x_t *dev, uint16_t *mm);

/*
    usage:

    #include <vl53l0x/vl53l0x.h>

    vl53l0x_t vl53l0x;
    uint16_t mm;

    gpio_init_digital(VL53L0X_XSHUT, GPIO_MODE_OUTPUT_PP, GPIO_PULL_UP);
    gpio_digital_write(VL53L0X_XSHUT, 1);  //turn vl53l0x on
    task_sleep(50);   //allow it to turn on

    ret = vl53l0x_init(&vl53l0x, (i2c_t)I2C3, &i2c3_mutex, 1);
    
    vl53l0x_turn_on_default(&vl53l0x, VL53L0X_MODE_CONTINUOUS, 0);
    gpio_init_interrupt(VL53L0X_GPIO1, GPIO_MODE_INTERRUPT_FEDGE, GPIO_PULL_UP, vl53_on_int);

    on int: vl53l0x_read(&vl53l0x, &mm);
*/
