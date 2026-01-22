#include "atecc608.h"
#include "atecc608_registers.h"
#include <task.h>
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

static void atecc608_crc(const uint8_t *data, uint32_t length, uint8_t *crc_le);
static uint8_t atecc608_exec_time(const uint8_t command);

static uint32_t atecc608_send_receive_command(atecc608_t *dev, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *tx_data, uint8_t tx_length, uint8_t *rx_data, uint8_t rx_length);
static uint32_t atecc608_send_command(atecc608_t *dev, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *tx_data, uint8_t tx_length);
static uint32_t atecc608_receive_response(atecc608_t *dev, uint8_t *rx_data, uint8_t rx_length);

static uint32_t atecc608_write(atecc608_t *dev, uint8_t zone, uint16_t address, uint8_t *data, uint8_t length);
static uint32_t atecc608_read(atecc608_t *dev, uint8_t zone, uint16_t address, uint8_t *data, uint8_t length);

uint32_t atecc608_init(atecc608_t *dev, gpio_pin_t sda_pin, i2c_t i2c, mutex_t *i2c_mutex, uint8_t i2c_address) {
    uint32_t result = 0;

    assert(dev);

    dev->sda = sda_pin;
    dev->i2c = i2c;
    dev->i2c_mutex = i2c_mutex;
    dev->i2c_address = i2c_address;

    result += atecc608_wakeup(dev);
    result += atecc608_send_receive_command(dev, ATECC608_REG_INFO, ATECC608_INFO_REVISION, 0, 0, 0, 0, 4);
    if((result == 0) && (dev->response.data[2] == ATECC608A_INFO_REVISION_ID)) {
        result += atecc608_read_conf(dev);
    }
    else {
        result = 1;
    }

    result += atecc608_idle(dev);
    return result;
}


uint32_t atecc608_wakeup(atecc608_t *dev) {
    assert(dev);

    if(dev->i2c_mutex) {
        mutex_lock(dev->i2c_mutex);
    }

    dev->response.data[0] = 0x00;    
    gpio_deinit(dev->sda);
    gpio_init_digital(dev->sda, GPIO_MODE_OUTPUT_OD, GPIO_PULL_DOWN);
    gpio_digital_write(dev->sda, 0);
    task_sleep(2);
    gpio_deinit(dev->sda);
    gpio_init_special(dev->sda, GPIO_SPECIAL_FUNCTION_I2C, (uint32_t)dev->i2c);
    task_sleep(3);
    if((atecc608_receive_response(dev, 0, 1) == 0) && (dev->response.data[0] == ATECC608_RESPONSE_WOKEUP)) {
        return 0;
    }
    else {
        if(dev->i2c_mutex) {
            mutex_unlock(dev->i2c_mutex);
        }
        return 1;
    }
}

void atecc608_release(atecc608_t *dev) {
    assert(dev);

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }
}

uint32_t atecc608_idle(atecc608_t *dev) {
    assert(dev);

    uint32_t result;
    dev->command.word_address = ATECC608_WA_IDLE;
    dev->command.count = 0;

    result = i2c_master_write(dev->i2c, dev->i2c_address, &dev->command.word_address, dev->command.count + 1);   //count + w_a
    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }

    return result;
}


uint32_t atecc608_sleep(atecc608_t *dev) {
    assert(dev);

    uint32_t result;

    dev->command.word_address = ATECC608_WA_SLEEP;
    dev->command.count = 0;

    if(dev->i2c_mutex) {
        mutex_unlock(dev->i2c_mutex);
    }
    result = i2c_master_write(dev->i2c, dev->i2c_address, &dev->command.word_address, dev->command.count + 1);   //count + w_a
    return result;
}

uint32_t atecc608_get_key_valid(atecc608_t *dev, uint8_t slot, uint8_t *valid) {
    assert(dev);
    assert(slot < 16);
    assert(valid);
    
    uint32_t result = 0;
    result = atecc608_send_receive_command(dev, ATECC608_REG_INFO, ATECC608_INFO_KEYVALID, slot, 0, 0, 0, 4);
    if(result == 0) {
        *valid = dev->response.data[0];
    }

    return result;
}

uint32_t atecc608_get_state(atecc608_t *dev, atecc608_state_t *state) {
    assert(dev);
    assert(state);

    uint32_t result = 0;
    result = atecc608_send_receive_command(dev, ATECC608_REG_INFO, ATECC608_INFO_STATE, 0, 0, 0, 0, 4);
    if(result == 0) {
        state->tempkey.key_id = dev->response.data[0] & 0x0F;
        state->tempkey.source_flag = (dev->response.data[0] >> 4) & 0x01;
        state->tempkey.gendig_data = (dev->response.data[0] >> 5) & 0x01;
        state->tempkey.genkey_data = (dev->response.data[0] >> 6) & 0x01;
        state->tempkey.nomac_flag = (dev->response.data[0] >> 7) & 0x01;
        state->tempkey.valid = (dev->response.data[1] >> 7) & 0x01;
        
        state->authkey_id = (dev->response.data[1] >> 3) & 0x0F;
        state->auth_valid = (dev->response.data[1] >> 2) & 0x01;
        state->rng_sram = (dev->response.data[1] >> 1) & 0x01;
        state->rng_eeprom = dev->response.data[1] & 0x01;
    }

    return result;
}


uint32_t atecc608_rand(atecc608_t *dev, uint8_t *pool) {
    assert(dev);
    assert(pool);

    return atecc608_send_receive_command(dev, ATECC608_REG_RANDOM, 1, 0, 0, 0, pool, ATECC608_RAND_POOL_SIZE);
}

void atecc608_get_serial_number(atecc608_t *dev, uint8_t *serial_number) {
    assert(dev);
    assert(serial_number);

    serial_number[0] = dev->config.fields.sn03[0];
    serial_number[1] = dev->config.fields.sn03[1];
    serial_number[2] = dev->config.fields.sn03[2];
    serial_number[3] = dev->config.fields.sn03[3];
    serial_number[4] = dev->config.fields.sn48[0];
    serial_number[5] = dev->config.fields.sn48[1];
    serial_number[6] = dev->config.fields.sn48[2];
    serial_number[7] = dev->config.fields.sn48[3];
    serial_number[8] = dev->config.fields.sn48[4];
}

uint32_t atecc608_sha256_init(atecc608_t *dev) {
    assert(dev);

    return atecc608_send_command(dev, ATECC608_REG_SHA, 0, 0, 0, 0);
}

uint32_t atecc608_sha256_update(atecc608_t *dev, const uint8_t *data, uint32_t length) {
    assert(dev);
    assert(data);
    assert(length);

    uint32_t result = 0;

    uint32_t count = 0;
    while(count < length) {
        if(length - count >= 64) {
            result += atecc608_send_command(dev, ATECC608_REG_SHA, 1, 64, data + count, 64);
            count += 64;
        }
        else {
            result += atecc608_send_command(dev, ATECC608_REG_SHA, 1, length - count, data + count, length - count);
            count = length;
        }
    }

    return result;
}

uint32_t atecc608_sha256_final(atecc608_t *dev, uint8_t *hash) {
    assert(dev);
    assert(hash);

    return atecc608_send_receive_command(dev, ATECC608_REG_SHA, 2, 0, 0, 0, hash, ATECC608_SHA256_BLOCK_SIZE);
}

uint32_t atecc608_hmac_init(atecc608_t *dev, uint8_t slot_or_tempkey) {
    uint16_t param2;
    assert(dev);

    if(slot_or_tempkey < 16) {
        param2 = slot_or_tempkey;
    }
    else if(slot_or_tempkey == 16) {
        param2 = 0xFFFF;
    }
    else {
        assert(0);
    }

    return atecc608_send_command(dev, ATECC608_REG_SHA, 4, param2, 0, 0);
}

uint32_t atecc608_hmac_update(atecc608_t *dev, const uint8_t *data, uint32_t length) {
    assert(dev);
    assert(data);
    assert(length);

    uint32_t result = 0;

    uint32_t count = 0;
    while(count < length) {
        if(length - count >= 64) {
            result += atecc608_send_command(dev, ATECC608_REG_SHA, 1, 64, data + count, 64);
            count += 64;
        }
        else {
            result += atecc608_send_command(dev, ATECC608_REG_SHA, 1, length - count, data + count, length - count);
            count = length;
        }
    }

    return result;
}

uint32_t atecc608_hmac_final(atecc608_t *dev, uint8_t *hash, uint8_t store) {
    assert(dev);
    assert(hash);

    uint8_t param1;
    switch(store) {
        case 0:
            param1 = 0xC2;
            break;

        case 1:
            param1 = 0x02;
            break;
        
        case 2:
            param1 = 0x42;
            break;

        default:
            assert(0);
            break;
    }

    return atecc608_send_receive_command(dev, ATECC608_REG_SHA, param1, 0, 0, 0, hash, ATECC608_SHA256_BLOCK_SIZE);
}

uint16_t atecc608_get_slot_size(uint8_t slot) {
    if(slot < 8) {
        return 36;
    }
    else if(slot == 8) {
        return 416;
    }
    else if(slot < 16) {
        return 72;
    }
    else {
        return 0;
    }
}

uint32_t atecc608_read_slot(atecc608_t *dev, uint8_t slot, uint8_t *data) {
    assert(dev);
    assert(slot < 16);
    assert(data);

    uint32_t result = 0;

    uint16_t slot_size = atecc608_get_slot_size(slot);
    uint16_t i = 0;
    uint16_t address;
    uint8_t block = 0;
    uint8_t offset = 0;
    
    while(i < slot_size) {
        if((slot_size - i) > 32) {
            address = (block << 8) | (slot << 3);
            result += atecc608_read(dev, ATECC608_ZONE_SLOT, address, data + i, 32);
            i += 32;
            block++;
        }
        else {
            address = (block << 8) | (slot << 3) | offset;
            result += atecc608_read(dev, ATECC608_ZONE_SLOT, address, data + i, 4);
            i += 4;
            offset += 4;
        }
    }

    return result;
}

uint32_t atecc608_write_slot(atecc608_t *dev, uint8_t slot, uint8_t *data) {
    assert(dev);
    assert(slot < 16);
    assert(data);

    uint32_t result = 0;
    
    uint16_t slot_size = atecc608_get_slot_size(slot);
    uint16_t i = 0;
    uint16_t address;
    uint8_t block = 0;
    uint8_t offset = 0;
    
    while(i < slot_size) {
        if((slot_size - i) > 32) {
            address = (block << 8) | (slot << 3);
            result += atecc608_write(dev, ATECC608_ZONE_SLOT, address, data + i, 32);
            i += 32;
            block++;
        }
        else {
            address = (block << 8) | (slot << 3) | offset;
            result += atecc608_write(dev, ATECC608_ZONE_SLOT, address, data + i, 4);
            i += 4;
            offset += 1;
        }
    }
    return result;
}

uint32_t atecc608_read_conf(atecc608_t *dev) {
    uint32_t result = 0;

    assert(dev);

    result += atecc608_read(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK0, dev->config.raw, 32);
    result += atecc608_read(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK1, dev->config.raw + 32, 32);
    result += atecc608_read(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK2, dev->config.raw + 64, 32);
    result += atecc608_read(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK3, dev->config.raw + 96, 32);

    return result;
}

uint32_t atecc608_write_conf(atecc608_t *dev) {
    uint32_t result = 0;

    assert(dev);

    //first 16 bytes are non-writeable
    result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK0 + 4, dev->config.raw + 16, 4);
    result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK0 + 5, dev->config.raw + 20, 4);
    result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK0 + 6, dev->config.raw + 24, 4);
    result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK0 + 7, dev->config.raw + 28, 4);
    result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK1, dev->config.raw + 32, 32);
    for(uint8_t i = 0; i < 32; i += 4) {
        if(i != 20) {   //data[84..87] not writeable
            result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK2 + (i / 4), dev->config.raw + 64 + i, 4);
        }
    }
    result += atecc608_write(dev, ATECC608_ZONE_CONFIG, ATECC608_CONFIG_BLOCK3, dev->config.raw + 96, 32);
    
    return result;
}


uint8_t atecc608_get_locked(atecc608_t *dev) {
    assert(dev);

    return ((dev->config.fields.lock_config == 0x00) && (dev->config.fields.lock_value == 0x00));
}

uint8_t atecc608_get_config_locked(atecc608_t *dev) {
    assert(dev);
    return (dev->config.fields.lock_config == 0x00);
}

uint8_t atecc608_get_data_locked(atecc608_t *dev) {
    assert(dev);
    return (dev->config.fields.lock_value == 0x00);
}

uint8_t atecc608_get_slot_locked(atecc608_t *dev, uint8_t slot) {
    assert(dev);
    assert(slot < 16);

    uint8_t result;
    if(slot < 8) {
        result = (dev->config.fields.slot_locked[0] & (1 << slot));
    }
    else {
        result = (dev->config.fields.slot_locked[1] & (1 << (slot - 8)));
    }

    return (result == 0);
}

uint32_t atecc608_lock_config(atecc608_t *dev) {
    uint32_t result = 0;
    assert(dev);
    result = atecc608_send_receive_command(dev, ATECC608_REG_LOCK, 0x80 | ATECC608_ZONE_CONFIG, 0, 0, 0, 0, 1);
    result += atecc608_read_conf(dev);
    return result;
}

uint32_t atecc608_lock_data(atecc608_t *dev) {
    uint32_t result = 0;
    assert(dev);
    result = atecc608_send_receive_command(dev, ATECC608_REG_LOCK, 0x80 | ATECC608_ZONE_DATA, 0, 0, 0, 0, 1);
    result += atecc608_read_conf(dev);
    return result;
}

uint32_t atecc608_lock_slot(atecc608_t *dev, uint8_t slot) {
    assert(dev);
    assert(slot < 16);
    
    uint32_t result = 0;
    
    result = atecc608_send_receive_command(dev, ATECC608_REG_LOCK, (slot << 2) | ATECC608_ZONE_SLOT, 0, 0, 0, 0, 1);
    result += atecc608_read_conf(dev);
    return result;
}


/*
    internal functions
*/

static void atecc608_crc(const uint8_t *data, uint32_t length, uint8_t *crc_le) {
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t data_bit, crc_bit;

    for (uint32_t counter = 0; counter < length; counter++) {
        for (uint8_t shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
            data_bit = (data[counter] & shift_register) ? 1 : 0;
            crc_bit = crc_register >> 15;
            crc_register <<= 1;
            if (data_bit != crc_bit) {
                crc_register ^= polynom;
            }
        }
    }

    crc_le[0] = (uint8_t)(crc_register & 0x00FF);
    crc_le[1] = (uint8_t)(crc_register >> 8);
}

static uint8_t atecc608_exec_time(const uint8_t command) {
    switch(command) {
        case ATECC608_REG_CHECKMAC:
            return 13;
            break;

        case ATECC608_REG_COUNTER:
            return 20;
            break;

        case ATECC608_REG_DERIVE_KEY:
            return 50;
            break;

        case ATECC608_REG_ECDH:
            return 58;
            break;

        case ATECC608_REG_GENDIG:
            return 11;
            break;

        case ATECC608_REG_GENKEY:
            return 115;
            break;

        case ATECC608_REG_HMAC:
            return 23;
            break;

        case ATECC608_REG_INFO:
            return 2;
            break;

        case ATECC608_REG_LOCK:
            return 32;
            break;

        case ATECC608_REG_MAC:
            return 14;
            break;

        case ATECC608_REG_NONCE:
            return 29;
            break;

        case ATECC608_REG_PAUSE:
            return 3;
            break;

        case ATECC608_REG_PRIVWRITE:
            return 48;
            break;

        case ATECC608_REG_RANDOM:
            return 23;
            break;

        case ATECC608_REG_READ:
            return 5;
            break;

        case ATECC608_REG_SHA:
            return 9;
            break;

        case ATECC608_REG_SIGN:
            return 60;
            break;

        case ATECC608_REG_UPDATE_EXTRA:
            return 10;
            break;

        case ATECC608_REG_VERIFY:
            return 72;
            break;

        case ATECC608_REG_WRITE:
            return 26;
            break;

        default:
            assert(0);
            return 255;
            break;
    }
}

static uint32_t atecc608_send_receive_command(atecc608_t *dev, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *tx_data, uint8_t tx_length, uint8_t *rx_data, uint8_t rx_length) {
    uint32_t result = 0;

    if(result == 0) {
        dev->command.word_address = ATECC608_WA_COMMAND;
        dev->command.count = 7 + tx_length;
        dev->command.opcode = opcode;
        dev->command.param1 = param1;
        dev->command.param2 = param2;

        for(uint8_t i = 0; i < tx_length; i++) {
            dev->command.data[i] = tx_data[i];
        }

        atecc608_crc(&dev->command.count, dev->command.count - 2, &dev->command.count + dev->command.count - 2);
        result += i2c_master_write(dev->i2c, dev->i2c_address, &dev->command.word_address, dev->command.count + 1);

        uint8_t exec_time = atecc608_exec_time(dev->command.opcode);
        task_sleep(exec_time + 1);

        if((result == 0) && (rx_length != 0)) {
            result += i2c_master_read(dev->i2c, dev->i2c_address, &dev->response.count, rx_length + 3);

            if(result == 0) {
                uint8_t rx_crc[2];
                atecc608_crc(&dev->response.count, dev->response.count - 2, rx_crc);

                if(dev->response.count != rx_length + 3) {
                    result += 1;
                }

                if((rx_crc[0] != dev->response.data[dev->response.count - 3]) || (rx_crc[1] != dev->response.data[dev->response.count - 2])) {
                    result += 1;
                }

                if((result == 0) && (rx_data != 0)) {
                    for(uint8_t i = 0; i < rx_length; i++) {
                        rx_data[i] = dev->response.data[i];
                    }
                }
            }
        }
    }

    return result;
}

static uint32_t atecc608_send_command(atecc608_t *dev, uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *tx_data, uint8_t tx_length) {
    uint32_t result;

    dev->command.word_address = ATECC608_WA_COMMAND;
    dev->command.count = 7 + tx_length;
    dev->command.opcode = opcode;
    dev->command.param1 = param1;
    dev->command.param2 = param2;

    for(uint8_t i = 0; i < tx_length; i++) {
        dev->command.data[i] = tx_data[i];
    }

    atecc608_crc(&dev->command.count, dev->command.count - 2, &dev->command.count + dev->command.count - 2);
    result = i2c_master_write(dev->i2c, dev->i2c_address, &dev->command.word_address, dev->command.count + 1);

    uint8_t exec_time = atecc608_exec_time(dev->command.opcode);
    task_sleep(exec_time + 1);

    return result;
}

static uint32_t atecc608_receive_response(atecc608_t *dev, uint8_t *rx_data, uint8_t rx_length) {
    uint32_t result = 0;

    if(rx_length != 0) {
        result += i2c_master_read(dev->i2c, dev->i2c_address, &dev->response.count, rx_length + 3);
        
        if(result == 0) {
            uint8_t rx_crc[2];
            atecc608_crc(&dev->response.count, dev->response.count - 2, rx_crc);

            if(dev->response.count != rx_length + 3) {
                result += 1;
            }

            if((rx_crc[0] != dev->response.data[dev->response.count - 3]) || (rx_crc[1] != dev->response.data[dev->response.count - 2])) {
                result += 1;
            }

            if((result == 0) && (rx_data != 0)) {
                for(uint8_t i = 0; i < rx_length; i++) {
                    rx_data[i] = dev->response.data[i];
                }
            }
        }
    }

    return result;
}

static uint32_t atecc608_write(atecc608_t *dev, uint8_t zone, uint16_t address, uint8_t *data, uint8_t length) {
    uint32_t result = 0;

    assert((length == 4) || (length == 32));

    if(length == 32) {
        zone |= 0x80;
    }

    result += atecc608_send_receive_command(dev, ATECC608_REG_WRITE, zone, address, data, length, 0, 1);
    result += (dev->response.data[0] != ATECC608_RESPONSE_SUCCESS);

    return result;
}

static uint32_t atecc608_read(atecc608_t *dev, uint8_t zone, uint16_t address, uint8_t *data, uint8_t length) {
    assert((length == 4) || (length == 32));

    if(length == 32) {
        zone |= 0x80;
    }

    return atecc608_send_receive_command(dev, ATECC608_REG_READ, zone, address, 0, 0, data, length);
}
