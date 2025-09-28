#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

static const int LED_Y = 5;
static const int LED_B = 9;
static const int BTN   = 28;

static volatile bool g_btn_event = false;

static bool y_cb(struct repeating_timer *t) {
    volatile bool *stop = (volatile bool *)t->user_data;
    static bool on = false;
    if (*stop) { gpio_put(LED_Y, 0); on = false; return false; }
    on = !on;
    gpio_put(LED_Y, on);
    return true;
}

static bool b_cb(struct repeating_timer *t) {
    volatile bool *stop = (volatile bool *)t->user_data;
    static bool on = false;
    if (*stop) { gpio_put(LED_B, 0); on = false; return false; }
    on = !on;
    gpio_put(LED_B, on);
    return true;
}

static bool alarm_cb(struct repeating_timer *t) {
    volatile bool *stop = (volatile bool *)t->user_data;
    *stop = true;
    gpio_put(LED_Y, 0);
    gpio_put(LED_B, 0);
    gpio_set_irq_enabled(BTN, GPIO_IRQ_EDGE_FALL, true);
    return false;
}

static void btn_isr(uint gpio, uint32_t events) {
    if (gpio == (uint)BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        g_btn_event = true;
        gpio_set_irq_enabled(BTN, GPIO_IRQ_EDGE_FALL, false);
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_Y);
    gpio_set_dir(LED_Y, GPIO_OUT);
    gpio_put(LED_Y, 0);

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_B, 0);

    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN);
    gpio_set_irq_enabled_with_callback(BTN, GPIO_IRQ_EDGE_FALL, true, btn_isr);

    struct repeating_timer ty, tb, talarm;
    volatile bool stop = false;
    bool running = false;

    while (true) {
        if (g_btn_event && !running) {
            stop = false;
            running = true;
            add_repeating_timer_ms(500, y_cb, (void *)&stop, &ty);
            add_repeating_timer_ms(150, b_cb, (void *)&stop, &tb);
            add_repeating_timer_ms(-5000, alarm_cb, (void *)&stop, &talarm);
            g_btn_event = false;
        }
        __asm volatile("wfi");
        if (stop && running) {
            running = false;
        }
    }
}