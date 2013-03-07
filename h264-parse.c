#include <assert.h>
#include <stdio.h>
#include "h264-parse.h"

#define NOT_IMPLEMENTED     assert(0 && "not implemented")

struct slice_parameters {
    int nal_ref_idc;
    int nal_unit_type;
    int first_mb_in_slice;
    int slice_type;
    int pic_parameter_set_id;
    int frame_num;
    int field_pic_flag;
    int bottom_field_flag;
    int idr_pic_id;
    int pic_order_cnt_lsb;
    int delta_pic_order_cnt_bottom;
    int delta_pic_order_cnt[2];
    int redundant_pic_cnt;
    int direct_spatial_mv_pred_flag;
    int num_ref_idx_active_override_flag;
    int num_ref_idx_l0_active_minus1;
    int num_ref_idx_l1_active_minus1;
    int luma_log2_weight_denom;
    int chroma_log2_weight_denom;
    unsigned int luma_weight_l0_flag;
    int luma_weight_l0[32];
    int luma_offset_l0[32];
    unsigned int chroma_weight_l0_flag;
    int chroma_weight_l0[32][2];
    int chroma_offset_l0[32][2];
    unsigned int luma_weight_l1_flag;
    int luma_weight_l1[32];
    int luma_offset_l1[32];
    unsigned int chroma_weight_l1_flag;
    int chroma_weight_l1[32][2];
    int chroma_offset_l1[32][2];
};

static
void
parse_ref_pic_list_modification(rbsp_state_t *st, const VdpPictureInfoH264 *vdppi,
                                struct slice_parameters *sp);

static
void
parse_pred_weight_table(rbsp_state_t *st, const VdpPictureInfoH264 *vdppi,
                        const int ChromaArrayType, struct slice_parameters *sp);

static
void
fill_default_pred_weight_table(const VdpPictureInfoH264 *vdppi, struct slice_parameters *sp);

void
parse_slice_header(rbsp_state_t *st, const VdpPictureInfoH264 *vdppi,
                   const int ChromaArrayType, VASliceParameterBufferH264 *vasp)
{
    struct slice_parameters sp;

    rbsp_get_u(st, 1); // forbidden_zero_bit
    sp.nal_ref_idc = rbsp_get_u(st, 2);
    sp.nal_unit_type = rbsp_get_u(st, 5);

    if (14 == sp.nal_unit_type || 20 == sp.nal_unit_type) {
        NOT_IMPLEMENTED;
    }

    sp.first_mb_in_slice = rbsp_get_uev(st);
    sp.slice_type = rbsp_get_uev(st);
    if (sp.slice_type > 4) sp.slice_type -= 5;    // wrap 5-9 to 0-4

    sp.pic_parameter_set_id = rbsp_get_uev(st);

    // TODO: separate_colour_plane_flag is 0 for all but YUV444. Now ok, but should detect properly.

    sp.frame_num = rbsp_get_u(st, vdppi->log2_max_frame_num_minus4 + 4);
    sp.field_pic_flag = 0;
    sp.bottom_field_flag = 0;
    if (!vdppi->frame_mbs_only_flag) {
        sp.field_pic_flag = rbsp_get_u(st, 1);
        if (sp.field_pic_flag) {
            sp.bottom_field_flag = rbsp_get_u(st, 1);
        }
    }
    sp.idr_pic_id = 0;
    if (NAL_IDR_SLICE == sp.nal_unit_type)    // IDR picture
        sp.idr_pic_id = rbsp_get_uev(st);

    sp.pic_order_cnt_lsb = 0;
    sp.delta_pic_order_cnt_bottom = 0;
    if (0 == vdppi->pic_order_cnt_type) {
        sp.pic_order_cnt_lsb = rbsp_get_u(st, vdppi->log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (vdppi->pic_order_present_flag && !vdppi->field_pic_flag) {
            sp.delta_pic_order_cnt_bottom = rbsp_get_sev(st);
        }
    }

    sp.delta_pic_order_cnt[0] = sp.delta_pic_order_cnt[1] = 0;
    if (1 == vdppi->pic_order_cnt_type && !vdppi->delta_pic_order_always_zero_flag) {
        sp.delta_pic_order_cnt[0] = rbsp_get_sev(st);
        if (vdppi->pic_order_present_flag && !vdppi->field_pic_flag)
            sp.delta_pic_order_cnt[1] = rbsp_get_sev(st);
    }

    sp.redundant_pic_cnt = 0;
    if (vdppi->redundant_pic_cnt_present_flag)
        sp.redundant_pic_cnt = rbsp_get_uev(st);

    sp.direct_spatial_mv_pred_flag = 0;
    if (SLICE_TYPE_B == sp.slice_type)
        sp.direct_spatial_mv_pred_flag = rbsp_get_u(st, 1);

    sp.num_ref_idx_active_override_flag = 0;
    sp.num_ref_idx_l0_active_minus1 = 0;
    sp.num_ref_idx_l1_active_minus1 = 0;
    if (SLICE_TYPE_P == sp.slice_type || SLICE_TYPE_SP == sp.slice_type ||
        SLICE_TYPE_B == sp.slice_type)
    {
        sp.num_ref_idx_active_override_flag = rbsp_get_u(st, 1);
        if (sp.num_ref_idx_active_override_flag) {
            sp.num_ref_idx_l0_active_minus1 = rbsp_get_uev(st);
            if (SLICE_TYPE_B == sp.slice_type)
                sp.num_ref_idx_l1_active_minus1 = rbsp_get_uev(st);
        }
    }

    if (20 == sp.nal_unit_type) {
        NOT_IMPLEMENTED;
    } else {
        parse_ref_pic_list_modification(st, vdppi, &sp);
    }

    fill_default_pred_weight_table(vdppi, &sp);
    if ((vdppi->weighted_pred_flag &&
        (SLICE_TYPE_P == sp.slice_type || SLICE_TYPE_SP == sp.slice_type)) ||
        (1 == vdppi->weighted_bipred_idc && SLICE_TYPE_B == sp.slice_type))
    {
        parse_pred_weight_table(st, vdppi, ChromaArrayType, &sp);
    }

    if (sp.nal_ref_idc != 0) {
        // dec_ref_pic_marking( )
        if (NAL_IDR_SLICE == sp.nal_unit_type) {
            fprintf(stderr, "no_output_of_prior_pics_flag = %d\n", rbsp_get_u(st, 1));
            fprintf(stderr, "long_term_reference_flag = %d\n", rbsp_get_u(st, 1));
        } else {
            int adaptive_ref_pic_marking_mode_flag = rbsp_get_u(st, 1);
            fprintf(stderr, "adaptive_ref_pic_marking_mode_flag = %d\n", adaptive_ref_pic_marking_mode_flag);
            if (adaptive_ref_pic_marking_mode_flag) {
                int memory_management_control_operation;
                do {
                    memory_management_control_operation = rbsp_get_uev(st);
                    fprintf(stderr, "memory_management_control_operation = %d\n", memory_management_control_operation);
                    if (1 == memory_management_control_operation ||
                        3 == memory_management_control_operation)
                    {
                        fprintf(stderr, "difference_of_pic_nums_minus1 = %d\n", rbsp_get_uev(st));
                    }
                    if (2 == memory_management_control_operation) {
                        fprintf(stderr, "long_term_pic_num = %d\n", rbsp_get_uev(st));
                    }
                    if (3 == memory_management_control_operation ||
                        6 == memory_management_control_operation)
                    {
                        fprintf(stderr, "long_term_frame_idx = %d\n", rbsp_get_uev(st));
                    }
                    if (4 == memory_management_control_operation) {
                        fprintf(stderr, "max_long_term_frame_idx_plus1 = %d\n", rbsp_get_uev(st));
                    }
                } while (memory_management_control_operation != 0);
            }
        }
        // end of dec_ref_pic_marking( )
    }
    if (vdppi->entropy_coding_mode_flag &&
        SLICE_TYPE_I != sp.slice_type && SLICE_TYPE_SI != sp.slice_type)
    {
        fprintf(stderr, "cabac_init_idc = %d\n", rbsp_get_uev(st));
    }

}


static
void
parse_ref_pic_list_modification(rbsp_state_t *st, const VdpPictureInfoH264 *vdppi,
                                struct slice_parameters *sp)
{
    if (2 != sp->slice_type && 4 != sp->slice_type) {
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

    if (1 == sp->slice_type) {
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
}

static
void
fill_default_pred_weight_table(const VdpPictureInfoH264 *vdppi, struct slice_parameters *sp)
{
    sp->luma_log2_weight_denom = 0;
    sp->chroma_log2_weight_denom = 0;
    sp->luma_weight_l0_flag = 0;
    sp->luma_weight_l1_flag = 0;
    sp->chroma_weight_l0_flag = 0;
    sp->chroma_weight_l1_flag = 0;
    for (int k = 0; k < vdppi->num_ref_idx_l0_active_minus1 + 1; k ++) {
        sp->luma_weight_l0[k] = 1;
        sp->luma_offset_l0[k] = 0;
        sp->chroma_weight_l0[k][0] = sp->chroma_weight_l0[k][1] = 1;
        sp->chroma_offset_l0[k][0] = sp->chroma_offset_l0[k][1] = 0;
    }
    for (int k = 0; k < vdppi->num_ref_idx_l1_active_minus1 + 1; k ++) {
        sp->luma_weight_l1[k] = 1;
        sp->luma_offset_l1[k] = 0;
        sp->chroma_weight_l1[k][0] = sp->chroma_weight_l1[k][1] = 1;
        sp->chroma_offset_l1[k][0] = sp->chroma_offset_l1[k][1] = 0;
    }
}


static
void
parse_pred_weight_table(rbsp_state_t *st, const VdpPictureInfoH264 *vdppi,
                        const int ChromaArrayType, struct slice_parameters *sp)
{
    sp->luma_log2_weight_denom = rbsp_get_uev(st);
    if (0 != ChromaArrayType)
        sp->chroma_log2_weight_denom = rbsp_get_uev(st);

    for (int k = 0; k <= vdppi->num_ref_idx_l0_active_minus1; k ++) {
        sp->luma_weight_l0_flag = rbsp_get_u(st, 1);
        if (sp->luma_weight_l0_flag) {
            sp->luma_weight_l0[k] = rbsp_get_sev(st);
            sp->luma_offset_l0[k] = rbsp_get_sev(st);
        }
        if (0 != ChromaArrayType) {
            sp->chroma_weight_l0_flag = rbsp_get_u(st, 1);
            if (sp->chroma_weight_l0_flag) {
                for (int j = 0; j < 2; j ++) {
                    sp->chroma_weight_l0[k][j] = rbsp_get_sev(st);
                    sp->chroma_offset_l0[k][j] = rbsp_get_sev(st);
                }
            }
        }
    }

    if (1 == sp->slice_type) {
        for (int k = 0; k <= vdppi->num_ref_idx_l1_active_minus1; k ++) {
            sp->luma_weight_l1_flag = rbsp_get_u(st, 1);
            if (sp->luma_weight_l1_flag) {
                sp->luma_weight_l1[k] = rbsp_get_sev(st);
                sp->luma_offset_l1[k] = rbsp_get_sev(st);
            }
            if (0 != ChromaArrayType) {
                sp->chroma_weight_l1_flag = rbsp_get_u(st, 1);
                if (sp->chroma_weight_l1_flag) {
                    for (int j = 0; j < 2; j ++) {
                        sp->chroma_weight_l1[k][j] = rbsp_get_sev(st);
                        sp->chroma_offset_l1[k][j] = rbsp_get_sev(st);
                    }
                }
            }
        }
    }
}
