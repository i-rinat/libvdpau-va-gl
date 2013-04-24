/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdarg.h>
#include "vdpau-trace.h"
#include "reverse-constant.h"

FILE *tlog = NULL;  ///< trace target
const char *trace_header =       "[VS] ";
const char *trace_header_blank = "     ";
int enabled = 1;
void (*trace_hook)(void *, int, int);
void *trace_hook_param = NULL;

void
traceEnableTracing(int flag)
{
    enabled = !!flag;
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
traceSetHook(void (*hook)(void *param, int origin, int after), void *param)
{
    trace_hook = hook;
    trace_hook_param = param;
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
    if (!enabled) return;
    va_list args;
    if (trace_hook) trace_hook(trace_hook_param, -2, 0);
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

void
traceVdpGetErrorString(const char *impl_state, VdpStatus status)
{
    (void)impl_state;
    (void)status;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_GET_ERROR_STRING, 0);
    //fprintf(tlog, "%s%s VdpGetErrorString status=%d\n", trace_header, impl_state, status);
}

void
traceVdpGetApiVersion(const char *impl_state, uint32_t *api_version)
{
    (void)api_version;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_GET_API_VERSION, 0);
    fprintf(tlog, "%s%s VdpGetApiVersion\n", trace_header, impl_state);
}

void
traceVdpDecoderQueryCapabilities(const char *impl_state, VdpDevice device,
                                 VdpDecoderProfile profile, VdpBool *is_supported,
                                 uint32_t *max_level, uint32_t *max_macroblocks,
                                 uint32_t *max_width, uint32_t *max_height)
{
    (void)is_supported;
    (void)max_level;
    (void)max_macroblocks;
    (void)max_width;
    (void)max_height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpDecoderQueryCapabilities device=%d, profile=%s\n",
        trace_header, impl_state, device, reverse_decoder_profile(profile));
}

void
traceVdpDecoderCreate(const char *impl_state, VdpDevice device, VdpDecoderProfile profile,
                      uint32_t width, uint32_t height, uint32_t max_references, VdpDecoder *decoder)
{
    (void)decoder;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_DECODER_CREATE, 0);
    fprintf(tlog, "%s%s VdpDecoderCreate device=%d, profile=%s, width=%d, height=%d, "
        "max_references=%d\n", trace_header, impl_state, device, reverse_decoder_profile(profile),
        width, height, max_references);
}

void
traceVdpDecoderDestroy(const char *impl_state, VdpDecoder decoder)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_DECODER_DESTROY, 0);
    fprintf(tlog, "%s%s VdpDecoderDestroy decoder=%d\n", trace_header, impl_state, decoder);
}

void
traceVdpDecoderGetParameters(const char *impl_state, VdpDecoder decoder,
                             VdpDecoderProfile *profile, uint32_t *width, uint32_t *height)
{
    (void)profile;
    (void)width;
    (void)height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_DECODER_GET_PARAMETERS, 0);
    fprintf(tlog, "%s%s VdpDecoderGetParameters decoder=%d\n", trace_header, impl_state, decoder);
}

void
traceVdpDecoderRender(const char *impl_state, VdpDecoder decoder, VdpVideoSurface target,
                      VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                      VdpBitstreamBuffer const *bitstream_buffers)
{
    (void)bitstream_buffers;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_DECODER_RENDER, 0);
    fprintf(tlog, "%s%s VdpDecoderRender decoder=%d, target=%d, picture_info=%p, "
        "bitstream_buffer_count=%d\n", trace_header, impl_state, decoder, target, picture_info,
        bitstream_buffer_count);
}

void
traceVdpOutputSurfaceQueryCapabilities(const char *impl_state, VdpDevice device,
                                       VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                                       uint32_t *max_width, uint32_t *max_height)
{
    (void)is_supported;
    (void)max_width;
    (void)max_height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryCapabilities device=%d, surface_rgba_format=%s\n",
        trace_header, impl_state, device, reverse_rgba_format(surface_rgba_format));
}

void
traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(const char *impl_state, VdpDevice device,
                                                       VdpRGBAFormat surface_rgba_format,
                                                       VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param,
                   VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryGetPutBitsNativeCapabilities device=%d, "
        "surface_rgba_format=%s\n", trace_header, impl_state, device,
        reverse_rgba_format(surface_rgba_format));
}

void
traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities(const char *impl_state, VdpDevice device,
                                                     VdpRGBAFormat surface_rgba_format,
                                                     VdpIndexedFormat bits_indexed_format,
                                                     VdpColorTableFormat color_table_format,
                                                     VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryPutBitsIndexedCapabilities device=%d, "
        "surface_rgba_format=%s, bits_indexed_format=%s, color_table_format=%s\n",
        trace_header, impl_state, device, reverse_rgba_format(surface_rgba_format),
        reverse_indexed_format(bits_indexed_format),
        reverse_color_table_format(color_table_format));
}

void
traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(const char *impl_state, VdpDevice device,
                                                   VdpRGBAFormat surface_rgba_format,
                                                   VdpYCbCrFormat bits_ycbcr_format,
                                                   VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceQueryPutBitsYCbCrCapabilities device=%d, "
        "surface_rgba_format=%s, bits_ycbcr_format=%s\n", trace_header, impl_state,
        device, reverse_rgba_format(surface_rgba_format), reverse_ycbcr_format(bits_ycbcr_format));
}

void
traceVdpOutputSurfaceCreate(const char *impl_state, VdpDevice device, VdpRGBAFormat rgba_format,
                            uint32_t width, uint32_t height, VdpOutputSurface *surface)
{
    (void)surface;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d\n",
        trace_header, impl_state, device, reverse_rgba_format(rgba_format), width, height);
}

void
traceVdpOutputSurfaceDestroy(const char *impl_state, VdpOutputSurface surface)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceDestroy surface=%d\n", trace_header, impl_state, surface);
}

void
traceVdpOutputSurfaceGetParameters(const char *impl_state, VdpOutputSurface surface,
                                   VdpRGBAFormat *rgba_format, uint32_t *width, uint32_t *height)
{
    (void)rgba_format;
    (void)width;
    (void)height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceGetParameters surface=%d\n", trace_header, impl_state,
        surface);
}

void
traceVdpOutputSurfaceGetBitsNative(const char *impl_state, VdpOutputSurface surface,
                                   VdpRect const *source_rect, void *const *destination_data,
                                   uint32_t const *destination_pitches)
{
    (void)destination_data;
    (void)destination_pitches;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceGetBitsNative surface=%d, source_rect=%s\n",
        trace_header, impl_state, surface, rect2string(source_rect));
}

void
traceVdpOutputSurfacePutBitsNative(const char *impl_state, VdpOutputSurface surface,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect)
{
    (void)source_data;
    (void)source_pitches;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE, 0);
    fprintf(tlog, "%s%s VdpOutputSurfacePutBitsNative surface=%d, destination_rect=%s\n",
        trace_header, impl_state, surface, rect2string(destination_rect));
}

void
traceVdpOutputSurfacePutBitsIndexed(const char *impl_state, VdpOutputSurface surface,
                                    VdpIndexedFormat source_indexed_format,
                                    void const *const *source_data, uint32_t const *source_pitch,
                                    VdpRect const *destination_rect,
                                    VdpColorTableFormat color_table_format, void const *color_table)
{
    (void)source_data;
    (void)source_pitch;
    (void)color_table;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED, 0);
    fprintf(tlog, "%s%s VdpOutputSurfacePutBitsIndexed surface=%d, source_indexed_format=%s, "
        "destination_rect=%s, color_table_format=%s\n", trace_header, impl_state, surface,
        reverse_indexed_format(source_indexed_format), rect2string(destination_rect),
        reverse_color_table_format(color_table_format));
}

void
traceVdpOutputSurfacePutBitsYCbCr(const char *impl_state, VdpOutputSurface surface,
                                  VdpYCbCrFormat source_ycbcr_format,
                                  void const *const *source_data, uint32_t const *source_pitches,
                                  VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    (void)source_data;
    (void)source_pitches;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, 0);
    fprintf(tlog, "%s%s VdpOutputSurfacePutBitsYCbCr surface=%d, source_ycbcr_format=%s, "
        "destination_rect=%s, csc_matrix=%p\n", trace_header, impl_state, surface,
        reverse_ycbcr_format(source_ycbcr_format), rect2string(destination_rect), csc_matrix);
}

void
traceVdpVideoMixerQueryFeatureSupport(const char *impl_state, VdpDevice device,
                                      VdpVideoMixerFeature feature, VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, 0);
    fprintf(tlog, "%s%s VdpVideoMixerQueryFeatureSupport device=%d, feature=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_feature(feature));
}

void
traceVdpVideoMixerQueryParameterSupport(const char *impl_state, VdpDevice device,
                                        VdpVideoMixerParameter parameter,
                                        VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, 0);
    fprintf(tlog, "%s%s VdpVideoMixerQueryParameterSupport device=%d, parameter=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_parameter(parameter));
}

void
traceVdpVideoMixerQueryAttributeSupport(const char *impl_state, VdpDevice device,
                                        VdpVideoMixerAttribute attribute, VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, 0);
    fprintf(tlog, "%s%s VdpVideoMixerQueryAttributeSupport device=%d, attribute=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_attribute(attribute));
}

void
traceVdpVideoMixerQueryParameterValueRange(const char *impl_state, VdpDevice device,
                                           VdpVideoMixerParameter parameter,
                                           void *min_value, void *max_value)
{
    (void)min_value;
    (void)max_value;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, 0);
    fprintf(tlog, "%s%s VdpVideoMixerQueryParameterValueRange device=%d, parameter=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_parameter(parameter));
}

void
traceVdpVideoMixerQueryAttributeValueRange(const char *impl_state, VdpDevice device,
                                           VdpVideoMixerAttribute attribute,
                                           void *min_value, void *max_value)
{
    (void)min_value;
    (void)max_value;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, 0);
    fprintf(tlog, "%s%s VdpVideoMixerQueryAttributeValueRange device=%d, attribute=%s\n",
        trace_header, impl_state, device, reverse_video_mixer_attribute(attribute));
}

void
traceVdpVideoMixerCreate(const char *impl_state, VdpDevice device, uint32_t feature_count,
                         VdpVideoMixerFeature const *features, uint32_t parameter_count,
                         VdpVideoMixerParameter const *parameters,
                         void const *const *parameter_values, VdpVideoMixer *mixer)
{
    (void)mixer;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_CREATE, 0);
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
}

void
traceVdpVideoMixerSetFeatureEnables(const char *impl_state, VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool const *feature_enables)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, 0);
    fprintf(tlog, "%s%s VdpVideoMixerSetFeatureEnables mixer=%d, feature_count=%d\n",
        trace_header, impl_state, mixer, feature_count);
    for (uint32_t k = 0; k < feature_count; k ++) {
        fprintf(tlog, "%s      feature %d (%s) %s\n", trace_header_blank,
            features[k], reverse_video_mixer_feature(features[k]),
            feature_enables[k] ? "enabled" : "disabled");
    }
}

void
traceVdpVideoMixerSetAttributeValues(const char *impl_state, VdpVideoMixer mixer,
                                     uint32_t attribute_count,
                                     VdpVideoMixerAttribute const *attributes,
                                     void const *const *attribute_values)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, 0);
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
}

void
traceVdpVideoMixerGetFeatureSupport(const char *impl_state, VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool *feature_supports)
{
    (void)feature_supports;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT, 0);
    fprintf(tlog, "%s%s VdpVideoMixerGetFeatureSupport mixer=%d, feature_count=%d\n",
        trace_header, impl_state, mixer, feature_count);
    for (unsigned int k = 0; k < feature_count; k ++)
        fprintf(tlog, "%s      feature %s\n", trace_header_blank,
            reverse_video_mixer_feature(features[k]));
}

void
traceVdpVideoMixerGetFeatureEnables(const char *impl_state, VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool *feature_enables)
{
    (void)feature_enables;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES, 0);
    fprintf(tlog, "%s%s VdpVideoMixerGetFeatureEnables mixer=%d, feature_count=%d\n",
        trace_header, impl_state, mixer, feature_count);
    for (unsigned int k = 0; k < feature_count; k ++)
        fprintf(tlog, "%s      feature %s\n", trace_header_blank,
            reverse_video_mixer_feature(features[k]));
}

void
traceVdpVideoMixerGetParameterValues(const char *impl_state, VdpVideoMixer mixer,
                                     uint32_t parameter_count,
                                     VdpVideoMixerParameter const *parameters,
                                     void *const *parameter_values)
{
    (void)parameter_values;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES, 0);
    fprintf(tlog, "%s%s VdpVideoMixerGetParameterValues mixer=%d, parameter_count=%d\n",
        trace_header, impl_state, mixer, parameter_count);
    for (unsigned int k = 0; k < parameter_count; k ++)
        fprintf(tlog, "%s      parameter %s\n", trace_header_blank,
            reverse_video_mixer_parameter(parameters[k]));
}

void
traceVdpVideoMixerGetAttributeValues(const char *impl_state, VdpVideoMixer mixer,
                                     uint32_t attribute_count,
                                     VdpVideoMixerAttribute const *attributes,
                                     void *const *attribute_values)
{
    (void)attribute_values;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES, 0);
    fprintf(tlog, "%s%s VdpVideoMixerGetAttributeValues mixer=%d, attribute_count=%d\n",
        trace_header, impl_state, mixer, attribute_count);
    for (unsigned int k = 0; k < attribute_count; k ++)
        fprintf(tlog, "%s      attribute %s\n", trace_header_blank,
            reverse_video_mixer_attribute(attributes[k]));
}

void
traceVdpVideoMixerDestroy(const char *impl_state, VdpVideoMixer mixer)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_DESTROY, 0);
    fprintf(tlog, "%s%s VdpVideoMixerDestroy mixer=%d\n", trace_header, impl_state, mixer);
}

void
traceVdpVideoMixerRender(const char *impl_state, VdpVideoMixer mixer,
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
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_MIXER_RENDER, 0);
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
}

void
traceVdpPresentationQueueTargetDestroy(const char *impl_state,
                                       VdpPresentationQueueTarget presentation_queue_target)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueTargetDestroy presentation_queue_target=%d\n",
        trace_header, impl_state, presentation_queue_target);
}

void
traceVdpPresentationQueueCreate(const char *impl_state, VdpDevice device,
                                VdpPresentationQueueTarget presentation_queue_target,
                                VdpPresentationQueue *presentation_queue)
{
    (void)presentation_queue;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueCreate device=%d, presentation_queue_target=%d\n",
        trace_header, impl_state, device, presentation_queue_target);
}

void
traceVdpPresentationQueueDestroy(const char *impl_state, VdpPresentationQueue presentation_queue)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueDestroy presentation_queue=%d\n",
        trace_header, impl_state, presentation_queue);
}

void
traceVdpPresentationQueueSetBackgroundColor(const char *impl_state,
                                            VdpPresentationQueue presentation_queue,
                                            VdpColor *const background_color)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueSetBackgroundColor presentation_queue=%d, "
        "background_color=(%.2f,%.2f,%.2f,%.2f)\n",
        trace_header, impl_state, presentation_queue, background_color->red,
        background_color->green, background_color->blue, background_color->alpha);
}

void
traceVdpPresentationQueueGetBackgroundColor(const char *impl_state,
                                            VdpPresentationQueue presentation_queue,
                                            VdpColor *background_color)
{
    (void)background_color;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueGetBackgroundColor  presentation_queue=%d\n",
        trace_header, impl_state, presentation_queue);
}

void
traceVdpPresentationQueueGetTime(const char *impl_state, VdpPresentationQueue presentation_queue,
                                 VdpTime *current_time)
{
    (void)current_time;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueGetTime presentation_queue=%d\n",
        trace_header, impl_state, presentation_queue);
}

void
traceVdpPresentationQueueDisplay(const char *impl_state, VdpPresentationQueue presentation_queue,
                                 VdpOutputSurface surface, uint32_t clip_width,
                                 uint32_t clip_height, VdpTime earliest_presentation_time)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueDisplay presentation_queue=%d, surface=%d, "
        "clip_width=%d, clip_height=%d,\n", trace_header, impl_state, presentation_queue, surface,
        clip_width, clip_height);
    fprintf(tlog, "%s      earliest_presentation_time=%"PRIu64"\n", trace_header_blank,
        earliest_presentation_time);
}

void
traceVdpPresentationQueueBlockUntilSurfaceIdle(const char *impl_state,
                                               VdpPresentationQueue presentation_queue,
                                               VdpOutputSurface surface,
                                               VdpTime *first_presentation_time)

{
    (void)first_presentation_time;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueBlockUntilSurfaceIdle presentation_queue=%d, "
        "surface=%d\n", trace_header, impl_state, presentation_queue, surface);
}

void
traceVdpPresentationQueueQuerySurfaceStatus(const char *impl_state,
                                            VdpPresentationQueue presentation_queue,
                                            VdpOutputSurface surface,
                                            VdpPresentationQueueStatus *status,
                                            VdpTime *first_presentation_time)
{
    (void)status;
    (void)first_presentation_time;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param,
                               VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueQuerySurfaceStatus presentation_queue=%d, "
        "surface=%d\n", trace_header, impl_state, presentation_queue, surface);
}

void
traceVdpVideoSurfaceQueryCapabilities(const char *impl_state, VdpDevice device,
                                      VdpChromaType surface_chroma_type, VdpBool *is_supported,
                                      uint32_t *max_width, uint32_t *max_height)
{
    (void)is_supported;
    (void)max_width;
    (void)max_height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpVideoSurfaceQueryCapabilities device=%d, surface_chroma_type=%s\n",
        trace_header, impl_state, device, reverse_chroma_type(surface_chroma_type));
}

void
traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(const char *impl_state, VdpDevice device,
                                                     VdpChromaType surface_chroma_type,
                                                     VdpYCbCrFormat bits_ycbcr_format,
                                                     VdpBool *is_supported)
{
    (void)is_supported;
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param,
                   VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities device=%d, "
        "surface_chroma_type=%s, bits_ycbcr_format=%s\n", trace_header, impl_state,
        device, reverse_chroma_type(surface_chroma_type), reverse_ycbcr_format(bits_ycbcr_format));
}

void
traceVdpVideoSurfaceCreate(const char *impl_state, VdpDevice device, VdpChromaType chroma_type,
                           uint32_t width, uint32_t height, VdpVideoSurface *surface)
{
    (void)surface;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_SURFACE_CREATE, 0);
    fprintf(tlog, "%s%s VdpVideoSurfaceCreate, device=%d, chroma_type=%s, width=%d, height=%d\n",
        trace_header, impl_state, device, reverse_chroma_type(chroma_type), width, height);
}

void
traceVdpVideoSurfaceDestroy(const char *impl_state, VdpVideoSurface surface)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, 0);
    fprintf(tlog, "%s%s VdpVideoSurfaceDestroy surface=%d\n", trace_header, impl_state, surface);
}

void
traceVdpVideoSurfaceGetParameters(const char *impl_state, VdpVideoSurface surface,
                                  VdpChromaType *chroma_type, uint32_t *width, uint32_t *height)
{
    (void)chroma_type;
    (void)width;
    (void)height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, 0);
    fprintf(tlog, "%s%s VdpVideoSurfaceGetParameters surface=%d\n", trace_header, impl_state,
        surface);
}

void
traceVdpVideoSurfaceGetBitsYCbCr(const char *impl_state, VdpVideoSurface surface,
                                 VdpYCbCrFormat destination_ycbcr_format,
                                 void *const *destination_data, uint32_t const *destination_pitches)
{
    (void)destination_data;
    (void)destination_pitches;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, 0);
    fprintf(tlog, "%s%s VdpVideoSurfaceGetBitsYCbCr surface=%d, destination_ycbcr_format=%s\n",
        trace_header, impl_state, surface, reverse_ycbcr_format(destination_ycbcr_format));
}

void
traceVdpVideoSurfacePutBitsYCbCr(const char *impl_state, VdpVideoSurface surface,
                                 VdpYCbCrFormat source_ycbcr_format, void const *const *source_data,
                                 uint32_t const *source_pitches)
{
    (void)source_data;
    (void)source_pitches;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR, 0);
    fprintf(tlog, "%s%s VdpVideoSurfacePutBitsYCbCr surface=%d, source_ycbcr_format=%s\n",
        trace_header, impl_state, surface, reverse_ycbcr_format(source_ycbcr_format));
}

void
traceVdpBitmapSurfaceQueryCapabilities(const char *impl_state, VdpDevice device,
                                       VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                                       uint32_t *max_width, uint32_t *max_height)
{
    (void)is_supported;
    (void)max_width;
    (void)max_height;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, 0);
    fprintf(tlog, "%s%s VdpBitmapSurfaceQueryCapabilities device=%d, surface_rgba_format=%s\n",
        trace_header, impl_state, device, reverse_rgba_format(surface_rgba_format));
}

void
traceVdpBitmapSurfaceCreate(const char *impl_state, VdpDevice device, VdpRGBAFormat rgba_format,
                            uint32_t width, uint32_t height, VdpBool frequently_accessed,
                            VdpBitmapSurface *surface)
{
    (void)surface;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_BITMAP_SURFACE_CREATE, 0);
    fprintf(tlog, "%s%s VdpBitmapSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d,\n"
        "%s      frequently_accessed=%d\n", trace_header, impl_state, device,
        reverse_rgba_format(rgba_format), width, height, trace_header_blank, frequently_accessed);
}

void
traceVdpBitmapSurfaceDestroy(const char *impl_state, VdpBitmapSurface surface)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, 0);
    fprintf(tlog, "%s%s VdpBitmapSurfaceDestroy surface=%d\n", trace_header, impl_state, surface);
}

void
traceVdpBitmapSurfaceGetParameters(const char *impl_state, VdpBitmapSurface surface,
                                   VdpRGBAFormat *rgba_format, uint32_t *width, uint32_t *height,
                                   VdpBool *frequently_accessed)
{
    (void)rgba_format;
    (void)width;
    (void)height;
    (void)frequently_accessed;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS, 0);
    fprintf(tlog, "%s%s VdpBitmapSurfaceGetParameters surface=%d\n",
        trace_header, impl_state, surface);
}

void
traceVdpBitmapSurfacePutBitsNative(const char *impl_state, VdpBitmapSurface surface,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect)
{
    (void)source_data;
    (void)source_pitches;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, 0);
    fprintf(tlog, "%s%s VdpBitmapSurfacePutBitsNative surface=%d, destination_rect=%s\n",
        trace_header, impl_state, surface, rect2string(destination_rect));
}

void
traceVdpDeviceDestroy(const char *impl_state, VdpDevice device)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_DEVICE_DESTROY, 0);
    fprintf(tlog, "%s%s VdpDeviceDestroy device=%d\n", trace_header, impl_state, device);
}

void
traceVdpGetInformationString(const char *impl_state, char const **information_string)
{
    (void)information_string;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_GET_INFORMATION_STRING, 0);
    fprintf(tlog, "%s%s VdpGetInformationString\n", trace_header, impl_state);
}

void
traceVdpGenerateCSCMatrix(const char *impl_state, VdpProcamp *procamp, VdpColorStandard standard,
                          VdpCSCMatrix *csc_matrix)
{
    (void)csc_matrix;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_GENERATE_CSC_MATRIX, 0);
    fprintf(tlog, "%s%s VdpGenerateCSCMatrix ", trace_header, impl_state);
    if (procamp) {
        fprintf(tlog, "brightness=%f, contrast=%f, saturation=%f, ", procamp->brightness,
            procamp->contrast, procamp->saturation);
    }
    fprintf(tlog, "standard=%s\n", reverse_color_standard(standard));
}

void
traceVdpOutputSurfaceRenderOutputSurface(const char *impl_state,
                                         VdpOutputSurface destination_surface,
                                         VdpRect const *destination_rect,
                                         VdpOutputSurface source_surface,
                                         VdpRect const *source_rect, VdpColor const *colors,
                                         VdpOutputSurfaceRenderBlendState const *blend_state,
                                         uint32_t flags)
{
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceRenderOutputSurface destination_surface=%d, "
        "destination_rect=%s,\n", trace_header, impl_state,
        destination_surface, rect2string(destination_rect));
    fprintf(tlog, "%s      source_surface=%d, source_rect=%s\n",
        trace_header_blank, source_surface, rect2string(source_rect));
    fprintf(tlog,   "%s      blend_state.blend_factor_source_color=%s\n"
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
}

void
traceVdpOutputSurfaceRenderBitmapSurface(const char *impl_state,
                                         VdpOutputSurface destination_surface,
                                         VdpRect const *destination_rect,
                                         VdpBitmapSurface source_surface,
                                         VdpRect const *source_rect, VdpColor const *colors,
                                         VdpOutputSurfaceRenderBlendState const *blend_state,
                                         uint32_t flags)
{
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param, VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, 0);
    fprintf(tlog, "%s%s VdpOutputSurfaceRenderBitmapSurface destination_surface=%d, "
        "destination_rect=%s,\n", trace_header, impl_state,
        destination_surface, rect2string(destination_rect));
    fprintf(tlog, "%s      source_surface=%d, source_rect=%s\n",
        trace_header_blank, source_surface, rect2string(source_rect));
    fprintf(tlog,   "%s      blend_state.blend_factor_source_color=%s\n"
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
}

void
traceVdpPreemptionCallbackRegister(const char *impl_state, VdpDevice device,
                                   VdpPreemptionCallback callback, void *context)
{
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, 0);
    fprintf(tlog, "%s%s VdpPreemptionCallbackRegister device=%d, callback=%p, context=%p\n",
        trace_header, impl_state, device, callback, context);
}

void
traceVdpPresentationQueueTargetCreateX11(const char *impl_state, VdpDevice device,
                                         Drawable drawable, VdpPresentationQueueTarget *target)
{
    (void)target;
    if (!enabled) return;
    if (trace_hook)
        trace_hook(trace_hook_param, VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11, 0);
    fprintf(tlog, "%s%s VdpPresentationQueueTargetCreateX11, device=%d, drawable=%u\n",
        trace_header, impl_state, device, ((unsigned int)drawable));
}

void
traceVdpGetProcAddress(const char *impl_state, VdpDevice device, VdpFuncId function_id,
                       void **function_pointer)
{
    (void)function_pointer;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, VDP_FUNC_ID_GET_PROC_ADDRESS, 0);
    fprintf(tlog, "%s%s VdpGetProcAddress, device=%d, function_id=%s\n",
        trace_header, impl_state, device, reverse_func_id(function_id));
}

void
traceVdpDeviceCreateX11(const char *trace_state, Display *display, int screen, VdpDevice *device,
                        VdpGetProcAddress **get_proc_address)
{
    (void)device;
    (void)get_proc_address;
    if (!enabled) return;
    if (trace_hook) trace_hook(trace_hook_param, -1, 0);
    fprintf(tlog, "%s%s vdp_imp_device_create_x11 display=%p, screen=%d\n", trace_header,
        trace_state, display, screen);
}
