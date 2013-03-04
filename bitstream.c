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
