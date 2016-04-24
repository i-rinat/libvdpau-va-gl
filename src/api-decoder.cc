/*
 * Copyright 2013-2016  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "api-decoder.hh"
#include "api-video-surface.hh"
#include "glx-context.hh"
#include "h264-parse.hh"
#include "handle-storage.hh"
#include "reverse-constant.hh"
#include "trace.hh"
#include <stdlib.h>
#include <string.h>


using std::make_shared;
using std::shared_ptr;
using std::vector;


namespace vdp { namespace Decoder {

Resource::Resource(shared_ptr<vdp::Device::Resource> a_device, VdpDecoderProfile a_profile,
                   uint32_t a_width, uint32_t a_height, uint32_t n_max_references)
    : profile{a_profile}
    , width{a_width}
    , height{a_height}
    , max_references{n_max_references}
{
    device = a_device;
    VADisplay va_dpy = device->va_dpy;

    if (!device->va_available)
        throw vdp::invalid_decoder_profile();

    // initialize free_list. Initially they all free
    for (int k = 0; k < vdp::kMaxRenderTargets; k ++)
        free_list.push_back(k);

    VAProfile va_profile;
    VAStatus status;
    bool final_try = false;
    VdpDecoderProfile next_profile = profile;

    // Try to create decoder for asked profile. On failure try to create more advanced one
    while (not final_try) {
        profile = next_profile;
        switch (profile) {
        case VDP_DECODER_PROFILE_H264_CONSTRAINED_BASELINE:
            va_profile = VAProfileH264ConstrainedBaseline;
            render_targets.resize(vdp::kNumRenderTargets);
            next_profile = VDP_DECODER_PROFILE_H264_BASELINE;
            break;

        case VDP_DECODER_PROFILE_H264_BASELINE:
            va_profile = VAProfileH264Baseline;
            render_targets.resize(vdp::kNumRenderTargets);
            next_profile = VDP_DECODER_PROFILE_H264_MAIN;
            break;

        case VDP_DECODER_PROFILE_H264_MAIN:
            va_profile = VAProfileH264Main;
            render_targets.resize(vdp::kNumRenderTargets);
            next_profile = VDP_DECODER_PROFILE_H264_HIGH;
            break;

        case VDP_DECODER_PROFILE_H264_HIGH:
            va_profile = VAProfileH264High;
            render_targets.resize(vdp::kNumRenderTargets);
            // there is no more advanced profile, so it's final try
            final_try = true;
            break;

        default:
            traceError("Decoder::Resource::Resource(): decoder %s not implemented\n",
                       reverse_decoder_profile(profile));
            throw vdp::invalid_decoder_profile();
        }

        status = vaCreateConfig(va_dpy, va_profile, VAEntrypointVLD, nullptr, 0,
                                &config_id);
        if (status == VA_STATUS_SUCCESS)        // break loop if decoder created
            break;
    }

    if (status != VA_STATUS_SUCCESS)
        throw vdp::generic_error();

    // Create surfaces. All video surfaces created here, rather than in VdpVideoSurfaceCreate.
    // VAAPI requires surfaces to be bound with context on its creation time, while VDPAU allows
    // to do it later. So here is a trick: VDP video surfaces get their va_surf dynamically in
    // DecoderRender.

    // TODO: check format of surfaces created
#if VA_CHECK_VERSION(0, 34, 0)
    status = vaCreateSurfaces(va_dpy, VA_RT_FORMAT_YUV420, width, height, render_targets.data(),
                              render_targets.size(), nullptr, 0);
#else
    status = vaCreateSurfaces(va_dpy, width, height, VA_RT_FORMAT_YUV420, render_targets.size(),
                              render_targets.data());
#endif
    if (status != VA_STATUS_SUCCESS)
        throw vdp::generic_error();

    status = vaCreateContext(va_dpy, config_id, width, height, VA_PROGRESSIVE,
                             render_targets.data(), render_targets.size(), &context_id);

    if (status != VA_STATUS_SUCCESS)
        throw vdp::generic_error();
}

Resource::~Resource()
{
    try {
        if (device->va_available) {
            const VADisplay va_dpy = device->va_dpy;
            vaDestroySurfaces(va_dpy, render_targets.data(), render_targets.size());
            vaDestroyContext(va_dpy, context_id);
            vaDestroyConfig(va_dpy, config_id);
        }

    } catch (...) {
        traceError("Decoder::Resource::~Resource(): caught exception\n");
    }
}

VdpStatus
CreateImpl(VdpDevice device_id, VdpDecoderProfile profile, uint32_t width, uint32_t height,
           uint32_t max_references, VdpDecoder *decoder)
{
    if (!decoder)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

    auto data = make_shared<Resource>(device, profile, width, height, max_references);

    *decoder = ResourceStorage<Resource>::instance().insert(data);
    return VDP_STATUS_OK;
}

VdpStatus
Create(VdpDevice device_id, VdpDecoderProfile profile, uint32_t width, uint32_t height,
       uint32_t max_references, VdpDecoder *decoder)
{
    return check_for_exceptions(CreateImpl, device_id, profile, width, height, max_references,
                                decoder);
}

VdpStatus
DestroyImpl(VdpDecoder decoder_id)
{
    ResourceRef<Resource> decoder{decoder_id};

    ResourceStorage<Resource>::instance().drop(decoder_id);
    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpDecoder decoder_id)
{
    return check_for_exceptions(DestroyImpl, decoder_id);
}

VdpStatus
GetParametersImpl(VdpDecoder decoder_id, VdpDecoderProfile *profile, uint32_t *width,
                  uint32_t *height)
{
    ResourceRef<Resource> decoder{decoder_id};

    if (profile)
        *profile = decoder->profile;

    if (width)
        *width = decoder->width;

    if (height)
        *height = decoder->height;

    return VDP_STATUS_OK;
}

VdpStatus
GetParameters(VdpDecoder decoder_id, VdpDecoderProfile *profile, uint32_t *width, uint32_t *height)
{
    return check_for_exceptions(GetParametersImpl, decoder_id, profile, width, height);
}

VdpStatus
h264_translate_reference_frames(shared_ptr<vdp::VideoSurface::Resource> &dst_surf,
                                shared_ptr<Resource> &decoder,
                                VAPictureParameterBufferH264 *pic_param,
                                const VdpPictureInfoH264 *vdppi)
{
    // take new VA surface from buffer if needed
    if (dst_surf->va_surf == VA_INVALID_SURFACE) {
        if (decoder->free_list.size() == 0)
            return VDP_STATUS_RESOURCES;

        auto idx = decoder->free_list.back();
        decoder->free_list.pop_back();

        dst_surf->decoder = decoder;
        dst_surf->va_surf = decoder->render_targets[idx];
        dst_surf->rt_idx  = idx;
    }

    // current frame
    pic_param->CurrPic.picture_id   = dst_surf->va_surf;
    pic_param->CurrPic.frame_idx    = vdppi->frame_num;
    pic_param->CurrPic.flags  = vdppi->is_reference ? VA_PICTURE_H264_SHORT_TERM_REFERENCE : 0;

    if (vdppi->field_pic_flag) {
        pic_param->CurrPic.flags |= vdppi->bottom_field_flag ? VA_PICTURE_H264_BOTTOM_FIELD
                                                             : VA_PICTURE_H264_TOP_FIELD;
    }

    pic_param->CurrPic.TopFieldOrderCnt     = vdppi->field_order_cnt[0];
    pic_param->CurrPic.BottomFieldOrderCnt  = vdppi->field_order_cnt[1];

    // mark all pictures invalid preliminary
    for (int k = 0; k < 16; k ++)
        reset_va_picture_h264(&pic_param->ReferenceFrames[k]);

    // reference frames
    for (int k = 0; k < vdppi->num_ref_frames; k ++) {
        if (vdppi->referenceFrames[k].surface == VDP_INVALID_HANDLE) {
            reset_va_picture_h264(&pic_param->ReferenceFrames[k]);
            continue;
        }

        VdpReferenceFrameH264 const *vdp_ref = &vdppi->referenceFrames[k];

        ResourceRef<vdp::VideoSurface::Resource> video_surf{vdp_ref->surface};
        VAPictureH264 *va_ref = &pic_param->ReferenceFrames[k];

        // take new VA surface from buffer if needed
        if (video_surf->va_surf == VA_INVALID_SURFACE) {
            if (decoder->free_list.size() == 0)
                return VDP_STATUS_RESOURCES;

            const auto idx = decoder->free_list.back();
            decoder->free_list.pop_back();

            dst_surf->decoder = decoder;
            dst_surf->va_surf = decoder->render_targets[idx];
            dst_surf->rt_idx  = idx;
        }

        va_ref->picture_id = video_surf->va_surf;
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

template<typename T1, typename T2>
inline void
set_ptr_val(T1 *ptr, T2 val)
{
    if (ptr)
        *ptr = val;
}

VdpStatus
QueryCapabilitiesImpl(VdpDevice device_id, VdpDecoderProfile profile, VdpBool *is_supported,
                      uint32_t *max_level, uint32_t *max_macroblocks, uint32_t *max_width,
                      uint32_t *max_height)
{
    if (!is_supported || !max_level || !max_macroblocks || !max_width || !max_height)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

    set_ptr_val(max_level, 0);
    set_ptr_val(max_macroblocks, 0);
    set_ptr_val(max_width, 0);
    set_ptr_val(max_height, 0);
    set_ptr_val(is_supported, 0);

    if (!device->va_available)
        return VDP_STATUS_OK;

    vector<VAProfile> va_profile_list(vaMaxNumProfiles(device->va_dpy));

    int num_profiles;
    VAStatus status = vaQueryConfigProfiles(device->va_dpy, va_profile_list.data(),
                                            &num_profiles);
    if (status != VA_STATUS_SUCCESS)
        return VDP_STATUS_ERROR;

    struct {
        int mpeg2_simple;
        int mpeg2_main;
        int h264_baseline;
        int h264_main;
        int h264_high;
        int vc1_simple;
        int vc1_main;
        int vc1_advanced;
    } available_profiles = {};

    for (int k = 0; k < num_profiles; k ++) {
        switch (va_profile_list[k]) {
        case VAProfileMPEG2Main:
            available_profiles.mpeg2_main = 0;
            // fall through

        case VAProfileMPEG2Simple:
            available_profiles.mpeg2_simple = 0;
            break;

        case VAProfileH264High:
            available_profiles.h264_high = 1;
            // fall through

        case VAProfileH264Main:
            available_profiles.h264_main = 1;
            // fall through

        case VAProfileH264Baseline:
            available_profiles.h264_baseline = 1;
            // fall though

        case VAProfileH264ConstrainedBaseline:
            break;

        case VAProfileVC1Advanced:
            available_profiles.vc1_advanced = 0;
            // fall though

        case VAProfileVC1Main:
            available_profiles.vc1_main = 0;
            // fall though

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

    // TODO: How to determine max width and height width libva?
    set_ptr_val(max_width, 2048);
    set_ptr_val(max_height, 2048);
    set_ptr_val(max_macroblocks, 16384);

    switch (profile) {
    case VDP_DECODER_PROFILE_MPEG2_SIMPLE:
        set_ptr_val(is_supported, available_profiles.mpeg2_simple);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_MPEG2_HL);
        break;

    case VDP_DECODER_PROFILE_MPEG2_MAIN:
        set_ptr_val(is_supported, available_profiles.mpeg2_main);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_MPEG2_HL);
        break;

    case VDP_DECODER_PROFILE_H264_CONSTRAINED_BASELINE:
        set_ptr_val(is_supported, available_profiles.h264_baseline ||
                                  available_profiles.h264_main);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_H264_5_1);
        break;

    case VDP_DECODER_PROFILE_H264_BASELINE:
        set_ptr_val(is_supported, available_profiles.h264_baseline);
        // TODO: Does underlying libva really support 5.1?
        set_ptr_val(max_level, VDP_DECODER_LEVEL_H264_5_1);
        break;

    case VDP_DECODER_PROFILE_H264_MAIN:
        set_ptr_val(is_supported, available_profiles.h264_main);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_H264_5_1);
        break;

    case VDP_DECODER_PROFILE_H264_HIGH:
        set_ptr_val(is_supported, available_profiles.h264_high);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_H264_5_1);
        break;

    case VDP_DECODER_PROFILE_VC1_SIMPLE:
        set_ptr_val(is_supported, available_profiles.vc1_simple);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_VC1_SIMPLE_MEDIUM);
        break;

    case VDP_DECODER_PROFILE_VC1_MAIN:
        set_ptr_val(is_supported, available_profiles.vc1_main);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_VC1_MAIN_HIGH);
        break;

    case VDP_DECODER_PROFILE_VC1_ADVANCED:
        set_ptr_val(is_supported, available_profiles.vc1_advanced);
        set_ptr_val(max_level, VDP_DECODER_LEVEL_VC1_ADVANCED_L4);
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
QueryCapabilities(VdpDevice device_id, VdpDecoderProfile profile, VdpBool *is_supported,
                  uint32_t *max_level, uint32_t *max_macroblocks, uint32_t *max_width,
                  uint32_t *max_height)
{
    return check_for_exceptions(QueryCapabilitiesImpl, device_id, profile, is_supported, max_level,
                                max_macroblocks, max_width, max_height);
}

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
        pic_param->num_slice_groups_minus1              = 0; // TODO: vdppi->slice_count - 1; ???

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

VdpStatus
Render_h264(shared_ptr<Resource> decoder, shared_ptr<vdp::VideoSurface::Resource> dst_surf,
            VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
            VdpBitstreamBuffer const *bitstream_buffers)
{
    VADisplay va_dpy = decoder->device->va_dpy;
    VAStatus status;
    const auto *vdppi = static_cast<VdpPictureInfoH264 const *>(picture_info);

    // TODO: figure out where to get level
    const uint32_t level = 41;

    // preparing picture parameters and IQ matrix
    VAPictureParameterBufferH264 pic_param = {};
    VAIQMatrixBufferH264 iq_matrix;

    const auto vs = h264_translate_reference_frames(dst_surf, decoder, &pic_param, vdppi);
    if (vs != VDP_STATUS_OK) {
        if (vs == VDP_STATUS_RESOURCES) {
            traceError("Decoder::Render_h264(): no surfaces left in buffer\n");
            return VDP_STATUS_RESOURCES;
        }

        return VDP_STATUS_ERROR;
    }

    h264_translate_pic_param(&pic_param, decoder->width, decoder->height, vdppi, level);
    h264_translate_iq_matrix(&iq_matrix, vdppi);

    {
        GLXLockGuard guard;
        VABufferID pic_param_buf, iq_matrix_buf;


        status = vaCreateBuffer(va_dpy, decoder->context_id, VAPictureParameterBufferType,
                                sizeof(VAPictureParameterBufferH264), 1, &pic_param,
                                &pic_param_buf);

        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        status = vaCreateBuffer(va_dpy, decoder->context_id, VAIQMatrixBufferType,
                                sizeof(VAIQMatrixBufferH264), 1, &iq_matrix, &iq_matrix_buf);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        // send data to decoding hardware
        status = vaBeginPicture(va_dpy, decoder->context_id, dst_surf->va_surf);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        status = vaRenderPicture(va_dpy, decoder->context_id, &pic_param_buf, 1);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        status = vaRenderPicture(va_dpy, decoder->context_id, &iq_matrix_buf, 1);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        vaDestroyBuffer(va_dpy, pic_param_buf);
        vaDestroyBuffer(va_dpy, iq_matrix_buf);
    }

    // merge bitstream buffers
    vector<uint8_t> merged_bitstream;

    for (uint32_t k = 0; k < bitstream_buffer_count; k ++) {
        const auto *buf = static_cast<const uint8_t *>(bitstream_buffers[k].bitstream);
        const auto buf_len = bitstream_buffers[k].bitstream_bytes;

        merged_bitstream.insert(merged_bitstream.end(), buf, buf + buf_len);
    }

    // Slice parameters

    // All slice data have been merged into one continuous buffer. But we must supply
    // slices one by one to the hardware decoder, so we need to delimit them. VDPAU
    // requires bitstream buffers to include slice start code (0x00 0x00 0x01). Those
    // will be used to calculate offsets and sizes of slice data in code below.

    RBSPState st_g{merged_bitstream};      // reference, global state
    int64_t nal_offset;

    try {
        nal_offset = st_g.navigate_to_nal_unit();

    } catch (const RBSPState::error &) {
        traceError("Decoder::Render_h264(): no NAL header\n");
        return VDP_STATUS_ERROR;
    }

    do {
        VASliceParameterBufferH264 sp_h264 = {};

        // make a copy of global rbsp state for using in slice header parser
        RBSPState st{st_g};
        int64_t nal_offset_next;

        st.reset_bit_counter();

        try {
            nal_offset_next = st_g.navigate_to_nal_unit();

        } catch (const RBSPState::error &) {
            nal_offset_next = -1;
        }

        // calculate end of current slice. Note (-3). It's slice start code length.
        const unsigned int end_pos = (nal_offset_next > 0) ? (nal_offset_next - 3)
                                                           : merged_bitstream.size();
        sp_h264.slice_data_size     = end_pos - nal_offset;
        sp_h264.slice_data_offset   = 0;
        sp_h264.slice_data_flag     = VA_SLICE_DATA_FLAG_ALL;

        // TODO: this may be not entirely true for YUV444
        // but if we limiting to YUV420, that's ok
        int ChromaArrayType = pic_param.seq_fields.bits.chroma_format_idc;

        // parse slice header and use its data to fill slice parameter buffer
        parse_slice_header(st, &pic_param, ChromaArrayType, vdppi->num_ref_idx_l0_active_minus1,
                           vdppi->num_ref_idx_l1_active_minus1, &sp_h264);

        VABufferID slice_parameters_buf;

        GLXLockGuard guard;

        status = vaCreateBuffer(va_dpy, decoder->context_id, VASliceParameterBufferType,
                                sizeof(VASliceParameterBufferH264), 1, &sp_h264,
                                &slice_parameters_buf);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        status = vaRenderPicture(va_dpy, decoder->context_id, &slice_parameters_buf, 1);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        VABufferID slice_buf;
        status = vaCreateBuffer(va_dpy, decoder->context_id, VASliceDataBufferType,
                                sp_h264.slice_data_size, 1, merged_bitstream.data() + nal_offset,
                                &slice_buf);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        status = vaRenderPicture(va_dpy, decoder->context_id, &slice_buf, 1);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;

        vaDestroyBuffer(va_dpy, slice_parameters_buf);
        vaDestroyBuffer(va_dpy, slice_buf);

        if (nal_offset_next < 0)        // nal_offset_next equals -1 when there is no slice
            break;                      // start code found. Thus that was the final slice.
        nal_offset = nal_offset_next;
    } while (1);

    {
        GLXLockGuard guard;
        status = vaEndPicture(va_dpy, decoder->context_id);
        if (status != VA_STATUS_SUCCESS)
            return VDP_STATUS_ERROR;
    }

    dst_surf->sync_va_to_glx = true;
    return VDP_STATUS_OK;
}

VdpStatus
RenderImpl(VdpDecoder decoder_id, VdpVideoSurface target, VdpPictureInfo const *picture_info,
           uint32_t bitstream_buffer_count, VdpBitstreamBuffer const *bitstream_buffers)
{
    if (not picture_info || not bitstream_buffers)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<Resource> decoder{decoder_id};
    ResourceRef<vdp::VideoSurface::Resource> dst_surf{target};

    if (decoder->profile == VDP_DECODER_PROFILE_H264_CONSTRAINED_BASELINE ||
        decoder->profile == VDP_DECODER_PROFILE_H264_BASELINE ||
        decoder->profile == VDP_DECODER_PROFILE_H264_MAIN ||
        decoder->profile == VDP_DECODER_PROFILE_H264_HIGH)
    {
        // TODO: check exit code
        Render_h264(decoder, dst_surf, picture_info, bitstream_buffer_count, bitstream_buffers);
    } else {
        traceError("Decoder::RenderImpl(): no implementation for profile %s\n",
                   reverse_decoder_profile(decoder->profile));
        return VDP_STATUS_NO_IMPLEMENTATION;
    }

    return VDP_STATUS_OK;
}

VdpStatus
Render(VdpDecoder decoder_id, VdpVideoSurface target, VdpPictureInfo const *picture_info,
       uint32_t bitstream_buffer_count, VdpBitstreamBuffer const *bitstream_buffers)
{
    return check_for_exceptions(RenderImpl, decoder_id, target, picture_info,
                                bitstream_buffer_count, bitstream_buffers);
}

} } // namespace vdp::Decoder
