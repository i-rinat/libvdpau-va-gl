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

#include "api-decoder.hh"
#include "api.hh"
#include <GL/gl.h>
#include <memory>


namespace vdp { namespace VideoSurface {

struct Resource: public vdp::GenericResource
{
    Resource(std::shared_ptr<vdp::Device::Resource> a_device, VdpChromaType a_chroma_type,
             uint32_t a_width, uint32_t a_height);

    ~Resource();

    VdpChromaType   chroma_type;    ///< video chroma type
    uint32_t        width;
    uint32_t        height;
    uint32_t        stride;
    uint32_t        chroma_width;
    uint32_t        chroma_height;
    uint32_t        chroma_stride;
    VASurfaceID     va_surf;        ///< VA-API surface
    bool            sync_va_to_glx; ///< whenever VA-API surface should be converted to GL texture
    GLuint          tex_id;         ///< GL texture id (RGBA)
    GLuint          fbo_id;         ///< framebuffer object id
    int32_t         rt_idx;         ///< index in VdpDecoder's render_targets
    std::vector<uint8_t>    y_plane;
    std::vector<uint8_t>    u_plane;
    std::vector<uint8_t>    v_plane;

    std::shared_ptr<vdp::Decoder::Resource> decoder;        ///< associated VdpDecoder
};

VdpVideoSurfaceQueryCapabilities                QueryCapabilities;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities QueryGetPutBitsYCbCrCapabilities;
VdpVideoSurfaceCreate                           Create;
VdpVideoSurfaceDestroy                          Destroy;
VdpVideoSurfaceGetParameters                    GetParameters;
VdpVideoSurfaceGetBitsYCbCr                     GetBitsYCbCr;
VdpVideoSurfacePutBitsYCbCr                     PutBitsYCbCr;

} } // namespace vdp::VideoSurface
