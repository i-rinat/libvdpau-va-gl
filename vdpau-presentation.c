/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of LGPLv3. See COPYING for details.
 */

#define GL_GLEXT_PROTOTYPES
#include <stdlib.h>
#include <sys/time.h>
#include <vdpau/vdpau.h>
#include <GL/gl.h>
#include "ctx-stack.h"
#include "globals.h"
#include "handle-storage.h"
#include "vdpau-locking.h"
#include "vdpau-soft.h"
#include "vdpau-trace.h"

#include "watermark.h"

VdpStatus
softVdpPresentationQueueBlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue,
                                              VdpOutputSurface surface,
                                              VdpTime *first_presentation_time)

{
    if (! handlestorage_valid(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE))
        return VDP_STATUS_INVALID_HANDLE;

    if (! handlestorage_valid(surface, HANDLETYPE_OUTPUT_SURFACE))
        return VDP_STATUS_INVALID_HANDLE;

    // use current time as presentation time
    softVdpPresentationQueueGetTime(presentation_queue, first_presentation_time);

    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueQuerySurfaceStatus(VdpPresentationQueue presentation_queue,
                                           VdpOutputSurface surface,
                                           VdpPresentationQueueStatus *status,
                                           VdpTime *first_presentation_time)
{
    (void)presentation_queue; (void)surface; (void)first_presentation_time;
    *status = VDP_PRESENTATION_QUEUE_STATUS_VISIBLE;
    return VDP_STATUS_OK;
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
    return VDP_STATUS_OK;
}

VdpStatus
softVdpPresentationQueueDestroy(VdpPresentationQueue presentation_queue)
{
    VdpPresentationQueueData *data =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    handlestorage_expunge(presentation_queue);
    data->device->refcount --;
    data->target->refcount --;

    free(data);
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
    struct timeval tv;
    gettimeofday(&tv, NULL);
    *current_time = (uint64_t)tv.tv_sec * 1000000000LL + (uint64_t)tv.tv_usec * 1000LL;
    return VDP_STATUS_OK;
}


VdpStatus
softVdpPresentationQueueDisplay(VdpPresentationQueue presentation_queue, VdpOutputSurface surface,
                                uint32_t clip_width, uint32_t clip_height,
                                VdpTime earliest_presentation_time)
{
    (void)earliest_presentation_time;   // TODO: take into accound earliest_presentation_time
    VdpOutputSurfaceData *surfData = handlestorage_get(surface, HANDLETYPE_OUTPUT_SURFACE);
    VdpPresentationQueueData *pqueueData =
        handlestorage_get(presentation_queue, HANDLETYPE_PRESENTATION_QUEUE);
    if (NULL == surfData || NULL == pqueueData)
        return VDP_STATUS_INVALID_HANDLE;
    if (pqueueData->device != surfData->device)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    VdpDeviceData *deviceData = surfData->device;

    glx_context_push_global(deviceData->display, pqueueData->target->drawable, pqueueData->target->glc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpPresentationQueueDisplay): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

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
