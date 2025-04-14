#ifndef SUPPORT_H
#define SUPPORT_H

#include <esp_err.h>
#include <esp_http_client.h>
#include <esp_event.h>
#include <esp_wifi.h>

// Function to handle HTTP client events
esp_err_t response_event_handler(esp_http_client_event_t *evt);

// Event handler for system events
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

// Function to initialize Wi-Fi in station mode
void wifi_init_sta(void);

#endif // SUPPORT_H