#include "am2302.h"
#include <periph/cpu.h>
#include <task.h>
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

static void am2302_isr(am2302_t *dev) {
    assert(dev);

    uint16_t current_timer_value = timer_read(dev->timer);
    uint8_t bit_value = (gpio_digital_read(dev->pin) == 0);
    uint16_t current_value = current_timer_value - dev->old_timer_value;
    dev->old_timer_value = current_timer_value;

    if(dev->measuring_edge > 2) {
        if(bit_value == 1) {
            if(dev->measuring_bit > 31) {
                dev->working_checksum <<= 1;
                dev->working_checksum += (current_value > dev->zero_timing);
            }
            else if(dev->measuring_bit > 15) {
                dev->working_temperature <<= 1;
                dev->working_temperature += (current_value > dev->zero_timing);
            }
            else {
                dev->working_humidity <<= 1;
                dev->working_humidity += (current_value > dev->zero_timing);
            }
            dev->measuring_bit++;
        }
        else {
            dev->zero_timing = current_value;
        }
    }

    dev->measuring_edge++;
}

void am2302_init(am2302_t *dev, gpio_pin_t pin, timer_t timer) {
    assert(dev);

    dev->pin = pin;
    dev->timer = timer;
}

uint8_t am2302_read(am2302_t *dev, am2302_data_t *data) {
    assert(dev);
    assert(data);

    uint8_t valid = 0;

    dev->measuring_edge = 0;
    dev->measuring_bit = 0;
    dev->old_timer_value = 0;
    dev->zero_timing = 0;

    dev->working_temperature = 0;
    dev->working_humidity = 0;
    dev->working_checksum = 0;

    gpio_init_digital(dev->pin, GPIO_MODE_OUTPUT_OD, GPIO_PULL_UP);
    gpio_digital_write(dev->pin, 0);
    task_sleep(3);
    gpio_deinit(dev->pin);
    gpio_init_interrupt_param(dev->pin, GPIO_MODE_INTERRUPT_RFEDGE, GPIO_PULL_UP, (gpio_isr_param_t)am2302_isr, (uint32_t)dev);
    timer_init(dev->timer, 1, 0);
    timer_start(dev->timer, 0xFFFF);
    
    task_sleep(10);
    
    timer_stop(dev->timer);
    timer_deinit(dev->timer);
    gpio_deinit(dev->pin);

    if(dev->measuring_bit == 40) {
        if(dev->working_humidity & 0x8000) {
            dev->working_humidity &= 0x7FFF;
        }
        uint8_t *p = (uint8_t*)&dev->working_humidity;
        uint8_t check = *p + *(p + 1);
        p = (uint8_t*)&dev->working_temperature;
        check += *p + *(p + 1);

        valid = (check == dev->working_checksum);
        data->temperature = dev->working_temperature;
        data->humidity = dev->working_humidity;
    }

    return valid;
}
