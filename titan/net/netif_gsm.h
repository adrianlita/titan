#pragma once

#include "netif.h"

typedef enum {
    WIFI_MODE_UNKNOWN = 0,
    WIFI_MODE_STANDALONE = 1,
    WIFI_MODE_AP = 2,
    WIFI_MODE_MIXED = 3,
} wifi_mode_t;

typedef int16_t wifi_rssi_t;

typedef enum {
    WIFI_ENCRYPTION_UNKNOWN = 0,
    WIFI_ENCRYPTION_OPEN = 1,
    WIFI_ENCRYPTION_WEP = 2,
    WIFI_ENCRYPTION_PSK = 3,
    WIFI_ENCRYPTION_PSK2 = 4,
} wifi_encryption_t;

typedef struct __wifi_ap {
    char *ssid;     //checkAL
    wifi_rssi_t rssi;
    wifi_encryption_t encryption;
} wifi_ap_t;

typedef wifi_mode_t (*netif_wifi_get_mode_t)(void);
typedef int8_t (*netif_wifi_ap_list_t)(char **aps);
typedef int8_t (*netif_wifi_ap_connect_t)(char *ap);
typedef int8_t (*netif_wifi_ap_disconnect_t)(void);
typedef int8_t (*netif_wifi_ap_get_connected_t)(char *ap);

typedef wifi_rssi_t (*netif_wifi_get_rssi_t)(void);

typedef struct _netif_wifi {    //extends netif_t
    netif_t base;
    wifi_mode_t mode;

    //callbacks
    netif_wifi_get_mode_t wifi_get_mode;
    netif_wifi_ap_list_t wifi_ap_list;
    netif_wifi_ap_connect_t wifi_ap_connect;
    netif_wifi_ap_disconnect_t wifi_ap_disconnect;
    netif_wifi_ap_get_connected_t wifi_ap_get_connected;
    netif_wifi_get_rssi_t wifi_get_rssi;
} netif_wifi_t;



wifi_mode_t netif_wifi_get_mode(netif_wifi_t *interface);
int8_t netif_wifi_ap_list(netif_wifi_t *interface, char **aps);
int8_t netif_wifi_ap_connect(netif_wifi_t *interface, char *ap);
int8_t netif_wifi_ap_disconnect(netif_wifi_t *interface);
int8_t netif_wifi_ap_get_connected(netif_wifi_t *interface, char *ap);

wifi_rssi_t netif_wifi_get_rssi(netif_wifi_t *interface);
