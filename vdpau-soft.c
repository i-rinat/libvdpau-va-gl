#define _XOPEN_SOURCE
#include <assert.h>
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
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include "reverse-constant.h"
#include "handle-storage.h"
#include "vdpau-trace.h"


static char const *
implemetation_description_string = "OpenGL/libswscale backend for VDPAU";

typedef struct {
    HandleType  type;
    Display    *display;
    int         screen;
    GLXContext  glc;
    Window      root;
    GLuint      fbo_id;
} VdpDeviceData;

typedef struct {
    HandleType type;
    VdpDeviceData *device;
    Drawable drawable;
} VdpPresentationQueueTargetData;

typedef struct {
    HandleType type;
    VdpDeviceData *device;
    VdpPresentationQueueTargetData *target;
    XShmSegmentInfo shminfo;
    XImage *image;
    uint32_t prev_width;
    uint32_t prev_height;
} VdpPresentationQueueData;

typedef struct {
    HandleType type;
    VdpDeviceData *device;
} VdpVideoMixerData;

typedef struct {
    HandleType          type;
    VdpDeviceData      *device;
    VdpRGBAFormat       rgba_format;
    GLuint              tex_id;
    uint32_t            width;
    uint32_t            height;
} VdpOutputSurfaceData;

typedef struct {
    HandleType type;
    VdpDeviceData *device;
    VdpChromaType chroma_type;
    uint32_t width;
    uint32_t stride;
    uint32_t height;
    void *y_plane;
    void *v_plane;
    void *u_plane;
} VdpVideoSurfaceData;

typedef struct {
    HandleType      type;
    VdpDeviceData  *device;
    VdpRGBAFormat   rgba_format;
    GLuint          tex_id;
    uint32_t        width;
    uint32_t        height;
} VdpBitmapSurfaceData;

// ====================
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
    return reverse_status(status);
}

static
VdpStatus
softVdpGetApiVersion(uint32_t *api_version)
{
    traceVdpGetApiVersion("{full}", api_version);
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpDecoderQueryCapabilities(VdpDevice device, VdpDecoderProfile profile, VdpBool *is_supported,
                                uint32_t *max_level, uint32_t *max_macroblocks,
                                uint32_t *max_width, uint32_t *max_height)
{
    traceVdpDecoderQueryCapabilities("{zilch}", device, profile, is_supported, max_level,
        max_macroblocks, max_width, max_height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderCreate(VdpDevice device, VdpDecoderProfile profile, uint32_t width, uint32_t height,
                     uint32_t max_references, VdpDecoder *decoder)
{
    traceVdpDecoderCreate("{zilch}", device, profile, width, height, max_references, decoder);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderDestroy(VdpDecoder decoder)
{
    traceVdpDecoderDestroy("{zilch}", decoder);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderGetParameters(VdpDecoder decoder, VdpDecoderProfile *profile,
                            uint32_t *width, uint32_t *height)
{
    traceVdpDecoderGetParameters("{zilch}", decoder, profile, width, height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpDecoderRender(VdpDecoder decoder, VdpVideoSurface target,
                     VdpPictureInfo const *picture_info, uint32_t bitstream_buffer_count,
                     VdpBitstreamBuffer const *bitstream_buffers)
{
    traceVdpDecoderRender("{zilch}", decoder, target, picture_info, bitstream_buffer_count,
        bitstream_buffers);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    traceVdpOutputSurfaceQueryCapabilities("{zilch}", device, surface_rgba_format, is_supported,
        max_width, max_height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpBool *is_supported)
{
    traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities("{zilch}", device, surface_rgba_format,
        is_supported);
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
    traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities("{zilch}", device, surface_rgba_format,
        bits_indexed_format, color_table_format, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities("{zilch}", device, surface_rgba_format,
        bits_ycbcr_format, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpOutputSurface *surface)
{
    traceVdpOutputSurfaceCreate("{part}", device, rgba_format, width, height, surface);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    //TODO: figure out reasonable limits
    if (width > 4096 || height > 4096)
        return VDP_STATUS_INVALID_SIZE;

    //TODO: other formats
    if (VDP_RGBA_FORMAT_B8G8R8A8 != rgba_format) {
        fprintf(stderr, "error: unsupported rgba format: %s\n", reverse_rgba_format(rgba_format));
        return VDP_STATUS_INVALID_RGBA_FORMAT;
    }

    VdpOutputSurfaceData *data = (VdpOutputSurfaceData *)calloc(1, sizeof(VdpOutputSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_OUTPUT_SURFACE;
    data->width = width;
    data->height = height;
    data->device = deviceData;
    data->rgba_format = rgba_format;

    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // reserve texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    traceVdpOutputSurfaceDestroy("{full}", surface);

    VdpOutputSurfaceData *data = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->device;

    glXMakeCurrent(data->device->display, deviceData->root, deviceData->glc);
    glDeleteTextures(1, &data->tex_id);

    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height)
{
    traceVdpOutputSurfaceGetParameters("{zilch}", surface, rgba_format, width, height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    traceVdpOutputSurfaceGetBitsNative("{zilch}", surface, source_rect, destination_data,
        destination_pitches);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    traceVdpOutputSurfacePutBitsNative("{zilch}", surface, source_data, source_pitches,
        destination_rect);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
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
softVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                 void const *const *source_data, uint32_t const *source_pitches,
                                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    traceVdpOutputSurfacePutBitsYCbCr("{zilch}", surface, source_ycbcr_format, source_data,
        source_pitches, destination_rect, csc_matrix);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                     VdpBool *is_supported)
{
    traceVdpVideoMixerQueryFeatureSupport("{zilch}", device, feature, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                       VdpBool *is_supported)
{
    traceVdpVideoMixerQueryParameterSupport("{zilch}", device, parameter, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                       VdpBool *is_supported)
{
    traceVdpVideoMixerQueryAttributeSupport("{zilch}", device, attribute, is_supported);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                          void *min_value, void *max_value)
{
    traceVdpVideoMixerQueryParameterValueRange("{zilch}", device, parameter, min_value, max_value);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                          void *min_value, void *max_value)
{
    traceVdpVideoMixerQueryAttributeValueRange("{zilch}", device, attribute, min_value, max_value);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                        VdpVideoMixerFeature const *features, uint32_t parameter_count,
                        VdpVideoMixerParameter const *parameters,
                        void const *const *parameter_values, VdpVideoMixer *mixer)
{
    traceVdpVideoMixerCreate("{part}", device, feature_count, features, parameter_count, parameters,
        parameter_values, mixer);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoMixerData *data = (VdpVideoMixerData *)calloc(1, sizeof(VdpVideoMixerData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_VIDEO_MIXER;
    data->device = deviceData;

    *mixer = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features,
                                   VdpBool const *feature_enables)
{
    traceVdpVideoMixerSetFeatureEnables("{part}", mixer, feature_count, features, feature_enables);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void const *const *attribute_values)
{
    traceVdpVideoMixerSetAttributeValues("{part}", mixer, attribute_count, attributes,
        attribute_values);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    traceVdpVideoMixerGetFeatureSupport("{zilch}", mixer, feature_count, features,
        feature_supports);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    traceVdpVideoMixerGetFeatureEnables("{zilch}", mixer, feature_count, features, feature_enables);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                    VdpVideoMixerParameter const *parameters,
                                    void *const *parameter_values)
{
    traceVdpVideoMixerGetParameterValues("{zilch}", mixer, parameter_count, parameters,
        parameter_values);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void *const *attribute_values)
{
    traceVdpVideoMixerGetAttributeValues("{zilch}", mixer, attribute_count, attributes,
        attribute_values);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    traceVdpVideoMixerDestroy("{full}", mixer);

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
    traceVdpVideoMixerRender("{part}", mixer, background_surface, background_source_rect,
        current_picture_structure, video_surface_past_count, video_surface_past,
        video_surface_current, video_surface_future_count, video_surface_future, video_source_rect,
        destination_surface, destination_rect, destination_video_rect, layer_count, layers);

    //TODO: handle rectangles
    VdpVideoSurfaceData *source_surface =
        handlestorage_get(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == source_surface)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *dest_surface =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dest_surface)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = source_surface->device;

    uint8_t *img_buf = malloc(dest_surface->width * dest_surface->height * 4);
    if (NULL == img_buf) return VDP_STATUS_RESOURCES;

    struct SwsContext *sws_ctx =
        sws_getContext(source_surface->width, source_surface->height, PIX_FMT_YUV420P,
            dest_surface->width, dest_surface->height,
            PIX_FMT_RGBA, SWS_POINT, NULL, NULL, NULL);

    uint8_t const * const src_planes[] =
        { source_surface->y_plane, source_surface->v_plane, source_surface->u_plane, NULL };
    int src_strides[] =
        {source_surface->stride, source_surface->stride/2, source_surface->stride/2, 0};
    uint8_t *dst_planes[] = {img_buf, NULL, NULL, NULL};
    int dst_strides[] = {dest_surface->width * 4, 0, 0, 0};

    int res = sws_scale(sws_ctx,
                        src_planes, src_strides, 0, source_surface->height,
                        dst_planes, dst_strides);
    sws_freeContext(sws_ctx);
    if (res != dest_surface->height) {
        fprintf(stderr, "scaling failed\n");
        return VDP_STATUS_ERROR;
    }

    // copy converted image to texture
    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glBindTexture(GL_TEXTURE_2D, dest_surface->tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dest_surface->width, dest_surface->height,
        GL_RGBA, GL_UNSIGNED_BYTE, img_buf);
    free(img_buf);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    traceVdpPresentationQueueTargetDestroy("{full}", presentation_queue_target);

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
    traceVdpPresentationQueueCreate("{part}", device, presentation_queue_target,
        presentation_queue);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *targetData =
        handlestorage_get(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == targetData) return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueData *data =
        (VdpPresentationQueueData *)calloc(1, sizeof(VdpPresentationQueueData));
    if (NULL == data) return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_PRESENTATION_QUEUE;
    data->device = deviceData;
    data->target = targetData;
    data->image = NULL;
    data->prev_width = 0;
    data->prev_height = 0;
    *presentation_queue = handlestorage_add(data);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    traceVdpPresentationQueueDestroy("{full}", presentation_queue);

    VdpPresentationQueueData *data =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == data) return VDP_STATUS_INVALID_HANDLE;

    if (data->image) {
        XShmDetach(data->target->device->display, &data->shminfo);
        free(data->image);
        shmdt(data->shminfo.shmaddr);
    }

    free(data);
    handlestorage_expunge(presentation_queue);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *const background_color)
{
    traceVdpPresentationQueueSetBackgroundColor("{zilch}", presentation_queue, background_color);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *background_color)
{
    traceVdpPresentationQueueGetBackgroundColor("{zilch}", presentation_queue, background_color);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                                VdpTime *current_time)
{
    traceVdpPresentationQueueGetTime("{full}", presentation_queue, current_time);

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
    traceVdpPresentationQueueDisplay("{part}", presentation_queue, surface, clip_width, clip_height,
        earliest_presentation_time);

    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpPresentationQueueData *pqueueData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == surfData || NULL == pqueueData) return VDP_STATUS_INVALID_HANDLE;
    if (pqueueData->device != surfData->device) return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = surfData->device;

    glXMakeCurrent(deviceData->display, pqueueData->target->drawable, deviceData->glc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, surfData->width - 1, surfData->height - 1, 0, -1.0, 1.0);
    glViewport(0, 0, surfData->width, surfData->height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, surfData->tex_id);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, 0);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(surfData->width - 1, 0);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(surfData->width - 1, surfData->height - 1);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, surfData->height - 1);
    glEnd();
    glXSwapBuffers(deviceData->display, pqueueData->target->drawable);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                              VdpOutputSurface surface,
                                              VdpTime *first_presentation_time)

{
    traceVdpPresentationQueueBlockUntilSurfaceIdle("{full}", presentation_queue, surface,
        first_presentation_time);

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
    traceVdpPresentationQueueQuerySurfaceStatus("{part}", presentation_queue, surface,
        status, first_presentation_time);

    *status = VDP_PRESENTATION_QUEUE_STATUS_VISIBLE;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpBool *is_supported, uint32_t *max_width,
                                     uint32_t *max_height)
{
    traceVdpVideoSurfaceQueryCapabilities("{part}", device, surface_chroma_type, is_supported,
        max_width, max_height);

    *is_supported = 1;
    *max_width = 1920;
    *max_height = 1080;

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpChromaType surface_chroma_type,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities("{part}", device, surface_chroma_type,
        bits_ycbcr_format, is_supported);

    *is_supported = 1;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                          uint32_t height, VdpVideoSurface *surface)
{
    traceVdpVideoSurfaceCreate("{part}", device, chroma_type, width, height, surface);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoSurfaceData *data = (VdpVideoSurfaceData *)calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    uint32_t const stride = (width % 4 == 0) ? width : (width & ~0x3UL) + 4;
    data->type = HANDLETYPE_VIDEO_SURFACE;
    data->device = deviceData;
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
    traceVdpVideoSurfaceDestroy("{full}", surface);

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
    traceVdpVideoSurfaceGetParameters("{zilch}", surface, chroma_type, width, height);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    traceVdpVideoSurfaceGetBitsYCbCr("{zilch}", surface, destination_ycbcr_format,
        destination_data, destination_pitches);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                void const *const *source_data, uint32_t const *source_pitches)
{
    traceVdpVideoSurfacePutBitsYCbCr("{part}", surface, source_ycbcr_format, source_data,
        source_pitches);

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
        dst += surfaceData->stride / 2;
        src += source_pitches[1];
    }

    dst = surfaceData->u_plane;     src = source_data[2];
    for (uint32_t k = 0; k < surfaceData->height / 2; k ++) {
        memcpy(dst, src, surfaceData->width/2);
        dst += surfaceData->stride / 2;
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
    traceVdpBitmapSurfaceQueryCapabilities("{part}", device, surface_rgba_format, is_supported,
        max_width, max_height);

    if (! handlestorage_valid(device, HANDLETYPE_DEVICE))
        return VDP_STATUS_INVALID_HANDLE;

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
    traceVdpBitmapSurfaceCreate("{full}", device, rgba_format, width, height, frequently_accessed,
        surface);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData) return VDP_STATUS_INVALID_HANDLE;

    VdpBitmapSurfaceData *data = (VdpBitmapSurfaceData *)calloc(1, sizeof(VdpBitmapSurfaceData));
    if (NULL == data) return VDP_STATUS_RESOURCES;

    //TODO: other format handling
    if (rgba_format != VDP_RGBA_FORMAT_B8G8R8A8)
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    data->type = HANDLETYPE_BITMAP_SURFACE;
    data->device = deviceData;
    data->rgba_format = rgba_format;
    data->width = width;
    data->height = height;

    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    *surface = handlestorage_add(data);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    traceVdpBitmapSurfaceDestroy("{full}", surface);

    VdpBitmapSurfaceData *data = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->device;

    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glDeleteTextures(1, &data->tex_id);

    free(data);
    handlestorage_expunge(surface);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    traceVdpBitmapSurfaceGetParameters("{zilch}", surface, rgba_format, width, height,
        frequently_accessed);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    traceVdpBitmapSurfacePutBitsNative("{full}", surface, source_data, source_pitches,
        destination_rect);

    VdpBitmapSurfaceData *dstSurfData = handlestorage_get(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->device;

    //TODO: fix handling other formats
    if (VDP_RGBA_FORMAT_B8G8R8A8 != dstSurfData->rgba_format)
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    VdpRect d_rect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect) d_rect = *destination_rect;

    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]/4);
    glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, d_rect.x0, d_rect.y0,
        d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0, GL_BGRA, GL_UNSIGNED_BYTE, source_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpDeviceDestroy(VdpDevice device)
{
    traceVdpDeviceDestroy("{full}", device);

    VdpDeviceData *data = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    // TODO: Is it right to reset context? App using its own will not be happy with reset.
    glXMakeCurrent(data->display, data->root, data->glc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &data->fbo_id);
    glXMakeCurrent(data->display, None, NULL);
    glXDestroyContext(data->display, data->glc);

    free(data);
    handlestorage_expunge(device);
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpGetInformationString(char const **information_string)
{
    traceVdpGetInformationString("{full}", information_string);
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    traceVdpGenerateCSCMatrix("{part}", procamp, standard, csc_matrix);

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
GLuint
vdpBlendFuncToGLBlendFunc(VdpOutputSurfaceRenderBlendFactor blend_factor)
{
    switch (blend_factor) {
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO:
        return GL_ZERO;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE:
        return GL_ONE;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_COLOR:
        return GL_SRC_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA:
        return GL_SRC_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_ALPHA:
        return GL_DST_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_COLOR:
        return GL_DST_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA_SATURATE:
        return GL_SRC_ALPHA_SATURATE;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_COLOR:
        return GL_CONSTANT_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_ALPHA:
        return GL_CONSTANT_ALPHA;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    default:
        return GL_INVALID_VALUE;
    }
}

static
GLenum
vdpBlendEquationToGLEquation(VdpOutputSurfaceRenderBlendEquation blend_equation)
{
    switch (blend_equation) {
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_SUBTRACT:
        return GL_FUNC_SUBTRACT;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_REVERSE_SUBTRACT:
        return GL_FUNC_REVERSE_SUBTRACT;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD:
        return GL_FUNC_ADD;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MIN:
        return GL_MIN;
    case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MAX:
        return GL_MAX;
    default:
        return GL_INVALID_VALUE;
    }
}

struct blend_state_struct {
    GLuint srcFuncRGB;
    GLuint srcFuncAlpha;
    GLuint dstFuncRGB;
    GLuint dstFuncAlpha;
    GLuint modeRGB;
    GLuint modeAlpha;
    int invalid_func;
    int invalid_eq;
};

static
struct blend_state_struct
vdpBlendStateToGLBlendState(VdpOutputSurfaceRenderBlendState const *blend_state)
{
    struct blend_state_struct bs;
    bs.invalid_func = 0;
    bs.invalid_eq = 0;
    bs.srcFuncRGB = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_source_color);
    bs.srcFuncAlpha = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_source_alpha);
    bs.dstFuncRGB = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_destination_color);
    bs.dstFuncAlpha = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_destination_alpha);

    if (GL_INVALID_VALUE == bs.srcFuncRGB || GL_INVALID_VALUE == bs.srcFuncAlpha ||
        GL_INVALID_VALUE == bs.dstFuncRGB || GL_INVALID_VALUE == bs.dstFuncAlpha)
    {
        bs.invalid_func = 1;
    }

    bs.modeRGB = vdpBlendEquationToGLEquation(blend_state->blend_equation_color);
    bs.modeAlpha = vdpBlendEquationToGLEquation(blend_state->blend_equation_alpha);
    if (GL_INVALID_VALUE == bs.modeRGB || GL_INVALID_VALUE == bs.modeAlpha)
        bs.invalid_eq = 1;

    return bs;
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
    traceVdpOutputSurfaceRenderOutputSurface("{full}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);

    if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurfData) return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *srcSurfData =
        handlestorage_get(source_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData) return VDP_STATUS_INVALID_HANDLE;
    if (srcSurfData->device != dstSurfData->device) return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = srcSurfData->device;

    const int dstWidth = dstSurfData->width;
    const int dstHeight = dstSurfData->height;
    const int srcWidth = srcSurfData->width;
    const int srcHeight = srcSurfData->height;
    VdpRect s_rect = {0, 0, srcWidth, srcHeight};
    VdpRect d_rect = {0, 0, dstWidth, dstHeight};

    if (source_rect) s_rect = *source_rect;
    if (destination_rect) d_rect = *destination_rect;

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);
    if (bs.invalid_func) return VDP_STATUS_INVALID_BLEND_FACTOR;
    if (bs.invalid_eq) return VDP_STATUS_INVALID_BLEND_EQUATION;

    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glBindFramebuffer(GL_FRAMEBUFFER, deviceData->fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        dstSurfData->tex_id, 0);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != gl_status) {
        fprintf(stderr, "framebuffer not ready, %d, %s\n", gl_status, gluErrorString(gl_status));
        return VDP_STATUS_ERROR;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstWidth-1, 0, dstHeight-1, -1.0f, 1.0f);
    glViewport(0, 0, dstWidth, dstHeight);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // paint source surface over
    glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/srcWidth, 1.0f/srcHeight, 1.0f);

    // blend
    glBlendFuncSeparate(bs.srcFuncRGB, bs.dstFuncRGB, bs.srcFuncAlpha, bs.dstFuncAlpha);
    glBlendEquationSeparate(bs.modeRGB, bs.modeAlpha);

    glBegin(GL_QUADS);
    glTexCoord2i(s_rect.x0,   s_rect.y0);   glVertex2f(d_rect.x0,   d_rect.y0);
    glTexCoord2i(s_rect.x1-1, s_rect.y0);   glVertex2f(d_rect.x1-1, d_rect.y0);
    glTexCoord2i(s_rect.x1-1, s_rect.y1-1); glVertex2f(d_rect.x1-1, d_rect.y1-1);
    glTexCoord2i(s_rect.x0,   s_rect.y1-1); glVertex2f(d_rect.x0,   d_rect.y1-1);
    glEnd();

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
    traceVdpOutputSurfaceRenderBitmapSurface("{part}", destination_surface, destination_rect,
        source_surface, source_rect, colors, blend_state, flags);

    if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    VdpOutputSurfaceData *dstSurfData =
        handlestorage_get(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpBitmapSurfaceData *srcSurfData =
        handlestorage_get(source_surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData || NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    if (srcSurfData->device != dstSurfData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = srcSurfData->device;

    const int srcWidth = srcSurfData->width;
    const int srcHeight = srcSurfData->height;
    const int dstWidth = dstSurfData->width;
    const int dstHeight = dstSurfData->height;

    VdpRect s_rect = {0, 0, srcWidth, srcHeight};
    VdpRect d_rect = {0, 0, dstWidth, srcHeight};
    if (source_rect) s_rect = *source_rect;
    if (destination_rect) d_rect = *destination_rect;

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);
    if (bs.invalid_func) return VDP_STATUS_INVALID_BLEND_FACTOR;
    if (bs.invalid_eq) return VDP_STATUS_INVALID_BLEND_EQUATION;

    glXMakeCurrent(deviceData->display, deviceData->root, deviceData->glc);
    glBindFramebuffer(GL_FRAMEBUFFER, deviceData->fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        dstSurfData->tex_id, 0);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != gl_status) {
        fprintf(stderr, "framebuffer not ready, %d, %s\n", gl_status, gluErrorString(gl_status));
        return VDP_STATUS_ERROR;
    }
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstWidth-1, 0, dstHeight-1, -1.0f, 1.0f);
    glViewport(0, 0, dstWidth, dstHeight);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // paint source surface over
    glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/srcWidth, 1.0f/srcHeight, 1.0f);

    // blend
    glBlendFuncSeparate(bs.srcFuncRGB, bs.dstFuncRGB, bs.srcFuncAlpha, bs.dstFuncAlpha);
    glBlendEquationSeparate(bs.modeRGB, bs.modeAlpha);

    glBegin(GL_QUADS);
    glTexCoord2i(s_rect.x0,   s_rect.y0);   glVertex2f(d_rect.x0,   d_rect.y0);
    glTexCoord2i(s_rect.x1-1, s_rect.y0);   glVertex2f(d_rect.x1-1, d_rect.y0);
    glTexCoord2i(s_rect.x1-1, s_rect.y1-1); glVertex2f(d_rect.x1-1, d_rect.y1-1);
    glTexCoord2i(s_rect.x0,   s_rect.y1-1); glVertex2f(d_rect.x0,   d_rect.y1-1);
    glEnd();

    return VDP_STATUS_OK;
}

static
VdpStatus
softVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    traceVdpPreemptionCallbackRegister("{zilch}", device, callback, context);
    return VDP_STATUS_NO_IMPLEMENTATION;
}

static
VdpStatus
softVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                        VdpPresentationQueueTarget *target)
{
    traceVdpPresentationQueueTargetCreateX11("{part}", device, drawable, target);

    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *data =
        (VdpPresentationQueueTargetData *)calloc(1, sizeof(VdpPresentationQueueTargetData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_PRESENTATION_QUEUE_TARGET;
    data->device = deviceData;
    data->drawable = drawable;

    *target = handlestorage_add(data);

    return VDP_STATUS_OK;
}

// =========================
static
VdpStatus
softVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    traceVdpGetProcAddress("{full}", device, function_id, function_pointer);

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
    traceVdpDeviceCreateX11("{full}", display, screen, device, get_proc_address);

    if (NULL == display)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *data = (VdpDeviceData *)calloc(1, sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_DEVICE;
    data->display = display;
    data->screen = screen;

    // initialize OpenGL context
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vi;
    vi = glXChooseVisual(display, screen, att);
    if (NULL == vi) {
        traceTrace("error: glXChooseVisual\n");
        return VDP_STATUS_ERROR;
    }

    data->glc = glXCreateContext(display, vi, NULL, GL_TRUE);
    data->root = DefaultRootWindow(display);

    glXMakeCurrent(display, data->root, data->glc);
    glGenFramebuffers(1, &data->fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, data->fbo_id);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    *device = handlestorage_add(data);
    *get_proc_address = &softVdpGetProcAddress;

    return VDP_STATUS_OK;
}
