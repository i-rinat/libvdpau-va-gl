/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#include "vdpau-locking.h"
#include "vdpau-soft.h"
#include "vdpau-trace.h"
#include <assert.h>

// wrappers for glX functions

Bool
locked_glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
    XLockDisplay(dpy);
    Bool ret = glXMakeCurrent(dpy, drawable, ctx);
    XUnlockDisplay(dpy);
    return ret;
}

void
locked_glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    XLockDisplay(dpy);
    glXSwapBuffers(dpy, drawable);
    XUnlockDisplay(dpy);
}
