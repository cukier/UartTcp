#ifndef _MAIN_INCLUDE_KTCP_H
#define _MAIN_INCLUDE_KTCP_H

void bridge_tcp_init(void *pvParameters);
void tcp_tx_task(void *pvParameters);
void tcp_rx_task(void *pvParameters);

#endif