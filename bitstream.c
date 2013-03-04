#include "bitstream.h"
#include <assert.h>

void
rbsp_attach_buffer(rbsp_state_t *state, const char *buf, size_t byte_count)
{
    state->buf_ptr      = buf;
    state->byte_count   = byte_count;
    state->cur_ptr      = buf;
    state->cur_byte_value = 0;
    state->bit_ptr      = 7;
    state->zeros_in_row = 0;
}

int
inline
rbsp_consume_byte(rbsp_state_t *state)
{
    if (state->cur_ptr >= state->buf_ptr + state->byte_count)
        return -1;

    unsigned char c = state->cur_byte_value = *state->cur_ptr++;
    if (0 == c) state->zeros_in_row ++;
    else state->zeros_in_row = 0;

    if (2 == state->zeros_in_row) {
        state->zeros_in_row = 0;
        unsigned char epb = *state->cur_ptr++;
        // if epb is not actually have 0x03 value, it's not an emulation prevention
        if (0x03 != epb) state->cur_ptr--;  // rewind
    }

    return c;
}

int
rbsp_consume_bit(rbsp_state_t *state)
{
    assert (state->cur_ptr < state->buf_ptr + state->byte_count);

    int value = !!(*state->cur_ptr & (1 << state->bit_ptr));
    if (state->bit_ptr > 0) {
        state->bit_ptr --;
    } else {
        state->bit_ptr = 7;
        state->cur_ptr ++;
    }
    return value;
}

unsigned int
rbsp_get_u(rbsp_state_t *state, int bitcount)
{
    unsigned int value = 0;
    for (int k = 0; k < bitcount; k ++)
        value = (value << 1) + rbsp_consume_bit(state);

    return value;
}

unsigned int
rbsp_get_uev(rbsp_state_t *state)
{
    int zerobit_count = -1;
    int current_bit = 0;
    do {
        zerobit_count ++;
        current_bit = rbsp_consume_bit(state);
    } while (0 == current_bit);

    if (0 == zerobit_count) return 0;

    return (1 << zerobit_count) - 1 + rbsp_get_u(state, zerobit_count);
}

int
rbsp_get_sev(rbsp_state_t *state)
{
    int zerobit_count = -1;
    int current_bit = 0;
    do {
        zerobit_count ++;
        current_bit = rbsp_consume_bit(state);
    } while (0 == current_bit);

    if (0 == zerobit_count) return 0;

    int value = (1 << zerobit_count) + rbsp_get_u(state, zerobit_count);

    if (value & 1)
        return -value/2;

    return value/2;
}
