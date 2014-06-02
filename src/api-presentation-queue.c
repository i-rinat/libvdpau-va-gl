/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
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
do_presentation_queue_display(VdpPresentationQueueData *pqData)
{
    assert(pqData->queue.used > 0);

    const int entry = pqData->queue.head;
    VdpDeviceData *deviceData = pqData->deviceData;
    VdpOutputSurface surface = pqData->queue.item[entry].surface;
    const uint32_t clip_width = pqData->queue.item[entry].clip_width;
    const uint32_t clip_height = pqData->queue.item[entry].clip_height;

    // remove first entry from queue
    pqData->queue.used --;
    pqData->queue.freelist[pqData->queue.head] = pqData->queue.firstfree;
    pqData->queue.firstfree = pqData->queue.head;
    pqData->queue.head = pqData->queue.item[pqData->queue.head].next;

    VdpOutputSurfaceData *surfData = handle_acquire(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (surfData == NULL)
        return;

    glx_ctx_lock();
    recreate_pixmaps_if_geometry_changed(pqData->targetData);
    glx_ctx_unlock();
    glx_ctx_push_global(deviceData->display, pqData->targetData->glx_pixmap,
                        pqData->targetData->glc);

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
    glx_ctx_pop();

    x11_push_eh();
    glx_ctx_lock();
    XSync(deviceData->display, False);
    XCopyArea(deviceData->display, pqData->targetData->pixmap, pqData->targetData->drawable,
              pqData->targetData->plain_copy_gc, 0, 0, target_width, target_height, 0, 0);
    XSync(deviceData->display, False);
    int x11_err = x11_pop_eh();
    glx_ctx_unlock();
    if (x11_err != Success) {
        char buf[200] = { 0 };
        XGetErrorText(deviceData->display, x11_err, buf, sizeof(buf));
        traceError("warning (%s): caught X11 error %s\n", __func__, buf);
    }

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

    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
    }
}

static
void *
presentation_thread(void *param)
{
    VdpPresentationQueueData *pqData = (VdpPresentationQueueData *)param;

    pthread_mutex_lock(&pqData->queue_mutex);
    pthread_barrier_wait(&pqData->thread_start_barrier);
    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        struct timespec target_time = now;

        while (1) {
            int ret;
            ret = pthread_cond_timedwait(&pqData->new_work_available,
                                         &pqData->queue_mutex, &target_time);
            if (ret != 0 && ret != ETIMEDOUT) {
                traceError("error (%s): pthread_cond_timedwait failed with code %d\n", __func__,
                           ret);
                pthread_mutex_unlock(&pqData->queue_mutex);
                return NULL;
            }

            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            if (pqData->thread_state == 1) {
                pqData->thread_state = 2;
                pthread_mutex_unlock(&pqData->queue_mutex);
                return NULL;
            }
            if (pqData->queue.head != -1) {
                struct timespec ht = vdptime2timespec(pqData->queue.item[pqData->queue.head].t);
                if (now.tv_sec > ht.tv_sec ||
                    (now.tv_sec == ht.tv_sec && now.tv_nsec > ht.tv_nsec))
                {
                    // break loop and process event
                    break;
                } else {
                    // sleep until next event
                    target_time = ht;
                }
            } else {
                // queue empty, no work to do. Wait for next event
                target_time = now;
                target_time.tv_sec += 1;
            }
        }

        // do event processing
        do_presentation_queue_display(pqData);
    }
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
    data->thread_state = 0;

    ref_device(deviceData);
    ref_pq_target(targetData);
    *presentation_queue = handle_insert(data);

    // initialize queue
    data->queue.head = -1;
    data->queue.used = 0;
    for (unsigned int k = 0; k < PRESENTATION_QUEUE_LENGTH; k ++) {
        data->queue.item[k].next = -1;
        // other fields are zero due to calloc
    }
    for (unsigned int k = 0; k < PRESENTATION_QUEUE_LENGTH - 1; k ++)
        data->queue.freelist[k] = k + 1;
    data->queue.freelist[PRESENTATION_QUEUE_LENGTH - 1] = -1;
    data->queue.firstfree = 0;

    pthread_cond_init(&data->new_work_available, NULL);
    pthread_mutex_init(&data->queue_mutex, NULL);
    pthread_barrier_init(&data->thread_start_barrier, NULL, 2);

    // launch worker thread
    pthread_mutex_lock(&data->queue_mutex);
    pthread_create(&data->worker_thread, NULL, presentation_thread, data);
    handle_release(device);
    handle_release(presentation_queue_target);

    pthread_mutex_unlock(&data->queue_mutex);

    // wait till worker thread passes first lock
    pthread_barrier_wait(&data->thread_start_barrier);
    // wait till worker thread unlocks queue_mutex. That means, it started listening to
    // conditional variable
    pthread_mutex_lock(&data->queue_mutex);
    pthread_mutex_unlock(&data->queue_mutex);

    return VDP_STATUS_OK;
}

VdpStatus
vdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    VdpPresentationQueueData *pqData =
        handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    pthread_mutex_lock(&pqData->queue_mutex);
    pqData->thread_state = 1;   // send termination request
    pthread_cond_broadcast(&pqData->new_work_available);
    pthread_mutex_unlock(&pqData->queue_mutex);

    pthread_join(pqData->worker_thread, NULL);

    pthread_cond_destroy(&pqData->new_work_available);
    pthread_barrier_destroy(&pqData->thread_start_barrier);
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

    // push work to queue
    while (pqData && pqData->queue.used >= PRESENTATION_QUEUE_LENGTH) {
        // wait while queue is full
        // TODO: check for deadlock here
        handle_release(presentation_queue);
        usleep(10*1000);
        pqData = handle_acquire(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    }

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

    pthread_mutex_lock(&pqData->queue_mutex);
    pqData->queue.used ++;
    int new_item = pqData->queue.firstfree;
    assert(new_item != -1);
    pqData->queue.firstfree = pqData->queue.freelist[new_item];

    pqData->queue.item[new_item].t = earliest_presentation_time;
    pqData->queue.item[new_item].clip_width = clip_width;
    pqData->queue.item[new_item].clip_height = clip_height;
    pqData->queue.item[new_item].surface = surface;
    surfData->first_presentation_time = 0;
    surfData->status = VDP_PRESENTATION_QUEUE_STATUS_QUEUED;

    // keep queue sorted
    if (pqData->queue.head == -1 ||
        earliest_presentation_time < pqData->queue.item[pqData->queue.head].t)
    {
        pqData->queue.item[new_item].next = pqData->queue.head;
        pqData->queue.head = new_item;
    } else {
        int ptr = pqData->queue.head;
        int prev = ptr;
        while (ptr != -1 && pqData->queue.item[ptr].t <= earliest_presentation_time) {
            prev = ptr;
            ptr = pqData->queue.item[ptr].next;
        }
        pqData->queue.item[new_item].next = ptr;
        pqData->queue.item[prev].next = new_item;
    }
    pthread_mutex_unlock(&pqData->queue_mutex);

    if (global.quirks.log_pq_delay) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        surfData->queued_at = timespec2vdptime(now);
    }

    pthread_mutex_lock(&pqData->queue_mutex);
    pthread_cond_broadcast(&pqData->new_work_available);
    pthread_mutex_unlock(&pqData->queue_mutex);

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
