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

#include "reverse-constant.hh"


#define CASE(q) case q: return #q


const char *
reverse_rgba_format(VdpRGBAFormat rgba_format)
{
    switch (rgba_format) {
    CASE(VDP_RGBA_FORMAT_B8G8R8A8);
    CASE(VDP_RGBA_FORMAT_R8G8B8A8);
    CASE(VDP_RGBA_FORMAT_R10G10B10A2);
    CASE(VDP_RGBA_FORMAT_B10G10R10A2);
    CASE(VDP_RGBA_FORMAT_A8);
    default:
        return "Unknown RGBA format";
    }
}

const char *
reverse_ycbcr_format(VdpYCbCrFormat ycbcr_format)
{
    switch (ycbcr_format) {
    CASE(VDP_YCBCR_FORMAT_NV12);
    CASE(VDP_YCBCR_FORMAT_YV12);
    CASE(VDP_YCBCR_FORMAT_UYVY);
    CASE(VDP_YCBCR_FORMAT_YUYV);
    CASE(VDP_YCBCR_FORMAT_Y8U8V8A8);
    CASE(VDP_YCBCR_FORMAT_V8U8Y8A8);
    default:
        return "Unknown YCbCr format";
    }
}

const char *
reverse_decoder_profile(VdpDecoderProfile profile)
{
    switch (profile) {
    CASE(VDP_DECODER_PROFILE_MPEG1);
    CASE(VDP_DECODER_PROFILE_MPEG2_SIMPLE);
    CASE(VDP_DECODER_PROFILE_MPEG2_MAIN);
    CASE(VDP_DECODER_PROFILE_H264_CONSTRAINED_BASELINE);
    CASE(VDP_DECODER_PROFILE_H264_BASELINE);
    CASE(VDP_DECODER_PROFILE_H264_MAIN);
    CASE(VDP_DECODER_PROFILE_H264_HIGH);
    CASE(VDP_DECODER_PROFILE_VC1_SIMPLE);
    CASE(VDP_DECODER_PROFILE_VC1_MAIN);
    CASE(VDP_DECODER_PROFILE_VC1_ADVANCED);
    CASE(VDP_DECODER_PROFILE_MPEG4_PART2_SP);
    CASE(VDP_DECODER_PROFILE_MPEG4_PART2_ASP);
    CASE(VDP_DECODER_PROFILE_DIVX4_QMOBILE);
    CASE(VDP_DECODER_PROFILE_DIVX4_MOBILE);
    CASE(VDP_DECODER_PROFILE_DIVX4_HOME_THEATER);
    CASE(VDP_DECODER_PROFILE_DIVX4_HD_1080P);
    CASE(VDP_DECODER_PROFILE_DIVX5_QMOBILE);
    CASE(VDP_DECODER_PROFILE_DIVX5_MOBILE);
    CASE(VDP_DECODER_PROFILE_DIVX5_HOME_THEATER);
    CASE(VDP_DECODER_PROFILE_DIVX5_HD_1080P);
    default:
        return "Unknown decoder profile";
    }
}

const char *
reverse_status(VdpStatus status)
{
    switch (status) {
    CASE(VDP_STATUS_OK);
    CASE(VDP_STATUS_NO_IMPLEMENTATION);
    CASE(VDP_STATUS_DISPLAY_PREEMPTED);
    CASE(VDP_STATUS_INVALID_HANDLE);
    CASE(VDP_STATUS_INVALID_POINTER);
    CASE(VDP_STATUS_INVALID_CHROMA_TYPE);
    CASE(VDP_STATUS_INVALID_Y_CB_CR_FORMAT);
    CASE(VDP_STATUS_INVALID_RGBA_FORMAT);
    CASE(VDP_STATUS_INVALID_INDEXED_FORMAT);
    CASE(VDP_STATUS_INVALID_COLOR_STANDARD);
    CASE(VDP_STATUS_INVALID_COLOR_TABLE_FORMAT);
    CASE(VDP_STATUS_INVALID_BLEND_FACTOR);
    CASE(VDP_STATUS_INVALID_BLEND_EQUATION);
    CASE(VDP_STATUS_INVALID_FLAG);
    CASE(VDP_STATUS_INVALID_DECODER_PROFILE);
    CASE(VDP_STATUS_INVALID_VIDEO_MIXER_FEATURE);
    CASE(VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER);
    CASE(VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE);
    CASE(VDP_STATUS_INVALID_VIDEO_MIXER_PICTURE_STRUCTURE);
    CASE(VDP_STATUS_INVALID_FUNC_ID);
    CASE(VDP_STATUS_INVALID_SIZE);
    CASE(VDP_STATUS_INVALID_VALUE);
    CASE(VDP_STATUS_INVALID_STRUCT_VERSION);
    CASE(VDP_STATUS_RESOURCES);
    CASE(VDP_STATUS_HANDLE_DEVICE_MISMATCH);
    CASE(VDP_STATUS_ERROR);
    default:
        return "Unknown VDP error";
    }
}

const char *
reverse_indexed_format(VdpIndexedFormat indexed_format)
{
    switch (indexed_format) {
    CASE(VDP_INDEXED_FORMAT_A4I4);
    CASE(VDP_INDEXED_FORMAT_I4A4);
    CASE(VDP_INDEXED_FORMAT_A8I8);
    CASE(VDP_INDEXED_FORMAT_I8A8);
    default:
        return "Unknown indexed format";
    }
}
