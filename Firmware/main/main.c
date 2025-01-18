#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h" // Add this line

#define INTERRUPT_PIN_POS_EDGE 0
#define INTERRUPT_PIN_NEG_EDGE 2
#define SWITCH_LED 1

#define ON_BOARD_LED 8

int number_of_interrupts = 0;

TickType_t pos_edge_time = 0;
int neg_edge_flag = 0;

int switch_led_state = 0;

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
    // Configure the PWM pin
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

    while (1)
    {
        // Main loop can be used for other tasks
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (neg_edge_flag == 1)
        {
            neg_edge_flag = 0;

            TickType_t time_diff = xTaskGetTickCount() - pos_edge_time;

            if (time_diff > 25)
            {
                // Long Press
                switch_led_state = 1;
                ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 8192); // Set duty cycle to 25%
                ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
            }
            else
            {
                // Short Press
                printf("Short Press\n");
                switch_led_state = !switch_led_state;
                if (switch_led_state == 0)
                {
                    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 0); // Set duty cycle to 0%
                    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                }
                else
                {
                    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 2048); // Set duty cycle to 25%
                    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
                }
            }
        }
    }
}