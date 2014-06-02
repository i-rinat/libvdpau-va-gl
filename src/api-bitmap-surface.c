/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#include "ctx-stack.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>
#include <vdpau/vdpau.h>
#include "api.h"
#include "trace.h"


VdpStatus
vdpBitmapSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
                       uint32_t height, VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    VdpStatus err_code;
    if (!surface)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpBitmapSurfaceData *data = calloc(1, sizeof(VdpBitmapSurfaceData));
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
        traceError("error (%s): %s not implemented\n", __func__, reverse_rgba_format(rgba_format));
        free(data);
        err_code = VDP_STATUS_INVALID_RGBA_FORMAT;
        goto quit;
    }

    data->type = HANDLETYPE_BITMAP_SURFACE;
    data->device = device;
    data->deviceData = deviceData;
    data->rgba_format = rgba_format;
    data->width = width;
    data->height = height;
    data->frequently_accessed = frequently_accessed;

    // Frequently accessed bitmaps reside in system memory rather that in GPU texture.
    data->dirty = 0;
    if (frequently_accessed) {
        data->bitmap_data = calloc(width * height, data->bytes_per_pixel);
        if (NULL == data->bitmap_data) {
            traceError("error (%s): calloc returned NULL\n", __func__);
            free(data);
            err_code = VDP_STATUS_RESOURCES;
            goto quit;
        }
    } else {
        data->bitmap_data = NULL;
    }

    glx_ctx_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, data->gl_internal_format, width, height, 0,
                 data->gl_format, data->gl_type, NULL);
    glFinish();
    GLuint gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        // Requested RGBA format was wrong
        traceError("error (%s): texture failure, gl error (%d, %s)\n", __func__, gl_error,
                   gluErrorString(gl_error));
        free(data);
        glx_ctx_pop();
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        free(data);
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    ref_device(deviceData);
    *surface = handle_insert(data);

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
vdpBitmapSurfaceDestroy(VdpBitmapSurface surface)
{
    VdpBitmapSurfaceData *data = handle_acquire(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->deviceData;

    if (data->frequently_accessed) {
        free(data->bitmap_data);
        data->bitmap_data = NULL;
    }

    glx_ctx_push_thread_local(deviceData);
    glDeleteTextures(1, &data->tex_id);

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        handle_release(surface);
        return VDP_STATUS_ERROR;
    }

    handle_expunge(surface);
    unref_device(deviceData);
    free(data);
    return VDP_STATUS_OK;
}

VdpStatus
vdpBitmapSurfaceGetParameters(VdpBitmapSurface surface, VdpRGBAFormat *rgba_format,
                              uint32_t *width, uint32_t *height, VdpBool *frequently_accessed)
{
    VdpBitmapSurfaceData *srcSurfData = handle_acquire(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == rgba_format || NULL == width || NULL == height || NULL == frequently_accessed) {
        handle_release(surface);
        return VDP_STATUS_INVALID_POINTER;
    }

    *rgba_format = srcSurfData->rgba_format;
    *width = srcSurfData->width;
    *height = srcSurfData->height;
    *frequently_accessed = srcSurfData->frequently_accessed;

    handle_release(surface);
    return VDP_STATUS_OK;
}

VdpStatus
vdpBitmapSurfacePutBitsNative(VdpBitmapSurface surface, void const *const *source_data,
                              uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    VdpStatus err_code;
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpBitmapSurfaceData *dstSurfData = handle_acquire(surface, HANDLETYPE_BITMAP_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->deviceData;

    VdpRect d_rect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        d_rect = *destination_rect;

    if (dstSurfData->frequently_accessed) {
        if (0 == d_rect.x0 && dstSurfData->width == d_rect.x1 && source_pitches[0] == d_rect.x1) {
            // full width
            const int bytes_to_copy =
                (d_rect.x1 - d_rect.x0) * (d_rect.y1 - d_rect.y0) * dstSurfData->bytes_per_pixel;
            memcpy(dstSurfData->bitmap_data +
                        d_rect.y0 * dstSurfData->width * dstSurfData->bytes_per_pixel,
                   source_data[0], bytes_to_copy);
        } else {
            const unsigned int bytes_in_line = (d_rect.x1-d_rect.x0)*dstSurfData->bytes_per_pixel;
            for (unsigned int y = d_rect.y0; y < d_rect.y1; y ++) {
                memcpy(dstSurfData->bitmap_data +
                            (y * dstSurfData->width + d_rect.x0) * dstSurfData->bytes_per_pixel,
                       source_data[0] + (y - d_rect.y0) * source_pitches[0],
                       bytes_in_line);
            }
        }
        dstSurfData->dirty = 1;
    } else {
        glx_ctx_push_thread_local(deviceData);

        glBindTexture(GL_TEXTURE_2D, dstSurfData->tex_id);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]/dstSurfData->bytes_per_pixel);
        if (4 != dstSurfData->bytes_per_pixel)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, d_rect.x0, d_rect.y0,
            d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0,
            dstSurfData->gl_format, dstSurfData->gl_type, source_data[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        if (4 != dstSurfData->bytes_per_pixel)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glFinish();

        GLenum gl_error = glGetError();
        glx_ctx_pop();
        if (GL_NO_ERROR != gl_error) {
            traceError("error (%s): gl error %d\n", __func__, gl_error);
            err_code = VDP_STATUS_ERROR;
            goto quit;
        }
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
vdpBitmapSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                  VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
{
    VdpStatus err_code;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == is_supported || NULL == max_width || NULL == max_height) {
        err_code = VDP_STATUS_INVALID_POINTER;
        goto quit;
    }

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

    glx_ctx_push_thread_local(deviceData);
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
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
