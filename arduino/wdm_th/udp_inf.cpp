
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "device.h"
#include "esp8266_mlib.h"
#include "udp_inf.h"

#define DB      Serial.printf
#ifndef DB
  #define DB
#endif

///////////////////////////////////////LOCAL CONSTANTS/////////////////////////////////////////////
#define UDP_PACKET_SIZE_MAX     128

#define PACKET_MARKER           0xa8

#define OPU_CONNECT             0x01
#define OPU_STATUS              0x02
#define OPU_ACK                 0x03

#define OPH_CONNACK             0x01
#define OPH_ACK                 0x02
#define OPH_CMD                 0x03

///////////////////////////////////////LOCAL VARIABLES/////////////////////////////////////////////
/* UDP settings */
static uint16_t g_server_port;
static char *g_security;
static uint8_t g_node_id[6];

/* Status */
static IPAddress g_server_ip;
static uint32_t g_tx_sequence;

/* UDP socket */
static WiFiUDP udp;
static uint8_t g_ack_flg;

///////////////////////////////////////PUBLIC FUNCTIONS////////////////////////////////////////////

void udp_inf::init(const uint8_t *id, const char *security, uint32_t server_ip, uint16_t server_port)
{
    memcpy(g_node_id, id, 6);
    g_server_ip = IPAddress(server_ip);
    g_server_port = server_port;

    // Init UDP:
    udp.begin(0);
}

/*  UDP packet: [Marker=0xa0][Sequence(4)][NodeId(6)][Frame()][FCS(8)]
*       Frame()         = [Opcode(1)][Data()]
        Data()          = [DevCnt(1)=N][DeviceStatusList(N x 4)]
        DeviceStatus()  = [Offset(1)][Type(1)][Value(4)][Rssi(1)][Power(1)]
*   
*/
void udp_inf::send_STATUS(int dev_cnt, DEVICE_INFO_t dev_list[])
{
    static uint8_t tx_buf[128];
    uint8_t i = 0;
    uint8_t dev_idx = 0;

    DB("\r\n%s: dev_cnt=%u", __FUNCTION__, dev_cnt);

    // Limit device count to 10:
    if (dev_cnt > 10) {
        return;
    }

    // Create packet:
    tx_buf[i++] = PACKET_MARKER;    
    g_tx_sequence++;    
    esp8266_mlib::u32_to_buf(g_tx_sequence, &tx_buf[i]);
    i += 4;
    memcpy(&tx_buf[i], g_node_id, 6);
    i += 6;
    tx_buf[i++] = OPU_STATUS; // OPU_STATUS
    tx_buf[i++] = dev_cnt; // ONE device.
    for (dev_idx = 0; dev_idx < dev_cnt; dev_idx++) {
        const DEVICE_INFO_t *p_dev = &dev_list[dev_idx];
        tx_buf[i++] = p_dev->offset;
        tx_buf[i++] = p_dev->type;        
        esp8266_mlib::u32_to_buf(p_dev->v, &tx_buf[i]);
        i += 4;
        tx_buf[i++] = p_dev->r;
        tx_buf[i++] = p_dev->p;
    }
    memset(&tx_buf[i], 0x00, 8);
    i += 8;

    // Send:
    if (!udp.beginPacket(g_server_ip, g_server_port)) {
        DB(" -> begin failed!");
        return;
    }
    if (udp.write(tx_buf, i) != i) {
        DB(" -> write fail: not enough memory??");
    }
    if (!udp.endPacket()) {
        DB(" -> sending failed");
        return;
    }

    // Without the following command, the UDP packet will not be sent when enter DeepSleep right after calling this function.
    yield();

    // Wait for ACK:
    g_ack_flg = 0;
    for (i = 0; i < 20; i++) {   
        rx_manager();
        if (g_ack_flg) {
            DB(" -> received ACK!");
            break;
        }
        delay(50);
        DB(".");
    }
}

void udp_inf::rx_manager()
{
    static byte rx_buf[UDP_PACKET_SIZE_MAX];
    int rx_sz = udp.parsePacket();

    if (rx_sz > 0) {
        DB("\r\n -> Received %d bytes from %s, port %d\n", rx_sz, udp.remoteIP().toString().c_str(), udp.remotePort());
        
        int len = udp.read(rx_buf, UDP_PACKET_SIZE_MAX);
        if (len > 0) {
            uint32_t idx = 0;
            uint8_t rx_marker = rx_buf[idx++];
            uint32_t seq = esp8266_mlib::buf_to_u32(&rx_buf[idx]);
            idx += 4;
            uint8_t rx_id[6];
            memcpy(rx_id, &rx_buf[idx], 6);
            idx += 6;
            
            // Validate header:            
            if (rx_marker != PACKET_MARKER) {
                DB(" -> Invalid Marker!");
                return;
            }
            for (int i = 0; i < 6; i++) {
                if (rx_id[i] != g_node_id[i]) {
                    DB(" -> Invalid ID!");
                    return;
                }
            }

            // Process based on Opcode:
            uint8_t rx_op = rx_buf[idx++];
            DB(" -> op=%02Xh", rx_op);
            if (rx_op == OPH_ACK) {
                uint32_t ack_seq = esp8266_mlib::buf_to_u32(&rx_buf[idx]);
                idx += 4;
                uint8_t ack_op = rx_buf[idx++];
                DB(" -> ack.seq=%d, ack.op=%d", ack_seq, ack_op);
                if ((rx_op == OPH_ACK)) {
                    DB(" -> ACKed!!!");
                    g_ack_flg = 1;
                }
            } else if (rx_op == OPH_CMD) {
                uint8_t offset = rx_buf[idx++];
                uint8_t cmd = rx_buf[idx++];

                DB(" -> offset=%u, cmd=%u", offset, cmd);              
            }
        }
    }
}
