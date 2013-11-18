#ifndef __COMMON_H_
#define __COMMON_H_

#include <xccompat.h>

#define CRC_BYTES               (sizeof(unsigned int))
#define PKT_DELAY_BYTES         (sizeof(unsigned int))
#define PKT_SIZE_BYTES          (sizeof(unsigned int))
#define MAX_PACKET_SEQUENCE     20
#define MIN_FRAME_SIZE          64
#define MAX_FRAME_SIZE          1522
#define LAST_FRAME              (1<<7)
#define END_OF_PACKET_SEQUENCE  (1<<5)
#define MAX_BUFFER_WORDS  ((MAX_FRAME_SIZE+3)>>2)

typedef enum {
  EVENT_WAIT,
  EVENT_SOF,
  EVENT_FRAME_INCOMPLETE,
  EVENT_EOF,
  EVENT_EOP
}event_t;

typedef enum {
  RX_SOF,
  RX_COMPLETE
}rx_to_app_t;

typedef enum {
  HOST_CMD_TX,
  HOST_CMD_TX_ACK,
} host_to_app_t;

// packet control
typedef struct packet_control{
  unsigned int packet_number;
  unsigned int frame_delay;
  unsigned int frame_size;
  unsigned int frame_crc;
}packet_control_t;
/**
 * \brief   The interface between the xscope receiver and checker core
 */
interface xscope_config {
  void put_buffer(unsigned int xscope_buff[]);
};

#ifdef __XC__
#define CHANEND_PARAM(param, name) param name
#else
#define CHANEND_PARAM(param, name) unsigned name
#endif

#endif /* __COMMON_H_ */

