#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h" // Add this line
#include "cJSON.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"

#define WIFI_SSID "WalterRothlin_2"
#define WIFI_PASS "waltiClaudia007"
#define URL "http://homepi.local:8099/"

#define RESPONSE_BUFFER_SIZE 128

#define INTERRUPT_PIN_POS_EDGE 0
#define INTERRUPT_PIN_NEG_EDGE 1
#define SWITCH_LED 2

#define ON_BOARD_LED 8

enum push_type
{
    SHORT_PRESS,
    LONG_PRESS,
    NO_PRESS
};

int number_of_interrupts = 0;

TickType_t pos_edge_time = 0;
int neg_edge_flag = 0;

int switch_led_state = 0;
int on_board_led_state = 0;

const int board_id = 1;

int led_brightness[4] = {0, 0, 0, 0};

void set_led_brightness(int brightness, ledc_channel_config_t ledc_channel)
{
    printf("Setting brightness to %d\n", brightness);
    int duty_cyle = 8192 * brightness / 100;
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duty_cyle);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

esp_err_t response_evnet_handler(esp_http_client_event_t *evt)
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

void make_rest_request(enum push_type press_type)
{
    char *press_type_str;
    switch (press_type)
    {
    case SHORT_PRESS:
        press_type_str = "SHORT_PRESS";
        break;
    case LONG_PRESS:
        press_type_str = "LONG_PRESS";
        break;
    default:
        press_type_str = "NO_PRESS";
        break;
    }

    // Make a get request to this URL http://homepi.local:8099/button_pressed_event?id=1&PressType=LongPress&IP=99
    char url[100];
    sprintf(url, "http://homepi.local:8099/button_pressed_event?id=%d&PressType=%s&IP=99", board_id, press_type_str);

    char local_response_buffer[200];

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .event_handler = response_evnet_handler,
        .user_data = local_response_buffer,

    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int code = esp_http_client_get_status_code(client);
        printf("Status Code: %d\n", code);
    }
    else
    {
        printf("Error: %s\n", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void IRAM_ATTR gpio_isr_handler(void *arg)
{
    if ((uint32_t)arg == INTERRUPT_PIN_POS_EDGE)
    {
        pos_edge_time = xTaskGetTickCount();
    }
    else if ((uint32_t)arg == INTERRUPT_PIN_NEG_EDGE)
    {
        neg_edge_flag = 1;
    }

    number_of_interrupts++;
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
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

void app_main(void)
{
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize WiFi
    wifi_init_sta();

    // Wait until WiFi is connected
    printf("Connecting to WiFi with SSID: %s\n", WIFI_SSID);

    // Configure the interrupt pin
    gpio_config_t io_conf_pos;
    io_conf_pos.intr_type = GPIO_INTR_POSEDGE;
    io_conf_pos.mode = GPIO_MODE_INPUT;
    io_conf_pos.pin_bit_mask = (1ULL << INTERRUPT_PIN_POS_EDGE);
    io_conf_pos.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_pos.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf_pos);

    // Install the ISR service
    gpio_install_isr_service(0);
    gpio_isr_handler_add((gpio_num_t)INTERRUPT_PIN_POS_EDGE, gpio_isr_handler, (void *)INTERRUPT_PIN_POS_EDGE);

    // Configure the interrupt pin
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << INTERRUPT_PIN_NEG_EDGE);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Install the ISR service
    gpio_isr_handler_add((gpio_num_t)INTERRUPT_PIN_NEG_EDGE, gpio_isr_handler, (void *)INTERRUPT_PIN_NEG_EDGE);

    // Configure the Switch pin
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000, // Frequency in Hertz
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = SWITCH_LED,
        .duty = 4096, // Duty cycle, 50% (2^13 / 2)
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);

    // Configure the On-Board LED
    gpio_config_t io_conf_led;
    io_conf_led.intr_type = GPIO_INTR_DISABLE;
    io_conf_led.mode = GPIO_MODE_OUTPUT;
    io_conf_led.pin_bit_mask = (1ULL << ON_BOARD_LED);
    io_conf_led.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_led.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf_led);

    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (esp_wifi_connect() == ESP_OK)
        {
            break;
        }
        printf("Connecting to WiFi...\n");

        on_board_led_state = !on_board_led_state;
        gpio_set_level(ON_BOARD_LED, on_board_led_state);
    }

    // Print the IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    ESP_LOGI("", "IP Address: " IPSTR, IP2STR(&ip_info.ip));
    gpio_set_level(ON_BOARD_LED, 0);

    printf("Connected to WiFi\n");

    set_led_brightness(0, ledc_channel);

    while (1)
    {
        // Main loop can be used for other tasks
        enum push_type button_pressed = NO_PRESS;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (neg_edge_flag == 1)
        {
            neg_edge_flag = 0;

            TickType_t time_diff = xTaskGetTickCount() - pos_edge_time;

            if (time_diff > 25)
            {
                // Long Press
                printf("Long Press\n");
                button_pressed = LONG_PRESS;
            }
            else
            {
                // Short Press
                printf("Short Press\n");
                button_pressed = SHORT_PRESS;
            }

            make_rest_request(button_pressed);
            set_led_brightness(led_brightness[0], ledc_channel);
        }
    }
}