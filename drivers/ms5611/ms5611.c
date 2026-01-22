#include "ms5611.h"
#include <assert.h>
#include <error.h>
#include <task.h>

TITAN_DEBUG_FILE_MARK;

#define MS5611_ADDRESS                0xEE

#define MS5611_CMD_ADC_READ           0x00
#define MS5611_CMD_RESET              0x1E
#define MS5611_CMD_CONV_D1            0x40
#define MS5611_CMD_CONV_D2            0x50
#define MS5611_CMD_READ_PROM          0xA0

uint32_t ms5611_init(ms5611_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t csb_pin_value, ms5611_osr_t osr) {
    assert(dev != 0);

    uint8_t buffer[2];
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;

    dev->i2c_address = MS5611_ADDRESS;
    if(csb_pin_value) {
        dev->i2c_address = MS5611_ADDRESS - 0x02;
    }

    dev->osr = osr;

    switch(osr) {
        case MS5611_OSR_ULTRA_LOW_POWER:
            dev->read_time = 1;
            break;

        case MS5611_OSR_LOW_POWER:
            dev->read_time = 2;
            break;

        case MS5611_OSR_STANDARD:
            dev->read_time = 3;
            break;

        case MS5611_OSR_HIGH_RES:
            dev->read_time = 5;
            break;

        case MS5611_OSR_ULTRA_HIGH_RES:
            dev->read_time = 10;
            break;
    }

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer[0] = MS5611_CMD_RESET;
    ret = i2c_master_write(dev->i2c, dev->i2c_address, buffer, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else {
        if(dev->i2c_mutex) {
            mutex_unlock(dev->i2c_mutex);
        }

        task_sleep(5);  //checkAL aici e bine cu milisecunde

        if(dev->i2c_mutex) {
            mutex_lock(dev->i2c_mutex);
        }

        for(uint8_t i = 0; i < 8; i++) {
            ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MS5611_CMD_READ_PROM + i*2, 1, buffer, 2);
            dev->c[i] = buffer[1] + 256*buffer[0];
        }

        if(ret != 0) {
            ret = EIO;
        }
        else {
            //check CRC
            uint16_t crc_read = dev->c[7];
            uint16_t crc_calc = 0x0000;
            dev->c[7] &= 0xFF00;
            
            for (int8_t cnt = 0; cnt < 16; cnt++) {// choose LSB or MSB
                if (cnt % 2==1) {
                    crc_calc ^= (uint16_t)((dev->c[cnt >> 1]) & 0x00FF);
                }
                else {
                    crc_calc ^= (uint16_t)(dev->c[cnt >> 1] >> 8);
                } 
                for (uint8_t n_bit = 8; n_bit > 0; n_bit--) {
                    if (crc_calc & (0x8000)) {
                        crc_calc = (crc_calc << 1) ^ 0x3000;
                    }
                    else {
                        crc_calc = (crc_calc << 1);
                    }
                }
            }

            crc_calc= (0x000F & (crc_calc >> 12)); // final 4-bit reminder is CRC code
            dev->c[7] = crc_read; // restore the crc_read to its original place

            crc_calc ^= 0x0000;
            crc_calc &= 0x0F;
            crc_read &= 0x0F;
            if(crc_calc != crc_read) {
                ret = EINVAL;
            }
        }
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t ms5611_read(ms5611_t *dev, ms5611_data_t *data) {
    assert(dev != 0);

    uint8_t buffer[3];
    uint32_t d[3];
    uint32_t ret = 0;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer[0] = MS5611_CMD_CONV_D1 + (uint8_t)dev->osr;
    ret += i2c_master_write(dev->i2c, dev->i2c_address, buffer, 1);
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }
    task_sleep(dev->read_time);  //checkAL aici e bine cu milisecunde
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }
    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MS5611_CMD_ADC_READ, 1, buffer, 3);
    d[1] = buffer[2] + 256 * buffer[1] + 65536 * buffer[0];


    buffer[0] = MS5611_CMD_CONV_D2 + (uint8_t)dev->osr;
    ret += i2c_master_write(dev->i2c, dev->i2c_address, buffer, 1);
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }
    task_sleep(dev->read_time);  //checkAL aici e bine cu milisecunde
    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }
    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, MS5611_CMD_ADC_READ, 1, buffer, 3);
    d[2] = buffer[2] + 256 * buffer[1] + 65536 * buffer[0];

    int32_t dT = d[2] - 256 * dev->c[5];
    data->temperature = ((int64_t)dT * dev->c[6]) / 8388608;
    data->temperature += 2000;

    int64_t off = (int64_t)dev->c[2] * 65536;
    off += ((int64_t)dev->c[4] * dT) / 128;

    int64_t sens = (int64_t)dev->c[1] * 32768;
    sens += ((int64_t)dev->c[3] * dT) / 256;

    int64_t off2 = 0;
    int64_t sens2 = 0;
    if(data->temperature < 2000) {
        off2 = 5 * ((data->temperature - 2000) * (data->temperature - 2000)) / 2;
        sens2 = off2 / 2;
    }

    if(data->temperature < -1500) {
        off2 += 7 * ((data->temperature + 1500) * (data->temperature + 1500));
        sens2 += 11 * ((data->temperature + 1500) * (data->temperature + 1500)) / 2;
    }

    off -= off2;
    sens -= sens2;
    
    data->pressure = ((int64_t)d[1] * sens / 2097152 - off) / 32768;

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    if(ret != 0) {
        ret = EIO;
    }

    return ret;
}
