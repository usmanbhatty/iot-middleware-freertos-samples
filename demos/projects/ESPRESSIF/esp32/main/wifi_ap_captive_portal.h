#ifndef WIFI_AP_CAPTIVE_PORTAL_H
#define WIFI_AP_CAPTIVE_PORTAL_H

#include "esp_err.h"

/**
 * @brief Initializes ESP32 in SoftAP mode for Wi-Fi credential gathering.
 */
void wifi_init_softap(void);

/**
 * @brief Attempts to connect to stored Wi-Fi credentials.
 * @return ESP_OK if successful, ESP_FAIL otherwise.
 */
esp_err_t connect_to_stored_wifi(void);

/**
 * @brief Starts the Captive Portal Web Server
 */
void start_captive_portal(void);


#endif // WIFI_AP_CAPTIVE_PORTAL_H
