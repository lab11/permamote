#include <stdio.h>
#include <string.h>

#include "pb_encode.h"

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

void permamote_response_handler (void* context, otMessage* message, const
                                 otMessageInfo* message_info, otError result) {
  if (result == OT_ERROR_NONE) {
    printf("got response!\n");
  }
}

otError permamote_coap_send(otIp6Address* dest_addr,
    const char* path, bool confirmable, Message* msg) {
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

  memcpy(&(msg->header), &header, sizeof(header));

  uint8_t packed_data [256];

  pb_ostream_t stream;
  stream = pb_ostream_from_buffer(packed_data, sizeof(packed_data));
  pb_encode(&stream, Message_fields, msg);
  size_t len = stream.bytes_written;
  APP_ERROR_CHECK_BOOL(len < 256);

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

  otError error = thread_coap_send(thread_instance, OT_COAP_CODE_PUT, coap_type, dest_addr, path, packed_data, len, permamote_response_handler);

  // increment sequence number if successful
  if (error == OT_ERROR_NONE) {
    seq_no++;
  }

  return error;
}
