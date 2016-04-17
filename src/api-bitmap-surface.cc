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

#include "api-bitmap-surface.hh"
#include "api-device.hh"
#include "compat-vdpau.hh"
#include "glx-context.hh"
#include "handle-storage.hh"
#include "reverse-constant.hh"
#include "trace.hh"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>


using std::shared_ptr;

namespace vdp { namespace BitmapSurface {

Resource::Resource(shared_ptr<vdp::Device::Resource> a_device, VdpRGBAFormat a_rgba_format,
                   uint32_t a_width, uint32_t a_height, VdpBool a_frequently_accessed)
    : rgba_format{a_rgba_format}
    , width{a_width}
    , height{a_height}
    , frequently_accessed{a_frequently_accessed}
{
    device = a_device;

    switch (rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
        gl_internal_format = GL_RGBA;
        gl_format =          GL_BGRA;
        gl_type =            GL_UNSIGNED_BYTE;
        bytes_per_pixel =    4;
        break;

    case VDP_RGBA_FORMAT_R8G8B8A8:
        gl_internal_format = GL_RGBA;
        gl_format =          GL_RGBA;
        gl_type =            GL_UNSIGNED_BYTE;
        bytes_per_pixel =    4;
        break;

    case VDP_RGBA_FORMAT_R10G10B10A2:
        gl_internal_format = GL_RGB10_A2;
        gl_format =          GL_RGBA;
        gl_type =            GL_UNSIGNED_INT_10_10_10_2;
        bytes_per_pixel =    4;
        break;

    case VDP_RGBA_FORMAT_B10G10R10A2:
        gl_internal_format = GL_RGB10_A2;
        gl_format =          GL_BGRA;
        gl_type =            GL_UNSIGNED_INT_10_10_10_2;
        bytes_per_pixel =    4;
        break;

    case VDP_RGBA_FORMAT_A8:
        gl_internal_format = GL_RGBA;
        gl_format =          GL_RED;
        gl_type =            GL_UNSIGNED_BYTE;
        bytes_per_pixel =    1;
        break;

    default:
        traceError("BitmapSurface::Resource::Resource(): %s not implemented\n",
                   reverse_rgba_format(rgba_format));
        throw invalid_rgba_format();
    }

    // Frequently accessed bitmaps reside in system memory rather that in GPU texture.
    dirty = 0;
    if (frequently_accessed)
        bitmap_data.reserve(width * height * bytes_per_pixel);

    GLXThreadLocalContext glc_guard{device};

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, gl_internal_format, width, height, 0, gl_format, gl_type,
                 nullptr);
    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        // Requested RGBA format was wrong
        traceError("BitmapSurface::Resource::Resource(): texture failure, gl error (%d, %s)\n",
                   gl_error, gluErrorString(gl_error));
        throw generic_error();
    }
}

Resource::~Resource()
{
    try {
        GLXThreadLocalContext glc_guard{device};
        glDeleteTextures(1, &tex_id);

        const auto gl_error = glGetError();
        if (gl_error != GL_NO_ERROR)
            traceError("BitmapSurface::Resource::~Resource(): gl error %d\n", gl_error);

    } catch (...) {
        traceError("BitmapSurface::Resource::~Resource(): caught exception\n");
    }
}

VdpStatus
CreateImpl(VdpDevice device_id, VdpRGBAFormat rgba_format, uint32_t width, uint32_t height,
           VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    if (!surface)
        return VDP_STATUS_INVALID_HANDLE;

    vdp::ResourceRef<vdp::Device::Resource> device{device_id};

    auto data = std::make_shared<Resource>(device, rgba_format, width, height, frequently_accessed);

    *surface = vdp::ResourceStorage<Resource>::instance().insert(data);

    return VDP_STATUS_OK;
}

VdpStatus
Create(VdpDevice device_id, VdpRGBAFormat rgba_format, uint32_t width, uint32_t height,
       VdpBool frequently_accessed, VdpBitmapSurface *surface)
{
    return check_for_exceptions(CreateImpl, device_id, rgba_format, width, height,
                                frequently_accessed, surface);
}

VdpStatus
DestroyImpl(VdpBitmapSurface surface_id)
{
    vdp::ResourceRef<vdp::BitmapSurface::Resource> surface{surface_id};

    ResourceStorage<Resource>::instance().drop(surface_id);
    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpBitmapSurface surface_id)
{
    return check_for_exceptions(DestroyImpl, surface_id);
}

VdpStatus
GetParametersImpl(VdpBitmapSurface surface_id, VdpRGBAFormat *rgba_format, uint32_t *width,
                  uint32_t *height, VdpBool *frequently_accessed)
{
    if (!rgba_format || !width || !height || !frequently_accessed)
        return VDP_STATUS_INVALID_POINTER;

    vdp::ResourceRef<Resource> src_surf{surface_id};

    *rgba_format = src_surf->rgba_format;
    *width =       src_surf->width;
    *height =      src_surf->height;
    *frequently_accessed = src_surf->frequently_accessed;

    return VDP_STATUS_OK;
}

VdpStatus
GetParameters(VdpBitmapSurface surface_id, VdpRGBAFormat *rgba_format, uint32_t *width,
              uint32_t *height, VdpBool *frequently_accessed)
{
    return check_for_exceptions(GetParametersImpl, surface_id, rgba_format, width, height,
                                frequently_accessed);
}

VdpStatus
PutBitsNativeImpl(VdpBitmapSurface surface_id, void const *const *source_data,
                  uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;

    vdp::ResourceRef<Resource> dst_surf{surface_id};

    VdpRect d_rect = {0, 0, dst_surf->width, dst_surf->height};
    if (destination_rect)
        d_rect = *destination_rect;

    if (dst_surf->frequently_accessed) {
        if (d_rect.x0 == 0 && dst_surf->width == d_rect.x1 && source_pitches[0] == d_rect.x1) {
            // full width, can copy all lines with a single memcpy
            const size_t bytes_to_copy = (d_rect.x1 - d_rect.x0) * (d_rect.y1 - d_rect.y0) *
                                         dst_surf->bytes_per_pixel;
            memcpy(dst_surf->bitmap_data.data() +
                   d_rect.y0 * dst_surf->width * dst_surf->bytes_per_pixel,
                   source_data[0], bytes_to_copy);
        } else {
            // copy line by line
            const size_t bytes_in_line = (d_rect.x1 - d_rect.x0) * dst_surf->bytes_per_pixel;
            auto source_data_0 = static_cast<const char *>(source_data[0]);
            for (auto y = d_rect.y0; y < d_rect.y1; y ++) {
                memcpy(dst_surf->bitmap_data.data() +
                       (y * dst_surf->width + d_rect.x0) * dst_surf->bytes_per_pixel,
                       source_data_0 + (y - d_rect.y0) * source_pitches[0],
                       bytes_in_line);
            }
        }

        dst_surf->dirty = true;

    } else {
        GLXThreadLocalContext glc_guard{dst_surf->device};

        glBindTexture(GL_TEXTURE_2D, dst_surf->tex_id);
        glPixelStorei(GL_UNPACK_ROW_LENGTH,
                      source_pitches[0] / dst_surf->bytes_per_pixel);

        if (dst_surf->bytes_per_pixel != 4)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexSubImage2D(GL_TEXTURE_2D, 0, d_rect.x0, d_rect.y0,
                        d_rect.x1 - d_rect.x0, d_rect.y1 - d_rect.y0,
                        dst_surf->gl_format, dst_surf->gl_type, source_data[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        if (dst_surf->bytes_per_pixel != 4)
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

        glFinish();

        const auto gl_error = glGetError();

        if (gl_error != GL_NO_ERROR) {
            traceError("BitmapSurface::PutBitsNativeImpl(): gl error %d\n", gl_error);
            return VDP_STATUS_ERROR;
        }
    }

    return VDP_STATUS_OK;
}

VdpStatus
PutBitsNative(VdpBitmapSurface surface_id, void const *const *source_data,
              uint32_t const *source_pitches, VdpRect const *destination_rect)
{
    return check_for_exceptions(PutBitsNativeImpl, surface_id, source_data, source_pitches,
                                destination_rect);
}

VdpStatus
QueryCapabilitiesImpl(VdpDevice device_id, VdpRGBAFormat surface_rgba_format, VdpBool *is_supported,
                      uint32_t *max_width, uint32_t *max_height)
{
    if (!is_supported || !max_width || !max_height)
        return VDP_STATUS_INVALID_POINTER;

    vdp::ResourceRef<vdp::Device::Resource> device{device_id};

    switch (surface_rgba_format) {
    case VDP_RGBA_FORMAT_B8G8R8A8:
    case VDP_RGBA_FORMAT_R8G8B8A8:
    case VDP_RGBA_FORMAT_R10G10B10A2:
    case VDP_RGBA_FORMAT_B10G10R10A2:
    case VDP_RGBA_FORMAT_A8:
        *is_supported = 1;          // all these formats are supported by every OpenGL
        break;                      // implementation
    default:
        *is_supported = 0;
        break;
    }

    GLXThreadLocalContext glc_guard{device};

    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("BitmapSurface::QueryCapabilitiesImpl(): gl error %d\n", gl_error);
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

} } // namespace vdp::BitmapSurface
