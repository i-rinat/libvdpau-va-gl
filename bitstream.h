#ifndef __BITSTREAM_H
#define __BITSTREAM_H

#include <unistd.h>

/** @brief State of raw byte stream payload comsumer */
typedef struct _rbsp_state_struct {
    const char *buf_ptr;            ///< pointer to beginning of the buffer
    size_t      byte_count;         ///< size of buffer
    const char *cur_ptr;            ///< pointer to currently processed byte
    char        cur_byte_value;     ///< value of currently processed byte
    int         bit_ptr;            ///< pointer to currently processed bit
    int         zeros_in_row;       ///< number of consequetive zero bytes so far
} rbsp_state_t;


/** @brief Initialize rbsp state
 *
 *  @param [out]    state
 *  @param [in]     buf         pointer to byte string
 *  @param [in]     byte_count  number of bytes in @param buf
 *
 *  @retval void
 */
void rbsp_attach_buffer(rbsp_state_t *state, const char *buf, size_t byte_count);

/** @brief Consumes and returns one byte from rbsp
 *
 *  This function handles emulation prevention bytes internally, without their
 *  exposure to caller. Returns value of successfully consumed byte.

 */
int rbsp_consume_byte(rbsp_state_t *state);


#endif
