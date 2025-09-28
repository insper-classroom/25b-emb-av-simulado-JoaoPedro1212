#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#define LED_Y 5
#define LED_B 9
#define BTN   28

static struct repeating_timer t_y;
static struct repeating_timer t_b;
static alarm_id_t stop_alarm = 0;

static volatile bool req_start = false;
static volatile bool blinking = false;
static bool y_state = false;
static bool b_state = false;

static bool y_cb(struct repeating_timer *t) {
    if (!blinking) { gpio_put(LED_Y, 0); return false; }
    y_state = !y_state;
    gpio_put(LED_Y, y_state);
    return true;
}

static bool b_cb(struct repeating_timer *t) {
    if (!blinking) { gpio_put(LED_B, 0); return false; }
    b_state = !b_state;
    gpio_put(LED_B, b_state);
    return true;
}

static int64_t stop_cb(alarm_id_t id, void *user_data) {
    blinking = false;
    cancel_repeating_timer(&t_y);
    cancel_repeating_timer(&t_b);
    y_state = false;
    b_state = false;
    gpio_put(LED_Y, 0);
    gpio_put(LED_B, 0);
    stop_alarm = 0;
    return 0;
}

static void btn_isr(uint gpio, uint32_t events) {
    (void)gpio; (void)events;
    req_start = true;
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

    while (true) {
        if (req_start) {
            req_start = false;

            if (blinking) {
                if (stop_alarm != 0) cancel_alarm(stop_alarm);
                cancel_repeating_timer(&t_y);
                cancel_repeating_timer(&t_b);
            }

            blinking = true;
            y_state = true;
            b_state = true;
            gpio_put(LED_Y, 1);
            gpio_put(LED_B, 1);

            add_repeating_timer_ms(500, y_cb, NULL, &t_y);
            add_repeating_timer_ms(150, b_cb, NULL, &t_b);
            stop_alarm = add_alarm_in_ms(5000, stop_cb, NULL, true);
        }
        tight_loop_contents();
    }
}