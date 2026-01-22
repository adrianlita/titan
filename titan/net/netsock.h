#pragma once

#include "netif.h"
#include <stdint.h>

typedef struct __netsock {
	netif_t *interface;

} netsock_t;

uint32_t netsock_create(netsock_t *sock, netif_t *interface);
uint32_t netsock_destroy(netsock_t *sock);
uint32_t netsock_connect(netsock_t *sock, const char *address, uint32_t timeout);
uint32_t netsock_disconnect(netsock_t *sock);
uint32_t netsock_listen(netsock_t *sock);
uint32_t netsock_write(netsock_t *sock, const uint8_t *buffer, const uint32_t length);
uint32_t netsock_read(netsock_t *sock, uint8_t *buffer, uint32_t length);
uint32_t netsock_read_try(netsock_t *sock, uint8_t *buffer, uint32_t length, uint32_t timeout);
uint32_t netsock_get_read_bytes(netsock_t *sock);
uint32_t netsock_get_error(netsock_t *sock);
