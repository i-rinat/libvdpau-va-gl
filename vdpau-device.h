/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#ifndef __VDPAU_DEVICE_H
#define __VDPAU_DEVICE_H

#include "handle-storage.h"
#include <va/va.h>

typedef struct {
    HandleType  type;
    void       *self;
    int         refcount;
    Display    *display;
    Display    *display_orig;
    int         screen;
    GLXContext  glc;
    Window      root;
    GLuint      fbo_id;
    VADisplay   va_dpy;
    int         va_available;
    int         va_version_major;
    int         va_version_minor;
    GLuint      watermark_tex_id;
} VdpDeviceData;

#endif /* __VDPAU_DEVICE_H */
