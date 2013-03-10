#ifndef __H264_PARSE_H
#define __H264_PARSE_H

#include <va/va.h>
#include "bitstream.h"

#define SLICE_TYPE_P    0
#define SLICE_TYPE_B    1
#define SLICE_TYPE_I    2
#define SLICE_TYPE_SP   3
#define SLICE_TYPE_SI   4

#define NAL_UNSPECIFIED     0
#define NAL_SLICE           1
#define NAL_SLICE_DATA_A    2
#define NAL_SLICE_DATA_B    3
#define NAL_SLICE_DATA_C    4
#define NAL_IDR_SLICE       5

void
parse_slice_header(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                   const int ChromaArrayType,  unsigned int p_num_ref_idx_l0_active_minus1,
                   unsigned int p_num_ref_idx_l1_active_minus1, VASliceParameterBufferH264 *vasp);

#endif
