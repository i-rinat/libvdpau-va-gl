/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define GL_GLEXT_PROTOTYPES
#include "ctx-stack.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include "trace.h"
#include <vdpau/vdpau.h>

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

VdpStatus
vdpOutputSurfaceCreate(VdpDevice device, VdpRGBAFormat rgba_format, uint32_t width,
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
        traceError("error (%s): %s is not implemented\n", __func__,
                   reverse_rgba_format(rgba_format));
        free(data);
        err_code = VDP_STATUS_INVALID_RGBA_FORMAT;
        goto quit;
    }

    data->type = HANDLETYPE_OUTPUT_SURFACE;
    data->width = width;
    data->height = height;
    data->device = device;
    data->deviceData = deviceData;
    data->rgba_format = rgba_format;

    glx_ctx_push_thread_local(deviceData);
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
        traceError("error (%s): framebuffer not ready, %d, %s\n", __func__, gl_status,
                   gluErrorString(gl_status));
        glx_ctx_pop();
        free(data);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        free(data);
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
vdpOutputSurfaceDestroy(VdpOutputSurface surface)
{
    VdpStatus err_code;
    VdpOutputSurfaceData *data = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = data->deviceData;

    glx_ctx_push_thread_local(deviceData);
    glDeleteTextures(1, &data->tex_id);
    glDeleteFramebuffers(1, &data->fbo_id);

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    handle_expunge(surface);
    unref_device(deviceData);
    free(data);
    return VDP_STATUS_OK;

quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
vdpOutputSurfaceGetBitsNative(VdpOutputSurface surface, VdpRect const *source_rect,
                              void *const *destination_data, uint32_t const *destination_pitches)
{
    VdpStatus err_code;
    if (!destination_data || !destination_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpOutputSurfaceData *srcSurfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = srcSurfData->deviceData;

    VdpRect srcRect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (source_rect)
        srcRect = *source_rect;

    glx_ctx_push_thread_local(deviceData);
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
    glx_ctx_pop();
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
vdpOutputSurfaceGetParameters(VdpOutputSurface surface, VdpRGBAFormat *rgba_format,
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
vdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface, VdpIndexedFormat source_indexed_format,
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
    VdpDeviceData *deviceData = surfData->deviceData;

    VdpRect dstRect = {0, 0, surfData->width, surfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    // there is no other formats anyway
    if (VDP_COLOR_TABLE_FORMAT_B8G8R8X8 != color_table_format) {
        err_code = VDP_STATUS_INVALID_COLOR_TABLE_FORMAT;
        goto quit;
    }
    const uint32_t *color_table32 = color_table;

    glx_ctx_push_thread_local(deviceData);

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
            glx_ctx_pop();
            if (GL_NO_ERROR != gl_error) {
                traceError("error (%s): gl error %d\n", __func__, gl_error);
                err_code = VDP_STATUS_ERROR;
                goto quit;
            }

            err_code = VDP_STATUS_OK;
            goto quit;
        } while (0);
        break;
    default:
        traceError("error (%s): unsupported indexed format %s\n", __func__,
                   reverse_indexed_format(source_indexed_format));
        err_code = VDP_STATUS_INVALID_INDEXED_FORMAT;
        goto quit;
    }

quit:
    handle_release(surface);
    return err_code;
}

VdpStatus
vdpOutputSurfacePutBitsNative(VdpOutputSurface surface, void const *const *source_data,
                              uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    VdpStatus err_code;
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;
    VdpOutputSurfaceData *dstSurfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == dstSurfData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = dstSurfData->deviceData;

    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    glx_ctx_push_thread_local(deviceData);
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
    glx_ctx_pop();
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
vdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                             void const *const *source_data, uint32_t const *source_pitches,
                             VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    (void)surface; (void)source_ycbcr_format; (void)source_data; (void)source_pitches;
    (void)destination_rect; (void)csc_matrix;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpOutputSurfaceQueryCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                  VdpBool *is_supported, uint32_t *max_width, uint32_t *max_height)
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

VdpStatus
vdpOutputSurfaceQueryGetPutBitsNativeCapabilities(VdpDevice device,
                                                  VdpRGBAFormat surface_rgba_format,
                                                  VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpOutputSurfaceQueryPutBitsIndexedCapabilities(VdpDevice device,
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
vdpOutputSurfaceQueryPutBitsYCbCrCapabilities(VdpDevice device,
                                              VdpRGBAFormat surface_rgba_format,
                                              VdpYCbCrFormat bits_ycbcr_format,
                                              VdpBool *is_supported)
{
    (void)device; (void)surface_rgba_format; (void)bits_ycbcr_format; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
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
    if (srcSurfData && srcSurfData->deviceData != dstSurfData->deviceData) {
        err_code = VDP_STATUS_HANDLE_DEVICE_MISMATCH;
        goto quit;
    }
    VdpDeviceData *deviceData = dstSurfData->deviceData;

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

    glx_ctx_push_thread_local(deviceData);
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
        if (srcSurfData->rgba_format == VDP_RGBA_FORMAT_A8) {
            glUseProgram(deviceData->shaders[glsl_red_to_alpha_swizzle].program);
            glUniform1i(deviceData->shaders[glsl_red_to_alpha_swizzle].uniform.tex_0, 0);
        }
    }

    compose_surfaces(bs, s_rect, d_rect, colors, flags, !!srcSurfData);

    glUseProgram(0);
    glFinish();

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
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
vdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
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
    if (srcSurfData && srcSurfData->deviceData != dstSurfData->deviceData) {
        err_code = VDP_STATUS_HANDLE_DEVICE_MISMATCH;
        goto quit;
    }
    VdpDeviceData *deviceData = dstSurfData->deviceData;

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

    glx_ctx_push_thread_local(deviceData);
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
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
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
