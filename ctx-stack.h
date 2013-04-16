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
#include "vdpau-device.h"

void glx_context_push_global(Display *dpy, Drawable wnd, GLXContext glc);
void glx_context_push_thread_local(VdpDeviceData *deviceData);
void glx_context_pop(void);
GHashTable *glx_context_new_glc_hash_table(void);
void glx_context_destroy_glc_hash_table(Display *dpy, GHashTable *ht);

#endif /* __CTX_STACK_H */
