#ifndef LED_STATE_H
#define LED_STATE_H

#include <stdint.h>

typedef struct LEDState {
  uint8_t is_on;
  int8_t dim_level;
  uint16_t H;
  uint16_t S;
  uint16_t V;
  uint16_t R;
  uint16_t G;
  uint16_t B;
} LEDState;

void led_state_update_rgb(LEDState *led_state);
void led_state_set_hsv(LEDState *led_state, uint16_t hue, uint16_t saturation, uint16_t value);

#endif
