#include "bluetooth.h"
#include <task.h>
#include <notification.h>
#include <string.h>
#include <assert.h>
#include <error.h>

TITAN_DEBUG_FILE_MARK;

#define BLUETOOTH_TASK_STACK_SIZE   1024

static task_stack_t bluetooth_task_stack[BLUETOOTH_TASK_STACK_SIZE] in_task_stack_memory;
static task_t bluetooth_task;

static btif_t *btif;
static uint8_t btif_len;

static void bluetooth_main(void) {
    notification_set_active(0xFFFFFFFF);
    uint32_t pending = 0;
    while(1) {
        pending = notification_wait();
        for(int i = 0; i < btif_len; i++) {
            if(pending & btif[i].bluetooth_task_notification) {
                while(bluetooth_sync_buffer_has_elements(&btif[i].sync)) {
                    bluetooth_sync_buffer_message_t *message = bluetooth_sync_buffer_extract_get_element(&btif[i].sync);
                    uint8_t found = 0;
                    switch(message->type) {
                        case BLUETOOTH_SYNC_BUFFER_CLIENT_CONNECTED:
                            if(btif[i].bluetooth_on_connected) {
                                btif[i].bluetooth_on_connected(&btif[i], message->conn_handle, (const bluetooth_address_t *)message->value);
                            }
                            break;

                        case BLUETOOTH_SYNC_BUFFER_CLIENT_DISCONNECTED:
                            if(btif[i].bluetooth_on_disconnected) {
                                btif[i].bluetooth_on_disconnected(&btif[i], message->conn_handle);
                            }
                            if(btif[i].gatt_server) {
                                for(int j = 0; j < btif[i].gatt_server->service_len; j++) {
                                    for(int k = 0; j < btif[i].gatt_server->service[j].characteristic_len; k++) {
                                        btif[i].gatt_server->service[j].characteristic[k].cccd_value = 0;
                                    }
                                }
                            }
                            break;

                        case BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_READ_REQUEST:
                            for(int j = 0; j < btif[i].gatt_server->service_len; j++) {
                                for(int k = 0; j < btif[i].gatt_server->service[j].characteristic_len; k++) {
                                    if(btif[i].gatt_server->service[j].characteristic[k].handle == message->param1) {
                                        btif[i].gatt_server->service[j].characteristic[k].callback(&btif[i], message->conn_handle, &btif[i].gatt_server->service[j].characteristic[k].uuid, 0, 0, GATT_WRITE_OPTIONS_UNKNOWN);
                                        found = 1;
                                        break;
                                    }
                                }
                                if(found) {
                                    break;
                                }
                            }
                            break;

                        case BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_WRITE_REQUEST:
                            for(int j = 0; j < btif[i].gatt_server->service_len; j++) {
                                for(int k = 0; j < btif[i].gatt_server->service[j].characteristic_len; k++) {
                                    if(btif[i].gatt_server->service[j].characteristic[k].handle == message->param1) {
                                        btif[i].gatt_server->service[j].characteristic[k].callback(&btif[i], message->conn_handle, &btif[i].gatt_server->service[j].characteristic[k].uuid, message->value, message->value_size, (gatt_write_options_t)message->param2);
                                        found = 1;
                                        break;
                                    }
                                    if(btif[i].gatt_server->service[j].characteristic[k].cccd_handle == message->param1) {
                                        btif[i].gatt_server->service[j].characteristic[k].cccd_value = message->value[0] + 256 * message->value[1];
                                        found = 1;
                                        break;
                                    }
                                }
                                if(found) {
                                    break;
                                }
                            }
                            break;

                        case BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_INDICATION_CONFIRMATION:
                            for(int j = 0; j < btif[i].gatt_server->service_len; j++) {
                                for(int k = 0; j < btif[i].gatt_server->service[j].characteristic_len; k++) {
                                    if(btif[i].gatt_server->service[j].characteristic[k].handle == message->param1) {
                                        semaphore_unlock(&btif[i].gatt_server->service[j].characteristic[k].indication_sem);
                                        found = 1;
                                        break;
                                    }
                                }
                                if(found) {
                                    break;
                                }
                            }
                            break;
                    }
                    
                    bluetooth_sync_buffer_extract(&btif[i].sync);
                }
            }
        }
    }
}

void bluetooth_interface_init(btif_t *interface, bluetooth_sync_buffer_message_t *sync_buffer, uint32_t sync_buffer_size) {
    assert(interface);
    assert(sync_buffer);
    assert(sync_buffer_size);

    memset(interface, 0, sizeof(btif_t));

    bluetooth_sync_buffer_init(&interface->sync, sync_buffer, sync_buffer_size);
}

uint32_t bluetooth_init(btif_t *interfaces, uint8_t interfaces_len) {
    assert(interfaces);
    assert(interfaces_len);

    btif_len = 0;

    task_init(&bluetooth_task, bluetooth_task_stack, BLUETOOTH_TASK_STACK_SIZE, 1);
    task_attr_set_start_type(&bluetooth_task, TASK_START_NOW, 0);
    task_create(&bluetooth_task, bluetooth_main);

    btif = interfaces;
    btif_len = interfaces_len;

    for(int i = 0; i < btif_len; i++) {
        btif[i].bluetooth_task = &bluetooth_task;
        btif[i].bluetooth_task_notification = (1 << i);

        //call bluetooth_init from the driver's POV
        if(btif[i].bluetooth_init != 0) {
            if(btif[i].bluetooth_init(btif[i].dev) != EOK) {
                return EIO;
            }
        }
    }
    return EOK;
}

void bluetooth_deinit(void) {
    task_kill(&bluetooth_task);
    for(int i = 0; i < btif_len; i++) {
        if(btif[i].bluetooth_deinit) {
            btif[i].bluetooth_deinit(btif[i].dev);
        }
    }
}

void bluetooth_register_connection_change_callbacks(btif_t *interface, bluetooth_on_connected_t bluetooth_on_connected, bluetooth_on_disconnected_t bluetooth_on_disconnected) {
    interface->bluetooth_on_connected = bluetooth_on_connected;
    interface->bluetooth_on_disconnected = bluetooth_on_disconnected;
}

uint32_t bluetooth_get_mac_address(btif_t *interface, netif_mac_address_t *mac_address) {
    assert(interface);
    assert(mac_address);

    if(interface->bluetooth_get_mac_address) {
        return interface->bluetooth_get_mac_address(interface->dev, mac_address);

    }
    else {
        return ENOSUP;
    }
}
uint32_t bluetooth_set_mac_address(btif_t *interface, const netif_mac_address_t *mac_address) {
    assert(interface);
    assert(mac_address);

    if(interface->bluetooth_get_mac_address) {
        return interface->bluetooth_set_mac_address(interface->dev, mac_address);

    }
    else {
        return ENOSUP;
    }
}


uint32_t bluetooth_get_discoverability_mode(btif_t *interface, bluetooth_discoverability_mode_t *discoverability_mode) {
    assert(interface);

    if(interface->bluetooth_get_discoverability_mode) {
        return interface->bluetooth_get_discoverability_mode(interface->dev, discoverability_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_set_discoverability_mode(btif_t *interface, bluetooth_discoverability_mode_t discoverability_mode) {
    assert(interface);

    if(interface->bluetooth_set_discoverability_mode) {
        return interface->bluetooth_set_discoverability_mode(interface->dev, discoverability_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_get_connectability_mode(btif_t *interface, bluetooth_connectability_mode_t *connectability_mode) {
    assert(interface);

    if(interface->bluetooth_get_connectability_mode) {
        return interface->bluetooth_get_connectability_mode(interface->dev, connectability_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_set_connectability_mode(btif_t *interface, bluetooth_connectability_mode_t connectability_mode) {
    assert(interface);

    if(interface->bluetooth_set_connectability_mode) {
        return interface->bluetooth_set_connectability_mode(interface->dev, connectability_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_get_pairing_mode(btif_t *interface, bluetooth_pairing_mode_t *pairing_mode) {
    assert(interface);

    if(interface->bluetooth_get_pairing_mode) {
        return interface->bluetooth_get_pairing_mode(interface->dev, pairing_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_set_pairing_mode(btif_t *interface, bluetooth_pairing_mode_t pairing_mode) {
    assert(interface);

    if(interface->bluetooth_set_pairing_mode) {
        return interface->bluetooth_set_pairing_mode(interface->dev, pairing_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_get_security_mode(btif_t *interface, bluetooth_security_mode_t *security_mode) {
    assert(interface);

    if(interface->bluetooth_get_security_mode) {
        return interface->bluetooth_get_security_mode(interface->dev, security_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_set_security_mode(btif_t *interface, bluetooth_security_mode_t security_mode) {
    assert(interface);

    if(interface->bluetooth_set_security_mode) {
        return interface->bluetooth_set_security_mode(interface->dev, security_mode);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_get_local_name(btif_t *interface, char *local_name) {
    assert(interface);

    if(interface->bluetooth_get_local_name) {
        return interface->bluetooth_get_local_name(interface->dev, local_name);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_set_local_name(btif_t *interface, const char *local_name) {
    assert(interface);

    if(interface->bluetooth_set_local_name) {
        return interface->bluetooth_set_local_name(interface->dev, local_name);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_get_low_energy_role(btif_t *interface, bluetooth_low_energy_role_t *low_energy_role) {
    assert(interface);

    if(interface->bluetooth_get_low_energy_role) {
        return interface->bluetooth_get_low_energy_role(interface->dev, low_energy_role);

    }
    else {
        return ENOSUP;
    }
}

uint32_t bluetooth_set_low_energy_role(btif_t *interface, bluetooth_low_energy_role_t low_energy_role) {
    assert(interface);

    if(interface->bluetooth_set_low_energy_role) {
        return interface->bluetooth_set_low_energy_role(interface->dev, low_energy_role);

    }
    else {
        return ENOSUP;
    }
}

//checkAL refacem
uint32_t bluetooth_le_periph_set_advertising_service(btif_t *interface, const gatt_uuid_t *uuid) {
    assert(interface);
    assert(uuid);
 
    return interface->bluetooth_le_periph_set_advertising_service(interface->dev, uuid);
}
