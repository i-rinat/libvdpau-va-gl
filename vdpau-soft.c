/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define _XOPEN_SOURCE
#define GL_GLEXT_PROTOTYPES
#include <assert.h>
#include <glib.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <va/va.h>
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include "bitstream.h"
#include "ctx-stack.h"
#include "h264-parse.h"
#include "reverse-constant.h"
#include "handle-storage.h"
#include "vdpau-trace.h"
#include "vdpau-locking.h"
#include "watermark.h"
#include "globals.h"


#define DESCRIBE(xparam, format)    fprintf(stderr, #xparam " = %" #format "\n", xparam)

static char const *
implemetation_description_string = "OpenGL/VAAPI/libswscale backend for VDPAU";


static
uint32_t
chroma_storage_size_divider(VdpChromaType chroma_type)
{
    switch (chroma_type) {
    case VDP_CHROMA_TYPE_420: return 4;
    case VDP_CHROMA_TYPE_422: return 2;
    case VDP_CHROMA_TYPE_444: return 1;
    default: return 1;
    }
}

static
const char *
softVdpGetErrorString(VdpStatus status)
{
    return reverse_status(status);
}

VdpStatus
softVdpGetApiVersion(uint32_t *api_version)
{
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                                uint32_t *max_level, uint32_t *max_macroblocks,
                                uint32_t *max_width, uint32_t *max_height)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == is_supported || NULL == max_level || NULL == max_macroblocks ||
        NULL == max_width || NULL == max_height)
    {
        return VDP_STATUS_INVALID_POINTER;
    }

    *max_level = 0;
    *max_macroblocks = 0;
    *max_width = 0;
    *max_height = 0;

    if (! deviceData->va_available) {
        *is_supported = 0;
        return VDP_STATUS_OK;
    }

    VAProfile *va_profile_list = malloc(sizeof(VAProfile) * vaMaxNumProfiles(deviceData->va_dpy));
    if (NULL == va_profile_list)
        return VDP_STATUS_RESOURCES;

    int num_profiles;
    VAStatus status = vaQueryConfigProfiles(deviceData->va_dpy, va_profile_list, &num_profiles);
    if (VA_STATUS_SUCCESS != status) {
        free(va_profile_list);
        return VDP_STATUS_ERROR;
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

    return VDP_STATUS_OK;
}

VdpStatus
softVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                     uint32_t max_references, VdpDecoder *decoder)
{
    VdpStatus retval = VDP_STATUS_ERROR;
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;
    if (!deviceData->va_available)
        return VDP_STATUS_INVALID_DECODER_PROFILE;
    VADisplay va_dpy = deviceData->va_dpy;

    VdpDecoderData *data = calloc(1, sizeof(VdpDecoderData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

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
            retval = VDP_STATUS_INVALID_DECODER_PROFILE;
            goto error;
        }

        status = vaCreateConfig(va_dpy, va_profile, VAEntrypointVLD, NULL, 0, &data->config_id);
        if (VA_STATUS_SUCCESS == status)        // break loop if decoder created
            break;
    }

    if (VA_STATUS_SUCCESS != status)
        goto error;

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
    if (VA_STATUS_SUCCESS != status)
        goto error;

    status = vaCreateContext(va_dpy, data->config_id, width, height, VA_PROGRESSIVE,
        data->render_targets, data->num_render_targets, &data->context_id);
    if (VA_STATUS_SUCCESS != status)
        goto error;

    deviceData->refcount ++;
    *decoder = handlestorage_add(data);

    return VDP_STATUS_OK;
error:
    free(data);
    return retval;
}

VdpStatus
softVdpDecoderDestroy(VdpDecoder decoder)
{
    VdpDecoderData *decoderData = handlestorage_get(decoder, HANDLETYPE_DECODER);
    if (NULL == decoderData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = decoderData->device;

    if (deviceData->va_available) {
        VADisplay va_dpy = deviceData->va_dpy;
        vaDestroySurfaces(va_dpy, decoderData->render_targets, decoderData->num_render_targets);
        vaDestroyContext(va_dpy, decoderData->context_id);
        vaDestroyConfig(va_dpy, decoderData->config_id);
    }

    handlestorage_expunge(decoder);
    deviceData->refcount --;
    free(decoderData);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                            uint32_t *width, uint32_t *height)
{
    VdpDecoderData *decoderData = handlestorage_get(decoder, HANDLETYPE_DECODER);
    if (NULL == decoderData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == profile || NULL == width || NULL == height)
        return VDP_STATUS_INVALID_HANDLE;

    *profile = decoderData->profile;
    *width   = decoderData->width;
    *height  = decoderData->height;

    return VDP_STATUS_OK;
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
            handlestorage_get(vdp_ref->surface, HANDLETYPE_VIDEO_SURFACE);
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
    }

    return VDP_STATUS_OK;
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
        PIC_FIELDS(deblocking_filter_control_present_flag) = vdppi->deblocking_filter_control_present_flag;
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
    
    if(sizeof(iq_matrix->ScalingList4x4) == sizeof(vdppi->scaling_lists_4x4))
        memcpy(iq_matrix->ScalingList4x4, vdppi->scaling_lists_4x4, sizeof(iq_matrix->ScalingList4x4));
    else
    {
        for (int j = 0; j < 6; j ++)
            for (int k = 0; k < 16; k ++)
                iq_matrix->ScalingList4x4[j][k] = vdppi->scaling_lists_4x4[j][k];
    }

    if(sizeof(iq_matrix->ScalingList8x8) == sizeof(vdppi->scaling_lists_8x8))
        memcpy(iq_matrix->ScalingList8x8, vdppi->scaling_lists_8x8, sizeof(iq_matrix->ScalingList8x8));
    else
    {
        for (int j = 0; j < 2; j ++)
            for (int k = 0; k < 64; k ++)
                iq_matrix->ScalingList8x8[j][k] = vdppi->scaling_lists_8x8[j][k];
    }
}

VdpStatus
softVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                     VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                     VdpBitstreamBuffer const *bitstream_buffers)
{
    VdpDecoderData *decoderData = handlestorage_get(decoder, HANDLETYPE_DECODER);
    VdpVideoSurfaceData *dstSurfData = handlestorage_get(target, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == decoderData || NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = decoderData->device;
    VADisplay va_dpy = deviceData->va_dpy;
    VAStatus status;
    VdpStatus vs;

    if (VDP_DECODER_PROFILE_H264_BASELINE == decoderData->profile ||
        VDP_DECODER_PROFILE_H264_MAIN ==     decoderData->profile ||
        VDP_DECODER_PROFILE_H264_HIGH ==     decoderData->profile)
    {
        VdpPictureInfoH264 const *vdppi = (void *)picture_info;

        // TODO: figure out where to get level
        uint32_t level = 41;

        // preparing picture parameters
        VABufferID pic_param_buf;
        VAPictureParameterBufferH264 *pic_param;

        status = vaCreateBuffer(va_dpy, decoderData->context_id, VAPictureParameterBufferType,
            sizeof(VAPictureParameterBufferH264), 1, NULL, &pic_param_buf);
        if (VA_STATUS_SUCCESS != status)
            goto error;

        status = vaMapBuffer(va_dpy, pic_param_buf, (void **)&pic_param);
        if (VA_STATUS_SUCCESS != status)
            goto error;

        vs = h264_translate_reference_frames(dstSurfData, decoderData, pic_param, vdppi);
        if (VDP_STATUS_RESOURCES == vs)
            goto error_no_surfaces_left;
        if (VDP_STATUS_OK != vs)
            goto error;

        h264_translate_pic_param(pic_param, decoderData->width, decoderData->height, vdppi, level);
        vaUnmapBuffer(va_dpy, pic_param_buf);

        //  IQ Matrix
        VABufferID iq_matrix_buf;
        VAIQMatrixBufferH264 *iq_matrix;

        status = vaCreateBuffer(va_dpy, decoderData->context_id, VAIQMatrixBufferType,
            sizeof(VAIQMatrixBufferH264), 1, NULL, &iq_matrix_buf);
        if (VA_STATUS_SUCCESS != status)
            goto error;

        status = vaMapBuffer(va_dpy, iq_matrix_buf, (void **)&iq_matrix);
        if (VA_STATUS_SUCCESS != status)
            goto error;

        h264_translate_iq_matrix(iq_matrix, vdppi);
        vaUnmapBuffer(va_dpy, iq_matrix_buf);

        // send data to decoding hardware
        status = vaBeginPicture(va_dpy, decoderData->context_id, dstSurfData->va_surf);
        if (VA_STATUS_SUCCESS != status)
            goto error;
        status = vaRenderPicture(va_dpy, decoderData->context_id, &pic_param_buf, 1);
        if (VA_STATUS_SUCCESS != status)
            goto error;
        status = vaRenderPicture(va_dpy, decoderData->context_id, &iq_matrix_buf, 1);
        if (VA_STATUS_SUCCESS != status)
            goto error;

        vaDestroyBuffer(va_dpy, pic_param_buf);
        vaDestroyBuffer(va_dpy, iq_matrix_buf);

        // merge bitstream buffers
        int total_bitstream_bytes = 0;
        for (unsigned int k = 0; k < bitstream_buffer_count; k ++)
            total_bitstream_bytes += bitstream_buffers[k].bitstream_bytes;

        uint8_t *merged_bitstream = malloc(total_bitstream_bytes);
        if (NULL == merged_bitstream)
            goto error_resources;

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
        if (nal_offset < 0)
            goto error_no_nal_header;

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
            int ChromaArrayType = pic_param->seq_fields.bits.chroma_format_idc;

            // parse slice header and use its data to fill slice parameter buffer
            parse_slice_header(&st, pic_param, ChromaArrayType, vdppi->num_ref_idx_l0_active_minus1,
                               vdppi->num_ref_idx_l1_active_minus1, &sp_h264);

            VABufferID slice_parameters_buf;
            status = vaCreateBuffer(va_dpy, decoderData->context_id, VASliceParameterBufferType,
                sizeof(VASliceParameterBufferH264), 1, &sp_h264, &slice_parameters_buf);
            if (VA_STATUS_SUCCESS != status)
                goto error;
            status = vaRenderPicture(va_dpy, decoderData->context_id, &slice_parameters_buf, 1);
            if (VA_STATUS_SUCCESS != status)
                goto error;

            VABufferID slice_buf;
            status = vaCreateBuffer(va_dpy, decoderData->context_id, VASliceDataBufferType,
                sp_h264.slice_data_size, 1, merged_bitstream + nal_offset, &slice_buf);
            if (VA_STATUS_SUCCESS != status)
                goto error;

            status = vaRenderPicture(va_dpy, decoderData->context_id, &slice_buf, 1);
            if (VA_STATUS_SUCCESS != status)
                goto error;

            vaDestroyBuffer(va_dpy, slice_parameters_buf);
            vaDestroyBuffer(va_dpy, slice_buf);

            if (nal_offset_next < 0)        // nal_offset_next equals -1 when there is no slice
                break;                      // start code found. Thus that was the final slice.
            nal_offset = nal_offset_next;
        } while (1);

        status = vaEndPicture(va_dpy, decoderData->context_id);
        if (VA_STATUS_SUCCESS != status)
            goto error;

        free(merged_bitstream);
    } else {
        traceError("error (softVdpDecoderRender): no implementation for profile %s\n",
                   reverse_decoder_profile(decoderData->profile));
        return VDP_STATUS_NO_IMPLEMENTATION;
    }

    return VDP_STATUS_OK;
error:
    traceError("error (softVdpDecoderRender): something gone wrong\n");
    return VDP_STATUS_ERROR;
error_no_nal_header:
    traceError("error (softVdpDecoderRender): no NAL header\n");
    return VDP_STATUS_ERROR;
error_resources:
    return VDP_STATUS_RESOURCES;
error_no_surfaces_left:
    traceError("error (softVdpDecoderRender): no surfaces left in buffer\n");
    return VDP_STATUS_ERROR;
}

VdpStatus
softVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == is_supported || NULL == max_width || NULL == max_height)
        return VDP_STATUS_INVALID_POINTER;

    switch (surface_rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
    case VDP_RGBA_FORMAT_R8G8B8A8:
    case VDP_RGBA_FORMAT_R10G10B10A2:
    case VDP_RGBA_FORMAT_B10G10R10A2:
    case VDP_RGBA_FORMAT_A8:
        *is_supported = 1;          // All these formats should be supported by OpenGL
        break;                      // implementation.
    default:
        *is_supported = 0;
        break;
    }

    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceQueryCapabilities): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    *max_width = max_texture_size;
    *max_height = max_texture_size;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                    VdpRGBAFormat surface_rgba_format,
                                                    VdpIndexedFormat bits_indexed_format,
                                                    VdpColorTableFormat color_table_format,
                                                    VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)bits_indexed_format; (void)color_table_format;
    (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)bits_ycbcr_format; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpOutputSurface *surface)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    //TODO: figure out reasonable limits
    if (width > 4096 || height > 4096)
        return VDP_STATUS_INVALID_SIZE;

    VdpOutputSurfaceData *data = (VdpOutputSurfaceData *)calloc(1, sizeof(VdpOutputSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    switch (rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_BGRA;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_R8G8B8A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_RGBA;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_R10G10B10A2:
        data->gl_internal_format = GL_RGB10_A2;
        data->gl_format = GL_RGBA;
        data->gl_type = GL_UNSIGNED_INT_10_10_10_2;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_B10G10R10A2:
        data->gl_internal_format = GL_RGB10_A2;
        data->gl_format = GL_BGRA;
        data->gl_type = GL_UNSIGNED_INT_10_10_10_2;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_RED;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 1;
        break;
    default:
        traceError("error (VdpOutputSurfaceCreate): %s is not implemented\n",
                   reverse_rgba_format(rgba_format));
        free(data);
        return VDP_STATUS_INVALID_RGBA_FORMAT;
    }

    data->type = HANDLETYPE_OUTPUT_SURFACE;
    data->width = width;
    data->height = height;
    data->device = deviceData;
    data->rgba_format = rgba_format;

    glx_context_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // reserve texture
    glTexImage2D(GL_TEXTURE_2D, 0, data->gl_internal_format, width, height, 0, data->gl_format,
                 data->gl_type, NULL);

    glGenFramebuffers(1, &data->fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, data->fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->tex_id, 0);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != gl_status) {
        traceError("error (VdpOutputSurfaceCreate): "
                   "framebuffer not ready, %d, %s\n", gl_status, gluErrorString(gl_status));
        glx_context_pop();
        free(data);
        return VDP_STATUS_ERROR;
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceCreate): gl error %d\n", gl_error);
        free(data);
        return VDP_STATUS_ERROR;
    }

    deviceData->refcount ++;
    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    VdpOutputSurfaceData *data = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->device;

    glx_context_push_thread_local(deviceData);
    glDeleteTextures(1, &data->tex_id);
    glDeleteFramebuffers(1, &data->fbo_id);

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceDestroy): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    handlestorage_expunge(surface);
    deviceData->refcount --;
    free(data);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height)
{
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == rgba_format || NULL == width || NULL == height)
        return VDP_STATUS_INVALID_HANDLE;

    *rgba_format = surfData->rgba_format;
    *width       = surfData->width;
    *height      = surfData->height;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    VdpOutputSurfaceData *srcSurfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = srcSurfData->device;

    VdpRect srcRect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (source_rect)
        srcRect = *source_rect;

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, srcSurfData->fbo_id);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, destination_pitches[0] / srcSurfData->bytes_per_pixel);
    if (4 != srcSurfData->bytes_per_pixel)
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(srcRect.x0, srcRect.y0, srcRect.x1 - srcRect.x0, srcRect.y1 - srcRect.y0,
                 srcSurfData->gl_format, srcSurfData->gl_type, destination_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    if (4 != srcSurfData->bytes_per_pixel)
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceGetBitsNative): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    VdpOutputSurfaceData *dstSurfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->device;

    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    glx_context_push_thread_local(deviceData);
    glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0] / dstSurfData->bytes_per_pixel);
    if (4 != dstSurfData->bytes_per_pixel)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, dstRect.x0, dstRect.y0,
                    dstRect.x1 - dstRect.x0, dstRect.y1 - dstRect.y0,
                    dstSurfData->gl_format, dstSurfData->gl_type, source_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    if (4 != dstSurfData->bytes_per_pixel)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfacePutBitsNative): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
                                   void const *const *source_data, uint32_t const *source_pitch,
                                   VdpRect const *destination_rect,
                                   VdpColorTableFormat color_table_format, void const *color_table)
{
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = surfData->device;

    VdpRect dstRect = {0, 0, surfData->width, surfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    // there is no other formats anyway
    if (VDP_COLOR_TABLE_FORMAT_B8G8R8X8 != color_table_format)
        return VDP_STATUS_INVALID_COLOR_TABLE_FORMAT;
    const uint32_t *color_table32 = color_table;

    glx_context_push_thread_local(deviceData);

    switch (source_indexed_format) {
    case VDP_INDEXED_FORMAT_I8A8:
        // TODO: use shader?
        do {
            const uint32_t dstRectWidth = dstRect.x1 - dstRect.x0;
            const uint32_t dstRectHeight = dstRect.y1 - dstRect.y0;
            uint32_t *unpacked_buf = malloc(4 * dstRectWidth * dstRectHeight);
            if (NULL == unpacked_buf)
                return VDP_STATUS_RESOURCES;

            for (unsigned int y = 0; y < dstRectHeight; y ++) {
                const uint8_t *src_ptr = source_data[0];
                src_ptr += y * source_pitch[0];
                uint32_t *dst_ptr = unpacked_buf + y * dstRectWidth;
                for (unsigned int x = 0; x < dstRectWidth; x ++) {
                    const uint8_t i = *src_ptr++;
                    const uint32_t a = (*src_ptr++) << 24;
                    dst_ptr[x] = (color_table32[i] & 0x00ffffff) + a;
                }
            }

            glBindTexture(GL_TEXTURE_2D, surfData->tex_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, dstRect.x0, dstRect.y0,
                            dstRect.x1 - dstRect.x0, dstRect.y1 - dstRect.y0,
                            GL_BGRA, GL_UNSIGNED_BYTE, unpacked_buf);
            glFinish();
            free(unpacked_buf);

            GLenum gl_error = glGetError();
            glx_context_pop();
            if (GL_NO_ERROR != gl_error) {
                traceError("error (VdpOutputSurfacePutBitsIndexed): gl error %d\n", gl_error);
                return VDP_STATUS_ERROR;
            }

            return VDP_STATUS_OK;
        } while (0);
        break;
    default:
        traceError("error (VdpOutputSurfacePutBitsIndexed): unsupported indexed format %s\n",
                   reverse_indexed_format(source_indexed_format));
        return VDP_STATUS_INVALID_INDEXED_FORMAT;
    }
}

VdpStatus
softVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                 void const *const *source_data, uint32_t const *source_pitches,
                                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    (void)surface; (void)source_ycbcr_format; (void)source_data; (void)source_pitches;
    (void)destination_rect; (void)csc_matrix;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                     VdpBool *is_supported)
{
    (void)device; (void)feature; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                       VdpBool *is_supported)
{
    (void)device; (void)parameter; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                       VdpBool *is_supported)
{
    (void)device; (void)attribute; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                          void *min_value, void *max_value)
{
    (void)device; (void)parameter; (void)min_value; (void)max_value;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                          void *min_value, void *max_value)
{
    (void)device; (void)attribute; (void)min_value; (void)max_value;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                        VdpVideoMixerFeature const *features, uint32_t parameter_count,
                        VdpVideoMixerParameter const *parameters,
                        void const *const *parameter_values, VdpVideoMixer *mixer)
{
    (void)feature_count; (void)features;    // TODO: mixer features
    (void)parameter_count; (void)parameters; (void)parameter_values;    // TODO: mixer parameters
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoMixerData *data = (VdpVideoMixerData *)calloc(1, sizeof(VdpVideoMixerData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_VIDEO_MIXER;
    data->device = deviceData;

    deviceData->refcount ++;
    *mixer = handlestorage_add(data);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features,
                                   VdpBool const *feature_enables)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_enables;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void const *const *attribute_values)
{
    (void)mixer; (void)attribute_count; (void)attributes; (void)attribute_values;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_supports;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_enables;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                    VdpVideoMixerParameter const *parameters,
                                    void *const *parameter_values)
{
    (void)mixer; (void)parameter_count; (void)parameters; (void)parameter_values;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void *const *attribute_values)
{
    (void)mixer; (void)attribute_count; (void)attributes; (void)attribute_values;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    VdpVideoMixerData *videoMixerData = handlestorage_get(mixer, HANDLETYPE_VIDEO_MIXER);
    if (NULL == videoMixerData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoMixerData->device;

    free(videoMixerData);
    deviceData->refcount --;
    handlestorage_expunge(mixer);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoMixerRender(VdpVideoMixer mixer, VdpOutputSurface background_surface,
                        VdpRect const *background_source_rect,
                        VdpVideoMixerPictureStructure current_picture_structure,
                        uint32_t video_surface_past_count,
                        VdpVideoSurface const *video_surface_past,
                        VdpVideoSurface video_surface_current, uint32_t video_surface_future_count,
                        VdpVideoSurface const *video_surface_future,
                        VdpRect const *video_source_rect, VdpOutputSurface destination_surface,
                        VdpRect const *destination_rect, VdpRect const *destination_video_rect,
                        uint32_t layer_count, VdpLayer const *layers)
{
    (void)mixer;    // TODO: mixer should be used to get mixing parameters
    // TODO: current implementation ignores previous and future surfaces, using only current.
    // Is that acceptable for interlaced video? Will VAAPI handle deinterlacing?

    (void)background_surface;   // TODO: background_surface. Is it safe to just ignore it?
    (void)background_source_rect;
    (void)current_picture_structure;
    (void)video_surface_past_count; (void)video_surface_past;
    (void)video_surface_future_count; (void)video_surface_future;
    (void)layer_count; (void)layers;

    VdpVideoSurfaceData *srcSurfData =
        handlestorage_get(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData || NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    if (srcSurfData->device != dstSurfData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = srcSurfData->device;

    VdpRect srcVideoRect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (video_source_rect)
        srcVideoRect = *video_source_rect;

    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    VdpRect dstVideoRect = srcVideoRect;
    if (destination_video_rect)
        dstVideoRect = *destination_video_rect;

    // TODO: dstRect should clip dstVideoRect

    glx_context_push_thread_local(deviceData);

    if (deviceData->va_available) {
        VAStatus status;
        if (NULL == srcSurfData->va_glx) {
            status = vaCreateSurfaceGLX(deviceData->va_dpy, GL_TEXTURE_2D, srcSurfData->tex_id,
                                        &srcSurfData->va_glx);
            if (VA_STATUS_SUCCESS != status) {
                glx_context_pop();
                return VDP_STATUS_ERROR;
            }
        }

        status = vaCopySurfaceGLX(deviceData->va_dpy, srcSurfData->va_glx, srcSurfData->va_surf, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
        glViewport(0, 0, dstSurfData->width, dstSurfData->height);
        glDisable(GL_BLEND);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f/srcSurfData->width, 1.0f/srcSurfData->height, 1.0f);

        // Clear dstRect area
        glDisable(GL_TEXTURE_2D);
        glColor4f(0, 0, 0, 1);
        glBegin(GL_QUADS);
            glVertex2f(dstRect.x0, dstRect.y0);
            glVertex2f(dstRect.x1, dstRect.y0);
            glVertex2f(dstRect.x1, dstRect.y1);
            glVertex2f(dstRect.x0, dstRect.y1);
        glEnd();

        // Render (maybe scaled) data from video surface
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);
        glColor4f(1, 1, 1, 1);
        glBegin(GL_QUADS);
            glTexCoord2i(srcVideoRect.x0, srcVideoRect.y0);
            glVertex2f(dstVideoRect.x0, dstVideoRect.y0);

            glTexCoord2i(srcVideoRect.x1, srcVideoRect.y0);
            glVertex2f(dstVideoRect.x1, dstVideoRect.y0);

            glTexCoord2i(srcVideoRect.x1, srcVideoRect.y1);
            glVertex2f(dstVideoRect.x1, dstVideoRect.y1);

            glTexCoord2i(srcVideoRect.x0, srcVideoRect.y1);
            glVertex2f(dstVideoRect.x0, dstVideoRect.y1);
        glEnd();

    } else {
        // fall back to software convertion
        // TODO: make sure not to do scaling in software, only colorspace conversion
        // TODO: use GL shaders to do colorspace conversion job
        // TODO: handle all three kind of rectangles and clipping
        const uint32_t dstVideoWidth  = dstVideoRect.x1 - dstVideoRect.x0;
        const uint32_t dstVideoHeight = dstVideoRect.y1 - dstVideoRect.y0;

        const uint32_t dstVideoStride = (dstVideoWidth & 3) ? (dstVideoWidth & ~3u) + 4
                                                            : dstVideoWidth;
        uint8_t *img_buf = malloc(dstVideoStride * dstVideoHeight * 4);
        if (NULL == img_buf) return VDP_STATUS_RESOURCES;

        struct SwsContext *sws_ctx =
            sws_getContext(srcSurfData->width, srcSurfData->height, PIX_FMT_YUV420P,
                dstVideoWidth, dstVideoHeight,
                PIX_FMT_RGBA, SWS_POINT, NULL, NULL, NULL);

        uint8_t const * const src_planes[] =
            { srcSurfData->y_plane, srcSurfData->v_plane, srcSurfData->u_plane, NULL };
        int src_strides[] = {srcSurfData->stride, srcSurfData->stride/2, srcSurfData->stride/2, 0};
        uint8_t *dst_planes[] = {img_buf, NULL, NULL, NULL};
        int dst_strides[] = {dstVideoStride * 4, 0, 0, 0};

        int res = sws_scale(sws_ctx,
                            src_planes, src_strides, 0, srcSurfData->height,
                            dst_planes, dst_strides);
        sws_freeContext(sws_ctx);
        if (res != (int)dstVideoHeight) {
            traceError("error (softVdpVideoMixerRender): libswscale scaling failed\n");
            glx_context_pop();
            return VDP_STATUS_ERROR;
        }

        // copy converted image to texture
        glPixelStorei(GL_UNPACK_ROW_LENGTH, dstVideoStride);
        glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);
        glTexSubImage2D(GL_TEXTURE_2D, 0,
            dstVideoRect.x0, dstVideoRect.y0,
            dstVideoRect.x1 - dstVideoRect.x0, dstVideoRect.y1 - dstVideoRect.y0,
            GL_BGRA, GL_UNSIGNED_BYTE, img_buf);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        free(img_buf);
    }
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoMixerRender): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpBool *is_supported, uint32_t *max_width,
                                     uint32_t *max_height)
{
    (void)device; (void)surface_chroma_type;
    // TODO: implement
    *is_supported = 1;
    *max_width = 1920;
    *max_height = 1080;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpChromaType surface_chroma_type,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    (void)device; (void)surface_chroma_type; (void)bits_ycbcr_format;
    *is_supported = 1;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                          uint32_t height, VdpVideoSurface *surface)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoSurfaceData *data = (VdpVideoSurfaceData *)calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    uint32_t const stride = (width % 4 == 0) ? width : (width & ~0x3UL) + 4;
    data->type = HANDLETYPE_VIDEO_SURFACE;
    data->device = deviceData;
    data->chroma_type = chroma_type;
    data->width = width;
    data->stride = stride;
    data->height = height;
    data->va_surf = VA_INVALID_SURFACE;
    data->va_glx = NULL;
    data->tex_id = 0;

    glx_context_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data->width, data->height, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfaceCreate): gl error %d\n", gl_error);
        free(data);
        return VDP_STATUS_ERROR;
    }

    if (deviceData->va_available) {
        // no VA surface creation here. Actual pool of VA surfaces should be allocated already
        // by VdpDecoderCreate. VdpDecoderCreate will update ->va_surf field as needed.
        data->y_plane = NULL;
        data->v_plane = NULL;
        data->u_plane = NULL;
    } else {
        //TODO: find valid storage size for chroma_type
        data->y_plane = malloc(stride * height);
        data->v_plane = malloc(stride * height / chroma_storage_size_divider(chroma_type));
        data->u_plane = malloc(stride * height / chroma_storage_size_divider(chroma_type));
        if (NULL == data->y_plane || NULL == data->v_plane || NULL == data->u_plane) {
            if (data->y_plane) free(data->y_plane);
            if (data->v_plane) free(data->v_plane);
            if (data->u_plane) free(data->u_plane);
            free(data);
            return VDP_STATUS_RESOURCES;
        }
    }

    deviceData->refcount ++;
    *surface = handlestorage_add(data);

    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    VdpVideoSurfaceData *videoSurfData = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == videoSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoSurfData->device;

    glx_context_push_thread_local(deviceData);
    glDeleteTextures(1, &videoSurfData->tex_id);

    GLenum gl_error = glGetError();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfaceDestroy): gl error %d\n", gl_error);
        glx_context_pop();
        return VDP_STATUS_ERROR;
    }

    if (videoSurfData->va_glx) {
        vaDestroySurfaceGLX(deviceData->va_dpy, videoSurfData->va_glx);
    }

    if (deviceData->va_available) {
        // .va_surf will be freed in VdpDecoderDestroy
    } else {
        free(videoSurfData->y_plane);
        free(videoSurfData->v_plane);
        free(videoSurfData->u_plane);
    }

    glx_context_pop();
    free(videoSurfData);
    deviceData->refcount --;
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                 uint32_t *width, uint32_t *height)
{
    VdpVideoSurfaceData *videoSurf = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == videoSurf)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == chroma_type || NULL == width || NULL == height)
        return VDP_STATUS_INVALID_POINTER;

    *chroma_type = videoSurf->chroma_type;
    *width       = videoSurf->width;
    *height      = videoSurf->height;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    VdpVideoSurfaceData *srcSurfData = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == srcSurfData) return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = srcSurfData->device;
    VADisplay va_dpy = deviceData->va_dpy;

    if (deviceData->va_available) {
        VAImage q;
        vaDeriveImage(va_dpy, srcSurfData->va_surf, &q);
        if (VA_FOURCC('N', 'V', '1', '2') == q.format.fourcc &&
            VDP_YCBCR_FORMAT_NV12 == destination_ycbcr_format)
        {
            uint8_t *img_data;
            vaMapBuffer(va_dpy, q.buf, (void **)&img_data);
            if (destination_pitches[0] == q.pitches[0] &&
                destination_pitches[1] == q.pitches[1])
            {
                memcpy(destination_data[0], img_data + q.offsets[0], q.width * q.height);
                memcpy(destination_data[1], img_data + q.offsets[1], q.width * q.height / 2);
            } else {
                uint8_t *src = img_data + q.offsets[0];
                uint8_t *dst = destination_data[0];
                for (unsigned int y = 0; y < q.height; y ++) {  // Y plane
                    memcpy (dst, src, q.width);
                    src += q.pitches[0];
                    dst += destination_pitches[0];
                }
                src = img_data + q.offsets[1];
                dst = destination_data[1];
                for (unsigned int y = 0; y < q.height / 2; y ++) {  // UV plane
                    memcpy(dst, src, q.width);  // q.width/2 samples of U and V each, hence q.width
                    src += q.pitches[1];
                    dst += destination_pitches[1];
                }
            }
            vaUnmapBuffer(va_dpy, q.buf);
        } else if (VA_FOURCC('N', 'V', '1', '2') == q.format.fourcc &&
                   VDP_YCBCR_FORMAT_YV12 == destination_ycbcr_format)
        {
            uint8_t *img_data;
            vaMapBuffer(va_dpy, q.buf, (void **)&img_data);

            // Y plane
            if (destination_pitches[0] == q.pitches[0]) {
                memcpy(destination_data[0], img_data + q.offsets[0], q.width * q.height);
            } else {
                uint8_t *src = img_data + q.offsets[0];
                uint8_t *dst = destination_data[0];
                for (unsigned int y = 0; y < q.height; y ++) {
                    memcpy (dst, src, q.width);
                    src += q.pitches[0];
                    dst += destination_pitches[0];
                }
            }

            // unpack mixed UV to separate planes
            for (unsigned int y = 0; y < q.height/2; y ++) {
                uint8_t *src = img_data + q.offsets[1] + y * q.pitches[1];
                uint8_t *dst_u = destination_data[1] + y * destination_pitches[1];
                uint8_t *dst_v = destination_data[2] + y * destination_pitches[2];

                for (unsigned int x = 0; x < q.width/2; x++) {
                    *dst_v++ = *src++;
                    *dst_u++ = *src++;
                }
            }

            vaUnmapBuffer(va_dpy, q.buf);
        } else {
            const char *c = (const char *)&q.format.fourcc;
            traceError("error (softVdpVideoSurfaceGetBitsYCbCr): not implemented conversion "
                       "VA FOURCC %c%c%c%c -> %s\n", *c, *(c+1), *(c+2), *(c+3),
                       reverse_ycbcr_format(destination_ycbcr_format));
            vaDestroyImage(va_dpy, q.image_id);
            return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
        }
        vaDestroyImage(va_dpy, q.image_id);
    } else {
        // software fallback
        traceError("error (softVdpVideoSurfaceGetBitsYCbCr): not implemented software fallback\n");
        return VDP_STATUS_ERROR;
    }

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfaceGetBitsYCbCr): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                void const *const *source_data, uint32_t const *source_pitches)
{
    //TODO: figure out what to do with other formats

    VdpVideoSurfaceData *dstSurfData = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->device;

    glx_context_push_thread_local(deviceData);

    if (deviceData->va_available) {
        if (VDP_YCBCR_FORMAT_YV12 != source_ycbcr_format) {
            traceError("error (softVdpVideoSurfacePutBitsYCbCr): not supported source_ycbcr_format "
                       "%s\n", reverse_ycbcr_format(source_ycbcr_format));
            return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
        }

        void *bgra_buf = malloc(dstSurfData->width * dstSurfData->height * 4);
        if (NULL == bgra_buf) {
            traceError("error (softVdpVideoSurfacePutBitsYCbCr): can not allocate memory\n");
            return VDP_STATUS_RESOURCES;
        }

        // TODO: other source formats
        struct SwsContext *sws_ctx =
            sws_getContext(dstSurfData->width, dstSurfData->height, PIX_FMT_YUV420P,
                           dstSurfData->width, dstSurfData->height, PIX_FMT_BGRA,
                           SWS_FAST_BILINEAR, NULL, NULL, NULL);
        if (NULL == sws_ctx) {
            traceError("error (softVdpVideoSurfacePutBitsYCbCr): can not create SwsContext\n");
            free(bgra_buf);
            return VDP_STATUS_RESOURCES;
        }

        const uint8_t * const srcSlice[] = { source_data[0], source_data[2], source_data[1], NULL };
        const int srcStride[] = { source_pitches[0], source_pitches[2], source_pitches[1], 0 };
        uint8_t * const dst[] = { bgra_buf, NULL, NULL, NULL };
        const int dstStride[] = { dstSurfData->width * 4, 0, 0, 0 };
        int res = sws_scale(sws_ctx, srcSlice, srcStride, 0, dstSurfData->height, dst, dstStride);
        if (res != (int)dstSurfData->height) {
            traceError("error (softVdpVideoSurfacePutBitsYCbCr): sws_scale returned %d while "
                       "%d expected\n", res, dstSurfData->height);
            free(bgra_buf);
            sws_freeContext(sws_ctx);
            return VDP_STATUS_ERROR;
        }
        sws_freeContext(sws_ctx);

        glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dstSurfData->width, dstSurfData->height,
                        GL_BGRA, GL_UNSIGNED_BYTE, bgra_buf);
        free(bgra_buf);
    } else {
        if (VDP_YCBCR_FORMAT_YV12 != source_ycbcr_format) {
            traceError("error (softVdpVideoSurfacePutBitsYCbCr): not supported source_ycbcr_format "
                       "%s\n", reverse_ycbcr_format(source_ycbcr_format));
            return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
        }

        uint8_t const *src;
        uint8_t *dst;
        dst = dstSurfData->y_plane;     src = source_data[0];
        for (uint32_t k = 0; k < dstSurfData->height; k ++) {
            memcpy(dst, src, dstSurfData->width);
            dst += dstSurfData->stride;
            src += source_pitches[0];
        }

        dst = dstSurfData->v_plane;     src = source_data[1];
        for (uint32_t k = 0; k < dstSurfData->height / 2; k ++) {
            memcpy(dst, src, dstSurfData->width / 2);
            dst += dstSurfData->stride / 2;
            src += source_pitches[1];
        }

        dst = dstSurfData->u_plane;     src = source_data[2];
        for (uint32_t k = 0; k < dstSurfData->height / 2; k ++) {
            memcpy(dst, src, dstSurfData->width/2);
            dst += dstSurfData->stride / 2;
            src += source_pitches[2];
        }
    }

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfacePutBitsYCbCr): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == is_supported || NULL == max_width || NULL == max_height)
        return VDP_STATUS_INVALID_POINTER;

    switch (surface_rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
    case VDP_RGBA_FORMAT_R8G8B8A8:
    case VDP_RGBA_FORMAT_R10G10B10A2:
    case VDP_RGBA_FORMAT_B10G10R10A2:
    case VDP_RGBA_FORMAT_A8:
        *is_supported = 1;          // All these formats should be supported by OpenGL
        break;                      // implementation.
    default:
        *is_supported = 0;
        break;
    }

    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpBitmapSurfaceQueryCapabilities): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    *max_width = max_texture_size;
    *max_height = max_texture_size;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpBitmapSurfaceData *data = (VdpBitmapSurfaceData *)calloc(1, sizeof(VdpBitmapSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    switch (rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_BGRA;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_R8G8B8A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_RGBA;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_R10G10B10A2:
        data->gl_internal_format = GL_RGB10_A2;
        data->gl_format = GL_RGBA;
        data->gl_type = GL_UNSIGNED_INT_10_10_10_2;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_B10G10R10A2:
        data->gl_internal_format = GL_RGB10_A2;
        data->gl_format = GL_BGRA;
        data->gl_type = GL_UNSIGNED_INT_10_10_10_2;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_RED;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 1;
        break;
    default:
        traceError("error (VdpBitmapSurfaceCreate): %s not implemented\n",
                   reverse_rgba_format(rgba_format));
        free(data);
        return VDP_STATUS_INVALID_RGBA_FORMAT;
    }

    data->type = HANDLETYPE_BITMAP_SURFACE;
    data->device = deviceData;
    data->rgba_format = rgba_format;
    data->width = width;
    data->height = height;
    data->frequently_accessed = frequently_accessed;

    // Frequently accessed bitmaps reside in system memory rather that in GPU texture.
    data->dirty = 0;
    if (frequently_accessed) {
        data->bitmap_data = (char *)calloc(width * height, data->bytes_per_pixel);
        if (NULL == data->bitmap_data) {
            traceError("error (VdpBitmapSurfaceCreate): calloc returned NULL\n");
            free(data);
            return VDP_STATUS_RESOURCES;
        }
    } else {
        data->bitmap_data = NULL;
    }

    glx_context_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, data->gl_internal_format, width, height, 0,
                 data->gl_format, data->gl_type, NULL);
    glFinish();
    GLuint gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        // Requested RGBA format was wrong
        traceError("error (VdpBitmapSurfaceCreate): texture failure, gl error (%d, %s)\n", gl_error,
                   gluErrorString(gl_error));
        free(data);
        glx_context_pop();
        return VDP_STATUS_ERROR;
    }
    if (VDP_RGBA_FORMAT_A8 == rgba_format) {
        // map red channel to alpha
        GLint swizzle_mask[] = {GL_ONE, GL_ONE, GL_ONE, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);
    }

    gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        free(data);
        traceError("error (VdpBitmapSurfaceCreate): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    deviceData->refcount ++;
    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    VdpBitmapSurfaceData *data = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->device;

    if (data->frequently_accessed) {
        free(data->bitmap_data);
        data->bitmap_data = NULL;
    }

    glx_context_push_thread_local(deviceData);
    glDeleteTextures(1, &data->tex_id);

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpBitmapSurfaceDestroy): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    handlestorage_expunge(surface);
    deviceData->refcount --;
    free(data);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    VdpBitmapSurfaceData *srcSurfData = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == rgba_format || NULL == width || NULL == height || NULL == frequently_accessed)
        return VDP_STATUS_INVALID_POINTER;

    *rgba_format = srcSurfData->rgba_format;
    *width = srcSurfData->width;
    *height = srcSurfData->height;
    *frequently_accessed = srcSurfData->frequently_accessed;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    VdpBitmapSurfaceData *dstSurfData = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->device;

    VdpRect d_rect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        d_rect = *destination_rect;

    if (dstSurfData->frequently_accessed) {
        if (0 == d_rect.x0 && dstSurfData->width == d_rect.x1 && source_pitches[0] == d_rect.x1) {
            // full width
            const int bytes_to_copy =
                (d_rect.x1 - d_rect.x0) * (d_rect.y1 - d_rect.y0) * dstSurfData->bytes_per_pixel;
            memcpy(dstSurfData->bitmap_data +
                        d_rect.y0 * dstSurfData->width * dstSurfData->bytes_per_pixel,
                   source_data[0], bytes_to_copy);
        } else {
            const unsigned int bytes_in_line = (d_rect.x1-d_rect.x0)*dstSurfData->bytes_per_pixel;
            for (unsigned int y = d_rect.y0; y < d_rect.y1; y ++) {
                memcpy(dstSurfData->bitmap_data +
                            (y * dstSurfData->width + d_rect.x0) * dstSurfData->bytes_per_pixel,
                       source_data[0] + (y - d_rect.y0) * source_pitches[0],
                       bytes_in_line);
            }
        }
        dstSurfData->dirty = 1;
    } else {
        glx_context_push_thread_local(deviceData);

        glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]/dstSurfData->bytes_per_pixel);
        if (4 != dstSurfData->bytes_per_pixel)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, d_rect.x0, d_rect.y0,
            d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0,
            dstSurfData->gl_format, dstSurfData->gl_type, source_data[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        if (4 != dstSurfData->bytes_per_pixel)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glFinish();

        GLenum gl_error = glGetError();
        glx_context_pop();
        if (GL_NO_ERROR != gl_error) {
            traceError("error (VdpBitmapSurfacePutBitsNative): gl error %d\n", gl_error);
            return VDP_STATUS_ERROR;
        }
    }

    return VDP_STATUS_OK;
}

void
print_handle_type(int handle, void *item, void *p)
{
    VdpGenericHandle *gh = item;
    struct {
        int cnt;
        int total_cnt;
        VdpDeviceData *deviceData;
    } *pp = p;
    pp->total_cnt ++;

    if (gh) {
        if (pp->deviceData == gh->parent) {
            traceError("handle %d type = %d\n", handle, gh->type);
            pp->cnt ++;
        }
    }
}

static
void
destroy_child_objects(int handle, void *item, void *p)
{
    const void *parent = p;
    VdpGenericHandle *gh = item;
    if (gh) {
        if (parent == gh->parent) {
            switch (gh->type) {
            case HANDLETYPE_DEVICE:
                // do nothing
                break;
            case HANDLETYPE_PRESENTATION_QUEUE_TARGET:
                softVdpPresentationQueueDestroy(handle);
                break;
            case HANDLETYPE_PRESENTATION_QUEUE:
                softVdpPresentationQueueDestroy(handle);
                break;
            case HANDLETYPE_VIDEO_MIXER:
                softVdpVideoMixerDestroy(handle);
                break;
            case HANDLETYPE_OUTPUT_SURFACE:
                softVdpOutputSurfaceDestroy(handle);
                break;
            case HANDLETYPE_VIDEO_SURFACE:
                softVdpVideoSurfaceDestroy(handle);
                break;
            case HANDLETYPE_BITMAP_SURFACE:
                softVdpBitmapSurfaceDestroy(handle);
                break;
            case HANDLETYPE_DECODER:
                softVdpDecoderDestroy(handle);
                break;
            default:
                traceError("warning (destroy_child_objects): unknown handle type %d\n", gh->type);
                break;
            }
        }
    }
}

VdpStatus
softVdpDeviceDestroy(VdpDevice device)
{
    VdpDeviceData *data = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    if (0 != data->refcount) {
        // Buggy client forgot to destroy dependend objects or decided that destroying
        // VdpDevice destroys all child object. Let's try to mitigate and prevent leakage.
        traceError("warning (softVdpDeviceDestroy): non-zero reference count (%d). "
                   "Trying to free child objects.\n", data->refcount);
        void *parent_object = data;
        handlestorage_execute_for_all(destroy_child_objects, parent_object);
    }

    if (0 != data->refcount) {
        traceError("error (softVdpDeviceDestroy): still non-zero reference count (%d)\n",
                   data->refcount);
        traceError("Here is the list of objects:\n");
        struct {
            int cnt;
            int total_cnt;
            VdpDeviceData *deviceData;
        } state = { .cnt = 0, .total_cnt = 0, .deviceData = data };

        handlestorage_execute_for_all(print_handle_type, &state);
        traceError("Objects leaked: %d\n", state.cnt);
        traceError("Objects visited during scan: %d\n", state.total_cnt);
        return VDP_STATUS_ERROR;
    }

    // cleaup libva
    if (data->va_available)
        vaTerminate(data->va_dpy);

    XLockDisplay(data->display);

    glx_context_push_thread_local(data);
    glDeleteTextures(1, &data->watermark_tex_id);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glx_context_pop();

    locked_glXMakeCurrent(data->display, None, NULL);

    glx_context_unref_glc_hash_table(data->display);

    handlestorage_expunge(device);
    XUnlockDisplay(data->display);

    handlestorage_xdpy_copy_unref(data->display_orig);

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpDeviceDestroy): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    free(data);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpGetInformationString(char const **information_string)
{
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    if (NULL == procamp || NULL == csc_matrix)
        return VDP_STATUS_INVALID_POINTER;
    if (VDP_PROCAMP_VERSION != procamp->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    VdpCSCMatrix *m = csc_matrix;
    switch (standard) {
    case VDP_COLOR_STANDARD_ITUR_BT_601:
        (*m)[0][0] = 1.16438f; (*m)[0][1] =  0.0f;     (*m)[0][2] =  1.59603f; (*m)[0][3] = -222.921f;
        (*m)[1][0] = 1.16438f; (*m)[1][1] = -0.39176f; (*m)[1][2] = -0.81297f; (*m)[1][3] =  135.576f;
        (*m)[2][0] = 1.16438f; (*m)[2][1] =  2.01723f; (*m)[2][2] =  0.0f;     (*m)[2][3] = -276.836f;
        break;
    case VDP_COLOR_STANDARD_ITUR_BT_709:
        (*m)[0][0] =  1.0f; (*m)[0][1] =  0.0f;     (*m)[0][2] =  1.402f;   (*m)[0][3] = -179.456f;
        (*m)[1][0] =  1.0f; (*m)[1][1] = -0.34414f; (*m)[1][2] = -0.71414f; (*m)[1][3] =  135.460f;
        (*m)[2][0] =  1.0f; (*m)[2][1] =  1.772f;   (*m)[2][2] =  0.0f;     (*m)[2][3] = -226.816f;
        break;
    case VDP_COLOR_STANDARD_SMPTE_240M:
        (*m)[0][0] =  0.58139f; (*m)[0][1] = -0.76437f; (*m)[0][2] =  1.5760f;  (*m)[0][3] = 0.0f;
        (*m)[1][0] =  0.58140f; (*m)[1][1] = -0.99101f; (*m)[1][2] = -0.47663f; (*m)[1][3] = 0.0f;
        (*m)[2][0] =  0.58139f; (*m)[2][1] =  1.0616f;  (*m)[2][2] =  0.00000f; (*m)[2][3] = 0.0f;
        break;
    default:
        return VDP_STATUS_INVALID_COLOR_STANDARD;
    }

    return VDP_STATUS_OK;
}

static
GLuint
vdpBlendFuncToGLBlendFunc(VdpOutputSurfaceRenderBlendFactor blend_factor)
{
    switch (blend_factor) {
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO:
        return GL_ZERO;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE:
        return GL_ONE;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_COLOR:
        return GL_SRC_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA:
        return GL_SRC_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_ALPHA:
        return GL_DST_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_COLOR:
        return GL_DST_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA_SATURATE:
        return GL_SRC_ALPHA_SATURATE;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_COLOR:
        return GL_CONSTANT_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_ALPHA:
        return GL_CONSTANT_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    default:
        return GL_INVALID_VALUE;
    }
}

static
GLenum
vdpBlendEquationToGLEquation(VdpOutputSurfaceRenderBlendEquation blend_equation)
{
    switch (blend_equation) {
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_SUBTRACT:
        return GL_FUNC_SUBTRACT;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_REVERSE_SUBTRACT:
        return GL_FUNC_REVERSE_SUBTRACT;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD:
        return GL_FUNC_ADD;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MIN:
        return GL_MIN;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MAX:
        return GL_MAX;
    default:
        return GL_INVALID_VALUE;
    }
}

struct blend_state_struct {
    GLuint srcFuncRGB;
    GLuint srcFuncAlpha;
    GLuint dstFuncRGB;
    GLuint dstFuncAlpha;
    GLuint modeRGB;
    GLuint modeAlpha;
    int invalid_func;
    int invalid_eq;
};

static
struct blend_state_struct
vdpBlendStateToGLBlendState(VdpOutputSurfaceRenderBlendState const *blend_state)
{
    struct blend_state_struct bs;
    bs.invalid_func = 0;
    bs.invalid_eq = 0;

    // it's ok to pass NULL as blend_state
    if (blend_state) {
        bs.srcFuncRGB = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_source_color);
        bs.srcFuncAlpha = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_source_alpha);
        bs.dstFuncRGB = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_destination_color);
        bs.dstFuncAlpha = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_destination_alpha);
    } else {
        bs.srcFuncRGB = bs.srcFuncAlpha = GL_ONE;
        bs.dstFuncRGB = bs.dstFuncAlpha = GL_ZERO;
    }

    if (GL_INVALID_VALUE == bs.srcFuncRGB || GL_INVALID_VALUE == bs.srcFuncAlpha ||
        GL_INVALID_VALUE == bs.dstFuncRGB || GL_INVALID_VALUE == bs.dstFuncAlpha)
    {
        bs.invalid_func = 1;
    }

    if (blend_state) {
        bs.modeRGB = vdpBlendEquationToGLEquation(blend_state->blend_equation_color);
        bs.modeAlpha = vdpBlendEquationToGLEquation(blend_state->blend_equation_alpha);
    } else {
        bs.modeRGB = bs.modeAlpha = GL_FUNC_ADD;
    }
    if (GL_INVALID_VALUE == bs.modeRGB || GL_INVALID_VALUE == bs.modeAlpha)
        bs.invalid_eq = 1;

    return bs;
}

VdpStatus
softVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpOutputSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    (void)flags;    // TODO: handle flags

    if (blend_state)
        if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version)
            return VDP_STATUS_INVALID_VALUE;

    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *srcSurfData =
        handlestorage_get(source_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    if (srcSurfData->device != dstSurfData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = srcSurfData->device;

    const int dstWidth = dstSurfData->width;
    const int dstHeight = dstSurfData->height;
    const int srcWidth = srcSurfData->width;
    const int srcHeight = srcSurfData->height;
    VdpRect s_rect = {0, 0, srcWidth, srcHeight};
    VdpRect d_rect = {0, 0, dstWidth, dstHeight};

    if (source_rect) s_rect = *source_rect;
    if (destination_rect) d_rect = *destination_rect;

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);
    if (bs.invalid_func)
        return VDP_STATUS_INVALID_BLEND_FACTOR;
    if (bs.invalid_eq)
        return VDP_STATUS_INVALID_BLEND_EQUATION;

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstWidth, 0, dstHeight, -1.0f, 1.0f);
    glViewport(0, 0, dstWidth, dstHeight);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // paint source surface over
    glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/srcWidth, 1.0f/srcHeight, 1.0f);

    // blend
    glBlendFuncSeparate(bs.srcFuncRGB, bs.dstFuncRGB, bs.srcFuncAlpha, bs.dstFuncAlpha);
    glBlendEquationSeparate(bs.modeRGB, bs.modeAlpha);

    // TODO: handle colors for every corner
    // TODO: handle rotation (flags)
    glColor4f(1, 1, 1, 1);
    if (colors)
        glColor4f(colors[0].red, colors[0].green, colors[0].blue, colors[0].alpha);

    glBegin(GL_QUADS);
    glTexCoord2i(s_rect.x0, s_rect.y0); glVertex2f(d_rect.x0, d_rect.y0);
    glTexCoord2i(s_rect.x1, s_rect.y0); glVertex2f(d_rect.x1, d_rect.y0);
    glTexCoord2i(s_rect.x1, s_rect.y1); glVertex2f(d_rect.x1, d_rect.y1);
    glTexCoord2i(s_rect.x0, s_rect.y1); glVertex2f(d_rect.x0, d_rect.y1);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceRenderOutputSurface): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpBitmapSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    (void)flags;    // TODO: handle flags

    if (blend_state)
        if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version)
            return VDP_STATUS_INVALID_VALUE;

    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpBitmapSurfaceData *srcSurfData =
        handlestorage_get(source_surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData || NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    if (srcSurfData->device != dstSurfData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = srcSurfData->device;

    VdpRect srcRect = {0, 0, srcSurfData->width, srcSurfData->height};
    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (source_rect)
        srcRect = *source_rect;
    if (destination_rect)
        dstRect = *destination_rect;

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);
    if (bs.invalid_func)
        return VDP_STATUS_INVALID_BLEND_FACTOR;
    if (bs.invalid_eq)
        return VDP_STATUS_INVALID_BLEND_EQUATION;

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
    glViewport(0, 0, dstSurfData->width, dstSurfData->height);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // paint source surface over
    glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);
    if (srcSurfData->dirty) {
        if (4 != srcSurfData->bytes_per_pixel)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcSurfData->width, srcSurfData->height,
                        srcSurfData->gl_format, srcSurfData->gl_type, srcSurfData->bitmap_data);
        if (4 != srcSurfData->bytes_per_pixel)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        srcSurfData->dirty = 0;
    }

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/srcSurfData->width, 1.0f/srcSurfData->height, 1.0f);

    // blend
    glBlendFuncSeparate(bs.srcFuncRGB, bs.dstFuncRGB, bs.srcFuncAlpha, bs.dstFuncAlpha);
    glBlendEquationSeparate(bs.modeRGB, bs.modeAlpha);

    // TODO: handle colors for every corner
    // TODO: handle rotation (flags)
    glColor4f(1, 1, 1, 1);
    if (colors)
        glColor4f(colors[0].red, colors[0].green, colors[0].blue, colors[0].alpha);

    glBegin(GL_QUADS);
    glTexCoord2i(srcRect.x0, srcRect.y0);   glVertex2f(dstRect.x0, dstRect.y0);
    glTexCoord2i(srcRect.x1, srcRect.y0);   glVertex2f(dstRect.x1, dstRect.y0);
    glTexCoord2i(srcRect.x1, srcRect.y1);   glVertex2f(dstRect.x1, dstRect.y1);
    glTexCoord2i(srcRect.x0, srcRect.y1);   glVertex2f(dstRect.x0, dstRect.y1);
    glEnd();

    glColor4f(1, 1, 1, 1);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceRenderBitmapSurface): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    (void)device; (void)callback; (void)context;
    return VDP_STATUS_OK;
}


// =========================
VdpStatus
softVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    (void)device;   // there is no difference between various devices. All have same procedures
    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = &softVdpGetErrorString;
        break;
    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = &softVdpGetProcAddress;
        break;
    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = &lockedVdpGetApiVersion;
        break;
    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = &lockedVdpGetInformationString;
        break;
    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = &lockedVdpDeviceDestroy;
        break;
    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = &lockedVdpGenerateCSCMatrix;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &lockedVdpVideoSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &lockedVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = &lockedVdpVideoSurfaceCreate;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = &lockedVdpVideoSurfaceDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = &lockedVdpVideoSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = &lockedVdpVideoSurfaceGetBitsYCbCr;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &lockedVdpVideoSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &lockedVdpOutputSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = &lockedVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = &lockedVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &lockedVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = &lockedVdpOutputSurfaceCreate;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = &lockedVdpOutputSurfaceDestroy;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = &lockedVdpOutputSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = &lockedVdpOutputSurfaceGetBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &lockedVdpOutputSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = &lockedVdpOutputSurfacePutBitsIndexed;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &lockedVdpOutputSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &lockedVdpBitmapSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = &lockedVdpBitmapSurfaceCreate;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = &lockedVdpBitmapSurfaceDestroy;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = &lockedVdpBitmapSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &lockedVdpBitmapSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = &lockedVdpOutputSurfaceRenderOutputSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = &lockedVdpOutputSurfaceRenderBitmapSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // *function_pointer = &lockedVdpOutputSurfaceRenderVideoSurfaceLuma;
        *function_pointer = NULL;
        break;
    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = &lockedVdpDecoderQueryCapabilities;
        break;
    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = &lockedVdpDecoderCreate;
        break;
    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = &lockedVdpDecoderDestroy;
        break;
    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = &lockedVdpDecoderGetParameters;
        break;
    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = &lockedVdpDecoderRender;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = &lockedVdpVideoMixerQueryFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = &lockedVdpVideoMixerQueryParameterSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = &lockedVdpVideoMixerQueryAttributeSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = &lockedVdpVideoMixerQueryParameterValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = &lockedVdpVideoMixerQueryAttributeValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = &lockedVdpVideoMixerCreate;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = &lockedVdpVideoMixerSetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = &lockedVdpVideoMixerSetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = &lockedVdpVideoMixerGetFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = &lockedVdpVideoMixerGetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = &lockedVdpVideoMixerGetParameterValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = &lockedVdpVideoMixerGetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = &lockedVdpVideoMixerDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = &lockedVdpVideoMixerRender;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = &lockedVdpPresentationQueueTargetDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = &lockedVdpPresentationQueueCreate;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = &lockedVdpPresentationQueueDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = &lockedVdpPresentationQueueSetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = &lockedVdpPresentationQueueGetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = &lockedVdpPresentationQueueGetTime;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = &lockedVdpPresentationQueueDisplay;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = &lockedVdpPresentationQueueBlockUntilSurfaceIdle;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = &lockedVdpPresentationQueueQuerySurfaceStatus;
        break;
    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = &lockedVdpPreemptionCallbackRegister;
        break;
    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = &lockedVdpPresentationQueueTargetCreateX11;
        break;
    default:
        *function_pointer = NULL;
        break;
    } // switch

    if (NULL == *function_pointer)
        return VDP_STATUS_INVALID_FUNC_ID;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpDeviceCreateX11(Display *display_orig, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address)
{
    if (NULL == display_orig)
        return VDP_STATUS_INVALID_POINTER;

    // Let's get own connection to the X server
    Display *display = handlestorage_xdpy_copy_ref(display_orig);
    if (NULL == display)
        return VDP_STATUS_ERROR;

    if (global.quirks.buggy_XCloseDisplay) {
        // XCloseDisplay could segfault on fglrx. To avoid calling XCloseDisplay,
        // make one more reference to xdpy copy.
        (void)handlestorage_xdpy_copy_ref(display_orig);
    }

    VdpDeviceData *data = (VdpDeviceData *)calloc(1, sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    XLockDisplay(display);

    data->type = HANDLETYPE_DEVICE;
    data->display = display;
    data->display_orig = display_orig;   // save supplied pointer too
    data->screen = screen;
    data->refcount = 0;
    data->root = DefaultRootWindow(display);

    // create master GLX context to share data between further created ones
    glx_context_ref_glc_hash_table(display, screen);
    data->root_glc = glx_context_get_root_context();

    glx_context_push_thread_local(data);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // initialize VAAPI
    if (global.quirks.avoid_va) {
        // pretend there is no VA-API available
        data->va_available = 0;
    } else {
        data->va_dpy = vaGetDisplayGLX(display);
        data->va_available = 0;

        VAStatus status = vaInitialize(data->va_dpy, &data->va_version_major,
                                       &data->va_version_minor);
        if (VA_STATUS_SUCCESS == status) {
            data->va_available = 1;
            traceInfo("libva (version %d.%d) library initialized\n",
                      data->va_version_major, data->va_version_minor);
        } else {
            data->va_available = 0;
            traceInfo("warning: failed to initialize libva. "
                      "No video decode acceleration available.\n");
        }
    }

    glGenTextures(1, &data->watermark_tex_id);
    glBindTexture(GL_TEXTURE_2D, data->watermark_tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, watermark_width, watermark_height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, watermark_data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFinish();

    *device = handlestorage_add(data);
    *get_proc_address = &softVdpGetProcAddress;

    GLenum gl_error = glGetError();
    glx_context_pop();
    XUnlockDisplay(display);
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpDeviceCreateX11): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}
