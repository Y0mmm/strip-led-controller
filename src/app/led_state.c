#include "led_state.h"

static uint16_t led_math_clamp_unit(uint16_t value) {
  if (value > 1000U) {
    return 1000U;
  }
  return value;
}

static void hsv_to_rgb(uint16_t *red, uint16_t *green, uint16_t *blue, uint16_t hue, uint16_t saturation, uint16_t value) {
  uint16_t normalized_hue = (uint16_t) (hue % 360U);
  uint16_t region = (uint16_t) (normalized_hue / 60U);
  uint16_t remainder = (uint16_t) (normalized_hue % 60U);
  uint32_t chroma = ((uint32_t) value * (uint32_t) saturation) / 1000U;
  uint16_t sector_distance;
  uint16_t x;
  uint16_t match;

  if ((region & 1U) == 0U) {
    sector_distance = remainder;
  } else {
    sector_distance = (uint16_t) (60U - remainder);
  }
  x = (uint16_t) (((uint32_t) chroma * (uint32_t) sector_distance) / 60U);
  match = (uint16_t) ((uint16_t) chroma <= value ? value - (uint16_t) chroma : 0U);

  if (region == 0U) {
    *red = (uint16_t) chroma;
    *green = x;
    *blue = 0U;
  } else if (region == 1U) {
    *red = x;
    *green = (uint16_t) chroma;
    *blue = 0U;
  } else if (region == 2U) {
    *red = 0U;
    *green = (uint16_t) chroma;
    *blue = x;
  } else if (region == 3U) {
    *red = 0U;
    *green = x;
    *blue = (uint16_t) chroma;
  } else if (region == 4U) {
    *red = x;
    *green = 0U;
    *blue = (uint16_t) chroma;
  } else {
    *red = (uint16_t) chroma;
    *green = 0U;
    *blue = x;
  }

  *red = led_math_clamp_unit((uint16_t) (*red + match));
  *green = led_math_clamp_unit((uint16_t) (*green + match));
  *blue = led_math_clamp_unit((uint16_t) (*blue + match));
}

void led_state_update_rgb(LEDState *led_state) {
  uint16_t red = 0U;
  uint16_t green = 0U;
  uint16_t blue = 0U;

  hsv_to_rgb(&red, &green, &blue, led_state->H, led_state->S, led_state->V);

  led_state->R = red;
  led_state->G = green;
  led_state->B = blue;
}

void led_state_set_hsv(LEDState *led_state, uint16_t hue, uint16_t saturation, uint16_t value) {
  led_state->H = hue;
  led_state->S = led_math_clamp_unit(saturation);
  led_state->V = led_math_clamp_unit(value);
  led_state_update_rgb(led_state);
}
