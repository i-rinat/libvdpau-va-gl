#define GL_GLEXT_PROTOTYPES
#include "ctx-stack.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "shaders.h"
#include <stdlib.h>
#include <string.h>
#include <va/va.h>
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include "vdpau-soft.h"
#include "vdpau-trace.h"


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
