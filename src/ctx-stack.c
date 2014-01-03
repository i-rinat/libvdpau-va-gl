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
#include "trace.h"
#include <sys/syscall.h>
#include <unistd.h>

static __thread Display    *glx_ctx_stack_display;
static __thread Drawable    glx_ctx_stack_wnd;
static __thread GLXContext  glx_ctx_stack_glc;
static __thread int         glx_ctx_stack_same;
static __thread int         glx_ctx_stack_element_count = 0;
static          GHashTable *glc_hash_table = NULL;
static          int         glc_hash_table_ref_count = 0;
static          GLXContext  root_glc = NULL;
static          XVisualInfo *root_vi = NULL;
static          int         x11_error_code = 0;
static          void       *x11_prev_handler = NULL;
static          pthread_mutex_t glx_ctx_stack_mutex = PTHREAD_MUTEX_INITIALIZER;

void
glx_context_lock(void)
{
    pthread_mutex_lock(&glx_ctx_stack_mutex);
}

void
glx_context_unlock(void)
{
    pthread_mutex_unlock(&glx_ctx_stack_mutex);
}

void
glx_context_push_global(Display *dpy, Drawable wnd, GLXContext glc)
{
    glx_context_lock();
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
    glx_context_lock();
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

    glx_context_unlock();
}

void
glx_context_ref_glc_hash_table(Display *dpy, int screen)
{
    glx_context_lock();
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
    glx_context_unlock();
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
    glx_context_lock();
    glc_hash_table_ref_count --;
    if (0 == glc_hash_table_ref_count) {
        g_hash_table_foreach(glc_hash_table, glc_hash_destroy_func, dpy);
        g_hash_table_unref(glc_hash_table);
        glc_hash_table = NULL;

        glXDestroyContext(dpy, root_glc);
        XFree(root_vi);
    }
    glx_context_unlock();
}

GLXContext
glx_context_get_root_context(void)
{
    return root_glc;
}

static
int
x11_error_handler(Display *dpy, XErrorEvent *ee)
{
    x11_error_code = ee->error_code;
    return 0;
}

void
x11_push_eh(void)
{
    x11_error_code = 0;
    void *ptr = XSetErrorHandler(&x11_error_handler);

    if (ptr != x11_error_handler)
        x11_prev_handler = ptr;
}

int
x11_pop_eh(void)
{
    // Although this looks like right thing to do, brief testing shows it's highly unstable.
    // So this code will stay here commented out as a reminder.
/*
    void *ptr = XSetErrorHandler(x11_prev_handler);
    if (ptr != x11_error_handler) {
        // if someone have managed to set own handler after ours, restore it
        void *ptr2 = XSetErrorHandler(ptr);
        if (ptr != ptr2) {
            // someone again has set another handler
            traceError("warning (%s): someone set X error handler while restore previous\n",
                       __func__);
            XSetErrorHandler(ptr2);
        }
    }
*/
    return x11_error_code;
}
