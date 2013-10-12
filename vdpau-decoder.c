/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#include <stdlib.h>
#include <string.h>
#include "ctx-stack.h"
#include "h264-parse.h"
#include "vdpau-trace.h"
#include "vdpau-soft.h"


VdpStatus
softVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                     uint32_t max_references, VdpDecoder *decoder)
{
    VdpStatus err_code;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;
    if (!deviceData->va_available) {
        err_code = VDP_STATUS_INVALID_DECODER_PROFILE;
        goto quit;
    }
    VADisplay va_dpy = deviceData->va_dpy;

    VdpDecoderData *data = calloc(1, sizeof(VdpDecoderData));
    if (NULL == data) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    data->type = HANDLETYPE_DECODER;
    data->device = deviceData;
    data->profile = profile;
    data->width = width;
    data->height = height;
    data->max_references = max_references;
    data->next_surface_idx = 0;

    VAProfile va_profile;
    VAStatus status;
    int final_try = 0;
    VdpDecoderProfile next_profile = profile;

    // Try to create decoder for asked profile. On failure try to create more advanced one
    while (! final_try) {
        profile = next_profile;
        switch (profile) {
        case VDP_DECODER_PROFILE_H264_BASELINE:
            va_profile = VAProfileH264Baseline;
            data->num_render_targets = NUM_RENDER_TARGETS_H264;
            next_profile = VDP_DECODER_PROFILE_H264_MAIN;
            break;
        case VDP_DECODER_PROFILE_H264_MAIN:
            va_profile = VAProfileH264Main;
            data->num_render_targets = NUM_RENDER_TARGETS_H264;
            next_profile = VDP_DECODER_PROFILE_H264_HIGH;
            break;
        case VDP_DECODER_PROFILE_H264_HIGH:
            va_profile = VAProfileH264High;
            data->num_render_targets = NUM_RENDER_TARGETS_H264;
            // there is no more advanced profile, so it's final try
            final_try = 1;
            break;
        default:
            traceError("error (softVdpDecoderCreate): decoder %s not implemented\n",
                       reverse_decoder_profile(profile));
            err_code = VDP_STATUS_INVALID_DECODER_PROFILE;
            goto quit_free_data;
        }

        status = vaCreateConfig(va_dpy, va_profile, VAEntrypointVLD, NULL, 0, &data->config_id);
        if (VA_STATUS_SUCCESS == status)        // break loop if decoder created
            break;
    }

    if (VA_STATUS_SUCCESS != status) {
        err_code = VDP_STATUS_ERROR;
        goto quit_free_data;
    }

    // Create surfaces. All video surfaces created here, rather than in VdpVideoSurfaceCreate.
    // VAAPI requires surfaces to be bound with context on its creation time, while VDPAU allows
    // to do it later. So here is a trick: VDP video surfaces get their va_surf dynamically in
    // DecoderRender.

    // TODO: check format of surfaces created
#if VA_CHECK_VERSION(0, 34, 0)
    status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, width, height,
        data->render_targets, data->num_render_targets, NULL, 0);
#else
    status = vaCreateSurfaces(va_dpy, width, height, VA_RT_FORMAT_YUV420,
        data->num_render_targets, data->render_targets);
#endif
    if (VA_STATUS_SUCCESS != status) {
        err_code = VDP_STATUS_ERROR;
        goto quit_free_data;
    }

    status = vaCreateContext(va_dpy, data->config_id, width, height, VA_PROGRESSIVE,
        data->render_targets, data->num_render_targets, &data->context_id);
    if (VA_STATUS_SUCCESS != status) {
        err_code = VDP_STATUS_ERROR;
        goto quit_free_data;
    }

    deviceData->refcount ++;
    *decoder = handle_insert(data);

    err_code = VDP_STATUS_OK;
    goto quit;

quit_free_data:
    free(data);
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
softVdpDecoderDestroy(VdpDecoder decoder)
{
    VdpDecoderData *decoderData = handle_acquire(decoder, HANDLETYPE_DECODER);
    if (NULL == decoderData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = decoderData->device;

    if (deviceData->va_available) {
        VADisplay va_dpy = deviceData->va_dpy;
        vaDestroySurfaces(va_dpy, decoderData->render_targets, decoderData->num_render_targets);
        vaDestroyContext(va_dpy, decoderData->context_id);
        vaDestroyConfig(va_dpy, decoderData->config_id);
    }

    handle_expunge(decoder);
    deviceData->refcount --;
    free(decoderData);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                            uint32_t *width, uint32_t *height)
{
    VdpStatus err_code;
    VdpDecoderData *decoderData = handle_acquire(decoder, HANDLETYPE_DECODER);
    if (NULL == decoderData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == profile || NULL == width || NULL == height) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }

    *profile = decoderData->profile;
    *width   = decoderData->width;
    *height  = decoderData->height;

    err_code = VDP_STATUS_OK;
quit:
    handle_release(decoder);
    return err_code;
}

static
VdpStatus
h264_translate_reference_frames(VdpVideoSurfaceData *dstSurfData, VdpDecoderData *decoderData,
                                VAPictureParameterBufferH264 *pic_param,
                                const VdpPictureInfoH264 *vdppi)
{
    // take new VA surface from buffer if needed
    if (VA_INVALID_SURFACE == dstSurfData->va_surf) {
        if (decoderData->next_surface_idx >= decoderData->num_render_targets)
            return VDP_STATUS_RESOURCES;
        dstSurfData->va_surf = decoderData->render_targets[decoderData->next_surface_idx];
        decoderData->next_surface_idx ++;
    }

    // current frame
    pic_param->CurrPic.picture_id   = dstSurfData->va_surf;
    pic_param->CurrPic.frame_idx    = vdppi->frame_num;
    pic_param->CurrPic.flags  = vdppi->is_reference ? VA_PICTURE_H264_SHORT_TERM_REFERENCE : 0;
    if (vdppi->field_pic_flag) {
        pic_param->CurrPic.flags |=
            vdppi->bottom_field_flag ? VA_PICTURE_H264_BOTTOM_FIELD : VA_PICTURE_H264_TOP_FIELD;
    }

    pic_param->CurrPic.TopFieldOrderCnt     = vdppi->field_order_cnt[0];
    pic_param->CurrPic.BottomFieldOrderCnt  = vdppi->field_order_cnt[1];

    // mark all pictures invalid preliminary
    for (int k = 0; k < 16; k ++)
        reset_va_picture_h264(&pic_param->ReferenceFrames[k]);

    // reference frames
    for (int k = 0; k < vdppi->num_ref_frames; k ++) {
        if (VDP_INVALID_HANDLE == vdppi->referenceFrames[k].surface) {
            reset_va_picture_h264(&pic_param->ReferenceFrames[k]);
            continue;
        }

        VdpReferenceFrameH264 const *vdp_ref = &(vdppi->referenceFrames[k]);
        VdpVideoSurfaceData *vdpSurfData =
            handle_acquire(vdp_ref->surface, HANDLETYPE_VIDEO_SURFACE);
        VAPictureH264 *va_ref = &(pic_param->ReferenceFrames[k]);
        if (NULL == vdpSurfData) {
            traceError("error (h264_translate_reference_frames): NULL == vdpSurfData");
            return VDP_STATUS_ERROR;
        }

        // take new VA surface from buffer if needed
        if (VA_INVALID_SURFACE == vdpSurfData->va_surf) {
            if (decoderData->next_surface_idx >= decoderData->num_render_targets)
                return VDP_STATUS_RESOURCES;
            vdpSurfData->va_surf = decoderData->render_targets[decoderData->next_surface_idx];
            decoderData->next_surface_idx ++;
        }

        va_ref->picture_id = vdpSurfData->va_surf;
        va_ref->frame_idx = vdp_ref->frame_idx;
        va_ref->flags = vdp_ref->is_long_term ? VA_PICTURE_H264_LONG_TERM_REFERENCE
                                              : VA_PICTURE_H264_SHORT_TERM_REFERENCE;

        if (vdp_ref->top_is_reference && vdp_ref->bottom_is_reference) {
            // Full frame. This block intentionally left blank. No flags set.
        } else {
            if (vdp_ref->top_is_reference)
                va_ref->flags |= VA_PICTURE_H264_TOP_FIELD;
            else
                va_ref->flags |= VA_PICTURE_H264_BOTTOM_FIELD;
        }

        va_ref->TopFieldOrderCnt    = vdp_ref->field_order_cnt[0];
        va_ref->BottomFieldOrderCnt = vdp_ref->field_order_cnt[1];
        handle_release(vdp_ref->surface);
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                                uint32_t *max_level, uint32_t *max_macroblocks,
                                uint32_t *max_width, uint32_t *max_height)
{
    VdpStatus err_code;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == is_supported || NULL == max_level || NULL == max_macroblocks ||
        NULL == max_width || NULL == max_height)
    {
        err_code = VDP_STATUS_INVALID_POINTER;
        goto quit;
    }

    *max_level = 0;
    *max_macroblocks = 0;
    *max_width = 0;
    *max_height = 0;

    if (! deviceData->va_available) {
        *is_supported = 0;
        err_code = VDP_STATUS_OK;
        goto quit;
    }

    VAProfile *va_profile_list = malloc(sizeof(VAProfile) * vaMaxNumProfiles(deviceData->va_dpy));
    if (NULL == va_profile_list) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    int num_profiles;
    VAStatus status = vaQueryConfigProfiles(deviceData->va_dpy, va_profile_list, &num_profiles);
    if (VA_STATUS_SUCCESS != status) {
        free(va_profile_list);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    struct {
        int mpeg2_simple;
        int mpeg2_main;
        int h264_baseline;
        int h264_main;
        int h264_high;
        int vc1_simple;
        int vc1_main;
        int vc1_advanced;
    } available_profiles = {0, 0, 0, 0, 0, 0, 0, 0};

    for (int k = 0; k < num_profiles; k ++) {
        switch (va_profile_list[k]) {
        case VAProfileMPEG2Main:
            available_profiles.mpeg2_main = 0;
            /* fall through */
        case VAProfileMPEG2Simple:
            available_profiles.mpeg2_simple = 0;
            break;

        case VAProfileH264High:
            available_profiles.h264_high = 1;
            /* fall through */
        case VAProfileH264Main:
            available_profiles.h264_main = 1;
            /* fall through */
        case VAProfileH264Baseline:
            available_profiles.h264_baseline = 1;
            /* fall though */
        case VAProfileH264ConstrainedBaseline:
            break;

        case VAProfileVC1Advanced:
            available_profiles.vc1_advanced = 0;
            /* fall though */
        case VAProfileVC1Main:
            available_profiles.vc1_main = 0;
            /* fall though */
        case VAProfileVC1Simple:
            available_profiles.vc1_simple = 0;
            break;

        // unhandled profiles
        case VAProfileH263Baseline:
        case VAProfileJPEGBaseline:
        default:
            // do nothing
            break;
        }
    }
    free(va_profile_list);

    *is_supported = 0;
    // TODO: How to determine max width and height width libva?
    *max_width = 2048;
    *max_height = 2048;
    *max_macroblocks = 16384;
    switch (profile) {
    case VDP_DECODER_PROFILE_MPEG2_SIMPLE:
        *is_supported = available_profiles.mpeg2_simple;
        *max_level = VDP_DECODER_LEVEL_MPEG2_HL;
        break;
    case VDP_DECODER_PROFILE_MPEG2_MAIN:
        *is_supported = available_profiles.mpeg2_main;
        *max_level = VDP_DECODER_LEVEL_MPEG2_HL;
        break;

    case VDP_DECODER_PROFILE_H264_BASELINE:
        *is_supported = available_profiles.h264_baseline;
        // TODO: Do underlying libva really support 5.1?
        *max_level = VDP_DECODER_LEVEL_H264_5_1;
        break;
    case VDP_DECODER_PROFILE_H264_MAIN:
        *is_supported = available_profiles.h264_main;
        *max_level = VDP_DECODER_LEVEL_H264_5_1;
        break;
    case VDP_DECODER_PROFILE_H264_HIGH:
        *is_supported = available_profiles.h264_high;
        *max_level = VDP_DECODER_LEVEL_H264_5_1;
        break;

    case VDP_DECODER_PROFILE_VC1_SIMPLE:
        *is_supported = available_profiles.vc1_simple;
        *max_level = VDP_DECODER_LEVEL_VC1_SIMPLE_MEDIUM;
        break;
    case VDP_DECODER_PROFILE_VC1_MAIN:
        *is_supported = available_profiles.vc1_main;
        *max_level = VDP_DECODER_LEVEL_VC1_MAIN_HIGH;
        break;
    case VDP_DECODER_PROFILE_VC1_ADVANCED:
        *is_supported = available_profiles.vc1_advanced;
        *max_level = VDP_DECODER_LEVEL_VC1_ADVANCED_L4;
        break;

    // unsupported
    case VDP_DECODER_PROFILE_MPEG1:
    case VDP_DECODER_PROFILE_MPEG4_PART2_SP:
    case VDP_DECODER_PROFILE_MPEG4_PART2_ASP:
    case VDP_DECODER_PROFILE_DIVX4_QMOBILE:
    case VDP_DECODER_PROFILE_DIVX4_MOBILE:
    case VDP_DECODER_PROFILE_DIVX4_HOME_THEATER:
    case VDP_DECODER_PROFILE_DIVX4_HD_1080P:
    case VDP_DECODER_PROFILE_DIVX5_QMOBILE:
    case VDP_DECODER_PROFILE_DIVX5_MOBILE:
    case VDP_DECODER_PROFILE_DIVX5_HOME_THEATER:
    case VDP_DECODER_PROFILE_DIVX5_HD_1080P:
    default:
        break;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

static
void
h264_translate_pic_param(VAPictureParameterBufferH264 *pic_param, uint32_t width, uint32_t height,
                         const VdpPictureInfoH264 *vdppi, uint32_t level)
{
        pic_param->picture_width_in_mbs_minus1          = (width - 1) / 16;
        pic_param->picture_height_in_mbs_minus1         = (height - 1) / 16;
        pic_param->bit_depth_luma_minus8                = 0; // TODO: deal with more than 8 bits
        pic_param->bit_depth_chroma_minus8              = 0; // same for luma
        pic_param->num_ref_frames                       = vdppi->num_ref_frames;

#define SEQ_FIELDS(fieldname) pic_param->seq_fields.bits.fieldname
#define PIC_FIELDS(fieldname) pic_param->pic_fields.bits.fieldname

        SEQ_FIELDS(chroma_format_idc)                   = 1; // TODO: not only YUV420
        SEQ_FIELDS(residual_colour_transform_flag)      = 0;
        SEQ_FIELDS(gaps_in_frame_num_value_allowed_flag)= 0;
        SEQ_FIELDS(frame_mbs_only_flag)                 = vdppi->frame_mbs_only_flag;
        SEQ_FIELDS(mb_adaptive_frame_field_flag)        = vdppi->mb_adaptive_frame_field_flag;
        SEQ_FIELDS(direct_8x8_inference_flag)           = vdppi->direct_8x8_inference_flag;
        SEQ_FIELDS(MinLumaBiPredSize8x8)                = (level >= 31);
        SEQ_FIELDS(log2_max_frame_num_minus4)           = vdppi->log2_max_frame_num_minus4;
        SEQ_FIELDS(pic_order_cnt_type)                  = vdppi->pic_order_cnt_type;
        SEQ_FIELDS(log2_max_pic_order_cnt_lsb_minus4)   = vdppi->log2_max_pic_order_cnt_lsb_minus4;
        SEQ_FIELDS(delta_pic_order_always_zero_flag)    = vdppi->delta_pic_order_always_zero_flag;
        pic_param->num_slice_groups_minus1              = vdppi->slice_count - 1; // ???

        pic_param->slice_group_map_type                 = 0; // ???
        pic_param->slice_group_change_rate_minus1       = 0; // ???
        pic_param->pic_init_qp_minus26                  = vdppi->pic_init_qp_minus26;
        pic_param->pic_init_qs_minus26                  = 0; // ???
        pic_param->chroma_qp_index_offset               = vdppi->chroma_qp_index_offset;
        pic_param->second_chroma_qp_index_offset        = vdppi->second_chroma_qp_index_offset;
        PIC_FIELDS(entropy_coding_mode_flag)            = vdppi->entropy_coding_mode_flag;
        PIC_FIELDS(weighted_pred_flag)                  = vdppi->weighted_pred_flag;
        PIC_FIELDS(weighted_bipred_idc)                 = vdppi->weighted_bipred_idc;
        PIC_FIELDS(transform_8x8_mode_flag)             = vdppi->transform_8x8_mode_flag;
        PIC_FIELDS(field_pic_flag)                      = vdppi->field_pic_flag;
        PIC_FIELDS(constrained_intra_pred_flag)         = vdppi->constrained_intra_pred_flag;
        PIC_FIELDS(pic_order_present_flag)              = vdppi->pic_order_present_flag;
        PIC_FIELDS(deblocking_filter_control_present_flag) =
                                                    vdppi->deblocking_filter_control_present_flag;
        PIC_FIELDS(redundant_pic_cnt_present_flag)      = vdppi->redundant_pic_cnt_present_flag;
        PIC_FIELDS(reference_pic_flag)                  = vdppi->is_reference;
        pic_param->frame_num                            = vdppi->frame_num;
#undef SEQ_FIELDS
#undef PIC_FIELDS
}

static
void
h264_translate_iq_matrix(VAIQMatrixBufferH264 *iq_matrix, const VdpPictureInfoH264 *vdppi)
{
    for (int j = 0; j < 6; j ++)
        for (int k = 0; k < 16; k ++)
            iq_matrix->ScalingList4x4[j][k] = vdppi->scaling_lists_4x4[j][k];

    for (int j = 0; j < 2; j ++)
        for (int k = 0; k < 64; k ++)
            iq_matrix->ScalingList8x8[j][k] = vdppi->scaling_lists_8x8[j][k];
}

static
VdpStatus
softVdpDecoderRender_h264(VdpDecoderData *decoderData, VdpVideoSurfaceData *dstSurfData,
                          VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                          VdpBitstreamBuffer const *bitstream_buffers)
{
    VdpDeviceData *deviceData = decoderData->device;
    VADisplay va_dpy = deviceData->va_dpy;
    VAStatus status;
    VdpStatus vs, err_code;
    VdpPictureInfoH264 const *vdppi = (void *)picture_info;

    // TODO: figure out where to get level
    uint32_t level = 41;

    // preparing picture parameters and IQ matrix
    VABufferID pic_param_buf, iq_matrix_buf;
    VAPictureParameterBufferH264 pic_param;
    VAIQMatrixBufferH264 iq_matrix;

    vs = h264_translate_reference_frames(dstSurfData, decoderData, &pic_param, vdppi);
    if (VDP_STATUS_OK != vs) {
        if (VDP_STATUS_RESOURCES == vs) {
            traceError("error (softVdpDecoderRender): no surfaces left in buffer\n");
            err_code = VDP_STATUS_RESOURCES;
        } else {
            err_code = VDP_STATUS_ERROR;
        }
        goto quit;
    }

    h264_translate_pic_param(&pic_param, decoderData->width, decoderData->height, vdppi, level);
    h264_translate_iq_matrix(&iq_matrix, vdppi);

    glx_context_lock();
    status = vaCreateBuffer(va_dpy, decoderData->context_id, VAPictureParameterBufferType,
        sizeof(VAPictureParameterBufferH264), 1, &pic_param, &pic_param_buf);
    if (VA_STATUS_SUCCESS != status) {
        glx_context_unlock();
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    status = vaCreateBuffer(va_dpy, decoderData->context_id, VAIQMatrixBufferType,
        sizeof(VAIQMatrixBufferH264), 1, &iq_matrix, &iq_matrix_buf);
    if (VA_STATUS_SUCCESS != status) {
        glx_context_unlock();
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    // send data to decoding hardware
    status = vaBeginPicture(va_dpy, decoderData->context_id, dstSurfData->va_surf);
    if (VA_STATUS_SUCCESS != status) {
        glx_context_unlock();
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }
    status = vaRenderPicture(va_dpy, decoderData->context_id, &pic_param_buf, 1);
    if (VA_STATUS_SUCCESS != status) {
        glx_context_unlock();
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }
    status = vaRenderPicture(va_dpy, decoderData->context_id, &iq_matrix_buf, 1);
    if (VA_STATUS_SUCCESS != status) {
        glx_context_unlock();
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    vaDestroyBuffer(va_dpy, pic_param_buf);
    vaDestroyBuffer(va_dpy, iq_matrix_buf);
    glx_context_unlock();

    // merge bitstream buffers
    int total_bitstream_bytes = 0;
    for (unsigned int k = 0; k < bitstream_buffer_count; k ++)
        total_bitstream_bytes += bitstream_buffers[k].bitstream_bytes;

    uint8_t *merged_bitstream = malloc(total_bitstream_bytes);
    if (NULL == merged_bitstream) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    do {
        unsigned char *ptr = merged_bitstream;
        for (unsigned int k = 0; k < bitstream_buffer_count; k ++) {
            memcpy(ptr, bitstream_buffers[k].bitstream, bitstream_buffers[k].bitstream_bytes);
            ptr += bitstream_buffers[k].bitstream_bytes;
        }
    } while(0);

    // Slice parameters

    // All slice data have been merged into one continuous buffer. But we must supply
    // slices one by one to the hardware decoder, so we need to delimit them. VDPAU
    // requires bitstream buffers to include slice start code (0x00 0x00 0x01). Those
    // will be used to calculate offsets and sizes of slice data in code below.

    rbsp_state_t st_g;      // reference, global state
    rbsp_attach_buffer(&st_g, merged_bitstream, total_bitstream_bytes);
    int nal_offset = rbsp_navigate_to_nal_unit(&st_g);
    if (nal_offset < 0) {
        traceError("error (softVdpDecoderRender): no NAL header\n");
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    do {
        VASliceParameterBufferH264 sp_h264;
        memset(&sp_h264, 0, sizeof(VASliceParameterBufferH264));

        // make a copy of global rbsp state for using in slice header parser
        rbsp_state_t st = rbsp_copy_state(&st_g);
        rbsp_reset_bit_counter(&st);
        int nal_offset_next = rbsp_navigate_to_nal_unit(&st_g);

        // calculate end of current slice. Note (-3). It's slice start code length.
        const unsigned int end_pos = (nal_offset_next > 0) ? (nal_offset_next - 3)
                                                           : total_bitstream_bytes;
        sp_h264.slice_data_size     = end_pos - nal_offset;
        sp_h264.slice_data_offset   = 0;
        sp_h264.slice_data_flag     = VA_SLICE_DATA_FLAG_ALL;

        // TODO: this may be not entirely true for YUV444
        // but if we limiting to YUV420, that's ok
        int ChromaArrayType = pic_param.seq_fields.bits.chroma_format_idc;

        // parse slice header and use its data to fill slice parameter buffer
        parse_slice_header(&st, &pic_param, ChromaArrayType, vdppi->num_ref_idx_l0_active_minus1,
                           vdppi->num_ref_idx_l1_active_minus1, &sp_h264);

        VABufferID slice_parameters_buf;
        glx_context_lock();
        status = vaCreateBuffer(va_dpy, decoderData->context_id, VASliceParameterBufferType,
            sizeof(VASliceParameterBufferH264), 1, &sp_h264, &slice_parameters_buf);
        if (VA_STATUS_SUCCESS != status) {
            glx_context_unlock();
            err_code = VDP_STATUS_ERROR;
            goto quit;
        }
        status = vaRenderPicture(va_dpy, decoderData->context_id, &slice_parameters_buf, 1);
        if (VA_STATUS_SUCCESS != status) {
            glx_context_unlock();
            err_code = VDP_STATUS_ERROR;
            goto quit;
        }

        VABufferID slice_buf;
        status = vaCreateBuffer(va_dpy, decoderData->context_id, VASliceDataBufferType,
            sp_h264.slice_data_size, 1, merged_bitstream + nal_offset, &slice_buf);
        if (VA_STATUS_SUCCESS != status) {
            glx_context_unlock();
            err_code = VDP_STATUS_ERROR;
            goto quit;
        }

        status = vaRenderPicture(va_dpy, decoderData->context_id, &slice_buf, 1);
        if (VA_STATUS_SUCCESS != status) {
            glx_context_unlock();
            err_code = VDP_STATUS_ERROR;
            goto quit;
        }

        vaDestroyBuffer(va_dpy, slice_parameters_buf);
        vaDestroyBuffer(va_dpy, slice_buf);
        glx_context_unlock();

        if (nal_offset_next < 0)        // nal_offset_next equals -1 when there is no slice
            break;                      // start code found. Thus that was the final slice.
        nal_offset = nal_offset_next;
    } while (1);

    glx_context_lock();
    status = vaEndPicture(va_dpy, decoderData->context_id);
    glx_context_unlock();
    if (VA_STATUS_SUCCESS != status) {
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    free(merged_bitstream);
    err_code = VDP_STATUS_OK;
quit:
    return err_code;
}

VdpStatus
softVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                     VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                     VdpBitstreamBuffer const *bitstream_buffers)
{
    VdpStatus err_code;
    VdpDecoderData *decoderData = handle_acquire(decoder, HANDLETYPE_DECODER);
    VdpVideoSurfaceData *dstSurfData = handle_acquire(target, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == decoderData || NULL == dstSurfData) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }

    if (VDP_DECODER_PROFILE_H264_BASELINE == decoderData->profile ||
        VDP_DECODER_PROFILE_H264_MAIN ==     decoderData->profile ||
        VDP_DECODER_PROFILE_H264_HIGH ==     decoderData->profile)
    {
        // TODO: check exit code
        softVdpDecoderRender_h264(decoderData, dstSurfData, picture_info, bitstream_buffer_count,
                                  bitstream_buffers);
    } else {
        traceError("error (softVdpDecoderRender): no implementation for profile %s\n",
                   reverse_decoder_profile(decoderData->profile));
        err_code = VDP_STATUS_NO_IMPLEMENTATION;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(decoder);
    handle_release(target);
    return err_code;
}
