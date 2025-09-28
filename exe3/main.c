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

const int LED_PIN_B = 8;
const int LED_PIN_Y = 13;

typedef struct input {
    int num_led1;
    int num_led2;
} input_t;

QueueHandle_t xQueueInput;

static QueueHandle_t xQueueLed1;
static QueueHandle_t xQueueLed2;
static SemaphoreHandle_t xSemaphoreLed2;

static void blink_n(int pin, int n) {
    if (n < 0) n = 0;
    for (int i = 0; i < n; i++) {
        gpio_put(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_put(pin, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void input_task(void* p) {
    input_t test_case;

    test_case.num_led1 = 3;
    test_case.num_led2 = 4;
    xQueueSend(xQueueInput, &test_case, 0);

    test_case.num_led1 = 0;
    test_case.num_led2 = 2;
    xQueueSend(xQueueInput, &test_case, 0);

    while (true) {

    }
}

static void main_task(void *p) {
    gpio_init(LED_PIN_B);
    gpio_set_dir(LED_PIN_B, GPIO_OUT);
    gpio_put(LED_PIN_B, 0);

    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    gpio_put(LED_PIN_Y, 0);

    input_t in;
    for (;;) {
        if (xQueueReceive(xQueueInput, &in, portMAX_DELAY) == pdTRUE) {
            xQueueSend(xQueueLed1, &in.num_led1, portMAX_DELAY);
            xQueueSend(xQueueLed2, &in.num_led2, portMAX_DELAY);
        }
    }
}

static void led_1_task(void *p) {
    int n;
    for (;;) {
        if (xQueueReceive(xQueueLed1, &n, portMAX_DELAY) == pdTRUE) {
            blink_n(LED_PIN_B, n);
            gpio_put(LED_PIN_B, 0);
            xSemaphoreGive(xSemaphoreLed2);
        }
    }
}

static void led_2_task(void *p) {
    int n;
    for (;;) {
        if (xSemaphoreTake(xSemaphoreLed2, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(xQueueLed2, &n, portMAX_DELAY) == pdTRUE) {
                blink_n(LED_PIN_Y, n);
                gpio_put(LED_PIN_Y, 0);
            }
        }
    }
}

int main() {
    stdio_init_all();

    xQueueInput = xQueueCreate(32, sizeof(input_t));
    xTaskCreate(input_task, "Input", 256, NULL, 1, NULL);

    xQueueLed1 = xQueueCreate(16, sizeof(int));
    xQueueLed2 = xQueueCreate(16, sizeof(int));
    xSemaphoreLed2 = xSemaphoreCreateBinary();

    xTaskCreate(main_task, "Main", 512, NULL, 2, NULL);
    xTaskCreate(led_1_task, "LED1", 256, NULL, 2, NULL);
    xTaskCreate(led_2_task, "LED2", 256, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1) {}

    return 0;
}