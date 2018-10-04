#pragma once
#include "time.h"
#include "simple_thread.h"
#include "thread_coap.h"
#include "app_error.h"

#define PERMAMOTE_PACKET_VERSION 0

typedef struct {
  char* path;
  uint8_t* id;
  uint8_t id_len;
  struct timeval timestamp;
  uint8_t* data;
  uint8_t data_len;
} permamote_packet_t;

void permamote_coap_send(const otIp6Address* dest, const char* path, bool confirmable, const permamote_packet_t* packet);
