#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "ksys.h"

extern QueueHandle_t uart_tx_queue; // TCP RX -> UART TX
extern QueueHandle_t tcp_tx_queue;  // UART RX -> TCP TX

void bridge_tcp_init() {
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

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

  while (1) {
    struct sockaddr_in source_addr;
    socklen_t addr_len = sizeof(source_addr);
    int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
    if (sock < 0)
      continue;

    // Signal TCP TX task which socket to use (e.g., via global or event group)
    extern int current_client_sock;
    current_client_sock = sock;

    bridge_msg_t msg;
    while (1) {
      int len = recv(sock, msg.data, BUF_SIZE, 0);
      if (len <= 0)
        break;

      msg.length = len;
      xQueueSend(uart_tx_queue, &msg, portMAX_DELAY);
    }

    current_client_sock = -1;
    close(sock);
  }
}

void tcp_tx_task(void *pvParameters) {
  bridge_msg_t msg;
  extern int current_client_sock;

  while (1) {
    if (xQueueReceive(tcp_tx_queue, &msg, portMAX_DELAY) == pdPASS) {
      if (current_client_sock != -1) {
        send(current_client_sock, msg.data, msg.length, 0);
      }
    }
  }
}
