#pragma once

#include <periph/i2c.h>
#include <periph/gpio.h>
#include <mutex.h>

#define ATECC608_DEFAULT_I2C_ADDRESS        0xC0

#define ATECC608_CONF_SIZE                  128
#define ATECC608_RAND_POOL_SIZE             32
#define ATECC608_SERIAL_NUMBER_SIZE         9
#define ATECC608_SHA256_BLOCK_SIZE          32            // SHA256 outputs a 32 byte digest

typedef union {
    uint8_t raw[128];
    struct {
        uint8_t sn03[4];        //0
        uint8_t revnum[4];      //4
        uint8_t sn48[5];        //8
        uint8_t aes_enable;     //13
        uint8_t i2c_enable;     //14
        uint8_t reserved0;      //15
        uint8_t i2c_address;    //16
        uint8_t reserved1;      //17
        uint8_t count_match;       //18
        uint8_t chipmode;       //19
        uint16_t slot_config[16];//20-51
        uint8_t counter0[8];    //52-59
        uint8_t counter1[8];    //60-67
        uint8_t use_lock;   //68
        uint8_t volatile_key_permission;    //69
        uint8_t secure_boot[2]; //70-71
        uint8_t kdf_iv_loc;   //72 - index
        uint8_t kdf_iv_str;   //73
        uint8_t reserved2[10];    //74-83
        uint8_t user_extra; //84
        uint8_t user_extra_addr; //85
        uint8_t lock_value; //86
        uint8_t lock_config; //87
        uint8_t slot_locked[2]; //88-89
        uint8_t chip_options;   //90
        uint8_t reserved3;       //91
        uint8_t x509format[4]; //92-95
        uint16_t key_config[16];  //96-127
    } fields;
} atecc608_config_t;

typedef struct {
    struct {
        uint8_t nomac_flag;
        uint8_t genkey_data;
        uint8_t gendig_data;
        uint8_t source_flag;
        uint8_t key_id;
        uint8_t valid;
    } tempkey;

    uint8_t authkey_id;
    uint8_t auth_valid;
    uint8_t rng_sram;
    uint8_t rng_eeprom;
} atecc608_state_t;

typedef struct { 
    uint8_t   word_address;
    uint8_t   count;
    uint8_t   opcode;
    uint8_t   param1;       // often same as mode
    uint16_t  param2;
    uint8_t   data[130];    // includes 2-byte CRC.  data size is determined by largest possible data section of any
                            // command + crc (see: x08 verify data1 + data2 + data3 + data4)
                            // this is an explicit design trade-off (space) resulting in simplicity in use
                            // and implementation
} atecc608_command_t;

typedef struct { 
  uint8_t   count;
  uint8_t   data[150]; // includes 2-byte CRC.  data size is determined by largest possible data section of any
} atecc608_response_t;

typedef struct __atecc608 {
    gpio_pin_t sda;
    i2c_t i2c;
    mutex_t *i2c_mutex;
    uint8_t i2c_address;

    atecc608_config_t config;

    atecc608_command_t command;
    atecc608_response_t response;
} atecc608_t;

uint32_t atecc608_init(atecc608_t *dev, gpio_pin_t sda_pin, i2c_t i2c, mutex_t *i2c_mutex, uint8_t i2c_address);
uint32_t atecc608_wakeup(atecc608_t *dev);      //wakes up the device and gain mutex. if wakeup fails, mutex is released
void atecc608_release(atecc608_t *dev);         //only releases the mutex. the device will go to IDLE after some time
uint32_t atecc608_idle(atecc608_t *dev);        //put the device to IDLE and release the mutex NOW
uint32_t atecc608_sleep(atecc608_t *dev);       //put the device to SLEEP and release the mutex ow

/* functions below must be guarded with _wakeup and _idle(), _release() or _sleep() */
uint32_t atecc608_get_key_valid(atecc608_t *dev, uint8_t slot, uint8_t *valid);
uint32_t atecc608_get_state(atecc608_t *dev, atecc608_state_t *state);

uint32_t atecc608_rand(atecc608_t *dev, uint8_t *pool);

uint32_t atecc608_sha256_init(atecc608_t *dev);
uint32_t atecc608_sha256_update(atecc608_t *dev, const uint8_t *data, uint32_t length);
uint32_t atecc608_sha256_final(atecc608_t *dev, uint8_t *hash);

//check AL trebuie testat, si tflx merge si zice multe bune
uint32_t atecc608_hmac_init(atecc608_t *dev, uint8_t slot_or_tempkey);  //0-15 is slot, 16 is tempkey
uint32_t atecc608_hmac_update(atecc608_t *dev, const uint8_t *data, uint32_t length);
uint32_t atecc608_hmac_final(atecc608_t *dev, uint8_t *hash, uint8_t store);    //0 only output, 1 tempkey, 2 message digest buffer

uint16_t atecc608_get_slot_size(uint8_t slot);      //returns size in bytes, 0 if invalid slot
uint32_t atecc608_read_slot(atecc608_t *dev, uint8_t slot, uint8_t *data);  //checkAL not sure if works
uint32_t atecc608_write_slot(atecc608_t *dev, uint8_t slot, uint8_t *data); //checkAL not sure if works

uint32_t atecc608_read_conf(atecc608_t *dev);   //checkAL not sure if works
uint32_t atecc608_write_conf(atecc608_t *dev);  //checkAL not sure if works
/* functions above must be guarded with _wakeup and _idle(), _release() or _sleep() */

void atecc608_get_serial_number(atecc608_t *dev, uint8_t *serial_number);   //returns from dev->config

uint8_t atecc608_get_locked(atecc608_t *dev);                       //returns from dev->config
uint8_t atecc608_get_config_locked(atecc608_t *dev);                //returns from dev->config
uint8_t atecc608_get_data_locked(atecc608_t *dev);                  //returns from dev->config
uint8_t atecc608_get_slot_locked(atecc608_t *dev, uint8_t slot);    //returns from dev->config

/* functions below must be guarded with _wakeup and _idle(), _release() or _sleep() */
uint32_t atecc608_lock_config(atecc608_t *dev);                     //careful upon usage as it might brick the device. automatically reloads config
uint32_t atecc608_lock_data(atecc608_t *dev);                       //careful upon usage as it might brick the device. automatically reloads config
uint32_t atecc608_lock_slot(atecc608_t *dev, uint8_t slot);         //careful upon usage as it might brick the device. automatically reloads config

