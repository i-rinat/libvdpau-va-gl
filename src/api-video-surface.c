/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define GL_GLEXT_PROTOTYPES
#include "ctx-stack.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <libswscale/swscale.h>
#include "pixfmt_compat.h"
#include "shaders.h"
#include <stdlib.h>
#include <string.h>
#include <va/va.h>
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include "api.h"
#include "trace.h"


VdpStatus
vdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type, uint32_t width, uint32_t height,
                      VdpVideoSurface *surface)
{
    VdpStatus err_code;
    if (!surface)
        return VDP_STATUS_INVALID_POINTER;
    if (chroma_type != VDP_CHROMA_TYPE_420 &&
        chroma_type != VDP_CHROMA_TYPE_422 &&
        chroma_type != VDP_CHROMA_TYPE_444)
    {
        return VDP_STATUS_INVALID_CHROMA_TYPE;
    }

    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoSurfaceData *data = calloc(1, sizeof(VdpVideoSurfaceData));
    if (NULL == data) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    data->type = HANDLETYPE_VIDEO_SURFACE;
    data->device = device;
    data->deviceData = deviceData;
    data->chroma_type = chroma_type;
    data->width = width;
    data->height = height;

    switch (chroma_type) {
    case VDP_CHROMA_TYPE_420:
        data->chroma_width = ((width + 1) & (~1u)) / 2;
        data->chroma_height = ((height + 1) & (~1u)) / 2;
        data->stride = (width + 0xfu) & (~0xfu);
        break;
    case VDP_CHROMA_TYPE_422:
        data->chroma_width = ((width + 1) & (~1u)) / 2;
        data->chroma_height = height;
        data->stride = (width + 2 * data->chroma_width + 0xfu) & (~0xfu);
        break;
    case VDP_CHROMA_TYPE_444:
        data->chroma_width = width;
        data->chroma_height = height;
        data->stride = (4 * width + 0xfu) & (~0xfu);
        break;
    }
    data->chroma_stride = (data->chroma_width + 0xfu) & (~0xfu);

    data->va_surf = VA_INVALID_SURFACE;
    data->tex_id = 0;
    data->sync_va_to_glx = 0;
    data->decoder = VDP_INVALID_HANDLE;
    data->y_plane = NULL;
    data->u_plane = NULL;
    data->v_plane = NULL;

    glx_ctx_push_thread_local(deviceData);
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
        glx_ctx_pop();
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }
    glFinish();

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    // no VA surface creation here. Actual pool of VA surfaces should be allocated already
    // by VdpDecoderCreate. VdpDecoderCreate will update ->va_surf field as needed.

    ref_device(deviceData);
    *surface = handle_insert(data);

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
vdpVideoSurfaceDestroy(VdpVideoSurface surface)
{
    VdpVideoSurfaceData *videoSurfData = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == videoSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoSurfData->deviceData;

    glx_ctx_push_thread_local(deviceData);
    glDeleteTextures(1, &videoSurfData->tex_id);

    GLenum gl_error = glGetError();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        glx_ctx_pop();
        handle_release(surface);
        return VDP_STATUS_ERROR;
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

    if (videoSurfData->y_plane)
        free(videoSurfData->y_plane);
    if (videoSurfData->u_plane)
        free(videoSurfData->u_plane);
    // do not free videoSurfData->v_plane, it's just pointer into the middle of u_plane

    glx_ctx_pop();
    unref_device(deviceData);
    handle_expunge(surface);
    free(videoSurfData);
    return VDP_STATUS_OK;
}

VdpStatus
vdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat destination_ycbcr_format,
                            void *const *destination_data, uint32_t const *destination_pitches)
{
    VdpStatus err_code;
    if (!destination_data || !destination_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpVideoSurfaceData *srcSurfData = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = srcSurfData->deviceData;
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
                const uint32_t sz = (uint32_t)q.width * (uint32_t)q.height;
                memcpy(destination_data[0], img_data + q.offsets[0], sz);
                memcpy(destination_data[1], img_data + q.offsets[1], sz / 2);
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
                const uint32_t sz = (uint32_t)q.width * (uint32_t)q.height;
                memcpy(destination_data[0], img_data + q.offsets[0], sz);
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
            traceError("error (%s): not implemented conversion VA FOURCC %c%c%c%c -> %s\n",
                       __func__, *c, *(c+1), *(c+2), *(c+3),
                       reverse_ycbcr_format(destination_ycbcr_format));
            vaDestroyImage(va_dpy, q.image_id);
            err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
            goto quit;
        }
        vaDestroyImage(va_dpy, q.image_id);
    } else {
        // software fallback
        traceError("error (%s): not implemented software fallback\n", __func__);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
vdpVideoSurfaceGetParameters(VdpVideoSurface surface, VdpChromaType *chroma_type,
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

static
int
vdpau_ycbcr_to_av_pixfmt(int fmt)
{
    switch (fmt) {
    case VDP_YCBCR_FORMAT_NV12:     return AV_PIX_FMT_NV12;
    case VDP_YCBCR_FORMAT_YV12:     return AV_PIX_FMT_YUV420P;
    case VDP_YCBCR_FORMAT_UYVY:     return AV_PIX_FMT_UYVY422;
    case VDP_YCBCR_FORMAT_YUYV:     return AV_PIX_FMT_YUYV422;
    case VDP_YCBCR_FORMAT_Y8U8V8A8: return AV_PIX_FMT_NONE;
    case VDP_YCBCR_FORMAT_V8U8Y8A8: return AV_PIX_FMT_NONE;
    default:                        return AV_PIX_FMT_NONE;
    }
}

static
VdpStatus
_video_surface_ensure_allocated(VdpVideoSurfaceData *surf)
{
    const uint32_t chroma_plane_size =
        (surf->chroma_stride * surf->chroma_height + 0xfu) & (~0xfu);
    if (surf->y_plane)
        return VDP_STATUS_OK;

    switch (surf->chroma_type) {
    case VDP_CHROMA_TYPE_420:
        surf->y_plane = malloc(surf->stride * surf->height);
        if (!surf->y_plane)
            return VDP_STATUS_RESOURCES;
        surf->u_plane = malloc(chroma_plane_size * 2);
        if (!surf->u_plane) {
            free(surf->y_plane);
            return VDP_STATUS_RESOURCES;
        }
        surf->v_plane = surf->u_plane + chroma_plane_size;
        return VDP_STATUS_OK;

    case VDP_CHROMA_TYPE_422:
        surf->y_plane = malloc(surf->stride * surf->height);
        if (!surf->y_plane)
            return VDP_STATUS_RESOURCES;
        surf->u_plane = surf->v_plane = NULL;
        return VDP_STATUS_OK;

    case VDP_CHROMA_TYPE_444:
        surf->y_plane = malloc(surf->stride * surf->height);
        if (!surf->y_plane)
            return VDP_STATUS_RESOURCES;
        surf->u_plane = surf->v_plane = NULL;
        return VDP_STATUS_OK;

    default:
        return VDP_STATUS_INVALID_CHROMA_TYPE;
    }
}

static
VdpStatus
vdpVideoSurfacePutBitsYCbCr_swscale(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                                    void const *const *source_data, uint32_t const *source_pitches)
{
    VdpStatus err_code;
    // TODO: implement this
    VdpVideoSurfaceData *dstSurfData = handle_acquire(surface, HANDLETYPE_VIDEO_SURFACE);
    // TODO: remove following (void)'s
    (void)vdpau_ycbcr_to_av_pixfmt;
    (void)source_pitches;
    (void)source_data;

    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;

    // sanity check
    switch (source_ycbcr_format) {
    case VDP_YCBCR_FORMAT_NV12:
        // fall through
    case VDP_YCBCR_FORMAT_YV12:
        if (dstSurfData->chroma_type != VDP_CHROMA_TYPE_420) {
            err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
            goto err;
        }
        break;
    case VDP_YCBCR_FORMAT_UYVY:
        // fall through
    case VDP_YCBCR_FORMAT_YUYV:
        if (dstSurfData->chroma_type != VDP_CHROMA_TYPE_422) {
            err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
            goto err;
        }
        break;
    case VDP_YCBCR_FORMAT_Y8U8V8A8:
        // fall through
    case VDP_YCBCR_FORMAT_V8U8Y8A8:
        if (dstSurfData->chroma_type != VDP_CHROMA_TYPE_444) {
            err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
            goto err;
        }
        break;
    default:
        err_code = VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
        goto err;
    }

    _video_surface_ensure_allocated(dstSurfData);
    dstSurfData->format = source_ycbcr_format;
    switch (source_ycbcr_format) {
    case VDP_YCBCR_FORMAT_NV12:

    case VDP_YCBCR_FORMAT_YV12:   // 420

    case VDP_YCBCR_FORMAT_UYVY:   // 422
    case VDP_YCBCR_FORMAT_YUYV:   // 422
    case VDP_YCBCR_FORMAT_Y8U8V8A8:   // 444
    case VDP_YCBCR_FORMAT_V8U8Y8A8:   // 444
        break;
    }




    err_code = VDP_STATUS_OK;
err:
    handle_release(surface);
    return err_code;
}

static
VdpStatus
vdpVideoSurfacePutBitsYCbCr_glsl(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
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
    VdpDeviceData *deviceData = dstSurfData->deviceData;

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

    glx_ctx_push_thread_local(deviceData);
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
        glUseProgram(deviceData->shaders[glsl_NV12_RGBA].program);
        glUniform1i(deviceData->shaders[glsl_NV12_RGBA].uniform.tex_0, 0);
        glUniform1i(deviceData->shaders[glsl_NV12_RGBA].uniform.tex_1, 1);
        break;
    case VDP_YCBCR_FORMAT_YV12:
        glUseProgram(deviceData->shaders[glsl_YV12_RGBA].program);
        glUniform1i(deviceData->shaders[glsl_YV12_RGBA].uniform.tex_0, 0);
        glUniform1i(deviceData->shaders[glsl_YV12_RGBA].uniform.tex_1, 1);
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
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto err;
    }

    err_code = VDP_STATUS_OK;
err:
    handle_release(surface);
    return err_code;
}

VdpStatus
vdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                            void const *const *source_data, uint32_t const *source_pitches)
{
    int using_glsl = 1;
    VdpStatus ret;

    if (using_glsl) {
        ret = vdpVideoSurfacePutBitsYCbCr_glsl(surface, source_ycbcr_format, source_data,
                                               source_pitches);
    } else {
        ret = vdpVideoSurfacePutBitsYCbCr_swscale(surface, source_ycbcr_format, source_data,
                                                  source_pitches);
    }
    return ret;
}

VdpStatus
vdpVideoSurfaceQueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                 VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
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
vdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
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
