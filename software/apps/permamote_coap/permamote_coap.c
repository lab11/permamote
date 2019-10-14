#include <stdio.h>
#include <string.h>

#include "ab1815.h"

#include "flash_storage.h"
#include "permamote_coap.h"
#include "thread_dns.h"

static uint32_t seq_no = 0;
extern uint8_t device_id[6];

const otIp6Address unspecified_ipv6 =
{
    .mFields =
    {
        .m8 = {0}
    }
};

otError permamote_coap_send(otIp6Address* dest_addr,
    const char* path, bool confirmable, PermamoteMessage* msg) {
  if (otIp6IsAddressEqual(dest_addr, &unspecified_ipv6)) {
    return OT_ERROR_ADDRESS_QUERY;
  }

  otInstance * thread_instance = thread_get_instance();
  Header header = Header_init_default;
  header.version = PERMAMOTE_PACKET_VERSION;
  memcpy(header.id.bytes, device_id, sizeof(device_id));
  header.id.size = sizeof(device_id);
  struct timeval time = ab1815_get_time_unix();
  header.tv_sec = time.tv_sec;
  header.tv_usec = time.tv_usec;
  header.seq_no = seq_no;

  msg->header = header;

  size_t len = 0;//sensor_data__get_packed_size(msg);
  //APP_ERROR_CHECK_BOOL(len < 256);

  uint8_t packed_data [256];
  //sensor_data__pack(msg, packed_data);

  //data[ptr++] = packet->id_len;
  //memcpy(data+ptr, packet->id, packet->id_len);
  //ptr += packet->id_len;
  //data[ptr++] = PERMAMOTE_PACKET_VERSION;
  //memcpy(data+ptr, &seq_no, sizeof(seq_no));
  //ptr += sizeof(seq_no);
  //memcpy(data+ptr, &time_sec, sizeof(time_sec));
  //ptr += sizeof(time_sec);
  //memcpy(data+ptr, &time_usec, sizeof(time_usec));
  //ptr += sizeof(time_usec);
  //memcpy(data+ptr, packet->data, packet->data_len);
  //ptr += packet->data_len;

  otCoapType coap_type = confirmable ? OT_COAP_TYPE_CONFIRMABLE : OT_COAP_TYPE_NON_CONFIRMABLE;

  otError error = thread_coap_send(thread_instance, OT_COAP_CODE_PUT, coap_type, dest_addr, path, packed_data, len);

  // increment sequence number if successful
  if (error == OT_ERROR_NONE) {
    seq_no++;
  }

  return error;
}
