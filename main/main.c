#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/usb_serial_jtag.h"

#include "wifi.h"
#include "dns_server.h"
#include "http_server.h"
#include "player.h"
#include "world.h"
#include "npc.h"
#include "commands.h"

#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"

static const char *TAG = "main";

extern void dns_server_task(void *pvParameters);
extern void http_server_task(void *pvParameters);

static void random_events_task(void *pvParameters)
{
    const char *events[] = {
        "\r\n[A strong wind scatters loose trash across the forest.]\r\n",
        "\r\n[The raccoon committee votes unanimously for more snacks.]\r\n",
        "\r\n[A mysterious boat horn echoes from the river.]\r\n",
        "\r\n[The Wolf sneezes. It echoes across the valley.]\r\n",
        "\r\n[A raccoon drops a trash can. Classic.]\r\n",
        "\r\n[The Oregon Sailor's boat rocks violently. Probably fine.]\r\n",
        "\r\n[A suspicious banana rolls down the hill.]\r\n",
        "\r\n[The Little Red Raccoon cackles maniacally in the distance.]\r\n",
        "\r\n[Someone's trash has been sorted. How dare they.]\r\n",
        "\r\n[A squirrel steals a bottle cap. Bold move.]\r\n",
    };
    const int event_count = sizeof(events) / sizeof(events[0]);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(120000 + rand() % 60000));

        if (player_get_active_count() > 0) {
            int idx = rand() % event_count;
            player_broadcast_all(events[idx], NULL);
        }
    }
}

void app_main(void)
{
    usb_serial_jtag_driver_config_t usb_config = {
        .rx_buffer_size = 256,
        .tx_buffer_size = 256,
    };
    usb_serial_jtag_driver_install(&usb_config);

    printf("\r\n=== BOOT START ===\r\n");
    fflush(stdout);

    srand(esp_random());

    ESP_LOGI(TAG, "Little Red Raccoon MUD starting...");

    player_init();
    world_init();
    npc_init();

    ESP_LOGI(TAG, "Initializing WiFi AP...");
    ESP_ERROR_CHECK(wifi_init_ap());

    esp_netif_ip_info_t ip_info;
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    bool ip_valid = (ap_netif != NULL && esp_netif_get_ip_info(ap_netif, &ip_info) == ESP_OK);
    if (ip_valid) {
        ESP_LOGI(TAG, "AP IP address: " IPSTR, IP2STR(&ip_info.ip));
    } else {
        ESP_LOGW(TAG, "Could not get AP IP info");
    }

    ESP_LOGI(TAG, "Initializing DNS server...");
    ESP_ERROR_CHECK(dns_server_init());

    ESP_LOGI(TAG, "Initializing HTTP server...");
    ESP_ERROR_CHECK(http_server_init());

    printf("\r\n");
    printf("---\r\n");
    printf("Little Red Raccoon MUD\r\n");
    printf("SSID: %s\r\n", WIFI_AP_SSID);
    printf("Password: %s\r\n", WIFI_AP_PASSWORD);
    printf("Captive Portal: Enabled\r\n");
    printf("DNS Redirect: Enabled\r\n");
    printf("HTTP Server: Running\r\n");
    if (ip_valid) {
        printf("Connect at: http://" IPSTR "/\r\n", IP2STR(&ip_info.ip));
    }
    printf("-----------------\r\n");
    printf("\r\n");
    fflush(stdout);

    xTaskCreate(dns_server_task, "dns_srv", 4096, NULL, 5, NULL);
    xTaskCreate(http_server_task, "http_srv", 12288, NULL, 5, NULL);
    xTaskCreate(random_events_task, "random_evt", 4096, NULL, 3, NULL);

    if (ip_valid) {
        ESP_LOGI(TAG, "MUD server ready. Connect to WiFi and open http://" IPSTR "/", IP2STR(&ip_info.ip));
    } else {
        ESP_LOGI(TAG, "MUD server ready (IP unknown, check AP)");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
