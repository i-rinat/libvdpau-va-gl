/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define _XOPEN_SOURCE
#define GL_GLEXT_PROTOTYPES
#include <assert.h>
#include <malloc.h>
#include <libswscale/swscale.h>
#include <string.h>
#include <va/va.h>
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include "bitstream.h"
#include "ctx-stack.h"
#include "h264-parse.h"
#include "reverse-constant.h"
#include "handle-storage.h"
#include "vdpau-trace.h"
#include "watermark.h"
#include "globals.h"
#include "shaders.h"


#define DESCRIBE(xparam, format)    fprintf(stderr, #xparam " = %" #format "\n", xparam)

static char const *
implemetation_description_string = "OpenGL/VAAPI/libswscale backend for VDPAU";


static
const char *
softVdpGetErrorString(VdpStatus status)
{
    return reverse_status(status);
}

VdpStatus
softVdpGetApiVersion(uint32_t *api_version)
{
    if (!api_version)
        return VDP_STATUS_INVALID_POINTER;
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported, uint32_t *max_width,
                                      uint32_t *max_height)
{
    VdpStatus err_code;
    if (!is_supported || !max_width || !max_height)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    switch (surface_rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
    case VDP_RGBA_FORMAT_R8G8B8A8:
    case VDP_RGBA_FORMAT_R10G10B10A2:
    case VDP_RGBA_FORMAT_B10G10R10A2:
    case VDP_RGBA_FORMAT_A8:
        *is_supported = 1;          // All these formats should be supported by OpenGL
        break;                      // implementation.
    default:
        *is_supported = 0;
        break;
    }

    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceQueryCapabilities): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    *max_width = max_texture_size;
    *max_height = max_texture_size;

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                      VdpRGBAFormat surface_rgba_format,
                                                      VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
                                                    VdpRGBAFormat surface_rgba_format,
                                                    VdpIndexedFormat bits_indexed_format,
                                                    VdpColorTableFormat color_table_format,
                                                    VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)bits_indexed_format; (void)color_table_format;
    (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpYCbCrFormat bits_ycbcr_format,
                                                  VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)bits_ycbcr_format; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                           uint32_t height, VdpOutputSurface *surface)
{
    VdpStatus err_code;
    if (!surface)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    //TODO: figure out reasonable limits
    if (width > 4096 || height > 4096) {
        err_code = VDP_STATUS_INVALID_SIZE;
        goto quit;
    }

    VdpOutputSurfaceData *data = calloc(1, sizeof(VdpOutputSurfaceData));
    if (NULL == data) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    switch (rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_BGRA;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_R8G8B8A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_RGBA;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_R10G10B10A2:
        data->gl_internal_format = GL_RGB10_A2;
        data->gl_format = GL_RGBA;
        data->gl_type = GL_UNSIGNED_INT_10_10_10_2;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_B10G10R10A2:
        data->gl_internal_format = GL_RGB10_A2;
        data->gl_format = GL_BGRA;
        data->gl_type = GL_UNSIGNED_INT_10_10_10_2;
        data->bytes_per_pixel = 4;
        break;
    case VDP_RGBA_FORMAT_A8:
        data->gl_internal_format = GL_RGBA;
        data->gl_format = GL_RED;
        data->gl_type = GL_UNSIGNED_BYTE;
        data->bytes_per_pixel = 1;
        break;
    default:
        traceError("error (VdpOutputSurfaceCreate): %s is not implemented\n",
                   reverse_rgba_format(rgba_format));
        free(data);
        err_code = VDP_STATUS_INVALID_RGBA_FORMAT;
        goto quit;
    }

    data->type = HANDLETYPE_OUTPUT_SURFACE;
    data->width = width;
    data->height = height;
    data->device = deviceData;
    data->rgba_format = rgba_format;

    glx_context_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // reserve texture
    glTexImage2D(GL_TEXTURE_2D, 0, data->gl_internal_format, width, height, 0, data->gl_format,
                 data->gl_type, NULL);

    glGenFramebuffers(1, &data->fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, data->fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->tex_id, 0);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != gl_status) {
        traceError("error (VdpOutputSurfaceCreate): "
                   "framebuffer not ready, %d, %s\n", gl_status, gluErrorString(gl_status));
        glx_context_pop();
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceCreate): gl error %d\n", gl_error);
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    deviceData->refcount ++;
    *surface = handle_insert(data);

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
softVdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    VdpStatus err_code;
    VdpOutputSurfaceData *data = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->device;

    glx_context_push_thread_local(deviceData);
    glDeleteTextures(1, &data->tex_id);
    glDeleteFramebuffers(1, &data->fbo_id);

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceDestroy): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    handle_expunge(surface);
    deviceData->refcount --;
    free(data);
    return VDP_STATUS_OK;

quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
softVdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
                                  uint32_t *width, uint32_t *height)
{
    if (!rgba_format || !width || !height)
        return VDP_STATUS_INVALID_POINTER;
    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;

    // TODO: check surfData validity again
    *rgba_format = surfData->rgba_format;
    *width       = surfData->width;
    *height      = surfData->height;

    handle_release(surface);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                                  void *const *destination_data,
                                  uint32_t const *destination_pitches)
{
    VdpStatus err_code;
    if (!destination_data || !destination_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpOutputSurfaceData *srcSurfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = srcSurfData->device;

    VdpRect srcRect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (source_rect)
        srcRect = *source_rect;

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, srcSurfData->fbo_id);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, destination_pitches[0] / srcSurfData->bytes_per_pixel);
    if (4 != srcSurfData->bytes_per_pixel)
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(srcRect.x0, srcRect.y0, srcRect.x1 - srcRect.x0, srcRect.y1 - srcRect.y0,
                 srcSurfData->gl_format, srcSurfData->gl_type, destination_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    if (4 != srcSurfData->bytes_per_pixel)
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceGetBitsNative): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
softVdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    VdpStatus err_code;
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpOutputSurfaceData *dstSurfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->device;

    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    glx_context_push_thread_local(deviceData);
    glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0] / dstSurfData->bytes_per_pixel);
    if (4 != dstSurfData->bytes_per_pixel)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, dstRect.x0, dstRect.y0,
                    dstRect.x1 - dstRect.x0, dstRect.y1 - dstRect.y0,
                    dstSurfData->gl_format, dstSurfData->gl_type, source_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    if (4 != dstSurfData->bytes_per_pixel)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfacePutBitsNative): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
softVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
                                   void const *const *source_data, uint32_t const *source_pitch,
                                   VdpRect const *destination_rect,
                                   VdpColorTableFormat color_table_format, void const *color_table)
{
    VdpStatus err_code;
    if (!source_data || !source_pitch || !color_table)
        return VDP_STATUS_INVALID_POINTER;
    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = surfData->device;

    VdpRect dstRect = {0, 0, surfData->width, surfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    // there is no other formats anyway
    if (VDP_COLOR_TABLE_FORMAT_B8G8R8X8 != color_table_format) {
        err_code = VDP_STATUS_INVALID_COLOR_TABLE_FORMAT;
        goto quit;
    }
    const uint32_t *color_table32 = color_table;

    glx_context_push_thread_local(deviceData);

    switch (source_indexed_format) {
    case VDP_INDEXED_FORMAT_I8A8:
        // TODO: use shader?
        do {
            const uint32_t dstRectWidth = dstRect.x1 - dstRect.x0;
            const uint32_t dstRectHeight = dstRect.y1 - dstRect.y0;
            uint32_t *unpacked_buf = malloc(4 * dstRectWidth * dstRectHeight);
            if (NULL == unpacked_buf) {
                err_code = VDP_STATUS_RESOURCES;
                goto quit;
            }

            for (unsigned int y = 0; y < dstRectHeight; y ++) {
                const uint8_t *src_ptr = source_data[0];
                src_ptr += y * source_pitch[0];
                uint32_t *dst_ptr = unpacked_buf + y * dstRectWidth;
                for (unsigned int x = 0; x < dstRectWidth; x ++) {
                    const uint8_t i = *src_ptr++;
                    const uint32_t a = (*src_ptr++) << 24;
                    dst_ptr[x] = (color_table32[i] & 0x00ffffff) + a;
                }
            }

            glBindTexture(GL_TEXTURE_2D, surfData->tex_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, dstRect.x0, dstRect.y0,
                            dstRect.x1 - dstRect.x0, dstRect.y1 - dstRect.y0,
                            GL_BGRA, GL_UNSIGNED_BYTE, unpacked_buf);
            glFinish();
            free(unpacked_buf);

            GLenum gl_error = glGetError();
            glx_context_pop();
            if (GL_NO_ERROR != gl_error) {
                traceError("error (VdpOutputSurfacePutBitsIndexed): gl error %d\n", gl_error);
                err_code = VDP_STATUS_ERROR;
                goto quit;
            }

            err_code = VDP_STATUS_OK;
            goto quit;
        } while (0);
        break;
    default:
        traceError("error (VdpOutputSurfacePutBitsIndexed): unsupported indexed format %s\n",
                   reverse_indexed_format(source_indexed_format));
        err_code = VDP_STATUS_INVALID_INDEXED_FORMAT;
        goto quit;
    }

quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
softVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                 void const *const *source_data, uint32_t const *source_pitches,
                                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    (void)surface; (void)source_ycbcr_format; (void)source_data; (void)source_pitches;
    (void)destination_rect; (void)csc_matrix;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                     VdpBool *is_supported)
{
    (void)device; (void)feature; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                       VdpBool *is_supported)
{
    (void)device; (void)parameter; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                       VdpBool *is_supported)
{
    (void)device; (void)attribute; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                          void *min_value, void *max_value)
{
    (void)device; (void)parameter; (void)min_value; (void)max_value;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                          void *min_value, void *max_value)
{
    (void)device; (void)attribute; (void)min_value; (void)max_value;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                        VdpVideoMixerFeature const *features, uint32_t parameter_count,
                        VdpVideoMixerParameter const *parameters,
                        void const *const *parameter_values, VdpVideoMixer *mixer)
{
    VdpStatus err_code;
    if (!mixer)
        return VDP_STATUS_INVALID_POINTER;
    (void)feature_count; (void)features;    // TODO: mixer features
    (void)parameter_count; (void)parameters; (void)parameter_values;    // TODO: mixer parameters
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoMixerData *data = calloc(1, sizeof(VdpVideoMixerData));
    if (NULL == data) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    data->type = HANDLETYPE_VIDEO_MIXER;
    data->device = deviceData;

    deviceData->refcount ++;
    *mixer = handle_insert(data);

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
softVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features,
                                   VdpBool const *feature_enables)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_enables;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void const *const *attribute_values)
{
    (void)mixer; (void)attribute_count; (void)attributes; (void)attribute_values;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_supports;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                                   VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_enables;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                    VdpVideoMixerParameter const *parameters,
                                    void *const *parameter_values)
{
    (void)mixer; (void)parameter_count; (void)parameters; (void)parameter_values;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                    VdpVideoMixerAttribute const *attributes,
                                    void *const *attribute_values)
{
    (void)mixer; (void)attribute_count; (void)attributes; (void)attribute_values;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
softVdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    VdpVideoMixerData *videoMixerData = handle_acquire(mixer, HANDLETYPE_VIDEO_MIXER);
    if (NULL == videoMixerData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoMixerData->device;

    deviceData->refcount --;
    handle_expunge(mixer);
    free(videoMixerData);
    return VDP_STATUS_OK;
}

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
    VdpStatus err_code;
    (void)mixer;    // TODO: mixer should be used to get mixing parameters
    // TODO: current implementation ignores previous and future surfaces, using only current.
    // Is that acceptable for interlaced video? Will VAAPI handle deinterlacing?

    (void)background_surface;   // TODO: background_surface. Is it safe to just ignore it?
    (void)background_source_rect;
    (void)current_picture_structure;
    (void)video_surface_past_count; (void)video_surface_past;
    (void)video_surface_future_count; (void)video_surface_future;
    (void)layer_count; (void)layers;

    VdpVideoSurfaceData *srcSurfData =
        handle_acquire(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    VdpOutputSurfaceData *dstSurfData =
        handle_acquire(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData || NULL == dstSurfData) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }
    if (srcSurfData->device != dstSurfData->device) {
        err_code = VDP_STATUS_HANDLE_DEVICE_MISMATCH;
        goto quit;
    }
    VdpDeviceData *deviceData = srcSurfData->device;

    VdpRect srcVideoRect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (video_source_rect)
        srcVideoRect = *video_source_rect;

    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    VdpRect dstVideoRect = srcVideoRect;
    if (destination_video_rect)
        dstVideoRect = *destination_video_rect;

    // TODO: dstRect should clip dstVideoRect

    glx_context_push_thread_local(deviceData);

    if (srcSurfData->sync_va_to_glx) {
        VAStatus status;
        if (NULL == srcSurfData->va_glx) {
            status = vaCreateSurfaceGLX(deviceData->va_dpy, GL_TEXTURE_2D, srcSurfData->tex_id,
                                        &srcSurfData->va_glx);
            if (VA_STATUS_SUCCESS != status) {
                glx_context_pop();
                err_code = VDP_STATUS_ERROR;
                goto quit;
            }
        }

        status = vaCopySurfaceGLX(deviceData->va_dpy, srcSurfData->va_glx, srcSurfData->va_surf, 0);
        // TODO: check result of previous call
        srcSurfData->sync_va_to_glx = 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
    glViewport(0, 0, dstSurfData->width, dstSurfData->height);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/srcSurfData->width, 1.0f/srcSurfData->height, 1.0f);

    // Clear dstRect area
    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 1);
    glBegin(GL_QUADS);
        glVertex2f(dstRect.x0, dstRect.y0);
        glVertex2f(dstRect.x1, dstRect.y0);
        glVertex2f(dstRect.x1, dstRect.y1);
        glVertex2f(dstRect.x0, dstRect.y1);
    glEnd();

    // Render (maybe scaled) data from video surface
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        glTexCoord2i(srcVideoRect.x0, srcVideoRect.y0);
        glVertex2f(dstVideoRect.x0, dstVideoRect.y0);

        glTexCoord2i(srcVideoRect.x1, srcVideoRect.y0);
        glVertex2f(dstVideoRect.x1, dstVideoRect.y0);

        glTexCoord2i(srcVideoRect.x1, srcVideoRect.y1);
        glVertex2f(dstVideoRect.x1, dstVideoRect.y1);

        glTexCoord2i(srcVideoRect.x0, srcVideoRect.y1);
        glVertex2f(dstVideoRect.x0, dstVideoRect.y1);
    glEnd();
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoMixerRender): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(video_surface_current);
    handle_release(destination_surface);
    return err_code;
}

VdpStatus
softVdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpBool *is_supported, uint32_t *max_width,
                                     uint32_t *max_height)
{
    if (!is_supported || !max_width || !max_height)
        return VDP_STATUS_INVALID_POINTER;
    (void)device; (void)surface_chroma_type;
    // TODO: implement
    *is_supported = 1;
    *max_width = 1920;
    *max_height = 1080;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device,
                                                    VdpChromaType surface_chroma_type,
                                                    VdpYCbCrFormat bits_ycbcr_format,
                                                    VdpBool *is_supported)
{
    if (!is_supported)
        return VDP_STATUS_INVALID_POINTER;
    (void)device; (void)surface_chroma_type; (void)bits_ycbcr_format;
    // TODO: implement
    *is_supported = 1;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width,
                          uint32_t height, VdpVideoSurface *surface)
{
    VdpStatus err_code;
    if (!surface)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoSurfaceData *data = calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    data->type = HANDLETYPE_VIDEO_SURFACE;
    data->device = deviceData;
    data->chroma_type = chroma_type;
    data->width = width;
    data->height = height;
    data->va_surf = VA_INVALID_SURFACE;
    data->va_glx = NULL;
    data->tex_id = 0;
    data->sync_va_to_glx = 0;
    data->decoder = VDP_INVALID_HANDLE;

    glx_context_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data->width, data->height, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, NULL);

    glGenFramebuffers(1, &data->fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, data->fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->tex_id, 0);
    GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (GL_FRAMEBUFFER_COMPLETE != gl_status) {
        traceError("error (%s): framebuffer not ready, %d, %s\n", __func__, gl_status,
                   gluErrorString(gl_status));
        glx_context_pop();
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfaceCreate): gl error %d\n", gl_error);
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    // no VA surface creation here. Actual pool of VA surfaces should be allocated already
    // by VdpDecoderCreate. VdpDecoderCreate will update ->va_surf field as needed.

    deviceData->refcount ++;
    *surface = handle_insert(data);

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
softVdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    VdpVideoSurfaceData *videoSurfData = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == videoSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoSurfData->device;

    glx_context_push_thread_local(deviceData);
    glDeleteTextures(1, &videoSurfData->tex_id);

    GLenum gl_error = glGetError();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfaceDestroy): gl error %d\n", gl_error);
        glx_context_pop();
        handle_release(surface);
        return VDP_STATUS_ERROR;
    }

    if (videoSurfData->va_glx) {
        vaDestroySurfaceGLX(deviceData->va_dpy, videoSurfData->va_glx);
    }

    if (deviceData->va_available) {
        // return VA surface to the free list
        if (videoSurfData->decoder != VDP_INVALID_HANDLE) {
            VdpDecoderData *dd = handle_acquire(videoSurfData->decoder, HANDLETYPE_DECODER);
            if (NULL != dd) {
                free_list_push(dd->free_list, &dd->free_list_head, videoSurfData->rt_idx);
                handle_release(videoSurfData->decoder);
            }
        }
        // .va_surf will be freed in VdpDecoderDestroy
    }

    glx_context_pop();
    deviceData->refcount --;
    handle_expunge(surface);
    free(videoSurfData);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
                                 uint32_t *width, uint32_t *height)
{
    if (!chroma_type || !width || !height)
        return VDP_STATUS_INVALID_POINTER;
    VdpVideoSurfaceData *videoSurf = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == videoSurf)
        return VDP_STATUS_INVALID_HANDLE;

    *chroma_type = videoSurf->chroma_type;
    *width       = videoSurf->width;
    *height      = videoSurf->height;

    handle_release(surface);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                                void *const *destination_data, uint32_t const *destination_pitches)
{
    VdpStatus err_code;
    if (!destination_data || !destination_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpVideoSurfaceData *srcSurfData = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = srcSurfData->device;
    VADisplay va_dpy = deviceData->va_dpy;

    if (deviceData->va_available) {
        VAImage q;
        vaDeriveImage(va_dpy, srcSurfData->va_surf, &q);
        if (VA_FOURCC('N', 'V', '1', '2') == q.format.fourcc &&
            VDP_YCBCR_FORMAT_NV12 == destination_ycbcr_format)
        {
            uint8_t *img_data;
            vaMapBuffer(va_dpy, q.buf, (void **)&img_data);
            if (destination_pitches[0] == q.pitches[0] &&
                destination_pitches[1] == q.pitches[1])
            {
                memcpy(destination_data[0], img_data + q.offsets[0], q.width * q.height);
                memcpy(destination_data[1], img_data + q.offsets[1], q.width * q.height / 2);
            } else {
                uint8_t *src = img_data + q.offsets[0];
                uint8_t *dst = destination_data[0];
                for (unsigned int y = 0; y < q.height; y ++) {  // Y plane
                    memcpy (dst, src, q.width);
                    src += q.pitches[0];
                    dst += destination_pitches[0];
                }
                src = img_data + q.offsets[1];
                dst = destination_data[1];
                for (unsigned int y = 0; y < q.height / 2; y ++) {  // UV plane
                    memcpy(dst, src, q.width);  // q.width/2 samples of U and V each, hence q.width
                    src += q.pitches[1];
                    dst += destination_pitches[1];
                }
            }
            vaUnmapBuffer(va_dpy, q.buf);
        } else if (VA_FOURCC('N', 'V', '1', '2') == q.format.fourcc &&
                   VDP_YCBCR_FORMAT_YV12 == destination_ycbcr_format)
        {
            uint8_t *img_data;
            vaMapBuffer(va_dpy, q.buf, (void **)&img_data);

            // Y plane
            if (destination_pitches[0] == q.pitches[0]) {
                memcpy(destination_data[0], img_data + q.offsets[0], q.width * q.height);
            } else {
                uint8_t *src = img_data + q.offsets[0];
                uint8_t *dst = destination_data[0];
                for (unsigned int y = 0; y < q.height; y ++) {
                    memcpy (dst, src, q.width);
                    src += q.pitches[0];
                    dst += destination_pitches[0];
                }
            }

            // unpack mixed UV to separate planes
            for (unsigned int y = 0; y < q.height/2; y ++) {
                uint8_t *src = img_data + q.offsets[1] + y * q.pitches[1];
                uint8_t *dst_u = destination_data[1] + y * destination_pitches[1];
                uint8_t *dst_v = destination_data[2] + y * destination_pitches[2];

                for (unsigned int x = 0; x < q.width/2; x++) {
                    *dst_v++ = *src++;
                    *dst_u++ = *src++;
                }
            }

            vaUnmapBuffer(va_dpy, q.buf);
        } else {
            const char *c = (const char *)&q.format.fourcc;
            traceError("error (softVdpVideoSurfaceGetBitsYCbCr): not implemented conversion "
                       "VA FOURCC %c%c%c%c -> %s\n", *c, *(c+1), *(c+2), *(c+3),
                       reverse_ycbcr_format(destination_ycbcr_format));
            vaDestroyImage(va_dpy, q.image_id);
            err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
            goto quit;
        }
        vaDestroyImage(va_dpy, q.image_id);
    } else {
        // software fallback
        traceError("error (softVdpVideoSurfaceGetBitsYCbCr): not implemented software fallback\n");
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfaceGetBitsYCbCr): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
softVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                void const *const *source_data, uint32_t const *source_pitches)
{
    VdpStatus err_code;
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;
    // TODO: implement VDP_YCBCR_FORMAT_UYVY
    // TODO: implement VDP_YCBCR_FORMAT_YUYV
    // TODO: implement VDP_YCBCR_FORMAT_Y8U8V8A8
    // TODO: implement VDP_YCBCR_FORMAT_V8U8Y8A8

    VdpVideoSurfaceData *dstSurfData = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->device;

    switch (source_ycbcr_format) {
    case VDP_YCBCR_FORMAT_NV12:
    case VDP_YCBCR_FORMAT_YV12:
        /* do nothing */
        break;
    case VDP_YCBCR_FORMAT_UYVY:
    case VDP_YCBCR_FORMAT_YUYV:
    case VDP_YCBCR_FORMAT_Y8U8V8A8:
    case VDP_YCBCR_FORMAT_V8U8Y8A8:
    default:
        traceError("error (%s): not implemented source YCbCr format '%s'\n", __func__,
                   reverse_ycbcr_format(source_ycbcr_format));
        err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
        goto err;
    }

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);

    GLuint tex_id[2];
    glGenTextures(2, tex_id);
    glEnable(GL_TEXTURE_2D);

    switch (source_ycbcr_format) {
    case VDP_YCBCR_FORMAT_NV12:
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_id[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // UV plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dstSurfData->width/2, dstSurfData->height/2, 0,
                     GL_RG, GL_UNSIGNED_BYTE, source_data[1]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // Y plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dstSurfData->width, dstSurfData->height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, source_data[0]);
        break;
    case VDP_YCBCR_FORMAT_YV12:
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_id[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dstSurfData->width/2, dstSurfData->height, 0,
                     GL_RED, GL_UNSIGNED_BYTE, NULL);
        // U plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[2]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, dstSurfData->width/2, dstSurfData->height/2, GL_RED,
                        GL_UNSIGNED_BYTE, source_data[2]);
        // V plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[1]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, dstSurfData->height/2, dstSurfData->width/2,
                        dstSurfData->height/2, GL_RED, GL_UNSIGNED_BYTE, source_data[1]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // Y plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dstSurfData->width, dstSurfData->height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, source_data[0]);
        break;
    case VDP_YCBCR_FORMAT_UYVY:
    case VDP_YCBCR_FORMAT_YUYV:
    case VDP_YCBCR_FORMAT_Y8U8V8A8:
    case VDP_YCBCR_FORMAT_V8U8Y8A8:
    default:
        /* never reached */
        break;
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
    glViewport(0, 0, dstSurfData->width, dstSurfData->height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    switch (source_ycbcr_format) {
    case VDP_YCBCR_FORMAT_NV12:
        glUseProgram(glsl_shaders[glsl_NV12_RGBA].program);
        glUniform1i(glsl_shaders[glsl_NV12_RGBA].uniform.tex_0, 0);
        glUniform1i(glsl_shaders[glsl_NV12_RGBA].uniform.tex_1, 1);
        break;
    case VDP_YCBCR_FORMAT_YV12:
        glUseProgram(glsl_shaders[glsl_YV12_RGBA].program);
        glUniform1i(glsl_shaders[glsl_YV12_RGBA].uniform.tex_0, 0);
        glUniform1i(glsl_shaders[glsl_YV12_RGBA].uniform.tex_1, 1);
        break;
    case VDP_YCBCR_FORMAT_UYVY:
    case VDP_YCBCR_FORMAT_YUYV:
    case VDP_YCBCR_FORMAT_Y8U8V8A8:
    case VDP_YCBCR_FORMAT_V8U8Y8A8:
    default:
        /* do nothing */
        break;
    }

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(dstSurfData->width, 0);
        glTexCoord2f(1, 1); glVertex2f(dstSurfData->width, dstSurfData->height);
        glTexCoord2f(0, 1); glVertex2f(0, dstSurfData->height);
    glEnd();

    glUseProgram(0);
    glFinish();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(2, tex_id);

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpVideoSurfacePutBitsYCbCr): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto err;
    }

    err_code = VDP_STATUS_OK;
err:
    handle_release(surface);
    return err_code;
}

void
print_handle_type(int handle, void *item, void *p)
{
    VdpGenericHandle *gh = item;
    struct {
        int cnt;
        int total_cnt;
        VdpDeviceData *deviceData;
    } *pp = p;
    pp->total_cnt ++;

    if (gh) {
        if (pp->deviceData == gh->parent) {
            traceError("handle %d type = %d\n", handle, gh->type);
            pp->cnt ++;
        }
    }
}

static
void
destroy_child_objects(int handle, void *item, void *p)
{
    const void *parent = p;
    VdpGenericHandle *gh = item;
    if (gh) {
        if (parent == gh->parent) {
            switch (gh->type) {
            case HANDLETYPE_DEVICE:
                // do nothing
                break;
            case HANDLETYPE_PRESENTATION_QUEUE_TARGET:
                softVdpPresentationQueueDestroy(handle);
                break;
            case HANDLETYPE_PRESENTATION_QUEUE:
                softVdpPresentationQueueDestroy(handle);
                break;
            case HANDLETYPE_VIDEO_MIXER:
                softVdpVideoMixerDestroy(handle);
                break;
            case HANDLETYPE_OUTPUT_SURFACE:
                softVdpOutputSurfaceDestroy(handle);
                break;
            case HANDLETYPE_VIDEO_SURFACE:
                softVdpVideoSurfaceDestroy(handle);
                break;
            case HANDLETYPE_BITMAP_SURFACE:
                softVdpBitmapSurfaceDestroy(handle);
                break;
            case HANDLETYPE_DECODER:
                softVdpDecoderDestroy(handle);
                break;
            default:
                traceError("warning (destroy_child_objects): unknown handle type %d\n", gh->type);
                break;
            }
        }
    }
}

VdpStatus
softVdpDeviceDestroy(VdpDevice device)
{
    VdpStatus err_code;
    VdpDeviceData *data = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    if (0 != data->refcount) {
        // Buggy client forgot to destroy dependend objects or decided that destroying
        // VdpDevice destroys all child object. Let's try to mitigate and prevent leakage.
        traceError("warning (softVdpDeviceDestroy): non-zero reference count (%d). "
                   "Trying to free child objects.\n", data->refcount);
        void *parent_object = data;
        handle_execute_for_all(destroy_child_objects, parent_object);
    }

    if (0 != data->refcount) {
        traceError("error (softVdpDeviceDestroy): still non-zero reference count (%d)\n",
                   data->refcount);
        traceError("Here is the list of objects:\n");
        struct {
            int cnt;
            int total_cnt;
            VdpDeviceData *deviceData;
        } state = { .cnt = 0, .total_cnt = 0, .deviceData = data };

        handle_execute_for_all(print_handle_type, &state);
        traceError("Objects leaked: %d\n", state.cnt);
        traceError("Objects visited during scan: %d\n", state.total_cnt);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    // cleaup libva
    if (data->va_available)
        vaTerminate(data->va_dpy);

    glx_context_push_thread_local(data);
    glDeleteTextures(1, &data->watermark_tex_id);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glx_context_pop();

    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    glXMakeCurrent(data->display, None, NULL);
    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);

    glx_context_unref_glc_hash_table(data->display);

    handle_xdpy_unref(data->display_orig);
    handle_expunge(device);
    free(data);

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpDeviceDestroy): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit_skip_release;
    }

    return VDP_STATUS_OK;

quit:
    handle_release(device);
quit_skip_release:
    return err_code;
}

VdpStatus
softVdpGetInformationString(char const **information_string)
{
    if (!information_string)
        return VDP_STATUS_INVALID_POINTER;
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    if (!csc_matrix)
        return VDP_STATUS_INVALID_POINTER;
    if (procamp && VDP_PROCAMP_VERSION != procamp->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    // TODO: do correct matricies calculation
    VdpCSCMatrix *m = csc_matrix;
    switch (standard) {
    case VDP_COLOR_STANDARD_ITUR_BT_601:
        (*m)[0][0] = 1.164f; (*m)[0][1] =  0.0f;   (*m)[0][2] =  1.596f; (*m)[0][3] = -222.9f;
        (*m)[1][0] = 1.164f; (*m)[1][1] = -0.392f; (*m)[1][2] = -0.813f; (*m)[1][3] =  135.6f;
        (*m)[2][0] = 1.164f; (*m)[2][1] =  2.017f; (*m)[2][2] =  0.0f;   (*m)[2][3] = -276.8f;
        break;
    case VDP_COLOR_STANDARD_ITUR_BT_709:
        (*m)[0][0] =  1.0f; (*m)[0][1] =  0.0f;   (*m)[0][2] =  1.402f; (*m)[0][3] = -179.4f;
        (*m)[1][0] =  1.0f; (*m)[1][1] = -0.344f; (*m)[1][2] = -0.714f; (*m)[1][3] =  135.5f;
        (*m)[2][0] =  1.0f; (*m)[2][1] =  1.772f; (*m)[2][2] =  0.0f;   (*m)[2][3] = -226.8f;
        break;
    case VDP_COLOR_STANDARD_SMPTE_240M:
        (*m)[0][0] =  0.581f; (*m)[0][1] = -0.764f; (*m)[0][2] =  1.576f; (*m)[0][3] = 0.0f;
        (*m)[1][0] =  0.581f; (*m)[1][1] = -0.991f; (*m)[1][2] = -0.477f; (*m)[1][3] = 0.0f;
        (*m)[2][0] =  0.581f; (*m)[2][1] =  1.062f; (*m)[2][2] =  0.000f; (*m)[2][3] = 0.0f;
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

    // it's ok to pass NULL as blend_state
    if (blend_state) {
        bs.srcFuncRGB = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_source_color);
        bs.srcFuncAlpha = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_source_alpha);
        bs.dstFuncRGB = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_destination_color);
        bs.dstFuncAlpha = vdpBlendFuncToGLBlendFunc(blend_state->blend_factor_destination_alpha);
    } else {
        bs.srcFuncRGB = bs.srcFuncAlpha = GL_ONE;
        bs.dstFuncRGB = bs.dstFuncAlpha = GL_ZERO;
    }

    if (GL_INVALID_VALUE == bs.srcFuncRGB || GL_INVALID_VALUE == bs.srcFuncAlpha ||
        GL_INVALID_VALUE == bs.dstFuncRGB || GL_INVALID_VALUE == bs.dstFuncAlpha)
    {
        bs.invalid_func = 1;
    }

    if (blend_state) {
        bs.modeRGB = vdpBlendEquationToGLEquation(blend_state->blend_equation_color);
        bs.modeAlpha = vdpBlendEquationToGLEquation(blend_state->blend_equation_alpha);
    } else {
        bs.modeRGB = bs.modeAlpha = GL_FUNC_ADD;
    }
    if (GL_INVALID_VALUE == bs.modeRGB || GL_INVALID_VALUE == bs.modeAlpha)
        bs.invalid_eq = 1;

    return bs;
}

static
void
compose_surfaces(struct blend_state_struct bs, VdpRect srcRect, VdpRect dstRect,
                 VdpColor const *colors, int flags, int has_src_surf)
{
    glBlendFuncSeparate(bs.srcFuncRGB, bs.dstFuncRGB, bs.srcFuncAlpha, bs.dstFuncAlpha);
    glBlendEquationSeparate(bs.modeRGB, bs.modeAlpha);

    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        if (has_src_surf) {
            switch (flags & 3) {
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_0:
                glTexCoord2i(srcRect.x0, srcRect.y0); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_90:
                glTexCoord2i(srcRect.x0, srcRect.y1); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_180:
                glTexCoord2i(srcRect.x1, srcRect.y1); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_270:
                glTexCoord2i(srcRect.x1, srcRect.y0); break;
            }
        }
        if (colors)
            glColor4f(colors[0].red, colors[0].green, colors[0].blue, colors[0].alpha);
        glVertex2f(dstRect.x0, dstRect.y0);

        if (has_src_surf) {
            switch (flags & 3) {
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_0:
                glTexCoord2i(srcRect.x1, srcRect.y0); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_90:
                glTexCoord2i(srcRect.x0, srcRect.y0); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_180:
                glTexCoord2i(srcRect.x0, srcRect.y1); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_270:
                glTexCoord2i(srcRect.x1, srcRect.y1); break;
            }
        }
        if (colors && (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX))
            glColor4f(colors[1].red, colors[1].green, colors[1].blue, colors[1].alpha);
        glVertex2f(dstRect.x1, dstRect.y0);

        if (has_src_surf) {
            switch (flags & 3) {
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_0:
                glTexCoord2i(srcRect.x1, srcRect.y1); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_90:
                glTexCoord2i(srcRect.x1, srcRect.y0); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_180:
                glTexCoord2i(srcRect.x0, srcRect.y0); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_270:
                glTexCoord2i(srcRect.x0, srcRect.y1); break;
            }
        }
        if (colors && (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX))
            glColor4f(colors[2].red, colors[2].green, colors[2].blue, colors[2].alpha);
        glVertex2f(dstRect.x1, dstRect.y1);

        if (has_src_surf) {
            switch (flags & 3) {
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_0:
                glTexCoord2i(srcRect.x0, srcRect.y1); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_90:
                glTexCoord2i(srcRect.x1, srcRect.y1); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_180:
                glTexCoord2i(srcRect.x1, srcRect.y0); break;
            case VDP_OUTPUT_SURFACE_RENDER_ROTATE_270:
                glTexCoord2i(srcRect.x0, srcRect.y0); break;
            }
        }
        if (colors && (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX))
            glColor4f(colors[3].red, colors[3].green, colors[3].blue, colors[3].alpha);
        glVertex2f(dstRect.x0, dstRect.y1);
    glEnd();
    glColor4f(1, 1, 1, 1);
}

VdpStatus
softVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpOutputSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    VdpStatus err_code;

    if (blend_state) {
        if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version) {
            err_code = VDP_STATUS_INVALID_VALUE;
            goto quit_skip_release;
        }
    }

    VdpOutputSurfaceData *dstSurfData =
        handle_acquire(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpOutputSurfaceData *srcSurfData = handle_acquire(source_surface, HANDLETYPE_OUTPUT_SURFACE);

    if (NULL == dstSurfData) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }
    if (srcSurfData && srcSurfData->device != dstSurfData->device) {
        err_code = VDP_STATUS_HANDLE_DEVICE_MISMATCH;
        goto quit;
    }
    VdpDeviceData *deviceData = dstSurfData->device;

    VdpRect s_rect = {0, 0, 0, 0};
    VdpRect d_rect = {0, 0, dstSurfData->width, dstSurfData->height};
    s_rect.x1 = srcSurfData ? srcSurfData->width : 1;
    s_rect.y1 = srcSurfData ? srcSurfData->height : 1;

    if (source_rect)
        s_rect = *source_rect;
    if (destination_rect)
        d_rect = *destination_rect;

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);
    if (bs.invalid_func) {
        err_code = VDP_STATUS_INVALID_BLEND_FACTOR;
        goto quit;
    }
    if (bs.invalid_eq) {
        err_code = VDP_STATUS_INVALID_BLEND_EQUATION;
        goto quit;
    }

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
    glViewport(0, 0, dstSurfData->width, dstSurfData->height);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (srcSurfData) {
        glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f/srcSurfData->width, 1.0f/srcSurfData->height, 1.0f);
    }

    compose_surfaces(bs, s_rect, d_rect, colors, flags, !!srcSurfData);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceRenderOutputSurface): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(source_surface);
    handle_release(destination_surface);
quit_skip_release:
    return err_code;
}

VdpStatus
softVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                        VdpRect const *destination_rect,
                                        VdpBitmapSurface source_surface, VdpRect const *source_rect,
                                        VdpColor const *colors,
                                        VdpOutputSurfaceRenderBlendState const *blend_state,
                                        uint32_t flags)
{
    VdpStatus err_code;

    if (blend_state) {
        if (VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION != blend_state->struct_version) {
            err_code = VDP_STATUS_INVALID_VALUE;
            goto quit_skip_release;
        }
    }

    VdpOutputSurfaceData *dstSurfData =
        handle_acquire(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpBitmapSurfaceData *srcSurfData = handle_acquire(source_surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }
    if (srcSurfData && srcSurfData->device != dstSurfData->device) {
        err_code = VDP_STATUS_HANDLE_DEVICE_MISMATCH;
        goto quit;
    }
    VdpDeviceData *deviceData = dstSurfData->device;

    VdpRect s_rect = {0, 0, 0, 0};
    VdpRect d_rect = {0, 0, dstSurfData->width, dstSurfData->height};
    s_rect.x1 = srcSurfData ? srcSurfData->width : 1;
    s_rect.y1 = srcSurfData ? srcSurfData->height : 1;

    if (source_rect)
        s_rect = *source_rect;
    if (destination_rect)
        d_rect = *destination_rect;

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);
    if (bs.invalid_func) {
        err_code = VDP_STATUS_INVALID_BLEND_FACTOR;
        goto quit;
    }
    if (bs.invalid_eq) {
        err_code = VDP_STATUS_INVALID_BLEND_EQUATION;
        goto quit;
    }

    glx_context_push_thread_local(deviceData);
    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
    glViewport(0, 0, dstSurfData->width, dstSurfData->height);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (srcSurfData) {
        glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);
        if (srcSurfData->dirty) {
            if (4 != srcSurfData->bytes_per_pixel)
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcSurfData->width, srcSurfData->height,
                            srcSurfData->gl_format, srcSurfData->gl_type, srcSurfData->bitmap_data);
            if (4 != srcSurfData->bytes_per_pixel)
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            srcSurfData->dirty = 0;
        }

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f/srcSurfData->width, 1.0f/srcSurfData->height, 1.0f);
    }

    compose_surfaces(bs, s_rect, d_rect, colors, flags, !!srcSurfData);
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpOutputSurfaceRenderBitmapSurface): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(source_surface);
    handle_release(destination_surface);
quit_skip_release:
    return err_code;
}

VdpStatus
softVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    (void)device; (void)callback; (void)context;
    return VDP_STATUS_OK;
}


// =========================
VdpStatus
softVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    (void)device;   // there is no difference between various devices. All have same procedures
    if (!function_pointer)
        return VDP_STATUS_INVALID_POINTER;
    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = &softVdpGetErrorString;
        break;
    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = &softVdpGetProcAddress;
        break;
    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = &traceVdpGetApiVersion;
        break;
    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = &traceVdpGetInformationString;
        break;
    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = &traceVdpDeviceDestroy;
        break;
    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = &traceVdpGenerateCSCMatrix;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpVideoSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = &traceVdpVideoSurfaceCreate;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = &traceVdpVideoSurfaceDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = &traceVdpVideoSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = &traceVdpVideoSurfaceGetBitsYCbCr;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &traceVdpVideoSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = &traceVdpOutputSurfaceCreate;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = &traceVdpOutputSurfaceDestroy;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = &traceVdpOutputSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = &traceVdpOutputSurfaceGetBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &traceVdpOutputSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = &traceVdpOutputSurfacePutBitsIndexed;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &traceVdpOutputSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpBitmapSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = &traceVdpBitmapSurfaceCreate;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = &traceVdpBitmapSurfaceDestroy;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = &traceVdpBitmapSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &traceVdpBitmapSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = &traceVdpOutputSurfaceRenderOutputSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = &traceVdpOutputSurfaceRenderBitmapSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // *function_pointer = &traceVdpOutputSurfaceRenderVideoSurfaceLuma;
        *function_pointer = NULL;
        break;
    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpDecoderQueryCapabilities;
        break;
    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = &traceVdpDecoderCreate;
        break;
    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = &traceVdpDecoderDestroy;
        break;
    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = &traceVdpDecoderGetParameters;
        break;
    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = &traceVdpDecoderRender;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = &traceVdpVideoMixerQueryFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = &traceVdpVideoMixerQueryParameterSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = &traceVdpVideoMixerQueryAttributeSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = &traceVdpVideoMixerQueryParameterValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = &traceVdpVideoMixerQueryAttributeValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = &traceVdpVideoMixerCreate;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = &traceVdpVideoMixerSetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = &traceVdpVideoMixerSetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = &traceVdpVideoMixerGetFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = &traceVdpVideoMixerGetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = &traceVdpVideoMixerGetParameterValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = &traceVdpVideoMixerGetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = &traceVdpVideoMixerDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = &traceVdpVideoMixerRender;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = &traceVdpPresentationQueueTargetDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = &traceVdpPresentationQueueCreate;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = &traceVdpPresentationQueueDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = &traceVdpPresentationQueueSetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = &traceVdpPresentationQueueGetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = &traceVdpPresentationQueueGetTime;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = &traceVdpPresentationQueueDisplay;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = &traceVdpPresentationQueueBlockUntilSurfaceIdle;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = &traceVdpPresentationQueueQuerySurfaceStatus;
        break;
    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = &traceVdpPreemptionCallbackRegister;
        break;
    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = &traceVdpPresentationQueueTargetCreateX11;
        break;
    default:
        *function_pointer = NULL;
        break;
    } // switch

    if (NULL == *function_pointer)
        return VDP_STATUS_INVALID_FUNC_ID;
    return VDP_STATUS_OK;
}

static
VdpStatus
compile_shaders(void)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    const int shader_count = sizeof(glsl_shaders)/sizeof(glsl_shaders[0]);
    VdpStatus retval = VDP_STATUS_ERROR;

    pthread_mutex_lock(&lock);
    for (int k = 0; k < shader_count; k ++) {
        struct shader_s *s = &glsl_shaders[k];
        if (!s->program) {
            GLint errmsg_len;
            int ok;

            s->f_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(s->f_shader, 1, &s->body, &s->len);
            glCompileShader(s->f_shader);
            glGetShaderiv(s->f_shader, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                glGetShaderiv(s->f_shader, GL_INFO_LOG_LENGTH, &errmsg_len);
                char *errmsg = malloc(errmsg_len);
                glGetShaderInfoLog(s->f_shader, errmsg_len, NULL, errmsg);
                traceError("error (%s): compilation of shader #%d failed with '%s'\n", __func__, k,
                           errmsg);
                free(errmsg);
                glDeleteShader(s->f_shader);
                goto err;
            }

            s->program = glCreateProgram();
            glAttachShader(s->program, s->f_shader);
            glLinkProgram(s->program);
            glGetProgramiv(s->program, GL_LINK_STATUS, &ok);
            if (!ok) {
                glGetProgramiv(s->program, GL_INFO_LOG_LENGTH, &errmsg_len);
                char *errmsg = malloc(errmsg_len);
                glGetProgramInfoLog(s->program, errmsg_len, NULL, errmsg);
                traceError("error (%s): linking of shader #%d failed with '%s'\n", __func__, k,
                           errmsg);
                free(errmsg);
                glDeleteProgram(s->program);
                glDeleteShader(s->f_shader);
                goto err;
            }

            switch (k) {
            case glsl_YV12_RGBA:
            case glsl_NV12_RGBA:
                s->uniform.tex_0 = glGetUniformLocation(s->program, "tex[0]");
                s->uniform.tex_1 = glGetUniformLocation(s->program, "tex[1]");
                break;
            default:
                /* nothing */
                break;
            }
        }
    }

    retval = VDP_STATUS_OK;
err:
    pthread_mutex_unlock(&lock);
    return retval;
}

VdpStatus
softVdpDeviceCreateX11(Display *display_orig, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address)
{
    if (!display_orig || !device || !get_proc_address)
        return VDP_STATUS_INVALID_POINTER;

    // Let's get own connection to the X server
    Display *display = handle_xdpy_ref(display_orig);
    if (NULL == display)
        return VDP_STATUS_ERROR;

    if (global.quirks.buggy_XCloseDisplay) {
        // XCloseDisplay could segfault on fglrx. To avoid calling XCloseDisplay,
        // make one more reference to xdpy copy.
        handle_xdpy_ref(display_orig);
    }

    VdpDeviceData *data = calloc(1, sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_DEVICE;
    data->display = display;
    data->display_orig = display_orig;   // save supplied pointer too
    data->screen = screen;
    data->refcount = 0;
    data->root = DefaultRootWindow(display);

    // create master GLX context to share data between further created ones
    glx_context_ref_glc_hash_table(display, screen);
    data->root_glc = glx_context_get_root_context();

    glx_context_push_thread_local(data);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // initialize VAAPI
    if (global.quirks.avoid_va) {
        // pretend there is no VA-API available
        data->va_available = 0;
    } else {
        data->va_dpy = vaGetDisplayGLX(display);
        data->va_available = 0;

        VAStatus status = vaInitialize(data->va_dpy, &data->va_version_major,
                                       &data->va_version_minor);
        if (VA_STATUS_SUCCESS == status) {
            data->va_available = 1;
            traceInfo("libva (version %d.%d) library initialized\n",
                      data->va_version_major, data->va_version_minor);
        } else {
            data->va_available = 0;
            traceInfo("warning: failed to initialize libva. "
                      "No video decode acceleration available.\n");
        }
    }

    compile_shaders();

    glGenTextures(1, &data->watermark_tex_id);
    glBindTexture(GL_TEXTURE_2D, data->watermark_tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, watermark_width, watermark_height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, watermark_data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFinish();

    *device = handle_insert(data);
    *get_proc_address = &softVdpGetProcAddress;

    GLenum gl_error = glGetError();
    glx_context_pop();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpDeviceCreateX11): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}
