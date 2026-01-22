#include "nina_b312.h"
#include <task.h>
#include <notification.h>
#include <assert.h>
#include <error.h>

#include <string.h>
#include <stdio.h>

TITAN_DEBUG_FILE_MARK;

static void nina_b312_uart_rx(uint32_t param, uint8_t data) {
    nina_b312_t *dev = (nina_b312_t*)param;

    dev->rx_buffer[dev->rx_buffer_in] = data;
    dev->rx_buffer_in++;
    notification_send(&dev->driver_task, 0x00000001);
}

static uint8_t at_send_command(nina_b312_t *dev, const char *command, nina_b312_rc_t *rc, char *content, uint32_t content_max_length, uint32_t timeout) {
    assert(dev);

    uart_write(dev->uart, (uint8_t*)command, strlen(command));
    explorer_log("%s", command);
    if(semaphore_lock_try(&dev->rx_response_received, timeout) != 0) {
        //got response
        *rc = dev->rx_response;
        if(content) {
            strncpy(content, dev->rx_content, content_max_length);
        }
        dev->rx_content[0] = 0;
        semaphore_unlock(&dev->rx_release);
        return 1;
    }
    else {
        //timed out
        return 0;
    }
}

static uint8_t at_wait_for_unsolicited_response(nina_b312_t *dev, nina_b312_urc_t *rc, char *content, uint32_t content_max_length, uint32_t timeout) {
    assert(dev);

    if(semaphore_lock_try(&dev->rx_unsolicited_received, timeout) != 0) {
        //got response
        *rc = dev->rx_uresponse;
        if(content) {
            strncpy(content, dev->rx_content, content_max_length);
        }
        dev->rx_content[0] = 0;
        semaphore_unlock(&dev->rx_release);
        return 1;
    }
    else {
        //timed out
        return 0;
    }
}

static void nina_b312_main(void *param) {
    assert(param);

    nina_b312_t *dev = param;

    notification_set_active(0x00000001);
    while(1) {
        notification_wait();
        
        //we should have data here
        while(dev->rx_buffer_out != dev->rx_buffer_in) {
            explorer_log("%c", dev->rx_buffer[dev->rx_buffer_out]);
            dev->rx_command[dev->rx_command_length] = dev->rx_buffer[dev->rx_buffer_out];
            dev->rx_buffer_out++;
            dev->rx_command_length++;

            if(dev->rx_command_length > 1) {
                if(dev->rx_command[dev->rx_command_length - 1] == '\n') {
                    if(dev->rx_command[dev->rx_command_length - 2] == '\r') {
                        dev->rx_command[dev->rx_command_length] = 0;
                        //process the message
                        //unsolicited messages
                        if(strcmp(dev->rx_command, "+STARTUP\r\n") == 0) {
                            dev->rx_uresponse = NINA_B312_STARTUP;
                            if(semaphore_get_waiting_queue(&dev->rx_unsolicited_received)) {
                                semaphore_unlock(&dev->rx_unsolicited_received);
                                semaphore_lock(&dev->rx_release);
                            }
                        }
                        else if(strncmp(dev->rx_command, "+UUBTGRR:", 9) == 0) {
                            //got a read request
                            bluetooth_sync_buffer_message_t *message = bluetooth_sync_buffer_insert_get_element(&dev->btif->sync);
                            message->type = BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_READ_REQUEST;
                            message->conn_handle = 0;
                            message->param1 = 0;

                            int i = 9;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                message->conn_handle *= 10;
                                message->conn_handle += dev->rx_command[i] - '0';
                                i++;
                            }

                            i++;
                            while((dev->rx_command[i] != '\r') && (i < sizeof(dev->rx_command))) {
                                message->param1 *= 10;
                                message->param1 += dev->rx_command[i] - '0';
                                i++;
                            }

                            bluetooth_sync_buffer_insert(&dev->btif->sync);
                            notification_send(dev->btif->bluetooth_task, dev->btif->bluetooth_task_notification);
                        }
                        else if(strncmp(dev->rx_command, "+UUBTGRW:", 9) == 0) {
                            //got a write request
                            bluetooth_sync_buffer_message_t *message = bluetooth_sync_buffer_insert_get_element(&dev->btif->sync);
                            message->type = BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_WRITE_REQUEST;
                            message->conn_handle = 0;
                            message->param1 = 0;
                            message->param2 = 0;

                            int i = 9;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                message->conn_handle *= 10;
                                message->conn_handle += dev->rx_command[i] - '0';
                                i++;
                            }

                            i++;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                message->param1 *= 10;
                                message->param1 += dev->rx_command[i] - '0';
                                i++;
                            }

                            uint8_t j = 0;
                            memset(message->value, 0, BLUETOOTH_SYNC_BUFFER_VALUE_SIZE);
                            i++;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                char c = dev->rx_command[i];
                                message->value[j] *= 16;
                                if(c <= '9') {
                                    message->value[j] += c - '0';
                                }
                                else if(c <= 'F') {
                                    message->value[j] += c - 'A' + 10;
                                }
                                else {
                                    message->value[j] += c - 'f' + 10;
                                }

                                i++;
                                if(i % 2 == 0) {
                                    j++;
                                }
                            }
                            message->value_size = j;

                            i++;
                            int options = 0;
                            while((dev->rx_command[i] != '\r') && (i < sizeof(dev->rx_command))) {
                                options *= 10;
                                options += dev->rx_command[i] - '0';
                                i++;
                            }

                            switch(options) {
                                case 0:
                                    message->param2 = GATT_WRITE_OPTIONS_WRITE_WITHOUT_RESPONSE;
                                    break;

                                case 1:
                                    message->param2 = GATT_WRITE_OPTIONS_WRITE;
                                    break;

                                case 2:
                                    message->param2 = GATT_WRITE_OPTIONS_WRITE_LONG;
                                    break;

                                default:
                                    message->param2 = GATT_WRITE_OPTIONS_UNKNOWN;
                                    break;
                            }

                            bluetooth_sync_buffer_insert(&dev->btif->sync);
                            notification_send(dev->btif->bluetooth_task, dev->btif->bluetooth_task_notification);
                        }
                        else if(strncmp(dev->rx_command, "+UUBTGIC:", 9) == 0) {
                            //got a read request
                            bluetooth_sync_buffer_message_t *message = bluetooth_sync_buffer_insert_get_element(&dev->btif->sync);
                            message->type = BLUETOOTH_SYNC_BUFFER_CLIENT_GATT_INDICATION_CONFIRMATION;
                            message->conn_handle = 0;
                            message->param1 = 0;

                            int i = 9;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                message->conn_handle *= 10;
                                message->conn_handle += dev->rx_command[i] - '0';
                                i++;
                            }

                            i++;
                            while((dev->rx_command[i] != '\r') && (i < sizeof(dev->rx_command))) {
                                message->param1 *= 10;
                                message->param1 += dev->rx_command[i] - '0';
                                i++;
                            }

                            bluetooth_sync_buffer_insert(&dev->btif->sync);
                            notification_send(dev->btif->bluetooth_task, dev->btif->bluetooth_task_notification);
                        }
                        else if(strncmp(dev->rx_command, "+UUBTACLC:", 10) == 0) {
                            bluetooth_sync_buffer_message_t *message = bluetooth_sync_buffer_insert_get_element(&dev->btif->sync);
                            message->type = BLUETOOTH_SYNC_BUFFER_CLIENT_CONNECTED;
                            message->conn_handle = 0;
                            
                            int i = 10;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                message->conn_handle *= 10;
                                message->conn_handle += dev->rx_command[i] - '0';
                                i++;
                            }

                            i++;
                            while((dev->rx_command[i] != ',') && (i < sizeof(dev->rx_command))) {
                                message->param1 *= 10;
                                message->param1 += dev->rx_command[i] - '0';
                                i++;
                            }

                            uint8_t j = 0;
                            char value[32];
                            i++;
                            while((dev->rx_command[i] != 'r') && (dev->rx_command[i] != 'p') && (dev->rx_command[i] != '\r') && (i < sizeof(dev->rx_command))) {
                                value[j] = dev->rx_command[i];
                                j++;
                                i++;
                            }
                            value[j] = 0;

                            bluetooth_address_t *addr = (bluetooth_address_t *)message->value;
                            message->value_size = sizeof(bluetooth_address_t);

                            netif_mac_address_set(&addr->address, value);
                            if(dev->rx_command[i] != 'r') {
                                addr->type = BLUETOOTH_ADDRESS_TYPE_RANDOM;
                            }
                            else if(dev->rx_command[i] != 'p') {
                                addr->type = BLUETOOTH_ADDRESS_TYPE_PUBLIC;
                            }

                            bluetooth_sync_buffer_insert(&dev->btif->sync);
                            notification_send(dev->btif->bluetooth_task, dev->btif->bluetooth_task_notification);
                        }
                        else if(strncmp(dev->rx_command, "+UUBTACLD:", 10) == 0) {
                            bluetooth_sync_buffer_message_t *message = bluetooth_sync_buffer_insert_get_element(&dev->btif->sync);
                            message->type = BLUETOOTH_SYNC_BUFFER_CLIENT_DISCONNECTED;
                            message->conn_handle = 0;
                            
                            int i = 10;
                            while((dev->rx_command[i] != '\r') && (i < sizeof(dev->rx_command))) {
                                message->conn_handle *= 10;
                                message->conn_handle += dev->rx_command[i] - '0';
                                i++;
                            }

                            bluetooth_sync_buffer_insert(&dev->btif->sync);
                            notification_send(dev->btif->bluetooth_task, dev->btif->bluetooth_task_notification);
                        }
                        //response messages
                        else if(strcmp(dev->rx_command, "OK\r\n") == 0) {
                            dev->rx_response = NINA_B312_OK;
                            semaphore_unlock(&dev->rx_response_received);
                            semaphore_lock(&dev->rx_release);
                        }
                        else if(strcmp(dev->rx_command, "ERROR\r\n") == 0) {
                            dev->rx_response = NINA_B312_ERROR;
                            semaphore_unlock(&dev->rx_response_received);
                            semaphore_lock(&dev->rx_release);
                        }
                        else {
                            //store the content
                            strncat(dev->rx_content, dev->rx_command, 256);
                        }
                        
                    }
                    dev->rx_command_length = 0;
                }
            }
        }
    }  
}


//required callbacks by bluetooth driver
static uint32_t nina_b312_bluetooth_init(void *device);
static uint32_t nina_b312_bluetooth_get_mac_address(void *device, netif_mac_address_t *mac_address);

static uint32_t nina_b312_bluetooth_get_discoverability_mode(void *device, bluetooth_discoverability_mode_t *discoverability_mode);
static uint32_t nina_b312_bluetooth_set_discoverability_mode(void *device, bluetooth_discoverability_mode_t discoverability_mode);
static uint32_t nina_b312_bluetooth_get_connectability_mode(void *device, bluetooth_connectability_mode_t *connectability_mode);
static uint32_t nina_b312_bluetooth_set_connectability_mode(void *device, bluetooth_connectability_mode_t connectability_mode);
static uint32_t nina_b312_bluetooth_get_pairing_mode(void *device, bluetooth_pairing_mode_t *pairing_mode);
static uint32_t nina_b312_bluetooth_set_pairing_mode(void *device, bluetooth_pairing_mode_t pairing_mode);
static uint32_t nina_b312_bluetooth_get_security_mode(void *device, bluetooth_security_mode_t *security_mode);
static uint32_t nina_b312_bluetooth_set_security_mode(void *device, bluetooth_security_mode_t security_mode);
static uint32_t nina_b312_bluetooth_get_local_name(void *device, char *local_name);
static uint32_t nina_b312_bluetooth_set_local_name(void *device, const char *local_name);

static uint32_t nina_b312_bluetooth_get_low_energy_role(void *device, bluetooth_low_energy_role_t *low_energy_role);
static uint32_t nina_b312_bluetooth_set_low_energy_role(void *device, bluetooth_low_energy_role_t low_energy_role);

//checkAL redo
static uint32_t nina_b312_bluetooth_le_periph_set_advertising_service(void *device, const gatt_uuid_t *uuid);

static uint32_t nina_b312_gatt_server_start(void *device);
static uint32_t nina_b312_gatt_server_respond_to_read(void *device, uint32_t conn_handle, const uint8_t *value, uint8_t value_len);
static uint32_t nina_b312_gatt_server_send_notification(void *device, uint32_t conn_handle, uint32_t char_handle, const uint8_t *value, uint8_t value_len);
static uint32_t nina_b312_gatt_server_send_indication(void *device, uint32_t conn_handle, uint32_t char_handle, const uint8_t *value, uint8_t value_len);

void nina_b312_init(nina_b312_t *dev, btif_t *btif, uart_t uart, gpio_pin_t rst_pin, gpio_pin_t tx_pin, gpio_pin_t rx_pin, gpio_pin_t rts_pin, gpio_pin_t cts_pin) {
    assert(dev);
    assert(uart);

    //populate parent callbacks
    btif->dev = dev;
    btif->bluetooth_init = nina_b312_bluetooth_init;

    btif->bluetooth_get_mac_address = nina_b312_bluetooth_get_mac_address;
    btif->bluetooth_get_discoverability_mode = nina_b312_bluetooth_get_discoverability_mode;
    btif->bluetooth_set_discoverability_mode = nina_b312_bluetooth_set_discoverability_mode;
    btif->bluetooth_get_connectability_mode = nina_b312_bluetooth_get_connectability_mode;
    btif->bluetooth_set_connectability_mode = nina_b312_bluetooth_set_connectability_mode;
    btif->bluetooth_get_pairing_mode = nina_b312_bluetooth_get_pairing_mode;
    btif->bluetooth_set_pairing_mode = nina_b312_bluetooth_set_pairing_mode;
    btif->bluetooth_get_security_mode = nina_b312_bluetooth_get_security_mode;
    btif->bluetooth_set_security_mode = nina_b312_bluetooth_set_security_mode;
    btif->bluetooth_get_local_name = nina_b312_bluetooth_get_local_name;
    btif->bluetooth_set_local_name = nina_b312_bluetooth_set_local_name;
    btif->bluetooth_get_low_energy_role = nina_b312_bluetooth_get_low_energy_role;
    btif->bluetooth_set_low_energy_role = nina_b312_bluetooth_set_low_energy_role;

    //checkAL modifica
    btif->bluetooth_le_periph_set_advertising_service = nina_b312_bluetooth_le_periph_set_advertising_service;

    btif->gatt_server = &dev->gatt_server;  //set to 1 when supporing
    dev->gatt_server.gatt_server_start = nina_b312_gatt_server_start;
    dev->gatt_server.gatt_server_respond_to_read = nina_b312_gatt_server_respond_to_read;
    dev->gatt_server.gatt_server_send_notification = nina_b312_gatt_server_send_notification;
    dev->gatt_server.gatt_server_send_indication = nina_b312_gatt_server_send_indication;

    //do driver specific init
    dev->uart = uart;
    dev->btif = btif;
    dev->rst_pin = rst_pin;
    dev->tx_pin = tx_pin;
    dev->rx_pin = rx_pin;
    dev->rts_pin = rts_pin;
    dev->cts_pin = cts_pin;

    gpio_init_special(tx_pin, GPIO_SPECIAL_FUNCTION_UART, uart);
    gpio_init_special(rx_pin, GPIO_SPECIAL_FUNCTION_UART, uart);
    gpio_init_special(rts_pin, GPIO_SPECIAL_FUNCTION_UART, uart);
    gpio_init_special(cts_pin, GPIO_SPECIAL_FUNCTION_UART, uart);
    gpio_init_digital(rst_pin, GPIO_MODE_OUTPUT_PP, GPIO_PULL_NOPULL);
    gpio_digital_write(rst_pin, 0); //reset device
    uart_init_param(uart, 115200, UART_PARITY_NONE, UART_STOP_BITS_1, UART_FLOW_CONTROL_RTS_CTS, nina_b312_uart_rx, (uint32_t)dev);
    task_sleep(250);
    
    semaphore_create(&dev->rx_unsolicited_received, 0);
    semaphore_create(&dev->rx_response_received, 0);
    semaphore_create(&dev->rx_release, 0);
    dev->rx_buffer_in = 0;
    dev->rx_buffer_out = 0;
    dev->rx_command_length = 0;
    dev->rx_content[0] = 0;

    task_init(&dev->driver_task, dev->driver_task_stack, NINA_B312_STACK_SIZE, 1);  //checkAL alt priority?
    task_attr_set_start_type(&dev->driver_task, TASK_START_NOW, 0);
    task_create_a(&dev->driver_task, nina_b312_main, dev);
}

void nina_b312_deinit(nina_b312_t *dev) {
    assert(dev);

    task_kill(&dev->driver_task);
    uart_deinit(dev->uart);
    gpio_deinit(dev->rst_pin);
    gpio_deinit(dev->tx_pin);
    gpio_deinit(dev->rx_pin);
    gpio_deinit(dev->rts_pin);
    gpio_deinit(dev->cts_pin);    
}

static uint32_t nina_b312_bluetooth_init(void *device) {
    assert(device);
    nina_b312_t *dev = device;

    char content[64];
    nina_b312_rc_t rc_response;
    nina_b312_urc_t urc_response;

    //release device from reset
    gpio_digital_write(dev->rst_pin, 1);

    //wait for +STARTUP
    if(at_wait_for_unsolicited_response(dev, &urc_response, 0, 0, 2500) == 0) {
        return EIO;
    }

    //disable echoing
    if(at_send_command(dev, "ATE0\r\n", &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }
    
    //check that the device is NINA
    if(at_send_command(dev, "ATI0\r\n", &rc_response, content, sizeof(content), 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            if(strstr(content, "NINA") == 0) {
                return ENODEV;
            }
        }
    }

    //disable SPS service
    if(at_send_command(dev, "AT+UDSC=0,0\r\n", &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_get_mac_address(void *device, netif_mac_address_t *mac_address) {
    assert(device);
    assert(mac_address);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UMLA";
    sprintf(command, "AT+%s=1\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && (*f != ':')) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    f++;
                    char *mac = f;
                    while((*f != 0) && (*f != '\r')) {
                        f++;
                    }
                    *f = 0;
                    netif_mac_address_set(mac_address, mac);
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_get_discoverability_mode(void *device, bluetooth_discoverability_mode_t *discoverability_mode) {
    assert(device);
    assert(discoverability_mode);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UBTDM";
    sprintf(command, "AT+%s?\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && !((*f >= '0') && (*f <= '9'))) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    switch(*f) {
                        case '1':
                            *discoverability_mode = BLUETOOTH_DISCOVERABILITY_DISABLED;
                            break;

                        case '2':
                            *discoverability_mode = BLUETOOTH_DISCOVERABILITY_LIMITED;
                            break;

                        case '3':
                            *discoverability_mode = BLUETOOTH_DISCOVERABILITY_GENERAL;
                            break;

                        default:
                            *discoverability_mode = BLUETOOTH_DISCOVERABILITY_UNKNOWN;
                            break;
                    }
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_set_discoverability_mode(void *device, bluetooth_discoverability_mode_t discoverability_mode) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    int param = 0;
    switch(discoverability_mode) {
        case BLUETOOTH_DISCOVERABILITY_DISABLED:
            param = 1;
            break;
            
        case BLUETOOTH_DISCOVERABILITY_LIMITED:
            param = 2;
            break;

        case BLUETOOTH_DISCOVERABILITY_GENERAL:
            param = 3;
            break;

        default:
            return ENOSUP;
            break;
    }

    char *atcommand = "UBTDM";
    sprintf(command, "AT+%s=%d\r\n", atcommand, param);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_get_connectability_mode(void *device, bluetooth_connectability_mode_t *connectable_mode) {
    assert(device);
    assert(connectable_mode);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UBTCM";
    sprintf(command, "AT+%s?\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && !((*f >= '0') && (*f <= '9'))) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    switch(*f) {
                        case '1':
                            *connectable_mode = BLUETOOTH_CONNECTABILITY_DISABLED;
                            break;

                        case '2':
                            *connectable_mode = BLUETOOTH_CONNECTABILITY_ENABLED;
                            break;

                        default:
                            *connectable_mode = BLUETOOTH_CONNECTABILITY_UNKNOWN;
                            break;
                    }
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_set_connectability_mode(void *device, bluetooth_connectability_mode_t connectability_mode) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    int param = 0;
    switch(connectability_mode) {
        case BLUETOOTH_CONNECTABILITY_DISABLED:
            param = 1;
            break;

        case BLUETOOTH_CONNECTABILITY_ENABLED:
            param = 2;
            break;

        default:
            return ENOSUP;
            break;
    }

    char *atcommand = "UBTCM";
    sprintf(command, "AT+%s=%d\r\n", atcommand, param);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_get_pairing_mode(void *device, bluetooth_pairing_mode_t *pairing_mode) {
    assert(device);
    assert(pairing_mode);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UBTPM";
    sprintf(command, "AT+%s?\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && !((*f >= '0') && (*f <= '9'))) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    switch(*f) {
                        case '1':
                            *pairing_mode = BLUETOOTH_PAIRING_DISABLED;
                            break;

                        case '2':
                            *pairing_mode = BLUETOOTH_PAIRING_ENABLED;
                            break;

                        default:
                            *pairing_mode = BLUETOOTH_PAIRING_UNKNOWN;
                            break;
                    }
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_set_pairing_mode(void *device, bluetooth_pairing_mode_t pairing_mode) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    int param = 0;
    switch(pairing_mode) {
        case BLUETOOTH_PAIRING_DISABLED:
            param = 1;
            break;

        case BLUETOOTH_PAIRING_ENABLED:
            param = 2;
            break;

        default:
            return ENOSUP;
            break;
    }

    char *atcommand = "UBTPM";
    sprintf(command, "AT+%s=%d\r\n", atcommand, param);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_get_security_mode(void *device, bluetooth_security_mode_t *security_mode) {
    assert(device);
    assert(security_mode);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UBTSM";
    sprintf(command, "AT+%s?\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && !((*f >= '0') && (*f <= '9'))) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    switch(*f) {
                        case '1':
                            *security_mode = BLUETOOTH_SECURITY_MODE_1_DISABLED;
                            break;

                        case '2':
                            *security_mode = BLUETOOTH_SECURITY_MODE_2_JUST_WORKS;
                            break;

                        case '3':
                            *security_mode = BLUETOOTH_SECURITY_MODE_3_DISPLAY_ONLY;
                            break;

                        case '4':
                            *security_mode = BLUETOOTH_SECURITY_MODE_4_DISPLAY_YES_NO;
                            break;

                        case '5':
                            *security_mode = BLUETOOTH_SECURITY_MODE_5_KEYBOARD_ONLY;
                            break;

                        case '6':
                            *security_mode = BLUETOOTH_SECURITY_MODE_6_OOB;
                            break;

                        default:
                            *security_mode = BLUETOOTH_SECURITY_MODE_UNKNOWN;
                            break;
                    }
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_set_security_mode(void *device, bluetooth_security_mode_t security_mode) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    int param = 0;
    switch(security_mode) {
        case BLUETOOTH_SECURITY_MODE_1_DISABLED:
            param = 1;
            break;

        case BLUETOOTH_SECURITY_MODE_2_JUST_WORKS:
            param = 2;
            break;

        case BLUETOOTH_SECURITY_MODE_3_DISPLAY_ONLY:
            param = 3;
            break;

        case BLUETOOTH_SECURITY_MODE_4_DISPLAY_YES_NO:
            param = 4;
            break;

        case BLUETOOTH_SECURITY_MODE_5_KEYBOARD_ONLY:
            param = 5;
            break;

        case BLUETOOTH_SECURITY_MODE_6_OOB:
            param = 6;
            break;

        default:
            return ENOSUP;
            break;
    }

    char *atcommand = "UBTSM";
    sprintf(command, "AT+%s=%d\r\n", atcommand, param);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_get_local_name(void *device, char *local_name) {
    assert(device);
    assert(local_name);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UBTLN";
    sprintf(command, "AT+%s?\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && (*f != ':')) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    f++;
                    while((*f != 0) && (*f != '\r')) {
                        *local_name = *f;
                        local_name++;
                        f++;
                    }
                    *local_name = 0;
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_set_local_name(void *device, const char *local_name) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    
    char *atcommand = "UBTLN";
    sprintf(command, "AT+%s=%s\r\n", atcommand, local_name);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_get_low_energy_role(void *device, bluetooth_low_energy_role_t *low_energy_role) {
    assert(device);
    assert(low_energy_role);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    char response[64];

    char *atcommand = "UBTLE";
    sprintf(command, "AT+%s?\r\n", atcommand);
    if(at_send_command(dev, command, &rc_response, response, 64, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            char *f = strstr(response, atcommand);
            if(f == 0) {
                return EBADRESP;
            }
            else {
                f += strlen(atcommand);
                while((*f != 0) && !((*f >= '0') && (*f <= '9'))) {
                    f++;
                }

                if(*f == 0) {
                    return EBADRESP;
                }
                else {
                    switch(*f) {
                        case '0':
                            *low_energy_role = BLUETOOTH_LOW_ENERGY_DISABLED;
                            break;

                        case '1':
                            *low_energy_role = BLUETOOTH_LOW_ENERGY_CENTRAL;
                            break;

                        case '2':
                            *low_energy_role = BLUETOOTH_LOW_ENERGY_PERIPHERAL;
                            break;

                        case '3':
                            *low_energy_role = BLUETOOTH_LOW_ENERGY_CENTRAL_PERIPHERAL;
                            break;

                        default:
                            *low_energy_role = BLUETOOTH_LOW_ENERGY_UNKNOWN;
                            break;
                    }
                }
            }
        }
    }
    
    return EOK;
}

static uint32_t nina_b312_bluetooth_set_low_energy_role(void *device, bluetooth_low_energy_role_t low_energy_role) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[32];
    int param = 0;
    switch(low_energy_role) {
        case BLUETOOTH_LOW_ENERGY_DISABLED:
            param = 0;
            break;

        case BLUETOOTH_LOW_ENERGY_CENTRAL:
            param = 1;
            break;

        case BLUETOOTH_LOW_ENERGY_PERIPHERAL:
            param = 2;
            break;

        case BLUETOOTH_LOW_ENERGY_CENTRAL_PERIPHERAL:
            param = 3;
            break;

        default:
            return ENOSUP;
            break;
    }

    char *atcommand = "UBTLE";
    sprintf(command, "AT+%s=%d\r\n", atcommand, param);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_bluetooth_le_periph_set_advertising_service(void *device, const gatt_uuid_t *uuid) {
    assert(device);
    nina_b312_t *dev = device;

    nina_b312_rc_t rc_response;
    char command[128];
    char rev_uuid[64];
    gatt_uuid_to_string_reverse(uuid, rev_uuid);

    char uuid_type[5];
    if(uuid->value_type == GATT_UUID_16BIT) {
        strcpy(uuid_type, "0303");
    }
    else {
        strcpy(uuid_type, "1107");
    }

    sprintf(command, "AT+UBTAD=020A06051218002800%s%s\r\n", uuid_type, rev_uuid);
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_gatt_server_add_service(void *device, gatt_service_t *service) {
    assert(device);
    assert(service);

    nina_b312_t *dev = device;
    nina_b312_rc_t rc_response;

    char content[33];
    gatt_uuid_to_string(&service->uuid, content);
    
    char command[64];
    sprintf(command, "AT+UBTGSER=%s\r\n", content); 
    if(at_send_command(dev, command, &rc_response, content, sizeof(content), 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            int i = 0;
            while((content[i] != '+') && (i < sizeof(content))) {
                i++;
            }

            if(strstr(content + i, "+UBTGSER:") != content + i) {
                return EBADRESP;
            }
            else {
                uint32_t chandle = 0;

                i += 9; //+UBTGSER:
                while((content[i] != ',') && (i < sizeof(content))) {
                    chandle *= 10;
                    chandle += content[i] - '0';
                    i++;
                }

                service->handle = chandle;
            }
        }
    }

    return EOK;
}

static uint32_t nina_b312_gatt_server_add_characteristic(void *device, gatt_characteristic_t *characteristic) {
    assert(device);
    assert(characteristic);

    nina_b312_t *dev = device;
    nina_b312_rc_t rc_response;

    char content[33];
    gatt_uuid_to_string(&characteristic->uuid, content);

    int prop = 0;
    int rs = 0;
    int ws = 0;

    if(characteristic->properties & GATT_CHAR_PROPERTY_BROADCAST) {
        prop |= 0x01;
    }
    if(characteristic->properties & GATT_CHAR_PROPERTY_READ) {
        prop |= 0x02;
    }
    if(characteristic->properties & GATT_CHAR_PROPERTY_WRITE_WITHOUT_RESPONSE) {
        prop |= 0x04;
    }
    if(characteristic->properties & GATT_CHAR_PROPERTY_WRITE) {
        prop |= 0x08;
    }
    if(characteristic->properties & GATT_CHAR_PROPERTY_NOTIFY) {
        prop |= 0x10;
    }
    if(characteristic->properties & GATT_CHAR_PROPERTY_INDICATE) {
        prop |= 0x20;
    }
    if(characteristic->properties & GATT_CHAR_PROPERTY_SIGNED_WRITE) {
        prop |= 0x40;
    }

    switch(characteristic->read_security) {
        case GATT_ENCRYPTION_TYPE_NONE:
            rs = 1;
            break;

        case GATT_ENCRYPTION_TYPE_UNAUTHENTICATED:
            rs = 2;
            break;

        case GATT_ENCRYPTION_TYPE_AUTHENTICATED:
            rs = 3;
            break;

        case GATT_ENCRYPTION_TYPE_AUTHENTICATED_LESC:
            rs = 4;
            break;
    }

    switch(characteristic->write_security) {
        case GATT_ENCRYPTION_TYPE_NONE:
            ws = 1;
            break;

        case GATT_ENCRYPTION_TYPE_UNAUTHENTICATED:
            ws = 2;
            break;

        case GATT_ENCRYPTION_TYPE_AUTHENTICATED:
            ws = 3;
            break;

        case GATT_ENCRYPTION_TYPE_AUTHENTICATED_LESC:
            ws = 4;
            break;
    }
    
    char command[128];
    sprintf(command, "AT+UBTGCHA=%s,%02X,%d,%d\r\n", content, prop, rs, ws);
    if(at_send_command(dev, command, &rc_response, content, sizeof(content), 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
        else {
            int i = 0;
            while((content[i] != '+') && (i < sizeof(content))) {
                i++;
            }

            if(strstr(content + i, "+UBTGCHA:") != content + i) {
                return EBADRESP;
            }
            else {
                uint32_t chandle = 0;

                i += 9; //+UBTGCHA:
                while((content[i] != ',') && (i < sizeof(content))) {
                    chandle *= 10;
                    chandle += content[i] - '0';
                    i++;
                }
                characteristic->handle = chandle;
                
                i++;

                chandle = 0;
                while((content[i] != '\r') && (i < sizeof(content))) {
                    chandle *= 10;
                    chandle += content[i] - '0';
                    i++;
                }
                characteristic->cccd_handle = chandle;
            }
        }
    }

    return EOK;
}

static uint32_t nina_b312_gatt_server_start(void *device) {
    assert(device);

    nina_b312_t *dev = device;
    gatt_server_t *server = &dev->gatt_server;

    if(server == 0) {
        return ENOSUP;
    }

    for(int i = 0; i < server->service_len; i++) {
        if(nina_b312_gatt_server_add_service(device, &server->service[i]) != EOK) {
            return EBADRESP;
        }

        for(int j = 0; j < server->service[i].characteristic_len; j++) {
            if(nina_b312_gatt_server_add_characteristic(device, &server->service[i].characteristic[j]) != EOK) {
                return EBADRESP;
            }
        }
    }

    return EOK;
}

static uint32_t nina_b312_gatt_server_respond_to_read(void *device, uint32_t conn_handle, const uint8_t *value, uint8_t value_len) {
    assert(device);
    assert(value);

    nina_b312_t *dev = device;
    nina_b312_rc_t rc_response;

    char command[256];
    sprintf(command, "AT+UBTGRR=%d,", conn_handle);
    for(int i = 0; i < value_len; i++) {
        char byte[3];
        sprintf(byte, "%02X", value[i]);
        strcat(command, byte);
    }
    strcat(command, "\r\n");
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_gatt_server_send_notification(void *device, uint32_t conn_handle, uint32_t char_handle, const uint8_t *value, uint8_t value_len) {
    assert(device);
    assert(value);

    nina_b312_t *dev = device;
    nina_b312_rc_t rc_response;

    char command[256];
    sprintf(command, "AT+UBTGSN=%d,%d,", conn_handle, char_handle);
    for(int i = 0; i < value_len; i++) {
        char byte[3];
        sprintf(byte, "%02X", value[i]);
        strcat(command, byte);
    }
    strcat(command, "\r\n");
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}

static uint32_t nina_b312_gatt_server_send_indication(void *device, uint32_t conn_handle, uint32_t char_handle, const uint8_t *value, uint8_t value_len) {
    assert(device);
    assert(value);

    nina_b312_t *dev = device;
    nina_b312_rc_t rc_response;

    char command[256];
    sprintf(command, "AT+UBTGSI=%d,%d,", conn_handle, char_handle);
    for(int i = 0; i < value_len; i++) {
        char byte[3];
        sprintf(byte, "%02X", value[i]);
        strcat(command, byte);
    }
    strcat(command, "\r\n");
    if(at_send_command(dev, command, &rc_response, 0, 0, 1000) == 0) {
        return ETIMEOUT;
    }
    else {
        if(rc_response != NINA_B312_OK) {
            return EBADRESP;
        }
    }

    return EOK;
}
