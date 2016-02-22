/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "h264-parse.h"

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
parse_pred_weight_table(rbsp_state_t *st, const int ChromaArrayType, struct slice_parameters *sp);

static
void
parse_dec_ref_pic_marking(rbsp_state_t *st, struct slice_parameters *sp);

static
void
do_fill_va_slice_parameter_buffer(struct slice_parameters const * const sp,
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
    vasp->chroma_log2_weight_denom = sp->chroma_log2_weight_denom;

    vasp->luma_weight_l0_flag = sp->luma_weight_l0_flag;
    for (int k = 0; k < 32; k ++)
        vasp->luma_weight_l0[k] = sp->luma_weight_l0[k];

    for (int k = 0; k < 32; k ++)
        vasp->luma_offset_l0[k] = sp->luma_offset_l0[k];

    vasp->chroma_weight_l0_flag = sp->chroma_weight_l0_flag;
    for (int k = 0; k < 32; k ++)
        vasp->chroma_weight_l0[k][0] = sp->chroma_weight_l0[k][0];

    for (int k = 0; k < 32; k ++)
        vasp->chroma_weight_l0[k][1] = sp->chroma_weight_l0[k][1];

    for (int k = 0; k < 32; k ++)
        vasp->chroma_offset_l0[k][0] = sp->chroma_offset_l0[k][0];

    for (int k = 0; k < 32; k ++)
        vasp->chroma_offset_l0[k][1] = sp->chroma_offset_l0[k][1];

    vasp->luma_weight_l1_flag = sp->luma_weight_l1_flag;
    for (int k = 0; k < 32; k ++)
        vasp->luma_weight_l1[k] = sp->luma_weight_l1[k];

    for (int k = 0; k < 32; k ++)
        vasp->luma_offset_l1[k] = sp->luma_offset_l1[k];

    vasp->chroma_weight_l1_flag = sp->chroma_weight_l1_flag;
    for (int k = 0; k < 32; k ++)
        vasp->chroma_weight_l1[k][0] = sp->chroma_weight_l1[k][0];

    for (int k = 0; k < 32; k ++)
        vasp->chroma_weight_l1[k][1] = sp->chroma_weight_l1[k][1];

    for (int k = 0; k < 32; k ++)
        vasp->chroma_offset_l1[k][0] = sp->chroma_offset_l1[k][0];

    for (int k = 0; k < 32; k ++)
        vasp->chroma_offset_l1[k][1] = sp->chroma_offset_l1[k][1];
}

void
reset_va_picture_h264(VAPictureH264 *p)
{
    p->picture_id       = VA_INVALID_SURFACE;
    p->frame_idx        = 0;
    p->flags            = VA_PICTURE_H264_INVALID;
    p->TopFieldOrderCnt = 0;
    p->BottomFieldOrderCnt  = 0;
}

struct comparison_fcn1_context {
    int                  descending;
    int                  what;
    const VAPictureH264 *ReferenceFrames;
};

static
gint
comparison_fcn_1(gconstpointer p1, gconstpointer p2, gpointer context)
{
    const int idx_1 = *(const int *)p1;
    const int idx_2 = *(const int *)p2;

    struct comparison_fcn1_context *ctx = context;

    int value1 = 0, value2 = 0;
    switch (ctx->what) {
    case 1:         // top field
        value1 = ctx->ReferenceFrames[idx_1].TopFieldOrderCnt;
        value2 = ctx->ReferenceFrames[idx_2].TopFieldOrderCnt;
        break;
    case 2:         // bottom field
        value1 = ctx->ReferenceFrames[idx_1].BottomFieldOrderCnt;
        value2 = ctx->ReferenceFrames[idx_2].BottomFieldOrderCnt;
        break;
    case 3:         // frame_idx
        value1 = ctx->ReferenceFrames[idx_1].frame_idx;
        value2 = ctx->ReferenceFrames[idx_2].frame_idx;
        break;
    default:
        assert(0 && "wrong what field");
    }

    int result;
    if (value1 < value2)
        result = -1;
    else if (value1 > value2)
        result = 1;
    else
        result = 0;

    if (ctx->descending) return -result;
    return result;
}

static
void
fill_ref_pic_list(struct slice_parameters *sp, const VAPictureParameterBufferH264 *vapp)
{
    int idcs_asc[32], idcs_desc[32];
    struct comparison_fcn1_context ctx;

    if (sp->slice_type == SLICE_TYPE_I || sp->slice_type == SLICE_TYPE_SI)
        return;

    ctx.ReferenceFrames = vapp->ReferenceFrames;

    int frame_count = 0;
    for (int k = 0; k < vapp->num_ref_frames; k ++) {
        if (vapp->ReferenceFrames[k].flags & VA_PICTURE_H264_INVALID)
            continue;
        sp->RefPicList0[frame_count] = vapp->ReferenceFrames[k];
        idcs_asc[frame_count] = idcs_desc[frame_count] = k;
        frame_count ++;
    }

    if (sp->slice_type == SLICE_TYPE_P || sp->slice_type == SLICE_TYPE_SP) {
        // TODO: implement interlaced P slices
        ctx.what = 1;
        ctx.descending = 0;
        g_qsort_with_data(idcs_asc, frame_count, sizeof(idcs_asc[0]), &comparison_fcn_1, &ctx);
        ctx.descending = 1;
        g_qsort_with_data(idcs_desc, frame_count, sizeof(idcs_desc[0]), &comparison_fcn_1, &ctx);

        int ptr = 0;
        for (int k = 0; k < frame_count; k ++)
            if (vapp->ReferenceFrames[idcs_desc[k]].flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
                sp->RefPicList0[ptr++] = vapp->ReferenceFrames[idcs_desc[k]];

        for (int k = 0; k < frame_count; k ++)
            if (vapp->ReferenceFrames[idcs_asc[k]].flags & VA_PICTURE_H264_LONG_TERM_REFERENCE)
                sp->RefPicList0[ptr++] = vapp->ReferenceFrames[idcs_asc[k]];

    } else if (sp->slice_type == SLICE_TYPE_B && !vapp->pic_fields.bits.field_pic_flag) {
        ctx.what = 1;
        ctx.descending = 0;
        g_qsort_with_data(idcs_asc, frame_count, sizeof(idcs_asc[0]), &comparison_fcn_1, &ctx);
        ctx.descending = 1;
        g_qsort_with_data(idcs_desc, frame_count, sizeof(idcs_desc[0]), &comparison_fcn_1, &ctx);

        int ptr0 = 0;
        int ptr1 = 0;
        for (int k = 0; k < frame_count; k ++) {
            const VAPictureH264 *rf = &vapp->ReferenceFrames[idcs_desc[k]];
            if (rf->flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
                if (rf->TopFieldOrderCnt < vapp->CurrPic.TopFieldOrderCnt)
                    sp->RefPicList0[ptr0++] = *rf;

            rf = &vapp->ReferenceFrames[idcs_asc[k]];
            if (rf->flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
                if (rf->TopFieldOrderCnt >= vapp->CurrPic.TopFieldOrderCnt)
                    sp->RefPicList1[ptr1++] = *rf;
        }
        for (int k = 0; k < frame_count; k ++) {
            const VAPictureH264 *rf = &vapp->ReferenceFrames[idcs_asc[k]];
            if (rf->flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
                if (rf->TopFieldOrderCnt >= vapp->CurrPic.TopFieldOrderCnt)
                    sp->RefPicList0[ptr0++] = *rf;

            rf = &vapp->ReferenceFrames[idcs_desc[k]];
            if (rf->flags & VA_PICTURE_H264_SHORT_TERM_REFERENCE)
                if (rf->TopFieldOrderCnt < vapp->CurrPic.TopFieldOrderCnt)
                    sp->RefPicList1[ptr1++] = *rf;
        }
        for (int k = 0; k < frame_count; k ++) {
            const VAPictureH264 *rf = &vapp->ReferenceFrames[idcs_asc[k]];
            if (rf->flags & VA_PICTURE_H264_LONG_TERM_REFERENCE) {
                sp->RefPicList0[ptr0++] = *rf;
                sp->RefPicList1[ptr1++] = *rf;
            }
        }
    } else {
        // TODO: implement interlaced B slices
        assert(0 && "not implemeted: interlaced SLICE_TYPE_B sorting");
    }
}

void
parse_slice_header(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                   const int ChromaArrayType, unsigned int p_num_ref_idx_l0_active_minus1,
                   unsigned int p_num_ref_idx_l1_active_minus1, VASliceParameterBufferH264 *vasp)
{
    struct slice_parameters sp = { 0 };

    for (int k = 0; k < 32; k ++) {
        reset_va_picture_h264(&sp.RefPicList0[k]);
        reset_va_picture_h264(&sp.RefPicList1[k]);
    }

    rbsp_get_u(st, 1); // forbidden_zero_bit
    sp.nal_ref_idc = rbsp_get_u(st, 2);
    sp.nal_unit_type = rbsp_get_u(st, 5);

    if (sp.nal_unit_type == 14 || sp.nal_unit_type == 20) {
        NOT_IMPLEMENTED("nal unit types 14 and 20");
    }

    sp.first_mb_in_slice = rbsp_get_uev(st);
    sp.slice_type = rbsp_get_uev(st);

    if (sp.slice_type > 4)
        sp.slice_type -= 5;     // wrap 5-9 to 0-4

    // as now we know slice_type, time to fill RefPicListX
    fill_ref_pic_list(&sp, vapp);

    sp.pic_parameter_set_id = rbsp_get_uev(st);

    // TODO: separate_colour_plane_flag is 0 for all but YUV444. Now ok, but should detect properly.
    // See 7.3.3

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
    if (sp.nal_unit_type == NAL_IDR_SLICE)    // IDR picture
        sp.idr_pic_id = rbsp_get_uev(st);

    sp.pic_order_cnt_lsb = 0;
    sp.delta_pic_order_cnt_bottom = 0;
    if (vapp->seq_fields.bits.pic_order_cnt_type == 0) {
        sp.pic_order_cnt_lsb =
            rbsp_get_u(st, vapp->seq_fields.bits.log2_max_pic_order_cnt_lsb_minus4 + 4);
        if (vapp->pic_fields.bits.pic_order_present_flag &&
            !vapp->pic_fields.bits.field_pic_flag)
        {
            sp.delta_pic_order_cnt_bottom = rbsp_get_sev(st);
        }
    }

    sp.delta_pic_order_cnt[0] = 0;
    sp.delta_pic_order_cnt[1] = 0;
    if (vapp->seq_fields.bits.pic_order_cnt_type == 1 &&
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
    if (sp.slice_type == SLICE_TYPE_B)
        sp.direct_spatial_mv_pred_flag = rbsp_get_u(st, 1);

    sp.num_ref_idx_active_override_flag = 0;
    sp.num_ref_idx_l0_active_minus1 = 0;
    sp.num_ref_idx_l1_active_minus1 = 0;
    if (sp.slice_type == SLICE_TYPE_P || sp.slice_type == SLICE_TYPE_SP ||
        sp.slice_type == SLICE_TYPE_B)
    {
        sp.num_ref_idx_l0_active_minus1 = p_num_ref_idx_l0_active_minus1;
        if (sp.slice_type != SLICE_TYPE_P)
            sp.num_ref_idx_l1_active_minus1 = p_num_ref_idx_l1_active_minus1;

        sp.num_ref_idx_active_override_flag = rbsp_get_u(st, 1);
        if (sp.num_ref_idx_active_override_flag) {
            sp.num_ref_idx_l0_active_minus1 = rbsp_get_uev(st);
            if (sp.slice_type == SLICE_TYPE_B)
                sp.num_ref_idx_l1_active_minus1 = rbsp_get_uev(st);
        }
    }

    if (sp.nal_unit_type == 20) {
        NOT_IMPLEMENTED("nal unit type 20");
    } else {
        parse_ref_pic_list_modification(st, vapp, &sp);
    }

    // here fields {luma,chroma}_weight_l{0,1}_flag differ from same-named flags from
    // H.264 recommendation. Each of those flags should be set to 1 if any of
    // weight tables differ from default
    sp.luma_weight_l0_flag = 0;
    sp.luma_weight_l1_flag = 0;
    sp.chroma_weight_l0_flag = 0;
    sp.chroma_weight_l1_flag = 0;
    if ((vapp->pic_fields.bits.weighted_pred_flag &&
        (sp.slice_type == SLICE_TYPE_P || sp.slice_type == SLICE_TYPE_SP)) ||
        (vapp->pic_fields.bits.weighted_bipred_idc == 1 && sp.slice_type == SLICE_TYPE_B))
    {
        parse_pred_weight_table(st, ChromaArrayType, &sp);
    }

    if (sp.nal_ref_idc != 0) {
        parse_dec_ref_pic_marking(st, &sp);
    }

    sp.cabac_init_idc = 0;
    if (vapp->pic_fields.bits.entropy_coding_mode_flag &&
        sp.slice_type != SLICE_TYPE_I && sp.slice_type != SLICE_TYPE_SI)
    {
            sp.cabac_init_idc = rbsp_get_uev(st);
    }

    sp.slice_qp_delta = rbsp_get_sev(st);

    sp.sp_for_switch_flag = 0;
    sp.slice_qs_delta = 0;
    if (sp.slice_type == SLICE_TYPE_SP || sp.slice_type == SLICE_TYPE_SI) {
        if (sp.slice_type == SLICE_TYPE_SP)
            sp.sp_for_switch_flag = rbsp_get_u(st, 1);
        sp.slice_qs_delta = rbsp_get_sev(st);
    }

    sp.disable_deblocking_filter_idc = 0;
    sp.slice_alpha_c0_offset_div2 = 0;
    sp.slice_beta_offset_div2 = 0;
    if (vapp->pic_fields.bits.deblocking_filter_control_present_flag) {
        sp.disable_deblocking_filter_idc = rbsp_get_uev(st);
        if (sp.disable_deblocking_filter_idc != 1) {
            sp.slice_alpha_c0_offset_div2 = rbsp_get_sev(st);
            sp.slice_beta_offset_div2 = rbsp_get_sev(st);
        }
    }

    if (vapp->num_slice_groups_minus1 > 0 && vapp->slice_group_map_type >= 3 &&
        vapp->slice_group_map_type <= 5)
    {
        NOT_IMPLEMENTED("don't know what length to consume\n");
    }

    do_fill_va_slice_parameter_buffer(&sp, vasp, st->bits_eaten);
}

static
void
parse_ref_pic_list_modification(rbsp_state_t *st, const VAPictureParameterBufferH264 *vapp,
                                struct slice_parameters *sp)
{
    const int MaxFrameNum = 1 << (vapp->seq_fields.bits.log2_max_frame_num_minus4 + 4);
    const int MaxPicNum = (vapp->pic_fields.bits.field_pic_flag) ? 2*MaxFrameNum : MaxFrameNum;

    if (sp->slice_type != SLICE_TYPE_I && sp->slice_type != SLICE_TYPE_SI) {
        int ref_pic_list_modification_flag_l0 = rbsp_get_u(st, 1);
        if (ref_pic_list_modification_flag_l0) {
            int modification_of_pic_nums_idc;
            int refIdxL0 = 0;
            unsigned int picNumL0 = vapp->frame_num;
            do {
                modification_of_pic_nums_idc = rbsp_get_uev(st);
                if (modification_of_pic_nums_idc < 2) {
                    int abs_diff_pic_num_minus1 = rbsp_get_uev(st);
                    if (modification_of_pic_nums_idc == 0) {
                        picNumL0 -= (abs_diff_pic_num_minus1 + 1);
                    } else { // modification_of_pic_nums_idc == 1
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
                        {
                            sp->RefPicList0[j++] = sp->RefPicList0[k];
                        }
                    }

                } else if (modification_of_pic_nums_idc == 2) {
                    NOT_IMPLEMENTED("long");
                    fprintf(stderr, "long_term_pic_num = %d\n", rbsp_get_uev(st));
                }

            } while (modification_of_pic_nums_idc != 3);

        }
    }

    if (sp->slice_type == SLICE_TYPE_B) {
        int ref_pic_list_modification_flag_l1 = rbsp_get_u(st, 1);
        if (ref_pic_list_modification_flag_l1) {
            NOT_IMPLEMENTED("ref pic list modification 1"); // TODO: implement this
            int modification_of_pic_nums_idc;
            do {
                modification_of_pic_nums_idc = rbsp_get_uev(st);
                if (modification_of_pic_nums_idc == 0 ||
                    modification_of_pic_nums_idc == 1)
                {
                    fprintf(stderr, "abs_diff_pic_num_minus1 = %d\n", rbsp_get_uev(st));
                } else if (modification_of_pic_nums_idc == 2) {
                    fprintf(stderr, "long_term_pic_num = %d\n", rbsp_get_uev(st));
                }
            } while (modification_of_pic_nums_idc != 3);
        }
    }
}

static
void
fill_default_pred_weight_table(struct slice_parameters *sp)
{
    const int default_luma_weight = (1 << sp->luma_log2_weight_denom);
    const int default_chroma_weight = (1 << sp->chroma_log2_weight_denom);
    for (int k = 0; k < sp->num_ref_idx_l0_active_minus1 + 1; k ++) {
        sp->luma_weight_l0[k] = default_luma_weight;
        sp->luma_offset_l0[k] = 0;
        sp->chroma_weight_l0[k][0] = sp->chroma_weight_l0[k][1] = default_chroma_weight;
        sp->chroma_offset_l0[k][0] = sp->chroma_offset_l0[k][1] = 0;
    }
    for (int k = 0; k < sp->num_ref_idx_l1_active_minus1 + 1; k ++) {
        sp->luma_weight_l1[k] = default_luma_weight;
        sp->luma_offset_l1[k] = 0;
        sp->chroma_weight_l1[k][0] = sp->chroma_weight_l1[k][1] = default_chroma_weight;
        sp->chroma_offset_l1[k][0] = sp->chroma_offset_l1[k][1] = 0;
    }
}

static
void
parse_pred_weight_table(rbsp_state_t *st, const int ChromaArrayType, struct slice_parameters *sp)
{
    sp->luma_log2_weight_denom = rbsp_get_uev(st);
    sp->chroma_log2_weight_denom = 0;
    if (ChromaArrayType != 0)
        sp->chroma_log2_weight_denom = rbsp_get_uev(st);

    fill_default_pred_weight_table(sp);

    const int default_luma_weight = (1 << sp->luma_log2_weight_denom);
    const int default_chroma_weight = (1 << sp->chroma_log2_weight_denom);

    for (int k = 0; k <= sp->num_ref_idx_l0_active_minus1; k ++) {
        int luma_weight_l0_flag = rbsp_get_u(st, 1);
        if (luma_weight_l0_flag) {
            sp->luma_weight_l0[k] = rbsp_get_sev(st);
            sp->luma_offset_l0[k] = rbsp_get_sev(st);
            if (default_luma_weight != sp->luma_weight_l0[k])
                sp->luma_weight_l0_flag = 1;
        }
        if (ChromaArrayType != 0) {
            int chroma_weight_l0_flag = rbsp_get_u(st, 1);
            if (chroma_weight_l0_flag) {
                for (int j = 0; j < 2; j ++) {
                    sp->chroma_weight_l0[k][j] = rbsp_get_sev(st);
                    sp->chroma_offset_l0[k][j] = rbsp_get_sev(st);
                    if (default_chroma_weight != sp->chroma_weight_l0[k][j])
                        sp->chroma_weight_l0_flag = 1;
                }
            }
        }
    }

    if (sp->slice_type == SLICE_TYPE_B) {
        for (int k = 0; k <= sp->num_ref_idx_l1_active_minus1; k ++) {
            int luma_weight_l1_flag = rbsp_get_u(st, 1);
            if (luma_weight_l1_flag) {
                sp->luma_weight_l1[k] = rbsp_get_sev(st);
                sp->luma_offset_l1[k] = rbsp_get_sev(st);
                if (default_luma_weight != sp->luma_weight_l1[k])
                    sp->luma_weight_l1_flag = 1;
            }
            if (ChromaArrayType != 0) {
                int chroma_weight_l1_flag = rbsp_get_u(st, 1);
                if (chroma_weight_l1_flag) {
                    for (int j = 0; j < 2; j ++) {
                        sp->chroma_weight_l1[k][j] = rbsp_get_sev(st);
                        sp->chroma_offset_l1[k][j] = rbsp_get_sev(st);
                        if (default_chroma_weight != sp->chroma_weight_l1[k][j])
                            sp->chroma_weight_l1_flag = 1;
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
    if (sp->nal_unit_type == NAL_IDR_SLICE) {
        sp->no_output_of_prior_pics_flag = rbsp_get_u(st, 1);
        sp->long_term_reference_flag = rbsp_get_u(st, 1);
    } else {
        int adaptive_ref_pic_marking_mode_flag = rbsp_get_u(st, 1);
        if (adaptive_ref_pic_marking_mode_flag) {
            // no need to do any action, just consume bits. All management should be done
            // on client side
            int memory_management_control_operation;
            do {
                memory_management_control_operation = rbsp_get_uev(st);
                if (memory_management_control_operation == 1 ||
                    memory_management_control_operation == 3)
                {
                    rbsp_get_uev(st);   // difference_of_pic_nums_minus1
                }
                if (memory_management_control_operation == 2) {
                    rbsp_get_uev(st);   // long_term_pic_num
                }
                if (memory_management_control_operation == 3 ||
                    memory_management_control_operation == 6)
                {
                    rbsp_get_uev(st);   // long_term_frame_idx
                }
                if (memory_management_control_operation == 4) {
                    rbsp_get_uev(st);   // max_long_term_frame_idx_plus1
                }
            } while (memory_management_control_operation != 0);
        }
    }
}
