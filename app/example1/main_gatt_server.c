#include <titan.h>
#include <stdint.h>
#include <string.h>
#include <task.h>
#include <notification.h>
#include "bsp.h"
#include <periph/gpio.h>

#include <utils/explorer/explorer.h>

#include <nina_b312/nina_b312.h>
#include <bluetooth/bluetooth.h>

#include <stdio.h>

static nina_b312_t nina_b312;
static btif_t ninaif;
static bluetooth_sync_buffer_message_t ninaif_buffer[4];

static volatile uint8_t connected = 0;
static volatile uint32_t c_handle = 0;

void ble_on_connected(void *iface, uint32_t conn_handle, const bluetooth_address_t *address) {
    char addr[NETIF_MAC_ADDRESS_PACKED_SIZE + 1];
    netif_mac_address_to_string_packed(&address->address, addr);

    explorer_log("BLE client connected (%d) %s\r\n", conn_handle, addr);
    c_handle = conn_handle;
    connected = 1;

}

void ble_on_disconnected(void *iface, uint32_t conn_handle) {
    explorer_log("BLE client disconnected (%d)\r\n", conn_handle);
    connected = 0;
}

void ble_on_event(void *iface, uint32_t conn_handle, gatt_uuid_t *uuid, const uint8_t *value, uint8_t value_len, gatt_write_options_t options) {
    btif_t *interface = iface;

    char uuid_str[33];
    gatt_uuid_to_string(uuid, uuid_str);

    if(value != 0) {
        explorer_log("Got write for %s (options:%d): %s\r\n", uuid_str, options, (char*)value);
        if(options == GATT_WRITE_OPTIONS_WRITE) {
            gatt_server_respond_to_read(interface, conn_handle, "ABCD", 4);
        }
    }
    else {
        gatt_uuid_to_string(uuid, uuid_str);
        explorer_log("Got read for: %s\r\n", uuid_str);
        
        extern uint32_t _kernel_tick;
        char val[20];
        sprintf(val, "%lu", _kernel_tick);

        gatt_server_respond_to_read(interface, conn_handle, val, strlen(val));
    }
}

void app_main(void) {
    //nucleo init

    //explorer init
    explorer_init((uart_t)LPUART1, DEBUG_UART_BAUDRATE, DEBUG_UART_TX, DEBUG_UART_RX);
    explorer_log("Starting up...\r\n");



    //nina_b312 init
    bluetooth_interface_init(&ninaif, ninaif_buffer, 4);
    nina_b312_init(&nina_b312, &ninaif, (uart_t)USART3, NINA_RST_PIN, NINA_UART_TX, NINA_UART_RX, NINA_UART_RTS_PIN, NINA_UART_CTS_PIN);
    explorer_log("nina_b312_init finished\r\n");
    
    //create and start ble task
    uint32_t rc = 0;

    rc = bluetooth_init(&ninaif, 1);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    explorer_log("bluetooth_init finished...\r\n");

    bluetooth_register_connection_change_callbacks(&ninaif, ble_on_connected, ble_on_disconnected);

    rc = bluetooth_set_discoverability_mode(&ninaif, BLUETOOTH_DISCOVERABILITY_GENERAL);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    rc = bluetooth_set_connectability_mode(&ninaif, BLUETOOTH_CONNECTABILITY_ENABLED);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    rc = bluetooth_set_pairing_mode(&ninaif, BLUETOOTH_PAIRING_DISABLED);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    rc = bluetooth_set_low_energy_role(&ninaif, BLUETOOTH_LOW_ENERGY_PERIPHERAL);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    rc = bluetooth_set_security_mode(&ninaif, BLUETOOTH_SECURITY_MODE_1_DISABLED);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }

    char name[16];
    netif_mac_address_t mac;
    rc = bluetooth_get_mac_address(&ninaif, &mac);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    char mac_address[17];
    netif_mac_address_to_string_packed(&mac, mac_address);

    explorer_log("mac adddress is : %s\r\n", mac_address);
    
    strcpy(name, "BTTest-");
    strcat(name, mac_address + 6);
    rc = bluetooth_set_local_name(&ninaif, name);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    name[0] = 0;
    rc = bluetooth_get_local_name(&ninaif, name);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }
    explorer_log("local name is: %s\r\n", name);

    gatt_uuid_t service_uuid;
    gatt_uuid_set(&service_uuid, GATT_UUID_128BIT, "ADADBEEFDEADBEEFDEADBEEF00000000");
    
    gatt_characteristic_t gatt_characteristic[6];
    gatt_uuid_t uuid = service_uuid;
    for(int i = 0; i < 5; i++) {
        uuid.value[15] += 1;
        gatt_characteristic_init(&gatt_characteristic[i], &uuid, GATT_CHAR_PROPERTY_READ | GATT_CHAR_PROPERTY_WRITE_WITHOUT_RESPONSE | GATT_CHAR_PROPERTY_WRITE, GATT_ENCRYPTION_TYPE_NONE, GATT_ENCRYPTION_TYPE_NONE, ble_on_event);
    }

    uuid.value[15] += 1;
    gatt_characteristic_init(&gatt_characteristic[5], &uuid, GATT_CHAR_PROPERTY_READ | GATT_CHAR_PROPERTY_INDICATE, GATT_ENCRYPTION_TYPE_NONE, GATT_ENCRYPTION_TYPE_NONE, ble_on_event);

    gatt_service_t gatt_service_main;
    gatt_service_init(&gatt_service_main, &service_uuid, gatt_characteristic, 6);

    rc = gatt_server_start(&ninaif, &gatt_service_main, 1);
    if(rc != 0) {
        explorer_log("error here: %d\r\n", __LINE__);
    }

    bluetooth_le_periph_set_advertising_service(&ninaif, &service_uuid);

    explorer_log("gatt server setup finished\r\n");


    while(1) {
        if(connected) {
            explorer_log("[main] e conectat...\r\n");
            if(gatt_characteristic[5].cccd_value & GATT_CCCD_INDICATE_ENABLED) {
                char val[20];
                sprintf(val, "z-%lu", _kernel_tick);
                uint32_t rc = gatt_server_send_indication(&ninaif, c_handle, &gatt_characteristic[5], val, strlen(val), 1);

                explorer_log("[main] indication a zis %d...\r\n", rc);
            }
        }
        task_sleep(1000);
    }
}
