#pragma once

#include "bluetooth_sync_buffer.h"
#include <assert.h>

TITAN_DEBUG_FILE_MARK;

void bluetooth_sync_buffer_init(bluetooth_sync_buffer_t *sync, bluetooth_sync_buffer_message_t *buffer, uint32_t size) {
    assert(sync);
    assert(buffer);
    assert(size);

    sync->message = buffer;
    sync->size = size;
    sync->in = 0;
    sync->out = 0;
}

bluetooth_sync_buffer_message_t *bluetooth_sync_buffer_insert_get_element(bluetooth_sync_buffer_t *sync) {
    assert(sync);

    return &sync->message[sync->in];
}
void bluetooth_sync_buffer_insert(bluetooth_sync_buffer_t *sync) {
    assert(sync);

    sync->in++;
    if(sync->in >= sync->size) {
        sync->in = 0;
    }
}

bluetooth_sync_buffer_message_t *bluetooth_sync_buffer_extract_get_element(bluetooth_sync_buffer_t *sync) {
    assert(sync);

    return &sync->message[sync->out];
}
void bluetooth_sync_buffer_extract(bluetooth_sync_buffer_t *sync) {
    assert(sync);

    sync->out++;
    if(sync->out >= sync->size) {
        sync->out = 0;
    }
}

uint8_t bluetooth_sync_buffer_has_elements(bluetooth_sync_buffer_t *sync) {
    assert(sync);

    return (sync->out != sync->in);
}
