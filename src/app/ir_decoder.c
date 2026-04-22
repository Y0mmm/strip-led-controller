#include "ir_decoder.h"

#include "../drivers/peripherals.h"

#define NEC_LEADER_MARK_MIN_TICKS 87U
#define NEC_LEADER_MARK_MAX_TICKS 93U
#define NEC_LEADER_SPACE_MIN_TICKS 43U
#define NEC_LEADER_SPACE_MAX_TICKS 47U
#define NEC_REPEAT_SPACE_MIN_TICKS 21U
#define NEC_REPEAT_SPACE_MAX_TICKS 24U
#define NEC_BIT_MARK_MIN_TICKS 4U
#define NEC_BIT_MARK_MAX_TICKS 7U
#define NEC_ZERO_SPACE_MIN_TICKS 4U
#define NEC_ZERO_SPACE_MAX_TICKS 7U
#define NEC_ONE_SPACE_MIN_TICKS 15U
#define NEC_ONE_SPACE_MAX_TICKS 18U
#define NEC_FRAME_TIMEOUT_TICKS 120U
#define NEC_FRAME_BIT_COUNT 32U

static uint8_t ir_decoder_is_between(uint16_t value, uint16_t min_value, uint16_t max_value) {
  return (uint8_t) (value >= min_value && value <= max_value);
}

static void ir_decoder_clear_partial_frame(volatile IRDecoderState *decoder) {
  decoder->raw_bytes[0] = 0U;
  decoder->raw_bytes[1] = 0U;
  decoder->raw_bytes[2] = 0U;
  decoder->raw_bytes[3] = 0U;
  decoder->bit_index = 0U;
}

static void ir_decoder_goto_idle(volatile IRDecoderState *decoder) {
  peripherals_ir_timer_stop();
  decoder->timer_counter = 0U;
  decoder->state = IR_DECODER_PHASE_IDLE;
  ir_decoder_clear_partial_frame(decoder);
}

static void ir_decoder_restart_timing(volatile IRDecoderState *decoder) {
  peripherals_ir_timer_reset();
  decoder->timer_counter = 0U;
}

static void ir_decoder_store_bit(volatile IRDecoderState *decoder, uint8_t bit_value) {
  uint8_t byte_index = (uint8_t) (decoder->bit_index / 8U);
  uint8_t bit_position = (uint8_t) (decoder->bit_index % 8U);

  if (bit_value != 0U) {
    decoder->raw_bytes[byte_index] |= (uint8_t) (1U << bit_position);
  }

  decoder->bit_index += 1U;
}

static uint8_t ir_decoder_frame_is_valid(const volatile IRDecoderState *decoder) {
  uint8_t address_low = decoder->raw_bytes[0];
  uint8_t address_high = decoder->raw_bytes[1];
  uint8_t command = decoder->raw_bytes[2];
  uint8_t command_inverse = decoder->raw_bytes[3];
  uint8_t address_is_classic_nec = (uint8_t) ((uint8_t) ~address_low == address_high);
  uint8_t address_is_extended_nec = (uint8_t) (address_low != address_high);

  return (uint8_t) ((address_is_classic_nec != 0U || address_is_extended_nec != 0U) &&
                    (uint8_t) ~command == command_inverse);
}

static void ir_decoder_finalize_frame(volatile IRDecoderState *decoder) {
  if (ir_decoder_frame_is_valid(decoder) == 0U) {
    decoder->repeat_counter = 0U;
    decoder->status = IR_DECODER_STATUS_INVALID;
    ir_decoder_goto_idle(decoder);
    return;
  }

  decoder->frame.address_low = decoder->raw_bytes[0];
  decoder->frame.address_high = decoder->raw_bytes[1];
  decoder->frame.command = decoder->raw_bytes[2];
  decoder->frame.command_inverse = decoder->raw_bytes[3];
  decoder->repeat_counter = 0U;
  decoder->has_valid_frame = 1U;
  decoder->status = IR_DECODER_STATUS_FRAME;
  ir_decoder_goto_idle(decoder);
}

void ir_decoder_init(volatile IRDecoderState *decoder) {
  decoder->timer_counter = 0U;
  decoder->state = IR_DECODER_PHASE_IDLE;
  decoder->raw_bytes[0] = 0U;
  decoder->raw_bytes[1] = 0U;
  decoder->raw_bytes[2] = 0U;
  decoder->raw_bytes[3] = 0U;
  decoder->bit_index = 0U;
  decoder->status = IR_DECODER_STATUS_NONE;
  decoder->has_valid_frame = 0U;
  decoder->repeat_counter = 0U;
  decoder->frame.address_low = 0U;
  decoder->frame.address_high = 0U;
  decoder->frame.command = 0U;
  decoder->frame.command_inverse = 0U;
}

void ir_decoder_on_edge(volatile IRDecoderState *decoder) {
  uint8_t pin_state = peripherals_ir_read_pin_state();
  uint8_t bit_value = 0U;

  switch (decoder->state) {
    case IR_DECODER_PHASE_IDLE:
      if (pin_state != 0U) {
        ir_decoder_goto_idle(decoder);
        return;
      }
      ir_decoder_restart_timing(decoder);
      peripherals_ir_timer_start();
      decoder->state = IR_DECODER_PHASE_LEADER_MARK;
      return;

    case IR_DECODER_PHASE_LEADER_MARK:
      if (pin_state == 0U || ir_decoder_is_between(decoder->timer_counter,
                                                   NEC_LEADER_MARK_MIN_TICKS,
                                                   NEC_LEADER_MARK_MAX_TICKS) == 0U) {
        ir_decoder_goto_idle(decoder);
        return;
      }
      ir_decoder_restart_timing(decoder);
      decoder->state = IR_DECODER_PHASE_LEADER_SPACE;
      return;

    case IR_DECODER_PHASE_LEADER_SPACE:
      if (pin_state != 0U) {
        ir_decoder_goto_idle(decoder);
        return;
      }

      if (ir_decoder_is_between(decoder->timer_counter, NEC_LEADER_SPACE_MIN_TICKS, NEC_LEADER_SPACE_MAX_TICKS) != 0U) {
        ir_decoder_clear_partial_frame(decoder);
        decoder->repeat_counter = 0U;
        ir_decoder_restart_timing(decoder);
        decoder->state = IR_DECODER_PHASE_BIT_MARK;
        return;
      }

      if (ir_decoder_is_between(decoder->timer_counter, NEC_REPEAT_SPACE_MIN_TICKS, NEC_REPEAT_SPACE_MAX_TICKS) != 0U &&
          decoder->has_valid_frame != 0U) {
        decoder->repeat_counter += 1U;
        decoder->status = IR_DECODER_STATUS_REPEAT;
        ir_decoder_goto_idle(decoder);
        return;
      }

      ir_decoder_goto_idle(decoder);
      return;

    case IR_DECODER_PHASE_BIT_MARK:
      if (pin_state == 0U || ir_decoder_is_between(decoder->timer_counter,
                                                   NEC_BIT_MARK_MIN_TICKS,
                                                   NEC_BIT_MARK_MAX_TICKS) == 0U) {
        ir_decoder_goto_idle(decoder);
        return;
      }
      ir_decoder_restart_timing(decoder);
      decoder->state = IR_DECODER_PHASE_BIT_SPACE;
      return;

    case IR_DECODER_PHASE_BIT_SPACE:
      if (pin_state != 0U) {
        ir_decoder_goto_idle(decoder);
        return;
      }

      if (ir_decoder_is_between(decoder->timer_counter, NEC_ONE_SPACE_MIN_TICKS, NEC_ONE_SPACE_MAX_TICKS) != 0U) {
        bit_value = 1U;
      } else if (ir_decoder_is_between(decoder->timer_counter, NEC_ZERO_SPACE_MIN_TICKS, NEC_ZERO_SPACE_MAX_TICKS) == 0U) {
        ir_decoder_goto_idle(decoder);
        return;
      }

      ir_decoder_store_bit(decoder, bit_value);
      if (decoder->bit_index == NEC_FRAME_BIT_COUNT) {
        ir_decoder_finalize_frame(decoder);
        return;
      }

      ir_decoder_restart_timing(decoder);
      decoder->state = IR_DECODER_PHASE_BIT_MARK;
      return;

    default:
      ir_decoder_goto_idle(decoder);
      return;
  }
}

void ir_decoder_on_timer_tick(volatile IRDecoderState *decoder) {
  decoder->timer_counter += 1U;
  if (decoder->state != IR_DECODER_PHASE_IDLE && decoder->timer_counter > NEC_FRAME_TIMEOUT_TICKS) {
    ir_decoder_goto_idle(decoder);
  }
}

IRDecoderStatus ir_decoder_get_status(const volatile IRDecoderState *decoder) {
  return decoder->status;
}

void ir_decoder_clear_status(volatile IRDecoderState *decoder) {
  decoder->status = IR_DECODER_STATUS_NONE;
}

uint16_t ir_decoder_get_repeat_counter(const volatile IRDecoderState *decoder) {
  return decoder->repeat_counter;
}

void ir_decoder_reset_repeat_counter(volatile IRDecoderState *decoder) {
  decoder->repeat_counter = 0U;
}

uint8_t ir_decoder_get_command(const volatile IRDecoderState *decoder) {
  return decoder->frame.command;
}

void ir_decoder_get_frame(const volatile IRDecoderState *decoder, IRFrame *frame) {
  frame->address_low = decoder->frame.address_low;
  frame->address_high = decoder->frame.address_high;
  frame->command = decoder->frame.command;
  frame->command_inverse = decoder->frame.command_inverse;
}
