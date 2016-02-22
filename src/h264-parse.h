/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef VA_GL_SRC_H264_PARSE_H
#define VA_GL_SRC_H264_PARSE_H

#include <va/va.h>
#include "bitstream.h"


void
parse_slice_header(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                   const int ChromaArrayType,  unsigned int p_num_ref_idx_l0_active_minus1,
                   unsigned int p_num_ref_idx_l1_active_minus1, VASliceParameterBufferH264 *vasp);

void
reset_va_picture_h264(VAPictureH264 *p);

#endif /* VA_GL_SRC_H264_PARSE_H */
