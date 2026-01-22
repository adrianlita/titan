#include "pac1934.h"
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define PAC1934_REG_CMD_REFRESH         0x00
#define PAC1934_REG_CTRL                0x01
#define PAC1934_REG_ACC_COUNT           0x02
#define PAC1934_REG_VPOWER1_ACC         0x03
#define PAC1934_REG_VPOWER2_ACC         0x04
#define PAC1934_REG_VPOWER3_ACC         0x05
#define PAC1934_REG_VPOWER4_ACC         0x06
#define PAC1934_REG_VBUS1_ACC           0x07
#define PAC1934_REG_VBUS2_ACC           0x08
#define PAC1934_REG_VBUS3_ACC           0x09
#define PAC1934_REG_VBUS4_ACC           0x0A
#define PAC1934_REG_VSENSE1_ACC         0x0B
#define PAC1934_REG_VSENSE2_ACC         0x0C
#define PAC1934_REG_VSENSE3_ACC         0x0D
#define PAC1934_REG_VSENSE4_ACC         0x0E
#define PAC1934_REG_VBUS1_AVG_ACC       0x0F
#define PAC1934_REG_VBUS2_AVG_ACC       0x10
#define PAC1934_REG_VBUS3_AVG_ACC       0x11
#define PAC1934_REG_VBUS4_AVG_ACC       0x12
#define PAC1934_REG_VSENSE1_AVG_ACC     0x13
#define PAC1934_REG_VSENSE2_AVG_ACC     0x14
#define PAC1934_REG_VSENSE3_AVG_ACC     0x15
#define PAC1934_REG_VSENSE4_AVG_ACC     0x16
#define PAC1934_REG_VPOWER1             0x17
#define PAC1934_REG_VPOWER2             0x18
#define PAC1934_REG_VPOWER3             0x19
#define PAC1934_REG_VPOWER4             0x1A
#define PAC1934_REG_CHANNEL_DIS_SMB     0x1C
#define PAC1934_REG_NEG_PWR             0x1D
#define PAC1934_REG_CMD_REFRESH_G       0x1E
#define PAC1934_REG_CMD_REFRESH_V       0x1F
#define PAC1934_REG_SLOW                0x20

#define PAC1934_REG_CTRL_ACT            0x21
#define PAC1934_REG_CHANNEL_DIS_SMB_ACT 0x22
#define PAC1934_REG_NEG_PWR_ACT         0x23
#define PAC1934_REG_CTRL_LAT            0x24
#define PAC1934_REG_DIS_LAT             0x25
#define PAC1934_REG_NEG_PWR_LAT         0x26
#define PAC1934_REG_PRODUCT_ID          0xFD
#define PAC1934_REG_MF_ID               0xFE
#define PAC1934_REG_REV_ID              0xFF

#define PAC1934_PRODUCT_ID              0x5B
#define PAC1934_MANUFACTURER_ID         0x5D

uint32_t pac1934_init(pac1934_t *dev, i2c_t i2c, mutex_t *i2c_mutex, pac1934_addr_res_t addr_res) {
    assert(dev != 0);

    uint8_t mf_id = 0;
    uint8_t pr_id = 0;
    uint32_t ret = 0;

    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;
    dev->i2c_address = 0;
    dev->revision = 255;

    switch(addr_res) {
        case PAC1934_ADDR_RES0:
            dev->i2c_address = 0x20;
            break;

        case PAC1934_ADDR_RES499:
            dev->i2c_address = 0x22;
            break;

        case PAC1934_ADDR_RES806:
            dev->i2c_address = 0x24;
            break;

        case PAC1934_ADDR_RES1270:
            dev->i2c_address = 0x26;
            break;

        case PAC1934_ADDR_RES2050:
            dev->i2c_address = 0x28;
            break;

        case PAC1934_ADDR_RES3240:
            dev->i2c_address = 0x2A;
            break;

        case PAC1934_ADDR_RES5230:
            dev->i2c_address = 0x2C;
            break;

        case PAC1934_ADDR_RES8450:
            dev->i2c_address = 0x2E;
            break;

        case PAC1934_ADDR_RES13300:
            dev->i2c_address = 0x30;
            break;

        case PAC1934_ADDR_RES21500:
            dev->i2c_address = 0x32;
            break;

        case PAC1934_ADDR_RES34000:
            dev->i2c_address = 0x34;
            break;

        case PAC1934_ADDR_RES54900:
            dev->i2c_address = 0x36;
            break;

        case PAC1934_ADDR_RES88700:
            dev->i2c_address = 0x38;
            break;

        case PAC1934_ADDR_RES140000:
            dev->i2c_address = 0x3A;
            break;

        case PAC1934_ADDR_RES226000:
            dev->i2c_address = 0x3C;
            break;

        case PAC1934_ADDR_RESNONE:
            dev->i2c_address = 0x3E;
            break;

        default:
            assert(0);
            break;
    }

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_MF_ID, 1, (void*)&mf_id, 1);
    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_PRODUCT_ID, 1, (void*)&pr_id, 1);

    if(ret != 0) {
        ret = EIO;
    }
    else {
        if((pr_id != PAC1934_PRODUCT_ID) || (mf_id != PAC1934_MANUFACTURER_ID)) {
            ret = ENODEV;
        }
        else {
            i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_REV_ID, 1, &dev->revision, 1);
        }
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t pac1934_turn_on(pac1934_t *dev, pac1934_alert_type_t alert_type, pac1934_sample_rate_t sample_rate, pac1934_channel_config_t channel_config[4]) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t ctrl_reg;
    uint8_t channel_dis_reg;
    uint8_t neg_pwr_reg;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_CTRL_ACT, 1, (void*)&ctrl_reg, 1);
    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_CHANNEL_DIS_SMB_ACT, 1, (void*)&channel_dis_reg, 1);
    ret += i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_NEG_PWR_ACT, 1, (void*)&neg_pwr_reg, 1);
    if(ret != 0) {
        ret = EIO;
    }
    else {
        dev->sample_rate = (pac1934_sample_rate_t)(((ctrl_reg & 0xC0) >> 6));
        if((ctrl_reg & 0x10) > 0) {
            dev->sample_rate = PAC1934_SAMPLE_RATE_0HZ_ONE_SHOT;
        }

        if((ctrl_reg & 0x20) > 0) {
            dev->sample_rate = PAC1934_SAMPLE_RATE_0HZ_SLEEP;
        }

        //channel 2
        if((channel_dis_reg & 0x80) > 0) {
            dev->channel_config[0] = PAC1934_CHANNEL_OFF;
        }
        else {
            if((neg_pwr_reg & 0x80) > 0) {
                if((neg_pwr_reg & 0x08) > 0) {
                    dev->channel_config[0] = PAC1934_CHANNEL_BIDI_VI;
                }
                else {
                    dev->channel_config[0] = PAC1934_CHANNEL_BIDI_I;
                }
            }
            else {
                if((neg_pwr_reg & 0x08) > 0) {
                    dev->channel_config[0] = PAC1934_CHANNEL_BIDI_V;
                }
                else {
                    dev->channel_config[0] = PAC1934_CHANNEL_UNI_VI;
                }
            }
        }

        //channel 2
        if((channel_dis_reg & 0x40) > 0) {
            dev->channel_config[1] = PAC1934_CHANNEL_OFF;
        }
        else {
            if((neg_pwr_reg & 0x40) > 0) {
                if((neg_pwr_reg & 0x04) > 0) {
                    dev->channel_config[1] = PAC1934_CHANNEL_BIDI_VI;
                }
                else {
                    dev->channel_config[1] = PAC1934_CHANNEL_BIDI_I;
                }
            }
            else {
                if((neg_pwr_reg & 0x04) > 0) {
                    dev->channel_config[1] = PAC1934_CHANNEL_BIDI_V;
                }
                else {
                    dev->channel_config[1] = PAC1934_CHANNEL_UNI_VI;
                }
            }
        }

        //channel 3
        if((channel_dis_reg & 0x20) > 0) {
            dev->channel_config[2] = PAC1934_CHANNEL_OFF;
        }
        else {
            if((neg_pwr_reg & 0x20) > 0) {
                if((neg_pwr_reg & 0x02) > 0) {
                    dev->channel_config[2] = PAC1934_CHANNEL_BIDI_VI;
                }
                else
                {
                    dev->channel_config[2] = PAC1934_CHANNEL_BIDI_I;
                }
            }
            else {
                if((neg_pwr_reg & 0x02) > 0) {
                    dev->channel_config[2] = PAC1934_CHANNEL_BIDI_V;
                }
                else {
                    dev->channel_config[2] = PAC1934_CHANNEL_UNI_VI;
                }
            }
        }

        //channel 4
        if((channel_dis_reg & 0x10) > 0) {
            dev->channel_config[3] = PAC1934_CHANNEL_OFF;
        }
        else {
            if((neg_pwr_reg & 0x10) > 0) {
                if((neg_pwr_reg & 0x01) > 0) {
                    dev->channel_config[3] = PAC1934_CHANNEL_BIDI_VI;
                }
                else
                {
                    dev->channel_config[3] = PAC1934_CHANNEL_BIDI_I;
                }
            }
            else {
                if((neg_pwr_reg & 0x01) > 0) {
                    dev->channel_config[3] = PAC1934_CHANNEL_BIDI_V;
                }
                else {
                    dev->channel_config[3] = PAC1934_CHANNEL_UNI_VI;
                }
            }
        }


        //update with the new values
        dev->alert_type = alert_type;
        dev->sample_rate = sample_rate;
        dev->channel_config[0] = channel_config[0];
        dev->channel_config[1] = channel_config[1];
        dev->channel_config[2] = channel_config[2];
        dev->channel_config[3] = channel_config[3];

        ctrl_reg = 0;
        switch(dev->sample_rate) {
            case PAC1934_SAMPLE_RATE_0HZ_SLEEP:
                ctrl_reg |= (1 << 5);
                break;

            case PAC1934_SAMPLE_RATE_0HZ_ONE_SHOT:
                ctrl_reg |= (1 << 4);
                break;

            case PAC1934_SAMPLE_RATE_8HZ:
            case PAC1934_SAMPLE_RATE_64HZ:
            case PAC1934_SAMPLE_RATE_256HZ:
            case PAC1934_SAMPLE_RATE_1024HZ:
                ctrl_reg |= ((dev->sample_rate & 0x03) << 6);
                break;
        }

        //set alert to interrupt
        switch(dev->alert_type) {
            case PAC1934_ALERT_TYPE_DISABLE:
                break;

            case PAC1934_ALERT_TYPE_CYCLE_COMPLETE:
                ctrl_reg |= 0x0C;
                break;

            case PAC1934_ALERT_TYPE_ACC_OVF:
                ctrl_reg |= 0x0A;
                break;
        }

        //skip channels when inactive
        channel_dis_reg = 0;
        neg_pwr_reg = 0;

        switch(dev->channel_config[0]) {
            case PAC1934_CHANNEL_OFF:
                channel_dis_reg |= (1 << 7);
                break;

            case PAC1934_CHANNEL_UNI_VI:
                //leave both neg values to 0
                break;

            case PAC1934_CHANNEL_BIDI_V:
                neg_pwr_reg |= (1 << 3);  //set v bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_I:
                neg_pwr_reg |= (1 << 7);  //set i bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_VI:
                neg_pwr_reg |= (1 << 3);  //set v bidirectional
                neg_pwr_reg |= (1 << 7);  //set i bidirectional
                break;
        }

        switch(dev->channel_config[1]) {
            case PAC1934_CHANNEL_OFF:
                channel_dis_reg |= (1 << 6);
                break;

            case PAC1934_CHANNEL_UNI_VI:
                //leave both neg values to 0
                break;

            case PAC1934_CHANNEL_BIDI_V:
                neg_pwr_reg |= (1 << 2);  //set v bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_I:
                neg_pwr_reg |= (1 << 6);  //set i bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_VI:
                neg_pwr_reg |= (1 << 2);  //set v bidirectional
                neg_pwr_reg |= (1 << 6);  //set i bidirectional
                break;
        }

        switch(dev->channel_config[2]) {
            case PAC1934_CHANNEL_OFF:
                channel_dis_reg |= (1 << 5);
                break;

            case PAC1934_CHANNEL_UNI_VI:
                //leave both neg values to 0
                break;

            case PAC1934_CHANNEL_BIDI_V:
                neg_pwr_reg |= (1 << 1);  //set v bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_I:
                neg_pwr_reg |= (1 << 5);  //set i bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_VI:
                neg_pwr_reg |= (1 << 1);  //set v bidirectional
                neg_pwr_reg |= (1 << 5);  //set i bidirectional
                break;
        }

        switch(dev->channel_config[3]) {
            case PAC1934_CHANNEL_OFF:
                channel_dis_reg |= (1 << 4);
                break;

            case PAC1934_CHANNEL_UNI_VI:
                //leave both neg values to 0
                break;

            case PAC1934_CHANNEL_BIDI_V:
                neg_pwr_reg |= 0x01;  //set v bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_I:
                neg_pwr_reg |= (1 << 4);  //set i bidirectional
                break;

            case PAC1934_CHANNEL_BIDI_VI:
                neg_pwr_reg |= 0x01;  //set v bidirectional
                neg_pwr_reg |= (1 << 4);  //set i bidirectional
                break;
        }

        ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, PAC1934_REG_CTRL, 1, (void*)&ctrl_reg, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, PAC1934_REG_CHANNEL_DIS_SMB, 1, (void*)&channel_dis_reg, 1);
        ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, PAC1934_REG_NEG_PWR, 1, (void*)&neg_pwr_reg, 1);

        buffer = PAC1934_REG_CMD_REFRESH;
        ret += i2c_master_write(dev->i2c, dev->i2c_address, &buffer, 1);
    }

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t pac1934_turn_off(pac1934_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t ctrl_reg = 0;
    uint8_t channel_dis_reg = 0;
    uint8_t neg_pwr_reg = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    ret = i2c_master_write_mem(dev->i2c, dev->i2c_address, PAC1934_REG_CTRL_ACT, 1, (void*)&ctrl_reg, 1);
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, PAC1934_REG_CHANNEL_DIS_SMB_ACT, 1, (void*)&channel_dis_reg, 1);
    ret += i2c_master_write_mem(dev->i2c, dev->i2c_address, PAC1934_REG_NEG_PWR_ACT, 1, (void*)&neg_pwr_reg, 1);

    buffer = PAC1934_REG_CMD_REFRESH;
    ret += i2c_master_write(dev->i2c, dev->i2c_address, &buffer, 1);

    if(ret != 0) {
        ret = EIO;
    }
    

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t pac1934_refresh(pac1934_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = PAC1934_REG_CMD_REFRESH;
    ret = i2c_master_write(dev->i2c, dev->i2c_address, &buffer, 1);
    if(ret != 0) {
        ret = EIO;
    }
    
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t pac1934_refresh_v(pac1934_t *dev) {
    assert(dev != 0);

    uint32_t ret = 0;
    uint8_t buffer;

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    buffer = PAC1934_REG_CMD_REFRESH_V;
    ret = i2c_master_write(dev->i2c, dev->i2c_address, &buffer, 1);
    if(ret != 0) {
        ret = EIO;
    }
    
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}

uint32_t pac1934_read(pac1934_t *dev, uint32_t *accumulator_count, pac1934_channel_data_t channel[4]) {
    assert(dev != 0);
    assert(accumulator_count != 0);
    assert(channel != 0);

    uint32_t ret = 0;
    uint8_t total_read_bytes = 0;
    uint8_t buffer[80];
    uint8_t aux[8];

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }


    total_read_bytes = 3;
    if(dev->channel_config[0] != PAC1934_CHANNEL_OFF) {
        total_read_bytes += 18;
    }

    if(dev->channel_config[1] != PAC1934_CHANNEL_OFF) {
        total_read_bytes += 18;
    }

    if(dev->channel_config[2] != PAC1934_CHANNEL_OFF) {
        total_read_bytes += 18;
    }

    if(dev->channel_config[3] != PAC1934_CHANNEL_OFF) {
        total_read_bytes += 18;
    }

    ret = i2c_master_read_mem(dev->i2c, dev->i2c_address, PAC1934_REG_ACC_COUNT, 1, buffer, total_read_bytes);
    if(ret != 0) {
        ret = EIO;
    }
    else {
        aux[3] = 0;
        aux[2] = buffer[0];
        aux[1] = buffer[1];
        aux[0] = buffer[2];
        *accumulator_count = *(uint32_t*)aux;

        uint8_t i = 3;
        for(uint8_t j = 0; j < 4; j++) {
            if(dev->channel_config[j] == PAC1934_CHANNEL_OFF) {
                channel[j].vpower_acc = 0;
            }
            else {
                //vpower_acc
                aux[7] = 0;
                aux[6] = 0;
                aux[5] = buffer[i++];
                aux[4] = buffer[i++];
                aux[3] = buffer[i++];
                aux[2] = buffer[i++];
                aux[1] = buffer[i++];
                aux[0] = buffer[i++];
                switch(dev->channel_config[j]) {
                    case PAC1934_CHANNEL_UNI_VI:
                        aux[7] = 0;
                        aux[6] = 0;
                        channel[j].vpower_acc = *(int64_t*)aux;
                        break;

                    case PAC1934_CHANNEL_BIDI_V:
                    case PAC1934_CHANNEL_BIDI_I:
                    case PAC1934_CHANNEL_BIDI_VI:
                        if((aux[5] & 0x80) > 0) {
                            aux[7] = 0xFF;
                            aux[6] = 0xFF;
                        }
                        else {
                            aux[7] = 0;
                            aux[6] = 0;
                        }
                        channel[j].vpower_acc = *(int64_t*)aux;
                        break;
                }
            }
        }

        for(uint8_t j = 0; j < 4; j++) {
            if(dev->channel_config[j] == PAC1934_CHANNEL_OFF) {
                channel[j].vbus = 0;
            }
            else {
                //vbus
                aux[1] = buffer[i++];
                aux[0] = buffer[i++];
                switch(dev->channel_config[j]) {
                    case PAC1934_CHANNEL_UNI_VI:
                    case PAC1934_CHANNEL_BIDI_I:
                        channel[j].vbus = *(uint16_t*)aux;
                        break;

                    case PAC1934_CHANNEL_BIDI_V:
                    case PAC1934_CHANNEL_BIDI_VI:
                        channel[j].vbus = *(int16_t*)aux;
                        break;
                }
            }
        }

        for(uint8_t j = 0; j < 4; j++) {  
            if(dev->channel_config[j] == PAC1934_CHANNEL_OFF) {
                channel[j].vsense = 0;
            }
            else {
                //vsense
                aux[1] = buffer[i++];
                aux[0] = buffer[i++];
                switch(dev->channel_config[j]) {
                    case PAC1934_CHANNEL_UNI_VI:
                    case PAC1934_CHANNEL_BIDI_V:
                        channel[j].vsense = *(uint16_t*)aux;
                        break;

                    case PAC1934_CHANNEL_BIDI_I:
                    case PAC1934_CHANNEL_BIDI_VI:
                        channel[j].vsense = *(int16_t*)aux;
                        break;
                }
            }
        }

        for(uint8_t j = 0; j < 4; j++) {
            if(dev->channel_config[j] == PAC1934_CHANNEL_OFF) {
                channel[j].vbus_avg = 0;
            }
            else {
                //vbus_avg
                aux[1] = buffer[i++];
                aux[0] = buffer[i++];
                switch(dev->channel_config[j]) {
                    case PAC1934_CHANNEL_UNI_VI:
                    case PAC1934_CHANNEL_BIDI_I:
                        channel[j].vbus_avg = *(uint16_t*)aux;
                        break;

                    case PAC1934_CHANNEL_BIDI_V:
                    case PAC1934_CHANNEL_BIDI_VI:
                        channel[j].vbus_avg = *(int16_t*)aux;
                        break;
                }
            }
        }

        for(uint8_t j = 0; j < 4; j++) {
            if(dev->channel_config[j] == PAC1934_CHANNEL_OFF) {
                channel[j].vsense_avg = 0;
            }
            else {  
                //vsense_avg
                aux[1] = buffer[i++];
                aux[0] = buffer[i++];
                switch(dev->channel_config[j]) {
                    case PAC1934_CHANNEL_UNI_VI:
                    case PAC1934_CHANNEL_BIDI_V:
                        channel[j].vsense_avg = *(uint16_t*)aux;
                        break;

                    case PAC1934_CHANNEL_BIDI_I:
                    case PAC1934_CHANNEL_BIDI_VI:
                        channel[j].vsense_avg = *(int16_t*)aux;
                        break;
                }
            }
        }

        for(uint8_t j = 0; j < 4; j++) {
            if(dev->channel_config[j] == PAC1934_CHANNEL_OFF) {
                channel[j].vpower = 0;
            }
            else {  
                //vpower
                aux[3] = buffer[i++];
                aux[2] = buffer[i++];
                aux[1] = buffer[i++];
                aux[0] = buffer[i++];
                switch(dev->channel_config[j]) {
                    case PAC1934_CHANNEL_UNI_VI:
                        aux[7] = 0;
                        aux[6] = 0;
                        channel[j].vpower = *(uint32_t*)aux;
                        break;

                    case PAC1934_CHANNEL_BIDI_V:
                    case PAC1934_CHANNEL_BIDI_I:
                    case PAC1934_CHANNEL_BIDI_VI:
                        if((aux[5] & 0x80) > 0) {
                            aux[7] = 0xFF;
                            aux[6] = 0xFF;
                        }
                        else {
                            aux[7] = 0;
                            aux[6] = 0;
                        }
                        channel[j].vpower = *(int32_t*)aux;
                        break;
                }
                channel[j].vpower /= 16;
            }
        }

        buffer[0] = PAC1934_REG_CMD_REFRESH_V;
        ret += i2c_master_write(dev->i2c, dev->i2c_address, buffer, 1);
        if(ret != 0) {
            ret = EIO;
        }
    }
    
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return ret;
}
