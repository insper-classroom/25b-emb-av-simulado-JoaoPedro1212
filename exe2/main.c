#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

typedef struct {
    uint led_y;
    uint led_b;
    uint btn;
    struct repeating_timer ty;
    struct repeating_timer tb;
    struct repeating_timer talarm;
    bool y_on;
    bool b_on;
    bool running;
} app_t;

static volatile app_t *g_app = NULL;

static bool y_cb(struct repeating_timer *t) {
    app_t *a = (app_t *)t->user_data;
    a->y_on = !a->y_on;
    gpio_put(a->led_y, a->y_on);
    return true;
}

static bool b_cb(struct repeating_timer *t) {
    app_t *a = (app_t *)t->user_data;
    a->b_on = !a->b_on;
    gpio_put(a->led_b, a->b_on);
    return true;
}

static bool alarm_cb(struct repeating_timer *t) {
    app_t *a = (app_t *)t->user_data;
    cancel_repeating_timer(&a->ty);
    cancel_repeating_timer(&a->tb);
    a->y_on = false;
    a->b_on = false;
    gpio_put(a->led_y, 0);
    gpio_put(a->led_b, 0);
    a->running = false;
    return false;
}

static void btn_isr(uint gpio, uint32_t events) {
    app_t *a = (app_t *)g_app;
    if (!a || gpio != a->btn) return;
    if (!a->running) {
        a->running = true;
        add_repeating_timer_ms(500, y_cb, a, &a->ty);
        add_repeating_timer_ms(150, b_cb, a, &a->tb);
        add_repeating_timer_ms(-5000, alarm_cb, a, &a->talarm);
    }
}

int main() {
    stdio_init_all();

    const uint LED_Y = 5;
    const uint LED_B = 9;
    const uint BTN   = 28;

    gpio_init(LED_Y);
    gpio_set_dir(LED_Y, GPIO_OUT);
    gpio_put(LED_Y, 0);

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_B, 0);

    gpio_init(BTN);
    gpio_set_dir(BTN, GPIO_IN);
    gpio_pull_up(BTN);

    app_t app = {
        .led_y = LED_Y,
        .led_b = LED_B,
        .btn = BTN,
        .y_on = false,
        .b_on = false,
        .running = false
    };
    g_app = &app;

    gpio_set_irq_enabled_with_callback(BTN, GPIO_IRQ_EDGE_FALL, true, btn_isr);

    while (true) {
        __asm volatile("wfi");
    }
}