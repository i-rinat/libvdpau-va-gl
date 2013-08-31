/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef __VDPAU_LOCKING_H
#define __VDPAU_LOCKING_H

#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include "globals.h"

Bool locked_glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx);
void locked_glXSwapBuffers(Display *dpy, GLXDrawable drawable);

#endif /* __VDPAU_LOCKING_H */
