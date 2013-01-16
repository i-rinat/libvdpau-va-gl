#define _XOPEN_SOURCE
#include <assert.h>
#include <cairo.h>
#include <glib.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <X11/extensions/XShm.h>
#include "reverse-constant.h"
#include "handle-storage.h"

#ifndef NDEBUG
    #define TRACE(str, ...)  printf("[VDPSOFT] " str "\n", __VA_ARGS__)
    #define TRACE1(str)  printf("[VDPSOFT] " str "\n")
#else
    #define TRACE(str, ...)
    #define TRACE1(str)
#endif

char const *implemetation_description_string = "VAAPI/software backend for VDPAU";

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
softVdpGetApiVersion(uint32_t *api_version)
{
    TRACE1("{full} VdpGetApiVersion");
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                                uint32_t *max_level, uint32_t *max_macroblocks,
                                uint32_t *max_width, uint32_t *max_height)
{
    TRACE1("{zilch} VdpDecoderQueryCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                     uint32_t max_references, VdpDecoder *decoder)
{
    TRACE1("{zilch} VdpDecoderCreate");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderDestroy(VdpDecoder decoder)
{
    TRACE1("{zilch} VdpDecoderDestroy");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                            uint32_t *width, uint32_t *height)
{
    TRACE1("{zilch} VdpDecoderGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                     VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                     VdpBitstreamBuffer const *bitstream_buffers)
{
    TRACE1("{zilch} VdpDecoderRender");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpBool *is_supported)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryGetPutBitsNativeCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                    VdpRGBAFormat surface_rgba_format,
                                                    VdpIndexedFormat bits_indexed_format,
                                                    VdpColorTableFormat color_table_format,
                                                    VdpBool *is_supported)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryPutBitsIndexedCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryPutBitsYCbCrCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpOutputSurface *surface)
{
    TRACE("{part} VdpOutputSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d",
        device, reverse_rgba_format(rgba_format), width, height);
    if (! handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    if (width > 4096 || height > 4096)
        return VDP_STATUS_INVALID_SIZE;


    if (VDP_RGBA_FORMAT_B8G8R8A8 != rgba_format) {
#ifndef NDEBUG
        printf("  unsupported RGBA format\n");
#endif
        return VDP_STATUS_INVALID_RGBA_FORMAT;
    }

    VdpOutputSurfaceData *data = (VdpOutputSurfaceData *)calloc(1, sizeof(VdpOutputSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_OUTPUT_SURFACE;
    data->device = device;
    data->rgba_format = rgba_format;
    data->cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (CAIRO_STATUS_SUCCESS != cairo_surface_status(data->cairo_surface)) {
        cairo_surface_destroy(data->cairo_surface);
        free(data);
        return VDP_STATUS_RESOURCES;
    }

    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    TRACE("{full} VdpOutputSurfaceDestroy surface=%d", surface);
    VdpOutputSurfaceData *data = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    cairo_surface_destroy(data->cairo_surface);
    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height)
{
    TRACE1("{zilch} VdpOutputSurfaceGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    TRACE1("{zilch} VdpOutputSurfaceGetBitsNative");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    TRACE1("{zilch} VdpVideoMixerQueryParameterSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
                                   void const *const *source_data, uint32_t const *source_pitch,
                                   VdpRect const *destination_rect,
                                   VdpColorTableFormat color_table_format, void const *color_table)
{
    TRACE1("{zilch} VdpOutputSurfacePutBitsIndexed");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                 void const *const *source_data, uint32_t const *source_pitches,
                                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    TRACE1("{zilch} VdpOutputSurfacePutBitsYCbCr");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                     VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoMixerQueryFeatureSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                       VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoMixerQueryParameterSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                       VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoMixerQueryAttributeSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                          void *min_value, void *max_value)
{
    TRACE1("{zilch} VdpVideoMixerQueryParameterValueRange");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                          void *min_value, void *max_value)
{
    TRACE1("{zilch} VdpVideoMixerQueryAttributeValueRange");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                        VdpVideoMixerFeature const *features, uint32_t parameter_count,
                        VdpVideoMixerParameter const *parameters,
                        void const *const *parameter_values, VdpVideoMixer *mixer)
{
    TRACE("{part} VdpVideoMixerCreate feature_count=%d, parameter_count=%d",
        feature_count, parameter_count);
#ifndef NDEBUG
    for (uint32_t k = 0; k < parameter_count; k ++) {
        printf("      parameter ");
        switch (parameters[k]) {
        case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
            printf("video surface width = %d\n", *(uint32_t*)parameter_values[k]);
            break;
        case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
            printf("video surface height = %d\n", *(uint32_t*)parameter_values[k]);
            break;
        case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
            printf("chroma type = %s\n", reverse_chroma_type(*(uint32_t*)parameter_values[k]));
            break;
        case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
            printf("layers = %d\n", *(uint32_t*)parameter_values[k]);
            break;
        default:
            return VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER;
        }
    }
#endif

    if (!handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoMixerData *data = (VdpVideoMixerData *)calloc(1, sizeof(VdpVideoMixerData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_VIDEO_MIXER;
    data->device = device;

    *mixer = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features,
                                   VdpBool const *feature_enables)
{
    TRACE("{part} VdpVideoMixerSetFeatureEnables mixer=%d, feature_count=%d", mixer, feature_count);
    for (uint32_t k = 0; k < feature_count; k ++) {
        TRACE("   feature %d %s (%s)", features[k], feature_enables[k] ? "enabled" : "disabled",
            reverse_video_mixer_feature(features[k]));
    }
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void const *const *attribute_values)
{
    TRACE("{part} VdpVideoMixerSetAttributeValues mixer=%d, attribute_count=%d",
        mixer, attribute_count);
#ifndef NDEBUG
    for (uint32_t k = 0; k < attribute_count; k ++) {
        TRACE("   attribute %d (%s)", attributes[k], reverse_video_mixer_attributes(attributes[k]));
        if (VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX == attributes[k]) {
            VdpCSCMatrix *matrix = (VdpCSCMatrix *)(attribute_values[k]);
            for (uint32_t j1 = 0; j1 < 3; j1 ++) {
                printf("      ");
                for (uint32_t j2 = 0; j2 < 4; j2 ++) {
                    printf("%11f", (double)((*matrix)[j1][j2]));
                }
                printf("\n");
            }
        }
    }
#endif

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    TRACE1("{zilch} VdpVideoMixerGetFeatureSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    TRACE1("{zilch} VdpVideoMixerGetFeatureEnables");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                    VdpVideoMixerParameter const *parameters,
                                    void *const *parameter_values)
{
    TRACE1("{zilch} VdpVideoMixerGetParameterValues");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void *const *attribute_values)
{
    TRACE1("{zilch} VdpVideoMixerGetAttributeValues");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    TRACE("{full} VdpVideoMixerDestroy mixer=%d", mixer);
    void *data = handlestorage_get(mixer, HANDLETYPE_VIDEO_MIXER);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(mixer);

    return VDP_STATUS_OK;
}

static
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
    TRACE1("{part} VdpVideoMixerRender");

#ifndef NDEBUG
    VdpRect const *rect;
    printf("      mixer=%d, background_surface=%d,", mixer, background_surface);
    printf(" background_source_rect=");

    rect = background_source_rect;
    if (NULL == rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", rect->x0, rect->y0, rect->x1, rect->y1);

    printf(",\n      current_picture_structure=%s,\n",
        reverser_video_mixer_picture_structure(current_picture_structure));
    printf("     ");
    if (video_surface_past_count > 0) {
        printf(" video_surface_past=[");
        for (uint32_t k = 0; k < video_surface_past_count; k ++) {
            if (0 != k) printf(",");
            printf("%d", video_surface_past[k]);
        }
        printf("],");
    } else {
        printf(" no_video_surface_past,");
    }
    printf(" video_surface_current=%d,", video_surface_current);
    if (video_surface_future_count > 0) {
        printf(" video_surface_future=[");
        for (uint32_t k = 0; k < video_surface_future_count; k ++) {
            if (0 != k) printf(",");
            printf("%d", video_surface_future[k]);
        }
        printf("],");
    } else {
        printf(" no_video_surface_future,");
    }

    printf("\n      video_source_rect=");
    rect = video_source_rect;
    if (NULL == rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", rect->x0, rect->y0, rect->x1, rect->y1);

    printf(", destination_surface=%d,", destination_surface);

    printf("\n      destination_rect=");
    rect = destination_rect;
    if (NULL == rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", rect->x0, rect->y0, rect->x1, rect->y1);

    printf(", video_surface_current=%d,", video_surface_current);

    if (layer_count > 0) {
        printf(" layers=[");
        for (uint32_t k = 0; k < layer_count; k ++) {
            if (0 != k) printf(",");
            printf("{%d,", layers[k].source_surface);
            rect = layers[k].source_rect;
            if (NULL == rect) printf("src:NULL,");
            else printf("src:[%d,%d,%d,%d],", rect->x0, rect->y0, rect->x1, rect->y1);
            rect = layers[k].destination_rect;
            if (NULL == rect) printf("dst:NULL,");
            else printf("dst:[%d,%d,%d,%d],", rect->x0, rect->y0, rect->x1, rect->y1);
            printf("}");
        }
        printf("]");
    } else {
        printf(" no_layers");
    }

    printf("\n");
#endif

    //TODO: handle rectangles

    VdpVideoSurfaceData *source_surface =
        handlestorage_get(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == source_surface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *dest_surface =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dest_surface)
        return VDP_STATUS_INVALID_HANDLE;

    struct SwsContext *sws_ctx =
        sws_getContext(source_surface->width, source_surface->height, PIX_FMT_YUV420P,
            cairo_image_surface_get_width(dest_surface->cairo_surface),
            cairo_image_surface_get_height(dest_surface->cairo_surface),
            PIX_FMT_RGBA, SWS_POINT, NULL, NULL, NULL);

    uint8_t const * const src_planes[] =
        { source_surface->y_plane, source_surface->v_plane, source_surface->u_plane, NULL };
    int src_strides[] =
        {source_surface->stride, source_surface->width/2, source_surface->width/2, 0};
    uint8_t *dst_planes[] = {NULL, NULL, NULL, NULL};
    dst_planes[0] = cairo_image_surface_get_data(dest_surface->cairo_surface);
    int dst_strides[] = {0, 0, 0, 0};
    dst_strides[0] = cairo_image_surface_get_stride(dest_surface->cairo_surface);
    cairo_surface_flush(dest_surface->cairo_surface);
    int res = sws_scale(sws_ctx,
                        src_planes, src_strides, 0, source_surface->height,
                        dst_planes, dst_strides);
    cairo_surface_mark_dirty(dest_surface->cairo_surface);
    sws_freeContext(sws_ctx);
    // printf ("res = %d, while height = %d\n", res, source_surface->height);
    if (res != source_surface->height)
        return VDP_STATUS_ERROR;

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    TRACE("{full} VdpPresentationQueueTargetDestroy presentation_queue_target=%d",
        presentation_queue_target);
    void *data = handlestorage_get(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(presentation_queue_target);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueCreate(VdpDevice device,
                               VdpPresentationQueueTarget presentation_queue_target,
                               VdpPresentationQueue *presentation_queue)
{
    TRACE("{part} VdpPresentationQueueCreate device=%d, presentation_queue_target=%d",
        device, presentation_queue_target);
    VdpPresentationQueueData *data =
        (VdpPresentationQueueData *)calloc(1, sizeof(VdpPresentationQueueData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;
    if (!handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;
    if (!handlestorage_valid(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET))
        return VDP_STATUS_INVALID_HANDLE;

    data->type = HANDLETYPE_PRESENTATION_QUEUE;
    data->device = device;
    data->presentation_queue_target = presentation_queue_target;

    *presentation_queue = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    TRACE("{full} VdpPresentationQueueDestroy presentation_queue=%d", presentation_queue);
    void *data = handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(presentation_queue);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *const background_color)
{
    TRACE1("{zilch} VdpPresentationQueueSetBackgroundColor");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *background_color)
{
    TRACE1("{zilch} VdpPresentationQueueGetBackgroundColor");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                                VdpTime *current_time)
{
    TRACE("{full} VdpPresentationQueueGetTime presentation_queue=%d", presentation_queue);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *current_time = (uint64_t)tv.tv_sec * 1000000000LL + (uint64_t)tv.tv_usec * 1000LL;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                uint32_t clip_width, uint32_t clip_height,
                                VdpTime earliest_presentation_time)
{
    TRACE("{part} VdpPresentationQueueDisplay presentation_queue=%d, surface=%d, "
        "clip_width=%d, clip_height=%d", presentation_queue, surface, clip_width, clip_height);

    VdpOutputSurfaceData *surfaceData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfaceData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueData *presentationQueueData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);

    if (NULL == presentationQueueData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *presentationQueueTargetData =
        handlestorage_get(presentationQueueData->presentation_queue_target,
        HANDLETYPE_PRESENTATION_QUEUE_TARGET);

    if (NULL == presentationQueueTargetData) return VDP_STATUS_INVALID_HANDLE;

    Drawable drawable = presentationQueueTargetData->drawable;

    VdpDeviceData *deviceData =
        handlestorage_get(presentationQueueTargetData->device, HANDLETYPE_DEVICE);

    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    Display *display = deviceData->display;
    int screen = deviceData->screen;

    int out_width = cairo_image_surface_get_width(surfaceData->cairo_surface);
    int out_height = cairo_image_surface_get_height(surfaceData->cairo_surface);

/*
    char *buf = (char*)cairo_image_surface_get_data(surfaceData->cairo_surface);
    XImage *image = XCreateImage(display, DefaultVisual(display, screen), 24, ZPixmap, 0,
        buf,
        out_width, out_height, 32,
        cairo_image_surface_get_stride(surfaceData->cairo_surface));

    if (NULL == image) {
        TRACE1("image become NULL in VdpPresentationQueueDisplay");
        return VDP_STATUS_RESOURCES;
    }

    XPutImage(display, drawable, DefaultGC(display, screen), image, 0, 0, 0, 0,
        out_width, out_height);

    free(image);
*/

    XShmSegmentInfo shminfo;
    XImage *image = XShmCreateImage(display, DefaultVisual(display, screen), 24, ZPixmap, NULL,
        &shminfo, out_width, out_height);
    if (NULL == image) {
        TRACE1("Error creating XImage in SHM");
        return VDP_STATUS_RESOURCES;
    }

    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT | 0777);
    if (shminfo.shmid < 0) {
        TRACE1("shm error");
        return VDP_STATUS_RESOURCES;
    }

    shminfo.shmaddr = shmat(shminfo.shmid, 0, 0);
    image->data = shminfo.shmaddr;

    shminfo.readOnly = False;
    // ErrorFlag = 0;
    // XSetErrorHandler(HandleXError);
    XShmAttach(display, &shminfo);
    XSync(display, False);

    shmctl(shminfo.shmid, IPC_RMID, 0);

    memcpy(image->data, cairo_image_surface_get_data(surfaceData->cairo_surface),
        cairo_image_surface_get_stride(surfaceData->cairo_surface) *
        cairo_image_surface_get_height(surfaceData->cairo_surface));

    XShmPutImage(display, drawable, DefaultGC(display, screen), image,
                 0, 0, 0, 0, out_width, out_height, False );

    XShmDetach(display, &shminfo);
    free(image);
    shmdt(shminfo.shmaddr);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                              VdpOutputSurface surface,
                                              VdpTime *first_presentation_time)

{
    TRACE("{full} VdpPresentationQueueBlockUntilSurfaceIdle presentation_queue=%d, surface=%d",
        presentation_queue, surface);

    if (! handlestorage_valid(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE))
        return VDP_STATUS_INVALID_HANDLE;

    if (! handlestorage_valid(surface, HANDLETYPE_OUTPUT_SURFACE))
        return VDP_STATUS_INVALID_HANDLE;

    // use current time as presentation time
    softVdpPresentationQueueGetTime(presentation_queue, first_presentation_time);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                           VdpOutputSurface surface,
                                           VdpPresentationQueueStatus *status,
                                           VdpTime *first_presentation_time)
{
    *status = VDP_PRESENTATION_QUEUE_STATUS_VISIBLE;
    TRACE1("{part} VdpPresentationQueueQuerySurfaceStatus");
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpBool *is_supported, uint32_t *max_width,
                                     uint32_t *max_height)
{
    TRACE1("{zilch} VdpVideoSurfaceQueryCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpChromaType surface_chroma_type,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                          uint32_t height, VdpVideoSurface *surface)
{
    TRACE("{part} VdpVideoSurfaceCreate, device=%d, chroma_type=%s, width=%d, height=%d",
        device, reverse_chroma_type(chroma_type), width, height);

    if (! handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoSurfaceData *data = (VdpVideoSurfaceData *)calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    uint32_t const stride = (width % 4 == 0) ? width : (width & ~0x3UL) + 4;

    data->type = HANDLETYPE_VIDEO_SURFACE;
    data->device = device;
    data->chroma_type = chroma_type;
    data->width = width;
    data->stride = stride;
    data->height = height;
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

    *surface = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    TRACE("{full} VdpVideoSurfaceDestroy surface=%d", surface);

    VdpVideoSurfaceData *data = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data->y_plane);
    free(data->v_plane);
    free(data->u_plane);
    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                 uint32_t *width, uint32_t *height)
{
    TRACE1("{zilch} VdpVideoSurfaceGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    TRACE1("{zilch} VdpVideoSurfaceGetBitsYCbCr");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                void const *const *source_data, uint32_t const *source_pitches)
{
    TRACE("{part} VdpVideoSurfacePutBitsYCbCr surface=%d, source_ycbcr_format=%s",
        surface, reverse_ycbcr_format(source_ycbcr_format));

    //TODO: figure out what to do with other formats
    if (VDP_YCBCR_FORMAT_YV12 != source_ycbcr_format)
        return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;

    VdpVideoSurfaceData *surfaceData = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == surfaceData)
        return VDP_STATUS_INVALID_HANDLE;

    uint8_t const *src;
    uint8_t *dst;
    dst = surfaceData->y_plane;     src = source_data[0];
    for (uint32_t k = 0; k < surfaceData->height; k ++) {
        memcpy(dst, src, surfaceData->width);
        dst += surfaceData->stride;
        src += source_pitches[0];
    }

    dst = surfaceData->v_plane;     src = source_data[1];
    for (uint32_t k = 0; k < surfaceData->height / 2; k ++) {
        memcpy(dst, src, surfaceData->width / 2);
        dst += surfaceData->width / 2;
        src += source_pitches[1];
    }

    dst = surfaceData->u_plane;     src = source_data[2];
    for (uint32_t k = 0; k < surfaceData->height / 2; k ++) {
        memcpy(dst, src, surfaceData->width/2);
        dst += surfaceData->width / 2;
        src += source_pitches[2];
    }

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    TRACE("{part} VdpBitmapSurfaceQueryCapabilities device=%d", device);
    if (! handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    printf("      format %s\n", reverse_rgba_format(surface_rgba_format));

    *is_supported = 1;
    *max_width = 2048;
    *max_height = 2048;

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    TRACE("{full} VdpBitmapSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d,"
        "frequently_accessed=%d", device, reverse_rgba_format(rgba_format), width, height,
        frequently_accessed);
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpBitmapSurfaceData *data = (VdpBitmapSurfaceData *)calloc(1, sizeof(VdpBitmapSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    //TODO: other format handling
    if (rgba_format != VDP_RGBA_FORMAT_B8G8R8A8)
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    data->cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (CAIRO_STATUS_SUCCESS != cairo_surface_status(data->cairo_surface)) {
        cairo_surface_destroy(data->cairo_surface);
        free(data);
        return VDP_STATUS_RESOURCES;
    }

    data->type = HANDLETYPE_BITMAP_SURFACE;
    data->device = device;
    data->rgba_format = rgba_format;

    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    TRACE("{full} VdpBitmapSurfaceDestroy surface=%d", surface);
    VdpBitmapSurfaceData *data = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    cairo_surface_destroy(data->cairo_surface);
    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    TRACE1("{zilch} VdpBitmapSurfaceGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    TRACE("{full} VdpBitmapSurfacePutBitsNative surface=%d", surface);
#ifndef NDEBUG
    printf("      destination_rect=");
    if (NULL == destination_rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", destination_rect->x0, destination_rect->y0,
        destination_rect->x1, destination_rect->y1);
    printf("\n");
#endif

    VdpBitmapSurfaceData *surfaceData = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == surfaceData)
        return VDP_STATUS_INVALID_HANDLE;

    //TODO: fix handling other formats
    if (VDP_RGBA_FORMAT_B8G8R8A8 != surfaceData->rgba_format)
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    VdpRect rect = {0, 0, 0, 0};
    if (destination_rect) {
        rect = *destination_rect;
    } else {
        rect.x1 = cairo_image_surface_get_width(surfaceData->cairo_surface);
        rect.y1 = cairo_image_surface_get_height(surfaceData->cairo_surface);
    }

    cairo_surface_t *src_surf =
        cairo_image_surface_create_for_data((unsigned char *)(source_data[0]),
        CAIRO_FORMAT_ARGB32, rect.x1-rect.x0, rect.y1-rect.y0, source_pitches[0]);
    if (CAIRO_STATUS_INVALID_STRIDE == cairo_surface_status(src_surf)) {
        printf("CAIRO_STATUS_INVALID_STRIDE\n");
        cairo_surface_destroy(src_surf);
        return VDP_STATUS_INVALID_VALUE;
    }

    cairo_t *cr = cairo_create(surfaceData->cairo_surface);
    cairo_set_source_surface(cr, src_surf, rect.x0, rect.y0);
    cairo_rectangle(cr, rect.x0, rect.y0, rect.x1 - rect.x0, rect.y1 - rect.y0);
    cairo_paint(cr);
    cairo_destroy(cr);

    cairo_surface_destroy(src_surf);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpDeviceDestroy(VdpDevice device)
{
    TRACE("{full} VdpDeviceDestroy device=%d", device);
    void *data = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(device);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpGetInformationString(char const **information_string)
{
    TRACE1("{full} VdpGetInformationString");
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    TRACE("{part} VdpGenerateCSCMatrix standard=%d", standard);
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
VdpStatus
softVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpOutputSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    TRACE("{dirty impl} VdpOutputSurfaceRenderOutputSurface destination_surface=%d, source_surface=%d",
        destination_surface, source_surface);
#ifndef NDEBUG
    printf("      destination_rect=");
    if (NULL == destination_rect) printf("NULL");
    else printf("(%d,%d,%d,%d)",    destination_rect->x0, destination_rect->y0,
                                    destination_rect->x1, destination_rect->y1);

    printf(", source_rect=");
    if (NULL == source_rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", source_rect->x0, source_rect->y0, source_rect->x1, source_rect->y1);
    printf("\n      colors=%p, flags=%d", colors, flags);
    printf( "\n      blend_state.blend_factor_source_color=%s"
            "\n      blend_state.blend_factor_destination_color=%s"
            "\n      blend_state.blend_factor_source_alpha=%s"
            "\n      blend_state.blend_factor_destination_color=%s"
            "\n      blend_state.blend_equation_color=%s"
            "\n      blend_state.blend_equation_alpha=%s"
            "\n      blend_constant = (%11f, %11f, %11f, %11f)",
            reverse_blend_factor(blend_state->blend_factor_source_color),
            reverse_blend_factor(blend_state->blend_factor_destination_color),
            reverse_blend_factor(blend_state->blend_factor_source_alpha),
            reverse_blend_factor(blend_state->blend_factor_destination_alpha),
            reverse_blend_equation(blend_state->blend_equation_color),
            reverse_blend_equation(blend_state->blend_equation_alpha),
            blend_state->blend_constant.red, blend_state->blend_constant.green,
            blend_state->blend_constant.blue, blend_state->blend_constant.alpha
    );
    printf("\n");
#endif

    if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    //TODO: stop doing dumb things and use swscale instead
    VdpOutputSurfaceData *dstSurface =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *srcSurface =
        handlestorage_get(source_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpRect s_rect = {0, 0, 0, 0};
    VdpRect d_rect = {0, 0, 0, 0};

    if (source_rect) {
        s_rect = *source_rect;
    } else {
        s_rect.x1 = cairo_image_surface_get_width(srcSurface->cairo_surface);
        s_rect.y1 = cairo_image_surface_get_height(srcSurface->cairo_surface);
    }

    if (destination_rect) {
        d_rect = *destination_rect;
    } else {
        d_rect.x1 = cairo_image_surface_get_width(dstSurface->cairo_surface);
        d_rect.y1 = cairo_image_surface_get_height(dstSurface->cairo_surface);
    }

    if (s_rect.x1 - s_rect.x0 == d_rect.x1 - d_rect.x0 &&
        s_rect.y1 - s_rect.y0 == d_rect.y1 - d_rect.y0)
    {
        // trivial case -- destination and source rectangles have the same width and height
        cairo_t *cr = cairo_create(dstSurface->cairo_surface);
        cairo_set_source_surface(cr, srcSurface->cairo_surface,
            d_rect.x0 - s_rect.x0, d_rect.y0 - s_rect.y0);
        cairo_rectangle(cr, d_rect.x0, d_rect.y0, d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0);
        cairo_paint(cr);
        cairo_destroy(cr);
    } else {
        // Scaling needed. First, scale image.
        cairo_surface_t *scaled_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                            d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0);
        cairo_surface_flush(scaled_surface);
        struct SwsContext *sws_ctx =
            sws_getContext(s_rect.x1 - s_rect.x0, s_rect.y1 - s_rect.y0, PIX_FMT_RGBA,
                d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0,
                PIX_FMT_RGBA, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        cairo_surface_flush(srcSurface->cairo_surface);
        uint8_t const * const src_planes[] =
            {cairo_image_surface_get_data(srcSurface->cairo_surface), NULL, NULL, NULL };
        int src_strides[] =
            {cairo_image_surface_get_stride(srcSurface->cairo_surface), 0, 0, 0};
        uint8_t *dst_planes[] = {cairo_image_surface_get_data(scaled_surface), NULL, NULL, NULL};
        int dst_strides[] = {cairo_image_surface_get_stride(scaled_surface), 0, 0, 0};
        int res = sws_scale(sws_ctx,
                            src_planes, src_strides, 0,  s_rect.y1 - s_rect.y0,
                            dst_planes, dst_strides);
        cairo_surface_mark_dirty(scaled_surface);
        sws_freeContext(sws_ctx);

        // then do drawing
        cairo_t *cr = cairo_create(dstSurface->cairo_surface);
        cairo_set_source_surface(cr, scaled_surface, 0, 0);
        cairo_rectangle(cr, d_rect.x0, d_rect.y0, d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0);
        cairo_paint(cr);
        cairo_destroy(cr);

        cairo_surface_destroy(scaled_surface);
    }

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpBitmapSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    TRACE("{part} VdpOutputSurfaceRenderBitmapSurface destination_surface=%d, source_surface=%d",
        destination_surface, source_surface);
#ifndef NDEBUG
    printf("      destination_rect=");
    if (NULL == destination_rect) printf("NULL");
    else printf("(%d,%d,%d,%d)",    destination_rect->x0, destination_rect->y0,
                                    destination_rect->x1, destination_rect->y1);

    printf(", source_rect=");
    if (NULL == source_rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", source_rect->x0, source_rect->y0, source_rect->x1, source_rect->y1);
    printf("\n      colors=%p, flags=%d", colors, flags);
    printf( "\n      blend_state.blend_factor_source_color=%s"
            "\n      blend_state.blend_factor_destination_color=%s"
            "\n      blend_state.blend_factor_source_alpha=%s"
            "\n      blend_state.blend_factor_destination_color=%s"
            "\n      blend_state.blend_equation_color=%s"
            "\n      blend_state.blend_equation_alpha=%s"
            "\n      blend_constant = (%11f, %11f, %11f, %11f)",
            reverse_blend_factor(blend_state->blend_factor_source_color),
            reverse_blend_factor(blend_state->blend_factor_destination_color),
            reverse_blend_factor(blend_state->blend_factor_source_alpha),
            reverse_blend_factor(blend_state->blend_factor_destination_alpha),
            reverse_blend_equation(blend_state->blend_equation_color),
            reverse_blend_equation(blend_state->blend_equation_alpha),
            blend_state->blend_constant.red, blend_state->blend_constant.green,
            blend_state->blend_constant.blue, blend_state->blend_constant.alpha
    );
    printf("\n");
#endif

    if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    VdpOutputSurfaceData *dstSurface =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpBitmapSurfaceData *srcSurface =
        handlestorage_get(source_surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == srcSurface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpRect s_rect = {0, 0, 0, 0};
    VdpRect d_rect = {0, 0, 0, 0};

    if (source_rect) {
        s_rect = *source_rect;
    } else {
        s_rect.x1 = cairo_image_surface_get_width(srcSurface->cairo_surface);
        s_rect.y1 = cairo_image_surface_get_height(srcSurface->cairo_surface);
    }

    if (destination_rect) {
        d_rect = *destination_rect;
    } else {
        d_rect.x1 = cairo_image_surface_get_width(dstSurface->cairo_surface);
        d_rect.y1 = cairo_image_surface_get_height(dstSurface->cairo_surface);
    }

    cairo_t *cr = cairo_create(dstSurface->cairo_surface);
    cairo_set_source_surface(cr, srcSurface->cairo_surface,
        d_rect.x0 - s_rect.x0, d_rect.y0 - s_rect.y0);
    cairo_rectangle(cr, d_rect.x0, d_rect.y0, d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0);
    cairo_paint(cr);
    cairo_destroy(cr);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    TRACE("{zilch} VdpPreemptionCallbackRegister callback=%p", callback);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                        VdpPresentationQueueTarget *target)
{
    TRACE("{part} VdpPresentationQueueTargetCreateX11, device=%d, drawable=%u", device,
        ((unsigned int)drawable));

    VdpPresentationQueueTargetData *data =
        (VdpPresentationQueueTargetData *)malloc(sizeof(VdpPresentationQueueTargetData));
    if (NULL == data)
        return VDP_STATUS_ERROR;

    if (!handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    data->type = HANDLETYPE_PRESENTATION_QUEUE_TARGET;
    data->device = device;
    data->drawable = drawable;

    *target = handlestorage_add(data);

    return VDP_STATUS_OK;
}

// =========================
static
VdpStatus
softVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    TRACE("{full} VdpGetProcAddress, device=%d, function_id=%s", device, reverse_func_id(function_id));
    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = &softVdpGetErrorString;
        break;
    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = &softVdpGetProcAddress;
        break;
    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = &softVdpGetApiVersion;
        break;
    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = &softVdpGetInformationString;
        break;
    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = &softVdpDeviceDestroy;
        break;
    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = &softVdpGenerateCSCMatrix;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &softVdpVideoSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = &softVdpVideoSurfaceCreate;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = &softVdpVideoSurfaceDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = &softVdpVideoSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = &softVdpVideoSurfaceGetBitsYCbCr;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &softVdpVideoSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &softVdpOutputSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = &softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = &softVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = &softVdpOutputSurfaceCreate;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = &softVdpOutputSurfaceDestroy;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = &softVdpOutputSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = &softVdpOutputSurfaceGetBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &softVdpOutputSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = &softVdpOutputSurfacePutBitsIndexed;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &softVdpOutputSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &softVdpBitmapSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = &softVdpBitmapSurfaceCreate;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = &softVdpBitmapSurfaceDestroy;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = &softVdpBitmapSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &softVdpBitmapSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = &softVdpOutputSurfaceRenderOutputSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = &softVdpOutputSurfaceRenderBitmapSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // *function_pointer = &softVdpOutputSurfaceRenderVideoSurfaceLuma;
        *function_pointer = NULL;
        break;
    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = &softVdpDecoderQueryCapabilities;
        break;
    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = &softVdpDecoderCreate;
        break;
    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = &softVdpDecoderDestroy;
        break;
    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = &softVdpDecoderGetParameters;
        break;
    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = &softVdpDecoderRender;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = &softVdpVideoMixerQueryFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = &softVdpVideoMixerQueryParameterSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = &softVdpVideoMixerQueryAttributeSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = &softVdpVideoMixerQueryParameterValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = &softVdpVideoMixerQueryAttributeValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = &softVdpVideoMixerCreate;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = &softVdpVideoMixerSetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = &softVdpVideoMixerSetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = &softVdpVideoMixerGetFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = &softVdpVideoMixerGetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = &softVdpVideoMixerGetParameterValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = &softVdpVideoMixerGetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = &softVdpVideoMixerDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = &softVdpVideoMixerRender;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = &softVdpPresentationQueueTargetDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = &softVdpPresentationQueueCreate;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = &softVdpPresentationQueueDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = &softVdpPresentationQueueSetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = &softVdpPresentationQueueGetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = &softVdpPresentationQueueGetTime;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = &softVdpPresentationQueueDisplay;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = &softVdpPresentationQueueBlockUntilSurfaceIdle;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = &softVdpPresentationQueueQuerySurfaceStatus;
        break;
    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = &softVdpPreemptionCallbackRegister;
        break;
    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = &softVdpPresentationQueueTargetCreateX11;
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
softVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address)
{
    TRACE("{full} vdp_imp_device_create_x11 display=%p, screen=%d", display, screen);
    if (NULL == display)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *data = (VdpDeviceData *)malloc(sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_DEVICE;
    data->display = display;
    data->screen = screen;

    *device = handlestorage_add(data);
    *get_proc_address = &softVdpGetProcAddress;

    return VDP_STATUS_OK;
}
