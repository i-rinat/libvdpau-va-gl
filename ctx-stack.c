/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

/*
 *  glx context stack
 */

#include "ctx-stack.h"
#include "globals.h"
#include <assert.h>
#include "vdpau-trace.h"
#include "vdpau-locking.h"

static __thread Display *glx_ctx_stack_display;
static __thread Drawable glx_ctx_stack_wnd;
static __thread GLXContext glx_ctx_stack_glc;
static __thread int glx_ctx_stack_element_count = 0;

void
glx_context_push(Display *dpy, Drawable wnd, GLXContext glc)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    assert(0 == glx_ctx_stack_element_count);

    glx_ctx_stack_display = glXGetCurrentDisplay();
    glx_ctx_stack_wnd =     glXGetCurrentDrawable();
    glx_ctx_stack_glc =     glXGetCurrentContext();
    glx_ctx_stack_element_count ++;

    traceInfo("glc push: 0x%p, %d, 0x%p\n", glx_ctx_stack_display, glx_ctx_stack_wnd, glx_ctx_stack_glc);
    traceInfo("glc set: 0x%p, %d, 0x%p\n", dpy, wnd, glc);

    locked_glXMakeCurrent(dpy, wnd, glc);

    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}

void
glx_context_pop()
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    assert(1 == glx_ctx_stack_element_count);

    traceInfo("glc pop stage 1: 0x%p, %d, 0x%p\n", glx_ctx_stack_display, glx_ctx_stack_wnd, glx_ctx_stack_glc);
    if (glx_ctx_stack_display)
        locked_glXMakeCurrent(glx_ctx_stack_display, glx_ctx_stack_wnd, glx_ctx_stack_glc);

    glx_ctx_stack_element_count --;
    traceInfo("glc pop stage 2\n");

    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}
