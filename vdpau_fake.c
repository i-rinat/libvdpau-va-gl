#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <stdio.h>
#include <assert.h>
#include <glib.h>
#include <libswscale/swscale.h>
#include <stdlib.h>
#include <string.h>
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

char const *implemetation_description_string = "VAAPI/software backend for VDPAU";

static
uint32_t
rgba_format_storage_size(VdpRGBAFormat rgba_format)
{
    switch (rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
    case VDP_RGBA_FORMAT_R8G8B8A8:
    case VDP_RGBA_FORMAT_R10G10B10A2:
    case VDP_RGBA_FORMAT_B10G10R10A2:
        return 4;
    case VDP_RGBA_FORMAT_A8:
        return 1;
    default:
        return 4;
    }
}

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
    TRACE1("{full} VdpGetApiVersion");
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                                uint32_t *max_level, uint32_t *max_macroblocks,
                                uint32_t *max_width, uint32_t *max_height)
{
    TRACE1("{zilch} VdpDecoderQueryCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                     uint32_t max_references, VdpDecoder *decoder)
{
    TRACE1("{zilch} VdpDecoderCreate");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderDestroy(VdpDecoder decoder)
{
    TRACE1("{zilch} VdpDecoderDestroy");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                            uint32_t *width, uint32_t *height)
{
    TRACE1("{zilch} VdpDecoderGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                     VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                     VdpBitstreamBuffer const *bitstream_buffers)
{
    TRACE1("{zilch} VdpDecoderRender");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpBool *is_supported)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryGetPutBitsNativeCapabilities");
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
    TRACE1("{zilch} VdpOutputSurfaceQueryPutBitsIndexedCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    TRACE1("{zilch} VdpOutputSurfaceQueryPutBitsYCbCrCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpOutputSurface *surface)
{
    TRACE("{part} VdpOutputSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d",
        device, reverse_rgba_format(rgba_format), width, height);
    if (! handlestorage_valid(device, HANDLE_TYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    if (width > 4096 || height > 4096)
        return VDP_STATUS_INVALID_SIZE;

    VdpOutputSurfaceData *data = (VdpOutputSurfaceData *)calloc(1, sizeof(VdpOutputSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    uint32_t const stride = (width % 4 == 0) ? width : (width & 0x3) + 4;

    data->type = HANDLE_TYPE_OUTPUT_SURFACE;
    data->device = device;
    data->rgba_format = rgba_format;
    data->width = width;
    data->stride = stride;
    data->height = height;
    data->buf = malloc(stride * height * rgba_format_storage_size(rgba_format));

    if (NULL == data->buf) {
        free(data);
        return VDP_STATUS_RESOURCES;
    }

    *surface = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    TRACE("{full} VdpOutputSurfaceDestroy surface=%d", surface);
    VdpOutputSurfaceData *data = handlestorage_get(surface, HANDLE_TYPE_OUTPUT_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data->buf);
    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height)
{
    TRACE1("{zilch} VdpOutputSurfaceGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    TRACE1("{zilch} VdpOutputSurfaceGetBitsNative");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    TRACE1("{zilch} VdpVideoMixerQueryParameterSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
                                   void const *const *source_data, uint32_t const *source_pitch,
                                   VdpRect const *destination_rect,
                                   VdpColorTableFormat color_table_format, void const *color_table)
{
    TRACE1("{zilch} VdpOutputSurfacePutBitsIndexed");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                 void const *const *source_data, uint32_t const *source_pitches,
                                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    TRACE1("{zilch} VdpOutputSurfacePutBitsYCbCr");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                     VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoMixerQueryFeatureSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                       VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoMixerQueryParameterSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                       VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoMixerQueryAttributeSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                          void *min_value, void *max_value)
{
    TRACE1("{zilch} VdpVideoMixerQueryParameterValueRange");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                          void *min_value, void *max_value)
{
    TRACE1("{zilch} VdpVideoMixerQueryAttributeValueRange");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                        VdpVideoMixerFeature const *features, uint32_t parameter_count,
                        VdpVideoMixerParameter const *parameters,
                        void const *const *parameter_values, VdpVideoMixer *mixer)
{
    TRACE("{part} VdpVideoMixerCreate feature_count=%d, parameter_count=%d",
        feature_count, parameter_count);

    if (!handlestorage_valid(device, HANDLE_TYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoMixerData *data = (VdpVideoMixerData *)calloc(1, sizeof(VdpVideoMixerData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLE_TYPE_VIDEO_MIXER;
    data->device = device;

    *mixer = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
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
fakeVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void const *const *attribute_values)
{
    TRACE("{part} VdpVideoMixerSetAttributeValues mixer=%d, attribute_count=%d",
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

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    TRACE1("{zilch} VdpVideoMixerGetFeatureSupport");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    TRACE1("{zilch} VdpVideoMixerGetFeatureEnables");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                    VdpVideoMixerParameter const *parameters,
                                    void *const *parameter_values)
{
    TRACE1("{zilch} VdpVideoMixerGetParameterValues");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void *const *attribute_values)
{
    TRACE1("{zilch} VdpVideoMixerGetAttributeValues");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    TRACE("{full} VdpVideoMixerDestroy mixer=%d", mixer);
    void *data = handlestorage_get(mixer, HANDLE_TYPE_VIDEO_MIXER);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(mixer);

    return VDP_STATUS_OK;
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

    VdpVideoSurfaceData *source_surface =
        handlestorage_get(video_surface_current, HANDLE_TYPE_VIDEO_SURFACE);
    if (NULL == source_surface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *dest_surface =
        handlestorage_get(destination_surface, HANDLE_TYPE_OUTPUT_SURFACE);
    if (NULL == dest_surface)
        return VDP_STATUS_INVALID_HANDLE;

    struct SwsContext *sws_ctx =
        sws_getContext(source_surface->width, source_surface->height, PIX_FMT_YUV420P,
            dest_surface->width, dest_surface->height, PIX_FMT_RGBA,
            SWS_POINT, NULL, NULL, NULL);

    uint8_t const * const src_planes[] =
        { source_surface->y_plane, source_surface->v_plane, source_surface->u_plane, NULL };
    int src_strides[] =
        {source_surface->stride, source_surface->width/2, source_surface->width/2, 0};
    uint8_t *dst_planes[] = {dest_surface->buf, NULL, NULL, NULL};
    int dst_strides[] = {dest_surface->stride * rgba_format_storage_size(dest_surface->rgba_format),
                         0, 0, 0};
    int res = sws_scale(sws_ctx,
                        src_planes, src_strides, 0, source_surface->height,
                        dst_planes, dst_strides);

    sws_freeContext(sws_ctx);

    // printf ("res = %d, while height = %d\n", res, source_surface->height);
    if (res != source_surface->height)
        return VDP_STATUS_ERROR;

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    TRACE("{full} VdpPresentationQueueTargetDestroy presentation_queue_target=%d",
        presentation_queue_target);
    void *data = handlestorage_get(presentation_queue_target, HANDLE_TYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(presentation_queue_target);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueCreate(VdpDevice device,
                               VdpPresentationQueueTarget presentation_queue_target,
                               VdpPresentationQueue *presentation_queue)
{
    TRACE("{part} VdpPresentationQueueCreate device=%d, presentation_queue_target=%d",
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
    data->presentation_queue_target = presentation_queue_target;

    *presentation_queue = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    TRACE("{full} VdpPresentationQueueDestroy presentation_queue=%d", presentation_queue);
    void *data = handlestorage_get(presentation_queue, HANDLE_TYPE_PRESENTATION_QUEUE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(presentation_queue);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *const background_color)
{
    TRACE1("{zilch} VdpPresentationQueueSetBackgroundColor");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *background_color)
{
    TRACE1("{zilch} VdpPresentationQueueGetBackgroundColor");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
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
fakeVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                uint32_t clip_width, uint32_t clip_height,
                                VdpTime earliest_presentation_time)
{
    TRACE("{part} VdpPresentationQueueDisplay presentation_queue=%d, surface=%d, "
        "clip_width=%d, clip_height=%d", presentation_queue, surface, clip_width, clip_height);

    VdpOutputSurfaceData *surfaceData = handlestorage_get(surface, HANDLE_TYPE_OUTPUT_SURFACE);
    if (NULL == surfaceData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueData *presentationQueueData =
        handlestorage_get(presentation_queue, HANDLE_TYPE_PRESENTATION_QUEUE);

    if (NULL == presentationQueueData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *presentationQueueTargetData =
        handlestorage_get(presentationQueueData->presentation_queue_target,
        HANDLE_TYPE_PRESENTATION_QUEUE_TARGET);

    if (NULL == presentationQueueTargetData) return VDP_STATUS_INVALID_HANDLE;

    Drawable drawable = presentationQueueTargetData->drawable;

    VdpDeviceData *deviceData =
        handlestorage_get(presentationQueueTargetData->device, HANDLE_TYPE_DEVICE);

    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    Display *display = deviceData->display;
    int screen = deviceData->screen;

    XImage *image = XCreateImage(display, DefaultVisual(display, screen), 24, ZPixmap, 0,
        (char *)(surfaceData->buf), surfaceData->width, surfaceData->height, 32,
        surfaceData->stride * rgba_format_storage_size(surfaceData->rgba_format));

    XPutImage(display, drawable, DefaultGC(display, screen), image, 0, 0, 0, 0,
        surfaceData->width, surfaceData->height);

    //void *dummy=malloc(4);  // as XDestroyImage frees data too, fool it
    //image->data = dummy;
    //XDestroyImage(image);

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                              VdpOutputSurface surface,
                                              VdpTime *first_presentation_time)

{
    TRACE("{full} VdpPresentationQueueBlockUntilSurfaceIdle presentation_queue=%d, surface=%d",
        presentation_queue, surface);

    if (! handlestorage_valid(presentation_queue, HANDLE_TYPE_PRESENTATION_QUEUE))
        return VDP_STATUS_INVALID_HANDLE;

    if (! handlestorage_valid(surface, HANDLE_TYPE_OUTPUT_SURFACE))
        return VDP_STATUS_INVALID_HANDLE;

    // use current time as presentation time
    fakeVdpPresentationQueueGetTime(presentation_queue, first_presentation_time);

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
    TRACE1("{part} VdpPresentationQueueQuerySurfaceStatus");
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpBool *is_supported, uint32_t *max_width,
                                     uint32_t *max_height)
{
    TRACE1("{zilch} VdpVideoSurfaceQueryCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpChromaType surface_chroma_type,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    TRACE1("{zilch} VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                          uint32_t height, VdpVideoSurface *surface)
{
    TRACE("{part} VdpVideoSurfaceCreate, device=%d, chroma_type=%s, width=%d, height=%d",
        device, reverse_chroma_type(chroma_type), width, height);

    if (! handlestorage_valid(device, HANDLE_TYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoSurfaceData *data = (VdpVideoSurfaceData *)calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    uint32_t const stride = (width % 4 == 0) ? width : (width & 0x3) + 4;

    data->type = HANDLE_TYPE_VIDEO_SURFACE;
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
fakeVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    TRACE("{full} VdpVideoSurfaceDestroy surface=%d", surface);

    VdpVideoSurfaceData *data = handlestorage_get(surface, HANDLE_TYPE_VIDEO_SURFACE);
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
fakeVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                 uint32_t *width, uint32_t *height)
{
    TRACE1("{zilch} VdpVideoSurfaceGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    TRACE1("{zilch} VdpVideoSurfaceGetBitsYCbCr");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                void const *const *source_data, uint32_t const *source_pitches)
{
    TRACE("{part} VdpVideoSurfacePutBitsYCbCr surface=%d, source_ycbcr_format=%s",
        surface, reverse_ycbcr_format(source_ycbcr_format));

    //TODO: figure out what to do with other formats
    if (VDP_YCBCR_FORMAT_YV12 != source_ycbcr_format)
        return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;

    VdpVideoSurfaceData *surfaceData = handlestorage_get(surface, HANDLE_TYPE_VIDEO_SURFACE);
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
fakeVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    TRACE("{part} VdpBitmapSurfaceQueryCapabilities device=%d", device);
    if (! handlestorage_valid(device, HANDLE_TYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

    printf("      format %s\n", reverse_rgba_format(surface_rgba_format));

    *is_supported = 1;
    *max_width = 2048;
    *max_height = 2048;

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    TRACE("{full} VdpBitmapSurfaceCreate device=%d, rgba_format=%s, width=%d, height=%d,"
        "frequently_accessed=%d", device, reverse_rgba_format(rgba_format), width, height,
        frequently_accessed);
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLE_TYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpBitmapSurfaceData *data = (VdpBitmapSurfaceData *)calloc(1, sizeof(VdpBitmapSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    uint32_t stride = (width % 4 == 0) ? width : (width & 3) + 4;
    void *buf = malloc(stride * height * rgba_format_storage_size(rgba_format));
    if (NULL == buf) {
        free(data);
        return VDP_STATUS_RESOURCES;
    }

    data->type = HANDLE_TYPE_BITMAP_SURFACE;
    data->device = device;
    data->rgba_format = rgba_format;
    data->width = width;
    data->height = height;
    data->stride = stride;
    data->buf = buf;

    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    TRACE("{full} VdpBitmapSurfaceDestroy surface=%d", surface);
    VdpBitmapSurfaceData *data = handlestorage_get(surface, HANDLE_TYPE_BITMAP_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data->buf);
    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    TRACE1("{zilch} VdpBitmapSurfaceGetParameters");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    TRACE("{WIP} VdpBitmapSurfacePutBitsNative surface=%d", surface);
#ifndef NDEBUG
    printf("      destination_rect=");
    if (NULL == destination_rect) printf("NULL");
    else printf("(%d,%d,%d,%d)", destination_rect->x0, destination_rect->y0,
        destination_rect->x1, destination_rect->y1);
    printf("\n");
#endif

    VdpBitmapSurfaceData *surfaceData = handlestorage_get(surface, HANDLE_TYPE_BITMAP_SURFACE);
    if (NULL == surfaceData)
        return VDP_STATUS_INVALID_HANDLE;

    //TODO: fix handling other formats
    if (VDP_RGBA_FORMAT_B8G8R8A8 != surfaceData->rgba_format)
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    VdpRect rect = {0, 0, surfaceData->width-1, surfaceData->height-1};
    if (NULL != destination_rect) rect = *destination_rect;

    for (uint32_t line = rect.y0; line <= rect.y1; line ++) {
        uint8_t *dst = (uint8_t *)surfaceData->buf + rect.y0 * surfaceData->stride * 4;
        uint8_t *src = (uint8_t *)(source_data[0]) + (line - rect.y0) * source_pitches[0];
        memcpy(dst, src, 4*(rect.x1 - rect.x0 + 1));
    }

    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpDeviceDestroy(VdpDevice device)
{
    TRACE("{full} VdpDeviceDestroy device=%d", device);
    void *data = handlestorage_get(device, HANDLE_TYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    free(data);
    handlestorage_expunge(device);
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpGetInformationString(char const **information_string)
{
    TRACE1("{full} VdpGetInformationString");
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

static
VdpStatus
fakeVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    TRACE1("{zilch} VdpGenerateCSCMatrix");
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
    TRACE1("{zilch} VdpOutputSurfaceRenderOutputSurface");
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
    TRACE1("{zilch} VdpOutputSurfaceRenderBitmapSurface");
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    TRACE("{zilch} VdpPreemptionCallbackRegister callback=%p", callback);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
fakeVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                        VdpPresentationQueueTarget *target)
{
    TRACE("{part} VdpPresentationQueueTargetCreateX11, device=%d, drawable=%u", device,
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
    TRACE("{full} VdpGetProcAddress, device=%d, function_id=%s", device, reverse_func_id(function_id));
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
    TRACE("{full} vdp_imp_device_create_x11 display=%p, screen=%d", display, screen);
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
