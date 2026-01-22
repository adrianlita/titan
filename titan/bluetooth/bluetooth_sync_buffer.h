#pragma once

#include <stdint.h>

#define BLUETOOTH_SYNC_BUFFER_VALUE_SIZE   64

typedef enum {
    BLUETOOTH_SYNC_BUFFER_CLIENT_CONNECTED,
    BLUETOOTH_SYNC_BUFFER_CLIENT_DISCONNECTED,
    BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_READ_REQUEST,
    BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_WRITE_REQUEST,
    BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_INDICATION_CONFIRMATION,
} bluetooth_sync_buffer_message_type_t;

typedef struct {
    bluetooth_sync_buffer_message_type_t type;
    uint32_t conn_handle;
    uint32_t param1;    //char_handle
    uint32_t param2;    //options
    uint8_t value[BLUETOOTH_SYNC_BUFFER_VALUE_SIZE];
    uint8_t value_size;
} bluetooth_sync_buffer_message_t;

typedef struct {
    bluetooth_sync_buffer_message_t *message;
    uint32_t in;
    uint32_t out;
    uint32_t size;
} bluetooth_sync_buffer_t;

void bluetooth_sync_buffer_init(bluetooth_sync_buffer_t *sync, bluetooth_sync_buffer_message_t *buffer, uint32_t size);

bluetooth_sync_buffer_message_t *bluetooth_sync_buffer_insert_get_element(bluetooth_sync_buffer_t *sync);
void bluetooth_sync_buffer_insert(bluetooth_sync_buffer_t *sync);

bluetooth_sync_buffer_message_t *bluetooth_sync_buffer_extract_get_element(bluetooth_sync_buffer_t *sync);
void bluetooth_sync_buffer_extract(bluetooth_sync_buffer_t *sync);

uint8_t bluetooth_sync_buffer_has_elements(bluetooth_sync_buffer_t *sync);
