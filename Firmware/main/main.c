#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "cJSON.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_client.h"

#include "support.h"


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

// Board Configuration
const int board_id = 1;
const int diff_long_short_press_tick = 25;

int number_of_interrupts = 0;

TickType_t pos_edge_time = 0;
int neg_edge_flag = 0;

int switch_led_state = 0;
int on_board_led_state = 0;
static int led_brightness[4] = {0, 0, 0, 0};

void set_led_brightness(int brightness, ledc_channel_config_t ledc_channel)
{
    // Set the brightness of the LED
    // brightness should be between 0 and 100
    if (brightness < 0 || brightness > 100)
    {
        printf("Invalid brightness value: %d\n", brightness);
        return;
    }
    printf("Setting brightness to %d\n", brightness);
    // Convert brightness percentage to duty cycle
    int duty_cyle = 8192 * brightness / 100;
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, duty_cyle);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
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
        .event_handler = response_event_handler,
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
        // Main loop
        enum push_type button_pressed = NO_PRESS;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (neg_edge_flag == 1)
        {
            neg_edge_flag = 0;

            TickType_t time_diff = xTaskGetTickCount() - pos_edge_time;

            if (time_diff > diff_long_short_press_tick)
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