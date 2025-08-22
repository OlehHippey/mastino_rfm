/*
 * dnsudp.c
 * Created on: 27 июн. 2025 г.
 * Author: ProgMen
 */

// dnsudp.c — оновлена версія з уніфікованою обробкою JSON

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "local_handle.h"
#include "global.h"
#define UDP_PORT 12345
#define RECV_BUF_LEN 256
static const char *TAG = "udp_server";

static void udp_server_task(void *pvParameters) {
    char rx_buffer[RECV_BUF_LEN];
    struct sockaddr_in listen_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(UDP_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket creation failed: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        ESP_LOGE(TAG, "Socket bind failed: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "UDP server listening on port %d", UDP_PORT);

    while (1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);

        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                           (struct sockaddr *)&source_addr, &socklen);

        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            continue;
        }

        rx_buffer[len] = 0;
        ESP_LOGI(TAG, "Received: %s", rx_buffer);

        char reply_buf[512] = {0};
        handle_command_json(rx_buffer, reply_buf, sizeof(reply_buf));

        sendto(sock, reply_buf, strlen(reply_buf), 0,
               (struct sockaddr *)&source_addr, sizeof(source_addr));
    }

    close(sock);
    vTaskDelete(NULL);
}

void start_udp_server(void) {

    mdns_udp_started = 2;
    ESP_LOGW("start_udp_server", "****************** CURRENT DEVICE UDP STARTED *******************");
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
}
