/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include "api.h"
#include "trace.h"
#include "reverse-constant.h"

static FILE *tlog = NULL;  ///< trace target
static const char *trace_header =       "[VS] ";
static const char *trace_header_blank = "     ";
static int trace_enabled = 1;
static void (*trace_hook)(void *, void *, int, int);
static void *trace_hook_longterm_param = NULL;

void
traceEnableTracing(int flag)
{
    trace_enabled = !!flag;
}

void
traceSetTarget(FILE *target)
{
    tlog = target;
}

void
traceResetTarget(void)
{
    tlog = stdout;
}

void
traceSetHook(void (*hook)(void *param1, void *param2, int origin, int after), void *param)
{
    trace_hook = hook;
    trace_hook_longterm_param = param;
}

void
traceCallHook(int origin, int after, void *shortterm_param)
{
    if (!trace_enabled)
        return;
    if (trace_hook)
        trace_hook(trace_hook_longterm_param, shortterm_param, origin, after);
}

void
traceSetHeader(const char *header, const char *header_blank)
{
    trace_header = header;
    trace_header_blank = header_blank;
}

void
traceInfo(const char *fmt, ...)
{
    if (!trace_enabled)
        return;
    va_list args;
    traceCallHook(-2, 0, NULL);
    fprintf(tlog, "%s", trace_header);
    va_start(args, fmt);
    vfprintf(tlog, fmt, args);
    va_end(args);
}

void
traceError(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "%s", trace_header);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

static
const char *
rect2string(VdpRect const *rect)
{
    // use buffer pool to enable printing many rects in one printf expression
    static char bufs[8][100];
    static int i_ptr = 0;
    i_ptr = (i_ptr + 1) % 8;
    char *buf = &bufs[i_ptr][0];
    if (NULL == rect) {
        snprintf(buf, 100, "NULL");
    } else {
        snprintf(buf, 100, "(%d,%d,%d,%d)", rect->x0, rect->y0, rect->x1, rect->y1);
    }
    return buf;
}

VdpStatus
traceVdpGetApiVersion(uint32_t *api_version)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_GET_API_VERSION, 0, NULL);
    fprintf(tlog, "%s%s VdpGetApiVersion\n", trace_header, impl_state);
skip:;
    VdpStatus ret = vdpGetApiVersion(api_version);
    traceCallHook(VDP_FUNC_ID_GET_API_VERSION, 1, (void *)ret);
    return ret;
}

VdpStatus
traceVdpDecoderQueryCapabilities(VdpDevice device,
                                 VdpDecoderProfile profile, VdpBool *is_supported,
                                 uint32_t *max_level, uint32_t *max_macroblocks,
                                 uint32_t *max_width, uint32_t *max_height)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpDecoderQueryCapabilities device=%d, profile=%s(%d)\n",
            trace_header, impl_state, device, reverse_decoder_profile(profile), profile);
skip:;
    VdpStatus ret = vdpDecoderQueryCapabilities(device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    traceCallHook(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile,
                      uint32_t width, uint32_t height, uint32_t max_references, VdpDecoder *decoder)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_DECODER_CREATE, 0, NULL);
    fprintf(tlog, "%s%s VdpDecoderCreate device=%d, profile=%s, width=%d, height=%d, "
        "max_references=%d\n", trace_header, impl_state, device, reverse_decoder_profile(profile),
        width, height, max_references);
skip:;
    VdpStatus ret = vdpDecoderCreate(device, profile, width, height, max_references, decoder);
    traceCallHook(VDP_FUNC_ID_DECODER_CREATE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpDecoderDestroy(VdpDecoder decoder)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_DECODER_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpDecoderDestroy decoder=%d\n", trace_header, impl_state, decoder);
skip:;
    VdpStatus ret = vdpDecoderDestroy(decoder);
    traceCallHook(VDP_FUNC_ID_DECODER_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpDecoderGetParameters(VdpDecoder decoder,
                             VdpDecoderProfile *profile, uint32_t *width, uint32_t *height)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_DECODER_GET_PARAMETERS, 0, NULL);
    fprintf(tlog, "%s%s VdpDecoderGetParameters decoder=%d\n", trace_header, impl_state, decoder);
skip:;
    VdpStatus ret = vdpDecoderGetParameters(decoder, profile, width, height);
    traceCallHook(VDP_FUNC_ID_DECODER_GET_PARAMETERS, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                      VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                      VdpBitstreamBuffer const *bitstream_buffers)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_DECODER_RENDER, 0, NULL);
    fprintf(tlog, "%s%s VdpDecoderRender decoder=%d, target=%d, picture_info=%p, "
        "bitstream_buffer_count=%d\n", trace_header, impl_state, decoder, target, picture_info,
        bitstream_buffer_count);
skip:;
    VdpStatus ret = vdpDecoderRender(decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    traceCallHook(VDP_FUNC_ID_DECODER_RENDER, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceQueryCapabilities(VdpDevice device,
                                       VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                                       uint32_t *max_width, uint32_t *max_height)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryCapabilities device=%d, surface_rgba_format=%s\n",
        trace_header, impl_state, device, reverse_rgba_format(surface_rgba_format));
skip:;
    VdpStatus ret = vdpOutputSurfaceQueryCapabilities(device, surface_rgba_format, is_supported,
                                                          max_width, max_height);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                       VdpRGBAFormat surface_rgba_format,
                                                       VdpBool *is_supported)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryGetPutBitsNativeCapabilities device=%d, "
        "surface_rgba_format=%s\n", trace_header, impl_state, device,
        reverse_rgba_format(surface_rgba_format));
skip:;
    VdpStatus ret =
        vdpOutputSurfaceQueryGetPutBitsNativeCapabilities(device, surface_rgba_format,
                                                              is_supported);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                     VdpRGBAFormat surface_rgba_format,
                                                     VdpIndexedFormat bits_indexed_format,
                                                     VdpColorTableFormat color_table_format,
                                                     VdpBool *is_supported)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryPutBitsIndexedCapabilities device=%d, "
        "surface_rgba_format=%s, bits_indexed_format=%s, color_table_format=%s\n",
        trace_header, impl_state, device, reverse_rgba_format(surface_rgba_format),
        reverse_indexed_format(bits_indexed_format),
        reverse_color_table_format(color_table_format));
skip:;
    VdpStatus ret = vdpOutputSurfaceQueryPutBitsIndexedCapabilities(device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                   VdpRGBAFormat surface_rgba_format,
                                                   VdpYCbCrFormat bits_ycbcr_format,
                                                   VdpBool *is_supported)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryPutBitsYCbCrCapabilities device=%d, "
        "surface_rgba_format=%s, bits_ycbcr_format=%s\n", trace_header, impl_state,
        device, reverse_rgba_format(surface_rgba_format), reverse_ycbcr_format(bits_ycbcr_format));
skip:;
    VdpStatus ret = vdpOutputSurfaceQueryPutBitsYCbCrCapabilities(device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format,
                            uint32_t width, uint32_t height, VdpOutputSurface *surface)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d\n",
        trace_header, impl_state, device, reverse_rgba_format(rgba_format), width, height);
skip:;
    VdpStatus ret = vdpOutputSurfaceCreate(device, rgba_format, width, height, surface);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceDestroy surface=%d\n", trace_header, impl_state, surface);
skip:;
    VdpStatus ret = vdpOutputSurfaceDestroy(surface);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceGetParameters(VdpOutputSurface surface,
                                   VdpRGBAFormat *rgba_format, uint32_t *width, uint32_t *height)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceGetParameters surface=%d\n", trace_header, impl_state,
        surface);
skip:;
    VdpStatus ret = vdpOutputSurfaceGetParameters(surface, rgba_format, width, height);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface,
                                   VdpRect const *source_rect, void *const *destination_data,
                                   uint32_t const *destination_pitches)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceGetBitsNative surface=%d, source_rect=%s\n",
        trace_header, impl_state, surface, rect2string(source_rect));
skip:;
    VdpStatus ret = vdpOutputSurfaceGetBitsNative(surface, source_rect, destination_data,
        destination_pitches);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfacePutBitsNative(VdpOutputSurface surface,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfacePutBitsNative surface=%d, destination_rect=%s\n",
        trace_header, impl_state, surface, rect2string(destination_rect));
skip:;
    VdpStatus ret = vdpOutputSurfacePutBitsNative(surface, source_data, source_pitches,
        destination_rect);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface,
                                    VdpIndexedFormat source_indexed_format,
                                    void const *const *source_data, uint32_t const *source_pitch,
                                    VdpRect const *destination_rect,
                                    VdpColorTableFormat color_table_format, void const *color_table)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfacePutBitsIndexed surface=%d, source_indexed_format=%s, "
        "destination_rect=%s, color_table_format=%s\n", trace_header, impl_state, surface,
        reverse_indexed_format(source_indexed_format), rect2string(destination_rect),
        reverse_color_table_format(color_table_format));
skip:;
    VdpStatus ret = vdpOutputSurfacePutBitsIndexed(surface, source_indexed_format, source_data,
        source_pitch, destination_rect, color_table_format, color_table);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface,
                                  VdpYCbCrFormat source_ycbcr_format,
                                  void const *const *source_data, uint32_t const *source_pitches,
                                  VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfacePutBitsYCbCr surface=%d, source_ycbcr_format=%s, "
        "destination_rect=%s, csc_matrix=%p\n", trace_header, impl_state, surface,
        reverse_ycbcr_format(source_ycbcr_format), rect2string(destination_rect), csc_matrix);
skip:;
    VdpStatus ret = vdpOutputSurfacePutBitsYCbCr(surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerQueryFeatureSupport(VdpDevice device,
                                      VdpVideoMixerFeature feature, VdpBool *is_supported)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerQueryFeatureSupport device=%d, feature=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_feature(feature));
skip:;
    VdpStatus ret = vdpVideoMixerQueryFeatureSupport(device, feature, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerQueryParameterSupport(VdpDevice device,
                                        VdpVideoMixerParameter parameter,
                                        VdpBool *is_supported)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerQueryParameterSupport device=%d, parameter=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_parameter(parameter));
skip:;
    VdpStatus ret = vdpVideoMixerQueryParameterSupport(device, parameter, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerQueryAttributeSupport(VdpDevice device,
                                        VdpVideoMixerAttribute attribute, VdpBool *is_supported)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerQueryAttributeSupport device=%d, attribute=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_attribute(attribute));
skip:;
    VdpStatus ret = vdpVideoMixerQueryAttributeSupport(device, attribute, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerQueryParameterValueRange(VdpDevice device,
                                           VdpVideoMixerParameter parameter,
                                           void *min_value, void *max_value)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerQueryParameterValueRange device=%d, parameter=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_parameter(parameter));
skip:;
    VdpStatus ret = vdpVideoMixerQueryParameterValueRange(device, parameter, min_value,
                                                              max_value);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerQueryAttributeValueRange(VdpDevice device,
                                           VdpVideoMixerAttribute attribute,
                                           void *min_value, void *max_value)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerQueryAttributeValueRange device=%d, attribute=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_attribute(attribute));
skip:;
    VdpStatus ret = vdpVideoMixerQueryAttributeValueRange(device, attribute, min_value,
                                                              max_value);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                         VdpVideoMixerFeature const *features, uint32_t parameter_count,
                         VdpVideoMixerParameter const *parameters,
                         void const *const *parameter_values, VdpVideoMixer *mixer)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_CREATE, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerCreate device=%d, feature_count=%d, parameter_count=%d\n",
        trace_header, impl_state, device, feature_count, parameter_count);
    for (uint32_t k = 0; k < feature_count; k ++)
        fprintf(tlog, "%s      feature %s\n", trace_header_blank,
            reverse_video_mixer_feature(features[k]));
    for (uint32_t k = 0; k < parameter_count; k ++) {
        fprintf(tlog, "%s      parameter ", trace_header_blank);
        switch (parameters[k]) {
        case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
            fprintf(tlog, "video surface width = %d\n", *(uint32_t*)parameter_values[k]);
            break;
        case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
            fprintf(tlog, "video surface height = %d\n", *(uint32_t*)parameter_values[k]);
            break;
        case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
            fprintf(tlog, "chroma type = %s\n",
                reverse_chroma_type(*(uint32_t*)parameter_values[k]));
            break;
        case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
            fprintf(tlog, "layers = %d\n", *(uint32_t*)parameter_values[k]);
            break;
        default:
            fprintf(tlog, "invalid\n");
            break;
        }
    }
skip:;
    VdpStatus ret = vdpVideoMixerCreate(device, feature_count, features, parameter_count,
                                            parameters, parameter_values, mixer);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_CREATE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool const *feature_enables)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerSetFeatureEnables mixer=%d, feature_count=%d\n",
        trace_header, impl_state, mixer, feature_count);
    for (uint32_t k = 0; k < feature_count; k ++) {
        fprintf(tlog, "%s      feature %d (%s) %s\n", trace_header_blank,
            features[k], reverse_video_mixer_feature(features[k]),
            feature_enables[k] ? "enabled" : "disabled");
    }
skip:;
    VdpStatus ret = vdpVideoMixerSetFeatureEnables(mixer, feature_count, features,
                                                       feature_enables);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer,
                                     uint32_t attribute_count,
                                     VdpVideoMixerAttribute const *attributes,
                                     void const *const *attribute_values)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerSetAttributeValues mixer=%d, attribute_count=%d\n",
        trace_header, impl_state, mixer, attribute_count);
    for (uint32_t k = 0; k < attribute_count; k ++) {
        fprintf(tlog, "%s   attribute %d (%s)\n", trace_header_blank, attributes[k],
            reverse_video_mixer_attribute(attributes[k]));
        if (VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX == attributes[k]) {
            VdpCSCMatrix *matrix = (VdpCSCMatrix *)(attribute_values[k]);
            for (uint32_t j1 = 0; j1 < 3; j1 ++) {
                fprintf(tlog, "%s      ", trace_header_blank);
                for (uint32_t j2 = 0; j2 < 4; j2 ++) {
                    fprintf(tlog, "%11f", (double)((*matrix)[j1][j2]));
                }
                fprintf(tlog, "\n");
            }
        }
    }
skip:;
    VdpStatus ret = vdpVideoMixerSetAttributeValues(mixer, attribute_count, attributes,
        attribute_values);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool *feature_supports)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerGetFeatureSupport mixer=%d, feature_count=%d\n",
        trace_header, impl_state, mixer, feature_count);
    for (unsigned int k = 0; k < feature_count; k ++)
        fprintf(tlog, "%s      feature %s\n", trace_header_blank,
            reverse_video_mixer_feature(features[k]));
skip:;
    VdpStatus ret = vdpVideoMixerGetFeatureSupport(mixer, feature_count, features,
        feature_supports);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool *feature_enables)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerGetFeatureEnables mixer=%d, feature_count=%d\n",
        trace_header, impl_state, mixer, feature_count);
    for (unsigned int k = 0; k < feature_count; k ++)
        fprintf(tlog, "%s      feature %s\n", trace_header_blank,
            reverse_video_mixer_feature(features[k]));
skip:;
    VdpStatus ret = vdpVideoMixerGetFeatureEnables(mixer, feature_count, features,
                                                       feature_enables);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerGetParameterValues(VdpVideoMixer mixer,
                                     uint32_t parameter_count,
                                     VdpVideoMixerParameter const *parameters,
                                     void *const *parameter_values)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerGetParameterValues mixer=%d, parameter_count=%d\n",
        trace_header, impl_state, mixer, parameter_count);
    for (unsigned int k = 0; k < parameter_count; k ++)
        fprintf(tlog, "%s      parameter %s\n", trace_header_blank,
            reverse_video_mixer_parameter(parameters[k]));
skip:;
    VdpStatus ret = vdpVideoMixerGetParameterValues(mixer, parameter_count, parameters,
        parameter_values);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer,
                                     uint32_t attribute_count,
                                     VdpVideoMixerAttribute const *attributes,
                                     void *const *attribute_values)
{
    const char *impl_state = "{zilch}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerGetAttributeValues mixer=%d, attribute_count=%d\n",
        trace_header, impl_state, mixer, attribute_count);
    for (unsigned int k = 0; k < attribute_count; k ++)
        fprintf(tlog, "%s      attribute %s\n", trace_header_blank,
            reverse_video_mixer_attribute(attributes[k]));
skip:;
    VdpStatus ret = vdpVideoMixerGetAttributeValues(mixer, attribute_count, attributes,
        attribute_values);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerDestroy mixer=%d\n", trace_header, impl_state, mixer);
skip:;
    VdpStatus ret = vdpVideoMixerDestroy(mixer);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoMixerRender(VdpVideoMixer mixer,
                         VdpOutputSurface background_surface, VdpRect const *background_source_rect,
                         VdpVideoMixerPictureStructure current_picture_structure,
                         uint32_t video_surface_past_count,
                         VdpVideoSurface const *video_surface_past,
                         VdpVideoSurface video_surface_current, uint32_t video_surface_future_count,
                         VdpVideoSurface const *video_surface_future,
                         VdpRect const *video_source_rect, VdpOutputSurface destination_surface,
                         VdpRect const *destination_rect, VdpRect const *destination_video_rect,
                         uint32_t layer_count, VdpLayer const *layers)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_RENDER, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoMixerRender mixer=%d, background_surface=%d, "
        "background_source_rect=%s,\n", trace_header, impl_state,
        mixer, background_surface, rect2string(background_source_rect));
    fprintf(tlog, "%s      current_picture_structure=%s, video_surface_past=[",
        trace_header_blank, reverser_video_mixer_picture_structure(current_picture_structure));
    for (uint32_t k = 0; k < video_surface_past_count; k ++) {
        if (0 != k) fprintf(tlog, ",");
        fprintf(tlog, "%d", video_surface_past[k]);
    }
    fprintf(tlog, "],\n%s      video_surface_current=%d, video_surface_future=[",
        trace_header_blank, video_surface_current);
    for (uint32_t k = 0; k < video_surface_future_count; k ++) {
        if (0 != k) fprintf(tlog, ",");
        fprintf(tlog, "%d", video_surface_future[k]);
    }
    fprintf(tlog, "],\n%s      video_source_rect=%s, destination_surface=%d, destination_rect=%s, "
         "destination_video_rect=%s, layers=[", trace_header_blank, rect2string(video_source_rect),
         destination_surface, rect2string(destination_rect), rect2string(destination_video_rect));
    for (uint32_t k = 0; k < layer_count; k ++) {
        if (0 != k) fprintf(tlog, ",");
        fprintf(tlog, "{%d,src:%s,dst:%s}", layers[k].source_surface,
            rect2string(layers[k].source_rect), rect2string(layers[k].destination_rect));
    }
    fprintf(tlog, "]\n");
skip:;
    VdpStatus ret = vdpVideoMixerRender(mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_RENDER, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueTargetDestroy presentation_queue_target=%d\n",
        trace_header, impl_state, presentation_queue_target);
skip:;
    VdpStatus ret = vdpPresentationQueueTargetDestroy(presentation_queue_target);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueCreate(VdpDevice device,
                                VdpPresentationQueueTarget presentation_queue_target,
                                VdpPresentationQueue *presentation_queue)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueCreate device=%d, presentation_queue_target=%d\n",
        trace_header, impl_state, device, presentation_queue_target);
skip:;
    VdpStatus ret = vdpPresentationQueueCreate(device, presentation_queue_target,
        presentation_queue);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueDestroy presentation_queue=%d\n",
        trace_header, impl_state, presentation_queue);
skip:;
    VdpStatus ret = vdpPresentationQueueDestroy(presentation_queue);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                            VdpColor *const background_color)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueSetBackgroundColor presentation_queue=%d, "
            "background_color=", trace_header, impl_state, presentation_queue);
    if (background_color) {
        fprintf(tlog, "(%.2f,%.2f,%.2f,%.2f)\n", background_color->red,
                background_color->green, background_color->blue, background_color->alpha);
    } else {
        fprintf(tlog, "NULL\n");
    }
skip:;
    VdpStatus ret = vdpPresentationQueueSetBackgroundColor(presentation_queue,
                                                               background_color);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                            VdpColor *background_color)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueGetBackgroundColor  presentation_queue=%d\n",
        trace_header, impl_state, presentation_queue);
skip:;
    VdpStatus ret = vdpPresentationQueueGetBackgroundColor(presentation_queue,
                                                               background_color);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                                 VdpTime *current_time)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueGetTime presentation_queue=%d\n",
        trace_header, impl_state, presentation_queue);
skip:;
    VdpStatus ret = vdpPresentationQueueGetTime(presentation_queue, current_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue,
                                 VdpOutputSurface surface, uint32_t clip_width,
                                 uint32_t clip_height, VdpTime earliest_presentation_time)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueDisplay presentation_queue=%d, surface=%d, "
        "clip_width=%d, clip_height=%d,\n", trace_header, impl_state, presentation_queue, surface,
        clip_width, clip_height);
    fprintf(tlog, "%s      earliest_presentation_time=%"PRIu64"\n", trace_header_blank,
        earliest_presentation_time);
skip:;
    VdpStatus ret = vdpPresentationQueueDisplay(presentation_queue, surface, clip_width,
                                                    clip_height, earliest_presentation_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                               VdpOutputSurface surface,
                                               VdpTime *first_presentation_time)

{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueBlockUntilSurfaceIdle presentation_queue=%d, "
        "surface=%d\n", trace_header, impl_state, presentation_queue, surface);
skip:;
    VdpStatus ret = vdpPresentationQueueBlockUntilSurfaceIdle(presentation_queue, surface,
        first_presentation_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                            VdpOutputSurface surface,
                                            VdpPresentationQueueStatus *status,
                                            VdpTime *first_presentation_time)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueQuerySurfaceStatus presentation_queue=%d, "
        "surface=%d\n", trace_header, impl_state, presentation_queue, surface);
skip:;
    VdpStatus ret = vdpPresentationQueueQuerySurfaceStatus(presentation_queue, surface,
        status, first_presentation_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfaceQueryCapabilities(VdpDevice device,
                                      VdpChromaType surface_chroma_type, VdpBool *is_supported,
                                      uint32_t *max_width, uint32_t *max_height)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfaceQueryCapabilities device=%d, surface_chroma_type=%s\n",
        trace_header, impl_state, device, reverse_chroma_type(surface_chroma_type));
skip:;
    VdpStatus ret = vdpVideoSurfaceQueryCapabilities(device, surface_chroma_type, is_supported,
        max_width, max_height);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                     VdpChromaType surface_chroma_type,
                                                     VdpYCbCrFormat bits_ycbcr_format,
                                                     VdpBool *is_supported)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities device=%d, "
        "surface_chroma_type=%s, bits_ycbcr_format=%s\n", trace_header, impl_state,
        device, reverse_chroma_type(surface_chroma_type), reverse_ycbcr_format(bits_ycbcr_format));
skip:;
    VdpStatus ret = vdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(device, surface_chroma_type,
        bits_ycbcr_format, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type,
                           uint32_t width, uint32_t height, VdpVideoSurface *surface)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfaceCreate, device=%d, chroma_type=%s, width=%d, height=%d\n",
        trace_header, impl_state, device, reverse_chroma_type(chroma_type), width, height);
skip:;
    VdpStatus ret = vdpVideoSurfaceCreate(device, chroma_type, width, height, surface);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfaceDestroy surface=%d\n", trace_header, impl_state, surface);
skip:;
    VdpStatus ret = vdpVideoSurfaceDestroy(surface);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfaceGetParameters(VdpVideoSurface surface,
                                  VdpChromaType *chroma_type, uint32_t *width, uint32_t *height)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfaceGetParameters surface=%d\n", trace_header, impl_state,
        surface);
skip:;
    VdpStatus ret = vdpVideoSurfaceGetParameters(surface, chroma_type, width, height);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface,
                                 VdpYCbCrFormat destination_ycbcr_format,
                                 void *const *destination_data, uint32_t const *destination_pitches)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfaceGetBitsYCbCr surface=%d, destination_ycbcr_format=%s\n",
        trace_header, impl_state, surface, reverse_ycbcr_format(destination_ycbcr_format));
skip:;
    VdpStatus ret = vdpVideoSurfaceGetBitsYCbCr(surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface,
                                 VdpYCbCrFormat source_ycbcr_format, void const *const *source_data,
                                 uint32_t const *source_pitches)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR, 0, NULL);
    fprintf(tlog, "%s%s VdpVideoSurfacePutBitsYCbCr surface=%d, source_ycbcr_format=%s\n",
        trace_header, impl_state, surface, reverse_ycbcr_format(source_ycbcr_format));
skip:;
    VdpStatus ret = vdpVideoSurfacePutBitsYCbCr(surface, source_ycbcr_format, source_data,
        source_pitches);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpBitmapSurfaceQueryCapabilities(VdpDevice device,
                                       VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                                       uint32_t *max_width, uint32_t *max_height)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, 0, NULL);
    fprintf(tlog, "%s%s VdpBitmapSurfaceQueryCapabilities device=%d, surface_rgba_format=%s\n",
        trace_header, impl_state, device, reverse_rgba_format(surface_rgba_format));
skip:;
    VdpStatus ret = vdpBitmapSurfaceQueryCapabilities(device, surface_rgba_format, is_supported,
        max_width, max_height);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format,
                            uint32_t width, uint32_t height, VdpBool frequently_accessed,
                            VdpBitmapSurface *surface)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_CREATE, 0, NULL);
    fprintf(tlog, "%s%s VdpBitmapSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d,\n"
        "%s      frequently_accessed=%d\n", trace_header, impl_state, device,
        reverse_rgba_format(rgba_format), width, height, trace_header_blank, frequently_accessed);
skip:;
    VdpStatus ret = vdpBitmapSurfaceCreate(device, rgba_format, width, height,
                                               frequently_accessed, surface);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_CREATE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpBitmapSurfaceDestroy surface=%d\n", trace_header, impl_state, surface);
skip:;
    VdpStatus ret = vdpBitmapSurfaceDestroy(surface);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface,
                                   VdpRGBAFormat *rgba_format, uint32_t *width, uint32_t *height,
                                   VdpBool *frequently_accessed)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS, 0, NULL);
    fprintf(tlog, "%s%s VdpBitmapSurfaceGetParameters surface=%d\n",
        trace_header, impl_state, surface);
skip:;
    VdpStatus ret = vdpBitmapSurfaceGetParameters(surface, rgba_format, width, height,
        frequently_accessed);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, 0, NULL);
    fprintf(tlog, "%s%s VdpBitmapSurfacePutBitsNative surface=%d, destination_rect=%s\n",
        trace_header, impl_state, surface, rect2string(destination_rect));
skip:;
    VdpStatus ret = vdpBitmapSurfacePutBitsNative(surface, source_data, source_pitches,
        destination_rect);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpDeviceDestroy(VdpDevice device)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_DEVICE_DESTROY, 0, NULL);
    fprintf(tlog, "%s%s VdpDeviceDestroy device=%d\n", trace_header, impl_state, device);
skip:;
    VdpStatus ret = vdpDeviceDestroy(device);
    traceCallHook(VDP_FUNC_ID_DEVICE_DESTROY, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpGetInformationString(char const **information_string)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_GET_INFORMATION_STRING, 0, NULL);
    fprintf(tlog, "%s%s VdpGetInformationString\n", trace_header, impl_state);
skip:;
    VdpStatus ret = vdpGetInformationString(information_string);
    traceCallHook(VDP_FUNC_ID_GET_INFORMATION_STRING, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard,
                          VdpCSCMatrix *csc_matrix)
{
    const char *impl_state = "{part}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_GENERATE_CSC_MATRIX, 0, NULL);
    fprintf(tlog, "%s%s VdpGenerateCSCMatrix ", trace_header, impl_state);
    if (procamp) {
        fprintf(tlog, "brightness=%f, contrast=%f, saturation=%f, ", procamp->brightness,
            procamp->contrast, procamp->saturation);
    }
    fprintf(tlog, "standard=%s\n", reverse_color_standard(standard));
skip:;
    VdpStatus ret = vdpGenerateCSCMatrix(procamp, standard, csc_matrix);
    traceCallHook(VDP_FUNC_ID_GENERATE_CSC_MATRIX, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                         VdpRect const *destination_rect,
                                         VdpOutputSurface source_surface,
                                         VdpRect const *source_rect, VdpColor const *colors,
                                         VdpOutputSurfaceRenderBlendState const *blend_state,
                                         uint32_t flags)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceRenderOutputSurface destination_surface=%d, "
        "destination_rect=%s,\n", trace_header, impl_state,
        destination_surface, rect2string(destination_rect));
    fprintf(tlog, "%s      source_surface=%d, source_rect=%s\n",
        trace_header_blank, source_surface, rect2string(source_rect));
    if (blend_state) {
        fprintf(tlog,
            "%s      blend_state.blend_factor_source_color=%s\n"
            "%s      blend_state.blend_factor_destination_color=%s\n"
            "%s      blend_state.blend_factor_source_alpha=%s\n"
            "%s      blend_state.blend_factor_destination_alpha=%s\n"
            "%s      blend_state.blend_equation_color=%s\n"
            "%s      blend_state.blend_equation_alpha=%s\n"
            "%s      blend_constant = (%11f, %11f, %11f, %11f)\n",
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_source_color),
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_destination_color),
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_source_alpha),
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_destination_alpha),
            trace_header_blank, reverse_blend_equation(blend_state->blend_equation_color),
            trace_header_blank, reverse_blend_equation(blend_state->blend_equation_alpha),
            trace_header_blank,
            blend_state->blend_constant.red, blend_state->blend_constant.green,
            blend_state->blend_constant.blue, blend_state->blend_constant.alpha);
    } else {
        fprintf(tlog, "%s      blend_state=NULL\n", trace_header_blank);
    }
    fprintf(tlog, "%s      flags = %s", trace_header_blank,
            reverse_output_surface_render_rotate(flags));
    if (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX)
        fprintf(tlog, "| VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX");
    fprintf(tlog, "\n");

    int color_count = 0;
    if (colors) {
        if (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX)
            color_count = 4;
        else
            color_count = 1;
    }
    fprintf(tlog, "%s      colors=[", trace_header_blank);
    for (int k = 0; k < color_count; k ++) {
        if (k > 0) fprintf(tlog, ", ");
        fprintf(tlog, "(%f,%f,%f,%f)", colors[k].red, colors[k].green, colors[k].blue,
                colors[k].alpha);
    }
    fprintf(tlog, "]\n");
skip:;
    VdpStatus ret = vdpOutputSurfaceRenderOutputSurface(destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                         VdpRect const *destination_rect,
                                         VdpBitmapSurface source_surface,
                                         VdpRect const *source_rect, VdpColor const *colors,
                                         VdpOutputSurfaceRenderBlendState const *blend_state,
                                         uint32_t flags)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, 0, NULL);
    fprintf(tlog, "%s%s VdpOutputSurfaceRenderBitmapSurface destination_surface=%d, "
        "destination_rect=%s,\n", trace_header, impl_state,
        destination_surface, rect2string(destination_rect));
    fprintf(tlog, "%s      source_surface=%d, source_rect=%s\n",
        trace_header_blank, source_surface, rect2string(source_rect));
    if (blend_state) {
        fprintf(tlog,
            "%s      blend_state.blend_factor_source_color=%s\n"
            "%s      blend_state.blend_factor_destination_color=%s\n"
            "%s      blend_state.blend_factor_source_alpha=%s\n"
            "%s      blend_state.blend_factor_destination_alpha=%s\n"
            "%s      blend_state.blend_equation_color=%s\n"
            "%s      blend_state.blend_equation_alpha=%s\n"
            "%s      blend_constant = (%11f, %11f, %11f, %11f)\n",
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_source_color),
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_destination_color),
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_source_alpha),
            trace_header_blank, reverse_blend_factor(blend_state->blend_factor_destination_alpha),
            trace_header_blank, reverse_blend_equation(blend_state->blend_equation_color),
            trace_header_blank, reverse_blend_equation(blend_state->blend_equation_alpha),
            trace_header_blank,
            blend_state->blend_constant.red, blend_state->blend_constant.green,
            blend_state->blend_constant.blue, blend_state->blend_constant.alpha);
    } else {
        fprintf(tlog, "%s      blend_state=NULL\n", trace_header_blank);
    }
    fprintf(tlog, "%s      flags = %s", trace_header_blank,
            reverse_output_surface_render_rotate(flags));
    if (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX)
        fprintf(tlog, "| VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX");
    fprintf(tlog, "\n");

    int color_count = 0;
    if (colors) {
        if (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX)
            color_count = 4;
        else
            color_count = 1;
    }
    fprintf(tlog, "%s      colors=[", trace_header_blank);
    for (int k = 0; k < color_count; k ++) {
        if (k > 0) fprintf(tlog, ", ");
        fprintf(tlog, "(%f,%f,%f,%f)", colors[k].red, colors[k].green, colors[k].blue,
                colors[k].alpha);
    }
    fprintf(tlog, "]\n");
skip:;
    VdpStatus ret = vdpOutputSurfaceRenderBitmapSurface(destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPreemptionCallbackRegister(VdpDevice device,
                                   VdpPreemptionCallback callback, void *context)
{
    const char *impl_state = "{zilch/fake success}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, 0, NULL);
    fprintf(tlog, "%s%s VdpPreemptionCallbackRegister device=%d, callback=%p, context=%p\n",
        trace_header, impl_state, device, callback, context);
skip:;
    VdpStatus ret = vdpPreemptionCallbackRegister(device, callback, context);
    traceCallHook(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                         VdpPresentationQueueTarget *target)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11, 0, NULL);
    fprintf(tlog, "%s%s VdpPresentationQueueTargetCreateX11, device=%d, drawable=%u\n",
        trace_header, impl_state, device, ((unsigned int)drawable));
skip:;
    VdpStatus ret = vdpPresentationQueueTargetCreateX11(device, drawable, target);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpGetProcAddress(VdpDevice device, VdpFuncId function_id,
                       void **function_pointer)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(VDP_FUNC_ID_GET_PROC_ADDRESS, 0, NULL);
    fprintf(tlog, "%s%s VdpGetProcAddress, device=%d, function_id=%s\n",
        trace_header, impl_state, device, reverse_func_id(function_id));
skip:;
    VdpStatus ret = vdpGetProcAddress(device, function_id, function_pointer);
    traceCallHook(VDP_FUNC_ID_GET_PROC_ADDRESS, 1, (void*)ret);
    return ret;
}

VdpStatus
traceVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                        VdpGetProcAddress **get_proc_address)
{
    const char *impl_state = "{full}";
    if (!trace_enabled)
        goto skip;
    traceCallHook(-1, 0, NULL);
    fprintf(tlog, "%s%s vdp_imp_device_create_x11 display=%p, screen=%d\n", trace_header,
        impl_state, display, screen);
skip:;
    VdpStatus ret = vdpDeviceCreateX11(display, screen, device, get_proc_address);
    traceCallHook(-1, 1, (void*)ret);
    return ret;
}
