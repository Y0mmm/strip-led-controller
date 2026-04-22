#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include <stdint.h>

#include "../app/led_state.h"

void peripherals_wait_for_swim(void);
void peripherals_init_ports(void);
void peripherals_init_gpio_interrupt(void);
void peripherals_init_ir_timer(void);
void peripherals_init_leds_timer(void);
void peripherals_init_led_feature_timer(void);

void peripherals_led_feature_timer_start(void);
void peripherals_led_feature_timer_stop(void);

void peripherals_leds_pwm_set_frequency(uint32_t cent_hz);
void peripherals_leds_set_duty_cycle(uint8_t channel, uint16_t deci_percent);
void peripherals_leds_set_state(const LEDState *led_state);
void peripherals_leds_start(const LEDState *led_state);
void peripherals_leds_stop(void);

uint8_t peripherals_ir_read_pin_state(void);
void peripherals_ir_timer_start(void);
void peripherals_ir_timer_stop(void);
void peripherals_ir_timer_reset(void);

#endif
