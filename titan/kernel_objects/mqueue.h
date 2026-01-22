#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>
#include <stdint.h>

/* 
    standard message queue:
        - all messages are passed by COPY
*/
typedef struct {
    primitive_t parent_receive;
    primitive_t parent_send;    //we don't care for the id here, only for the list
    uint32_t message_size;      //in bytes
    uint32_t length;            //queue length, in number of messages
    void *buffer;               //queue buffer
    uint32_t count_avail;       //current messages in queue
    uint32_t head;              //head index
    uint32_t tail;              //tail index
} mqueue_t;

void mqueue_create(mqueue_t *q, uint32_t message_size, uint32_t length, void *buffer) K_ISR_FORBID K_CSECT_NOMODIF;           //initialize queue
uint32_t mqueue_get_free(mqueue_t *q) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                                                  //get number of available message slots in queue
void mqueue_send(mqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                   //send message to queue (at the end), if space is available
uint8_t mqueue_send_try(mqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;          //try to send message to queue (at the end), waiting "timeout" ticks. returns 1 when sent, 0 when timed out. if timeout is 0, will return immediately
void mqueue_send_urgent(mqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                            //send urgent message to queue (will be put in the begining), if space is available
uint8_t mqueue_send_urgent_try(mqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;   //try to send urgent message to queue (will be put in the begining, waiting "timeout" ticks. returns 1 when sent, 0 when timed out. if timeout is 0, will return immediately
void mqueue_receive(mqueue_t *q, void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                      //receive message to queue, if space is available
uint8_t mqueue_receive_try(mqueue_t *q, void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;             //try to receive message to queue, waiting "timeout" ticks. returns 1 when received, 0 when timed out. if timeout is 0, will return immediately
uint8_t mqueue_peek(mqueue_t *q, void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                      //peek queue begining (see the first "receivable" message). returns 1 if message is available, 0 if queue is empty
