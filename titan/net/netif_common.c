#include "netif_common.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

void netif_mac_address_set(netif_mac_address_t *mac, const char *str_mac) {
    assert(mac);
    assert(str_mac);
    assert((strlen(str_mac) == NETIF_MAC_ADDRESS_SIZE) || (strlen(str_mac) == NETIF_MAC_ADDRESS_PACKED_SIZE));

    int k = 0;
    for(int i = 0; i < 6; i++) {
        char nibble[3];
        nibble[0] = str_mac[k++];
        nibble[1] = str_mac[k++];
        nibble[2] = 0;
        if(str_mac[k] == ':') {
            k++;
        }

        mac->value[i] = strtoul(nibble, 0, 16);
    }
}

void netif_mac_address_to_string(const netif_mac_address_t *mac, char *str_mac) {
    assert(mac);
    assert(str_mac);

    str_mac[0] = 0;
    for(int i = 0; i < 6; i++) {
        char nibble[4];
        if(i < 5) {
            sprintf(nibble, "%02X:", mac->value[i]);
        }
        else {
            sprintf(nibble, "%02X", mac->value[i]);
        }
        strcat(str_mac, nibble);
    }
}

void netif_mac_address_to_string_packed(const netif_mac_address_t *mac, char *str_mac) {
    assert(mac);
    assert(str_mac);

    str_mac[0] = 0;
    for(int i = 0; i < 6; i++) {
        char nibble[4];
        sprintf(nibble, "%02X", mac->value[i]);
        strcat(str_mac, nibble);
    }
}
