/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#include <stdio.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include "reverse-constant.h"

void
traceEnableTracing(int flag);

void
traceSetTarget(FILE *target);

void
traceResetTarget(void);

void
traceSetHeader(const char *header, const char *header_blank);

void
traceInfo(const char *buf, ...);

void
traceError(const char *buf, ...);

void
traceSetHook(void (*hook)(void *param1, void *param2, int origin, int after), void *param);

void
traceCallHook(int origin, int after, void *shortterm_param);

void
traceVdpGetErrorString(const char *impl_state, VdpStatus status);

void
traceVdpGetApiVersion(const char *impl_state, uint32_t *api_version);

void
traceVdpDecoderQueryCapabilities(const char *impl_state, VdpDevice device,
                                 VdpDecoderProfile profile, VdpBool *is_supported,
                                 uint32_t *max_level, uint32_t *max_macroblocks,
                                 uint32_t *max_width, uint32_t *max_height);

void
traceVdpDecoderCreate(const char *impl_state, VdpDevice device, VdpDecoderProfile profile,
                      uint32_t width, uint32_t height, uint32_t max_references,
                      VdpDecoder *decoder);

void
traceVdpDecoderDestroy(const char *impl_state, VdpDecoder decoder);

void
traceVdpDecoderGetParameters(const char *impl_state, VdpDecoder decoder,
                             VdpDecoderProfile *profile, uint32_t *width, uint32_t *height);

void
traceVdpDecoderRender(const char *impl_state, VdpDecoder decoder, VdpVideoSurface target,
                      VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                      VdpBitstreamBuffer const *bitstream_buffers);

void
traceVdpOutputSurfaceQueryCapabilities(const char *impl_state, VdpDevice device,
                                       VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                                       uint32_t *max_width, uint32_t *max_height);

void
traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(const char *impl_state, VdpDevice device,
                                                       VdpRGBAFormat surface_rgba_format,
                                                       VdpBool *is_supported);

void
traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities(const char *impl_state, VdpDevice device,
                                                     VdpRGBAFormat surface_rgba_format,
                                                     VdpIndexedFormat bits_indexed_format,
                                                     VdpColorTableFormat color_table_format,
                                                     VdpBool *is_supported);

void
traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(const char *impl_state, VdpDevice device,
                                                   VdpRGBAFormat surface_rgba_format,
                                                   VdpYCbCrFormat bits_ycbcr_format,
                                                   VdpBool *is_supported);

void
traceVdpOutputSurfaceCreate(const char *impl_state, VdpDevice device, VdpRGBAFormat rgba_format,
                            uint32_t width, uint32_t height, VdpOutputSurface *surface);

void
traceVdpOutputSurfaceDestroy(const char *impl_state, VdpOutputSurface surface);

void
traceVdpOutputSurfaceGetParameters(const char *impl_state, VdpOutputSurface surface,
                                   VdpRGBAFormat *rgba_format, uint32_t *width, uint32_t *height);

void
traceVdpOutputSurfaceGetBitsNative(const char *impl_state, VdpOutputSurface surface,
                                   VdpRect const *source_rect, void *const *destination_data,
                                   uint32_t const *destination_pitches);

void
traceVdpOutputSurfacePutBitsNative(const char *impl_state, VdpOutputSurface surface,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect);

void
traceVdpOutputSurfacePutBitsIndexed(const char *impl_state, VdpOutputSurface surface,
                                    VdpIndexedFormat source_indexed_format,
                                    void const *const *source_data, uint32_t const *source_pitch,
                                    VdpRect const *destination_rect,
                                    VdpColorTableFormat color_table_format,
                                    void const *color_table);

void
traceVdpOutputSurfacePutBitsYCbCr(const char *impl_state, VdpOutputSurface surface,
                                  VdpYCbCrFormat source_ycbcr_format,
                                  void const *const *source_data, uint32_t const *source_pitches,
                                  VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix);

void
traceVdpVideoMixerQueryFeatureSupport(const char *impl_state, VdpDevice device,
                                      VdpVideoMixerFeature feature, VdpBool *is_supported);

void
traceVdpVideoMixerQueryParameterSupport(const char *impl_state, VdpDevice device,
                                        VdpVideoMixerParameter parameter,
                                        VdpBool *is_supported);

void
traceVdpVideoMixerQueryAttributeSupport(const char *impl_state, VdpDevice device,
                                        VdpVideoMixerAttribute attribute, VdpBool *is_supported);

void
traceVdpVideoMixerQueryParameterValueRange(const char *impl_state, VdpDevice device,
                                           VdpVideoMixerParameter parameter,
                                           void *min_value, void *max_value);

void
traceVdpVideoMixerQueryAttributeValueRange(const char *impl_state, VdpDevice device,
                                           VdpVideoMixerAttribute attribute,
                                           void *min_value, void *max_value);

void
traceVdpVideoMixerCreate(const char *impl_state, VdpDevice device, uint32_t feature_count,
                         VdpVideoMixerFeature const *features, uint32_t parameter_count,
                         VdpVideoMixerParameter const *parameters,
                         void const *const *parameter_values, VdpVideoMixer *mixer);

void
traceVdpVideoMixerSetFeatureEnables(const char *impl_state, VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool const *feature_enables);

void
traceVdpVideoMixerSetAttributeValues(const char *impl_state, VdpVideoMixer mixer,
                                     uint32_t attribute_count,
                                     VdpVideoMixerAttribute const *attributes,
                                     void const *const *attribute_values);

void
traceVdpVideoMixerGetFeatureSupport(const char *impl_state, VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool *feature_supports);

void
traceVdpVideoMixerGetFeatureEnables(const char *impl_state, VdpVideoMixer mixer,
                                    uint32_t feature_count, VdpVideoMixerFeature const *features,
                                    VdpBool *feature_enables);

void
traceVdpVideoMixerGetParameterValues(const char *impl_state, VdpVideoMixer mixer,
                                     uint32_t parameter_count,
                                     VdpVideoMixerParameter const *parameters,
                                     void *const *parameter_values);

void
traceVdpVideoMixerGetAttributeValues(const char *impl_state, VdpVideoMixer mixer,
                                     uint32_t attribute_count,
                                     VdpVideoMixerAttribute const *attributes,
                                     void *const *attribute_values);

void
traceVdpVideoMixerDestroy(const char *impl_state, VdpVideoMixer mixer);

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
                         uint32_t layer_count, VdpLayer const *layers);

void
traceVdpPresentationQueueTargetDestroy(const char *impl_state,
                                       VdpPresentationQueueTarget presentation_queue_target);

void
traceVdpPresentationQueueCreate(const char *impl_state, VdpDevice device,
                                VdpPresentationQueueTarget presentation_queue_target,
                                VdpPresentationQueue *presentation_queue);

void
traceVdpPresentationQueueDestroy(const char *impl_state, VdpPresentationQueue presentation_queue);

void
traceVdpPresentationQueueSetBackgroundColor(const char *impl_state,
                                            VdpPresentationQueue presentation_queue,
                                            VdpColor *const background_color);

void
traceVdpPresentationQueueGetBackgroundColor(const char *impl_state,
                                            VdpPresentationQueue presentation_queue,
                                            VdpColor *background_color);

void
traceVdpPresentationQueueGetTime(const char *impl_state, VdpPresentationQueue presentation_queue,
                                 VdpTime *current_time);

void
traceVdpPresentationQueueDisplay(const char *impl_state, VdpPresentationQueue presentation_queue,
                                 VdpOutputSurface surface, uint32_t clip_width,
                                 uint32_t clip_height, VdpTime earliest_presentation_time);

void
traceVdpPresentationQueueBlockUntilSurfaceIdle(const char *impl_state,
                                               VdpPresentationQueue presentation_queue,
                                               VdpOutputSurface surface,
                                               VdpTime *first_presentation_time);

void
traceVdpPresentationQueueQuerySurfaceStatus(const char *impl_state,
                                            VdpPresentationQueue presentation_queue,
                                            VdpOutputSurface surface,
                                            VdpPresentationQueueStatus *status,
                                            VdpTime *first_presentation_time);

void
traceVdpVideoSurfaceQueryCapabilities(const char *impl_state, VdpDevice device,
                                      VdpChromaType surface_chroma_type, VdpBool *is_supported,
                                      uint32_t *max_width, uint32_t *max_height);

void
traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(const char *impl_state, VdpDevice device,
                                                     VdpChromaType surface_chroma_type,
                                                     VdpYCbCrFormat bits_ycbcr_format,
                                                     VdpBool *is_supported);

void
traceVdpVideoSurfaceCreate(const char *impl_state, VdpDevice device, VdpChromaType chroma_type,
                           uint32_t width, uint32_t height, VdpVideoSurface *surface);

void
traceVdpVideoSurfaceDestroy(const char *impl_state, VdpVideoSurface surface);

void
traceVdpVideoSurfaceGetParameters(const char *impl_state, VdpVideoSurface surface,
                                  VdpChromaType *chroma_type, uint32_t *width, uint32_t *height);

void
traceVdpVideoSurfaceGetBitsYCbCr(const char *impl_state, VdpVideoSurface surface,
                                 VdpYCbCrFormat destination_ycbcr_format,
                                 void *const *destination_data,
                                 uint32_t const *destination_pitches);

void
traceVdpVideoSurfacePutBitsYCbCr(const char *impl_state, VdpVideoSurface surface,
                                 VdpYCbCrFormat source_ycbcr_format, void const *const *source_data,
                                 uint32_t const *source_pitches);

void
traceVdpBitmapSurfaceQueryCapabilities(const char *impl_state, VdpDevice device,
                                       VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                                       uint32_t *max_width, uint32_t *max_height);

void
traceVdpBitmapSurfaceCreate(const char *impl_state, VdpDevice device, VdpRGBAFormat rgba_format,
                            uint32_t width, uint32_t height, VdpBool frequently_accessed,
                            VdpBitmapSurface *surface);

void
traceVdpBitmapSurfaceDestroy(const char *impl_state, VdpBitmapSurface surface);

void
traceVdpBitmapSurfaceGetParameters(const char *impl_state, VdpBitmapSurface surface,
                                   VdpRGBAFormat *rgba_format, uint32_t *width, uint32_t *height,
                                   VdpBool *frequently_accessed);

void
traceVdpBitmapSurfacePutBitsNative(const char *impl_state, VdpBitmapSurface surface,
                                   void const *const *source_data, uint32_t const *source_pitches,
                                   VdpRect const *destination_rect);

void
traceVdpDeviceDestroy(const char *impl_state, VdpDevice device);

void
traceVdpGetInformationString(const char *impl_state, char const **information_string);

void
traceVdpGenerateCSCMatrix(const char *impl_state, VdpProcamp *procamp, VdpColorStandard standard,
                          VdpCSCMatrix *csc_matrix);

void
traceVdpOutputSurfaceRenderOutputSurface(const char *impl_state,
                                         VdpOutputSurface destination_surface,
                                         VdpRect const *destination_rect,
                                         VdpOutputSurface source_surface,
                                         VdpRect const *source_rect, VdpColor const *colors,
                                         VdpOutputSurfaceRenderBlendState const *blend_state,
                                         uint32_t flags);

void
traceVdpOutputSurfaceRenderBitmapSurface(const char *impl_state,
                                         VdpOutputSurface destination_surface,
                                         VdpRect const *destination_rect,
                                         VdpBitmapSurface source_surface,
                                         VdpRect const *source_rect, VdpColor const *colors,
                                         VdpOutputSurfaceRenderBlendState const *blend_state,
                                         uint32_t flags);

void
traceVdpPreemptionCallbackRegister(const char *impl_state, VdpDevice device,
                                   VdpPreemptionCallback callback, void *context);

void
traceVdpPresentationQueueTargetCreateX11(const char *impl_state, VdpDevice device,
                                         Drawable drawable, VdpPresentationQueueTarget *target);

void
traceVdpGetProcAddress(const char *impl_state, VdpDevice device, VdpFuncId function_id,
                       void **function_pointer);

void
traceVdpDeviceCreateX11(const char *trace_state, Display *display, int screen, VdpDevice *device,
                        VdpGetProcAddress **get_proc_address);
