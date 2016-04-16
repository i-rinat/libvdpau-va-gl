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
#include <memory>
#include <stdint.h>
#include <va/va.h>
#include <vdpau/vdpau.h>
#include <vector>


namespace vdp { namespace Decoder {

struct Resource: public vdp::GenericResource
{
    Resource(std::shared_ptr<vdp::Device::Resource> a_device, VdpDecoderProfile a_profile,
             uint32_t a_width, uint32_t a_height, uint32_t n_max_references);

    ~Resource();

    VdpDecoderProfile   profile;        ///< decoder profile
    uint32_t            width;
    uint32_t            height;
    uint32_t            max_references; ///< maximum count of reference frames
    VAConfigID          config_id;      ///< VA-API config id
    VAContextID         context_id;     ///< VA-API context id

    std::vector<VASurfaceID>    render_targets; ///< spare VA surfaces
    std::vector<int32_t>        free_list;
};

VdpDecoderQueryCapabilities QueryCapabilities;
VdpDecoderCreate            Create;
VdpDecoderDestroy           Destroy;
VdpDecoderGetParameters     GetParameters;
VdpDecoderRender            Render;

} } // namespace vdp::Decoder
