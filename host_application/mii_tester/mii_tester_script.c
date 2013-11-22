/*
* Note that the device and listener should be run with the same port and IP.
* For example:
*
*  xrun --xscope-realtime --xscope-port 127.0.0.1:12346 app_mii_tester.xe
*
*  ./mii_tester_script -s 127.0.0.1 -p 12346
*
*/
#include "shared.h"
#include "mii_tester.h"
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

/*
* Includes for thread support
*/
#ifdef _WIN32
#include <winsock.h>
#else
#include <pthread.h>
#endif

// Need to define this as NULL to indicate that there is no console being used
const char *g_prompt = NULL;

extern int xscope_ep_upload_pending;
int packet_sequence_active = 0;
int g_i = 0;
int g_started = 0;
void hook_data_received(void *data, int data_len)
{
  assert(data_len == 8);
  g_i++;
  if (g_i == 199)
    g_started = 1;

  int value = *((int*)data);
  if (value == 1)
    xscope_ep_upload_pending = 0;
  else if (value == 2)
    packet_sequence_active = 0;
}

void hook_exiting()
{
}

const unsigned char mac_addr_destn[MAC_DST_BYTES] = {255,255,255,255,255,255};
const unsigned char mac_addr_src[MAC_SRC_BYTES]   = {255,255,255,255,255,255};
const unsigned char ether_type[ETH_TYPE_BYTES]    = {0xAB, 0x88};
unsigned char packet_data[MAX_FRAME_SIZE]         = {0};
const unsigned int polynomial                     = 0xEDB88320;
const unsigned int initial_crc                    = 0x9226F562;

/*
* Incorporate a word into a Cyclic Redundancy Checksum.
*/
static unsigned int crc8(unsigned int checksum, unsigned int data)
{
  int i;

  for(i = 0; i < 8; i++) {
    int xorBit = (checksum & 1);

    checksum = ((checksum >> 1) | ((data & 1) << 31));
    data = data >> 1;

    if(xorBit)
	  checksum = checksum ^ polynomial;
  }

  return checksum;
}
// rand() returns number from 0 to 32767 (0x0000 to 0x7FFF)
unsigned char get_random_packets(void)
{
  return( (rand() %( MAX_NO_OF_PACKET - MIN_NO_OF_PACKET + 1) + MIN_NO_OF_PACKET) );
}
unsigned int get_random_packet_size(void)
{
  return( (rand() %( MAX_FRAME_SIZE - MIN_FRAME_SIZE + 1) + MIN_FRAME_SIZE) );
}
/* 
* Time of 1bit frame @ 100Mbps
* 1 bit = (1/100e6) = 10nSec = 1 Timer Tick on xcore 
* IFG delay @ 100Mbps = 10nSec * 12bytes * 8bits = 960nSec = 96 Timer Tick on xcore = 12*8*1
*
* Time of 1bit frame @ 10Mbps
* 1 bit = (1/10e6) = 100nSec = 10 Timer Tick on xcore 
* IFG delay @ 10Mbps = 100nSec * 12bytes * 8bits = 9600nSec = 960 Timer Tick on xcore = 12*8*10
*/
unsigned int get_random_ifg_delay(void)
{
  unsigned int random_ifg_delay = 0;
  unsigned int min, max;

  if ((rand() % 100) > 10) {
    min = 8 * 8;
    max = 20 * 8;
  } else {
    min = MIN_IFG_BYTES * 8;
    max = MAX_IFG_BYTES * 8;
  }

  if (ETH_SPEED == 10) {
    min = min * 10;
    max = max * 10;
  }

  if (max != min) {
    random_ifg_delay = (rand() % (max - min)) + min;
  } else {
    random_ifg_delay = min;
  }
  return random_ifg_delay;
}

void get_initialised(void)
{
  int idx;

  memcpy(&(packet_data[0]), mac_addr_destn, MAC_DST_BYTES);
  memcpy(&(packet_data[6]), mac_addr_src, MAC_SRC_BYTES);
  memcpy(&(packet_data[12]), ether_type, ETH_TYPE_BYTES);

  for(idx=14; idx<MAX_FRAME_SIZE; idx++){
    packet_data[idx] = ((idx-14)%255)+1;    // initialize packet data with 1,2,3,..255
  }
}
int send_packet(int sockfd,unsigned char pkt_no)
{
  unsigned int idx,crc_value = initial_crc;
  unsigned int delay = get_random_ifg_delay();
  unsigned int num_data_bytes = get_random_packet_size();
  unsigned char pBuffer[MAX_BYTES_CAN_SEND] = {0};
  time_t start_time, current_time;
  int retval = XSCOPE_EP_FAILURE;

  packet_control_t packet_control;

  assert((num_data_bytes >= MIN_FRAME_SIZE) && (num_data_bytes <= MAX_FRAME_SIZE));

  packet_control.frame_info  = ((pkt_no & 0x3F) << 26);
  packet_control.frame_info |= ((delay & 0x7FFF) << 11);
  packet_control.frame_info |= ((num_data_bytes-CRC_BYTES) & 0x7FF);

  for(idx = 0; idx < (num_data_bytes-CRC_BYTES); idx++){
    crc_value = crc8(crc_value, packet_data[idx]);
  }		
  
  packet_control.frame_crc = crc_value;

  memcpy(pBuffer,(unsigned char *)&packet_control,PKT_CTRL_BYTES);

  time(&current_time);
  start_time = current_time;
  while ((retval != XSCOPE_EP_SUCCESS) && ((current_time - start_time) < 5)) {
    retval = xscope_ep_request_upload(sockfd, PKT_CTRL_BYTES, pBuffer);
    time(&current_time); // wait till we get success
  }
  
  assert(retval == XSCOPE_EP_SUCCESS);
//  Sleep(30);
  //printf("| %02d |  %05d  | %06d  | 0x%08X |\n",(pkt_no%END_OF_PACKET_SEQUENCE)+1,delay,(num_data_bytes-CRC_BYTES),packet_control.frame_crc);
  
  return 0;
}
/*
* A separate thread to generate random packets with random delay and size.
* This code is similar to random_traffic_generator
*/
#ifdef _WIN32
DWORD WINAPI packet_generation_thread(void *arg)
#else
void *packet_generation_thread(void *arg)
#endif
{
  int sockfd = *(int *)arg;
  unsigned char no_of_packets = 0;
  unsigned int total_no_of_packets = 0;
  int loop;
  
  while (!g_started);
  get_initialised();

  srand(time(0)); //initialize the seed
  
  //printf("+--------------------------------------------------------------------------------+\n");
  //printf("|       FROM  HOST  APPLICATION       |  FROM XCORE MII TESTER APPLICATION       |\n");
  //printf("+-------------------------------------+------------------------------------------+\n");
  //printf("| ## | TxDelay | PktSize |  Checksum  |                                          |\n");
  //printf("+-------------------------------------+------------------------------------------+\n");
	
  // Allow the target time to start
#ifdef _WIN32
  Sleep(2000);
#else
  sleep(2);
#endif

  while(1) {
    while (packet_sequence_active); // wait for sequence completion
    
    packet_sequence_active = 1;

    // get random packet number
    no_of_packets = get_random_packets(); 	
    total_no_of_packets += no_of_packets;
    printf("no_of_packets: %10d (%10d)\n", no_of_packets, total_no_of_packets);
    // always send no of packets less than '1', on last packet number add END_OF_PACKET
    for(loop=0; loop < no_of_packets-1; loop++)
    {
      if(send_packet(sockfd,loop) != XSCOPE_EP_SUCCESS)
        printf("send_packet : Failed !!\n");
    }

    /* send end of packet, so that mii tester xcore code starts
     * doing tx on mii lines
     */
    loop |= END_OF_PACKET_SEQUENCE;  
    if(send_packet(sockfd,loop) != XSCOPE_EP_SUCCESS)
      printf("send_packet : Failed !!\n");
	  
    printf("\n");
	
  }
  return 0;

}
void usage(char *argv[])
{
  printf("Usage: %s [-s server_ip] [-p port]\n", argv[0]);
  printf("  -s server_ip :   The IP address of the xscope server (default %s)\n", DEFAULT_SERVER_IP);
  printf("  -p port      :   The port of the xscope server (default %s)\n", DEFAULT_PORT);
  exit(1);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
  HANDLE pg_thread;
#else
  pthread_t pg_tid;
#endif

  char *server_ip = DEFAULT_SERVER_IP;
  char *port_str = DEFAULT_PORT;
  int err = 0;
  int sockfd = 0;
  int c = 0;

  while ((c = getopt(argc, argv, "s:p:")) != -1) {
    switch (c) {
      case 's':
        server_ip = optarg;
        break;
      case 'p':
        port_str = optarg;
        break;
      case ':': /* -f or -o without operand */
        fprintf(stderr, "Option -%c requires an operand\n", optopt);
        err++;
        break;
      case '?':
        fprintf(stderr, "Unrecognised option: '-%c'\n", optopt);
        err++;
    }
  }

  if (err)
    usage(argv);

  sockfd = initialise_common(server_ip, port_str);

  // Now start the packet generation
#ifdef _WIN32
  pg_thread = CreateThread(NULL, 0, packet_generation_thread, &sockfd, 0, NULL);
  if (pg_thread == NULL)
    print_and_exit("ERROR: Failed to create packet generation thread\n");
#else
  err = pthread_create(&pg_tid, NULL, &packet_generation_thread, &sockfd);
  if (err != 0)
    print_and_exit("ERROR: Failed to create packet generation thread\n");
#endif

  handle_socket(sockfd);

  return 0;
}

