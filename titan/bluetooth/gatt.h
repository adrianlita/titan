#pragma once

#include <stdint.h>
#include <semaphore.h>

typedef enum {
    GATT_UUID_16BIT = 2,
    GATT_UUID_128BIT = 16,
} gatt_uuid_type_t;

typedef struct {
    gatt_uuid_type_t value_type;
    uint8_t value[16];
} gatt_uuid_t;

#define GATT_CHAR_PROPERTY_BROADCAST                0x01
#define GATT_CHAR_PROPERTY_READ                     0x02
#define GATT_CHAR_PROPERTY_WRITE_WITHOUT_RESPONSE   0x04
#define GATT_CHAR_PROPERTY_WRITE                    0x08
#define GATT_CHAR_PROPERTY_NOTIFY                   0x10
#define GATT_CHAR_PROPERTY_INDICATE                 0x20
#define GATT_CHAR_PROPERTY_SIGNED_WRITE             0x40
typedef uint8_t gatt_char_property_t;

typedef enum {
    GATT_ENCRYPTION_TYPE_NONE = 0,
    GATT_ENCRYPTION_TYPE_UNAUTHENTICATED = 1,
    GATT_ENCRYPTION_TYPE_AUTHENTICATED = 2,
    GATT_ENCRYPTION_TYPE_AUTHENTICATED_LESC = 3,
} gatt_char_encryption_type_t;

typedef enum {
    GATT_WRITE_OPTIONS_UNKNOWN = 0,
    GATT_WRITE_OPTIONS_WRITE_WITHOUT_RESPONSE = 1,
    GATT_WRITE_OPTIONS_WRITE = 2,
    GATT_WRITE_OPTIONS_WRITE_LONG = 3,
} gatt_write_options_t;

typedef void (*gatt_server_callback_t)(void *interface, uint32_t conn_handle, gatt_uuid_t *uuid, const uint8_t *value, uint8_t value_len, gatt_write_options_t options);

#define GATT_CCCD_NOTIFY_ENABLED                0x01
#define GATT_CCCD_INDICATE_ENABLED              0x02

typedef struct {
    uint32_t handle;
    uint32_t cccd_handle;
    uint16_t cccd_value;
    semaphore_t indication_sem;

    gatt_uuid_t uuid;
    gatt_char_property_t properties;
    gatt_char_encryption_type_t read_security;
    gatt_char_encryption_type_t write_security;
    gatt_server_callback_t callback;
} gatt_characteristic_t;

typedef struct {
    uint32_t handle;

    gatt_uuid_t uuid;
    gatt_characteristic_t *characteristic;
    uint8_t characteristic_len;
} gatt_service_t;

typedef struct gatt_server_s {
    uint32_t (*gatt_server_start)(void *dev);
    uint32_t (*gatt_server_respond_to_read)(void *dev, uint32_t conn_handle, const uint8_t *value, uint8_t value_len);
    uint32_t (*gatt_server_send_notification)(void *dev, uint32_t conn_handle, uint32_t char_handle, const uint8_t *value, uint8_t value_len);
    uint32_t (*gatt_server_send_indication)(void *dev, uint32_t conn_handle, uint32_t char_handle, const uint8_t *value, uint8_t value_len);

    gatt_service_t *service;
    uint8_t service_len;
} gatt_server_t;

typedef struct {
  uint8_t checkAL;
} gatt_client_t;

void gatt_uuid_set(gatt_uuid_t *uuid, gatt_uuid_type_t type, const char *str_uuid);
void gatt_uuid_to_string(const gatt_uuid_t *uuid, char *str_uuid);
void gatt_uuid_to_string_reverse(const gatt_uuid_t *uuid, char *rev_uuid);

void gatt_characteristic_init(gatt_characteristic_t *characteristic, const gatt_uuid_t *uuid, gatt_char_property_t properties, gatt_char_encryption_type_t read_security, gatt_char_encryption_type_t write_security, gatt_server_callback_t callback);
void gatt_service_init(gatt_service_t *service, const gatt_uuid_t *uuid, gatt_characteristic_t *characteristic, uint32_t characteristic_len);

uint32_t gatt_server_start(void *iface, gatt_service_t *service, uint32_t service_len);
uint32_t gatt_server_respond_to_read(void *iface, uint32_t conn_handle, const uint8_t *value, uint8_t value_len);
uint32_t gatt_server_send_notification(void *iface, uint32_t conn_handle, gatt_characteristic_t *characteristic, const uint8_t *value, uint8_t value_len);
uint32_t gatt_server_send_indication(void *iface, uint32_t conn_handle, gatt_characteristic_t *characteristic, const uint8_t *value, uint8_t value_len, uint32_t timeout);
