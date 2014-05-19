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
#include <stdlib.h>
#include <string.h>
#ifdef __FreeBSD__
#include <sys/thr.h>
#endif


static __thread struct {
    Display        *dpy;
    Drawable        wnd;
    GLXContext      glc;
    int             element_count;
} ctx_stack = {
    .dpy = NULL,
    .wnd = None,
    .glc = NULL,
};

static  GHashTable *glc_hash_table = NULL;
static  int         glc_hash_table_ref_count = 0;
static  GLXContext  root_glc = NULL;
static  XVisualInfo *root_vi = NULL;
static  int         x11_error_code = 0;
static  void       *x11_prev_handler = NULL;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct val_s {
    Display    *dpy;
    GLXContext  glc;
};

void
glx_ctx_lock(void)
{
    pthread_mutex_lock(&lock);
}

void
glx_ctx_unlock(void)
{
    pthread_mutex_unlock(&lock);
}

static
void
value_destroy_func(gpointer data)
{
    struct val_s *val = data;
    glXMakeCurrent(val->dpy, None, NULL);
    glXDestroyContext(val->dpy, val->glc);
    free(val);
}

static
void *
make_val(Display *dpy, GLXContext glc)
{
    struct val_s *val = malloc(sizeof(struct val_s));
    val->dpy = dpy;
    val->glc = glc;
    return val;
}

static
gboolean
is_thread_expired(gpointer key, gpointer value, gpointer user_data)
{
#ifdef __linux__
    int thread_id = GPOINTER_TO_INT(key);
#elif __FreeBSD__
    long thread_id = (long)key;
#endif
    (void)value;
    (void)user_data;
#ifdef __linux__
    if (kill(thread_id, 0) == 0) {
#elif __FreeBSD__
    if (thr_kill(thread_id, 0) == 0) {
#endif
        // thread still exists, do not delete element
        return FALSE;
    }
    return TRUE;
}

void
glx_ctx_push_global(Display *dpy, Drawable wnd, GLXContext glc)
{
    glx_ctx_lock();
    assert(0 == ctx_stack.element_count);

    ctx_stack.dpy = glXGetCurrentDisplay();
    if (!ctx_stack.dpy)
        ctx_stack.dpy = dpy;
    ctx_stack.wnd =     glXGetCurrentDrawable();
    ctx_stack.glc =     glXGetCurrentContext();
    ctx_stack.element_count ++;

    glXMakeCurrent(dpy, wnd, glc);
}

void
glx_ctx_push_thread_local(VdpDeviceData *deviceData)
{
    glx_ctx_lock();
    Display *dpy = deviceData->display;
    const Window wnd = deviceData->root;
#ifdef __linux__
    int thread_id = (int)syscall(__NR_gettid);
#elif __FreeBSD__
    long thread_id;
    thr_self(&thread_id);
#endif

    ctx_stack.dpy = glXGetCurrentDisplay();
    if (!ctx_stack.dpy)
        ctx_stack.dpy = dpy;
    ctx_stack.wnd =     glXGetCurrentDrawable();
    ctx_stack.glc =     glXGetCurrentContext();
    ctx_stack.element_count ++;

#ifdef __linux__
    struct val_s *val = g_hash_table_lookup(glc_hash_table, GINT_TO_POINTER(thread_id));
#elif __FreeBSD__
    struct val_s *val = g_hash_table_lookup(glc_hash_table, (gconstpointer)thread_id);
#endif
    if (!val) {
        GLXContext glc = glXCreateContext(dpy, root_vi, root_glc, GL_TRUE);
        assert(glc);
        val = make_val(dpy, glc);
#ifdef __linux__
        g_hash_table_insert(glc_hash_table, GINT_TO_POINTER(thread_id), val);
#elif __FreeBSD__
        g_hash_table_insert(glc_hash_table, (gpointer)thread_id, val);
#endif

        // try cleanup expired entries
        g_hash_table_foreach_remove(glc_hash_table, is_thread_expired, NULL);
    }
    assert(val->dpy == dpy);

    glXMakeCurrent(dpy, wnd, val->glc);
}

void
glx_ctx_pop()
{
    assert(1 == ctx_stack.element_count);
    glXMakeCurrent(ctx_stack.dpy, ctx_stack.wnd, ctx_stack.glc);
    ctx_stack.element_count --;
    glx_ctx_unlock();
}

void
glx_ctx_ref_glc_hash_table(Display *dpy, int screen)
{
    glx_ctx_lock();
    if (0 == glc_hash_table_ref_count) {
        glc_hash_table = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                               NULL, value_destroy_func);
        glc_hash_table_ref_count = 1;

        GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
        root_vi = glXChooseVisual(dpy, screen, att);
        if (NULL == root_vi) {
            traceError("error (%s): glXChooseVisual failed\n", __func__);
            glx_ctx_unlock();
            return;
        }
        root_glc = glXCreateContext(dpy, root_vi, NULL, GL_TRUE);
    } else {
        glc_hash_table_ref_count ++;
    }
    glx_ctx_unlock();
}

void
glx_ctx_unref_glc_hash_table(Display *dpy)
{
    glx_ctx_lock();
    glc_hash_table_ref_count --;
    if (0 == glc_hash_table_ref_count) {
        g_hash_table_unref(glc_hash_table);
        glc_hash_table = NULL;

        glXDestroyContext(dpy, root_glc);
        XFree(root_vi);
    }
    glx_ctx_unlock();
}

GLXContext
glx_ctx_get_root_context(void)
{
    return root_glc;
}

static
int
x11_error_handler(Display *dpy, XErrorEvent *ee)
{
    (void)dpy;
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
