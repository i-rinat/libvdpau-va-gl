/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define GL_GLEXT_PROTOTYPES
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
#include "vdpau-locking.h"
#include "vdpau-soft.h"
#include "vdpau-trace.h"
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
softVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                              VdpOutputSurface surface,
                                              VdpTime *first_presentation_time)

{
    VdpPresentationQueueData *pqData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;

    // TODO: use locking instead of busy loop
    while (surfData->status != VDP_PRESENTATION_QUEUE_STATUS_IDLE) {
        release_global_lock();
        usleep(1000);
        acquire_global_lock();
    }

    *first_presentation_time = surfData->first_presentation_time;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                           VdpOutputSurface surface,
                                           VdpPresentationQueueStatus *status,
                                           VdpTime *first_presentation_time)
{
    VdpPresentationQueueData *pqData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == surfData)
        return VDP_STATUS_INVALID_HANDLE;
    if (NULL == status)
        return VDP_STATUS_INVALID_POINTER;

    *status = surfData->status;
    *first_presentation_time = surfData->first_presentation_time;

    return VDP_STATUS_OK;
}

static
void
do_presentation_queue_display(VdpPresentationQueueData *pqueueData)
{
    acquire_global_lock();
    pthread_mutex_lock(&pqueueData->queue_mutex);
    assert(pqueueData->queue.used > 0);
    const int entry = pqueueData->queue.head;

    VdpDeviceData *deviceData = pqueueData->device;
    VdpOutputSurfaceData *surfData = pqueueData->queue.item[entry].surfData;
    const uint32_t clip_width = pqueueData->queue.item[entry].clip_width;
    const uint32_t clip_height = pqueueData->queue.item[entry].clip_height;

    // remove first entry from queue
    pqueueData->queue.used --;
    pqueueData->queue.freelist[pqueueData->queue.head] = pqueueData->queue.firstfree;
    pqueueData->queue.firstfree = pqueueData->queue.head;
    pqueueData->queue.head = pqueueData->queue.item[pqueueData->queue.head].next;
    pthread_mutex_unlock(&pqueueData->queue_mutex);

    glx_context_push_global(deviceData->display, pqueueData->target->drawable, pqueueData->target->glc);

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

        glColor3f(0.8, 0.08, 0.35);
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

    locked_glXSwapBuffers(deviceData->display, pqueueData->target->drawable);

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

    release_global_lock();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpPresentationQueueDisplay): gl error %d\n", gl_error);
    }
}

static
void *
presentation_thread(void *param)
{
    VdpPresentationQueueData *pqData = param;
    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        struct timespec target_time = now;

        pthread_mutex_lock(&pqData->queue_mutex);
        while (1) {
            int ret = pthread_cond_timedwait(&pqData->new_work_available, &pqData->queue_mutex,
                                             &target_time);
            if (pqData->turning_off)
                goto quit;
            if (ret != 0 && ret != ETIMEDOUT) {
                traceError("presentation_thread: pthread_cond_timedwait failed with code %d\n", ret);
                goto quit;
            }

            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            if (pqData->queue.head != -1) {
                struct timespec ht = vdptime2timespec(pqData->queue.item[pqData->queue.head].t);
                if (now.tv_sec > ht.tv_sec || (now.tv_sec == ht.tv_sec && now.tv_nsec > ht.tv_nsec)) {
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
        pthread_mutex_unlock(&pqData->queue_mutex);
        do_presentation_queue_display(pqData);
    }

quit:
    return NULL;
}

VdpStatus
softVdpPresentationQueueCreate(VdpDevice device,
                               VdpPresentationQueueTarget presentation_queue_target,
                               VdpPresentationQueue *presentation_queue)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *targetData =
        handlestorage_get(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == targetData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueData *data =
        (VdpPresentationQueueData *)calloc(1, sizeof(VdpPresentationQueueData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_PRESENTATION_QUEUE;
    data->device = deviceData;
    data->target = targetData;
    data->bg_color.red = 0.0;
    data->bg_color.green = 0.0;
    data->bg_color.blue = 0.0;
    data->bg_color.alpha = 0.0;

    deviceData->refcount ++;
    targetData->refcount ++;
    *presentation_queue = handlestorage_add(data);

    // initialize queue
    data->queue.head = -1;
    data->queue.used = 0;
    for (unsigned int k = 0; k < PRESENTATION_QUEUE_LENGTH; k ++) {
        data->queue.item[k].next = -1;
        // other fields are zero due to calloc usage
    }
    for (unsigned int k = 0; k < PRESENTATION_QUEUE_LENGTH - 1; k ++)
        data->queue.freelist[k] = k + 1;
    data->queue.freelist[PRESENTATION_QUEUE_LENGTH - 1] = -1;
    data->queue.firstfree = 0;

    if (0 != pthread_mutex_init(&data->queue_mutex, NULL)) {
        traceError("VdpPresentationQueueCreate: can't create mutex");
        goto err;
    }
    if (0 != pthread_cond_init(&data->new_work_available, NULL)) {
        traceError("VdpPresentationQueueCreate: can't create condition variable");
        goto err;
    }

    // fire up worker thread
    if (0 != pthread_create(&data->worker_thread, NULL, presentation_thread, (void *)data)) {
        traceError("VdpPresentationQueueCreate: failed to launch worker thread");
        goto err;
    }

    return VDP_STATUS_OK;

err:
    free(data);
    return VDP_STATUS_RESOURCES;
}

VdpStatus
softVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    VdpPresentationQueueData *pqData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    pqData->turning_off = 1;
    pthread_cond_broadcast(&pqData->new_work_available);
    // release lock in order to allow worker thread successfully terminate
    release_global_lock();
    if (0 != pthread_join(pqData->worker_thread, NULL)) {
        traceError("VdpPresentationQueueDestroy: failed to stop worker thread");
        return VDP_STATUS_ERROR;
    }
    acquire_global_lock();

    handlestorage_expunge(presentation_queue);
    pqData->device->refcount --;
    pqData->target->refcount --;

    free(pqData);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueSetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *const background_color)
{
    VdpPresentationQueueData *pqData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
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

    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueGetBackgroundColor(VdpPresentationQueue presentation_queue,
                                           VdpColor *background_color)
{
    VdpPresentationQueueData *pqData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;

    if (NULL == background_color)
        return VDP_STATUS_INVALID_POINTER;
    *background_color = pqData->bg_color;

    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueGetTime(VdpPresentationQueue presentation_queue,
                                VdpTime *current_time)
{
    (void)presentation_queue;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    *current_time = timespec2vdptime(now);
    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                uint32_t clip_width, uint32_t clip_height,
                                VdpTime earliest_presentation_time)
{
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpPresentationQueueData *pqData = handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == surfData || NULL == pqData)
        return VDP_STATUS_INVALID_HANDLE;
    if (pqData->device != surfData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

    // push work to queue
    pthread_mutex_lock(&pqData->queue_mutex);
    while (pqData->queue.used >= PRESENTATION_QUEUE_LENGTH) {
        // wait while queue is full
        pthread_mutex_unlock(&pqData->queue_mutex);
        usleep(10*1000);
        pthread_mutex_lock(&pqData->queue_mutex);
    }
    pqData->queue.used ++;

    int new_item = pqData->queue.firstfree;
    assert(new_item != -1);
    pqData->queue.firstfree = pqData->queue.freelist[new_item];

    pqData->queue.item[new_item].t = earliest_presentation_time;
    pqData->queue.item[new_item].clip_width = clip_width;
    pqData->queue.item[new_item].clip_height = clip_height;
    pqData->queue.item[new_item].surfData = surfData;
    surfData->first_presentation_time = 0;
    surfData->status = VDP_PRESENTATION_QUEUE_STATUS_QUEUED;

    // keep queue sorted
    if (pqData->queue.head == -1 || earliest_presentation_time < pqData->queue.item[pqData->queue.head].t) {
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

    pthread_cond_broadcast(&pqData->new_work_available);

    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueTargetCreateX11(VdpDevice device, Drawable drawable,
                                        VdpPresentationQueueTarget *target)
{
    VdpDeviceData *deviceData = handlestorage_get(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpPresentationQueueTargetData *data =
        (VdpPresentationQueueTargetData *)calloc(1, sizeof(VdpPresentationQueueTargetData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_PRESENTATION_QUEUE_TARGET;
    data->device = deviceData;
    data->drawable = drawable;
    data->refcount = 0;

    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo *vi;
    vi = glXChooseVisual(deviceData->display, deviceData->screen, att);
    if (NULL == vi) {
        traceError("error (softVdpPresentationQueueTargetCreateX11): glXChooseVisual failed\n");
        free(data);
        return VDP_STATUS_ERROR;
    }

    // create context for dislaying result (can share display lists with deviceData->glc
    XLockDisplay(deviceData->display);
    data->glc = glXCreateContext(deviceData->display, vi, deviceData->root_glc, GL_TRUE);
    XUnlockDisplay(deviceData->display);

    deviceData->refcount ++;
    *target = handlestorage_add(data);

    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueTargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    VdpPresentationQueueTargetData *pqTargetData =
        handlestorage_get(presentation_queue_target, HANDLETYPE_PRESENTATION_QUEUE_TARGET);
    if (NULL == pqTargetData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = pqTargetData->device;

    if (0 != pqTargetData->refcount) {
        traceError("warning (softVdpPresentationQueueTargetDestroy): non-zero reference"
                   "count (%d)\n", pqTargetData->refcount);
        return VDP_STATUS_ERROR;
    }

    // drawable may be destroyed already, so one should activate global context
    glx_context_push_thread_local(deviceData);
    glXDestroyContext(deviceData->display, pqTargetData->glc);

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpPresentationQueueTargetDestroy): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    free(pqTargetData);
    deviceData->refcount --;
    handlestorage_expunge(presentation_queue_target);
    return VDP_STATUS_OK;
}
