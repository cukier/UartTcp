#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "lwip/sockets.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "ksys.h"

static QueueHandle_t uart_tx_queue = NULL; // TCP RX -> UART TX
static QueueHandle_t tcp_tx_queue = NULL;  // UART RX -> TCP TX
static SemaphoreHandle_t semphr = NULL;
static int current_client_sock = -1;

void bridge_tcp_init(void *pvParameters) {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  uart_tx_queue = ((ksys_t *)pvParameters)->uart_tx_queue;
  tcp_tx_queue = ((ksys_t *)pvParameters)->tcp_tx_queue;

  semphr = xSemaphoreCreateMutex();
  configASSERT(semphr);

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_BRIDGE_WIFI_SSID,
              .password = CONFIG_BRIDGE_WIFI_PASSWORD,
              .threshold.authmode = WIFI_AUTH_WPA2_PSK, // Typical security
          },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
}

void tcp_rx_task(void *pvParameters) {
  struct sockaddr_in addr;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_family = AF_INET;
  addr.sin_port = htons(CONFIG_BRIDGE_TCP_PORT);

  int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr));
  listen(listen_sock, 1);

  for (;;) {
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

    if (sock < 0)
      continue;

    if (xSemaphoreTake(semphr, pdMS_TO_TICKS(100)) == pdTRUE) {
      current_client_sock = sock;
      xSemaphoreGive(semphr);
    }

    for (;;) {
      uint8_t data[BUF_SIZE] = {0};

      int len = recv(sock, data, BUF_SIZE, 0);

      if (len > 0) {
        bridge_msg_t msg = {.data = malloc(len), .length = len};

        if (msg.data != NULL) {
          memcpy(msg.data, data, len);

          if (xQueueSend(uart_tx_queue, &msg, portMAX_DELAY) != pdPASS) {
            free(msg.data);
          }
        }
      }
    }

    if (xSemaphoreTake(semphr, pdMS_TO_TICKS(100)) == pdTRUE) {
      current_client_sock = -1;
      xSemaphoreGive(semphr);
    }

    close(sock);
  }
}

void tcp_tx_task(void *pvParameters) {
  bridge_msg_t msg = {0};
  int sock = -1;

  while (1) {
    if (xQueueReceive(tcp_tx_queue, &msg, portMAX_DELAY) == pdPASS) {
      if (xSemaphoreTake(semphr, pdMS_TO_TICKS(100)) == pdTRUE) {
        sock = current_client_sock;
        xSemaphoreGive(semphr);
      }

      if (sock != -1) {
        send(sock, msg.data, msg.length, 0);
        sock = -1;
      }
    }
  }
}
