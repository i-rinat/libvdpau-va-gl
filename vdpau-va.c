#include <stdlib.h>
#include <string.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <va/va.h>
#include <va/va_x11.h>
#include "reverse-constant.h"
#include "handle-storage.h"
#include "vdpau-trace.h"

#define failOnErrorWithRetval(fn_name, status, retval)    \
    if (VA_STATUS_SUCCESS != status) { \
        traceTrace(fn_name " failed with %d at %s:%d\n", status, __FILE__, __LINE__); \
        return retval; \
    }

static char const *implemetation_description_string = "VAAPI backend for VDPAU";

typedef struct {
    HandleType  type;
    Display     *display;
    VADisplay   va_dpy;
    int         screen;
} VdpDeviceData;

typedef struct {
    HandleType      type;
    VdpDeviceData   *device;
    Drawable        drawable;
} VdpPresentationQueueTargetData;

typedef struct {
    HandleType                      type;
    VdpDeviceData                   *device;
    VdpPresentationQueueTargetData  *target;
} VdpPresentationQueueData;

typedef struct {
    HandleType      type;
    VdpDeviceData  *device;
    VdpRGBAFormat   rgba_format;
    VASurfaceID     va_surf;
    VAImage         va_derived_image;
    VAImage         va_img;
    VASubpictureID  va_subpic;
    uint32_t        width;
    uint32_t        height;
} VdpOutputSurfaceData;

typedef struct {
    HandleType      type;
    VdpDeviceData  *device;
    VdpRGBAFormat   rgba_format;
    VAImage         va_img;
    uint32_t        width;
    uint32_t        height;
} VdpBitmapSurfaceData;

typedef struct {
    HandleType      type;
    VdpDeviceData  *device;
} VdpVideoMixerData;

typedef struct {
    HandleType      type;
    VdpDeviceData  *device;
    uint32_t        width;
    uint32_t        height;
    VdpChromaType   chroma_type;
    VASurfaceID     va_surf;
    VAImage         va_derived_image;
} VdpVideoSurfaceData;

// ===============

static
void
workaround_missing_data_size_of_derived_image(VAImage *va_img)
{
    switch (va_img->format.fourcc) {
    case VA_FOURCC_YV12:
        va_img->data_size = va_img->height * va_img->pitches[0]
                            + va_img->height/2 * (va_img->pitches[1] + va_img->pitches[2]);
        return;
    default:
        fprintf(stderr, "no workaround for %c%c%c%c\n", va_img->format.fourcc & 0xff,
            (va_img->format.fourcc >> 8) & 0xff, (va_img->format.fourcc >> 16) & 0xff,
            (va_img->format.fourcc >> 24) & 0xff);
        return;
    }
}

static
const char *
vaVdpGetErrorString(VdpStatus status)
{
    return reverse_status(status);
}

static
VdpStatus
vaVdpGetApiVersion(uint32_t *api_version)
{
    traceVdpGetApiVersion("{full}", api_version);
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                              uint32_t *max_level, uint32_t *max_macroblocks,
                              uint32_t *max_width, uint32_t *max_height)
{
    traceVdpDecoderQueryCapabilities("{zilch}", device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                   uint32_t max_references, VdpDecoder *decoder)
{
    traceVdpDecoderCreate("{zilch}", device, profile, width, height, max_references, decoder);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpDecoderDestroy(VdpDecoder decoder)
{
    traceVdpDecoderDestroy("{zilch}", decoder);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                          uint32_t *width, uint32_t *height)
{
    traceVdpDecoderGetParameters("{zilch}", decoder, profile, width, height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target, VdpPictureInfo const *picture_info,
                   uint32_t bitstream_buffer_count, VdpBitstreamBuffer const *bitstream_buffers)
{
    traceVdpDecoderRender("{zilch}", decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width,
                                    uint32_t *max_height)
{
    traceVdpOutputSurfaceQueryCapabilities("{zilch}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                    VdpRGBAFormat surface_rgba_format,
                                                    VdpBool *is_supported)
{
    traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities("{zilch}", device, surface_rgba_format,
        is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpIndexedFormat bits_indexed_format,
                                                  VdpColorTableFormat color_table_format,
                                                  VdpBool *is_supported)
{
    traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities("{zilch}", device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                                VdpYCbCrFormat bits_ycbcr_format,
                                                VdpBool *is_supported)
{
    traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities("{zilch}", device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VAImageFormat *
vaGetVAImageFormatForVdpRGBAFormat(VADisplay va_dpy, VdpRGBAFormat rgba_format)
{
    VAImageFormat *formats = calloc(sizeof(VAImageFormat), vaMaxNumSubpictureFormats(va_dpy));
    if (NULL == formats) return NULL;
    unsigned int num_formats;
    VAStatus status = vaQuerySubpictureFormats(va_dpy, formats, NULL, &num_formats);
    if (VA_STATUS_SUCCESS != status) {
        free(formats);
        return NULL;
    }

    VAImageFormat *fmt = NULL;
    for (unsigned int k = 0; k < num_formats; k ++) {
        // TODO: check for other formats
        if (VDP_RGBA_FORMAT_B8G8R8A8 == rgba_format) {
            if (formats[k].fourcc == VA_FOURCC('B','G','R','A')) {
                fmt = &formats[k];
                break;
            }
        }
    }

    VAImageFormat *retvalue = NULL;
    if (fmt) {
        retvalue = malloc(sizeof(VAImageFormat));
        *retvalue = *fmt;
    }
    free(formats);
    return retvalue;
}

static
VdpStatus
vaVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                         uint32_t height, VdpOutputSurface *surface)
{
    traceVdpOutputSurfaceCreate("{full}", device, rgba_format, width, height, surface);
    VAStatus status;
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *data = calloc(1, sizeof(VdpOutputSurfaceData));
    if (NULL == data) return VDP_STATUS_RESOURCES;

    status = vaCreateSurfaces(deviceData->va_dpy, width, height, VA_RT_FORMAT_YUV420, 1,
                              &data->va_surf);
    if (VA_STATUS_SUCCESS != status) {
        free(data);
        traceTrace("vaCreateSurfaces failed with code %d at %s:%d\n", status, __FILE__, __LINE__);
        return VDP_STATUS_ERROR;
    }

    // FIXME: I don't undestand why I must derive image from surface in order it to be displayed
    //        This seems like some kind of misunderstanding of library.
    status = vaDeriveImage(deviceData->va_dpy, data->va_surf, &data->va_derived_image);
    workaround_missing_data_size_of_derived_image(&data->va_derived_image);
    if (VA_STATUS_SUCCESS != status) {
        free(data);
        return VDP_STATUS_ERROR;
    }

    VAImageFormat *fmt = vaGetVAImageFormatForVdpRGBAFormat(deviceData->va_dpy, rgba_format);
    if (NULL == fmt) {
        free(data);
        return VDP_STATUS_INVALID_RGBA_FORMAT;
    }

    status = vaCreateImage(deviceData->va_dpy, fmt, width, height, &data->va_img);
    free(fmt);
    if (VA_STATUS_SUCCESS != status) {
        free(data);
        return VDP_STATUS_ERROR;
    }

    status = vaCreateSubpicture(deviceData->va_dpy, data->va_img.image_id, &data->va_subpic);
    if (VA_STATUS_SUCCESS != status) {
        free(data);
        return VDP_STATUS_ERROR;
    }

    data->type = HANDLETYPE_OUTPUT_SURFACE;
    data->device = deviceData;
    data->rgba_format = rgba_format;
    data->width = width;
    data->height = height;

    *surface = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    traceVdpOutputSurfaceDestroy("{full}", surface);
    VAStatus status;
    VdpOutputSurfaceData *data = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == data) return VDP_STATUS_INVALID_HANDLE;

    status = vaDeassociateSubpicture(data->device->va_dpy, data->va_subpic, &data->va_surf, 1);
    if (VA_STATUS_SUCCESS != status) return VDP_STATUS_ERROR;

    status = vaDestroySubpicture(data->device->va_dpy, data->va_subpic);
    if (VA_STATUS_SUCCESS != status) return VDP_STATUS_ERROR;

    status = vaDestroyImage(data->device->va_dpy, data->va_img.image_id);
    if (VA_STATUS_SUCCESS != status) return VDP_STATUS_ERROR;

    status = vaDestroyImage(data->device->va_dpy, data->va_derived_image.image_id);
    if (VA_STATUS_SUCCESS != status) return VDP_STATUS_ERROR;

    status = vaDestroySurfaces(data->device->va_dpy, &data->va_surf, 1);
    if (VA_STATUS_SUCCESS != status) return VDP_STATUS_ERROR;

    handlestorage_expunge(surface);
    free(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                uint32_t *width, uint32_t *height)
{
    traceVdpOutputSurfaceGetParameters("{zilch}", surface, rgba_format, width, height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    traceVdpOutputSurfaceGetBitsNative("{zilch}", surface, source_rect, destination_data,
        destination_pitches);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    traceVdpOutputSurfacePutBitsNative("{zilch}", surface, source_data, source_pitches,
        destination_rect);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
                                 void const *const *source_data, uint32_t const *source_pitch,
                                 VdpRect const *destination_rect,
                                 VdpColorTableFormat color_table_format, void const *color_table)
{
    traceVdpOutputSurfacePutBitsIndexed("{zilch}", surface, source_indexed_format, source_data,
        source_pitch, destination_rect, color_table_format, color_table);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                               void const *const *source_data, uint32_t const *source_pitches,
                               VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    traceVdpOutputSurfacePutBitsYCbCr("{zilch}", surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                   VdpBool *is_supported)
{
    traceVdpVideoMixerQueryFeatureSupport("{zilch}", device, feature, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                     VdpBool *is_supported)
{
    traceVdpVideoMixerQueryParameterSupport("{zilch}", device, parameter, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                     VdpBool *is_supported)
{
    traceVdpVideoMixerQueryAttributeSupport("{zilch}", device, attribute, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                        void *min_value, void *max_value)
{
    traceVdpVideoMixerQueryParameterValueRange("{zilch}", device, parameter, min_value, max_value);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                        void *min_value, void *max_value)
{
    traceVdpVideoMixerQueryAttributeValueRange("{zilch}", device, attribute, min_value, max_value);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                      VdpVideoMixerFeature const *features, uint32_t parameter_count,
                      VdpVideoMixerParameter const *parameters,
                      void const *const *parameter_values, VdpVideoMixer *mixer)
{
    traceVdpVideoMixerCreate("{part}", device, feature_count, features, parameter_count, parameters,
        parameter_values, mixer);
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    // TODO: handle parameters and features
    VdpVideoMixerData *data = calloc(1, sizeof(VdpVideoMixerData));
    data->type = HANDLETYPE_VIDEO_MIXER;
    data->device = deviceData;

    *mixer = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                 VdpVideoMixerFeature const *features,
                                 VdpBool const *feature_enables)
{
    traceVdpVideoMixerSetFeatureEnables("{zilch}", mixer, feature_count, features, feature_enables);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                  VdpVideoMixerAttribute const *attributes,
                                  void const *const *attribute_values)
{
    traceVdpVideoMixerSetAttributeValues("{zilch}", mixer, attribute_count, attributes,
        attribute_values);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                 VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    traceVdpVideoMixerGetFeatureSupport("{zilch}", mixer, feature_count, features,
        feature_supports);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                 VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    traceVdpVideoMixerGetFeatureEnables("{zilch}", mixer, feature_count, features, feature_enables);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                  VdpVideoMixerParameter const *parameters,
                                  void *const *parameter_values)
{
    traceVdpVideoMixerGetParameterValues("{zilch}", mixer, parameter_count, parameters,
        parameter_values);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                  VdpVideoMixerAttribute const *attributes,
                                  void *const *attribute_values)
{
    traceVdpVideoMixerGetAttributeValues("{zilch}", mixer, attribute_count, attributes,
        attribute_values);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    traceVdpVideoMixerDestroy("{full}", mixer);
    VdpVideoMixerData *mixerData = handlestorage_get(mixer, HANDLETYPE_VIDEO_MIXER);
    if (NULL == mixerData) return VDP_STATUS_INVALID_HANDLE;

    handlestorage_expunge(mixer);
    free(mixerData);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpVideoMixerRender(VdpVideoMixer mixer, VdpOutputSurface background_surface,
                      VdpRect const *background_source_rect,
                      VdpVideoMixerPictureStructure current_picture_structure,
                      uint32_t video_surface_past_count, VdpVideoSurface const *video_surface_past,
                      VdpVideoSurface video_surface_current, uint32_t video_surface_future_count,
                      VdpVideoSurface const *video_surface_future, VdpRect const *video_source_rect,
                      VdpOutputSurface destination_surface, VdpRect const *destination_rect,
                      VdpRect const *destination_video_rect, uint32_t layer_count,
                      VdpLayer const *layers)
{
    traceVdpVideoMixerRender("{dirty}", mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);

    VdpVideoMixerData *videoMixerData = handlestorage_get(mixer, HANDLETYPE_VIDEO_MIXER);
    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpVideoSurfaceData *srcSurfData =
        handlestorage_get(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == videoMixerData || NULL == dstSurfData || NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    if (videoMixerData->device != dstSurfData->device
        || videoMixerData->device != srcSurfData->device)
    {
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    }
    VADisplay va_dpy = videoMixerData->device->va_dpy;
    VAStatus status;

    // TODO: fix this dirty implementation
    char *dstBuf, *srcBuf;
    status = vaMapBuffer(va_dpy, dstSurfData->va_derived_image.buf, (void  **)&dstBuf);
    failOnErrorWithRetval("vaMapBuffer", status, VDP_STATUS_ERROR);
    status = vaMapBuffer(va_dpy, srcSurfData->va_derived_image.buf, (void  **)&srcBuf);
    failOnErrorWithRetval("vaMapBuffer", status, VDP_STATUS_ERROR);

    memcpy(dstBuf, srcBuf, srcSurfData->va_derived_image.data_size);

    status = vaUnmapBuffer(va_dpy, dstSurfData->va_derived_image.buf);
    failOnErrorWithRetval("vaUnmapBuffer", status, VDP_STATUS_ERROR);
    status = vaUnmapBuffer(va_dpy, srcSurfData->va_derived_image.buf);
    failOnErrorWithRetval("vaUnmapBuffer", status, VDP_STATUS_ERROR);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    traceVdpPresentationQueueTargetDestroy("{full}", presentation_queue_target);
    VdpPresentationQueueTargetData *data =
        handlestorage_get(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == data) return VDP_STATUS_INVALID_HANDLE;
    handlestorage_expunge(presentation_queue_target);
    free(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpPresentationQueueCreate(VdpDevice device, VdpPresentationQueueTarget presentation_queue_target,
                             VdpPresentationQueue *presentation_queue)
{
    traceVdpPresentationQueueCreate("{full}", device, presentation_queue_target,
        presentation_queue);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *targetData =
        handlestorage_get(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == targetData) return VDP_STATUS_INVALID_HANDLE;

    if (targetData->device != deviceData) return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

    VdpPresentationQueueData *data = calloc(1, sizeof(VdpPresentationQueueData));
    if (NULL == data) return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_PRESENTATION_QUEUE;
    data->device = deviceData;
    data->target = targetData;

    *presentation_queue = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    traceVdpPresentationQueueDestroy("{full}", presentation_queue);

    VdpPresentationQueueData *data =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == data) return VDP_STATUS_INVALID_HANDLE;
    handlestorage_expunge(presentation_queue);
    free(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                         VdpColor *const background_color)
{
    traceVdpPresentationQueueSetBackgroundColor("{zilch}", presentation_queue, background_color);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                         VdpColor *background_color)
{
    traceVdpPresentationQueueGetBackgroundColor("{zilch}", presentation_queue, background_color);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue, VdpTime *current_time)
{
    traceVdpPresentationQueueGetTime("{zilch}", presentation_queue, current_time);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                              uint32_t clip_width, uint32_t clip_height,
                              VdpTime earliest_presentation_time)
{
    traceVdpPresentationQueueDisplay("{full}", presentation_queue, surface, clip_width,
        clip_height, earliest_presentation_time);
    VdpPresentationQueueData *presentationQueueData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == presentationQueueData || NULL == surfData) return VDP_STATUS_INVALID_HANDLE;
    if (presentationQueueData->device != surfData->device) return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = surfData->device;

    VAStatus status = vaPutSurface(deviceData->va_dpy, surfData->va_surf,
                                   presentationQueueData->target->drawable,
                                   0, 0, surfData->width, surfData->height,
                                   0, 0, surfData->width, surfData->height,
                                   NULL, 0, VA_FRAME_PICTURE);
    failOnErrorWithRetval("vaPutSurface", status, VDP_STATUS_ERROR);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                            VdpOutputSurface surface,
                                            VdpTime *first_presentation_time)

{
    traceVdpPresentationQueueBlockUntilSurfaceIdle("{full}", presentation_queue, surface,
        first_presentation_time);
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpPresentationQueueData *presentationQueueData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == surfData || NULL == presentationQueueData)
        return VDP_STATUS_INVALID_HANDLE;
    if (surfData->device != presentationQueueData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = surfData->device;

    VAStatus status = vaSyncSurface(deviceData->va_dpy, surfData->va_surf);
    failOnErrorWithRetval("vaSyncSurface", status, VDP_STATUS_ERROR);

    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                         VdpOutputSurface surface,
                                         VdpPresentationQueueStatus *status,
                                         VdpTime *first_presentation_time)
{
    traceVdpPresentationQueueQuerySurfaceStatus("{zilch}", presentation_queue, surface,
        status, first_presentation_time);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                   VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
    traceVdpVideoSurfaceQueryCapabilities("{zilch}", device, surface_chroma_type, is_supported,
        max_width, max_height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpChromaType surface_chroma_type,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities("{zilch}", device, surface_chroma_type,
        bits_ycbcr_format, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                        uint32_t height, VdpVideoSurface *surface)
{
    traceVdpVideoSurfaceCreate("{part}", device, chroma_type, width, height, surface);
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    // TODO: Do I need other chroma types?
    if (VDP_CHROMA_TYPE_420 != chroma_type)
        return VDP_STATUS_INVALID_CHROMA_TYPE;

    VdpVideoSurfaceData *data = calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data) return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_VIDEO_SURFACE;
    data->device = deviceData;
    data->width = width;
    data->height = height;
    data->chroma_type = chroma_type;

    VAStatus status = vaCreateSurfaces(deviceData->va_dpy, width, height, VA_RT_FORMAT_YUV420,
                                       1, &data->va_surf);
    if (VA_STATUS_SUCCESS != status) {
        free(data);
        return VDP_STATUS_ERROR;
    }

    status = vaDeriveImage(deviceData->va_dpy, data->va_surf, &data->va_derived_image);
    workaround_missing_data_size_of_derived_image(&data->va_derived_image);
    if (VA_STATUS_SUCCESS != status) {
        free(data);
        return VDP_STATUS_ERROR;
    }

    *surface = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    traceVdpVideoSurfaceDestroy("{full}", surface);
    VdpVideoSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == surfData) return VDP_STATUS_INVALID_HANDLE;

    // ignore errors
    vaDestroyImage(surfData->device->va_dpy, surfData->va_derived_image.image_id);
    vaDestroySurfaces(surfData->device->va_dpy, &surfData->va_surf, 1);

    handlestorage_expunge(surface);
    free(surfData);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                               uint32_t *width, uint32_t *height)
{
    traceVdpVideoSurfaceGetParameters("{zilch}", surface, chroma_type, width, height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    traceVdpVideoSurfaceGetBitsYCbCr("{zilch}", surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                              void const *const *source_data, uint32_t const *source_pitches)
{
    traceVdpVideoSurfacePutBitsYCbCr("{part}", surface, source_ycbcr_format, source_data,
        source_pitches);
    VdpVideoSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == surfData) return VDP_STATUS_INVALID_HANDLE;
    VADisplay va_dpy = surfData->device->va_dpy;

    if (VDP_YCBCR_FORMAT_YV12 != source_ycbcr_format)
        return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;

    char *dstBuf;
    VAStatus status = vaMapBuffer(va_dpy, surfData->va_derived_image.buf, (void **)&dstBuf);
    failOnErrorWithRetval("vaMapBuffer", status, VDP_STATUS_ERROR);

    // FIXME: now assuming data not aligned. Add generic code path.
    memcpy(dstBuf, source_data[0], surfData->width * surfData->height);

    status = vaUnmapBuffer(va_dpy, surfData->va_derived_image.buf);
    failOnErrorWithRetval("vaUnmapBuffer", status, VDP_STATUS_ERROR);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpBool *is_supported, uint32_t *max_width,
                                    uint32_t *max_height)
{
    traceVdpBitmapSurfaceQueryCapabilities("{zilch}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                         uint32_t height, VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    traceVdpBitmapSurfaceCreate("{part}", device, rgba_format, width, height, frequently_accessed,
        surface);
    VAStatus status;
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    VAImageFormat *formats = calloc(sizeof(VAImageFormat),
                                    vaMaxNumSubpictureFormats(deviceData->va_dpy));
    if (NULL == formats) return VDP_STATUS_RESOURCES;
    unsigned int num_formats;
    status = vaQuerySubpictureFormats(deviceData->va_dpy, formats, NULL, &num_formats);
    if (VA_STATUS_SUCCESS != status) {
        traceTrace("vaVdpBitmapSurfaceCreate, error querying formats, status %d\n", status);
        free(formats);
        return VDP_STATUS_ERROR;
    }

    VAImageFormat *fmt = NULL;
    for (unsigned int k = 0; k < num_formats; k ++) {
        // TODO: check for other formats
        if (formats[k].fourcc == VA_FOURCC('B','G','R','A')) {
            fmt = &formats[k];
            break;
        }
    }
    if (NULL == fmt)
        return VDP_STATUS_ERROR;

    VAImage va_img;
    status = vaCreateImage(deviceData->va_dpy, fmt, width, height, &va_img);
    if (VA_STATUS_SUCCESS != status) {
        traceTrace("vaVdpBitmapSurfaceCreate, can't create surface, status %d\n", status);
        return VDP_STATUS_ERROR;
    }

    VdpBitmapSurfaceData *data = calloc(1, sizeof(VdpBitmapSurfaceData));
    if (NULL == data) return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_BITMAP_SURFACE;
    data->device = deviceData;
    data->rgba_format = rgba_format;
    data->width = width;
    data->height = height;
    data->va_img = va_img;

    *surface = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    traceVdpBitmapSurfaceDestroy("{full}", surface);
    VAStatus status;
    VdpBitmapSurfaceData *data = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == data) return VDP_STATUS_INVALID_HANDLE;
    status = vaDestroyImage(data->device->va_dpy, data->va_img.image_id);
    if (VA_STATUS_SUCCESS != status) return VDP_STATUS_ERROR;
    handlestorage_expunge(surface);
    free(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    traceVdpBitmapSurfaceGetParameters("{zilch}", surface, rgba_format, width, height,
        frequently_accessed);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    traceVdpBitmapSurfacePutBitsNative("{full}", surface, source_data, source_pitches,
        destination_rect);
    VAStatus status;
    VdpBitmapSurfaceData *surfaceData = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == surfaceData) return VDP_STATUS_INVALID_HANDLE;

    VdpRect rect = {0, 0, surfaceData->width, surfaceData->height};
    if (NULL != destination_rect) rect = *destination_rect;

    // TODO: handle other formats
    if (VDP_RGBA_FORMAT_B8G8R8A8 != surfaceData->rgba_format)
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    char *va_image_buf;
    status = vaMapBuffer(surfaceData->device->va_dpy,
                         surfaceData->va_img.buf, (void **)&va_image_buf);
    if (status != VA_STATUS_SUCCESS) {
        traceTrace("vaMapBuffer returned %d at %s:%d\n", status, __FILE__, __LINE__);
        return VDP_STATUS_ERROR;
    }

    if (0 == rect.x0 && 0 == rect.y0 && source_pitches[0] == surfaceData->width * 4) {
        // can copy with single memcpy
        memcpy(va_image_buf, source_data[0], surfaceData->va_img.data_size);
    } else {
        // fall back to generic code
        char *dst_ptr = va_image_buf + 4 * (surfaceData->width * rect.y0 + rect.x0);
        char const *src_ptr = source_data[0];
        for (unsigned int k = 0; k < rect.y1 - rect.y0; k ++) {
            memcpy(dst_ptr, src_ptr, (rect.x1 - rect.x0) * 4);
            src_ptr += source_pitches[0];
            dst_ptr += surfaceData->width * 4;
        }
    }

    vaUnmapBuffer(surfaceData->device->va_dpy, surfaceData->va_img.buf);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpDeviceDestroy(VdpDevice device)
{
    traceVdpDeviceDestroy("{full}", device);
    VdpDeviceData *data = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    handlestorage_expunge(device);
    vaTerminate(data->va_dpy);
    free(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpGetInformationString(char const **information_string)
{
    traceVdpGetInformationString("{full}", information_string);
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    traceVdpGenerateCSCMatrix("{zilch}", procamp, standard, csc_matrix);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                      VdpRect const *destination_rect,
                                      VdpOutputSurface source_surface, VdpRect const *source_rect,
                                      VdpColor const *colors,
                                      VdpOutputSurfaceRenderBlendState const *blend_state,
                                      uint32_t flags)
{
    traceVdpOutputSurfaceRenderOutputSurface("{dirty}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);

    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpOutputSurfaceData *srcSurfData =
        handlestorage_get(source_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurfData || NULL == srcSurfData) return VDP_STATUS_INVALID_HANDLE;
    if (dstSurfData->device != srcSurfData->device) return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VADisplay va_dpy = dstSurfData->device->va_dpy;
    VAStatus status;

    // TODO: fix this dirty implementation
    char *dstBuf, *srcBuf;
    status = vaMapBuffer(va_dpy, dstSurfData->va_derived_image.buf, (void  **)&dstBuf);
    failOnErrorWithRetval("vaMapBuffer", status, VDP_STATUS_ERROR);
    status = vaMapBuffer(va_dpy, srcSurfData->va_derived_image.buf, (void  **)&srcBuf);
    failOnErrorWithRetval("vaMapBuffer", status, VDP_STATUS_ERROR);

    memcpy(dstBuf, srcBuf, srcSurfData->va_derived_image.data_size);

    status = vaUnmapBuffer(va_dpy, dstSurfData->va_derived_image.buf);
    failOnErrorWithRetval("vaUnmapBuffer", status, VDP_STATUS_ERROR);
    status = vaUnmapBuffer(va_dpy, srcSurfData->va_derived_image.buf);
    failOnErrorWithRetval("vaUnmapBuffer", status, VDP_STATUS_ERROR);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                      VdpRect const *destination_rect,
                                      VdpBitmapSurface source_surface, VdpRect const *source_rect,
                                      VdpColor const *colors,
                                      VdpOutputSurfaceRenderBlendState const *blend_state,
                                      uint32_t flags)
{
    traceVdpOutputSurfaceRenderBitmapSurface("{part}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);

    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpBitmapSurfaceData *srcSurfData =
        handlestorage_get(source_surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData || NULL == srcSurfData) return VDP_STATUS_INVALID_HANDLE;
    if (dstSurfData->device != srcSurfData->device) return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = srcSurfData->device;
    VADisplay va_dpy = deviceData->va_dpy;
    VAStatus status;

    VdpRect dst_rect = {0, 0, dstSurfData->width, dstSurfData->height};
    VdpRect src_rect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (destination_rect) dst_rect = *destination_rect;
    if (source_rect) src_rect = *source_rect;

    // TODO: handle blend_options
    status = vaAssociateSubpicture(deviceData->va_dpy, dstSurfData->va_subpic,
                    &dstSurfData->va_surf, 1,
                    src_rect.x0, src_rect.y0, src_rect.x1 - src_rect.x0, src_rect.y1 - src_rect.y0,
                    dst_rect.x0, dst_rect.y0, dst_rect.x1 - dst_rect.x0, dst_rect.y1 - dst_rect.y0,
                    0);
    failOnErrorWithRetval("vaAssociateSubpicture", status, VDP_STATUS_ERROR);

    char *buf_src, *buf_dst;
    vaMapBuffer(va_dpy, srcSurfData->va_img.buf, (void**)&buf_src);
    vaMapBuffer(va_dpy, dstSurfData->va_img.buf, (void**)&buf_dst);

    memcpy(buf_dst, buf_src, srcSurfData->va_img.data_size);

    vaUnmapBuffer(va_dpy, srcSurfData->va_img.buf);
    vaUnmapBuffer(va_dpy, dstSurfData->va_img.buf);

    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    traceVdpPreemptionCallbackRegister("{zilch}", device, callback, context);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
vaVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                      VdpPresentationQueueTarget *target)
{
    traceVdpPresentationQueueTargetCreateX11("{full}", device, drawable, target);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *data = calloc(1, sizeof(VdpPresentationQueueTargetData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_PRESENTATION_QUEUE_TARGET;
    data->device = deviceData;
    data->drawable = drawable;

    *target = handlestorage_add(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
vaVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    traceVdpGetProcAddress("{full}", device, function_id, function_pointer);

    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = &vaVdpGetErrorString;
        break;
    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = &vaVdpGetProcAddress;
        break;
    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = &vaVdpGetApiVersion;
        break;
    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = &vaVdpGetInformationString;
        break;
    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = &vaVdpDeviceDestroy;
        break;
    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = &vaVdpGenerateCSCMatrix;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &vaVdpVideoSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &vaVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = &vaVdpVideoSurfaceCreate;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = &vaVdpVideoSurfaceDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = &vaVdpVideoSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = &vaVdpVideoSurfaceGetBitsYCbCr;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &vaVdpVideoSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &vaVdpOutputSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = &vaVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = &vaVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &vaVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = &vaVdpOutputSurfaceCreate;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = &vaVdpOutputSurfaceDestroy;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = &vaVdpOutputSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = &vaVdpOutputSurfaceGetBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &vaVdpOutputSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = &vaVdpOutputSurfacePutBitsIndexed;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &vaVdpOutputSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &vaVdpBitmapSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = &vaVdpBitmapSurfaceCreate;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = &vaVdpBitmapSurfaceDestroy;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = &vaVdpBitmapSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &vaVdpBitmapSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = &vaVdpOutputSurfaceRenderOutputSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = &vaVdpOutputSurfaceRenderBitmapSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // *function_pointer = &vaVdpOutputSurfaceRenderVideoSurfaceLuma;
        *function_pointer = NULL;
        break;
    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = &vaVdpDecoderQueryCapabilities;
        break;
    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = &vaVdpDecoderCreate;
        break;
    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = &vaVdpDecoderDestroy;
        break;
    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = &vaVdpDecoderGetParameters;
        break;
    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = &vaVdpDecoderRender;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = &vaVdpVideoMixerQueryFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = &vaVdpVideoMixerQueryParameterSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = &vaVdpVideoMixerQueryAttributeSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = &vaVdpVideoMixerQueryParameterValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = &vaVdpVideoMixerQueryAttributeValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = &vaVdpVideoMixerCreate;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = &vaVdpVideoMixerSetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = &vaVdpVideoMixerSetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = &vaVdpVideoMixerGetFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = &vaVdpVideoMixerGetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = &vaVdpVideoMixerGetParameterValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = &vaVdpVideoMixerGetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = &vaVdpVideoMixerDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = &vaVdpVideoMixerRender;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = &vaVdpPresentationQueueTargetDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = &vaVdpPresentationQueueCreate;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = &vaVdpPresentationQueueDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = &vaVdpPresentationQueueSetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = &vaVdpPresentationQueueGetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = &vaVdpPresentationQueueGetTime;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = &vaVdpPresentationQueueDisplay;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = &vaVdpPresentationQueueBlockUntilSurfaceIdle;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = &vaVdpPresentationQueueQuerySurfaceStatus;
        break;
    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = &vaVdpPreemptionCallbackRegister;
        break;
    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = &vaVdpPresentationQueueTargetCreateX11;
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
vaVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                     VdpGetProcAddress **get_proc_address)
{
    traceVdpDeviceCreateX11("{full}", display, screen, device, get_proc_address);

    VADisplay va_dpy = vaGetDisplay(display);
    int ver_major;
    int ver_minor;
    VAStatus res = vaInitialize(va_dpy, &ver_major, &ver_minor);
    if (VA_STATUS_SUCCESS != res)
        return VDP_STATUS_ERROR;
    traceTrace(" version %d.%d\n", ver_major, ver_minor);

    VdpDeviceData *data = calloc(1, sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type =    HANDLETYPE_DEVICE;
    data->display = display;
    data->screen =  screen;
    data->va_dpy =  va_dpy;

    *device = handlestorage_add(data);
    *get_proc_address = &vaVdpGetProcAddress;
    return VDP_STATUS_OK;
}
