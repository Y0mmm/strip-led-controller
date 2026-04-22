#ifndef LED_FEATURES_H
#define LED_FEATURES_H

#include <stdint.h>

#include "led_state.h"

typedef struct GoToSleepLEDTimerFeatureData {
  uint32_t duration;
  uint32_t remaining_time;
  uint8_t start_dim_level;
} GoToSleepLEDTimerFeatureData;

struct LEDTimerFeature;

typedef void (*LEDTimerFeatureCallback)(struct LEDTimerFeature *feature, LEDState *led_state);

typedef struct LEDTimerFeature {
  LEDTimerFeatureCallback callback;
  GoToSleepLEDTimerFeatureData sleep_data;
} LEDTimerFeature;

void led_features_init(LEDTimerFeature *feature);
void led_features_start_sleep(LEDTimerFeature *feature, LEDState *led_state);
void led_features_stop(LEDTimerFeature *feature);
void led_features_on_timer_tick(LEDTimerFeature *feature, LEDState *led_state);

#endif
