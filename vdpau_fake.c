#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/time.h>
#include "reverse-constant.h"
#include "handle-storage.h"

#ifndef NDEBUG
    #define TRACE(str, ...)  printf("[VDPFAKE] " str "\n", __VA_ARGS__)
    #define TRACE1(str)  printf("[VDPFAKE] " str "\n")
#else
    #define TRACE(str, ...)
    #define TRACE1(str)
#endif

static
const char *
fakeVdpGetErrorString(VdpStatus status)
{
    switch (status) {
    case VDP_STATUS_OK:
        return "VDP_STATUS_OK";
        break;
    case VDP_STATUS_NO_IMPLEMENTATION:
        return "VDP_STATUS_NO_IMPLEMENTATION";
        break;
    case VDP_STATUS_DISPLAY_PREEMPTED:
        return "VDP_STATUS_DISPLAY_PREEMPTED";
        break;
    case VDP_STATUS_INVALID_HANDLE:
        return "VDP_STATUS_INVALID_HANDLE";
        break;
    case VDP_STATUS_INVALID_POINTER:
        return "VDP_STATUS_INVALID_POINTER";
        break;
    case VDP_STATUS_INVALID_CHROMA_TYPE:
        return "VDP_STATUS_INVALID_CHROMA_TYPE";
        break;
    case VDP_STATUS_INVALID_Y_CB_CR_FORMAT:
        return "VDP_STATUS_INVALID_Y_CB_CR_FORMAT";
        break;
    case VDP_STATUS_INVALID_RGBA_FORMAT:
        return "VDP_STATUS_INVALID_RGBA_FORMAT";
        break;
    case VDP_STATUS_INVALID_INDEXED_FORMAT:
        return "VDP_STATUS_INVALID_INDEXED_FORMAT";
        break;
    case VDP_STATUS_INVALID_COLOR_STANDARD:
        return "VDP_STATUS_INVALID_COLOR_STANDARD";
        break;
    case VDP_STATUS_INVALID_COLOR_TABLE_FORMAT:
        return "VDP_STATUS_INVALID_COLOR_TABLE_FORMAT";
        break;
    case VDP_STATUS_INVALID_BLEND_FACTOR:
        return "VDP_STATUS_INVALID_BLEND_FACTOR";
        break;
    case VDP_STATUS_INVALID_BLEND_EQUATION:
        return "VDP_STATUS_INVALID_BLEND_EQUATION";
        break;
    case VDP_STATUS_INVALID_FLAG:
        return "VDP_STATUS_INVALID_FLAG";
        break;
    case VDP_STATUS_INVALID_DECODER_PROFILE:
        return "VDP_STATUS_INVALID_DECODER_PROFILE";
        break;
    case VDP_STATUS_INVALID_VIDEO_MIXER_FEATURE:
        return "VDP_STATUS_INVALID_VIDEO_MIXER_FEATURE";
        break;
    case VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER:
        return "VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER";
        break;
    case VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE:
        return "VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE";
        break;
    case VDP_STATUS_INVALID_VIDEO_MIXER_PICTURE_STRUCTURE:
        return "VDP_STATUS_INVALID_VIDEO_MIXER_PICTURE_STRUCTURE";
        break;
    case VDP_STATUS_INVALID_FUNC_ID:
        return "VDP_STATUS_INVALID_FUNC_ID";
        break;
    case VDP_STATUS_INVALID_SIZE:
        return "VDP_STATUS_INVALID_SIZE";
        break;
    case VDP_STATUS_INVALID_VALUE:
        return "VDP_STATUS_INVALID_VALUE";
        break;
    case VDP_STATUS_INVALID_STRUCT_VERSION:
        return "VDP_STATUS_INVALID_STRUCT_VERSION";
        break;
    case VDP_STATUS_RESOURCES:
        return "VDP_STATUS_RESOURCES";
        break;
    case VDP_STATUS_HANDLE_DEVICE_MISMATCH:
        return "VDP_STATUS_HANDLE_DEVICE_MISMATCH";
        break;
    case VDP_STATUS_ERROR:
        return "VDP_STATUS_ERROR";
        break;
    default:
        return "VDP Unknown error";
        break;
    } // switch
}

static
VdpStatus
fakeVdpGetApiVersion(uint32_t *api_version)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                                uint32_t *max_level, uint32_t *max_macroblocks,
                                uint32_t *max_width, uint32_t *max_height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                     uint32_t max_references, VdpDecoder *decoder)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderDestroy(VdpDecoder decoder)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                            uint32_t *width, uint32_t *height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                     VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                     VdpBitstreamBuffer const *bitstream_buffers)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpBool *is_supported)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                    VdpRGBAFormat surface_rgba_format,
                                                    VdpIndexedFormat bits_indexed_format,
                                                    VdpColorTableFormat color_table_format,
                                                    VdpBool *is_supported)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpOutputSurface *surface)
{
    TRACE1("fakeVdpOutputSurfaceCreate");
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    TRACE1("fakeVdpVideoMixerQueryParameterSupport stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
                                   void const *const *source_data, uint32_t const *source_pitch,
                                   VdpRect const *destination_rect,
                                   VdpColorTableFormat color_table_format, void const *color_table)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                 void const *const *source_data, uint32_t const *source_pitches,
                                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                     VdpBool *is_supported)
{
    TRACE1("fakeVdpVideoMixerQueryFeatureSupport stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                       VdpBool *is_supported)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                       VdpBool *is_supported)
{
    TRACE1("fakeVdpVideoMixerQueryAttributeSupport stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                          void *min_value, void *max_value)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                          void *min_value, void *max_value)
{
    TRACE1("fakeVdpVideoMixerQueryAttributeValueRange stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                        VdpVideoMixerFeature const *features, uint32_t parameter_count,
                        VdpVideoMixerParameter const *parameters,
                        void const *const *parameter_values, VdpVideoMixer *mixer)
{
    TRACE("fakeVdpVideoMixerCreate stub feature_count=%d, parameter_count=%d",
        feature_count, parameter_count);

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features,
                                   VdpBool const *feature_enables)
{
    TRACE("fakeVdpVideoMixerSetFeatureEnables mixer=%d, feature_count=%d", mixer, feature_count);
    for (uint32_t k = 0; k < feature_count; k ++) {
        TRACE("   feature %d %s (%s)", features[k], feature_enables[k] ? "enabled" : "disabled",
            reverse_video_mixer_feature(features[k]));
    }
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void const *const *attribute_values)
{
    TRACE("fakeVdpVideoMixerSetAttributeValues stub mixer=%d, attribute_count=%d",
        mixer, attribute_count);
    for (uint32_t k = 0; k < attribute_count; k ++) {
        TRACE("   attribute %d (%s)", attributes[k], reverse_video_mixer_attributes(attributes[k]));
        if (VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX == attributes[k]) {
            VdpCSCMatrix *matrix = (VdpCSCMatrix *)attribute_values[k];
            for (uint32_t j1 = 0; j1 < 3; j1 ++) {
                printf("      ");
                for (uint32_t j2 = 0; j2 < 4; j2 ++) {
                    printf("%11f", (double)*matrix[j1][j2]);
                }
                printf("\n");
            }
        }
    }

    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    TRACE1("fakeVdpVideoMixerGetFeatureSupport stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    TRACE1("fakeVdpVideoMixerGetFeatureEnables stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                    VdpVideoMixerParameter const *parameters,
                                    void *const *parameter_values)
{
    TRACE1("fakeVdpVideoMixerGetParameterValues stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void *const *attribute_values)
{
    TRACE1("fakeVdpVideoMixerGetAttributeValues stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    TRACE1("fakeVdpVideoMixerDestroy stub");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerRender(VdpVideoMixer mixer, VdpOutputSurface background_surface,
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
    TRACE1("fakeVdpVideoMixerRender");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueCreate(VdpDevice device,
                               VdpPresentationQueueTarget presentation_queue_target,
                               VdpPresentationQueue *presentation_queue)
{
    TRACE("fakeVdpPresentationQueueCreate device=%d, presentation_queue_target=%d",
        device, presentation_queue_target);
    VdpPresentationQueueData *data =
        (VdpPresentationQueueData *)calloc(1, sizeof(VdpPresentationQueueData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;
    if (!handlestorage_valid(device, HANDLE_TYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;
    if (!handlestorage_valid(presentation_queue_target, HANDLE_TYPE_PRESENTATION_QUEUE_TARGET))
        return VDP_STATUS_INVALID_HANDLE;

    data->type = HANDLE_TYPE_PRESENTATION_QUEUE;
    data->device = device;
    data->presentationQueueTarget = presentation_queue_target;

    *presentation_queue = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *const background_color)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *background_color)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                                VdpTime *current_time)
{
    TRACE("fakeVdpPresentationQueueGetTime presentation_queue=%d", presentation_queue);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *current_time = (uint64_t)tv.tv_sec * 1000000000LL + (uint64_t)tv.tv_usec * 1000LL;
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                uint32_t clip_width, uint32_t clip_height,
                                VdpTime earliest_presentation_time)
{
    TRACE1("fakeVdpPresentationQueueDisplay");
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                              VdpOutputSurface surface,
                                              VdpTime *first_presentation_time)

{
    TRACE1("fakeVdpPresentationQueueBlockUntilSurfaceIdle");
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                           VdpOutputSurface surface,
                                           VdpPresentationQueueStatus *status,
                                           VdpTime *first_presentation_time)
{
    *status = VDP_PRESENTATION_QUEUE_STATUS_VISIBLE;
    TRACE1("fakeVdpPresentationQueueQuerySurfaceStatus");
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpBool *is_supported, uint32_t *max_width,
                                     uint32_t *max_height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpChromaType surface_chroma_type,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                          uint32_t height, VdpVideoSurface *surface)
{
    TRACE1("fakeVdpVideoSurfaceCreate");
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                 uint32_t *width, uint32_t *height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                void const *const *source_data, uint32_t const *source_pitches)
{
    TRACE("fakeVdpVideoSurfacePutBitsYCbCr, %d, %d, %p", surface, source_ycbcr_format, *source_data);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDeviceDestroy(VdpDevice device)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpGetInformationString(char const **information_string)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    TRACE1("fakeVdpGenerateCSCMatrix not implemented");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpOutputSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpBitmapSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                        VdpPresentationQueueTarget *target)
{
    TRACE("fakeVdpPresentationQueueTargetCreateX11, device=%d, drawable=%u", device,
        ((unsigned int)drawable));

    VdpPresentationQueueTargetData *data =
        (VdpPresentationQueueTargetData *)malloc(sizeof(VdpPresentationQueueTargetData));
    if (NULL == data)
        return VDP_STATUS_ERROR;

    if (!handlestorage_valid(device, HANDLE_TYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    data->type = HANDLE_TYPE_PRESENTATION_QUEUE_TARGET;
    data->device = device;
    data->drawable = drawable;

    *target = handlestorage_add(data);

    return VDP_STATUS_OK;
}

// =========================
static
VdpStatus
fakeVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    TRACE("fakeVdpGetProcAddress, device=%d, function_id=%s", device, reverse_func_id(function_id));
    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = &fakeVdpGetErrorString;
        break;
    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = &fakeVdpGetProcAddress;
        break;
    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = &fakeVdpGetApiVersion;
        break;
    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = &fakeVdpGetInformationString;
        break;
    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = &fakeVdpDeviceDestroy;
        break;
    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = &fakeVdpGenerateCSCMatrix;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &fakeVdpVideoSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &fakeVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = &fakeVdpVideoSurfaceCreate;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = &fakeVdpVideoSurfaceDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = &fakeVdpVideoSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = &fakeVdpVideoSurfaceGetBitsYCbCr;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &fakeVdpVideoSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &fakeVdpOutputSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = &fakeVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = &fakeVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &fakeVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = &fakeVdpOutputSurfaceCreate;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = &fakeVdpOutputSurfaceDestroy;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = &fakeVdpOutputSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = &fakeVdpOutputSurfaceGetBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &fakeVdpOutputSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = &fakeVdpOutputSurfacePutBitsIndexed;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &fakeVdpOutputSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &fakeVdpBitmapSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = &fakeVdpBitmapSurfaceCreate;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = &fakeVdpBitmapSurfaceDestroy;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = &fakeVdpBitmapSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &fakeVdpBitmapSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = &fakeVdpOutputSurfaceRenderOutputSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = &fakeVdpOutputSurfaceRenderBitmapSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // *function_pointer = &fakeVdpOutputSurfaceRenderVideoSurfaceLuma;
        *function_pointer = NULL;
        break;
    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = &fakeVdpDecoderQueryCapabilities;
        break;
    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = &fakeVdpDecoderCreate;
        break;
    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = &fakeVdpDecoderDestroy;
        break;
    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = &fakeVdpDecoderGetParameters;
        break;
    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = &fakeVdpDecoderRender;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = &fakeVdpVideoMixerQueryFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = &fakeVdpVideoMixerQueryParameterSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = &fakeVdpVideoMixerQueryAttributeSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = &fakeVdpVideoMixerQueryParameterValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = &fakeVdpVideoMixerQueryAttributeValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = &fakeVdpVideoMixerCreate;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = &fakeVdpVideoMixerSetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = &fakeVdpVideoMixerSetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = &fakeVdpVideoMixerGetFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = &fakeVdpVideoMixerGetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = &fakeVdpVideoMixerGetParameterValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = &fakeVdpVideoMixerGetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = &fakeVdpVideoMixerDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = &fakeVdpVideoMixerRender;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = &fakeVdpPresentationQueueTargetDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = &fakeVdpPresentationQueueCreate;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = &fakeVdpPresentationQueueDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = &fakeVdpPresentationQueueSetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = &fakeVdpPresentationQueueGetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = &fakeVdpPresentationQueueGetTime;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = &fakeVdpPresentationQueueDisplay;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = &fakeVdpPresentationQueueBlockUntilSurfaceIdle;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = &fakeVdpPresentationQueueQuerySurfaceStatus;
        break;
    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = &fakeVdpPreemptionCallbackRegister;
        break;
    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = &fakeVdpPresentationQueueTargetCreateX11;
        break;
    default:
        *function_pointer = NULL;
        break;
    } // switch

    if (NULL == *function_pointer)
        return VDP_STATUS_INVALID_FUNC_ID;
    return VDP_STATUS_OK;
}

__attribute__ ((visibility("default")))
VdpStatus
vdp_imp_device_create_x11(Display *display, int screen, VdpDevice *device,
                          VdpGetProcAddress **get_proc_address)
{
    TRACE("vdp_imp_device_create_x11 display=%p, screen=%d", display, screen);
    if (NULL == display)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *data = (VdpDeviceData *)malloc(sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLE_TYPE_DEVICE;
    data->display = display;
    data->screen = screen;

    *device = handlestorage_add(data);
    *get_proc_address = &fakeVdpGetProcAddress;

    return VDP_STATUS_OK;
}

static
void
__fake_init(void) __attribute__((constructor));

static
void
__fake_init(void)
{
    handlestorage_initialize();
    TRACE1("library initialized");
}
