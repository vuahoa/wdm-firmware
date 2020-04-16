#ifndef _UDP_INF_H_
#define _UDP_INF_H_

#include "device.h"

class udp_inf
{
    public:
        /* Initialize UDP communication */
        static void init(const uint8_t *id, const char *security, uint32_t server_ip, uint16_t server_port);
		
		/* Transmission functions */
        static void send_STATUS(int dev_cnt, DEVICE_INFO_t dev_list[]);

    private:
        static void rx_manager();
};

#endif
