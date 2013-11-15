#include <xs1.h>
#include <print.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <platform.h>
#include <assert.h>
#include "random.h"
#include "common.h"
#include <xscope.h>
#include "smi.h"
#define DEBUG_UNIT APP_MII_TESTER
#include "debug_print.h"

#define XSCOPE_TILE     0
#define ETHERNET_TILE   1

#define PORT_ETH_RXCLK_0 on tile[1]: XS1_PORT_1B
#define PORT_ETH_RXD_0 on tile[1]: XS1_PORT_4A
#define PORT_ETH_TXD_0 on tile[1]: XS1_PORT_4B
#define PORT_ETH_RXDV_0 on tile[1]: XS1_PORT_1C
#define PORT_ETH_TXEN_0 on tile[1]: XS1_PORT_1F
#define PORT_ETH_TXCLK_0 on tile[1]: XS1_PORT_1G
#define PORT_ETH_MDIOC_0 on tile[1]: XS1_PORT_4C
#define PORT_ETH_MDIOFAKE_0 on tile[1]: XS1_PORT_8A
#define PORT_ETH_ERR_0 on tile[1]: XS1_PORT_4D

#define PORT_ETH_RXCLK_1 on tile[1]: XS1_PORT_1J
#define PORT_ETH_RXD_1 on tile[1]: XS1_PORT_4E
#define PORT_ETH_TXD_1 on tile[1]: XS1_PORT_4F
#define PORT_ETH_RXDV_1 on tile[1]: XS1_PORT_1K
#define PORT_ETH_TXEN_1 on tile[1]: XS1_PORT_1L
#define PORT_ETH_TXCLK_1 on tile[1]: XS1_PORT_1I
#define PORT_ETH_MDIO_1 on tile[1]: XS1_PORT_1M
#define PORT_ETH_MDC_1 on tile[1]: XS1_PORT_1N
#define PORT_ETH_INT_1 on tile[1]: XS1_PORT_1O
#define PORT_ETH_ERR_1 on tile[1]: XS1_PORT_1P

#define ETH_SFD    0xD    /**< Start of Frame Delimiter */

on tile[1]: smi_interface_t smi0 = { 0x80000000, XS1_PORT_8A, XS1_PORT_4C };
on tile[1]: smi_interface_t smi1 = { 0, XS1_PORT_1M, XS1_PORT_1N };

on tile[1]: out port tx_prf_gpio_0 = XS1_PORT_1E;
on tile[1]: out port tx_prf_gpio_1 = XS1_PORT_1H;

typedef struct mii_tx_ports {
  out buffered port:32    txd;    /**< MII TX data wire */
  in port                 txclk;  /**< MII TX clock wire */
  out port                txen;   /**< MII TX enable wire */
  clock                   clk_tx; /**< MII TX Clock Block **/
}mii_tx_ports_t;

typedef struct mii_rx_ports {
  in buffered port:32    rxd;    /**< MII RX data wire */
  in port                rxdv;   /**< MII RX data valid wire */
  in port                rxclk;  /**< MII RX clock wire */
  clock                  clk_rx; /**< MII RX Clock Block **/
  in port                rxer;   /**< MII RX error wire */
} mii_rx_ports_t;

on tile[1] : mii_tx_ports_t tx0 = {
  PORT_ETH_TXD_0,
  PORT_ETH_TXCLK_0,
  PORT_ETH_TXEN_0,
  XS1_CLKBLK_2,
};

on tile[1] : mii_rx_ports_t rx0 = {
  PORT_ETH_RXD_0,
  PORT_ETH_RXDV_0,
  PORT_ETH_RXCLK_0,
  XS1_CLKBLK_1,
  PORT_ETH_ERR_0
};
on tile[1] : mii_rx_ports_t rx1 = {
  PORT_ETH_RXD_1,
  PORT_ETH_RXDV_1,
  PORT_ETH_RXCLK_1,
  XS1_CLKBLK_3,
  PORT_ETH_ERR_1
};
on tile[1] : mii_tx_ports_t tx1 = {
  PORT_ETH_TXD_1,
  PORT_ETH_TXCLK_1,
  PORT_ETH_TXEN_1,
  XS1_CLKBLK_4
};

void xscope_user_init(void) {
  xscope_register(1, XSCOPE_CONTINUOUS, "ack", XSCOPE_UINT, "ack");
  xscope_config_io(XSCOPE_IO_BASIC);
}

// Timing tuning constants
#define PAD_DELAY_RECEIVE    0
#define PAD_DELAY_TRANSMIT   0
#define CLK_DELAY_RECEIVE    0
#define CLK_DELAY_TRANSMIT   7

/*
 * SMI must start in 100 Mb/s half duplex with auto-neg off.
 */
static void rx_init(mii_rx_ports_t &p) {
  set_port_use_on(p.rxclk);
  p.rxclk :> int x;
  set_port_use_on(p.rxd);
  set_port_use_on(p.rxdv);
  set_port_use_on(p.rxer);
  set_pad_delay(p.rxclk, PAD_DELAY_RECEIVE);
  set_port_strobed(p.rxd);
  set_port_slave(p.rxd);
  set_clock_on(p.clk_rx);
  set_clock_src(p.clk_rx, p.rxclk);
  set_clock_ready_src(p.clk_rx, p.rxdv);
  set_port_clock(p.rxd, p.clk_rx);
  set_port_clock(p.rxdv, p.clk_rx);
  set_clock_rise_delay(p.clk_rx, CLK_DELAY_RECEIVE);
  start_clock(p.clk_rx);
  clearbuf(p.rxd);
}
static void tx_init(mii_tx_ports_t &p) {
  set_port_use_on(p.txclk);
  set_port_use_on(p.txd);
  set_port_use_on(p.txen);
  set_pad_delay(p.txclk, PAD_DELAY_TRANSMIT);
  p.txd <: 0;
  p.txen <: 0;
  sync(p.txd);
  sync(p.txen);
  set_port_strobed(p.txd);
  set_port_master(p.txd);
  clearbuf(p.txd);
  set_port_ready_src(p.txen, p.txd);
  set_port_mode_ready(p.txen);
  set_clock_on(p.clk_tx);
  set_clock_src(p.clk_tx, p.txclk);
  set_port_clock(p.txd, p.clk_tx);
  set_port_clock(p.txen, p.clk_tx);
  set_clock_fall_delay(p.clk_tx, CLK_DELAY_TRANSMIT);
  start_clock(p.clk_tx);
  clearbuf(p.txd);
}
/*
 *
 */
void rx(in buffered port:32 rxd, in port rxdv, chanend c_rx_to_timestamp)
{
  timer t;
  unsigned buffer[MAX_BUFFER_WORDS];

  while(1) {
    unsigned end_of_frame = 0;
    unsigned word_count = 0;
    unsigned byte_count = 0;
    unsigned rx_ticks = 0;
    unsigned checksum = 0;

    rxdv when pinseq(1) :> int;
    rxd when pinseq(ETH_SFD) :> int;

    t :> rx_ticks;

    c_rx_to_timestamp <: rx_ticks;

    while(!end_of_frame) {
      select {

        case rxd :> unsigned word: {
          buffer[word_count++] = word;
          byte_count+=4;
          break;
        }

        case rxdv when pinseq(0) :> int: {
          unsigned tail;
          unsigned taillen = endin(rxd);
          rxd :> tail;

          if(taillen) {
            tail = tail >> (32 - taillen);
            buffer[word_count++] = tail;
            byte_count += (taillen >> 3);

            checksum = ((tail << (32 - taillen)) | (buffer[word_count-2] >> taillen));
          }
          else {
            checksum = buffer[word_count-1];
          }

          end_of_frame = 1;
          break;
        } /**< rxdv low */
      }
    } /**< frame */
  }
}
/*
 *
 */
void tx(out buffered port:32 txd, chanend c_data_handler_to_tx,chanend c_tx_to_timestamp)
{
  unsigned tx_ticks;
  unsigned char data_buff[1522];
  uintptr_t dptr;
  unsigned idx;
  timer t;

  for(idx = 0;idx<12;idx++)
    data_buff[idx] = 0xFF;

  data_buff[12] = 0xAB; data_buff[13] = 0x88;

  for(int idx=14;idx<(MAX_FRAME_SIZE-CRC_BYTES);idx++)
    data_buff[idx] = ((idx-14)%255)+1;    // initialize packet data with 1,2,3,..255

  asm volatile("mov %0, %1": "=r"(dptr):"r"(data_buff));

  while(1) {

    unsigned wait_time = 1000;
    unsigned size_in_bytes = 1292;
    unsigned checksum = 0xD323E598;
    unsigned data;

    slave {
      c_data_handler_to_tx :> wait_time;
      c_data_handler_to_tx :> size_in_bytes;
      c_data_handler_to_tx :> checksum;
    }
    memcpy(&data_buff[size_in_bytes],&checksum,CRC_BYTES);

    t :> tx_ticks;
    /**< when a tx cmd come from tx_to_app then send it out */
    t when timerafter(tx_ticks+wait_time) :> tx_ticks;

    c_tx_to_timestamp <: tx_ticks;

    txd <: 0x55555555;              /**< send ethernet preamble */
    txd <: 0xD5555555;              /**< send Start of frame delimiter */

    /**< send data from pointer, including checksum */
    for(idx=0; idx<((size_in_bytes+CRC_BYTES)>>2);idx++) {
      asm volatile("ldw %0, %1[%2]":"=r"(data):"r"(dptr), "r"(idx):"memory");
      txd <: data;
    }

    asm volatile("ldw %0, %1[%2]":"=r"(data):"r"(dptr), "r"(idx):"memory");
    /**< send the remaining no of bytes, if not in 4byte offset */
    if(size_in_bytes&3) {
      unsigned tailllen = ((size_in_bytes&3)*8);
      partout(txd, tailllen, data);
    }

    c_data_handler_to_tx <: HOST_CMD_TX_ACK;
  }
}
/*
 *
 */
void time_stamp(chanend c_tx_to_timestamp,chanend c_rx_to_timestamp)
{
  unsigned time_stamp_flag = 0;
  unsigned start_ticks,stop_ticks;

  while(1) {
    select {

      case c_tx_to_timestamp :> start_ticks:
        time_stamp_flag = 1;
        break;

      case c_rx_to_timestamp :> stop_ticks: {
        // Ignore spurious stop
        if (!time_stamp_flag)
          break;
        time_stamp_flag = 0;
        break;
      }
    }
  }
}
void data_handler(server interface xscope_config i_xscope_config,chanend c_data_handler_to_tx)
{
  packet_control_t packet_control[MAX_PACKET_SEQUENCE];
  int eop_flag = 0;
  char buff_access_flag = 0;
  unsigned frames_tobe_sent = 0;

  while(1) {
    select {

      case i_xscope_config.put_buffer(unsigned int xscope_buff[]): {
        unsigned packet_number = 0;
        if( (!buff_access_flag) && (!eop_flag) ){
          buff_access_flag = 1;

          packet_number = (xscope_buff[0] % END_OF_PACKET_SEQUENCE);

          assert(packet_number < MAX_PACKET_SEQUENCE);
          packet_control[packet_number].packet_number = packet_number;
          packet_control[packet_number].frame_delay = xscope_buff[1];
          packet_control[packet_number].frame_size = xscope_buff[2];
          packet_control[packet_number].frame_crc = xscope_buff[3];

          if(xscope_buff[0] & END_OF_PACKET_SEQUENCE) {
              frames_tobe_sent = packet_number+1;      /**< Added '1' since packet number always starts with '0' */
              eop_flag = 1;
          }

          buff_access_flag = 0;
        }
        else
          debug_printf("Frame Arrived During buffer handling or During Tx\n");

        break;
      }

      eop_flag => default:
          unsigned frames_send = 0;
          while(frames_send < frames_tobe_sent){
            master {
                c_data_handler_to_tx <: packet_control[frames_send].frame_delay;
                c_data_handler_to_tx <: packet_control[frames_send].frame_size;
                c_data_handler_to_tx <: packet_control[frames_send].frame_crc;
            }
            c_data_handler_to_tx :> unsigned temp;
            frames_send+=1;
          }
          eop_flag = 0;
       break;

    }
  }
}
/**
 * \brief   A core that listens to data being sent from the host and
 *          informs the data generator to form data
 */
void xscope_listener(chanend c_host_data,client interface xscope_config i_xscope_config)
{
  unsigned int xscope_buff[256/4];

  xscope_connect_data_from_host(c_host_data);

  while(1) {
    int num_byte_read;
    int data_gen_ack = 1;
    select {

      case xscope_data_from_host(c_host_data, (unsigned char *)xscope_buff, num_byte_read): {
        if(num_byte_read != 0) {
            i_xscope_config.put_buffer(&xscope_buff[0]);
            xscope_int(0, data_gen_ack);
        }
        break;
      }

    }
    //xscope_int(0, data_gen_ack);
  }
}
/*
 *
 */
int main(void) {

  chan c_host_data;
  chan c_data_handler_to_tx;
  chan c_tx_to_timestamp;
  chan c_rx_to_timestamp;
  interface xscope_config i_xscope_config;
  par {
    //xscope_host_data(c_host_data);
    on tile [1]: time_stamp(c_tx_to_timestamp,c_rx_to_timestamp);
    on tile [1]: data_handler(i_xscope_config,c_data_handler_to_tx);
    on tile [1]: xscope_listener(c_host_data,i_xscope_config);

    on tile [1]: {
      //set_core_fast_mode_on();
      rx_init(rx1);
      rx(rx1.rxd, rx1.rxdv, c_rx_to_timestamp);  /**< Receive Frames on circle slot */
    }
    on tile [1]: {
      //set_core_fast_mode_on();
      smi_init(smi0);
      tx_init(tx0);
      tx(tx0.txd,c_data_handler_to_tx,c_tx_to_timestamp);            /**< Transmit Frames on square slot */
    }

    on tile[1]: {
          timer t;
          int x;
          for (int i = 0; i < 200; i++) {
            t :> x;
            t when timerafter(x+100000) :> x;
            xscope_int(0, 1);
          }
        }
  }

  return 0;
}



