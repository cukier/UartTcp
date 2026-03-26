#ifndef _MAIN_INCLUDE_KSYS_H
#define _MAIN_INCLUDE_KSYS_H

#include "inttypes.h"
#include <stddef.h>

#define BUF_SIZE 1024

typedef struct {
    uint8_t *data;
    size_t length;
} bridge_msg_t;

typedef struct KsysStr {
  QueueHandle_t uart_tx_queue;
  QueueHandle_t tcp_tx_queue;
  int current_client_sock;
} ksys_t;

#endif