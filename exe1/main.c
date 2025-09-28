#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"

#include <stdint.h>

#define BTN_PIN   22
#define SW_PIN    28
#define NUM_LEDS  5
static const uint LED_PINS[NUM_LEDS] = {2, 3, 4, 5, 6};

#define DEBOUNCE_US_BTN  2000u
#define DEBOUNCE_US_SW   2000u

static volatile int g_count = 0;
static volatile int g_dir = +1;
static volatile uint32_t g_last_btn_ts = 0;
static volatile uint32_t g_last_sw_ts  = 0;

static void bar_init(void) {
    for (size_t i = 0; i < NUM_LEDS; i++) {
        gpio_init(LED_PINS[i]);
        gpio_set_dir(LED_PINS[i], GPIO_OUT);
        gpio_put(LED_PINS[i], 0);
    }
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    gpio_init(SW_PIN);
    gpio_set_dir(SW_PIN, GPIO_IN);
    gpio_pull_up(SW_PIN);
}

static void bar_display(int val) {
    if (val < 0) val = 0;
    if (val > NUM_LEDS) val = NUM_LEDS;
    for (size_t i = 0; i < NUM_LEDS; i++) {
        gpio_put(LED_PINS[i], (int)i < val ? 1 : 0);
    }
}

static void gpio_isr(uint gpio, uint32_t events) {
    uint32_t now = (uint32_t)to_us_since_boot(get_absolute_time());

    if (gpio == BTN_PIN) {
        if ((events & GPIO_IRQ_EDGE_FALL) != 0u) {
            if ((uint32_t)(now - g_last_btn_ts) > DEBOUNCE_US_BTN) {
                int next = g_count + g_dir;
                if (next < 0) next = 0;
                else if (next > NUM_LEDS) next = NUM_LEDS;
                g_count = next;
                g_last_btn_ts = now;
            }
        }
    } else if (gpio == SW_PIN) {
        if ((uint32_t)(now - g_last_sw_ts) > DEBOUNCE_US_SW) {
            if ((events & GPIO_IRQ_EDGE_RISE) != 0u)  g_dir = -1;
            if ((events & GPIO_IRQ_EDGE_FALL) != 0u)  g_dir = +1;
            g_last_sw_ts = now;
        }
    }
}

int main() {
    stdio_init_all();

    bar_init();

    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_isr);
    gpio_set_irq_enabled(SW_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    while (true) {
        bar_display(g_count);
        sleep_ms(5);
    }
}