#include "wifi_ap_captive_portal.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>  // Required for strlen(), memset(), sscanf()
#include "esp_http_server.h"  // Required for HTTP server
#include <stdlib.h>  // Required for strtol() in URL decoding
#include "esp_system.h"  // Required for esp_restart()
#include <ctype.h>  // Required for isxdigit()


#define WIFI_SSID "ESP32_AP"
#define WIFI_PASS "12345678"

#define WIFI_NAMESPACE "wifi_store"


// Function to decode URL-encoded strings
static void url_decode(char *decoded, const char *encoded, size_t max_len) {
    size_t i, j;
    char hex[3] = {0}; // Temporary buffer for hex values

    for (i = 0, j = 0; encoded[i] && j < max_len - 1; ++i) {
        if ((encoded[i] == '%') && isxdigit((unsigned char)encoded[i + 1]) && isxdigit((unsigned char)encoded[i + 2])) {
            // Extract two hex digits
            hex[0] = encoded[i + 1];
            hex[1] = encoded[i + 2];
            hex[2] = '\0';

            // Convert hex to ASCII character
            decoded[j++] = (char)strtol(hex, NULL, 16);
            i += 2; // Skip next two characters
        } else if (encoded[i] == '+') {
            decoded[j++] = ' ';  // Convert `+` to space
        } else {
            decoded[j++] = encoded[i];  // Copy normal character
        }
    }
    decoded[j] = '\0';  // Null-terminate the string
}



// Handle incoming Wi-Fi credentials from the web form

static esp_err_t wifi_post_handler(httpd_req_t *req) {
    char content[256];  
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        return ESP_FAIL;
    }
    content[ret] = '\0';

    char raw_ssid[64] = {0}, raw_password[64] = {0};
    char ssid[32] = {0}, password[64] = {0};

    // Extract SSID & password safely
    sscanf(content, "ssid=%63[^&]&password=%63[^\n]", raw_ssid, raw_password);

    // Decode URL-encoded values
    url_decode(ssid, raw_ssid, sizeof(ssid));
    url_decode(password, raw_password, sizeof(password));

    ESP_LOGI("CAPTIVE_PORTAL", "📥 Decoded SSID: %s, Password: %s", ssid, password);

    // Store Wi-Fi credentials in NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("CAPTIVE_PORTAL", "NVS partition truncated. Erasing & reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open NVS storage
    err = nvs_open("wifi_store", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CAPTIVE_PORTAL", "⚠️ Failed to open NVS storage.");
        return ESP_FAIL;
    }

    // Save SSID and Password
    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err != ESP_OK) {
        ESP_LOGE("CAPTIVE_PORTAL", "⚠️ Failed to store SSID.");
    }
    err = nvs_set_str(nvs_handle, "password", password);
    if (err != ESP_OK) {
        ESP_LOGE("CAPTIVE_PORTAL", "⚠️ Failed to store Password.");
    }

    // **Ensure commit before restart**
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("CAPTIVE_PORTAL", "⚠️ Failed to commit NVS data.");
        return ESP_FAIL;
    }

    // Close NVS handle after successful commit
    nvs_close(nvs_handle);

    ESP_LOGI("CAPTIVE_PORTAL", "✅ Wi-Fi credentials stored successfully! Restarting ESP32...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Small delay to allow NVS to save properly
    esp_restart();  // Restart ESP32 to apply new Wi-Fi settings

    return ESP_OK;
}



static esp_err_t wifi_get_handler(httpd_req_t *req)
{
    // const char resp[] =
    //     "<html><body><h2>Enter Wi-Fi Credentials</h2>"
    //     "<form method='POST' action='/wifi'>"
    //     "SSID: <input type='text' name='ssid'><br>"
    //     "Password: <input type='password' name='password'><br>"
    //     "<input type='submit' value='Connect'>"
    //     "</form></body></html>";

    // httpd_resp_send(req, resp, strlen(resp));
    // return ESP_OK;
    
    const char *html_response =
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">"
    "<title>Connect to Wi-Fi</title>"
    "<style>"
    "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background: linear-gradient(135deg, #f8f9fa, #e9ecef); }"
    ".container { width: 90%; max-width: 400px; background: white; padding: 20px; border-radius: 12px; box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.2); text-align: center; box-sizing: border-box; }"
    "h2 { color: #343a40; margin-bottom: 15px; font-size: 22px; }"
    ".logo { width: 150px; margin-bottom: 15px; }"
    "form { display: flex; flex-direction: column; gap: 12px; }"
    "input { width: 100%; padding: 12px; border: 1px solid #ced4da; border-radius: 8px; font-size: 16px; background-color: #f8f9fa; outline: none; box-sizing: border-box; }"
    "button { width: 100%; padding: 12px; background: #ea9f78; color: white; border: none; border-radius: 8px; cursor: pointer; font-size: 18px; transition: background 0.3s ease; }"
    "button:hover { background: #d98b65; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "  <img src=\"data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+CjxzdmcKICAgeG1sbnM6ZGM9Imh0dHA6Ly9wdXJsLm9yZy9kYy9lbGVtZW50cy8xLjEvIgogICB4bWxuczpjYz0iaHR0cDovL2NyZWF0aXZlY29tbW9ucy5vcmcvbnMjIgogICB4bWxuczpyZGY9Imh0dHA6Ly93d3cudzMub3JnLzE5OTkvMDIvMjItcmRmLXN5bnRheC1ucyMiCiAgIHhtbG5zOnN2Zz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciCiAgIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIKICAgeG1sbnM6c29kaXBvZGk9Imh0dHA6Ly9zb2RpcG9kaS5zb3VyY2Vmb3JnZS5uZXQvRFREL3NvZGlwb2RpLTAuZHRkIgogICB4bWxuczppbmtzY2FwZT0iaHR0cDovL3d3dy5pbmtzY2FwZS5vcmcvbmFtZXNwYWNlcy9pbmtzY2FwZSIKICAgd2lkdGg9IjYwaW4iCiAgIGhlaWdodD0iMTVpbiIKICAgdmlld0JveD0iMCAwIDE1MjQgMzgxLjAwMDAxIgogICB2ZXJzaW9uPSIxLjEiCiAgIGlkPSJzdmc4IgogICBpbmtzY2FwZTp2ZXJzaW9uPSIxLjAgKDQwMzVhNGYsIDIwMjAtMDUtMDEpIgogICBzb2RpcG9kaTpkb2NuYW1lPSJCbGFjayBsb2dvIGhvcml6b250YWwuc3ZnIj4KICA8ZGVmcwogICAgIGlkPSJkZWZzMiI+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIGVmZmVjdD0iYnNwbGluZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDExOTgiCiAgICAgICBpc192aXNpYmxlPSJ0cnVlIgogICAgICAgbHBldmVyc2lvbj0iMSIKICAgICAgIHdlaWdodD0iMzMuMzMzMzMzIgogICAgICAgc3RlcHM9IjIiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIGFwcGx5X25vX3dlaWdodD0idHJ1ZSIKICAgICAgIGFwcGx5X3dpdGhfd2VpZ2h0PSJ0cnVlIgogICAgICAgb25seV9zZWxlY3RlZD0iZmFsc2UiIC8+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIG9ubHlfc2VsZWN0ZWQ9ImZhbHNlIgogICAgICAgYXBwbHlfd2l0aF93ZWlnaHQ9InRydWUiCiAgICAgICBhcHBseV9ub193ZWlnaHQ9InRydWUiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIHN0ZXBzPSIyIgogICAgICAgd2VpZ2h0PSIzMy4zMzMzMzMiCiAgICAgICBscGV2ZXJzaW9uPSIxIgogICAgICAgaXNfdmlzaWJsZT0idHJ1ZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDEyMTciCiAgICAgICBlZmZlY3Q9ImJzcGxpbmUiIC8+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIGVmZmVjdD0iYnNwbGluZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDEyMzYiCiAgICAgICBpc192aXNpYmxlPSJ0cnVlIgogICAgICAgbHBldmVyc2lvbj0iMSIKICAgICAgIHdlaWdodD0iMzMuMzMzMzMzIgogICAgICAgc3RlcHM9IjIiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIGFwcGx5X25vX3dlaWdodD0idHJ1ZSIKICAgICAgIGFwcGx5X3dpdGhfd2VpZ2h0PSJ0cnVlIgogICAgICAgb25seV9zZWxlY3RlZD0iZmFsc2UiIC8+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIGVmZmVjdD0iYnNwbGluZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDEyNTUiCiAgICAgICBpc192aXNpYmxlPSJ0cnVlIgogICAgICAgbHBldmVyc2lvbj0iMSIKICAgICAgIHdlaWdodD0iMzMuMzMzMzMzIgogICAgICAgc3RlcHM9IjIiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIGFwcGx5X25vX3dlaWdodD0idHJ1ZSIKICAgICAgIGFwcGx5X3dpdGhfd2VpZ2h0PSJ0cnVlIgogICAgICAgb25seV9zZWxlY3RlZD0iZmFsc2UiIC8+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIG9ubHlfc2VsZWN0ZWQ9ImZhbHNlIgogICAgICAgYXBwbHlfd2l0aF93ZWlnaHQ9InRydWUiCiAgICAgICBhcHBseV9ub193ZWlnaHQ9InRydWUiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIHN0ZXBzPSIyIgogICAgICAgd2VpZ2h0PSIzMy4zMzMzMzMiCiAgICAgICBscGV2ZXJzaW9uPSIxIgogICAgICAgaXNfdmlzaWJsZT0idHJ1ZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDEyNzQiCiAgICAgICBlZmZlY3Q9ImJzcGxpbmUiIC8+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIGVmZmVjdD0iYnNwbGluZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDEyOTMiCiAgICAgICBpc192aXNpYmxlPSJ0cnVlIgogICAgICAgbHBldmVyc2lvbj0iMSIKICAgICAgIHdlaWdodD0iMzMuMzMzMzMzIgogICAgICAgc3RlcHM9IjIiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIGFwcGx5X25vX3dlaWdodD0idHJ1ZSIKICAgICAgIGFwcGx5X3dpdGhfd2VpZ2h0PSJ0cnVlIgogICAgICAgb25seV9zZWxlY3RlZD0iZmFsc2UiIC8+CiAgICA8aW5rc2NhcGU6cGF0aC1lZmZlY3QKICAgICAgIG9ubHlfc2VsZWN0ZWQ9ImZhbHNlIgogICAgICAgYXBwbHlfd2l0aF93ZWlnaHQ9InRydWUiCiAgICAgICBhcHBseV9ub193ZWlnaHQ9InRydWUiCiAgICAgICBoZWxwZXJfc2l6ZT0iMCIKICAgICAgIHN0ZXBzPSIyIgogICAgICAgd2VpZ2h0PSIzMy4zMzMzMzMiCiAgICAgICBscGV2ZXJzaW9uPSIxIgogICAgICAgaXNfdmlzaWJsZT0idHJ1ZSIKICAgICAgIGlkPSJwYXRoLWVmZmVjdDEzMTIiCiAgICAgICBlZmZlY3Q9ImJzcGxpbmUiIC8+CiAgPC9kZWZzPgogIDxzb2RpcG9kaTpuYW1lZHZpZXcKICAgICBpZD0iYmFzZSIKICAgICBwYWdlY29sb3I9IiNmZmZmZmYiCiAgICAgYm9yZGVyY29sb3I9IiM2NjY2NjYiCiAgICAgYm9yZGVyb3BhY2l0eT0iMS4wIgogICAgIGlua3NjYXBlOnBhZ2VvcGFjaXR5PSIwLjAiCiAgICAgaW5rc2NhcGU6cGFnZXNoYWRvdz0iMiIKICAgICBpbmtzY2FwZTp6b29tPSIwLjA5NzEyMDkzMSIKICAgICBpbmtzY2FwZTpjeD0iMzAzNC4wNjM2IgogICAgIGlua3NjYXBlOmN5PSIxMTM1Ljk5NTMiCiAgICAgaW5rc2NhcGU6ZG9jdW1lbnQtdW5pdHM9Im1tIgogICAgIGlua3NjYXBlOmN1cnJlbnQtbGF5ZXI9ImxheWVyMSIKICAgICBpbmtzY2FwZTpkb2N1bWVudC1yb3RhdGlvbj0iMCIKICAgICBzaG93Z3JpZD0iZmFsc2UiCiAgICAgdW5pdHM9ImluIgogICAgIGlua3NjYXBlOndpbmRvdy13aWR0aD0iMTI1MiIKICAgICBpbmtzY2FwZTp3aW5kb3ctaGVpZ2h0PSI2ODciCiAgICAgaW5rc2NhcGU6d2luZG93LXg9IjAiCiAgICAgaW5rc2NhcGU6d2luZG93LXk9IjIzIgogICAgIGlua3NjYXBlOndpbmRvdy1tYXhpbWl6ZWQ9IjAiIC8+CiAgPG1ldGFkYXRhCiAgICAgaWQ9Im1ldGFkYXRhNSI+CiAgICA8cmRmOlJERj4KICAgICAgPGNjOldvcmsKICAgICAgICAgcmRmOmFib3V0PSIiPgogICAgICAgIDxkYzpmb3JtYXQ+aW1hZ2Uvc3ZnK3htbDwvZGM6Zm9ybWF0PgogICAgICAgIDxkYzp0eXBlCiAgICAgICAgICAgcmRmOnJlc291cmNlPSJodHRwOi8vcHVybC5vcmcvZGMvZGNtaXR5cGUvU3RpbGxJbWFnZSIgLz4KICAgICAgICA8ZGM6dGl0bGUgLz4KICAgICAgPC9jYzpXb3JrPgogICAgPC9yZGY6UkRGPgogIDwvbWV0YWRhdGE+CiAgPGcKICAgICBpbmtzY2FwZTpsYWJlbD0iTGF5ZXIgMSIKICAgICBpbmtzY2FwZTpncm91cG1vZGU9ImxheWVyIgogICAgIGlkPSJsYXllcjEiPgogICAgPGcKICAgICAgIGlua3NjYXBlOmV4cG9ydC15ZHBpPSIzMDAiCiAgICAgICBpbmtzY2FwZTpleHBvcnQteGRwaT0iMzAwIgogICAgICAgaWQ9Imc5MjYiPgogICAgICA8cGF0aAogICAgICAgICBpZD0icGF0aDk1Ni04IgogICAgICAgICBzdHlsZT0iZmlsbDojZWE5Zjc4O2ZpbGwtb3BhY2l0eTowO2ZpbGwtcnVsZTpldmVub2RkO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoyMC4wMDAxO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjE7cGFpbnQtb3JkZXI6c3Ryb2tlIG1hcmtlcnMgZmlsbCIKICAgICAgICAgZD0iTSA0NC4zOTY1OTQsMTU3LjYxNzE2IEEgMTYxLjYxODk4LDE2Mi40ODIwOSA0NSAwIDAgNDYuMDgwOTg0LDIzMi4xOTg3IDE2MS42MTg5OCwxNjIuNDgyMDkgNDUgMCAwIDIzNS43NzgwNiwzNDguOTk4NTkgMTYxLjYxODk4LDE2Mi40ODIwOSA0NSAwIDAgMjM0LjA4OTY3LDI3NC40MTcgMTYxLjYxODk4LDE2Mi40ODIwOSA0NSAwIDAgNDQuMzk2NTk0LDE1Ny42MTcxNiBaIiAvPgogICAgICA8Y2lyY2xlCiAgICAgICAgIHI9IjE2Mi40NzkiCiAgICAgICAgIGN5PSIyMzUuOTAyNTkiCiAgICAgICAgIGN4PSIxNDcuNjcxMTMiCiAgICAgICAgIGlkPSJwYXRoOTU2LTgtMC0xIgogICAgICAgICBzdHlsZT0iZmlsbDojZWE5Zjc4O2ZpbGwtb3BhY2l0eTowO2ZpbGwtcnVsZTpldmVub2RkO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoyNTtzdHJva2UtbWl0ZXJsaW1pdDo0O3N0cm9rZS1kYXNoYXJyYXk6bm9uZTtzdHJva2Utb3BhY2l0eToxO3BhaW50LW9yZGVyOnN0cm9rZSBtYXJrZXJzIGZpbGwiCiAgICAgICAgIHRyYW5zZm9ybT0icm90YXRlKC0xNC43NTg4MzMpIiAvPgogICAgICA8cGF0aAogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiCiAgICAgICAgIGQ9Ik0gNTMuNTc2MTM0LDE2Ni43OTg5NyBDIDExMS4yNTA1MiwyMjQuNDczMzggMTY4LjkyMzg5LDI4Mi4xNDY3NSAyMjYuNTk2MzEsMzM5LjgxOTE5IgogICAgICAgICBpZD0icGF0aDExOTYiCiAgICAgICAgIGlua3NjYXBlOnBhdGgtZWZmZWN0PSIjcGF0aC1lZmZlY3QxMTk4IgogICAgICAgICBpbmtzY2FwZTpvcmlnaW5hbC1kPSJNIDUzLjU3NjEzNCwxNjYuNzk4OTcgQyAxMTEuMjUxNTQsMjI0LjQ3MjM2IDE2OC45MjQ5MywyODIuMTQ1NzEgMjI2LjU5NjMxLDMzOS44MTkxOSIgLz4KICAgICAgPHBhdGgKICAgICAgICAgaW5rc2NhcGU6b3JpZ2luYWwtZD0ibSA1Mi41NDkzNDIsMjE0LjgxNzg2IGMgMTYuMDM2MjkyLC0zZS00IDMyLjA3MjM5MiwtM2UtNCA0OC4xMDgxNDgsMCIKICAgICAgICAgaW5rc2NhcGU6cGF0aC1lZmZlY3Q9IiNwYXRoLWVmZmVjdDEyMTciCiAgICAgICAgIGlkPSJwYXRoMTE5Ni00IgogICAgICAgICBkPSJtIDUyLjU0OTM0MiwyMTQuODE3ODYgYyAxNi4wMzYyOTIsMCAzMi4wNzIzOTIsMCA0OC4xMDgxNDgsMCIKICAgICAgICAgc3R5bGU9ImZpbGw6bm9uZTtzdHJva2U6IzAwMDAwMDtzdHJva2Utd2lkdGg6MTIuNTtzdHJva2UtbGluZWNhcDpidXR0O3N0cm9rZS1saW5lam9pbjptaXRlcjtzdHJva2UtbWl0ZXJsaW1pdDo0O3N0cm9rZS1kYXNoYXJyYXk6bm9uZTtzdHJva2Utb3BhY2l0eToxIiAvPgogICAgICA8cGF0aAogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiCiAgICAgICAgIGQ9Im0gMTAxLjU5NTA0LDE2My43ODA5NyBjIC0yZS01LDE2LjcwMDA0IC00ZS01LDMzLjM5OTk5IC02ZS01LDUwLjA5OTQ0IgogICAgICAgICBpZD0icGF0aDExOTYtNC02IgogICAgICAgICBpbmtzY2FwZTpwYXRoLWVmZmVjdD0iI3BhdGgtZWZmZWN0MTIzNiIKICAgICAgICAgaW5rc2NhcGU6b3JpZ2luYWwtZD0ibSAxMDEuNTk1MDQsMTYzLjc4MDk3IGMgLTMuMWUtNCwxNi43MDAwNCAtMy4xZS00LDMzLjM5OTk5IC02ZS01LDUwLjA5OTQ0IiAvPgogICAgICA8cGF0aAogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiCiAgICAgICAgIGQ9Im0gNjUuMjMxNDM1LDI2MS4yOTQ3IGMgMjguNTQ4NTk2LC0xMGUtNiA1Ny4wOTY5NDUsLTNlLTUgODUuNjQ0Njg1LC00ZS01IgogICAgICAgICBpZD0icGF0aDExOTYtNC04IgogICAgICAgICBpbmtzY2FwZTpwYXRoLWVmZmVjdD0iI3BhdGgtZWZmZWN0MTI1NSIKICAgICAgICAgaW5rc2NhcGU6b3JpZ2luYWwtZD0ibSA2NS4yMzE0MzUsMjYxLjI5NDcgYyAyOC41NDg1OTYsLTVlLTQgNTcuMDk2OTQ1LC01ZS00IDg1LjY0NDY4NSwtNGUtNSIgLz4KICAgICAgPHBhdGgKICAgICAgICAgaW5rc2NhcGU6b3JpZ2luYWwtZD0ibSAxNDguMDcxODgsMTc3LjIwOTc3IGMgLTUuNWUtNCwyOC45NjM0NCAtNS41ZS00LDU3LjkyNjY1IDAsODYuODg5MTMiCiAgICAgICAgIGlua3NjYXBlOnBhdGgtZWZmZWN0PSIjcGF0aC1lZmZlY3QxMjc0IgogICAgICAgICBpZD0icGF0aDExOTYtNC04LTYiCiAgICAgICAgIGQ9Im0gMTQ4LjA3MTg4LDE3Ny4yMDk3NyBjIDAsMjguOTYzNDQgMCw1Ny45MjY2NSAwLDg2Ljg4OTEzIgogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiIC8+CiAgICAgIDxwYXRoCiAgICAgICAgIGlua3NjYXBlOm9yaWdpbmFsLWQ9Im0gMTA2LjQyOTY3LDMxNi43OTE4OSBjIDMyLjA2Nzc1LC01LjllLTQgNjQuMTM1MTUsLTUuOWUtNCA5Ni4yMDE4OSwwIgogICAgICAgICBpbmtzY2FwZTpwYXRoLWVmZmVjdD0iI3BhdGgtZWZmZWN0MTI5MyIKICAgICAgICAgaWQ9InBhdGgxMTk2LTQtNyIKICAgICAgICAgZD0ibSAxMDYuNDI5NjcsMzE2Ljc5MTg5IGMgMzIuMDY3NzUsMCA2NC4xMzUxNSwwIDk2LjIwMTg5LDAiCiAgICAgICAgIHN0eWxlPSJmaWxsOm5vbmU7c3Ryb2tlOiMwMDAwMDA7c3Ryb2tlLXdpZHRoOjEyLjU7c3Ryb2tlLWxpbmVjYXA6YnV0dDtzdHJva2UtbGluZWpvaW46bWl0ZXI7c3Ryb2tlLW1pdGVybGltaXQ6NDtzdHJva2UtZGFzaGFycmF5Om5vbmU7c3Ryb2tlLW9wYWNpdHk6MSIgLz4KICAgICAgPHBhdGgKICAgICAgICAgc3R5bGU9ImZpbGw6bm9uZTtzdHJva2U6IzAwMDAwMDtzdHJva2Utd2lkdGg6MTIuNTtzdHJva2UtbGluZWNhcDpidXR0O3N0cm9rZS1saW5lam9pbjptaXRlcjtzdHJva2UtbWl0ZXJsaW1pdDo0O3N0cm9rZS1kYXNoYXJyYXk6bm9uZTtzdHJva2Utb3BhY2l0eToxIgogICAgICAgICBkPSJtIDIwMy41NjkwNywyMTkuNjUyNSBjIDAsMzIuMDY3NzUgMCw2NC4xMzUxOSAwLDk2LjIwMTg5IgogICAgICAgICBpZD0icGF0aDExOTYtNC03LTEiCiAgICAgICAgIGlua3NjYXBlOnBhdGgtZWZmZWN0PSIjcGF0aC1lZmZlY3QxMzEyIgogICAgICAgICBpbmtzY2FwZTpvcmlnaW5hbC1kPSJtIDIwMy41NjkwNywyMTkuNjUyNSBjIC02ZS00LDMyLjA2Nzc1IC02ZS00LDY0LjEzNTE5IDAsOTYuMjAxODkiIC8+CiAgICAgIDxwYXRoCiAgICAgICAgIGlkPSJwYXRoMTQzOSIKICAgICAgICAgZD0ibSAxNzkuODQ5MDEsMTkwLjM4OTQxIHYgLTg2LjE0MzE0IGwgNjUuOTMwNCwtMTcuNjY1OTk2IgogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxNy41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiIC8+CiAgICAgIDxwYXRoCiAgICAgICAgIHNvZGlwb2RpOm5vZGV0eXBlcz0iY2NjIgogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxNy41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiCiAgICAgICAgIGQ9Ik0gMzA3LjEwODA2LDMxMC4zNTgxNCBWIDEwNC4yNDYyNyBMIDI0MS4xNzc2Miw4Ni41ODAyNzQiCiAgICAgICAgIGlkPSJwYXRoMTQzOS04IiAvPgogICAgICA8cGF0aAogICAgICAgICBpZD0icGF0aDE0NjAiCiAgICAgICAgIGQ9Im0gMjA5LjY2MzkyLDE0Ny4xODg0NyB2IDI5LjIxNDk0IgogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiIC8+CiAgICAgIDxwYXRoCiAgICAgICAgIHN0eWxlPSJmaWxsOm5vbmU7c3Ryb2tlOiMwMDAwMDA7c3Ryb2tlLXdpZHRoOjEyLjU7c3Ryb2tlLWxpbmVjYXA6YnV0dDtzdHJva2UtbGluZWpvaW46bWl0ZXI7c3Ryb2tlLW1pdGVybGltaXQ6NDtzdHJva2UtZGFzaGFycmF5Om5vbmU7c3Ryb2tlLW9wYWNpdHk6MSIKICAgICAgICAgZD0ibSAyNDMuNDc4NTIsMTQ3LjE4ODQ3IHYgMjkuMjE0OTQiCiAgICAgICAgIGlkPSJwYXRoMTQ2MC0zIiAvPgogICAgICA8cGF0aAogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiCiAgICAgICAgIGQ9Im0gMjc3LjI5MzExLDE0Ny4xODg0NyB2IDI5LjIxNDk0IgogICAgICAgICBpZD0icGF0aDE0NjAtOCIgLz4KICAgICAgPHBhdGgKICAgICAgICAgaWQ9InBhdGgxNDYwLTMtMCIKICAgICAgICAgZD0ibSAyNDMuNDc4NTcsMjA5Ljk5MTA2IC01ZS01LDI5LjIxNDg5IgogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiIC8+CiAgICAgIDxwYXRoCiAgICAgICAgIGlkPSJwYXRoMTQ2MC04LTQiCiAgICAgICAgIGQ9Im0gMjc3LjI5MzExLDIwOS45OTEwMSB2IDI5LjIxNDk0IgogICAgICAgICBzdHlsZT0iZmlsbDpub25lO3N0cm9rZTojMDAwMDAwO3N0cm9rZS13aWR0aDoxMi41O3N0cm9rZS1saW5lY2FwOmJ1dHQ7c3Ryb2tlLWxpbmVqb2luOm1pdGVyO3N0cm9rZS1taXRlcmxpbWl0OjQ7c3Ryb2tlLWRhc2hhcnJheTpub25lO3N0cm9rZS1vcGFjaXR5OjEiIC8+CiAgICAgIDxwYXRoCiAgICAgICAgIGlkPSJwYXRoMTQ2MC04LTQ5IgogICAgICAgICBkPSJtIDI3Ny4yOTMxMSwyNzIuNzkzNiB2IDI5LjIxNDkiCiAgICAgICAgIHN0eWxlPSJmaWxsOm5vbmU7c3Ryb2tlOiMwMDAwMDA7c3Ryb2tlLXdpZHRoOjEyLjU7c3Ryb2tlLWxpbmVjYXA6YnV0dDtzdHJva2UtbGluZWpvaW46bWl0ZXI7c3Ryb2tlLW1pdGVybGltaXQ6NDtzdHJva2UtZGFzaGFycmF5Om5vbmU7c3Ryb2tlLW9wYWNpdHk6MSIgLz4KICAgICAgPHBhdGgKICAgICAgICAgaWQ9InBhdGgxMDg5IgogICAgICAgICBkPSJtIDMwOS4wMTE1MSwyNDIuNTgyOTEgaCA1Ni4xODI3NCIKICAgICAgICAgc3R5bGU9ImZpbGw6bm9uZTtzdHJva2U6IzAwMDAwMDtzdHJva2Utd2lkdGg6MTU7c3Ryb2tlLWxpbmVjYXA6YnV0dDtzdHJva2UtbGluZWpvaW46bWl0ZXI7c3Ryb2tlLW1pdGVybGltaXQ6NDtzdHJva2UtZGFzaGFycmF5Om5vbmU7c3Ryb2tlLW9wYWNpdHk6MSIgLz4KICAgICAgPHRleHQKICAgICAgICAgeG1sOnNwYWNlPSJwcmVzZXJ2ZSIKICAgICAgICAgc3R5bGU9ImZvbnQtd2VpZ2h0OmJvbGQ7Zm9udC1zaXplOjE4My40NDM5OTk5OTk5OTk5ODg0MHB4O2xpbmUtaGVpZ2h0OjIuMTU7Zm9udC1mYW1pbHk6TW9udHNlcnJhdDstaW5rc2NhcGUtZm9udC1zcGVjaWZpY2F0aW9uOidNb250c2VycmF0IEJvbGQnO2ZpbGw6IzAwMDAwMDtmaWxsLW9wYWNpdHk6MTtzdHJva2Utd2lkdGg6MC4yNjQ1ODM7IgogICAgICAgICB4PSI0NjUuODM0MzUiCiAgICAgICAgIHk9IjI2MS41ODQ1NiIKICAgICAgICAgaWQ9InRleHQ0MyI+PHRzcGFuCiAgICAgICAgICAgc29kaXBvZGk6cm9sZT0ibGluZSIKICAgICAgICAgICBpZD0idHNwYW40MSIKICAgICAgICAgICB4PSI0NjUuODM0MzUiCiAgICAgICAgICAgeT0iMjYxLjU4NDU2IgogICAgICAgICAgIHN0eWxlPSJmb250LXNpemU6MTgzLjQ0Mzk5OTk5OTk5OTk4ODQwcHg7ZmlsbDojMDAwMDAwO2ZpbGwtb3BhY2l0eToxO3N0cm9rZS13aWR0aDowLjI2NDU4MzsiPldlQ29FeGlzdDwvdHNwYW4+PC90ZXh0PgogICAgPC9nPgogIDwvZz4KPC9zdmc+Cg==\" alt=\"Logo\" class=\"logo\">" /* Replace with Base64 */
    "  <h2>Connect to Wi-Fi</h2>"
    "  <form action=\"/wifi\" method=\"post\">"
    "    <input type=\"text\" name=\"ssid\" placeholder=\"Enter Wi-Fi Name/SSID\" required><br>"
    "    <input type=\"password\" name=\"password\" placeholder=\"Enter Password\" required><br>"
    "    <button type=\"submit\">Connect</button>"
    "  </form>"
    "</div>"
    "</body>"
    "</html>";

httpd_resp_set_type(req, "text/html");
return httpd_resp_send(req, html_response, HTTPD_RESP_USE_STRLEN);
    
}

void start_captive_portal()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t wifi_get_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = wifi_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &wifi_get_uri);

        httpd_uri_t wifi_post_uri = {
            .uri = "/wifi",
            .method = HTTP_POST,
            .handler = wifi_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &wifi_post_uri);

        ESP_LOGI("CAPTIVE_PORTAL", "Captive portal started!");
    }
}

void wifi_init_softap(void) {
    static bool wifi_initialized = false;  // Prevent multiple calls

    if (wifi_initialized) {
        ESP_LOGW("wifi_ap", "Wi-Fi AP already initialized. Skipping...");
        return;
    }

    ESP_LOGI("wifi_ap", "⚡ Starting Wi-Fi AP Mode...");

    // ✅ Ensure NVS is initialized before using Wi-Fi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("wifi_ap", "NVS partition was truncated. Erasing and reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ✅ Ensure ESP-NETIF is initialized
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // ✅ Create default Wi-Fi AP **before** initializing Wi-Fi
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .password = WIFI_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("wifi_ap", "✅ Wi-Fi AP started. SSID: %s", WIFI_SSID);

    wifi_initialized = true;  // ✅ Prevents multiple AP initializations

    start_captive_portal();  // ✅ Start Captive Portal
}

esp_err_t connect_to_stored_wifi(void) { 
    ESP_LOGI("WIFI", "📡 Connecting to stored Wi-Fi...");

    // **Step 1: Ensure NVS is initialized**
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW("WIFI", "⚠️ NVS partition is truncated. Erasing and reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);  

    // **Step 2: Initialize the TCP/IP stack BEFORE Wi-Fi**
    ESP_LOGI("WIFI", "🌐 Initializing TCP/IP stack...");
    esp_netif_init();  // **FIX**
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // **Step 3: Create Wi-Fi station interface**
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE("WIFI", "❌ Failed to create Wi-Fi station interface.");
        return ESP_FAIL;
    }

    // **Step 4: Initialize Wi-Fi**
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));  // **Ensure Wi-Fi is initialized AFTER TCP/IP**

    // **Step 5: Load stored Wi-Fi credentials**
    wifi_config_t wifi_config = {0};
    nvs_handle_t nvs_handle;
    err = nvs_open("wifi_store", NVS_READONLY, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGW("WIFI", "⚠️ No stored Wi-Fi credentials found.");
        return ESP_FAIL;
    }

    size_t ssid_len = sizeof(wifi_config.sta.ssid);
    size_t pass_len = sizeof(wifi_config.sta.password);

    // **Read SSID**
    err = nvs_get_str(nvs_handle, "ssid", (char *)wifi_config.sta.ssid, &ssid_len);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "❌ Failed to read SSID from NVS.");
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }

    // **Read Password**
    err = nvs_get_str(nvs_handle, "password", (char *)wifi_config.sta.password, &pass_len);
    if (err != ESP_OK) {
        ESP_LOGE("WIFI", "❌ Failed to read password from NVS.");
        nvs_close(nvs_handle);
        return ESP_FAIL;
    }

    nvs_close(nvs_handle);

    ESP_LOGI("WIFI", "📡 Stored SSID: %s", wifi_config.sta.ssid);

    // **Step 6: Set Wi-Fi mode and configuration**
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // **Step 7: Start Wi-Fi and Connect**
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI("WIFI", "✅ Wi-Fi connection initiated.");
    
    return ESP_OK;
}
