/*
 * Copyright 2013-2014  Rinat Ibragimov
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

#ifndef VA_GL_SRC_REVERSE_CONSTANT_H
#define VA_GL_SRC_REVERSE_CONSTANT_H

#include <vdpau/vdpau.h>

const char * reverse_status(VdpStatus status);
const char * reverse_func_id(VdpFuncId func_id);
const char * reverse_video_mixer_feature(VdpVideoMixerFeature mixer_feature);
const char * reverse_video_mixer_parameter(VdpVideoMixerParameter parameter);
const char * reverse_video_mixer_attribute(VdpVideoMixerAttribute attr);
const char * reverse_rgba_format(VdpRGBAFormat rgba_format);
const char * reverse_chroma_type(VdpChromaType chroma_type);
const char * reverse_ycbcr_format(VdpYCbCrFormat ycbcr_format);
const char * reverser_video_mixer_picture_structure(VdpVideoMixerPictureStructure s);
const char * reverse_blend_factor(VdpOutputSurfaceRenderBlendFactor blend_factor);
const char * reverse_blend_equation(VdpOutputSurfaceRenderBlendEquation blend_equation);
const char * reverse_decoder_profile(VdpDecoderProfile profile);
const char * reverse_indexed_format(VdpIndexedFormat indexed_format);
const char * reverse_color_table_format(VdpColorTableFormat color_table_format);
const char * reverse_color_standard(VdpColorStandard color_standard);
const char * reverse_output_surface_render_rotate(int flags);

#endif /* VA_GL_SRC_REVERSE_CONSTANT_H */
