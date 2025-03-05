# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

set(COMPONENT_INCLUDE_DIRS
    .
    ../config
)

idf_component_register(SRCS "azure_iot_freertos_esp32_main.c"
                            "wifi_ap_captive_portal.c"
                    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
                    REQUIRES esp_event esp_wifi freertos nvs_flash esp_http_server coreMQTT azure-sdk-for-c azure-iot-middleware-freertos sample-azure-iot)
