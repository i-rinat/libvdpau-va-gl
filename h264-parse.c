#include <assert.h>
#include <stdio.h>
#include "h264-parse.h"

#define NOT_IMPLEMENTED(str)        assert(0 && "not implemented" && str)

#define DESCRIBE(xparam, format)    fprintf(stderr, #xparam " = %" #format "\n", xparam)

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
    unsigned int no_output_of_prior_pics_flag;
    unsigned int long_term_reference_flag;
    unsigned int cabac_init_idc;
    int slice_qp_delta;
    unsigned int sp_for_switch_flag;
    int slice_qs_delta;
    unsigned int disable_deblocking_filter_idc;
    int slice_alpha_c0_offset_div2;
    int slice_beta_offset_div2;

    VAPictureH264 RefPicList0[32];
    VAPictureH264 RefPicList1[32];
};

static
void
parse_ref_pic_list_modification(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                                struct slice_parameters *sp);

static
void
parse_pred_weight_table(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                        const int ChromaArrayType, struct slice_parameters *sp);

static
void
fill_default_pred_weight_table(const VAPictureParameterBufferH264 *vapp, struct slice_parameters *sp);

static
void
parse_dec_ref_pic_marking(rbsp_state_t *st, struct slice_parameters *sp);

static
void
do_fill_va_slice_parameter_buffer(struct slice_parameters const * const sp,
                                  VAPictureParameterBufferH264 const * const vapp,
                                  VASliceParameterBufferH264 *vasp, int bit_offset)
{
    vasp->slice_data_bit_offset = bit_offset;
    vasp->first_mb_in_slice = sp->first_mb_in_slice;
    vasp->slice_type = sp->slice_type;
    vasp->direct_spatial_mv_pred_flag = sp->direct_spatial_mv_pred_flag;
    vasp->num_ref_idx_l0_active_minus1 = sp->num_ref_idx_l0_active_minus1;
    vasp->num_ref_idx_l1_active_minus1 = sp->num_ref_idx_l1_active_minus1;
    vasp->cabac_init_idc = sp->cabac_init_idc;
    vasp->slice_qp_delta = sp->slice_qp_delta;
    vasp->disable_deblocking_filter_idc = sp->disable_deblocking_filter_idc;
    vasp->slice_alpha_c0_offset_div2 = sp->slice_alpha_c0_offset_div2;
    vasp->slice_beta_offset_div2 = sp->slice_beta_offset_div2;

    for (int k = 0; k < 32; k ++) {
        vasp->RefPicList0[k] = sp->RefPicList0[k];
        vasp->RefPicList1[k] = sp->RefPicList1[k];
    }

    vasp->luma_log2_weight_denom = sp->luma_log2_weight_denom;
    vasp->chroma_log2_weight_denom = sp->luma_log2_weight_denom;

    vasp->luma_weight_l0_flag = sp->luma_weight_l0_flag;
    for (int k = 0; k < 32; k ++) vasp->luma_weight_l0[k] = sp->luma_weight_l0[k];
    for (int k = 0; k < 32; k ++) vasp->luma_offset_l0[k] = sp->luma_offset_l0[k];
    vasp->chroma_weight_l0_flag = sp->chroma_weight_l0_flag;
    for (int k = 0; k < 32; k ++) vasp->chroma_weight_l0[k][0] = sp->chroma_weight_l0[k][0];
    for (int k = 0; k < 32; k ++) vasp->chroma_weight_l0[k][1] = sp->chroma_weight_l0[k][1];
    for (int k = 0; k < 32; k ++) vasp->chroma_offset_l0[k][0] = sp->chroma_offset_l0[k][0];
    for (int k = 0; k < 32; k ++) vasp->chroma_offset_l0[k][1] = sp->chroma_offset_l0[k][1];

    vasp->luma_weight_l1_flag = sp->luma_weight_l1_flag;
    for (int k = 0; k < 32; k ++) vasp->luma_weight_l1[k] = sp->luma_weight_l1[k];
    for (int k = 0; k < 32; k ++) vasp->luma_offset_l1[k] = sp->luma_offset_l1[k];
    vasp->chroma_weight_l1_flag = sp->chroma_weight_l1_flag;
    for (int k = 0; k < 32; k ++) vasp->chroma_weight_l1[k][0] = sp->chroma_weight_l1[k][0];
    for (int k = 0; k < 32; k ++) vasp->chroma_weight_l1[k][1] = sp->chroma_weight_l1[k][1];
    for (int k = 0; k < 32; k ++) vasp->chroma_offset_l1[k][0] = sp->chroma_offset_l1[k][0];
    for (int k = 0; k < 32; k ++) vasp->chroma_offset_l1[k][1] = sp->chroma_offset_l1[k][1];
}

static
void
zeroify_refpiclist_entry(VAPictureH264 *p)
{
    p->picture_id       = VA_INVALID_SURFACE;
    p->frame_idx        = 0;
    p->flags            = VA_PICTURE_H264_INVALID;
    p->TopFieldOrderCnt = 0;
    p->BottomFieldOrderCnt  = 0;
}

void
parse_slice_header(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                   const int ChromaArrayType, unsigned int p_num_ref_idx_l0_active_minus1,
                   unsigned int p_num_ref_idx_l1_active_minus1, VASliceParameterBufferH264 *vasp)
{
    struct slice_parameters sp;

    for (int k = 0; k < 32; k ++) {
        zeroify_refpiclist_entry(&sp.RefPicList0[k]);
        zeroify_refpiclist_entry(&sp.RefPicList1[k]);
    }

    // TODO: properly sort
    int ptr = 0;
    for (int k = 0; k < vapp->num_ref_frames; k ++) {
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_INVALID)
            continue;
        sp.RefPicList0[ptr] = vapp->ReferenceFrames[k];
        ptr ++;
    }

    for (int k = 0; k < vapp->num_ref_frames; k ++) {
        fprintf(stderr, "╭─────────────────────────────────────────\n");
        fprintf(stderr, "│ref picture_id = %d\n", vapp->ReferenceFrames[k].picture_id);
        fprintf(stderr, "│ref frame_idx = %d\n", vapp->ReferenceFrames[k].frame_idx);
        fprintf(stderr, "│ref flags = ");
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_INVALID)
            fprintf(stderr, "INVALID ");
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_TOP_FIELD)
            fprintf(stderr, "TOP_FIELD ");
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_BOTTOM_FIELD)
            fprintf(stderr, "BOTTOM_FIELD ");
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
            fprintf(stderr, "SHORT_TERM_REFERENCE ");
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)
            fprintf(stderr, "LONG_TERM_REFERENCE");
        fprintf(stderr, "\n");
        fprintf(stderr, "│ref TopFieldOrderCnt = %d\n", vapp->ReferenceFrames[k].TopFieldOrderCnt);
        fprintf(stderr, "│ref BottomFieldOrderCnt = %d\n", vapp->ReferenceFrames[k].BottomFieldOrderCnt);
        fprintf(stderr, "╰─────────────────────────────────────────\n");
    }

    rbsp_get_u(st, 1); // forbidden_zero_bit
    sp.nal_ref_idc = rbsp_get_u(st, 2);
    sp.nal_unit_type = rbsp_get_u(st, 5);

    if (14 == sp.nal_unit_type || 20 == sp.nal_unit_type) {
        NOT_IMPLEMENTED("nal unit types 14 and 20");
    }

    sp.first_mb_in_slice = rbsp_get_uev(st);
    sp.slice_type = rbsp_get_uev(st);
    if (sp.slice_type > 4) sp.slice_type -= 5;    // wrap 5-9 to 0-4

    sp.pic_parameter_set_id = rbsp_get_uev(st);

    // TODO: separate_colour_plane_flag is 0 for all but YUV444. Now ok, but should detect properly.

    sp.frame_num = rbsp_get_u(st, vapp->seq_fields.bits.log2_max_frame_num_minus4 + 4);
    sp.field_pic_flag = 0;
    sp.bottom_field_flag = 0;
    if (!vapp->seq_fields.bits.frame_mbs_only_flag) {
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
    if (0 == vapp->seq_fields.bits.pic_order_cnt_type) {
        sp.pic_order_cnt_lsb =
            rbsp_get_u(st, vapp->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (vapp->pic_fields.bits.pic_order_present_flag &&
            !vapp->pic_fields.bits.field_pic_flag)
        {
            sp.delta_pic_order_cnt_bottom = rbsp_get_sev(st);
        }
    }

    sp.delta_pic_order_cnt[0] = sp.delta_pic_order_cnt[1] = 0;
    if (1 == vapp->seq_fields.bits.pic_order_cnt_type &&
        !vapp->seq_fields.bits.delta_pic_order_always_zero_flag)
    {
        sp.delta_pic_order_cnt[0] = rbsp_get_sev(st);
        if (vapp->pic_fields.bits.pic_order_present_flag && !vapp->pic_fields.bits.field_pic_flag)
            sp.delta_pic_order_cnt[1] = rbsp_get_sev(st);
    }

    sp.redundant_pic_cnt = 0;
    if (vapp->pic_fields.bits.redundant_pic_cnt_present_flag)
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
        sp.num_ref_idx_l0_active_minus1 = p_num_ref_idx_l0_active_minus1;
        sp.num_ref_idx_l1_active_minus1 = p_num_ref_idx_l1_active_minus1;

        sp.num_ref_idx_active_override_flag = rbsp_get_u(st, 1);
        if (sp.num_ref_idx_active_override_flag) {
            sp.num_ref_idx_l0_active_minus1 = rbsp_get_uev(st);
            if (SLICE_TYPE_B == sp.slice_type)
                sp.num_ref_idx_l1_active_minus1 = rbsp_get_uev(st);
        }
    }

    if (20 == sp.nal_unit_type) {
        NOT_IMPLEMENTED("nal unit type 20");
    } else {
        parse_ref_pic_list_modification(st, vapp, &sp);
    }

    fill_default_pred_weight_table(vapp, &sp);
    if ((vapp->pic_fields.bits.weighted_pred_flag &&
        (SLICE_TYPE_P == sp.slice_type || SLICE_TYPE_SP == sp.slice_type)) ||
        (1 == vapp->pic_fields.bits.weighted_bipred_idc && SLICE_TYPE_B == sp.slice_type))
    {
        parse_pred_weight_table(st, vapp, ChromaArrayType, &sp);
    }

    if (sp.nal_ref_idc != 0) {
        parse_dec_ref_pic_marking(st, &sp);
    }

    sp.cabac_init_idc = 0;
    if (vapp->pic_fields.bits.entropy_coding_mode_flag &&
        SLICE_TYPE_I != sp.slice_type && SLICE_TYPE_SI != sp.slice_type)
            sp.cabac_init_idc = rbsp_get_uev(st);

    sp.slice_qp_delta = rbsp_get_sev(st);

    sp.sp_for_switch_flag = 0;
    sp.slice_qs_delta = 0;
    if (SLICE_TYPE_SP == sp.slice_type || SLICE_TYPE_SI == sp.slice_type) {
        if (SLICE_TYPE_SP == sp.slice_type)
            sp.sp_for_switch_flag = rbsp_get_u(st, 1);
        sp.slice_qs_delta = rbsp_get_sev(st);
    }

    sp.disable_deblocking_filter_idc = 0;
    sp.slice_alpha_c0_offset_div2 = 0;
    sp.slice_beta_offset_div2 = 0;
    if (vapp->pic_fields.bits.deblocking_filter_control_present_flag) {
        sp.disable_deblocking_filter_idc = rbsp_get_uev(st);
        if (1 != sp.disable_deblocking_filter_idc) {
            sp.slice_alpha_c0_offset_div2 = rbsp_get_sev(st);
            sp.slice_beta_offset_div2 = rbsp_get_sev(st);
        }
    }

    if (vapp->num_slice_groups_minus1 > 0 && vapp->slice_group_map_type >= 3 &&
        vapp->slice_group_map_type <= 5)
    {
        NOT_IMPLEMENTED("don't know what length to consume\n");
    }

    fprintf(stderr, "num_ref_idx_l0_active_minus1 = %d\n", sp.num_ref_idx_l0_active_minus1);
    fprintf(stderr, "num_ref_idx_l1_active_minus1 = %d\n", sp.num_ref_idx_l1_active_minus1);

    do_fill_va_slice_parameter_buffer(&sp, vapp, vasp, st->bits_eaten);
}


static
void
parse_ref_pic_list_modification(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                                struct slice_parameters *sp)
{
    const int MaxFrameNum = 1 << (vapp->seq_fields.bits.log2_max_frame_num_minus4 + 4);
    const int MaxPicNum = (vapp->pic_fields.bits.field_pic_flag) ? 2*MaxFrameNum : MaxFrameNum;

    if (2 != sp->slice_type && 4 != sp->slice_type) {
        int ref_pic_list_modification_flag_l0 = rbsp_get_u(st, 1);
        if (ref_pic_list_modification_flag_l0) {
            int modification_of_pic_nums_idc;
            int refIdxL0 = 0;
            unsigned int picNumL0 = vapp->frame_num;
            do {
                modification_of_pic_nums_idc = rbsp_get_uev(st);
                if (modification_of_pic_nums_idc < 2) {
                    int abs_diff_pic_num_minus1 = rbsp_get_uev(st);
                    if (0 == modification_of_pic_nums_idc) {
                        picNumL0 -= (abs_diff_pic_num_minus1 + 1);
                    } else { // 1 == modification_of_pic_nums_idc
                        picNumL0 += (abs_diff_pic_num_minus1 + 1);
                    }

                    // wrap picNumL0
                    picNumL0 &= (MaxPicNum - 1);

                    // there is no need to subtract MaxPicNum as in (8-36) in 8.2.4.3.1
                    // because frame_num already wrapped

                    int j;
                    for (j = 0; j < vapp->num_ref_frames; j ++) {
                        if (vapp->ReferenceFrames[j].flags & VA_PICTURE_H264_INVALID)
                            continue;
                        if (vapp->ReferenceFrames[j].frame_idx == picNumL0 &&
                            (vapp->ReferenceFrames[j].flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE))
                                break;
                    }
                    assert (j < vapp->num_ref_frames);
                    VAPictureH264 swp = vapp->ReferenceFrames[j];
                    for (int k = sp->num_ref_idx_l0_active_minus1; k > refIdxL0; k --)
                        sp->RefPicList0[k] = sp->RefPicList0[k-1];
                    sp->RefPicList0[refIdxL0 ++] = swp;
                    j = refIdxL0;
                    for (int k = refIdxL0; k <= sp->num_ref_idx_l0_active_minus1 + 1; k ++) {
                        if (sp->RefPicList0[k].frame_idx != picNumL0 &&
                            (sp->RefPicList0[k].flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE))
                                sp->RefPicList0[j++] = sp->RefPicList0[k];
                    }

                } else if (2 == modification_of_pic_nums_idc) {
                    NOT_IMPLEMENTED("long");
                    fprintf(stderr, "long_term_pic_num = %d\n", rbsp_get_uev(st));
                }

            } while (modification_of_pic_nums_idc != 3);

        }
    }

    if (1 == sp->slice_type) {
        int ref_pic_list_modification_flag_l1 = rbsp_get_u(st, 1);
        if (ref_pic_list_modification_flag_l1) {
            NOT_IMPLEMENTED("ref pic list modification 1"); // TODO: implement this
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
fill_default_pred_weight_table(const VAPictureParameterBufferH264 *vapp,
                               struct slice_parameters *sp)
{
    (void)vapp; // TODO: do I need this param?
    sp->luma_log2_weight_denom = 0;
    sp->chroma_log2_weight_denom = 0;
    sp->luma_weight_l0_flag = 0;
    sp->luma_weight_l1_flag = 0;
    sp->chroma_weight_l0_flag = 0;
    sp->chroma_weight_l1_flag = 0;
    for (int k = 0; k < sp->num_ref_idx_l0_active_minus1 + 1; k ++) {
        sp->luma_weight_l0[k] = 1;
        sp->luma_offset_l0[k] = 0;
        sp->chroma_weight_l0[k][0] = sp->chroma_weight_l0[k][1] = 1;
        sp->chroma_offset_l0[k][0] = sp->chroma_offset_l0[k][1] = 0;
    }
    for (int k = 0; k < sp->num_ref_idx_l1_active_minus1 + 1; k ++) {
        sp->luma_weight_l1[k] = 1;
        sp->luma_offset_l1[k] = 0;
        sp->chroma_weight_l1[k][0] = sp->chroma_weight_l1[k][1] = 1;
        sp->chroma_offset_l1[k][0] = sp->chroma_offset_l1[k][1] = 0;
    }
}

static
void
parse_pred_weight_table(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                        const int ChromaArrayType, struct slice_parameters *sp)
{
    (void)vapp; // TODO: do I need this param?
    sp->luma_log2_weight_denom = rbsp_get_uev(st);
    if (0 != ChromaArrayType)
        sp->chroma_log2_weight_denom = rbsp_get_uev(st);

    for (int k = 0; k <= sp->num_ref_idx_l0_active_minus1; k ++) {
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
        for (int k = 0; k <= sp->num_ref_idx_l1_active_minus1; k ++) {
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

static
void
parse_dec_ref_pic_marking(rbsp_state_t *st, struct slice_parameters *sp)
{
    if (NAL_IDR_SLICE == sp->nal_unit_type) {
        sp->no_output_of_prior_pics_flag = rbsp_get_u(st, 1);
        sp->long_term_reference_flag = rbsp_get_u(st, 1);
    } else {
        int adaptive_ref_pic_marking_mode_flag = rbsp_get_u(st, 1);
        if (adaptive_ref_pic_marking_mode_flag) {
            NOT_IMPLEMENTED("adaptive ref pic marking");    // TODO: implement
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
}
