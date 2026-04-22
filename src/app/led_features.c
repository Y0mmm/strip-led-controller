#include "led_features.h"

#include "../config/led_config.h"
#include "../drivers/peripherals.h"

static void go_to_sleep_timer_feature_callback(LEDTimerFeature *feature, LEDState *led_state) {
  GoToSleepLEDTimerFeatureData *feature_data = &feature->sleep_data;
  uint8_t new_dim_level;

  if (feature_data->remaining_time == 0U) {
    peripherals_led_feature_timer_stop();
    peripherals_leds_stop();
    return;
  }

  feature_data->remaining_time -= 1U;

  if (feature_data->remaining_time == 0U) {
    peripherals_led_feature_timer_stop();
    peripherals_leds_stop();
    return;
  }

  new_dim_level = (uint8_t) (((uint32_t) feature_data->start_dim_level * feature_data->remaining_time) / feature_data->duration);
  led_state->dim_level = (int8_t) new_dim_level;
  led_state->V = led_log_abacus[led_state->dim_level];
  led_state_update_rgb(led_state);
  peripherals_leds_set_state(led_state);
}

void led_features_init(LEDTimerFeature *feature) {
  feature->callback = (LEDTimerFeatureCallback) 0;
  feature->sleep_data.duration = 0U;
  feature->sleep_data.remaining_time = 0U;
  feature->sleep_data.start_dim_level = 0U;
}

void led_features_start_sleep(LEDTimerFeature *feature, LEDState *led_state) {
  feature->sleep_data.duration = GO_TO_SLEEP_DURATION_TICKS;
  feature->sleep_data.remaining_time = feature->sleep_data.duration;
  feature->sleep_data.start_dim_level = (uint8_t) led_state->dim_level;
  feature->callback = go_to_sleep_timer_feature_callback;
  peripherals_led_feature_timer_start();
}

void led_features_stop(LEDTimerFeature *feature) {
  feature->callback = (LEDTimerFeatureCallback) 0;
  feature->sleep_data.duration = 0U;
  feature->sleep_data.remaining_time = 0U;
  feature->sleep_data.start_dim_level = 0U;
  peripherals_led_feature_timer_stop();
}

void led_features_on_timer_tick(LEDTimerFeature *feature, LEDState *led_state) {
  if (feature->callback != (LEDTimerFeatureCallback) 0) {
    feature->callback(feature, led_state);
  }
}
