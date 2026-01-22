#pragma once

#include <kernel_defines.h>
#include <kernel_atomic.h>
#include <stdint.h>

/* POINTER message queue: - all messages are passed by POINTER - only a pointer is copied */
typedef struct {
    primitive_t parent_receive;
    primitive_t parent_send;    //we don't care for the id here, only for the list
    uint32_t length;            //queue length, in number of messages
    void *buffer;               //queue buffer
    uint32_t count_avail;       //current messages in queue
    uint32_t head;              //head index
    uint32_t tail;              //tail index
} pqueue_t;

void pqueue_create(pqueue_t *q, uint32_t length, void *buffer) K_ISR_FORBID K_CSECT_NOMODIF;                                  //initialize queue
uint32_t pqueue_get_free(pqueue_t *q) K_ISR_SAFE K_CSECT_ENTRY K_CSECT_EXIT;                                                  //get number of available message slots in queue
void pqueue_send(pqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                   //send message to queue (at the end), if space is available
uint8_t pqueue_send_try(pqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;          //try to send message to queue (at the end), waiting "timeout" ticks. returns 1 when sent, 0 when timed out. if timeout is 0, will return immediately
void pqueue_send_urgent(pqueue_t *q, const void *message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                            //send urgent message to queue (will be put in the begining), if space is available
uint8_t pqueue_send_urgent_try(pqueue_t *q, const void *message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;   //try to send urgent message to queue (will be put in the begining, waiting "timeout" ticks. returns 1 when sent, 0 when timed out. if timeout is 0, will return immediately
void pqueue_receive(pqueue_t *q, void **message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                      //receive message to queue, if space is available
uint8_t pqueue_receive_try(pqueue_t *q, void **message, uint32_t timeout) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;             //try to receive message to queue, waiting "timeout" ticks. returns 1 when received, 0 when timed out. if timeout is 0, will return immediately
uint8_t pqueue_peek(pqueue_t *q, void **message) K_ISR_FORBID K_CSECT_ENTRY K_CSECT_EXIT;                                      //peek queue begining (see the first "receivable" message). returns 1 if message is available, 0 if queue is empty
