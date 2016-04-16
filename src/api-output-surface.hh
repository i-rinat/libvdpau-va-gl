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

#pragma once

#include "api.hh"
#include <GL/gl.h>
#include <memory>
#include <vdpau/vdpau.h>


namespace vdp { namespace OutputSurface {

struct Resource: public vdp::GenericResource
{
    Resource(std::shared_ptr<vdp::Device::Resource> a_device, VdpRGBAFormat a_rgba_format,
             uint32_t a_width, uint32_t a_height);

    ~Resource();

    VdpRGBAFormat   rgba_format;        ///< RGBA format of data stored
    GLuint          tex_id;             ///< associated GL texture id
    GLuint          fbo_id;             ///< framebuffer object id
    uint32_t        width;
    uint32_t        height;
    GLuint          gl_internal_format; ///< GL texture format: internal format
    GLuint          gl_format;          ///< GL texture format: preferred external format
    GLuint          gl_type;            ///< GL texture format: pixel type
    unsigned int    bytes_per_pixel;    ///< number of bytes per pixel
    VdpTime         first_presentation_time;    ///< first displayed time in queue
    VdpPresentationQueueStatus  status; ///< status in presentation queue
};

VdpOutputSurfaceQueryCapabilities                   QueryCapabilities;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities   QueryGetPutBitsNativeCapabilities;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities     QueryPutBitsIndexedCapabilities;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities       QueryPutBitsYCbCrCapabilities;
VdpOutputSurfaceCreate          Create;
VdpOutputSurfaceDestroy         Destroy;
VdpOutputSurfaceGetParameters   GetParameters;
VdpOutputSurfaceGetBitsNative   GetBitsNative;
VdpOutputSurfacePutBitsNative   PutBitsNative;
VdpOutputSurfacePutBitsIndexed  PutBitsIndexed;
VdpOutputSurfacePutBitsYCbCr    PutBitsYCbCr;
VdpOutputSurfaceRenderOutputSurface RenderOutputSurface;
VdpOutputSurfaceRenderBitmapSurface RenderBitmapSurface;

} } // namespace vdp::OutputSurface
