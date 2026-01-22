#include "bme680.h"
#include <assert.h>
#include <error.h>
#include <task.h>

TITAN_DEBUG_FILE_MARK;

#define BME680_I2C_ADDR                     0x76
#define BME680_WHOAMI_WORD                  0x61
#define BME680_RESET_WORD                   0xB6

#define BME680_REG_STATUS                   0x1D
#define BME680_REG_PRES_MSB                 0x1F
#define BME680_REG_PRES_LSB                 0x20
#define BME680_REG_PRES_XLSB                0x21
#define BME680_REG_TEMP_MSB                 0x22
#define BME680_REG_TEMP_LSB                 0x23
#define BME680_REG_TEMP_XLSB                0x24
#define BME680_REG_HUM_MSB                  0x25
#define BME680_REG_HUM_LSB                  0x26
#define BME680_REG_GAS_MSB                  0x2A
#define BME680_REG_GAS_LSB                  0x2B

#define BME680_REG_CTRL_GAS0                0x70
#define BME680_REG_CTRL_GAS1                0x71
#define BME680_REG_CTRL_HUM                 0x72
#define BME680_REG_CTRL_MEAS                0x74
#define BME680_REG_CONFIG                   0x75
#define BME680_REG_WHOAMI                   0xD0
#define BME680_REG_RESET                    0xE0

#define BME680_REG_CALIB1                   0x89
#define BME680_REG_CALIB2                   0xE1
#define BME680_REG_RES_HEAT_RANGE           0x02
#define BME680_REG_RES_HEAT_CORR            0x00
#define BME680_REG_RES_SW_ERR               0x04

#define BME680_REG_IDAC_HEAT0               0x50
#define BME680_REG_RES_HEAT0                0x5A
#define BME680_REG_GAS_WAIT0                0x64

#define BME680_GAS_WAIT_X1                  0
#define BME680_GAS_WAIT_X4                  1
#define BME680_GAS_WAIT_X16                 2
#define BME680_GAS_WAIT_X64                 3

#define BME680_OVERSAMPLING_SKIP            0
#define BME680_OVERSAMPLING_1               1
#define BME680_OVERSAMPLING_2               2
#define BME680_OVERSAMPLING_4               3
#define BME680_OVERSAMPLING_8               4
#define BME680_OVERSAMPLING_16              5

#define BME680_FILTER_COEFF_0               0
#define BME680_FILTER_COEFF_1               1
#define BME680_FILTER_COEFF_3               2
#define BME680_FILTER_COEFF_7               3
#define BME680_FILTER_COEFF_15              4
#define BME680_FILTER_COEFF_31              5
#define BME680_FILTER_COEFF_63              6
#define BME680_FILTER_COEFF_127             7

#define BME680_GAS_PROFILE_TEMPERATURE_MIN  200
#define BME680_GAS_PROFILE_TEMPERATURE_MAX  400


const uint64_t bme680_gas_lookup_k1_range[16] = {
    2147483647UL, 2147483647UL, 2147483647UL, 2147483647UL, 2147483647UL,
    2126008810UL, 2147483647UL, 2130303777UL, 2147483647UL, 2147483647UL,
    2143188679UL, 2136746228UL, 2147483647UL, 2126008810UL, 2147483647UL,
    2147483647UL
};
 
const uint64_t bme680_gas_lookup_k2_range[16] = {
    4096000000UL, 2048000000UL, 1024000000UL, 512000000UL,
    255744255UL, 127110228UL, 64000000UL, 32258064UL, 16016016UL,
    8000000UL, 4000000UL, 2000000UL, 1000000UL, 500000UL, 250000UL,
    125000UL
};


static uint8_t _bme680_calc_res_heat(bme680_t *dev, int16_t ambient, uint16_t target) {
    uint8_t res_heat = 0;

    if ((target >= BME680_GAS_PROFILE_TEMPERATURE_MIN) && (target <= BME680_GAS_PROFILE_TEMPERATURE_MAX)) {
        int32_t var1 = (((int32_t)ambient * dev->par_GH3) / 10) << 8;
        int32_t var2 = (dev->par_GH1 + 784) * (((((dev->par_GH2 + 154009) * target * 5) / 100) + 3276800) / 10);
        int32_t var3 = var1 + (var2 >> 1);
        int32_t var4 = (var3 / (dev->res_heat_range + 4));
        int32_t var5 = (131 * dev->res_heat_val) + 65536;
        int32_t res_heat_x100 = (int32_t)(((var4 / var5) - 250) * 34);
        res_heat = (uint8_t) ((res_heat_x100 + 50) / 100);
    }
    return res_heat;
}

uint32_t bme680_init(bme680_t *dev, i2c_t i2c, mutex_t *i2c_mutex, uint8_t addr_pin_value) {
    assert(dev != 0);
    
    uint8_t whoami = 0;
    uint32_t ret = 0;
    uint8_t buffer[41];

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;
    dev->i2c_address = 2 * (BME680_I2C_ADDR + (addr_pin_value & 0x01));

    dev->ctrl_config = BME680_FILTER_COEFF_0 << 2;
    dev->ctrl_gas0 = 0;
    dev->ctrl_gas1 = 1 << 4;                                                            //run_gas == 1, nb_conv = 0
    dev->ctrl_hum = BME680_OVERSAMPLING_1;                                              //humidity oversampling
    dev->ctrl_meas = (BME680_OVERSAMPLING_4 << 5) | (BME680_OVERSAMPLING_16 << 2);      //temperature and pressure oversampling. mode is 0 yet

    dev->gas_wait = (BME680_GAS_WAIT_X4 << 6) | (25 & 0x3F);
    dev->res_heat = _bme680_calc_res_heat(dev, 25, 350);    //checkAL poate e mai bine sa setam asta altfel ca ambientul e constant???

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_WHOAMI, 1, (void*)&whoami, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else
    if(whoami != BME680_WHOAMI_WORD) {
        ret = ENODEV;
    }
    else {
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_CALIB1, 1, buffer, 25);
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_CALIB2, 1, buffer + 25, 16);

        //store calibration values
        dev->par_T1 = (buffer[34] << 8) | buffer[33];
        dev->par_T2 = (buffer[2] << 8) | buffer[1];
        dev->par_T3 = buffer[3];
    
        dev->par_P1 = (buffer[6] << 8) | buffer[5];
        dev->par_P2 = (buffer[8] << 8) | buffer[7];
        dev->par_P3 = buffer[9];
        dev->par_P4 = (buffer[12] << 8) | buffer[11];
        dev->par_P5 = (buffer[14] << 8) | buffer[13];
        dev->par_P6 = buffer[16];
        dev->par_P7 = buffer[15];
        dev->par_P8 = (buffer[20] << 8) | buffer[19];
        dev->par_P9 = (buffer[22] << 8) | buffer[21];
        dev->par_P10 = buffer[23];
    
        dev->par_H1 = (buffer[27] << 4) | (buffer[26] & 0x0F);
        dev->par_H2 = (buffer[25] << 4) | (buffer[26] >> 4);
        dev->par_H3 = buffer[28];
        dev->par_H4 = buffer[29];
        dev->par_H5 = buffer[30];
        dev->par_H6 = buffer[31];
        dev->par_H7 = buffer[32];
    
        dev->par_GH1 = buffer[37];
        dev->par_GH2 = (buffer[36] << 8) | buffer[35];
        dev->par_GH3 = buffer[38];
    
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_RES_HEAT_RANGE, 1, &dev->res_heat_range, 1);
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_RES_HEAT_CORR, 1, &dev->res_heat_val, 1);
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_RES_SW_ERR, 1, &dev->range_switching_error, 1);

        dev->res_heat_range = (dev->res_heat_range >> 4) & 0x03;
        dev->range_switching_error = (dev->range_switching_error & 0xF0) >> 4;

        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_CONFIG, 1, &dev->ctrl_config, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_CTRL_MEAS, 1, &dev->ctrl_meas, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_CTRL_HUM, 1, &dev->ctrl_hum, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_GAS_WAIT0, 1, &dev->gas_wait, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_RES_HEAT0, 1, &dev->res_heat, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_CTRL_GAS1, 1, &dev->ctrl_gas1, 1);
        
        if(ret != 0) {
            ret = EIO;
        }
    }
    
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t bme680_reset(bme680_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = BME680_RESET_WORD;
    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_RESET, 1, &buffer, 1);
    if(ret != 0) {
        ret = EIO;
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t bme680_read(bme680_t *dev, bme680_data_t *data) {
    assert(dev != 0);
    assert(data != 0);

    uint32_t ret = 0;
    uint8_t buffer[4];
    uint32_t buffer_u32;
    int32_t var1;
    int32_t var2;
    int32_t var3;
    int32_t var4;
    

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    dev->ctrl_meas |= 1;
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, BME680_REG_CTRL_MEAS, 1, &dev->ctrl_meas, 1);
    if(ret != 0) {
        ret = EIO;
    }

    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_STATUS, 1, buffer, 1);
    while(((buffer[0] & 0x80) == 0) && (ret == 0)) {
        if(dev->i2c_mutex) {
            mutex_unlock(dev->i2c_mutex);
        }

        task_sleep(100);          //checkAL  aici vrem ceva cu ms

        if(dev->i2c_mutex) {
            mutex_lock(dev->i2c_mutex);
        }
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_STATUS, 1, buffer, 1);
    }

    if(ret == 0) {
        //temperature
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_TEMP_MSB, 1, buffer, 3);
        buffer_u32 = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
        var1 = ((int32_t)buffer_u32 >> 3) - ((int32_t)(dev->par_T1 << 1));
        var2 = (var1 * (int32_t) dev->par_T2) >> 11;
        var3 = ((((var1 >> 1) * (var1 >> 1)) >> 12) * ((int32_t)(dev->par_T3 << 4))) >> 14;
        dev->temp = var2 + var3;
        data->temperature = ((dev->temp * 5) + 128) >> 8;
        
        //pressure
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_PRES_MSB, 1, buffer, 3);
        buffer_u32 = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);

        var1 = (((int32_t)dev->temp) >> 1) - 64000;
        var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)dev->par_P6) >> 2;
        var2 = var2 + ((var1 * (int32_t)dev->par_P5) << 1);
        var2 = (var2 >> 2) + ((int32_t)dev->par_P4 << 16);
        var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)dev->par_P3 << 5)) >> 3) + (((int32_t)dev->par_P2 * var1) >> 1);
        var1 = var1 >> 18;
        var1 = ((32768 + var1) * (int32_t)dev->par_P1) >> 15;
        data->pressure = 1048576 - buffer_u32;
        data->pressure = (int32_t)((data->pressure - (var2 >> 12)) * ((int32_t)3125));
        var4 = (1 << 31);
        if (data->pressure >= var4) {
            data->pressure = ((data->pressure / (int32_t)var1) << 1);
        }
        else {
            data->pressure = ((data->pressure << 1) / (int32_t)var1);
        }
        var1 = ((int32_t)dev->par_P9 * (int32_t)(((data->pressure >> 3) * (data->pressure >> 3)) >> 13)) >> 12;
        var2 = ((int32_t)(data->pressure >> 2) * (int32_t)dev->par_P8) >> 13;
        var3 = ((int32_t)(data->pressure >> 8) * (int32_t)(data->pressure >> 8) * (int32_t)(data->pressure >> 8) * (int32_t)dev->par_P10) >> 17;
        data->pressure = (int32_t)(data->pressure) + ((var1 + var2 + var3 + ((int32_t)dev->par_P7 << 7)) >> 4);        

        //humidity
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_HUM_MSB, 1, buffer, 2);
        buffer_u32 = (buffer[0] << 8) | buffer[1];
        int32_t temp_scaled = (dev->temp * 5 + 128) >> 8;
        var1 = (int32_t)buffer_u32 - ((int32_t)((int32_t)dev->par_H1 << 4)) - (((temp_scaled * (int32_t)dev->par_H3) / ((int32_t)100)) >> 1);
        var2 = ((int32_t)dev->par_H2 * (((temp_scaled * (int32_t)dev->par_H4) / ((int32_t)100)) + (((temp_scaled * ((temp_scaled * (int32_t)dev->par_H5) / ((int32_t)100))) >> 6) / ((int32_t)100)) + (int32_t)(1 << 14))) >> 10;
        var3 = var1 * var2;
        var4 = ((((int32_t)dev->par_H6) << 7) + ((temp_scaled * (int32_t)dev->par_H7) / ((int32_t)100))) >> 4;
        var1 = ((var3 >> 14) * (var3 >> 14)) >> 10;
        var2 = (var4 * var1) >> 1;
    
        data->humidity = (var3 + var2) >> 12;
        if (data->humidity > 102400) {
            data->humidity = 102400;
        }
        else if (data->humidity < 0) {
            data->humidity = 0;
        }
        
        //gas resistance
        ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, BME680_REG_GAS_MSB, 1, buffer, 2);
        uint8_t gas_range = buffer[1] & 0x0F;
        uint16_t gas_reading = (buffer[0] << 2) | (buffer[1] >> 6);
        int64_t gvar1 = (int64_t)((1340 + (5 * (int64_t)dev->range_switching_error)) * ((int64_t)bme680_gas_lookup_k1_range[gas_range])) >> 16;
        int64_t gvar2 = (int64_t)((int64_t)gas_reading << 15) - (int64_t)(1 << 24) + gvar1;
        data->gas_resistance = (int32_t)(((((int64_t)bme680_gas_lookup_k2_range[gas_range] * (int64_t)gvar1) >> 9) + (gvar2 >> 1)) / gvar2);
    }
    


    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
