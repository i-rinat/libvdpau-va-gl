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
    traceVdpGetApiVersion("{full}", api_version);
    VdpStatus ret = softVdpGetApiVersion(api_version);
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
    traceVdpDecoderQueryCapabilities("{zilch}", device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    VdpStatus ret = softVdpDecoderQueryCapabilities(device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                       uint32_t max_references, VdpDecoder *decoder)
{
    acquire_lock();
    traceVdpDecoderCreate("{full}", device, profile, width, height, max_references, decoder);
    VdpStatus ret = softVdpDecoderCreate(device, profile, width, height, max_references, decoder);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderDestroy(VdpDecoder decoder)
{
    acquire_lock();
    traceVdpDecoderDestroy("{full}", decoder);
    VdpStatus ret = softVdpDecoderDestroy(decoder);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                              uint32_t *width, uint32_t *height)
{
    acquire_lock();
    traceVdpDecoderGetParameters("{zilch}", decoder, profile, width, height);
    VdpStatus ret = softVdpDecoderGetParameters(decoder, profile, width, height);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                       VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                       VdpBitstreamBuffer const *bitstream_buffers)
{
    acquire_lock();
    traceVdpDecoderRender("{WIP}", decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    VdpStatus ret = softVdpDecoderRender(decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                        VdpBool *is_supported, uint32_t *max_width,
                                        uint32_t *max_height)
{
    acquire_lock();
    traceVdpOutputSurfaceQueryCapabilities("{zilch}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    VdpStatus ret = softVdpOutputSurfaceQueryCapabilities(device, surface_rgba_format, is_supported,
                                                          max_width, max_height);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                        VdpRGBAFormat surface_rgba_format,
                                                        VdpBool *is_supported)
{
    acquire_lock();
    traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities("{zilch}", device, surface_rgba_format,
        is_supported);
    VdpStatus ret =
        softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(device, surface_rgba_format,
                                                              is_supported);
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
    traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities("{zilch}", device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
    VdpStatus ret = softVdpOutputSurfaceQueryPutBitsIndexedCapabilities(device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
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
    traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities("{zilch}", device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    VdpStatus ret = softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                             uint32_t height, VdpOutputSurface *surface)
{
    acquire_lock();
    traceVdpOutputSurfaceCreate("{part}", device, rgba_format, width, height, surface);
    VdpStatus ret = softVdpOutputSurfaceCreate(device, rgba_format, width, height, surface);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    acquire_lock();
    traceVdpOutputSurfaceDestroy("{full}", surface);
    VdpStatus ret = softVdpOutputSurfaceDestroy(surface);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                    uint32_t *width, uint32_t *height)
{
    acquire_lock();
    traceVdpOutputSurfaceGetParameters("{zilch}", surface, rgba_format, width, height);
    VdpStatus ret = softVdpOutputSurfaceGetParameters(surface, rgba_format, width, height);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                    void *const *destination_data,
                                    uint32_t const *destination_pitches)
{
    acquire_lock();
    traceVdpOutputSurfaceGetBitsNative("{WIP}", surface, source_rect, destination_data,
        destination_pitches);
    VdpStatus ret = softVdpOutputSurfaceGetBitsNative(surface, source_rect, destination_data,
        destination_pitches);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                    uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    acquire_lock();
    traceVdpOutputSurfacePutBitsNative("{full}", surface, source_data, source_pitches,
        destination_rect);
    VdpStatus ret = softVdpOutputSurfacePutBitsNative(surface, source_data, source_pitches,
        destination_rect);
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
    traceVdpOutputSurfacePutBitsIndexed("{part}", surface, source_indexed_format, source_data,
        source_pitch, destination_rect, color_table_format, color_table);
    VdpStatus ret = softVdpOutputSurfacePutBitsIndexed(surface, source_indexed_format, source_data,
        source_pitch, destination_rect, color_table_format, color_table);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    acquire_lock();
    traceVdpOutputSurfacePutBitsYCbCr("{zilch}", surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    VdpStatus ret = softVdpOutputSurfacePutBitsYCbCr(surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                       VdpBool *is_supported)
{
    acquire_lock();
    traceVdpVideoMixerQueryFeatureSupport("{zilch}", device, feature, is_supported);
    VdpStatus ret = softVdpVideoMixerQueryFeatureSupport(device, feature, is_supported);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                         VdpBool *is_supported)
{
    acquire_lock();
    traceVdpVideoMixerQueryParameterSupport("{zilch}", device, parameter, is_supported);
    VdpStatus ret = softVdpVideoMixerQueryParameterSupport(device, parameter, is_supported);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                         VdpBool *is_supported)
{
    acquire_lock();
    traceVdpVideoMixerQueryAttributeSupport("{zilch}", device, attribute, is_supported);
    VdpStatus ret = softVdpVideoMixerQueryAttributeSupport(device, attribute, is_supported);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                            void *min_value, void *max_value)
{
    acquire_lock();
    traceVdpVideoMixerQueryParameterValueRange("{zilch}", device, parameter, min_value, max_value);
    VdpStatus ret = softVdpVideoMixerQueryParameterValueRange(device, parameter, min_value,
                                                              max_value);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                            void *min_value, void *max_value)
{
    acquire_lock();
    traceVdpVideoMixerQueryAttributeValueRange("{zilch}", device, attribute, min_value, max_value);
    VdpStatus ret = softVdpVideoMixerQueryAttributeValueRange(device, attribute, min_value,
                                                              max_value);
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
    traceVdpVideoMixerCreate("{part}", device, feature_count, features, parameter_count, parameters,
        parameter_values, mixer);
    VdpStatus ret = softVdpVideoMixerCreate(device, feature_count, features, parameter_count,
                                            parameters, parameter_values, mixer);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                     VdpVideoMixerFeature const *features,
                                     VdpBool const *feature_enables)
{
    acquire_lock();
    traceVdpVideoMixerSetFeatureEnables("{part}", mixer, feature_count, features, feature_enables);
    VdpStatus ret = softVdpVideoMixerSetFeatureEnables(mixer, feature_count, features,
                                                       feature_enables);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                      VdpVideoMixerAttribute const *attributes,
                                      void const *const *attribute_values)
{
    acquire_lock();
    traceVdpVideoMixerSetAttributeValues("{part}", mixer, attribute_count, attributes,
        attribute_values);
    VdpStatus ret = softVdpVideoMixerSetAttributeValues(mixer, attribute_count, attributes,
        attribute_values);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                     VdpVideoMixerFeature const *features,
                                     VdpBool *feature_supports)
{
    acquire_lock();
    traceVdpVideoMixerGetFeatureSupport("{zilch}", mixer, feature_count, features,
        feature_supports);
    VdpStatus ret = softVdpVideoMixerGetFeatureSupport(mixer, feature_count, features,
        feature_supports);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                     VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    acquire_lock();
    traceVdpVideoMixerGetFeatureEnables("{zilch}", mixer, feature_count, features, feature_enables);
    VdpStatus ret = softVdpVideoMixerGetFeatureEnables(mixer, feature_count, features,
                                                       feature_enables);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                      VdpVideoMixerParameter const *parameters,
                                      void *const *parameter_values)
{
    acquire_lock();
    traceVdpVideoMixerGetParameterValues("{zilch}", mixer, parameter_count, parameters,
        parameter_values);
    VdpStatus ret = softVdpVideoMixerGetParameterValues(mixer, parameter_count, parameters,
        parameter_values);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                      VdpVideoMixerAttribute const *attributes,
                                      void *const *attribute_values)
{
    acquire_lock();
    traceVdpVideoMixerGetAttributeValues("{zilch}", mixer, attribute_count, attributes,
        attribute_values);
    VdpStatus ret = softVdpVideoMixerGetAttributeValues(mixer, attribute_count, attributes,
        attribute_values);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    acquire_lock();
    traceVdpVideoMixerDestroy("{full}", mixer);
    VdpStatus ret = softVdpVideoMixerDestroy(mixer);
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
    traceVdpVideoMixerRender("{part}", mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);
    VdpStatus ret = softVdpVideoMixerRender(mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    acquire_lock();
    traceVdpPresentationQueueTargetDestroy("{full}", presentation_queue_target);
    VdpStatus ret = softVdpPresentationQueueTargetDestroy(presentation_queue_target);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueCreate(VdpDevice device,
                                 VdpPresentationQueueTarget presentation_queue_target,
                                 VdpPresentationQueue *presentation_queue)
{
    acquire_lock();
    traceVdpPresentationQueueCreate("{part}", device, presentation_queue_target,
        presentation_queue);
    VdpStatus ret = softVdpPresentationQueueCreate(device, presentation_queue_target,
        presentation_queue);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    traceVdpPresentationQueueDestroy("{full}", presentation_queue);
    acquire_lock();
    VdpStatus ret = softVdpPresentationQueueDestroy(presentation_queue);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                             VdpColor *const background_color)
{
    acquire_lock();
    traceVdpPresentationQueueSetBackgroundColor("{full}", presentation_queue, background_color);
    VdpStatus ret = softVdpPresentationQueueSetBackgroundColor(presentation_queue,
                                                               background_color);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                             VdpColor *background_color)
{
    acquire_lock();
    traceVdpPresentationQueueGetBackgroundColor("{full}", presentation_queue, background_color);
    VdpStatus ret = softVdpPresentationQueueGetBackgroundColor(presentation_queue,
                                                               background_color);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue, VdpTime *current_time)
{
    acquire_lock();
    traceVdpPresentationQueueGetTime("{full}", presentation_queue, current_time);
    VdpStatus ret = softVdpPresentationQueueGetTime(presentation_queue, current_time);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                  uint32_t clip_width, uint32_t clip_height,
                                  VdpTime earliest_presentation_time)
{
    acquire_lock();
    traceVdpPresentationQueueDisplay("{full}", presentation_queue, surface, clip_width, clip_height,
        earliest_presentation_time);
    VdpStatus ret = softVdpPresentationQueueDisplay(presentation_queue, surface, clip_width,
                                                    clip_height, earliest_presentation_time);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                                VdpOutputSurface surface,
                                                VdpTime *first_presentation_time)

{
    acquire_lock();
    traceVdpPresentationQueueBlockUntilSurfaceIdle("{full}", presentation_queue, surface,
        first_presentation_time);
    VdpStatus ret = softVdpPresentationQueueBlockUntilSurfaceIdle(presentation_queue, surface,
        first_presentation_time);
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
    traceVdpPresentationQueueQuerySurfaceStatus("{part}", presentation_queue, surface,
        status, first_presentation_time);
    VdpStatus ret = softVdpPresentationQueueQuerySurfaceStatus(presentation_queue, surface,
        status, first_presentation_time);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                       VdpBool *is_supported, uint32_t *max_width,
                                       uint32_t *max_height)
{
    acquire_lock();
    traceVdpVideoSurfaceQueryCapabilities("{part}", device, surface_chroma_type, is_supported,
        max_width, max_height);
    VdpStatus ret = softVdpVideoSurfaceQueryCapabilities(device, surface_chroma_type, is_supported,
        max_width, max_height);
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
    traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities("{part}", device, surface_chroma_type,
        bits_ycbcr_format, is_supported);
    VdpStatus ret = softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(device, surface_chroma_type,
        bits_ycbcr_format, is_supported);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                            uint32_t height, VdpVideoSurface *surface)
{
    acquire_lock();
    traceVdpVideoSurfaceCreate("{part}", device, chroma_type, width, height, surface);
    VdpStatus ret = softVdpVideoSurfaceCreate(device, chroma_type, width, height, surface);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    acquire_lock();
    traceVdpVideoSurfaceDestroy("{full}", surface);
    VdpStatus ret = softVdpVideoSurfaceDestroy(surface);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                   uint32_t *width, uint32_t *height)
{
    acquire_lock();
    traceVdpVideoSurfaceGetParameters("{zilch}", surface, chroma_type, width, height);
    VdpStatus ret = softVdpVideoSurfaceGetParameters(surface, chroma_type, width, height);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    acquire_lock();
    traceVdpVideoSurfaceGetBitsYCbCr("{part}", surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    VdpStatus ret = softVdpVideoSurfaceGetBitsYCbCr(surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                  void const *const *source_data, uint32_t const *source_pitches)
{
    acquire_lock();
    traceVdpVideoSurfacePutBitsYCbCr("{part}", surface, source_ycbcr_format, source_data,
        source_pitches);
    VdpStatus ret = softVdpVideoSurfacePutBitsYCbCr(surface, source_ycbcr_format, source_data,
        source_pitches);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                        VdpBool *is_supported, uint32_t *max_width,
                                        uint32_t *max_height)
{
    acquire_lock();
    traceVdpBitmapSurfaceQueryCapabilities("{full}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    VdpStatus ret = softVdpBitmapSurfaceQueryCapabilities(device, surface_rgba_format, is_supported,
        max_width, max_height);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                             uint32_t height, VdpBool frequently_accessed,
                             VdpBitmapSurface *surface)
{
    acquire_lock();
    traceVdpBitmapSurfaceCreate("{full}", device, rgba_format, width, height, frequently_accessed,
        surface);
    VdpStatus ret = softVdpBitmapSurfaceCreate(device, rgba_format, width, height,
                                               frequently_accessed, surface);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    acquire_lock();
    traceVdpBitmapSurfaceDestroy("{full}", surface);
    VdpStatus ret = softVdpBitmapSurfaceDestroy(surface);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                    uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    acquire_lock();
    traceVdpBitmapSurfaceGetParameters("{full}", surface, rgba_format, width, height,
        frequently_accessed);
    VdpStatus ret = softVdpBitmapSurfaceGetParameters(surface, rgba_format, width, height,
        frequently_accessed);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                    uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    acquire_lock();
    traceVdpBitmapSurfacePutBitsNative("{full}", surface, source_data, source_pitches,
        destination_rect);
    VdpStatus ret = softVdpBitmapSurfacePutBitsNative(surface, source_data, source_pitches,
        destination_rect);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDeviceDestroy(VdpDevice device)
{
    acquire_lock();
    traceVdpDeviceDestroy("{full}", device);
    VdpStatus ret = softVdpDeviceDestroy(device);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpGetInformationString(char const **information_string)
{
    acquire_lock();
    traceVdpGetInformationString("{full}", information_string);
    VdpStatus ret = softVdpGetInformationString(information_string);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    acquire_lock();
    traceVdpGenerateCSCMatrix("{part}", procamp, standard, csc_matrix);
    VdpStatus ret = softVdpGenerateCSCMatrix(procamp, standard, csc_matrix);
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
    traceVdpOutputSurfaceRenderOutputSurface("{full}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    VdpStatus ret = softVdpOutputSurfaceRenderOutputSurface(destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
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
    traceVdpOutputSurfaceRenderBitmapSurface("{part}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    VdpStatus ret = softVdpOutputSurfaceRenderBitmapSurface(destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    acquire_lock();
    traceVdpPreemptionCallbackRegister("{zilch/fake success}", device, callback, context);
    VdpStatus ret = softVdpPreemptionCallbackRegister(device, callback, context);
    release_lock();
    return ret;
}


VdpStatus
lockedVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                          VdpPresentationQueueTarget *target)
{
    acquire_lock();
    traceVdpPresentationQueueTargetCreateX11("{part}", device, drawable, target);
    VdpStatus ret = softVdpPresentationQueueTargetCreateX11(device, drawable, target);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    acquire_lock();
    traceVdpGetProcAddress("{full}", device, function_id, function_pointer);
    VdpStatus ret = softVdpGetProcAddress(device, function_id, function_pointer);
    release_lock();
    return ret;
}

VdpStatus
lockedVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                         VdpGetProcAddress **get_proc_address)
{
    acquire_lock();
    traceVdpDeviceCreateX11("{full}", display, screen, device, get_proc_address);
    VdpStatus ret = softVdpDeviceCreateX11(display, screen, device, get_proc_address);
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
