#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include "../../STM8S001J3.h"

#define LED_DEBUG_PIN_NR 3
#define LED_DEBUG_PIN PIN3
#define LED_DEBUG_PORT sfr_PORTA

#define IR_PIN_NR 4
#define IR_PIN PIN4
#define IR_PORT sfr_PORTB

#define RED_PWM_CHAN 1
#define GREEN_PWM_CHAN 2
#define BLUE_PWM_CHAN 3

#define RED_PIN PIN3
#define RED_PORT sfr_PORTA

#define GREEN_PIN PIN5
#define GREEN_PORT sfr_PORTC

#define BLUE_PIN PIN3
#define BLUE_PORT sfr_PORTD

#define MCU_MASTER_CLOCK_HZ 16000000UL
#define SWIM_STARTUP_DELAY_LOOPS 2500000UL
#define LED_INTENSITY_SCALE_MAX 1000U
#define LED_PWM_TIMER_DEFAULT_PRESCALER 4U

/* TIM4 runs the IR decoder timebase at 100 us per tick: 16 MHz / 2^6 / (24 + 1) = 10 kHz. */
#define IR_TIMER_TICK_US 100U
#define IR_TIMER_PRESCALER 6U
#define IR_TIMER_ARR 24U

/* TIM1 overflows every 100 ms: 16 MHz / 16000 / 100 = 100 ms period. */
#define LED_FEATURE_TIMER_PSCRH 0x3e
#define LED_FEATURE_TIMER_PSCRL 0x7f
#define LED_FEATURE_TIMER_ARRH 0x00
#define LED_FEATURE_TIMER_ARRL 0x63

#endif
