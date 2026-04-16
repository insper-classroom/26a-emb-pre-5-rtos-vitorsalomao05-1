/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>


#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;

QueueHandle_t xQueueBtn;

void btn_callback(uint gpio, uint32_t events) {
    if (events & GPIO_IRQ_EDGE_FALL) {
        xQueueSendFromISR(xQueueBtn, &gpio, 0);
    }
}

void led_r_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);
    gpio_put(LED_PIN_R, 0);

    QueueHandle_t xQueueToggle = (QueueHandle_t)p;
    bool blinking = false;
    bool state = false;
    int sig;

    while (true) {
        TickType_t wait = blinking ? pdMS_TO_TICKS(100) : portMAX_DELAY;
        if (xQueueReceive(xQueueToggle, &sig, wait) == pdTRUE) {
            blinking = !blinking;
            state = false;
            gpio_put(LED_PIN_R, 0);
        } else if (blinking) {
            state = !state;
            gpio_put(LED_PIN_R, state);
        }
    }
}

void led_y_task(void *p) {
    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    gpio_put(LED_PIN_Y, 0);

    QueueHandle_t xQueueToggle = (QueueHandle_t)p;
    bool blinking = false;
    bool state = false;
    int sig;

    while (true) {
        TickType_t wait = blinking ? pdMS_TO_TICKS(100) : portMAX_DELAY;
        if (xQueueReceive(xQueueToggle, &sig, wait) == pdTRUE) {
            blinking = !blinking;
            state = false;
            gpio_put(LED_PIN_Y, 0);
        } else if (blinking) {
            state = !state;
            gpio_put(LED_PIN_Y, state);
        }
    }
}

void btn_task(void* p) {
    QueueHandle_t xQueueToggleR = xQueueCreate(8, sizeof(int));
    QueueHandle_t xQueueToggleY = xQueueCreate(8, sizeof(int));

    xTaskCreate(led_r_task, "LED_R_Task", 256, (void*)xQueueToggleR, 1, NULL);
    xTaskCreate(led_y_task, "LED_Y_Task", 256, (void*)xQueueToggleY, 1, NULL);

    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int gpio;
    int sig = 1;
    while (true) {
        if (xQueueReceive(xQueueBtn, &gpio, portMAX_DELAY) == pdTRUE) {
            if (gpio == BTN_PIN_R) {
                xQueueSend(xQueueToggleR, &sig, 0);
            } else if (gpio == BTN_PIN_Y) {
                xQueueSend(xQueueToggleY, &sig, 0);
            }
        }
    }
}



int main() {
    stdio_init_all();

    xQueueBtn = xQueueCreate(16, sizeof(int));

    xTaskCreate(btn_task, "BTN_Task 1", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}
