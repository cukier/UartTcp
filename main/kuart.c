#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/uart.h"

#include "ksys.h"

#define UART_PORT_NUM UART_NUM_1

extern QueueHandle_t uart_tx_queue; // TCP RX -> UART TX
extern QueueHandle_t tcp_tx_queue;  // UART RX -> TCP TX

void uart_init() {
  uart_config_t uart_config = {.baud_rate = CONFIG_BRIDGE_UART_BAUD,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

#if defined(CONFIG_BRIDGE_UART_PARITY_EVEN)
  uart_config.parity = UART_PARITY_EVEN;
#elif defined(CONFIG_BRIDGE_UART_PARITY_ODD)
  uart_config.parity = UART_PARITY_ODD;
#endif

  uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(UART_NUM_1, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_rx_task(void *pvParameters) {
  bridge_msg_t msg;

  while (1) {
    int len = uart_read_bytes(UART_PORT_NUM, msg.data, BUF_SIZE,
                              20 / portTICK_PERIOD_MS);
    if (len > 0) {
      msg.length = len;
      xQueueSend(tcp_tx_queue, &msg, portMAX_DELAY);
    }
  }
}

void uart_tx_task(void *pvParameters) {
  bridge_msg_t msg;

  while (1) {
    if (xQueueReceive(uart_tx_queue, &msg, portMAX_DELAY) == pdPASS) {
      uart_write_bytes(UART_PORT_NUM, (const char *)msg.data, msg.length);
    }
  }
}
