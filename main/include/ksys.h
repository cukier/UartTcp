#ifndef _MAIN_INCLUDE_KSYS_H
#define _MAIN_INCLUDE_KSYS_H

#include "inttypes.h"
#include <stddef.h>

#define BUF_SIZE 1024

typedef struct {
  uint8_t data[BUF_SIZE];
  size_t length;
} bridge_msg_t;

#endif