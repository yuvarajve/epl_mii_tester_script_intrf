#ifndef __COMMON_H_
#define __COMMON_H_

#include <xccompat.h>

#define CRC_BYTES               (sizeof(unsigned int))
#define PKT_DELAY_BYTES         (sizeof(unsigned int))
#define PKT_SIZE_BYTES          (sizeof(unsigned int))
#define MIN_IFG_DELAY           96       //Shouldn't go below this
#define MAX_PACKET_SEQUENCE     20
#define MIN_FRAME_SIZE          64
#define MAX_FRAME_SIZE          1522
#define LAST_FRAME              (1<<7)
#define END_OF_PACKET_SEQUENCE  (1<<5)
#define MAX_BUFFER_WORDS        ((MAX_FRAME_SIZE+3)>>2)
#define GET_PACKET_NO(x)        ((x>>26) & 0x3F)
#define GET_FRAME_DELAY(x)      ((x >> 11)&0x7FFF)
#define GET_FRAME_SIZE(x)       (x & 0x7FF)
#define IFG_COMPENSATION_FACTOR  7  //7bits

typedef enum {
  TX_0_INTRF,
  TX_1_INTRF
} tx_interface_t;

typedef enum {
  TX_SUCCESS,
  TX_ERROR,
  RX_SUCCESS,
  RX_ERROR,
  TOTAL_STATUS             /**< this must be last*/
}status_info_t;

// packet control
typedef struct packet_control{
  unsigned frame_delay;
  unsigned frame_size;
  unsigned frame_crc;
}packet_control_t;

typedef struct tx_packet_info {
    unsigned int no_of_bytes;
    unsigned int checksum;
    unsigned int tx_start_tick;
}tx_packet_info_t;
// timestamp info
typedef struct rx_packet_info {
  unsigned int no_of_bytes;
  unsigned int checksum;
  unsigned int rx_start_tick;
}rx_packet_info_t;
/**
 * \brief   The interface between the xscope receiver and checker core
 */
interface xscope_config {
  void put_buffer(unsigned int xscope_buff[]);
};

interface data_manager {
    void status(status_info_t status);
    [[notification]] slave void packet_arrived(void);
    [[clears_notification]] unsigned char get_packet(packet_control_t pkt_ctrl[]);

};

interface tx_config {
    [[guarded]] void put_packet_ctrl_to_tx(packet_control_t pkt_ctrl[],unsigned char num_of_pkt);
    [[notification]] slave void tx_completed(void);
    [[guarded,clears_notification]] unsigned char get_tx_pkt_info(tx_packet_info_t txpkt_info[]);
};

interface rx_config {
    [[guarded]] void put_packet_num_to_rx(unsigned char num_of_pkt);
    [[notification]] slave void rx_completed(void);
    [[guarded,clears_notification]] unsigned char get_rx_pkt_info(rx_packet_info_t rxpkt_info[]);
};

void data_handler(server interface xscope_config i_xscope_config,
                  server interface data_manager i_data_manager);
void data_controller(client interface data_manager i_data_manager,
                     client interface tx_config i_tx_config,
                     client interface rx_config i_rx_config);

#ifdef __XC__
#define CHANEND_PARAM(param, name) param name
#else
#define CHANEND_PARAM(param, name) unsigned name
#endif

#endif /* __COMMON_H_ */

