#ifndef IR_COMMANDS_H
#define IR_COMMANDS_H

#include <stdint.h>

#include "led_features.h"
#include "led_state.h"

void ir_commands_handle(uint8_t command, LEDState *led_state, LEDTimerFeature *feature);

#endif
