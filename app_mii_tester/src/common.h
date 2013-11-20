#ifndef __COMMON_H_
#define __COMMON_H_

#include <xccompat.h>

#define CRC_BYTES               (sizeof(unsigned int))
#define PKT_DELAY_BYTES         (sizeof(unsigned int))
#define PKT_SIZE_BYTES          (sizeof(unsigned int))
#define MIN_IFG_BYTES           12       //Shouldn't go below this
#define MAX_PACKET_SEQUENCE     20
#define MIN_FRAME_SIZE          64
#define MAX_FRAME_SIZE          1522
#define LAST_FRAME              (1<<7)
#define END_OF_PACKET_SEQUENCE  (1<<5)
#define MAX_BUFFER_WORDS        ((MAX_FRAME_SIZE+3)>>2)
#define GET_PACKET_NO(x)        ((x>>26) & 0x3F)
#define GET_FRAME_DELAY(x)      ((x >> 11)&0x7FFF)
#define GET_FRAME_SIZE(x)       (x & 0x7FF)

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
  TX_0_INTRF,
  TX_1_INTRF
} tx_interface_t;

// packet control
typedef struct packet_control{
  unsigned int frame_delay;
  unsigned int frame_size;
  unsigned int frame_crc;
}packet_control_t;

// timestamp info
typedef struct rx_packet_analysis {
  unsigned int ifg_start_tick;
  unsigned int ifg_end_tick;
  unsigned int no_of_bytes;
  unsigned int checksum;
}rx_packet_analysis_t;
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

