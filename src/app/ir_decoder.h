#ifndef IR_DECODER_H
#define IR_DECODER_H

#include <stdint.h>

typedef enum IRDecoderStatus {
  IR_DECODER_STATUS_NONE = 0,
  IR_DECODER_STATUS_FRAME,
  IR_DECODER_STATUS_REPEAT,
  IR_DECODER_STATUS_INVALID
} IRDecoderStatus;

typedef enum IRDecoderPhase {
  IR_DECODER_PHASE_IDLE = 0,
  IR_DECODER_PHASE_LEADER_MARK,
  IR_DECODER_PHASE_LEADER_SPACE,
  IR_DECODER_PHASE_BIT_MARK,
  IR_DECODER_PHASE_BIT_SPACE
} IRDecoderPhase;

typedef struct IRFrame {
  uint8_t address_low;
  uint8_t address_high;
  uint8_t command;
  uint8_t command_inverse;
} IRFrame;

typedef struct IRDecoderState {
  uint16_t timer_counter;
  IRDecoderPhase state;
  uint8_t raw_bytes[4];
  uint8_t bit_index;
  IRDecoderStatus status;
  uint8_t has_valid_frame;
  uint16_t repeat_counter;
  IRFrame frame;
} IRDecoderState;

void ir_decoder_init(volatile IRDecoderState *decoder);
void ir_decoder_on_edge(volatile IRDecoderState *decoder);
void ir_decoder_on_timer_tick(volatile IRDecoderState *decoder);
IRDecoderStatus ir_decoder_get_status(const volatile IRDecoderState *decoder);
void ir_decoder_clear_status(volatile IRDecoderState *decoder);
uint16_t ir_decoder_get_repeat_counter(const volatile IRDecoderState *decoder);
void ir_decoder_reset_repeat_counter(volatile IRDecoderState *decoder);
uint8_t ir_decoder_get_command(const volatile IRDecoderState *decoder);
void ir_decoder_get_frame(const volatile IRDecoderState *decoder, IRFrame *frame);

#endif
