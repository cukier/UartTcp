#ifndef _MAIN_INCLUDE_KUART_H
#define _MAIN_INCLUDE_KUART_H

void uart_init(void *pvParameters);
void uart_tx_task(void *pvParameters);
void uart_rx_task(void *pvParameters);

#endif