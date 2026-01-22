#pragma once

#include <stdint.h>

#define NETIF_MAC_ADDRESS_SIZE                  17
#define NETIF_MAC_ADDRESS_PACKED_SIZE           12

typedef struct {
    uint8_t value[6];
} netif_mac_address_t;

void netif_mac_address_set(netif_mac_address_t *mac, const char *str_mac);
void netif_mac_address_to_string(const netif_mac_address_t *mac, char *str_mac);
void netif_mac_address_to_string_packed(const netif_mac_address_t *mac, char *str_mac);
