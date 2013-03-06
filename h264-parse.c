#include <assert.h>
#include <stdio.h>
#include "h264-parse.h"

void
parse_slice_header(rbsp_state_t *st, const VdpPictureInfoH264 *vdppi,
                   const int ChromaArrayType, VASliceParameterBufferH264 *vasp)
{
    fprintf(stderr, "forbidden_zero_bit = %d\n", rbsp_get_u(st, 1));
    fprintf(stderr, "nal_ref_idc = %d\n", rbsp_get_u(st, 2));
    int nal_unit_type = rbsp_get_u(st, 5);
    fprintf(stderr, "nal_unit_type = %d\n", nal_unit_type);
    if (14 == nal_unit_type || 20 == nal_unit_type) {
        fprintf(stderr, "QQQQQQQQQQQQQQQQQQQQQQ\n");
    }

    fprintf(stderr, "first_mb_in_slice = %d\n", rbsp_get_uev(st));
    int slice_type = rbsp_get_uev(st);
    fprintf(stderr, "slice_type = %d\n", slice_type);
    if (slice_type > 4) slice_type -= 5;
    fprintf(stderr, "frame_num = %d\n",
        rbsp_get_u(st, vdppi->log2_max_frame_num_minus4 + 4));
    if (vdppi->frame_mbs_only_flag) {
        int field_pic_flag = rbsp_get_u(st, 1);
        fprintf(stderr, "field_pic_flag = %d\n", field_pic_flag);
        if (field_pic_flag) {
            fprintf(stderr, "bottom_field_flag = %d\n", rbsp_get_u(st, 1));
        }
    }
    if (NAL_IDR_SLICE == nal_unit_type) {    // IDR picture
        fprintf(stderr, "idr_pic_id = %d\n", rbsp_get_uev(st));
    }
    if (0 == vdppi->pic_order_cnt_type) {
        fprintf(stderr, "pic_order_cnt_lsb = %d\n",
            rbsp_get_u(st, vdppi->log2_max_pic_order_cnt_lsb_minus4 + 4));
        if (vdppi->pic_order_present_flag && !vdppi->field_pic_flag) {
            fprintf(stderr, "delta_pic_order_cnt_bottom = %d\n", rbsp_get_sev(st));
        }
    }
    if (1 == vdppi->pic_order_cnt_type && !vdppi->delta_pic_order_always_zero_flag) {
        fprintf(stderr, "delta_pic_order_cnt[0] = %d\n", rbsp_get_sev(st));
        if (vdppi->pic_order_present_flag && !vdppi->field_pic_flag) {
            fprintf(stderr, "delta_pic_order_cnt[1] = %d\n", rbsp_get_sev(st));
        }
    }
    if (vdppi->redundant_pic_cnt_present_flag) {
        fprintf(stderr, "redundant_pic_cnt = %d\n", rbsp_get_uev(st));
    }
    if (SLICE_TYPE_B == slice_type) {
        fprintf(stderr, "direct_spatial_mv_pred_flag = %d\n", rbsp_get_u(st, 1));
    }
    if (SLICE_TYPE_P == slice_type || SLICE_TYPE_SP == slice_type || SLICE_TYPE_B == slice_type) {
        int num_ref_idx_active_override_flag = rbsp_get_u(st, 1);
        if (num_ref_idx_active_override_flag) {
            fprintf(stderr, "num_ref_idx_l0_active_minus1 = %d\n", rbsp_get_uev(st));
            if (SLICE_TYPE_B == slice_type) {
                fprintf(stderr, "num_ref_idx_l1_active_minus1 = %d\n", rbsp_get_uev(st));
            }
        }
    }
    if (20 == nal_unit_type) {
        assert(0 && "not implemented");
    } else {
        // begin of ref_pic_list_modification( )
        if (2 != slice_type && 4 != slice_type) {
            int ref_pic_list_modification_flag_l0 = rbsp_get_u(st, 1);
            fprintf(stderr, "ref_pic_list_modification_flag_l0 = %d\n",
                ref_pic_list_modification_flag_l0);
            if (ref_pic_list_modification_flag_l0) {
                int modification_of_pic_nums_idc;
                do {
                    modification_of_pic_nums_idc = rbsp_get_uev(st);
                    if (0 == modification_of_pic_nums_idc ||
                        1 == modification_of_pic_nums_idc)
                    {
                        fprintf(stderr, "abs_diff_pic_num_minus1 = %d\n", rbsp_get_uev(st));
                    } else if (2 == modification_of_pic_nums_idc) {
                        fprintf(stderr, "long_term_pic_num = %d\n", rbsp_get_uev(st));
                    }
                } while (modification_of_pic_nums_idc != 3);

            }
        }

        if (1 == slice_type) {
            int ref_pic_list_modification_flag_l1 = rbsp_get_u(st, 1);
            if (ref_pic_list_modification_flag_l1) {
                int modification_of_pic_nums_idc;
                do {
                    modification_of_pic_nums_idc = rbsp_get_uev(st);
                    if (0 == modification_of_pic_nums_idc ||
                        1 == modification_of_pic_nums_idc)
                    {
                        fprintf(stderr, "abs_diff_pic_num_minus1 = %d\n", rbsp_get_uev(st));
                    } else if (2 == modification_of_pic_nums_idc) {
                        fprintf(stderr, "long_term_pic_num = %d\n", rbsp_get_uev(st));
                    }
                } while (modification_of_pic_nums_idc != 3);
            }
        }
        // end of ref_pic_list_modification( )
    }

    if ((vdppi->weighted_pred_flag &&
        (SLICE_TYPE_P == slice_type || SLICE_TYPE_SP == slice_type)) ||
        (1 == vdppi->weighted_bipred_idc && SLICE_TYPE_B == slice_type))
    {
        // begin of pred_weight_table( )
        fprintf(stderr, "luma_log2_weight_denom = %d\n", rbsp_get_uev(st));
        if (0 != ChromaArrayType) {
            fprintf(stderr, "chroma_log2_weight_denom = %d\n", rbsp_get_uev(st));
        }
        for (int k = 0; k <= vdppi->num_ref_idx_l0_active_minus1; k ++) {
            int luma_weight_l0_flag = rbsp_get_u(st, 1);
            fprintf(stderr, "luma_weight_l0_flag = %d\n", luma_weight_l0_flag);
            if (luma_weight_l0_flag) {
                fprintf(stderr, "luma_weight_l0[%d] = %d\n", k, rbsp_get_sev(st));
                fprintf(stderr, "luma_offset_l0[%d] = %d\n", k, rbsp_get_sev(st));
            }
            if (0 != ChromaArrayType) {
                int chroma_weight_l0_flag = rbsp_get_u(st, 1);
                fprintf(stderr, "chroma_weight_l0_flag = %d\n", chroma_weight_l0_flag);
                if (chroma_weight_l0_flag) {
                    for (int j = 0; j < 2; j ++) {
                        fprintf(stderr, "chroma_weight_l0[%d][%d] = %d\n", k, j, rbsp_get_sev(st));
                        fprintf(stderr, "chroma_offset_l0[%d][%d] = %d\n", k, j, rbsp_get_sev(st));
                    }
                }
            }
        }
        if (1 == slice_type) {
            for (int k = 0; k <= vdppi->num_ref_idx_l1_active_minus1; k ++) {
                int luma_weight_l1_flag = rbsp_get_u(st, 1);
                fprintf(stderr, "luma_weight_l1_flag = %d\n", luma_weight_l1_flag);
                if (luma_weight_l1_flag) {
                    fprintf(stderr, "luma_weight_l1[%d] = %d\n", k, rbsp_get_sev(st));
                    fprintf(stderr, "luma_offset_l1[%d] = %d\n", k, rbsp_get_sev(st));
                }
                if (0 != ChromaArrayType) {
                    int chroma_weight_l1_flag = rbsp_get_u(st, 1);
                    fprintf(stderr, "chroma_weight_l1_flag = %d\n", chroma_weight_l1_flag);
                    if (chroma_weight_l1_flag) {
                        for (int j = 0; j < 2; j ++) {
                            fprintf(stderr, "chroma_weight_l1[%d][%d] = %d\n", k, j, rbsp_get_sev(st));
                            fprintf(stderr, "chroma_offset_l1[%d][%d] = %d\n", k, j, rbsp_get_sev(st));
                        }
                    }
                }
            }
        }
        // end of pred_weight_table( )
    }

}

