/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef VA_GL_SRC_PIXFMT_COMPAT_H
#define VA_GL_SRC_PIXFMT_COMPAT_H

#include <libavutil/pixfmt.h>

#if (LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 42, 0)) || \
    (LIBAVUTIL_VERSION_INT == AV_VERSION_INT(51, 73, 101))

#define AV_PIX_FMT_NONE         PIX_FMT_NONE
#define AV_PIX_FMT_NV12         PIX_FMT_NV12
#define AV_PIX_FMT_YUV420P      PIX_FMT_YUV420P
#define AV_PIX_FMT_UYVY422      PIX_FMT_UYVY422
#define AV_PIX_FMT_YUYV422      PIX_FMT_YUYV422

#endif // old libavutil version

#endif // VA_GL_SRC_PIXFMT_COMPAT_H
