#include <stdint.h>

#include "STM8S001J3.h"
#include "src/app/ir_commands.h"
#include "src/app/ir_decoder.h"
#include "src/app/led_features.h"
#include "src/app/led_state.h"
#include "src/config/led_config.h"
#include "src/drivers/peripherals.h"

static volatile IRDecoderState ir_decoder;
static LEDState leds_state = {
  LED_DEFAULT_IS_ON,
  LED_DEFAULT_DIM_LEVEL,
  LED_DEFAULT_HUE,
  LED_DEFAULT_SATURATION,
  LED_DEFAULT_VALUE,
  LED_DEFAULT_RED,
  LED_DEFAULT_GREEN,
  LED_DEFAULT_BLUE
};
static LEDTimerFeature led_timer_feature;

ISR_HANDLER(PORTB_ISR, _EXTI1_VECTOR_) {
  ir_decoder_on_edge(&ir_decoder);
}

ISR_HANDLER(TIM4_UPD_ISR, _TIM4_OVR_UIF_VECTOR_) {
  ir_decoder_on_timer_tick(&ir_decoder);
  sfr_TIM4.SR.UIF = 0;
}

ISR_HANDLER(TIM1_UPD_ISR, _TIM1_OVR_UIF_VECTOR_) {
  led_features_on_timer_tick(&led_timer_feature, &leds_state);
  sfr_TIM1.SR1.UIF = 0;
}

void main(void) {
  DISABLE_INTERRUPTS();

  /* Run the core at the full 16 MHz HSI rate so all timer calculations match hw_config.h. */
  sfr_CLK.CKDIVR.byte = 0x00;

  peripherals_wait_for_swim();
  peripherals_init_ports();
  peripherals_init_ir_timer();
  peripherals_init_gpio_interrupt();
  peripherals_init_leds_timer();
  peripherals_leds_pwm_set_frequency(LED_PWM_FREQUENCY_CENTI_HZ);
  peripherals_init_led_feature_timer();

  ir_decoder_init(&ir_decoder);
  led_features_init(&led_timer_feature);

  ENABLE_INTERRUPTS();

  peripherals_leds_start(&leds_state);
  peripherals_leds_stop();

  while (1) {
    IRDecoderStatus decoder_status;

    WAIT_FOR_INTERRUPT();
    decoder_status = ir_decoder_get_status(&ir_decoder);
    if (decoder_status == IR_DECODER_STATUS_NONE) {
      NOP();
      continue;
    }

    ir_decoder_clear_status(&ir_decoder);
    if (decoder_status == IR_DECODER_STATUS_FRAME) {
      ir_commands_handle(ir_decoder_get_command(&ir_decoder), &leds_state, &led_timer_feature);
    } else if (decoder_status == IR_DECODER_STATUS_REPEAT) {
      if (ir_decoder_get_repeat_counter(&ir_decoder) >= IR_REPEAT_ACTION_THRESHOLD) {
        ir_decoder_reset_repeat_counter(&ir_decoder);
        ir_commands_handle(ir_decoder_get_command(&ir_decoder), &leds_state, &led_timer_feature);
      }
    }

    NOP();
  }
}
