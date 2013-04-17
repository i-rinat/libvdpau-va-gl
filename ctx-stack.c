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

#define _GNU_SOURCE
#include "ctx-stack.h"
#include "globals.h"
#include <assert.h>
#include "vdpau-trace.h"
#include "vdpau-locking.h"
#include <sys/syscall.h>
#include <unistd.h>

static __thread Display *glx_ctx_stack_display;
static __thread Drawable glx_ctx_stack_wnd;
static __thread GLXContext glx_ctx_stack_glc;
static __thread int glx_ctx_stack_element_count = 0;

void
glx_context_push_global(Display *dpy, Drawable wnd, GLXContext glc)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    assert(0 == glx_ctx_stack_element_count);

    glx_ctx_stack_display = glXGetCurrentDisplay();
    glx_ctx_stack_wnd =     glXGetCurrentDrawable();
    glx_ctx_stack_glc =     glXGetCurrentContext();
    glx_ctx_stack_element_count ++;

    locked_glXMakeCurrent(dpy, wnd, glc);

    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}

void
glx_context_push_thread_local(VdpDeviceData *deviceData)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    Display *dpy = deviceData->display;
    const Window wnd = deviceData->root;
    const gint thread_id = (gint) syscall(__NR_gettid);

    GLXContext glc = g_hash_table_lookup(deviceData->glc_hash_table, GINT_TO_POINTER(thread_id));
    if (!glc) {
        glc = glXCreateContext(dpy, deviceData->vi, deviceData->glc, GL_TRUE);
        assert(glc);
        g_hash_table_insert(deviceData->glc_hash_table, GINT_TO_POINTER(thread_id), glc);
    }

    glx_ctx_stack_display = glXGetCurrentDisplay();
    glx_ctx_stack_wnd =     glXGetCurrentDrawable();
    glx_ctx_stack_glc =     glXGetCurrentContext();
    glx_ctx_stack_element_count ++;

    locked_glXMakeCurrent(dpy, wnd, glc);

    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}

void
glx_context_pop()
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    assert(1 == glx_ctx_stack_element_count);

    if (glx_ctx_stack_display)
        locked_glXMakeCurrent(glx_ctx_stack_display, glx_ctx_stack_wnd, glx_ctx_stack_glc);

    glx_ctx_stack_element_count --;

    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}

GHashTable *
glx_context_new_glc_hash_table(void)
{
    return g_hash_table_new(g_direct_hash, g_direct_equal);
}

static
void
glc_hash_destroy_func(gpointer key, gpointer value, gpointer user_data)
{
    (void)key;
    GLXContext glc = value;
    Display *dpy = user_data;
    glXDestroyContext(dpy, glc);
}

void
glx_context_destroy_glc_hash_table(Display *dpy, GHashTable *ht)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    XLockDisplay(dpy);
    g_hash_table_foreach(ht, glc_hash_destroy_func, dpy);
    XUnlockDisplay(dpy);
    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}
