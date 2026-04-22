#include "ir_commands.h"

#include "../config/led_config.h"
#include "../drivers/peripherals.h"

typedef struct IRColorCommand {
  uint8_t command;
  uint16_t hue;
  uint16_t saturation;
} IRColorCommand;

static const IRColorCommand ir_color_commands[] = {
  {COMMAND_RED, RED_H, RED_S},
  {COMMAND_GREEN, GREEN_H, GREEN_S},
  {COMMAND_BLUE, BLUE_H, BLUE_S},
  {COMMAND_WHITE, WHITE_H, WHITE_S},
  {COMMAND_ORANGE, ORANGE_H, ORANGE_S},
  {COMMAND_PINK, PINK_H, PINK_S},
  {COMMAND_PINK_BARBIE, PINK_BARBIE_H, PINK_BARBIE_S},
  {COMMAND_CYAN, CYAN_H, CYAN_S},
  {COMMAND_YELLOW, YELLOW_H, YELLOW_S},
};

static void apply_current_led_state(LEDState *led_state) {
  led_state->V = led_log_abacus[led_state->dim_level];
  led_state_update_rgb(led_state);
  peripherals_leds_set_state(led_state);
}

static const IRColorCommand *find_color_command(uint8_t command) {
  uint8_t index;

  for (index = 0U; index < (uint8_t) (sizeof(ir_color_commands) / sizeof(ir_color_commands[0])); index++) {
    if (ir_color_commands[index].command == command) {
      return &ir_color_commands[index];
    }
  }

  return (const IRColorCommand *) 0;
}

static void apply_color_command(uint8_t command, LEDState *led_state, LEDTimerFeature *feature) {
  const IRColorCommand *color_command = find_color_command(command);

  if (color_command == (const IRColorCommand *) 0) {
    return;
  }

  led_features_stop(feature);
  led_state_set_hsv(led_state, color_command->hue, color_command->saturation, led_log_abacus[led_state->dim_level]);
  peripherals_leds_set_state(led_state);
}

void ir_commands_handle(uint8_t command, LEDState *led_state, LEDTimerFeature *feature) {
  switch (command) {
    case COMMAND_ON:
      led_state->is_on = 1U;
      if (led_state->dim_level == 0) {
        led_state->dim_level = 1;
        apply_current_led_state(led_state);
      }
      peripherals_leds_start(led_state);
      break;

    case COMMAND_OFF:
      led_state->is_on = 0U;
      peripherals_leds_stop();
      led_features_stop(feature);
      break;

    case COMMAND_DIM_UP:
      led_features_stop(feature);
      if (led_state->dim_level < MAX_DIM_LEVEL) {
        led_state->dim_level += DIM_LEVELS_STEPS;
        if (led_state->dim_level >= MAX_DIM_LEVEL) {
          led_state->dim_level = MAX_DIM_LEVEL;
        }
        apply_current_led_state(led_state);
        if (led_state->dim_level == DIM_LEVELS_STEPS) {
          led_state->is_on = 1U;
          peripherals_leds_start(led_state);
        }
      }
      break;

    case COMMAND_DIM_DOWN:
      led_features_stop(feature);
      if (led_state->dim_level > 0) {
        led_state->dim_level -= DIM_LEVELS_STEPS;
        if (led_state->dim_level < 0) {
          led_state->dim_level = 0;
        }
        apply_current_led_state(led_state);
      }
      if (led_state->dim_level <= 0) {
        peripherals_leds_stop();
      }
      break;

    case COMMAND_RED:
    case COMMAND_GREEN:
    case COMMAND_BLUE:
    case COMMAND_WHITE:
    case COMMAND_ORANGE:
    case COMMAND_PINK:
    case COMMAND_PINK_BARBIE:
    case COMMAND_CYAN:
    case COMMAND_YELLOW:
      apply_color_command(command, led_state, feature);
      break;

    case COMMAND_GOTOSLEEP:
      led_features_stop(feature);
      led_features_start_sleep(feature, led_state);
      break;

    default:
      break;
  }
}
