#include "peripherals.h"

#include "../config/hw_config.h"

#define scale_value(x, in_max, out_max) (((int32_t) (x) * (int32_t) (out_max)) / (in_max))
#define TIMER_AUTORELOAD_MAX UINT16_MAX

void peripherals_wait_for_swim(void) {
  uint32_t index;

  /*
   * Drive the debug LED pin as a plain GPIO output during startup.
   * DDR=1 makes PA3 an output, CR1=1 selects push-pull mode so the pin is actively
   * driven both high and low, and CR2=1 allows the faster output slope. The LED is
   * then forced high so there is a visible indicator while the firmware is waiting
   * for an optional SWIM debugger attachment window.
   */
  LED_DEBUG_PORT.DDR.byte |= LED_DEBUG_PIN;
  LED_DEBUG_PORT.CR1.byte |= LED_DEBUG_PIN;
  LED_DEBUG_PORT.CR2.byte |= LED_DEBUG_PIN;
  LED_DEBUG_PORT.ODR.byte |= LED_DEBUG_PIN;

  /*
   * This busy loop is intentionally crude: it simply holds the MCU in a harmless state
   * long enough for SWIM to connect after reset. The exact wall-clock time depends on
   * the 16 MHz system clock, but with the current loop count it is roughly a couple of
   * seconds.
   */
  for (index = 0; index < SWIM_STARTUP_DELAY_LOOPS; index++) {
    NOP();
  }

  LED_DEBUG_PORT.ODR.byte &= (uint8_t) (0xff ^ LED_DEBUG_PIN);
}

void peripherals_init_ports(void) {
  /*
   * IR input on PB4:
   * - DDR bit cleared: configure the pin as input.
   * - CR1 bit cleared: floating input, because the IR receiver already drives the line
   *   and we do not want an internal pull-up to distort idle/high timing.
   * - CR2 bit set: enable the external interrupt path on this pin so every edge can
   *   wake the decoder state machine immediately.
   */
  IR_PORT.DDR.byte &= (uint8_t) (0xff ^ IR_PIN);
  IR_PORT.CR1.byte &= (uint8_t) (0xff ^ IR_PIN);
  IR_PORT.CR2.byte |= IR_PIN;

  /*
   * LED PWM outputs:
   * each channel pin is switched to output mode and configured as push-pull with the
   * fast output stage enabled. These pins are later handed over to TIM2 PWM output
   * compare logic, but the port direction and electrical mode still need to be set here.
   *
   * PA3 -> TIM2_CH3 -> red
   * PC5 -> TIM2_CH1 -> green
   * PD3 -> TIM2_CH2 -> blue
   */
  RED_PORT.DDR.DDR3 = 1;
  RED_PORT.CR1.C13 = 1;
  RED_PORT.CR2.C23 = 1;

  GREEN_PORT.DDR.DDR5 = 1;
  GREEN_PORT.CR1.C15 = 1;
  GREEN_PORT.CR2.C25 = 1;

  BLUE_PORT.DDR.DDR3 = 1;
  BLUE_PORT.CR1.C13 = 1;
  BLUE_PORT.CR2.C23 = 1;
}

void peripherals_init_gpio_interrupt(void) {
  /*
   * PBIS=3 selects interrupt sensitivity on both rising and falling edges for port B.
   * The NEC receiver output toggles on every mark/space transition, so watching both
   * edges gives the decoder precise timing boundaries instead of only half the waveform.
   */
  sfr_ITC.CR1.PBIS = 3;
}

void peripherals_init_ir_timer(void) {
  /*
   * TIM4 is used only as a free-running timebase for the NEC decoder.
   * We fully define its state here before enabling it from the decoder logic.
   */
  sfr_TIM4.CR1.CEN = 0;
  sfr_TIM4.CNTR.byte = 0x00;

  /*
   * ARPE=1 buffers ARR writes so the timer period changes cleanly on an update event.
   * We do not retune TIM4 at runtime today, but keeping preload enabled avoids surprises
   * if that ever changes and matches standard timer setup practice.
   */
  sfr_TIM4.CR1.ARPE = 1;

  /*
   * Clear the event-generation register so we start from a neutral state before loading
   * the divider and period values.
   */
  sfr_TIM4.EGR.byte = 0x00;

  /*
   * TIM4 timing:
   * MCU clock = 16 MHz
   * prescaler = 2^6 = 64  -> timer clock = 250 kHz
   * timer tick = 4 us
   * ARR = 24 means the counter runs over 25 ticks
   * 25 * 4 us = 100 us update period
   *
   * That 100 us quantum is the base unit used everywhere in the NEC decoder thresholds.
   */
  sfr_TIM4.PSCR.PSC = IR_TIMER_PRESCALER;
  sfr_TIM4.ARR.byte = IR_TIMER_ARR;

  /*
   * UIE enables the update interrupt so the decoder can increment its timeout counter
   * every 100 us while a frame is being received.
   */
  sfr_TIM4.IER.UIE = 1;
}

void peripherals_init_leds_timer(void) {
  /*
   * Reset TIM2 back to a known baseline before reprogramming it for LED PWM.
   * This is intentionally verbose because TIM2 may be reinitialized when the PWM
   * frequency helper recomputes a new prescaler/period pair.
   */
  sfr_TIM2.CR1.byte = sfr_TIM2_CR1_RESET_VALUE;
  sfr_TIM2.IER.byte = sfr_TIM2_IER_RESET_VALUE;
  sfr_TIM2.SR1.byte = sfr_TIM2_SR1_RESET_VALUE;
  sfr_TIM2.SR2.byte = sfr_TIM2_SR2_RESET_VALUE;
  sfr_TIM2.EGR.byte = sfr_TIM2_EGR_RESET_VALUE;
  sfr_TIM2.CCMR1.byte = sfr_TIM2_CCMR1_RESET_VALUE;
  sfr_TIM2.CCMR2.byte = sfr_TIM2_CCMR2_RESET_VALUE;
  sfr_TIM2.CCMR3.byte = sfr_TIM2_CCMR3_RESET_VALUE;
  sfr_TIM2.CCER1.byte = sfr_TIM2_CCER1_RESET_VALUE;
  sfr_TIM2.CCER2.byte = sfr_TIM2_CCER2_RESET_VALUE;
  sfr_TIM2.CNTRH.byte = sfr_TIM2_CNTRH_RESET_VALUE;
  sfr_TIM2.CNTRL.byte = sfr_TIM2_CNTRL_RESET_VALUE;

  /*
   * A temporary prescaler value is loaded here only so the timer has a sane default
   * immediately after reset. The real PWM timing is established later by
   * peripherals_leds_pwm_set_frequency(), which overwrites both PSC and ARR.
   */
  sfr_TIM2.PSCR.PSC = LED_PWM_TIMER_DEFAULT_PRESCALER;
  sfr_TIM2.ARRH.byte = sfr_TIM2_ARRH_RESET_VALUE;
  sfr_TIM2.ARRL.byte = sfr_TIM2_ARRL_RESET_VALUE;
  sfr_TIM2.CCR1H.byte = sfr_TIM2_CCR1H_RESET_VALUE;
  sfr_TIM2.CCR1L.byte = sfr_TIM2_CCR1L_RESET_VALUE;
  sfr_TIM2.CCR2H.byte = sfr_TIM2_CCR2H_RESET_VALUE;
  sfr_TIM2.CCR2L.byte = sfr_TIM2_CCR2L_RESET_VALUE;
  sfr_TIM2.CCR3H.byte = sfr_TIM2_CCR3H_RESET_VALUE;
  sfr_TIM2.CCR3L.byte = sfr_TIM2_CCR3L_RESET_VALUE;

  /*
   * UG forces an update event so every preload/register write above is pushed into the
   * active timer domain immediately. That guarantees the next timer start uses exactly
   * the values we just loaded, not stale values from a previous configuration.
   */
  sfr_TIM2.EGR.UG = 1;
}

void peripherals_init_led_feature_timer(void) {
  /*
   * TIM1 is used as a low-frequency scheduler for time-based LED features, currently
   * the go-to-sleep fade. We configure it as a simple upcounter that generates one
   * periodic update interrupt every 10 ms.
   */
  sfr_TIM1.CR1.byte = sfr_TIM1_CR1_RESET_VALUE;

  /*
   * ARPE=1 buffers ARR so period updates happen cleanly on update events instead of
   * mid-cycle. URS=1 restricts update requests to genuine counter overflow/underflow
   * events or explicit UG writes, which keeps the timing ISR cadence predictable.
   */
  sfr_TIM1.CR1.ARPE = 1;
  sfr_TIM1.CR1.URS = 1;
  sfr_TIM1.CR2.byte = sfr_TIM1_CR2_RESET_VALUE;

  /*
   * TIM1 timing:
   * PSC = 0x3e7f = 15999, so the 16 MHz clock is divided by (15999 + 1) = 16000.
   * That leaves a 1 kHz timer clock, i.e. one counter increment every 1 ms.
   */
  sfr_TIM1.PSCRH.byte = LED_FEATURE_TIMER_PSCRH;
  sfr_TIM1.PSCRL.byte = LED_FEATURE_TIMER_PSCRL;
  sfr_TIM1.CR2.byte = sfr_TIM1_CR2_RESET_VALUE;
  sfr_TIM1.CNTRH.byte = 0x00;
  sfr_TIM1.CNTRL.byte = 0x00;

  /*
   * ARR = 0x0063 = 99, so the timer overflows every (99 + 1) = 100 counts.
   * With a 1 ms counter tick, that gives a 100 ms period? No: 100 counts * 1 ms = 100 ms.
   * This means the feature timer currently ticks every 100 ms, which is the cadence used
   * by the sleep feature duration constants.
   */
  sfr_TIM1.ARRH.byte = LED_FEATURE_TIMER_ARRH;
  sfr_TIM1.ARRL.byte = LED_FEATURE_TIMER_ARRL;

  /*
   * UG latches PSC/ARR into the active timer, and CEN stays cleared so the timer is
   * armed but not running until a feature explicitly starts it.
   */
  sfr_TIM1.EGR.UG = 1;
  sfr_TIM1.CR1.CEN = 0;
}

void peripherals_led_feature_timer_start(void) {
  /* Enable the update interrupt first, then generate an update and finally start counting. */
  sfr_TIM1.IER.UIE = 1;
  sfr_TIM1.EGR.UG = 1;
  sfr_TIM1.CR1.CEN = 1;
}

void peripherals_led_feature_timer_stop(void) {
  sfr_TIM1.CR1.CEN = 0;
  sfr_TIM1.IER.UIE = 0;
}

void peripherals_leds_pwm_set_frequency(uint32_t cent_hz) {
  uint8_t prescaler;
  uint16_t auto_reload;
  uint16_t tmp;
  uint32_t pwm_clock_centi_hz = MCU_MASTER_CLOCK_HZ * 100UL;

  /*
   * The API receives frequency in centi-Hz so 10000 means 100.00 Hz.
   * We search for the smallest TIM2 prescaler that keeps ARR within 16 bits, then derive
   * ARR from: f_pwm = f_clk / (2^PSC * (ARR + 1)).
   *
   * Multiplying the master clock by 100 lets us stay in integer arithmetic while working
   * directly with centi-Hz input values.
   */
  tmp = (uint16_t) (pwm_clock_centi_hz / (cent_hz * TIMER_AUTORELOAD_MAX));
  prescaler = 0U;
  while (tmp >= (uint16_t) (0x0001U << prescaler)) {
    prescaler++;
  }

  auto_reload = (uint16_t) ((pwm_clock_centi_hz >> prescaler) / cent_hz) - 1U;

  peripherals_init_leds_timer();

  if (cent_hz == 0UL || prescaler > 15U) {
    return;
  }

  /*
   * ARPE=1 means the new ARR value becomes active on update, not halfway through a PWM
   * cycle. That avoids glitches when the PWM period is reconfigured.
   */
  sfr_TIM2.CR1.ARPE = 1;
  sfr_TIM2.PSCR.PSC = prescaler;
  sfr_TIM2.ARRH.byte = (uint8_t) (auto_reload >> 8);
  sfr_TIM2.ARRL.byte = (uint8_t) auto_reload;

  /*
   * Channel setup for all three LEDs:
   * - CCxS=0 selects output compare mode rather than input capture.
   * - OCxM=6 selects PWM mode 1.
   *   In this mode the output is active while CNT < CCRx and inactive afterwards,
   *   which gives a duty cycle proportional to CCRx.
   * - OCxPE=1 enables CCR preload so duty cycle changes are synchronized to update events.
   * - CCxP=0 keeps normal polarity: active level is high.
   *
   * The channel enable bits (CCxE) are not asserted here; they are turned on when each
   * duty cycle is written, which keeps outputs quiet until real compare values exist.
   */
  sfr_TIM2.CCMR1.CC1S = 0;
  sfr_TIM2.CCMR1.OC1M = 6;
  sfr_TIM2.CCMR1.OC1PE = 1;
  sfr_TIM2.CCER1.CC1P = 0;

  sfr_TIM2.CCMR2.CC2S = 0;
  sfr_TIM2.CCMR2.OC2M = 6;
  sfr_TIM2.CCMR2.OC2PE = 1;
  sfr_TIM2.CCER1.CC2P = 0;

  sfr_TIM2.CCMR3.CC3S = 0;
  sfr_TIM2.CCMR3.OC3M = 6;
  sfr_TIM2.CCMR3.OC3PE = 1;
  sfr_TIM2.CCER2.CC3P = 0;

  /*
   * Force one update event so PSC, ARR, and all preloaded channel settings become active,
   * then start the counter. From this point TIM2 continuously generates the PWM carrier.
   */
  sfr_TIM2.EGR.UG = 1;
  sfr_TIM2.CR1.CEN = 1;
}

void peripherals_leds_set_duty_cycle(uint8_t channel, uint16_t deci_percent) {
  uint16_t capture_compare;
  uint16_t auto_reload;

  auto_reload = ((uint16_t) sfr_TIM2.ARRH.byte << 8) | (uint16_t) sfr_TIM2.ARRL.byte;

  if (deci_percent >= LED_INTENSITY_SCALE_MAX) {
    capture_compare = auto_reload + 1U;
  } else {
    capture_compare = (uint16_t) scale_value(deci_percent, LED_INTENSITY_SCALE_MAX, auto_reload);
  }

  if (channel == 1U) {
    sfr_TIM2.CCR1H.byte = (uint8_t) (capture_compare >> 8);
    sfr_TIM2.CCR1L.byte = (uint8_t) capture_compare;
    sfr_TIM2.CCER1.CC1E = 1;
  } else if (channel == 2U) {
    sfr_TIM2.CCR2H.byte = (uint8_t) (capture_compare >> 8);
    sfr_TIM2.CCR2L.byte = (uint8_t) capture_compare;
    sfr_TIM2.CCER1.CC2E = 1;
  } else if (channel == 3U) {
    sfr_TIM2.CCR3H.byte = (uint8_t) (capture_compare >> 8);
    sfr_TIM2.CCR3L.byte = (uint8_t) capture_compare;
    sfr_TIM2.CCER2.CC3E = 1;
  }
}

void peripherals_leds_set_state(const LEDState *led_state) {
  uint16_t red = led_state->R;
  uint16_t green = led_state->G;
  uint16_t blue = led_state->B;

  if (red > LED_INTENSITY_SCALE_MAX) {
    red = LED_INTENSITY_SCALE_MAX;
  }
  if (green > LED_INTENSITY_SCALE_MAX) {
    green = LED_INTENSITY_SCALE_MAX;
  }
  if (blue > LED_INTENSITY_SCALE_MAX) {
    blue = LED_INTENSITY_SCALE_MAX;
  }

  peripherals_leds_set_duty_cycle(RED_PWM_CHAN, red);
  peripherals_leds_set_duty_cycle(GREEN_PWM_CHAN, green);
  peripherals_leds_set_duty_cycle(BLUE_PWM_CHAN, blue);
}

void peripherals_leds_start(const LEDState *led_state) {
  sfr_TIM2.CR1.CEN = 1;
  peripherals_leds_set_state(led_state);
}

void peripherals_leds_stop(void) {
  peripherals_leds_set_duty_cycle(RED_PWM_CHAN, 0U);
  peripherals_leds_set_duty_cycle(GREEN_PWM_CHAN, 0U);
  peripherals_leds_set_duty_cycle(BLUE_PWM_CHAN, 0U);
  sfr_TIM2.EGR.UG = 1;
  sfr_TIM2.CR1.CEN = 0;
}

uint8_t peripherals_ir_read_pin_state(void) {
  return (uint8_t) ((IR_PORT.IDR.byte & IR_PIN) >> IR_PIN_NR);
}

void peripherals_ir_timer_start(void) {
  sfr_TIM4.CR1.CEN = 1;
}

void peripherals_ir_timer_stop(void) {
  sfr_TIM4.CR1.CEN = 0;
}

void peripherals_ir_timer_reset(void) {
  sfr_TIM4.CNTR.byte = 0x00;
}
