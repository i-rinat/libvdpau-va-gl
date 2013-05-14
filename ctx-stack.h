/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#ifndef __CTX_STACK_H
#define __CTX_STACK_H

#include "vdpau-soft.h"

void glx_context_push_global(Display *dpy, Drawable wnd, GLXContext glc);
void glx_context_push_thread_local(VdpDeviceData *deviceData);
void glx_context_pop(void);
void glx_context_ref_glc_hash_table(Display *dpy, int screen);
void glx_context_unref_glc_hash_table(Display *dpy);
GLXContext  glx_context_get_root_context(void);
#endif /* __CTX_STACK_H */
