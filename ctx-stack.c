/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
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
static __thread int glx_ctx_stack_same;
static __thread int glx_ctx_stack_element_count = 0;
GHashTable     *glc_hash_table = NULL;
int             glc_hash_table_ref_count = 0;
GLXContext      root_glc;
XVisualInfo    *root_vi;

void
glx_context_push_global(Display *dpy, Drawable wnd, GLXContext glc)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    assert(0 == glx_ctx_stack_element_count);

    glx_ctx_stack_display = glXGetCurrentDisplay();
    glx_ctx_stack_wnd =     glXGetCurrentDrawable();
    glx_ctx_stack_glc =     glXGetCurrentContext();
    glx_ctx_stack_same =    0;
    glx_ctx_stack_element_count ++;

    if (dpy == glx_ctx_stack_display && wnd == glx_ctx_stack_wnd && glc == glx_ctx_stack_glc) {
        // Same context. Don't call MakeCurrent.
        glx_ctx_stack_same = 1;
    } else {
        glx_ctx_stack_same = 0;
        glXMakeCurrent(dpy, wnd, glc);
    }
}

void
glx_context_push_thread_local(VdpDeviceData *deviceData)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    Display *dpy = deviceData->display;
    const Window wnd = deviceData->root;
    const gint thread_id = (gint) syscall(__NR_gettid);

    GLXContext glc = g_hash_table_lookup(glc_hash_table, GINT_TO_POINTER(thread_id));
    if (!glc) {
        glc = glXCreateContext(dpy, root_vi, root_glc, GL_TRUE);
        assert(glc);
        g_hash_table_insert(glc_hash_table, GINT_TO_POINTER(thread_id), glc);
    }

    glx_ctx_stack_display = glXGetCurrentDisplay();
    glx_ctx_stack_wnd =     glXGetCurrentDrawable();
    glx_ctx_stack_glc =     glXGetCurrentContext();
    glx_ctx_stack_element_count ++;

    if (dpy == glx_ctx_stack_display && wnd == glx_ctx_stack_wnd && glc == glx_ctx_stack_glc) {
        // Same context. Don't call MakeCurrent.
        glx_ctx_stack_same = 1;
    } else {
        glx_ctx_stack_same = 0;
        glXMakeCurrent(dpy, wnd, glc);
    }
}

void
glx_context_pop()
{
    assert(1 == glx_ctx_stack_element_count);

    if (!glx_ctx_stack_same) {
        if (glx_ctx_stack_display)
            glXMakeCurrent(glx_ctx_stack_display, glx_ctx_stack_wnd, glx_ctx_stack_glc);
    }

    glx_ctx_stack_element_count --;

    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}

void
glx_context_ref_glc_hash_table(Display *dpy, int screen)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    if (0 == glc_hash_table_ref_count) {
        glc_hash_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        glc_hash_table_ref_count = 1;

        GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        root_vi = glXChooseVisual(dpy, screen, att);
        if (NULL == root_vi) {
            traceError("error (glx_context_ref_glc_hash_table): glXChooseVisual failed\n");
            return;
        }
        root_glc = glXCreateContext(dpy, root_vi, NULL, GL_TRUE);
    } else {
        glc_hash_table_ref_count ++;
    }
    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
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
glx_context_unref_glc_hash_table(Display *dpy)
{
    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    glc_hash_table_ref_count --;
    if (0 == glc_hash_table_ref_count) {
        g_hash_table_foreach(glc_hash_table, glc_hash_destroy_func, dpy);
        g_hash_table_unref(glc_hash_table);
        glc_hash_table = NULL;

        glXDestroyContext(dpy, root_glc);
        XFree(root_vi);
    }
    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);
}

GLXContext
glx_context_get_root_context(void)
{
    return root_glc;
}
