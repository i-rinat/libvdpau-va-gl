/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define GL_GLEXT_PROTOTYPES
#define _GNU_SOURCE
#define _XOPEN_SOURCE 500
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <vdpau/vdpau.h>
#include <GL/gl.h>
#include "ctx-stack.h"
#include "globals.h"
#include "handle-storage.h"
#include "api.h"
#include "trace.h"
#include "watermark.h"


struct task_s {
    struct timespec         when;
    uint32_t                clip_width;
    uint32_t                clip_height;
    VdpOutputSurface        surface;
    unsigned int            wipe_tasks;
    VdpPresentationQueue    queue_id;
};

static GAsyncQueue *async_q = NULL;
static pthread_t    presentation_thread_id;


static
VdpTime
timespec2vdptime(struct timespec t)
{
    return (uint64_t)t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
}

static
struct timespec
vdptime2timespec(VdpTime t)
{
    struct timespec res;
    res.tv_sec = t / (1000*1000*1000);
    res.tv_nsec = t % (1000*1000*1000);
    return res;
}

VdpStatus
vdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                          VdpOutputSurface surface,
                                          VdpTime *first_presentation_time)

{
    if (!first_presentation_time)
        return VDP_STATUS_INVALID_POINTER;
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;
    handle_release(presentation_queue);

    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;

    // TODO: use locking instead of busy loop
    while (surfData->status != VDP_PRESENTATION_QUEUE_STATUS_IDLE) {
        handle_release(surface);
        usleep(1000);
        surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
        if (!surfData)
            return VDP_STATUS_ERROR;
    }

    *first_presentation_time = surfData->first_presentation_time;
    handle_release(surface);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                       VdpOutputSurface surface, VdpPresentationQueueStatus *status,
                                       VdpTime *first_presentation_time)
{
    if (!status || !first_presentation_time)
        return VDP_STATUS_INVALID_POINTER;
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData) {
        handle_release(presentation_queue);
        return VDP_STATUS_INVALID_HANDLE;
    }

    *status = surfData->status;
    *first_presentation_time = surfData->first_presentation_time;

    handle_release(presentation_queue);
    handle_release(surface);

    return VDP_STATUS_OK;
}

static
void
free_glx_pixmaps(VdpPresentationQueueTargetData *pqTargetData)
{
    Display *dpy = pqTargetData->deviceData->display;

    // if pixmap is None, nothing was allocated
    if (None == pqTargetData->pixmap)
        return;

    glXDestroyGLXPixmap(dpy, pqTargetData->glx_pixmap);
    XFreeGC(dpy, pqTargetData->plain_copy_gc);
    XFreePixmap(dpy, pqTargetData->pixmap);
    pqTargetData->pixmap = None;
}

// create new pixmap, glx pixmap, GC if size has changed.
// This function relies on external serializing Xlib access
static
void
recreate_pixmaps_if_geometry_changed(VdpPresentationQueueTargetData *pqTargetData)
{
    Window          root_wnd;
    int             xpos, ypos;
    unsigned int    width, height, border_width, depth;
    Display        *dpy = pqTargetData->deviceData->display;

    XGetGeometry(dpy, pqTargetData->drawable, &root_wnd, &xpos, &ypos, &width, &height,
                 &border_width, &depth);
    if (width != pqTargetData->drawable_width || height != pqTargetData->drawable_height) {
        free_glx_pixmaps(pqTargetData);
        pqTargetData->drawable_width = width;
        pqTargetData->drawable_height = height;

        pqTargetData->pixmap = XCreatePixmap(dpy, pqTargetData->deviceData->root,
                                             pqTargetData->drawable_width,
                                             pqTargetData->drawable_height, depth);
        XGCValues gc_values = {.function = GXcopy, .graphics_exposures = True };
        pqTargetData->plain_copy_gc = XCreateGC(dpy, pqTargetData->pixmap,
                                                GCFunction | GCGraphicsExposures, &gc_values);
        pqTargetData->glx_pixmap = glXCreateGLXPixmap(dpy, pqTargetData->xvi, pqTargetData->pixmap);
        XSync(dpy, False);
    }
}

static
void
do_presentation_queue_display(struct task_s *task)
{
    VdpPresentationQueueData *pqData =
        handle_acquire(task->queue_id, HANDLETYPE_PRESENTATION_QUEUE);
    if (!pqData)
        return;
    VdpDeviceData *deviceData = pqData->deviceData;
    const VdpOutputSurface surface = task->surface;
    const uint32_t clip_width = task->clip_width;
    const uint32_t clip_height = task->clip_height;

    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (surfData == NULL) {
        handle_release(task->queue_id);
        return;
    }

    glx_ctx_lock();
    recreate_pixmaps_if_geometry_changed(pqData->targetData);
    glXMakeCurrent(deviceData->display, pqData->targetData->glx_pixmap, pqData->targetData->glc);

    const uint32_t target_width  = (clip_width > 0)  ? clip_width  : surfData->width;
    const uint32_t target_height = (clip_height > 0) ? clip_height : surfData->height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, target_width, target_height, 0, -1.0, 1.0);
    glViewport(0, 0, target_width, target_height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/surfData->width, 1.0f/surfData->height, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, surfData->tex_id);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        glTexCoord2i(0, 0);                        glVertex2i(0, 0);
        glTexCoord2i(target_width, 0);             glVertex2i(target_width, 0);
        glTexCoord2i(target_width, target_height); glVertex2i(target_width, target_height);
        glTexCoord2i(0, target_height);            glVertex2i(0, target_height);
    glEnd();

    if (global.quirks.show_watermark) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        glBindTexture(GL_TEXTURE_2D, deviceData->watermark_tex_id);

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();

        glColor4f(1.0, 1.0, 1.0, 0.2);
        glBegin(GL_QUADS);
            glTexCoord2i(0, 0);
            glVertex2i(target_width - watermark_width, target_height - watermark_height);

            glTexCoord2i(1, 0);
            glVertex2i(target_width, target_height - watermark_height);

            glTexCoord2i(1, 1);
            glVertex2i(target_width, target_height);

            glTexCoord2i(0, 1);
            glVertex2i(target_width - watermark_width, target_height);
        glEnd();
    }

    glFinish();
    GLenum gl_error = glGetError();

    x11_push_eh();
    XCopyArea(deviceData->display, pqData->targetData->pixmap, pqData->targetData->drawable,
              pqData->targetData->plain_copy_gc, 0, 0, target_width, target_height, 0, 0);
    XSync(deviceData->display, False);
    int x11_err = x11_pop_eh();
    if (x11_err != Success) {
        char buf[200] = { 0 };
        XGetErrorText(deviceData->display, x11_err, buf, sizeof(buf));
        traceError("warning (%s): caught X11 error %s\n", __func__, buf);
    }
    glx_ctx_unlock();

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    surfData->first_presentation_time = timespec2vdptime(now);
    surfData->status = VDP_PRESENTATION_QUEUE_STATUS_IDLE;

    if (global.quirks.log_pq_delay) {
            const int64_t delta = timespec2vdptime(now) - surfData->queued_at;
            const struct timespec delta_ts = vdptime2timespec(delta);
            traceInfo("pqdelay %d.%09d %d.%09d\n", (int)now.tv_sec, (int)now.tv_nsec,
                      delta_ts.tv_sec, delta_ts.tv_nsec);
    }

    handle_release(surface);
    handle_release(task->queue_id);

    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
    }
}

gint
compare_func(gconstpointer a, gconstpointer b, gpointer user_data)
{
    const struct task_s *task_a = a;
    const struct task_s *task_b = b;

    if (task_a->when.tv_sec < task_b->when.tv_sec)
        return -1;
    else if (task_a->when.tv_sec > task_b->when.tv_sec)
        return 1;
    else if (task_a->when.tv_nsec < task_b->when.tv_nsec)
        return -1;
    else if (task_a->when.tv_nsec > task_b->when.tv_nsec)
        return 1;
    else
        return 0;
}

static
void *
presentation_thread(void *param)
{
    GQueue *int_q = g_queue_new();          // internal queue of task, always sorted

    while (1) {
        gint64 timeout;
        struct task_s *task = g_queue_peek_head(int_q);

        if (task) {
            // internal queue have a task
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            timeout = (task->when.tv_sec - now.tv_sec) * 1000 * 1000 +
                      (task->when.tv_nsec - now.tv_nsec) / 1000;
            if (timeout <= 0) {
                // task is ready to go
                g_queue_pop_head(int_q); // remove it from queue

                // run the task
                do_presentation_queue_display(task);
                g_slice_free(struct task_s, task);
                continue;
            }
        } else {
            // no tasks in queue, sleep for a while
            timeout = 1000 * 1000; // one second
        }

        task = g_async_queue_timeout_pop(async_q, timeout);
        if (task) {
            if (task->wipe_tasks) {
                // create new internal queue by filtering old
                GQueue *new_q = g_queue_new();
                while (!g_queue_is_empty(int_q)) {
                    struct task_s *t = g_queue_pop_head(int_q);
                    if (t->queue_id != task->queue_id)
                        g_queue_push_tail(new_q, t);
                }
                g_queue_free(int_q);
                int_q = new_q;

                g_slice_free(struct task_s, task);
                continue;
            }
            g_queue_insert_sorted(int_q, task, compare_func, NULL);
        }
    }

    g_queue_free(int_q);
    return NULL;
}

VdpStatus
vdpPresentationQueueCreate(VdpDevice device, VdpPresentationQueueTarget presentation_queue_target,
                           VdpPresentationQueue *presentation_queue)
{
    if (!presentation_queue)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *targetData =
        handle_acquire(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == targetData) {
        handle_release(device);
        return VDP_STATUS_INVALID_HANDLE;
    }

    VdpPresentationQueueData *data = calloc(1, sizeof(VdpPresentationQueueData));
    if (NULL == data) {
        handle_release(device);
        handle_release(presentation_queue_target);
        return VDP_STATUS_RESOURCES;
    }

    data->type = HANDLETYPE_PRESENTATION_QUEUE;
    data->device = device;
    data->deviceData = deviceData;
    data->target = presentation_queue_target;
    data->targetData = targetData;
    data->bg_color.red = 0.0;
    data->bg_color.green = 0.0;
    data->bg_color.blue = 0.0;
    data->bg_color.alpha = 0.0;

    ref_device(deviceData);
    ref_pq_target(targetData);
    *presentation_queue = handle_insert(data);

    // initialize queue and launch worker thread
    if (!async_q) {
        async_q = g_async_queue_new();
        pthread_create(&presentation_thread_id, NULL, presentation_thread, data);
    }

    handle_release(device);
    handle_release(presentation_queue_target);

    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    struct task_s *task = g_slice_new0(struct task_s);
    task->when = vdptime2timespec(0);   // as early as possible
    task->queue_id = presentation_queue;
    task->wipe_tasks = 1;
    g_async_queue_push(async_q, task);

    handle_expunge(presentation_queue);
    unref_device(pqData->deviceData);
    unref_pq_target(pqData->targetData);

    free(pqData);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                       VdpColor *const background_color)
{
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    if (background_color) {
        pqData->bg_color = *background_color;
    } else {
        pqData->bg_color.red = 0.0;
        pqData->bg_color.green = 0.0;
        pqData->bg_color.blue = 0.0;
        pqData->bg_color.alpha = 0.0;
    }

    handle_release(presentation_queue);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                       VdpColor *background_color)
{
    if (!background_color)
        return VDP_STATUS_INVALID_POINTER;
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    *background_color = pqData->bg_color;

    handle_release(presentation_queue);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue, VdpTime *current_time)
{
    if (!current_time)
        return VDP_STATUS_INVALID_POINTER;
    (void)presentation_queue;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    *current_time = timespec2vdptime(now);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                            uint32_t clip_width, uint32_t clip_height,
                            VdpTime earliest_presentation_time)
{
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData) {
        handle_release(presentation_queue);
        return VDP_STATUS_INVALID_HANDLE;
    }
    if (pqData->deviceData != surfData->deviceData) {
        handle_release(surface);
        handle_release(presentation_queue);
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    }

    struct task_s *task = g_slice_new0(struct task_s);

    task->when = vdptime2timespec(earliest_presentation_time);
    task->clip_width = clip_width;
    task->clip_height = clip_height;
    task->surface = surface;
    task->queue_id = presentation_queue;

    surfData->first_presentation_time = 0;
    surfData->status = VDP_PRESENTATION_QUEUE_STATUS_QUEUED;

    if (global.quirks.log_pq_delay) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        surfData->queued_at = timespec2vdptime(now);
    }

    g_async_queue_push(async_q, task);

    handle_release(presentation_queue);
    handle_release(surface);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                    VdpPresentationQueueTarget *target)
{
    if (!target)
        return VDP_STATUS_INVALID_POINTER;
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *data = calloc(1, sizeof(VdpPresentationQueueTargetData));
    if (NULL == data) {
        handle_release(device);
        return VDP_STATUS_RESOURCES;
    }

    glx_ctx_lock();
    data->type = HANDLETYPE_PRESENTATION_QUEUE_TARGET;
    data->device = device;
    data->deviceData = deviceData;
    data->drawable = drawable;
    data->refcount = 0;
    pthread_mutex_init(&data->refcount_mutex, NULL);

    // emulate geometry change. Hope there will be no drawables of such size
    data->drawable_width = (unsigned int)(-1);
    data->drawable_height = (unsigned int)(-1);
    data->pixmap = None;

    // No double buffering since we are going to render to glx pixmap
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
    data->xvi = glXChooseVisual(deviceData->display, deviceData->screen, att);
    if (NULL == data->xvi) {
        traceError("error (%s): glXChooseVisual failed\n", __func__);
        free(data);
        glx_ctx_unlock();
        handle_release(device);
        return VDP_STATUS_ERROR;
    }
    recreate_pixmaps_if_geometry_changed(data);

    // create context for dislaying result (can share display lists with deviceData->glc
    data->glc = glXCreateContext(deviceData->display, data->xvi, deviceData->root_glc, GL_TRUE);
    ref_device(deviceData);
    *target = handle_insert(data);
    glx_ctx_unlock();

    handle_release(device);
    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    VdpPresentationQueueTargetData *pqTargetData =
        handle_acquire(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == pqTargetData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = pqTargetData->deviceData;

    if (0 != pqTargetData->refcount) {
        traceError("warning (%s): non-zero reference count (%d)\n", __func__,
                   pqTargetData->refcount);
        handle_release(presentation_queue_target);
        return VDP_STATUS_ERROR;
    }

    // drawable may be destroyed already, so one should activate global context
    glx_ctx_push_thread_local(deviceData);
    glXDestroyContext(deviceData->display, pqTargetData->glc);
    free_glx_pixmaps(pqTargetData);

    GLenum gl_error = glGetError();
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        handle_release(presentation_queue_target);
        return VDP_STATUS_ERROR;
    }

    unref_device(deviceData);
    XFree(pqTargetData->xvi);
    pthread_mutex_destroy(&pqTargetData->refcount_mutex);
    handle_expunge(presentation_queue_target);
    free(pqTargetData);
    return VDP_STATUS_OK;
}
