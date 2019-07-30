#include <stdio.h>
#include <string.h>

#include "permamote_coap.h"
#include "thread_dns.h"

static uint32_t seq_no = 0;

const otIp6Address unspecified_ipv6 =
{
    .mFields =
    {
        .m8 = {0}
    }
};

otError permamote_coap_send(otIp6Address* dest_addr,
    const char* path, bool confirmable, const permamote_packet_t* packet) {
  otInstance * thread_instance = thread_get_instance();
  uint64_t time_sec = packet->timestamp.tv_sec;
  uint32_t time_usec = packet->timestamp.tv_usec;
  static uint8_t data [256];
  size_t ptr = 0;

  if (otIp6IsAddressEqual(dest_addr, &unspecified_ipv6)) {
    return OT_ERROR_ADDRESS_QUERY;
  }

  APP_ERROR_CHECK_BOOL(packet->data_len < 256);

  data[ptr++] = packet->id_len;
  memcpy(data+ptr, packet->id, packet->id_len);
  ptr += packet->id_len;
  data[ptr++] = PERMAMOTE_PACKET_VERSION;
  memcpy(data+ptr, &seq_no, sizeof(seq_no));
  ptr += sizeof(seq_no);
  memcpy(data+ptr, &time_sec, sizeof(time_sec));
  ptr += sizeof(time_sec);
  memcpy(data+ptr, &time_usec, sizeof(time_usec));
  ptr += sizeof(time_usec);
  memcpy(data+ptr, packet->data, packet->data_len);
  ptr += packet->data_len;

  otCoapType coap_type = confirmable ? OT_COAP_TYPE_CONFIRMABLE : OT_COAP_TYPE_NON_CONFIRMABLE;

  otError error = thread_coap_send(thread_instance, OT_COAP_CODE_PUT, coap_type, dest_addr, path, data, ptr);

  // increment sequence number if successful
  if (error == OT_ERROR_NONE) {
    seq_no++;
  }

  return error;
}
