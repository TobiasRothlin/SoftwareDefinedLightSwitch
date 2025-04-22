#include "support.h"
#include <string.h>
#include <stdio.h>
#include <cJSON.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>

#define WIFI_SSID "****"
#define WIFI_PASS "****"

#define RESPONSE_BUFFER_SIZE 128




esp_err_t response_event_handler(esp_http_client_event_t *evt)
{
    static char response_buffer[RESPONSE_BUFFER_SIZE] = {0};
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        printf("HTTP_EVENT_ERROR\n");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        printf("HTTP_EVENT_ON_CONNECTED\n");
        break;
    case HTTP_EVENT_HEADER_SENT:
        printf("HTTP_EVENT_HEADER_SENT\n");
        break;
    case HTTP_EVENT_ON_HEADER:
        printf("HTTP_EVENT_ON_HEADER\n");
        printf("%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA\n");

        // Ensure we don't overflow the buffer
        if (evt->data_len + strlen(response_buffer) < RESPONSE_BUFFER_SIZE)
        {
            strncat(response_buffer, (char *)evt->data, evt->data_len);
        }
        else
        {
            printf("Response buffer overflow\n");
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        printf("HTTP_EVENT_ON_FINISH\n");

        // Print the response buffer
        printf("------------------\n");
        printf("%s\n", response_buffer);
        printf("------------------\n");

        // Parse JSON data
        cJSON *json = cJSON_Parse(response_buffer);
        if (json == NULL)
        {
            printf("Failed to parse JSON\n");
            break;
        }

        cJSON *led_state = cJSON_GetObjectItem(json, "LEDState");
        if (led_state != NULL && cJSON_IsArray(led_state))
        {
            cJSON *led_state_item = cJSON_GetArrayItem(led_state, 0);
            if (led_state_item != NULL)
            {
                cJSON *led_id = cJSON_GetObjectItem(led_state_item, "Id");
                cJSON *brightness = cJSON_GetObjectItem(led_state_item, "Brightness");

                if (led_id != NULL && brightness != NULL)
                {
                    printf("LED Id: %d\n", led_id->valueint);
                    printf("Brightness: %d\n", brightness->valueint);
                    led_brightness[led_id->valueint] = brightness->valueint;
                }
            }
        }

        cJSON_Delete(json);

        // Clear the response buffer for the next request
        memset(response_buffer, 0, RESPONSE_BUFFER_SIZE);
        break;
    case HTTP_EVENT_DISCONNECTED:
        printf("HTTP_EVENT_DISCONNECTED\n");
        break;
    case HTTP_EVENT_REDIRECT:
        printf("HTTP_EVENT_REDIRECT\n");
        break;
    default:
        break;
    }
    return ESP_OK;
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("WiFi", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}