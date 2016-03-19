/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "bitstream.h"
#include <assert.h>
#include <string.h>

inline
void
rbsp_attach_buffer(rbsp_state_t *state, const uint8_t *buf, size_t byte_count)
{
    state->buf_ptr      = buf;
    state->byte_count   = byte_count;
    state->cur_ptr      = buf;
    state->bit_ptr      = 7;
    state->zeros_in_row = 0;
    state->bits_eaten   = 0;
}

rbsp_state_t
rbsp_copy_state(rbsp_state_t *state)
{
    return *state;
}

inline
int
rbsp_navigate_to_nal_unit(rbsp_state_t *state)
{
    int found = 1;
    int window[3] = {-1, -1, -1};
    do {
        if (state->cur_ptr >= state->buf_ptr + state->byte_count) {
            found = 0;      // no bytes left, no nal unit found
            break;
        }
        int c = *state->cur_ptr++;
        window[0] = window[1];
        window[1] = window[2];
        window[2] = c;
    } while (0 != window[0] || 0 != window[1] || 1 != window[2]);

    if (found)
        return (int)(state->cur_ptr - state->buf_ptr);

    return -1;
}

inline
void
rbsp_reset_bit_counter(rbsp_state_t *state)
{
    state->bits_eaten = 0;
}

inline
int
rbsp_consume_byte(rbsp_state_t *state)
{
    if (state->cur_ptr >= state->buf_ptr + state->byte_count)
        return -1;

    uint8_t c = *state->cur_ptr++;
    if (0 == c) state->zeros_in_row ++;
    else state->zeros_in_row = 0;

    if (state->zeros_in_row >= 2) {
        uint8_t epb = *state->cur_ptr++;
        if (0 != epb) state->zeros_in_row = 0;
        // if epb is not actually have 0x03 value, it's not an emulation prevention
        if (0x03 != epb) state->cur_ptr--;  // so rewind
    }

    return c;
}

inline
int
rbsp_consume_bit(rbsp_state_t *state)
{
    assert (state->cur_ptr < state->buf_ptr + state->byte_count);

    int value = !!(*state->cur_ptr & (1 << state->bit_ptr));
    if (state->bit_ptr > 0) {
        state->bit_ptr --;
    } else {
        state->bit_ptr = 7;
        rbsp_consume_byte(state);   // handles emulation prevention bytes
    }
    state->bits_eaten += 1;
    return value;
}

inline
unsigned int
rbsp_get_u(rbsp_state_t *state, int bitcount)
{
    unsigned int value = 0;
    for (int k = 0; k < bitcount; k ++)
        value = (value << 1) + rbsp_consume_bit(state);

    return value;
}

inline
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

inline
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
