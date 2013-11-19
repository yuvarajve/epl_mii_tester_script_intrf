#ifndef __MII_TESTER_H__
#define __MII_TESTER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ETH_SPEED              100    // in Mbps
#define MAC_DST_BYTES          6
#define MAC_SRC_BYTES          6
#define ETH_TYPE_BYTES         2
#define MIN_NO_OF_PACKET       1
#define MAX_NO_OF_PACKET       20
#define MIN_FRAME_SIZE         64      
#define MAX_FRAME_SIZE         1522   
#define CRC_BYTES			   sizeof(unsigned int)
#define PKT_DELAY_BYTES        sizeof(unsigned int)
#define PKT_SIZE_BYTES         sizeof(unsigned int)
#define MIN_IFG_BYTES          12       // Caution: Don't go below this
#define MAX_IFG_BYTES          1250     // 
#define MAX_BYTES_CAN_SEND     256
#define END_OF_PACKET_SEQUENCE (1<<5)

/* The entire Ethernet Packet Sequence and sent from host 
* +------------------------------------------------------+
* | packet_number | frame_delay | frame_size | frame_crc |
* +------------------------------------------------------+
* packet_number : 6-bits  ---+
* frame_delay   : 15-bits    | -> 32-bits
* frame_size    : 11-bits ---+
* frame_crc     : 32-bit  
*/

// packet control
typedef struct packet_control{
  unsigned int frame_info;
  unsigned int frame_crc;
}packet_control_t;

#define PKT_CTRL_BYTES       sizeof(packet_control_t)
#define ETH_FRAME_BYTES      sizeof(ethernet_frame_t)
#define DEFAULT_LEN          (PKT_DELAY_BYTES + PKT_SIZE_BYTES + CRC_BYTES)  

#endif // __MII_TESTER_H__
