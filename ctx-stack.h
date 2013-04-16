/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#ifndef __CTX_STACK_H
#define __CTX_STACK_H

#include <GL/glx.h>

void glx_context_push(Display *dpy, Drawable wnd, GLXContext glc);
void glx_context_pop();

#endif /* __CTX_STACK_H */
