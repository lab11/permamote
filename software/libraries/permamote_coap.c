#include <stdio.h>
#include <string.h>

#include "permamote_coap.h"

void permamote_coap_send(const otIp6Address* dest, const char* path, const permamote_packet_t* packet) {
  uint64_t time_sec = packet->timestamp.tv_sec;
  uint32_t time_usec = packet->timestamp.tv_usec;
  static uint8_t data [256];
  size_t ptr = 1;

  APP_ERROR_CHECK_BOOL(packet->data_len < 256);

  data[ptr++] = packet->id_len;
  memcpy(data+ptr, packet->id, packet->id_len);
  ptr += packet->id_len;
  data[ptr++] = PERMAMOTE_PACKET_VERSION;
  memcpy(data+ptr, &time_sec, sizeof(time_sec));
  memcpy(data+ptr, &time_usec, sizeof(time_usec));
  memcpy(data+ptr, packet->data, packet->data_len);

  thread_coap_send(thread_get_instance(), OT_COAP_CODE_PUT, OT_COAP_TYPE_NON_CONFIRMABLE, dest, path, data, ptr);
}
