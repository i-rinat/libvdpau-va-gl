/*
 * Copyright 2013-2016  Rinat Ibragimov
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

#pragma once

#include <stdint.h>
#include <unistd.h>


/** @brief State of raw byte stream payload comsumer */
typedef struct _rbsp_state_struct {
    const uint8_t  *buf_ptr;        ///< pointer to beginning of the buffer
    size_t          byte_count;     ///< size of buffer
    const uint8_t  *cur_ptr;        ///< pointer to currently processed byte
    int             bit_ptr;        ///< pointer to currently processed bit
    int             zeros_in_row;   ///< number of consequetive zero bytes so far
    int             bits_eaten;     ///< bit offset of current position not including EPB
} rbsp_state_t;


/** @brief Initialize rbsp state
 *
 *  @param [out]    state
 *  @param [in]     buf         pointer to byte string
 *  @param [in]     byte_count  number of bytes in @param buf
 *
 *  @retval void
 */
void
rbsp_attach_buffer(rbsp_state_t *state, const uint8_t *buf, size_t byte_count);

/** @brief Consumes and returns one byte from rbsp
 *
 *  This function handles emulation prevention bytes internally, without their
 *  exposure to caller. Returns value of successfully consumed byte.

 */
int
rbsp_consume_byte(rbsp_state_t *state);

rbsp_state_t
rbsp_copy_state(rbsp_state_t *state);

int
rbsp_navigate_to_nal_unit(rbsp_state_t *state);

void
rbsp_reset_bit_counter(rbsp_state_t *state);

int
rbsp_consume_bit(rbsp_state_t *state);

unsigned int
rbsp_get_u(rbsp_state_t *state, int bitcount);

unsigned int
rbsp_get_uev(rbsp_state_t *state);

int
rbsp_get_sev(rbsp_state_t *state);
