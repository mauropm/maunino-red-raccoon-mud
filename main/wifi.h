#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

#define WIFI_AP_SSID     "RACCOON_MUD"
#define WIFI_AP_PASSWORD "trashpanda"
#define WIFI_AP_CHANNEL  6
#define WIFI_MAX_CONN    10

esp_err_t wifi_init_ap(void);

#endif /* WIFI_H */
