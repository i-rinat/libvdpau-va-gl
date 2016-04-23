/*
 * Copyright 2013-2016  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define GL_GLEXT_PROTOTYPES
#include "api-bitmap-surface.hh"
#include "api-device.hh"
#include "api-output-surface.hh"
#include "glx-context.hh"
#include "handle-storage.hh"
#include "reverse-constant.hh"
#include "trace.hh"
#include <GL/gl.h>
#include <stdlib.h>
#include <vdpau/vdpau.h>
#include <vector>


using std::shared_ptr;
using std::make_shared;

namespace vdp { namespace OutputSurface {

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
                 VdpColor const *colors, int flags, bool has_src_surf)
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

    if (bs.srcFuncRGB == GL_INVALID_VALUE || bs.srcFuncAlpha == GL_INVALID_VALUE ||
        bs.dstFuncRGB == GL_INVALID_VALUE || bs.dstFuncAlpha == GL_INVALID_VALUE)
    {
        bs.invalid_func = 1;
    }

    if (blend_state) {
        bs.modeRGB =   vdpBlendEquationToGLEquation(blend_state->blend_equation_color);
        bs.modeAlpha = vdpBlendEquationToGLEquation(blend_state->blend_equation_alpha);
    } else {
        bs.modeRGB = bs.modeAlpha = GL_FUNC_ADD;
    }

    if (bs.modeRGB == GL_INVALID_VALUE || bs.modeAlpha == GL_INVALID_VALUE)
        bs.invalid_eq = 1;

    return bs;
}

Resource::Resource(shared_ptr<vdp::Device::Resource> a_device, VdpRGBAFormat a_rgba_format,
                   uint32_t a_width, uint32_t a_height)
    : rgba_format{a_rgba_format}
    , width{a_width}
    , height{a_height}
    , first_presentation_time{0}
    , status{VDP_PRESENTATION_QUEUE_STATUS_IDLE}
{
    // TODO: figure out reasonable limits
    if (width > 4096 || height > 4096)
        throw vdp::invalid_size();

    device = a_device;

    switch (rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
        gl_internal_format = GL_RGBA;
        gl_format = GL_BGRA;
        gl_type = GL_UNSIGNED_BYTE;
        bytes_per_pixel = 4;
        break;

    case VDP_RGBA_FORMAT_R8G8B8A8:
        gl_internal_format = GL_RGBA;
        gl_format = GL_RGBA;
        gl_type = GL_UNSIGNED_BYTE;
        bytes_per_pixel = 4;
        break;

    case VDP_RGBA_FORMAT_R10G10B10A2:
        gl_internal_format = GL_RGB10_A2;
        gl_format = GL_RGBA;
        gl_type = GL_UNSIGNED_INT_10_10_10_2;
        bytes_per_pixel = 4;
        break;

    case VDP_RGBA_FORMAT_B10G10R10A2:
        gl_internal_format = GL_RGB10_A2;
        gl_format = GL_BGRA;
        gl_type = GL_UNSIGNED_INT_10_10_10_2;
        bytes_per_pixel = 4;
        break;

    case VDP_RGBA_FORMAT_A8:
        gl_internal_format = GL_RGBA;
        gl_format = GL_RED;
        gl_type = GL_UNSIGNED_BYTE;
        bytes_per_pixel = 1;
        break;

    default:
        traceError("OutputSurface::Resource::Resource(): %s is not implemented\n",
                   reverse_rgba_format(rgba_format));
        throw vdp::invalid_rgba_format();
    }

    GLXThreadLocalContext guard{device};

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // reserve texture data
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, gl_type,
                 nullptr);

    glGenFramebuffers(1, &fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);

    const auto gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (gl_status != GL_FRAMEBUFFER_COMPLETE) {
        traceError("OutputSurface::Resource::Resource(): framebuffer not ready, %d\n", gl_status);
        throw vdp::generic_error();
    }

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("OutputSurface::Resource::Resource(): gl error %d\n", gl_error);
        throw vdp::generic_error();
    }
}

Resource::~Resource()
{
    try {
        GLXThreadLocalContext guard{device};

        glDeleteTextures(1, &tex_id);
        glDeleteFramebuffers(1, &fbo_id);

        const auto gl_error = glGetError();
        if (gl_error != GL_NO_ERROR)
            traceError("OutputSurface::Resource::~Resource(): gl error %d\n", gl_error);

    } catch (...) {
        traceError("OutputSurface::Resource::~Resource(): caught exception\n");
    }
}

VdpStatus
CreateImpl(VdpDevice device_id, VdpRGBAFormat rgba_format, uint32_t width, uint32_t height,
           VdpOutputSurface *surface)
{
    if (!surface)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

    auto data = std::make_shared<Resource>(device, rgba_format, width, height);

    *surface = ResourceStorage<Resource>::instance().insert(data);
    return VDP_STATUS_OK;
}

VdpStatus
Create(VdpDevice device_id, VdpRGBAFormat rgba_format, uint32_t width, uint32_t height,
       VdpOutputSurface *surface)
{
    return check_for_exceptions(CreateImpl, device_id, rgba_format, width, height, surface);
}

VdpStatus
DestroyImpl(VdpOutputSurface surface_id)
{
    ResourceRef<Resource> surface{surface_id};

    ResourceStorage<Resource>::instance().drop(surface_id);
    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpOutputSurface surface_id)
{
    return check_for_exceptions(DestroyImpl, surface_id);
}

VdpStatus
GetBitsNativeImpl(VdpOutputSurface surface_id, VdpRect const *source_rect,
                  void *const *destination_data, uint32_t const *destination_pitches)
{
    if (!destination_data || !destination_pitches)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<Resource> surface{surface_id};

    VdpRect src_rect = {0, 0, surface->width, surface->height};
    if (source_rect)
        src_rect = *source_rect;

    GLXThreadLocalContext guard{surface->device};

    glBindFramebuffer(GL_FRAMEBUFFER, surface->fbo_id);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, destination_pitches[0] / surface->bytes_per_pixel);

    if (surface->bytes_per_pixel != 4)
        glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glReadPixels(src_rect.x0, src_rect.y0, src_rect.x1 - src_rect.x0, src_rect.y1 - src_rect.y0,
                 surface->gl_format, surface->gl_type, destination_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    if (surface->bytes_per_pixel != 4)
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("OutputSurface::GetBitsNativeImpl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
GetBitsNative(VdpOutputSurface surface_id, VdpRect const *source_rect,
              void *const *destination_data, uint32_t const *destination_pitches)
{
    return check_for_exceptions(GetBitsNativeImpl, surface_id, source_rect, destination_data,
                                destination_pitches);
}

VdpStatus
GetParametersImpl(VdpOutputSurface surface_id, VdpRGBAFormat *rgba_format, uint32_t *width,
                  uint32_t *height)
{
    if (!rgba_format || !width || !height)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<Resource> surface{surface_id};

    *rgba_format = surface->rgba_format;
    *width       = surface->width;
    *height      = surface->height;

    return VDP_STATUS_OK;
}

VdpStatus
GetParameters(VdpOutputSurface surface_id, VdpRGBAFormat *rgba_format, uint32_t *width,
              uint32_t *height)
{
    return check_for_exceptions(GetParametersImpl, surface_id, rgba_format, width, height);
}

VdpStatus
PutBitsIndexedImpl(VdpOutputSurface surface_id, VdpIndexedFormat source_indexed_format,
                   void const *const *source_data, uint32_t const *source_pitch,
                   VdpRect const *destination_rect, VdpColorTableFormat color_table_format,
                   void const *color_table)
{
    if (!source_data || !source_pitch || !color_table)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<Resource> surface{surface_id};

    VdpRect dst_rect = {0, 0, surface->width, surface->height};
    if (destination_rect)
        dst_rect = *destination_rect;

    // there are no other formats anyway
    if (color_table_format != VDP_COLOR_TABLE_FORMAT_B8G8R8X8) {
        // TODO: investigate
        return VDP_STATUS_INVALID_COLOR_TABLE_FORMAT;
    }

    const auto color_table32 = static_cast<const uint32_t *>(color_table);

    GLXThreadLocalContext guard{surface->device};

    switch (source_indexed_format) {
    case VDP_INDEXED_FORMAT_I8A8:
        // TODO: use shader?
        do {
            const uint32_t dstRectWidth = dst_rect.x1 - dst_rect.x0;
            const uint32_t dstRectHeight = dst_rect.y1 - dst_rect.y0;

            std::vector<uint32_t> unpacked_buf(dstRectWidth * dstRectHeight);

            for (uint32_t y = 0; y < dstRectHeight; y ++) {
                auto src_ptr = static_cast<const uint8_t *>(source_data[0]);
                src_ptr += y * source_pitch[0];
                uint32_t *dst_ptr = unpacked_buf.data() + y * dstRectWidth;

                for (uint32_t x = 0; x < dstRectWidth; x ++) {
                    const uint8_t i = *src_ptr++;
                    const uint32_t a = (*src_ptr++) << 24;
                    dst_ptr[x] = (color_table32[i] & 0x00ffffff) + a;
                }
            }

            glBindTexture(GL_TEXTURE_2D, surface->tex_id);
            glTexSubImage2D(GL_TEXTURE_2D, 0, dst_rect.x0, dst_rect.y0,
                            dst_rect.x1 - dst_rect.x0, dst_rect.y1 - dst_rect.y0,
                            GL_BGRA, GL_UNSIGNED_BYTE, unpacked_buf.data());
            glFinish();

            const auto gl_error = glGetError();
            if (gl_error != GL_NO_ERROR) {
                traceError("OutputSurface::PutBitsIndexedImpl(): gl error %d\n", gl_error);
                return VDP_STATUS_ERROR;
            }
        } while (0);
        break;

    default:
        traceError("OutputSurface::PutBitsIndexedImpl(): unsupported indexed format %s\n",
                   reverse_indexed_format(source_indexed_format));
        return VDP_STATUS_INVALID_INDEXED_FORMAT;
    }

    return VDP_STATUS_OK;
}

VdpStatus
PutBitsIndexed(VdpOutputSurface surface_id, VdpIndexedFormat source_indexed_format,
               void const *const *source_data, uint32_t const *source_pitch,
               VdpRect const *destination_rect, VdpColorTableFormat color_table_format,
               void const *color_table)
{
    return check_for_exceptions(PutBitsIndexedImpl, surface_id, source_indexed_format, source_data,
                                source_pitch, destination_rect, color_table_format, color_table);
}

VdpStatus
PutBitsNativeImpl(VdpOutputSurface surface_id, void const *const *source_data,
                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<Resource> surface{surface_id};

    VdpRect dst_rect = {0, 0, surface->width, surface->height};
    if (destination_rect)
        dst_rect = *destination_rect;

    GLXThreadLocalContext guard{surface->device};

    glBindTexture(GL_TEXTURE_2D, surface->tex_id);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0] / surface->bytes_per_pixel);

    if (surface->bytes_per_pixel != 4)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexSubImage2D(GL_TEXTURE_2D, 0, dst_rect.x0, dst_rect.y0,
                    dst_rect.x1 - dst_rect.x0, dst_rect.y1 - dst_rect.y0,
                    surface->gl_format, surface->gl_type, source_data[0]);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    if (surface->bytes_per_pixel != 4)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("OutputSurface::PutBitsNativeImpl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
PutBitsNative(VdpOutputSurface surface_id, void const *const *source_data,
              uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    return check_for_exceptions(PutBitsNativeImpl, surface_id, source_data, source_pitches,
                                destination_rect);
}

VdpStatus
PutBitsYCbCrImpl(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
                 void const *const *source_data, uint32_t const *source_pitches,
                 VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    std::ignore = surface;
    std::ignore = source_ycbcr_format;
    std::ignore = source_data;
    std::ignore = source_pitches;
    std::ignore = destination_rect;
    std::ignore = csc_matrix;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
PutBitsYCbCr(VdpOutputSurface surface, VdpYCbCrFormat source_ycbcr_format,
             void const *const *source_data, uint32_t const *source_pitches,
             VdpRect const *destination_rect, VdpCSCMatrix const *csc_matrix)
{
    return check_for_exceptions(PutBitsYCbCrImpl, surface, source_ycbcr_format, source_data,
                                source_pitches, destination_rect, csc_matrix);
}

VdpStatus
QueryCapabilitiesImpl(VdpDevice device_id, VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                      uint32_t *max_width, uint32_t *max_height)
{
    if (!is_supported || !max_width || !max_height)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

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

    GLXThreadLocalContext guard{device};

    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("OutputSurface::QueryCapabilitiesImpl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    *max_width = max_texture_size;
    *max_height = max_texture_size;

    return VDP_STATUS_OK;
}

VdpStatus
QueryCapabilities(VdpDevice device_id, VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                  uint32_t *max_width, uint32_t *max_height)
{
    return check_for_exceptions(QueryCapabilitiesImpl, device_id, surface_rgba_format, is_supported,
                                max_width, max_height);
}

VdpStatus
QueryGetPutBitsNativeCapabilitiesImpl(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                      VdpBool *is_supported)
{
    std::ignore = device;
    std::ignore = surface_rgba_format;
    std::ignore = is_supported;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryGetPutBitsNativeCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                  VdpBool *is_supported)
{
    return check_for_exceptions(QueryGetPutBitsNativeCapabilitiesImpl, device, surface_rgba_format,
                                is_supported);
}

VdpStatus
QueryPutBitsIndexedCapabilitiesImpl(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                    VdpIndexedFormat bits_indexed_format,
                                    VdpColorTableFormat color_table_format, VdpBool *is_supported)
{
    std::ignore = device;
    std::ignore = surface_rgba_format;
    std::ignore = bits_indexed_format;
    std::ignore = color_table_format;
    std::ignore = is_supported;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryPutBitsIndexedCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                VdpIndexedFormat bits_indexed_format,
                                VdpColorTableFormat color_table_format, VdpBool *is_supported)
{
    return check_for_exceptions(QueryPutBitsIndexedCapabilitiesImpl, device, surface_rgba_format,
                                bits_indexed_format, color_table_format, is_supported);
}

VdpStatus
QueryPutBitsYCbCrCapabilitiesImpl(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                                  VdpYCbCrFormat bits_ycbcr_format, VdpBool *is_supported)
{
    std::ignore = device;
    std::ignore = surface_rgba_format;
    std::ignore = bits_ycbcr_format;
    std::ignore = is_supported;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryPutBitsYCbCrCapabilities(VdpDevice device, VdpRGBAFormat surface_rgba_format,
                              VdpYCbCrFormat bits_ycbcr_format, VdpBool *is_supported)
{
    return check_for_exceptions(QueryPutBitsYCbCrCapabilitiesImpl, device, surface_rgba_format,
                                bits_ycbcr_format, is_supported);
}

VdpStatus
RenderBitmapSurfaceImpl(VdpOutputSurface destination_surface, VdpRect const *destination_rect,
                        VdpBitmapSurface source_surface, VdpRect const *source_rect,
                        VdpColor const *colors, VdpOutputSurfaceRenderBlendState const *blend_state,
                        uint32_t flags)
{
    if (blend_state) {
        if (blend_state->struct_version != VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION)
            return VDP_STATUS_INVALID_VALUE;
    }

    ResourceRef<vdp::OutputSurface::Resource> dst_surf{destination_surface};

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);

    if (bs.invalid_func)
        return VDP_STATUS_INVALID_BLEND_FACTOR;

    if (bs.invalid_eq)
        return VDP_STATUS_INVALID_BLEND_EQUATION;

    GLXThreadLocalContext guard{dst_surf->device};

    glBindFramebuffer(GL_FRAMEBUFFER, dst_surf->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dst_surf->width, 0, dst_surf->height, -1.0f, 1.0f);
    glViewport(0, 0, dst_surf->width, dst_surf->height);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    VdpRect s_rect = {0, 0, 1, 1};

    if (source_surface != VDP_INVALID_HANDLE) {
        ResourceRef<vdp::BitmapSurface::Resource> src_surf{source_surface};

        if (dst_surf->device->id != src_surf->device->id)
            return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

        s_rect.x1 = src_surf->width;
        s_rect.y1 = src_surf->height;

        glBindTexture(GL_TEXTURE_2D, src_surf->tex_id);

        if (src_surf->dirty) {
            if (src_surf->bytes_per_pixel != 4)
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src_surf->width, src_surf->height,
                            src_surf->gl_format, src_surf->gl_type,
                            src_surf->bitmap_data.data());

            if (src_surf->bytes_per_pixel != 4)
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

            src_surf->dirty = false;
        }

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f / src_surf->width, 1.0f / src_surf->height, 1.0f);

        if (src_surf->rgba_format == VDP_RGBA_FORMAT_A8) {
            glUseProgram(src_surf->device->shaders[glsl_red_to_alpha_swizzle].program);
            glUniform1i(src_surf->device->shaders[glsl_red_to_alpha_swizzle].uniform.tex_0, 0);
        }
    }

    VdpRect d_rect = {0, 0, dst_surf->width, dst_surf->height};
    if (destination_rect)
        d_rect = *destination_rect;

    if (source_rect)
        s_rect = *source_rect;

    compose_surfaces(bs, s_rect, d_rect, colors, flags, source_surface != VDP_INVALID_HANDLE);

    glUseProgram(0);
    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("OutputSurface::RenderBitmapSurfaceImpl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
RenderBitmapSurface(VdpOutputSurface destination_surface, VdpRect const *destination_rect,
                    VdpBitmapSurface source_surface, VdpRect const *source_rect,
                    VdpColor const *colors, VdpOutputSurfaceRenderBlendState const *blend_state,
                    uint32_t flags)
{
    return check_for_exceptions(RenderBitmapSurfaceImpl, destination_surface, destination_rect,
                                source_surface, source_rect, colors, blend_state, flags);
}

VdpStatus
RenderOutputSurfaceImpl(VdpOutputSurface destination_surface, VdpRect const *destination_rect,
                        VdpOutputSurface source_surface, VdpRect const *source_rect,
                        VdpColor const *colors, VdpOutputSurfaceRenderBlendState const *blend_state,
                        uint32_t flags)
{
    if (blend_state) {
        if (blend_state->struct_version != VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION)
            return VDP_STATUS_INVALID_VALUE;
    }

    ResourceRef<Resource> dst_surf{destination_surface};

    // select blend functions
    struct blend_state_struct bs = vdpBlendStateToGLBlendState(blend_state);

    if (bs.invalid_func)
        return VDP_STATUS_INVALID_BLEND_FACTOR;

    if (bs.invalid_eq)
        return VDP_STATUS_INVALID_BLEND_EQUATION;

    GLXThreadLocalContext guard{dst_surf->device};

    glBindFramebuffer(GL_FRAMEBUFFER, dst_surf->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dst_surf->width, 0, dst_surf->height, -1.0f, 1.0f);
    glViewport(0, 0, dst_surf->width, dst_surf->height);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    VdpRect s_rect = {0, 0, 1, 1};

    if (source_surface != VDP_INVALID_HANDLE) {
        ResourceRef<vdp::OutputSurface::Resource> src_surf{source_surface};

        if (dst_surf->device->id != src_surf->device->id)
            return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

        s_rect.x1 = src_surf->width;
        s_rect.y1 = src_surf->height;

        glBindTexture(GL_TEXTURE_2D, src_surf->tex_id);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f / src_surf->width, 1.0f / src_surf->height, 1.0f);
    }

    VdpRect d_rect = {0, 0, dst_surf->width, dst_surf->height};
    if (destination_rect)
        d_rect = *destination_rect;
    if (source_rect)
        s_rect = *source_rect;

    compose_surfaces(bs, s_rect, d_rect, colors, flags, source_surface != VDP_INVALID_HANDLE);
    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("OutputSurface::RenderOutputSurfaceImpl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
RenderOutputSurface(VdpOutputSurface destination_surface, VdpRect const *destination_rect,
                    VdpOutputSurface source_surface, VdpRect const *source_rect,
                    VdpColor const *colors, VdpOutputSurfaceRenderBlendState const *blend_state,
                    uint32_t flags)
{
    return check_for_exceptions(RenderOutputSurfaceImpl, destination_surface, destination_rect,
                                source_surface, source_rect, colors, blend_state, flags);
}

} } // namespace vdp::OutputSurface
