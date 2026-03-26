#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "ksys.h"
#include "ktcp.h"
#include "kuart.h"

static ksys_t *k_sys_init() {
  ksys_t *ret = calloc(1, sizeof(ksys_t));

  configASSERT(ret);

  ret->tcp_tx_queue = xQueueCreate(10, sizeof(bridge_msg_t));
  ret->uart_tx_queue = xQueueCreate(10, sizeof(bridge_msg_t));

  configASSERT(ret->tcp_tx_queue);
  configASSERT(ret->uart_tx_queue);

  return ret;
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());

  ksys_t *sys = k_sys_init();

  bridge_tcp_init((void *)sys);
  uart_init((void *)sys);

  xTaskCreate(tcp_rx_task, "tcp_rx", 4096, NULL, 5, NULL);
  xTaskCreate(tcp_tx_task, "tcp_tx", 4096, NULL, 5, NULL);
  xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL, 5, NULL);
  xTaskCreate(uart_tx_task, "uart_tx", 4096, NULL, 5, NULL);
}
