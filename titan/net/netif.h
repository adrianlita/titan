#pragma once

#include "netif_common.h"

typedef enum {
    NETIF_TYPE_ETHERNET,
    NETIF_TYPE_WIFI,
    NETIF_TYPE_GSM,
} netif_type_t;

typedef uint32_t (*netif_connect_t)(uint32_t sock_no, const char *address, uint32_t timeout);
typedef uint32_t (*netif_disconnect_t)(uint32_t sock_no);
typedef uint32_t (*netif_listen_t)(uint32_t sock_no);
typedef uint32_t (*netif_write_t)(uint32_t sock_no, const uint8_t *buffer, const uint32_t length);
typedef uint32_t (*netif_read_t)(uint32_t sock_no, uint8_t *buffer, uint32_t length);

typedef struct _netif {
    netif_type_t if_type;

    uint8_t max_sockets;        //total number of supported sockets
    uint8_t max_out_sockets;    //max number of output sockets (used with connect())
    uint8_t max_in_sockets;     //max number of input sockets (used with with accept())

    uint8_t auto_dns;
    uint8_t default_mac_address;
    char mac_address[18];

    volatile uint8_t read_bytes;

    //callbacks
    netif_connect_t connect;
    netif_disconnect_t disconnect;
    netif_listen_t listen;
    netif_write_t write;
    netif_read_t read;
} netif_t;

int8_t netif_register_interface(netif_t *interface);

int8_t netif_get_connected(netif_t *interface);
