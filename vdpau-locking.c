/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#include "vdpau-locking.h"
#include "vdpau-soft.h"
#include "vdpau-trace.h"
#include <assert.h>


static
void
acquire_lock()
{
    pthread_mutex_lock(&global.mutex);
}

static
void
release_lock()
{
    pthread_mutex_unlock(&global.mutex);
}

VdpStatus
lockedVdpGetApiVersion(uint32_t *api_version)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_GET_API_VERSION, 0, NULL);
    traceVdpGetApiVersion("{full}", api_version);
    VdpStatus ret = softVdpGetApiVersion(api_version);
    traceCallHook(VDP_FUNC_ID_GET_API_VERSION, 1, (void *)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile,
                                  VdpBool *is_supported, uint32_t *max_level,
                                  uint32_t *max_macroblocks, uint32_t *max_width,
                                  uint32_t *max_height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, 0, NULL);
    traceVdpDecoderQueryCapabilities("{zilch}", device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    VdpStatus ret = softVdpDecoderQueryCapabilities(device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    traceCallHook(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                       uint32_t max_references, VdpDecoder *decoder)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_DECODER_CREATE, 0, NULL);
    traceVdpDecoderCreate("{full}", device, profile, width, height, max_references, decoder);
    VdpStatus ret = softVdpDecoderCreate(device, profile, width, height, max_references, decoder);
    traceCallHook(VDP_FUNC_ID_DECODER_CREATE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderDestroy(VdpDecoder decoder)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_DECODER_DESTROY, 0, NULL);
    traceVdpDecoderDestroy("{full}", decoder);
    VdpStatus ret = softVdpDecoderDestroy(decoder);
    traceCallHook(VDP_FUNC_ID_DECODER_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                              uint32_t *width, uint32_t *height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_DECODER_GET_PARAMETERS, 0, NULL);
    traceVdpDecoderGetParameters("{full}", decoder, profile, width, height);
    VdpStatus ret = softVdpDecoderGetParameters(decoder, profile, width, height);
    traceCallHook(VDP_FUNC_ID_DECODER_GET_PARAMETERS, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                       VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                       VdpBitstreamBuffer const *bitstream_buffers)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_DECODER_RENDER, 0, NULL);
    traceVdpDecoderRender("{part}", decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    VdpStatus ret = softVdpDecoderRender(decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    traceCallHook(VDP_FUNC_ID_DECODER_RENDER, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                        VdpBool *is_supported, uint32_t *max_width,
                                        uint32_t *max_height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES, 0, NULL);
    traceVdpOutputSurfaceQueryCapabilities("{full}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    VdpStatus ret = softVdpOutputSurfaceQueryCapabilities(device, surface_rgba_format, is_supported,
                                                          max_width, max_height);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                        VdpRGBAFormat surface_rgba_format,
                                                        VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, 0, NULL);
    traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities("{zilch}", device, surface_rgba_format,
        is_supported);
    VdpStatus ret =
        softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(device, surface_rgba_format,
                                                              is_supported);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpIndexedFormat bits_indexed_format,
                                                      VdpColorTableFormat color_table_format,
                                                      VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES, 0, NULL);
    traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities("{zilch}", device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
    VdpStatus ret = softVdpOutputSurfaceQueryPutBitsIndexedCapabilities(device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpRGBAFormat surface_rgba_format,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES, 0, NULL);
    traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities("{zilch}", device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    VdpStatus ret = softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                             uint32_t height, VdpOutputSurface *surface)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, 0, NULL);
    traceVdpOutputSurfaceCreate("{part}", device, rgba_format, width, height, surface);
    VdpStatus ret = softVdpOutputSurfaceCreate(device, rgba_format, width, height, surface);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, 0, NULL);
    traceVdpOutputSurfaceDestroy("{full}", surface);
    VdpStatus ret = softVdpOutputSurfaceDestroy(surface);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                    uint32_t *width, uint32_t *height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS, 0, NULL);
    traceVdpOutputSurfaceGetParameters("{full}", surface, rgba_format, width, height);
    VdpStatus ret = softVdpOutputSurfaceGetParameters(surface, rgba_format, width, height);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                    void *const *destination_data,
                                    uint32_t const *destination_pitches)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, 0, NULL);
    traceVdpOutputSurfaceGetBitsNative("{part}", surface, source_rect, destination_data,
        destination_pitches);
    VdpStatus ret = softVdpOutputSurfaceGetBitsNative(surface, source_rect, destination_data,
        destination_pitches);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                    uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE, 0, NULL);
    traceVdpOutputSurfacePutBitsNative("{full}", surface, source_data, source_pitches,
        destination_rect);
    VdpStatus ret = softVdpOutputSurfacePutBitsNative(surface, source_data, source_pitches,
        destination_rect);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface,
                                     VdpIndexedFormat source_indexed_format,
                                     void const *const *source_data, uint32_t const *source_pitch,
                                     VdpRect const *destination_rect,
                                     VdpColorTableFormat color_table_format,
                                     void const *color_table)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED, 0, NULL);
    traceVdpOutputSurfacePutBitsIndexed("{part}", surface, source_indexed_format, source_data,
        source_pitch, destination_rect, color_table_format, color_table);
    VdpStatus ret = softVdpOutputSurfacePutBitsIndexed(surface, source_indexed_format, source_data,
        source_pitch, destination_rect, color_table_format, color_table);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, 0, NULL);
    traceVdpOutputSurfacePutBitsYCbCr("{zilch}", surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    VdpStatus ret = softVdpOutputSurfacePutBitsYCbCr(surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                       VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, 0, NULL);
    traceVdpVideoMixerQueryFeatureSupport("{zilch}", device, feature, is_supported);
    VdpStatus ret = softVdpVideoMixerQueryFeatureSupport(device, feature, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                         VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, 0, NULL);
    traceVdpVideoMixerQueryParameterSupport("{zilch}", device, parameter, is_supported);
    VdpStatus ret = softVdpVideoMixerQueryParameterSupport(device, parameter, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                         VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, 0, NULL);
    traceVdpVideoMixerQueryAttributeSupport("{zilch}", device, attribute, is_supported);
    VdpStatus ret = softVdpVideoMixerQueryAttributeSupport(device, attribute, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                            void *min_value, void *max_value)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, 0, NULL);
    traceVdpVideoMixerQueryParameterValueRange("{zilch}", device, parameter, min_value, max_value);
    VdpStatus ret = softVdpVideoMixerQueryParameterValueRange(device, parameter, min_value,
                                                              max_value);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                            void *min_value, void *max_value)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, 0, NULL);
    traceVdpVideoMixerQueryAttributeValueRange("{zilch}", device, attribute, min_value, max_value);
    VdpStatus ret = softVdpVideoMixerQueryAttributeValueRange(device, attribute, min_value,
                                                              max_value);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                          VdpVideoMixerFeature const *features, uint32_t parameter_count,
                          VdpVideoMixerParameter const *parameters,
                          void const *const *parameter_values, VdpVideoMixer *mixer)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_CREATE, 0, NULL);
    traceVdpVideoMixerCreate("{part}", device, feature_count, features, parameter_count, parameters,
        parameter_values, mixer);
    VdpStatus ret = softVdpVideoMixerCreate(device, feature_count, features, parameter_count,
                                            parameters, parameter_values, mixer);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_CREATE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                     VdpVideoMixerFeature const *features,
                                     VdpBool const *feature_enables)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, 0, NULL);
    traceVdpVideoMixerSetFeatureEnables("{part}", mixer, feature_count, features, feature_enables);
    VdpStatus ret = softVdpVideoMixerSetFeatureEnables(mixer, feature_count, features,
                                                       feature_enables);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                      VdpVideoMixerAttribute const *attributes,
                                      void const *const *attribute_values)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, 0, NULL);
    traceVdpVideoMixerSetAttributeValues("{part}", mixer, attribute_count, attributes,
        attribute_values);
    VdpStatus ret = softVdpVideoMixerSetAttributeValues(mixer, attribute_count, attributes,
        attribute_values);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                     VdpVideoMixerFeature const *features,
                                     VdpBool *feature_supports)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT, 0, NULL);
    traceVdpVideoMixerGetFeatureSupport("{zilch}", mixer, feature_count, features,
        feature_supports);
    VdpStatus ret = softVdpVideoMixerGetFeatureSupport(mixer, feature_count, features,
        feature_supports);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                     VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES, 0, NULL);
    traceVdpVideoMixerGetFeatureEnables("{zilch}", mixer, feature_count, features, feature_enables);
    VdpStatus ret = softVdpVideoMixerGetFeatureEnables(mixer, feature_count, features,
                                                       feature_enables);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                      VdpVideoMixerParameter const *parameters,
                                      void *const *parameter_values)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES, 0, NULL);
    traceVdpVideoMixerGetParameterValues("{zilch}", mixer, parameter_count, parameters,
        parameter_values);
    VdpStatus ret = softVdpVideoMixerGetParameterValues(mixer, parameter_count, parameters,
        parameter_values);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                      VdpVideoMixerAttribute const *attributes,
                                      void *const *attribute_values)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES, 0, NULL);
    traceVdpVideoMixerGetAttributeValues("{zilch}", mixer, attribute_count, attributes,
        attribute_values);
    VdpStatus ret = softVdpVideoMixerGetAttributeValues(mixer, attribute_count, attributes,
        attribute_values);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_DESTROY, 0, NULL);
    traceVdpVideoMixerDestroy("{full}", mixer);
    VdpStatus ret = softVdpVideoMixerDestroy(mixer);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerRender(VdpVideoMixer mixer, VdpOutputSurface background_surface,
                          VdpRect const *background_source_rect,
                          VdpVideoMixerPictureStructure current_picture_structure,
                          uint32_t video_surface_past_count,
                          VdpVideoSurface const *video_surface_past,
                          VdpVideoSurface video_surface_current,
                          uint32_t video_surface_future_count,
                          VdpVideoSurface const *video_surface_future,
                          VdpRect const *video_source_rect, VdpOutputSurface destination_surface,
                          VdpRect const *destination_rect, VdpRect const *destination_video_rect,
                          uint32_t layer_count, VdpLayer const *layers)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_RENDER, 0, NULL);
    traceVdpVideoMixerRender("{part}", mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);
    VdpStatus ret = softVdpVideoMixerRender(mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);
    traceCallHook(VDP_FUNC_ID_VIDEO_MIXER_RENDER, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, 0, NULL);
    traceVdpPresentationQueueTargetDestroy("{full}", presentation_queue_target);
    VdpStatus ret = softVdpPresentationQueueTargetDestroy(presentation_queue_target);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueCreate(VdpDevice device,
                                 VdpPresentationQueueTarget presentation_queue_target,
                                 VdpPresentationQueue *presentation_queue)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, 0, NULL);
    traceVdpPresentationQueueCreate("{part}", device, presentation_queue_target,
        presentation_queue);
    VdpStatus ret = softVdpPresentationQueueCreate(device, presentation_queue_target,
        presentation_queue);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, 0, NULL);
    traceVdpPresentationQueueDestroy("{full}", presentation_queue);
    VdpStatus ret = softVdpPresentationQueueDestroy(presentation_queue);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                             VdpColor *const background_color)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, 0, NULL);
    traceVdpPresentationQueueSetBackgroundColor("{full}", presentation_queue, background_color);
    VdpStatus ret = softVdpPresentationQueueSetBackgroundColor(presentation_queue,
                                                               background_color);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                             VdpColor *background_color)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, 0, NULL);
    traceVdpPresentationQueueGetBackgroundColor("{full}", presentation_queue, background_color);
    VdpStatus ret = softVdpPresentationQueueGetBackgroundColor(presentation_queue,
                                                               background_color);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue, VdpTime *current_time)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME, 0, NULL);
    traceVdpPresentationQueueGetTime("{full}", presentation_queue, current_time);
    VdpStatus ret = softVdpPresentationQueueGetTime(presentation_queue, current_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                  uint32_t clip_width, uint32_t clip_height,
                                  VdpTime earliest_presentation_time)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, 0, NULL);
    traceVdpPresentationQueueDisplay("{full}", presentation_queue, surface, clip_width, clip_height,
        earliest_presentation_time);
    VdpStatus ret = softVdpPresentationQueueDisplay(presentation_queue, surface, clip_width,
                                                    clip_height, earliest_presentation_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                                VdpOutputSurface surface,
                                                VdpTime *first_presentation_time)

{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, 0, NULL);
    traceVdpPresentationQueueBlockUntilSurfaceIdle("{full}", presentation_queue, surface,
        first_presentation_time);
    VdpStatus ret = softVdpPresentationQueueBlockUntilSurfaceIdle(presentation_queue, surface,
        first_presentation_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                             VdpOutputSurface surface,
                                             VdpPresentationQueueStatus *status,
                                             VdpTime *first_presentation_time)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, 0, NULL);
    traceVdpPresentationQueueQuerySurfaceStatus("{part}", presentation_queue, surface,
        status, first_presentation_time);
    VdpStatus ret = softVdpPresentationQueueQuerySurfaceStatus(presentation_queue, surface,
        status, first_presentation_time);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                       VdpBool *is_supported, uint32_t *max_width,
                                       uint32_t *max_height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, 0, NULL);
    traceVdpVideoSurfaceQueryCapabilities("{part}", device, surface_chroma_type, is_supported,
        max_width, max_height);
    VdpStatus ret = softVdpVideoSurfaceQueryCapabilities(device, surface_chroma_type, is_supported,
        max_width, max_height);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                      VdpChromaType surface_chroma_type,
                                                      VdpYCbCrFormat bits_ycbcr_format,
                                                      VdpBool *is_supported)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, 0, NULL);
    traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities("{part}", device, surface_chroma_type,
        bits_ycbcr_format, is_supported);
    VdpStatus ret = softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(device, surface_chroma_type,
        bits_ycbcr_format, is_supported);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                            uint32_t height, VdpVideoSurface *surface)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, 0, NULL);
    traceVdpVideoSurfaceCreate("{part}", device, chroma_type, width, height, surface);
    VdpStatus ret = softVdpVideoSurfaceCreate(device, chroma_type, width, height, surface);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, 0, NULL);
    traceVdpVideoSurfaceDestroy("{full}", surface);
    VdpStatus ret = softVdpVideoSurfaceDestroy(surface);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                   uint32_t *width, uint32_t *height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, 0, NULL);
    traceVdpVideoSurfaceGetParameters("{full}", surface, chroma_type, width, height);
    VdpStatus ret = softVdpVideoSurfaceGetParameters(surface, chroma_type, width, height);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, 0, NULL);
    traceVdpVideoSurfaceGetBitsYCbCr("{part}", surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    VdpStatus ret = softVdpVideoSurfaceGetBitsYCbCr(surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                  void const *const *source_data, uint32_t const *source_pitches)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR, 0, NULL);
    traceVdpVideoSurfacePutBitsYCbCr("{part}", surface, source_ycbcr_format, source_data,
        source_pitches);
    VdpStatus ret = softVdpVideoSurfacePutBitsYCbCr(surface, source_ycbcr_format, source_data,
        source_pitches);
    traceCallHook(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                        VdpBool *is_supported, uint32_t *max_width,
                                        uint32_t *max_height)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, 0, NULL);
    traceVdpBitmapSurfaceQueryCapabilities("{full}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    VdpStatus ret = softVdpBitmapSurfaceQueryCapabilities(device, surface_rgba_format, is_supported,
        max_width, max_height);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                             uint32_t height, VdpBool frequently_accessed,
                             VdpBitmapSurface *surface)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_CREATE, 0, NULL);
    traceVdpBitmapSurfaceCreate("{full}", device, rgba_format, width, height, frequently_accessed,
        surface);
    VdpStatus ret = softVdpBitmapSurfaceCreate(device, rgba_format, width, height,
                                               frequently_accessed, surface);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_CREATE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, 0, NULL);
    traceVdpBitmapSurfaceDestroy("{full}", surface);
    VdpStatus ret = softVdpBitmapSurfaceDestroy(surface);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                    uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS, 0, NULL);
    traceVdpBitmapSurfaceGetParameters("{full}", surface, rgba_format, width, height,
        frequently_accessed);
    VdpStatus ret = softVdpBitmapSurfaceGetParameters(surface, rgba_format, width, height,
        frequently_accessed);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                    uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, 0, NULL);
    traceVdpBitmapSurfacePutBitsNative("{full}", surface, source_data, source_pitches,
        destination_rect);
    VdpStatus ret = softVdpBitmapSurfacePutBitsNative(surface, source_data, source_pitches,
        destination_rect);
    traceCallHook(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDeviceDestroy(VdpDevice device)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_DEVICE_DESTROY, 0, NULL);
    traceVdpDeviceDestroy("{full}", device);
    VdpStatus ret = softVdpDeviceDestroy(device);
    traceCallHook(VDP_FUNC_ID_DEVICE_DESTROY, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpGetInformationString(char const **information_string)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_GET_INFORMATION_STRING, 0, NULL);
    traceVdpGetInformationString("{full}", information_string);
    VdpStatus ret = softVdpGetInformationString(information_string);
    traceCallHook(VDP_FUNC_ID_GET_INFORMATION_STRING, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_GENERATE_CSC_MATRIX, 0, NULL);
    traceVdpGenerateCSCMatrix("{part}", procamp, standard, csc_matrix);
    VdpStatus ret = softVdpGenerateCSCMatrix(procamp, standard, csc_matrix);
    traceCallHook(VDP_FUNC_ID_GENERATE_CSC_MATRIX, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                          VdpRect const *destination_rect,
                                          VdpOutputSurface source_surface,
                                          VdpRect const *source_rect, VdpColor const *colors,
                                          VdpOutputSurfaceRenderBlendState const *blend_state,
                                          uint32_t flags)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, 0, NULL);
    traceVdpOutputSurfaceRenderOutputSurface("{full}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    VdpStatus ret = softVdpOutputSurfaceRenderOutputSurface(destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                          VdpRect const *destination_rect,
                                          VdpBitmapSurface source_surface,
                                          VdpRect const *source_rect, VdpColor const *colors,
                                          VdpOutputSurfaceRenderBlendState const *blend_state,
                                          uint32_t flags)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, 0, NULL);
    traceVdpOutputSurfaceRenderBitmapSurface("{part}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    VdpStatus ret = softVdpOutputSurfaceRenderBitmapSurface(destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    traceCallHook(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, 0, NULL);
    traceVdpPreemptionCallbackRegister("{zilch/fake success}", device, callback, context);
    VdpStatus ret = softVdpPreemptionCallbackRegister(device, callback, context);
    traceCallHook(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, 1, (void*)ret);
    release_lock();
    return ret;
}


VdpStatus
lockedVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                          VdpPresentationQueueTarget *target)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11, 0, NULL);
    traceVdpPresentationQueueTargetCreateX11("{part}", device, drawable, target);
    VdpStatus ret = softVdpPresentationQueueTargetCreateX11(device, drawable, target);
    traceCallHook(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    acquire_lock();
    traceCallHook(VDP_FUNC_ID_GET_PROC_ADDRESS, 0, NULL);
    traceVdpGetProcAddress("{full}", device, function_id, function_pointer);
    VdpStatus ret = softVdpGetProcAddress(device, function_id, function_pointer);
    traceCallHook(VDP_FUNC_ID_GET_PROC_ADDRESS, 1, (void*)ret);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                         VdpGetProcAddress **get_proc_address)
{
    acquire_lock();
    traceCallHook(-1, 0, NULL);
    traceVdpDeviceCreateX11("{full}", display, screen, device, get_proc_address);
    VdpStatus ret = softVdpDeviceCreateX11(display, screen, device, get_proc_address);
    traceCallHook(-1, 1, (void*)ret);
    release_lock();
    return ret;
}

// wrappers for glX functions

Bool
locked_glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
    XLockDisplay(dpy);
    Bool ret = glXMakeCurrent(dpy, drawable, ctx);
    XUnlockDisplay(dpy);
    return ret;
}

void
locked_glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    XLockDisplay(dpy);
    glXSwapBuffers(dpy, drawable);
    XUnlockDisplay(dpy);
}
