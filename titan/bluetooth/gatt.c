#pragma once

#include "gatt.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <error.h>
#include "bluetooth.h"

TITAN_DEBUG_FILE_MARK;

void gatt_uuid_set(gatt_uuid_t *uuid, gatt_uuid_type_t type, const char *str_uuid) {
    assert(uuid);
    assert(str_uuid);
    assert((strlen(str_uuid) == 4) || (strlen(str_uuid) == 32));

    int k = 0;
    uuid->value_type = type;
    for(int i = 0; i < (int)type; i++) {
        char nibble[3];
        nibble[0] = str_uuid[k++];
        nibble[1] = str_uuid[k++];
        nibble[2] = 0;

        uuid->value[i] = strtoul(nibble, 0, 16);
    }
}

void gatt_uuid_to_string(const gatt_uuid_t *uuid, char *str_uuid) {
    assert(uuid);
    assert(str_uuid);

    str_uuid[0] = 0;
    for(int i = 0; i < (int)uuid->value_type; i++) {
        char nibble[3];
        sprintf(nibble, "%02X", uuid->value[i]);
        strcat(str_uuid, nibble);
    }
}

void gatt_uuid_to_string_reverse(const gatt_uuid_t *uuid, char *rev_uuid) {
    assert(uuid);
    assert(rev_uuid);

    rev_uuid[0] = 0;
    for(int i = (int)uuid->value_type - 1; i >= 0; i--) {
        char nibble[3];
        sprintf(nibble, "%02X", uuid->value[i]);
        strcat(rev_uuid, nibble);
    }
}

void gatt_characteristic_init(gatt_characteristic_t *characteristic, const gatt_uuid_t *uuid, gatt_char_property_t properties, gatt_char_encryption_type_t read_security, gatt_char_encryption_type_t write_security, gatt_server_callback_t callback) {
    assert(characteristic);
    assert(uuid);
    assert(callback);

    characteristic->handle = 0;
    characteristic->cccd_handle = 0;
    characteristic->cccd_value = 0;

    if(properties & GATT_CHAR_PROPERTY_INDICATE) {
        semaphore_create(&characteristic->indication_sem, 0);
    }

    characteristic->uuid = *uuid;
    characteristic->properties = properties;
    characteristic->read_security = read_security;
    characteristic->write_security = write_security;
    characteristic->callback = callback;
}

void gatt_service_init(gatt_service_t *service, const gatt_uuid_t *uuid, gatt_characteristic_t *characteristic, uint32_t characteristic_len) {
    assert(service);
    assert(uuid);
    assert(characteristic);
    assert(characteristic_len);

    service->handle = 0;
    service->uuid = *uuid;
    service->characteristic = characteristic;
    service->characteristic_len = characteristic_len;
}

uint32_t gatt_server_start(void *iface, gatt_service_t *service, uint32_t service_len) {
    assert(iface);
    assert(service);
    assert(service_len);

    btif_t *interface = iface;
    gatt_server_t *server = interface->gatt_server;

    if(server == 0) {
        return ENOSUP;
    }

    if(server->gatt_server_start == 0) {
        return ENOSUP;
    }

    server->service = service;
    server->service_len = service_len;
    return server->gatt_server_start(interface->dev);
}

uint32_t gatt_server_respond_to_read(void *iface, uint32_t conn_handle, const uint8_t *value, uint8_t value_len) {
    assert(iface);
    assert(value);
    assert(value_len);

    btif_t *interface = iface;

    if(interface->gatt_server->gatt_server_respond_to_read == 0) {
        return ENOSUP;
    }
 
    return interface->gatt_server->gatt_server_respond_to_read(interface->dev, conn_handle, value, value_len);
}

uint32_t gatt_server_send_notification(void *iface, uint32_t conn_handle, gatt_characteristic_t *characteristic, const uint8_t *value, uint8_t value_len) {
    assert(iface);
    assert(characteristic);
    assert(value);
    assert(value_len);

    btif_t *interface = iface;

    if(interface->gatt_server->gatt_server_send_notification == 0) {
        return ENOSUP;
    }

    if((characteristic->cccd_value & GATT_CCCD_NOTIFY_ENABLED) == 0) {
        return EAGAIN;
    }

    return interface->gatt_server->gatt_server_send_notification(interface->dev, conn_handle, characteristic->handle, value, value_len);
}

uint32_t gatt_server_send_indication(void *iface, uint32_t conn_handle, gatt_characteristic_t *characteristic, const uint8_t *value, uint8_t value_len, uint32_t timeout) {
    assert(iface);
    assert(characteristic);
    assert(value);
    assert(value_len);

    btif_t *interface = iface;

    if(interface->gatt_server->gatt_server_send_indication == 0) {
        return ENOSUP;
    }

    if((characteristic->cccd_value & GATT_CCCD_INDICATE_ENABLED) == 0) {
        return EAGAIN;
    }

    uint32_t rc = interface->gatt_server->gatt_server_send_indication(interface->dev, conn_handle, characteristic->handle, value, value_len);
    if(rc != EOK) {
        return rc;
    }

    if(semaphore_lock_try(&characteristic->indication_sem, timeout)) {
        return EOK;
    }
    else {
        return ETIMEOUT;
    }
}
