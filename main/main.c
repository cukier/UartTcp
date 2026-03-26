#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "ksys.h"
#include "ktcp.h"
#include "kuart.h"

QueueHandle_t uart_tx_queue;
QueueHandle_t tcp_tx_queue;
int current_client_sock = -1;

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());

  bridge_tcp_init();
  uart_init();

  uart_tx_queue = xQueueCreate(10, sizeof(bridge_msg_t));
  tcp_tx_queue = xQueueCreate(10, sizeof(bridge_msg_t));

  xTaskCreate(tcp_rx_task, "tcp_rx", 4096, NULL, 5, NULL);
  xTaskCreate(tcp_tx_task, "tcp_tx", 4096, NULL, 5, NULL);
  xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, NULL);
  xTaskCreate(uart_tx_task, "uart_tx", 4096, NULL, 5, NULL);
}
