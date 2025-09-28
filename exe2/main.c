#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

static const int LED_Y = 5;
static const int LED_B = 9;
static const int BTN   = 28;

static volatile bool g_btn_event = false;

typedef struct {
    int led_y;
    int led_b;
    int btn;
    bool y_on;
    bool b_on;
    bool running;
    struct repeating_timer ty;
    struct repeating_timer tb;
} app_t;

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
    a->running = false;
    gpio_put(a->led_y, 0);
    gpio_put(a->led_b, 0);
    gpio_set_irq_enabled(a->btn, GPIO_IRQ_EDGE_FALL, true);
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

    app_t app = {.led_y = LED_Y, .led_b = LED_B, .btn = BTN, .y_on = false, .b_on = false, .running = false};
    struct repeating_timer talarm;

    while (true) {
        if (g_btn_event && !app.running) {
            add_repeating_timer_ms(500, y_cb, &app, &app.ty);
            add_repeating_timer_ms(150, b_cb, &app, &app.tb);
            add_repeating_timer_ms(-5000, alarm_cb, &app, &talarm);
            g_btn_event = false;
            app.running = true;
        }
    }
}