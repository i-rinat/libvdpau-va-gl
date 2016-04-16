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


namespace vdp { namespace VideoMixer {

struct Resource: public vdp::GenericResource
{
    Resource(std::shared_ptr<vdp::Device::Resource> a_device, uint32_t a_feature_count,
             VdpVideoMixerFeature const *a_features, uint32_t a_parameter_count,
             VdpVideoMixerParameter const *a_parameters, void const *const *a_parameter_values);

    ~Resource();

    void
    free_video_mixer_pixmaps();

    uint32_t        pixmap_width;       ///< last seen width
    uint32_t        pixmap_height;      ///< last seen height
    Pixmap          pixmap;             ///< target pixmap for vaPutSurface
    GLXPixmap       glx_pixmap;         ///< associated glx pixmap for texture-from-pixmap
    GLuint          tex_id;             ///< texture for texture-from-pixmap
};

VdpVideoMixerQueryFeatureSupport        QueryFeatureSupport;
VdpVideoMixerQueryParameterSupport      QueryParameterSupport;
VdpVideoMixerQueryAttributeSupport      QueryAttributeSupport;
VdpVideoMixerQueryParameterValueRange   QueryParameterValueRange;
VdpVideoMixerQueryAttributeValueRange   QueryAttributeValueRange;
VdpVideoMixerCreate                     Create;
VdpVideoMixerSetFeatureEnables          SetFeatureEnables;
VdpVideoMixerSetAttributeValues         SetAttributeValues;
VdpVideoMixerGetFeatureSupport          GetFeatureSupport;
VdpVideoMixerGetFeatureEnables          GetFeatureEnables;
VdpVideoMixerGetParameterValues         GetParameterValues;
VdpVideoMixerGetAttributeValues         GetAttributeValues;
VdpVideoMixerDestroy                    Destroy;
VdpVideoMixerRender                     Render;

} } // namespace vdp::VideoMixer
