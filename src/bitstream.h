/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef VA_GL_SRC_BITSTREAM_H
#define VA_GL_SRC_BITSTREAM_H

#include <unistd.h>
#include <stdint.h>

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
void rbsp_attach_buffer(rbsp_state_t *state, const uint8_t *buf, size_t byte_count);

/** @brief Consumes and returns one byte from rbsp
 *
 *  This function handles emulation prevention bytes internally, without their
 *  exposure to caller. Returns value of successfully consumed byte.

 */
int rbsp_consume_byte(rbsp_state_t *state);

rbsp_state_t rbsp_copy_state(rbsp_state_t *state);

int rbsp_navigate_to_nal_unit(rbsp_state_t *state);

void rbsp_reset_bit_counter(rbsp_state_t *state);

int
rbsp_consume_bit(rbsp_state_t *state);

unsigned int
rbsp_get_u(rbsp_state_t *state, int bitcount);

unsigned int
rbsp_get_uev(rbsp_state_t *state);

int
rbsp_get_sev(rbsp_state_t *state);

#endif /* VA_GL_SRC_BITSTREAM_H */
