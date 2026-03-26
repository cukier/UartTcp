#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "driver/uart.h"

#include "ksys.h"

#include <string.h>

#define UART_PORT_NUM UART_NUM_1

static QueueHandle_t uart_tx_queue = NULL; // TCP RX -> UART TX
static QueueHandle_t tcp_tx_queue = NULL;  // UART RX -> TCP TX

void uart_init(void *pvParameters) {
  uart_config_t uart_config = {.baud_rate = CONFIG_BRIDGE_UART_BAUD,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_tx_queue = ((ksys_t *)pvParameters)->uart_tx_queue;
  tcp_tx_queue = ((ksys_t *)pvParameters)->tcp_tx_queue;

#if defined(CONFIG_BRIDGE_UART_PARITY_EVEN)
  uart_config.parity = UART_PARITY_EVEN;
#elif defined(CONFIG_BRIDGE_UART_PARITY_ODD)
  uart_config.parity = UART_PARITY_ODD;
#endif

  uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM_1, &uart_config);
  uart_set_pin(CONFIG_BRIDGE_UART_PORT, CONFIG_BRIDGE_UART_TX_GPIO, CONFIG_BRIDGE_UART_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void uart_rx_task(void *pvParameters) {
  for (;;) {
    uint8_t data[BUF_SIZE] = {0};

    int len =
        uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, pdMS_TO_TICKS(100));

    if (len > 0) {
      bridge_msg_t msg = {.data = malloc(len), .length = len};

      if (msg.data != NULL) {
        memcpy(msg.data, data, len);

        if (xQueueSend(tcp_tx_queue, &msg, portMAX_DELAY) != pdPASS) {
          free(msg.data);
        }
      }
    }

    taskYIELD();
  }

  vTaskDelete(NULL);
}

void uart_tx_task(void *pvParameters) {
  for (;;) {
    bridge_msg_t msg = {0};

    if (xQueueReceive(uart_tx_queue, &msg, portMAX_DELAY) == pdPASS) {
      uart_write_bytes(UART_PORT_NUM, (const char *)msg.data, msg.length);
      free(msg.data);
    }

    taskYIELD();
  }

  vTaskDelete(NULL);
}
