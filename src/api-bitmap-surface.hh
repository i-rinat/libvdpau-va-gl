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

#include "api-device.hh"
#include "api.hh"
#include "compat-vdpau.hh"
#include <GL/gl.h>
#include <vector>


namespace vdp { namespace BitmapSurface {

struct Resource: public vdp::GenericResource
{
    Resource(std::shared_ptr<vdp::Device::Resource> a_device, VdpRGBAFormat a_rgba_format,
             uint32_t a_width, uint32_t a_height, VdpBool a_frequently_accessed);

    ~Resource();

    VdpRGBAFormat   rgba_format;        ///< RGBA format of data stored
    GLuint          tex_id;             ///< GL texture id
    uint32_t        width;
    uint32_t        height;
    VdpBool         frequently_accessed;///< 1 if surface should be optimized for frequent access
    unsigned int    bytes_per_pixel;    ///< number of bytes per bitmap pixel
    GLuint          gl_internal_format; ///< GL texture format: internal format
    GLuint          gl_format;          ///< GL texture format: preferred external format
    GLuint          gl_type;            ///< GL texture format: pixel type
    std::vector<char>   bitmap_data;    ///< system-memory buffer for frequently accessed bitmaps
    bool                dirty;          ///< dirty flag. True if system-memory buffer contains data
                                        ///< newer than GPU texture contents
};

VdpBitmapSurfaceQueryCapabilities   QueryCapabilities;
VdpBitmapSurfaceCreate              Create;
VdpBitmapSurfaceDestroy             Destroy;
VdpBitmapSurfaceGetParameters       GetParameters;
VdpBitmapSurfacePutBitsNative       PutBitsNative;

} } // namespace vdp::BitmapSurface
