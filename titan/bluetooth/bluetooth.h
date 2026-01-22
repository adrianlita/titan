#pragma once

#include <stdint.h>
#include <task.h>
#include <net/netif_common.h>
#include "bluetooth_sync_buffer.h"
#include "gatt.h"

typedef enum {
    BLUETOOTH_DISCOVERABILITY_UNKNOWN,
    BLUETOOTH_DISCOVERABILITY_LIMITED,
    BLUETOOTH_DISCOVERABILITY_GENERAL,
    BLUETOOTH_DISCOVERABILITY_DISABLED,
} bluetooth_discoverability_mode_t;

typedef enum {
    BLUETOOTH_CONNECTABILITY_UNKNOWN,
    BLUETOOTH_CONNECTABILITY_ENABLED,
    BLUETOOTH_CONNECTABILITY_DISABLED,
} bluetooth_connectability_mode_t;

typedef enum {
    BLUETOOTH_PAIRING_UNKNOWN,
    BLUETOOTH_PAIRING_ENABLED,
    BLUETOOTH_PAIRING_DISABLED,
} bluetooth_pairing_mode_t;

typedef enum {
    BLUETOOTH_SECURITY_MODE_UNKNOWN,
    BLUETOOTH_SECURITY_MODE_1_DISABLED,
    BLUETOOTH_SECURITY_MODE_2_JUST_WORKS,
    BLUETOOTH_SECURITY_MODE_3_DISPLAY_ONLY,
    BLUETOOTH_SECURITY_MODE_4_DISPLAY_YES_NO,
    BLUETOOTH_SECURITY_MODE_5_KEYBOARD_ONLY,
    BLUETOOTH_SECURITY_MODE_6_OOB,
} bluetooth_security_mode_t;

typedef enum {
    BLUETOOTH_LOW_ENERGY_UNKNOWN,
    BLUETOOTH_LOW_ENERGY_UNSUPPORTED,
    BLUETOOTH_LOW_ENERGY_CENTRAL,
    BLUETOOTH_LOW_ENERGY_PERIPHERAL,
    BLUETOOTH_LOW_ENERGY_CENTRAL_PERIPHERAL,
    BLUETOOTH_LOW_ENERGY_DISABLED,
} bluetooth_low_energy_role_t;

typedef enum {
    BLUETOOTH_ADDRESS_TYPE_UNKNOWN = 0,
    BLUETOOTH_ADDRESS_TYPE_PUBLIC = 1,
    BLUETOOTH_ADDRESS_TYPE_RANDOM = 2,
} bluetooth_address_type_t;

typedef struct {
    netif_mac_address_t address;
    bluetooth_address_type_t type;
} bluetooth_address_t;

typedef void (*bluetooth_on_connected_t)(void *interface, uint32_t conn_handle, const bluetooth_address_t *addr);
typedef void (*bluetooth_on_disconnected_t)(void *interface, uint32_t conn_handle);

typedef struct {
    //set by driver_init()
    void *dev;  //actual driver object
    uint32_t (*bluetooth_init)(void *dev);
    void (*bluetooth_deinit)(void *dev);

    uint32_t (*bluetooth_get_mac_address)(void *device, netif_mac_address_t *mac_address);
    uint32_t (*bluetooth_set_mac_address)(void *device, const netif_mac_address_t *mac_address);

    uint32_t (*bluetooth_get_discoverability_mode)(void *device, bluetooth_discoverability_mode_t *discoverability_mode);
    uint32_t (*bluetooth_set_discoverability_mode)(void *device, bluetooth_discoverability_mode_t discoverability_mode);
    uint32_t (*bluetooth_get_connectability_mode)(void *device, bluetooth_connectability_mode_t *connectability_mode);
    uint32_t (*bluetooth_set_connectability_mode)(void *device, bluetooth_connectability_mode_t connectability_mode);
    uint32_t (*bluetooth_get_pairing_mode)(void *device, bluetooth_pairing_mode_t *pairing_mode);
    uint32_t (*bluetooth_set_pairing_mode)(void *device, bluetooth_pairing_mode_t pairing_mode);
    uint32_t (*bluetooth_get_security_mode)(void *device, bluetooth_security_mode_t *security_mode);
    uint32_t (*bluetooth_set_security_mode)(void *device, bluetooth_security_mode_t security_mode);
    uint32_t (*bluetooth_get_local_name)(void *device, char *local_name);
    uint32_t (*bluetooth_set_local_name)(void *device, const char *local_name);
    uint32_t (*bluetooth_get_low_energy_role)(void *device, bluetooth_low_energy_role_t *low_energy_role);
    uint32_t (*bluetooth_set_low_energy_role)(void *device, bluetooth_low_energy_role_t low_energy_role);

    uint32_t (*bluetooth_le_periph_set_advertising_service)(void *dev, const gatt_uuid_t *uuid);


    //set by bluetooth service
    task_t *bluetooth_task;
    uint32_t bluetooth_task_notification;
    bluetooth_sync_buffer_t sync;

    bluetooth_on_connected_t bluetooth_on_connected;
    bluetooth_on_disconnected_t bluetooth_on_disconnected;

    gatt_server_t *gatt_server;
    gatt_client_t *gatt_client;
} btif_t;

void bluetooth_interface_init(btif_t *interface, bluetooth_sync_buffer_message_t *sync_buffer, uint32_t sync_buffer_size);
uint32_t bluetooth_init(btif_t *interfaces, uint8_t interfaces_size);
void bluetooth_deinit(void);

void bluetooth_register_connection_change_callbacks(btif_t *interface, bluetooth_on_connected_t bluetooth_on_connected, bluetooth_on_disconnected_t bluetooth_on_disconnected);

uint32_t bluetooth_get_mac_address(btif_t *interface, netif_mac_address_t *mac_address);
uint32_t bluetooth_set_mac_address(btif_t *interface, const netif_mac_address_t *mac_address);
uint32_t bluetooth_get_discoverability_mode(btif_t *interface, bluetooth_discoverability_mode_t *discoverability_mode);
uint32_t bluetooth_set_discoverability_mode(btif_t *interface, bluetooth_discoverability_mode_t discoverability_mode);
uint32_t bluetooth_get_connectability_mode(btif_t *interface, bluetooth_connectability_mode_t *connectability_mode);
uint32_t bluetooth_set_connectability_mode(btif_t *interface, bluetooth_connectability_mode_t connectability_mode);
uint32_t bluetooth_get_pairing_mode(btif_t *interface, bluetooth_pairing_mode_t *pairing_mode);
uint32_t bluetooth_set_pairing_mode(btif_t *interface, bluetooth_pairing_mode_t pairing_mode);
uint32_t bluetooth_get_security_mode(btif_t *interface, bluetooth_security_mode_t *security_mode);
uint32_t bluetooth_set_security_mode(btif_t *interface, bluetooth_security_mode_t security_mode);
uint32_t bluetooth_get_local_name(btif_t *interface, char *local_name);
uint32_t bluetooth_set_local_name(btif_t *interface, const char *local_name);
uint32_t bluetooth_get_low_energy_role(btif_t *interface, bluetooth_low_energy_role_t *low_energy_role);
uint32_t bluetooth_set_low_energy_role(btif_t *interface, bluetooth_low_energy_role_t low_energy_role);

//checkAL refacem
uint32_t bluetooth_le_periph_set_advertising_service(btif_t *interface, const gatt_uuid_t *uuid);

/*
    usage:
    - then run bluetooth_interface_init for all interfaces
    - first run driver*_init() for all bluetooth drivers
    - then run bluetooth_init with the list of available interfaces
*/