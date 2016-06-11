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
#include "api-video-surface.hh"
#include "api.hh"
#include "compat.hh"
#include "glx-context.hh"
#include "handle-storage.hh"
#include "reverse-constant.hh"
#include "shaders.h"
#include "trace.hh"
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>
#include <va/va.h>
#include <vdpau/vdpau.h>


using std::make_shared;
using std::shared_ptr;


namespace vdp { namespace VideoSurface {

Resource::Resource(std::shared_ptr<vdp::Device::Resource> a_device, VdpChromaType a_chroma_type,
                   uint32_t a_width, uint32_t a_height)
    : chroma_type{a_chroma_type}
    , width{a_width}
    , height{a_height}
    , rt_idx{0}
{
    device = a_device;

    if (chroma_type != VDP_CHROMA_TYPE_420 &&
        chroma_type != VDP_CHROMA_TYPE_422 &&
        chroma_type != VDP_CHROMA_TYPE_444)
    {
        throw vdp::invalid_chroma_type();
    }

    switch (chroma_type) {
    case VDP_CHROMA_TYPE_420:
        chroma_width = ((width + 1) & (~1u)) / 2;
        chroma_height = ((height + 1) & (~1u)) / 2;
        stride = (width + 0xfu) & (~0xfu);
        break;

    case VDP_CHROMA_TYPE_422:
        chroma_width = ((width + 1) & (~1u)) / 2;
        chroma_height = height;
        stride = (width + 2 * chroma_width + 0xfu) & (~0xfu);
        break;

    case VDP_CHROMA_TYPE_444:
        chroma_width = width;
        chroma_height = height;
        stride = (4 * width + 0xfu) & (~0xfu);
        break;
    }

    chroma_stride = (chroma_width + 0xfu) & (~0xfu);

    va_surf =        VA_INVALID_SURFACE;
    tex_id =         0;
    sync_va_to_glx = false;

    GLXThreadLocalContext guard{device};

    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

    glGenFramebuffers(1, &fbo_id);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0);

    const auto gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (gl_status != GL_FRAMEBUFFER_COMPLETE) {
        traceError("VideoSurface::Resource::Resource(): framebuffer not ready, %d\n", gl_status);
        throw vdp::generic_error();
    }

    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("VideoSurface::Resource::Resource(): gl error %d\n", gl_error);
        throw vdp::generic_error();
    }

    // no VA surface creation here. Actual pool of VA surfaces should be allocated already
    // by VdpDecoderCreate. VdpDecoderCreate will update ->va_surf field as needed.
}

Resource::~Resource()
{
    try {
        {
            GLXThreadLocalContext guard{device};
            glDeleteTextures(1, &tex_id);

            const auto gl_error = glGetError();
            if (gl_error != GL_NO_ERROR)
                traceError("VideoSurface::Resource::~Resource(): gl error %d\n", gl_error);
        }

        if (device->va_available) {
            // return VA surface to the free list, decoder owns them
            if (decoder)
                decoder->free_list.push_back(rt_idx);
        }

    } catch (...) {
        traceError("VideoSurface::Resource::~Resource(): caught exception\n");
    }
}

VdpStatus
CreateImpl(VdpDevice device_id, VdpChromaType chroma_type, uint32_t width, uint32_t height,
           VdpVideoSurface *surface)
{
    if (!surface)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

    auto data = make_shared<Resource>(device, chroma_type, width, height);

    *surface = ResourceStorage<Resource>::instance().insert(data);
    return VDP_STATUS_OK;
}

VdpStatus
Create(VdpDevice device_id, VdpChromaType chroma_type, uint32_t width, uint32_t height,
       VdpVideoSurface *surface)
{
    return check_for_exceptions(CreateImpl, device_id, chroma_type, width, height, surface);
}

VdpStatus
DestroyImpl(VdpVideoSurface surface_id)
{
    ResourceRef<Resource> surf{surface_id};

    ResourceStorage<Resource>::instance().drop(surface_id);
    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpVideoSurface surface_id)
{
    return check_for_exceptions(DestroyImpl, surface_id);
}

VdpStatus
GetBitsYCbCrImpl(VdpVideoSurface surface_id, VdpYCbCrFormat destination_ycbcr_format,
                 void *const *destination_data, uint32_t const *destination_pitches)
{
    if (!destination_data || !destination_pitches)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<Resource> surf{surface_id};
    VADisplay va_dpy = surf->device->va_dpy;

    if (surf->device->va_available) {
        VAImage q;
        vaDeriveImage(va_dpy, surf->va_surf, &q);
        if (q.format.fourcc == VA_FOURCC('N', 'V', '1', '2') &&
            destination_ycbcr_format == VDP_YCBCR_FORMAT_NV12)
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
                uint8_t *dst = static_cast<uint8_t *>(destination_data[0]);
                for (unsigned int y = 0; y < q.height; y ++) {  // Y plane
                    memcpy (dst, src, q.width);
                    src += q.pitches[0];
                    dst += destination_pitches[0];
                }
                src = img_data + q.offsets[1];
                dst = static_cast<uint8_t *>(destination_data[1]);
                for (unsigned int y = 0; y < q.height / 2; y ++) {  // UV plane
                    memcpy(dst, src, q.width);  // q.width/2 samples of U and V each, hence q.width
                    src += q.pitches[1];
                    dst += destination_pitches[1];
                }
            }
            vaUnmapBuffer(va_dpy, q.buf);
        } else if (q.format.fourcc == VA_FOURCC('N', 'V', '1', '2') &&
                   destination_ycbcr_format == VDP_YCBCR_FORMAT_YV12)
        {
            uint8_t *img_data;
            vaMapBuffer(va_dpy, q.buf, (void **)&img_data);

            // Y plane
            if (destination_pitches[0] == q.pitches[0]) {
                const uint32_t sz = (uint32_t)q.width * (uint32_t)q.height;
                memcpy(destination_data[0], img_data + q.offsets[0], sz);
            } else {
                uint8_t *src = img_data + q.offsets[0];
                uint8_t *dst = static_cast<uint8_t *>(destination_data[0]);
                for (unsigned int y = 0; y < q.height; y ++) {
                    memcpy(dst, src, q.width);
                    src += q.pitches[0];
                    dst += destination_pitches[0];
                }
            }

            // unpack mixed UV to separate planes
            for (unsigned int y = 0; y < q.height/2; y ++) {
                uint8_t *src = img_data + q.offsets[1] + y * q.pitches[1];
                uint8_t *dst_u = static_cast<uint8_t *>(destination_data[1]) +
                                 y * destination_pitches[1];
                uint8_t *dst_v = static_cast<uint8_t *>(destination_data[2]) +
                                 y * destination_pitches[2];

                for (unsigned int x = 0; x < q.width/2; x++) {
                    *dst_v++ = *src++;
                    *dst_u++ = *src++;
                }
            }

            vaUnmapBuffer(va_dpy, q.buf);
        } else {
            const char *c = (const char *)&q.format.fourcc;
            traceError("VideoSurface::GetBitsYCbCrImpl(): not implemented conversion VA FOURCC "
                       "%c%c%c%c -> %s\n", *c, *(c+1), *(c+2), *(c+3),
                       reverse_ycbcr_format(destination_ycbcr_format));
            vaDestroyImage(va_dpy, q.image_id);
            return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
        }

        vaDestroyImage(va_dpy, q.image_id);
    } else {
        // software fallback
        traceError("VideoSurface::GetBitsYCbCrImpl(): not implemented software fallback\n");
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
GetBitsYCbCr(VdpVideoSurface surface_id, VdpYCbCrFormat destination_ycbcr_format,
             void *const *destination_data, uint32_t const *destination_pitches)
{
    return check_for_exceptions(GetBitsYCbCrImpl, surface_id, destination_ycbcr_format,
                                destination_data, destination_pitches);
}

VdpStatus
GetParametersImpl(VdpVideoSurface surface_id, VdpChromaType *chroma_type, uint32_t *width,
                  uint32_t *height)
{
    ResourceRef<Resource> surf{surface_id};

    if (chroma_type)
        *chroma_type = surf->chroma_type;

    if (width)
        *width = surf->width;

    if (height)
        *height = surf->height;

    return VDP_STATUS_OK;
}

VdpStatus
GetParameters(VdpVideoSurface surface_id, VdpChromaType *chroma_type, uint32_t *width,
              uint32_t *height)
{
    return check_for_exceptions(GetParametersImpl, surface_id, chroma_type, width, height);
}

VdpStatus
PutBitsYCbCr_swscale(VdpVideoSurface surface_id, VdpYCbCrFormat source_ycbcr_format,
                     void const *const *source_data, uint32_t const *source_pitches)
{
    // TODO: implement this
    return VDP_STATUS_OK;
}

VdpStatus
PutBitsYCbCr_glsl(VdpVideoSurface surface_id, VdpYCbCrFormat source_ycbcr_format,
                  void const *const *source_data, uint32_t const *source_pitches)
{
    if (!source_data || !source_pitches)
        return VDP_STATUS_INVALID_POINTER;

    // TODO: implement VDP_YCBCR_FORMAT_UYVY
    // TODO: implement VDP_YCBCR_FORMAT_YUYV
    // TODO: implement VDP_YCBCR_FORMAT_Y8U8V8A8
    // TODO: implement VDP_YCBCR_FORMAT_V8U8Y8A8

    ResourceRef<Resource> surf{surface_id};

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
        traceError("VideoSurface::PutBitsYCbCr_glsl(): not implemented source YCbCr format '%s'\n",
                   reverse_ycbcr_format(source_ycbcr_format));
        return VDP_STATUS_INVALID_Y_CB_CR_FORMAT;
    }

    GLXThreadLocalContext guard{surf->device};

    glBindFramebuffer(GL_FRAMEBUFFER, surf->fbo_id);

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->width/2, surf->height/2, 0,
                     GL_RG, GL_UNSIGNED_BYTE, source_data[1]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // Y plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->width, surf->height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, source_data[0]);
        break;

    case VDP_YCBCR_FORMAT_YV12:
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex_id[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->width/2, surf->height, 0,
                     GL_RED, GL_UNSIGNED_BYTE, NULL);
        // U plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[2]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->width/2, surf->height/2, GL_RED,
                        GL_UNSIGNED_BYTE, source_data[2]);
        // V plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[1]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, surf->height/2, surf->width/2,
                        surf->height/2, GL_RED, GL_UNSIGNED_BYTE, source_data[1]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // Y plane
        glPixelStorei(GL_UNPACK_ROW_LENGTH, source_pitches[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->width, surf->height, 0, GL_RED,
                     GL_UNSIGNED_BYTE, source_data[0]);
        break;
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, surf->width, 0, surf->height, -1.0f, 1.0f);
    glViewport(0, 0, surf->width, surf->height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glDisable(GL_BLEND);

    switch (source_ycbcr_format) {
    case VDP_YCBCR_FORMAT_NV12:
        glUseProgram(surf->device->shaders[glsl_NV12_RGBA].program);
        glUniform1i(surf->device->shaders[glsl_NV12_RGBA].uniform.tex_0, 0);
        glUniform1i(surf->device->shaders[glsl_NV12_RGBA].uniform.tex_1, 1);
        break;

    case VDP_YCBCR_FORMAT_YV12:
        glUseProgram(surf->device->shaders[glsl_YV12_RGBA].program);
        glUniform1i(surf->device->shaders[glsl_YV12_RGBA].uniform.tex_0, 0);
        glUniform1i(surf->device->shaders[glsl_YV12_RGBA].uniform.tex_1, 1);
        break;
    }

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0,           0);
        glTexCoord2f(1, 0); glVertex2f(surf->width, 0);
        glTexCoord2f(1, 1); glVertex2f(surf->width, surf->height);
        glTexCoord2f(0, 1); glVertex2f(0,           surf->height);
    glEnd();

    glUseProgram(0);
    glFinish();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteTextures(2, tex_id);

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("VideoSurface::PutBitsYCbCr_glsl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
PutBitsYCbCrImpl(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
                 void const *const *source_data, uint32_t const *source_pitches)
{
    int using_glsl = 1;
    VdpStatus ret;

    if (using_glsl) {
        ret = PutBitsYCbCr_glsl(surface, source_ycbcr_format, source_data, source_pitches);
    } else {
        ret = PutBitsYCbCr_swscale(surface, source_ycbcr_format, source_data,
                                                  source_pitches);
    }

    return ret;
}

VdpStatus
PutBitsYCbCr(VdpVideoSurface surface, VdpYCbCrFormat source_ycbcr_format,
             void const *const *source_data, uint32_t const *source_pitches)
{
    return check_for_exceptions(PutBitsYCbCrImpl, surface, source_ycbcr_format, source_data,
                                source_pitches);
}

VdpStatus
QueryCapabilitiesImpl(VdpDevice device, VdpChromaType surface_chroma_type, VdpBool *is_supported,
                      uint32_t *max_width, uint32_t *max_height)
{
    // TODO: don't ignore
    std::ignore = device;
    std::ignore = surface_chroma_type;

    // TODO: implement
    if (is_supported)
        *is_supported = 1;

    if (max_width)
        *max_width = 1920;

    if (max_height)
        *max_height = 1080;

    return VDP_STATUS_OK;
}

VdpStatus
QueryCapabilities(VdpDevice device, VdpChromaType surface_chroma_type, VdpBool *is_supported,
                  uint32_t *max_width, uint32_t *max_height)
{
    return check_for_exceptions(QueryCapabilitiesImpl, device, surface_chroma_type, is_supported,
                                max_width, max_height);
}

VdpStatus
QueryGetPutBitsYCbCrCapabilitiesImpl(VdpDevice device, VdpChromaType surface_chroma_type,
                                     VdpYCbCrFormat bits_ycbcr_format, VdpBool *is_supported)
{
    // TODO: don't ignore
    std::ignore = device;
    std::ignore = surface_chroma_type;
    std::ignore = bits_ycbcr_format;

    // TODO: implement
    if (is_supported)
        *is_supported = 1;

    return VDP_STATUS_OK;
}

VdpStatus
QueryGetPutBitsYCbCrCapabilities(VdpDevice device, VdpChromaType surface_chroma_type,
                                 VdpYCbCrFormat bits_ycbcr_format, VdpBool *is_supported)
{
    return check_for_exceptions(QueryGetPutBitsYCbCrCapabilitiesImpl, device, surface_chroma_type,
                                bits_ycbcr_format, is_supported);
}

} } // namespace vdp::VideoSurface
