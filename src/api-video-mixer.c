/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define GL_GLEXT_PROTOTYPES
#include "ctx-stack.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include "api.h"
#include "trace.h"


VdpStatus
vdpVideoMixerCreate(VdpDevice device, uint32_t feature_count,
                    VdpVideoMixerFeature const *features, uint32_t parameter_count,
                    VdpVideoMixerParameter const *parameters,
                    void const *const *parameter_values, VdpVideoMixer *mixer)
{
    VdpStatus err_code;
    if (!mixer)
        return VDP_STATUS_INVALID_POINTER;
    (void)feature_count; (void)features;    // TODO: mixer features
    (void)parameter_count; (void)parameters; (void)parameter_values;    // TODO: mixer parameters
    VdpDeviceData *deviceData = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == deviceData)
        return VDP_STATUS_INVALID_HANDLE;

    VdpVideoMixerData *data = calloc(1, sizeof(VdpVideoMixerData));
    if (NULL == data) {
        err_code = VDP_STATUS_RESOURCES;
        goto quit;
    }

    data->type = HANDLETYPE_VIDEO_MIXER;
    data->device = device;
    data->deviceData = deviceData;

    ref_device(deviceData);
    *mixer = handle_insert(data);

    err_code = VDP_STATUS_OK;
quit:
    handle_release(device);
    return err_code;
}

VdpStatus
vdpVideoMixerDestroy(VdpVideoMixer mixer)
{
    VdpVideoMixerData *videoMixerData = handle_acquire(mixer, HANDLETYPE_VIDEO_MIXER);
    if (NULL == videoMixerData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoMixerData->deviceData;

    unref_device(deviceData);
    handle_expunge(mixer);
    free(videoMixerData);
    return VDP_STATUS_OK;
}

VdpStatus
vdpVideoMixerGetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                VdpVideoMixerAttribute const *attributes,
                                void *const *attribute_values)
{
    (void)mixer; (void)attribute_count; (void)attributes; (void)attribute_values;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                               VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_enables;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count,
                               VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_supports;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerGetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                                VdpVideoMixerParameter const *parameters,
                                void *const *parameter_values)
{
    (void)mixer; (void)parameter_count; (void)parameters; (void)parameter_values;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerQueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute,
                                   VdpBool *is_supported)
{
    (void)device; (void)attribute; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerQueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute,
                                      void *min_value, void *max_value)
{
    (void)device; (void)attribute; (void)min_value; (void)max_value;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerQueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature,
                                 VdpBool *is_supported)
{
    (void)device; (void)feature; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerQueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter,
                                   VdpBool *is_supported)
{
    (void)device; (void)parameter; (void)is_supported;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerQueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter,
                                      void *min_value, void *max_value)
{
    (void)device; (void)parameter; (void)min_value; (void)max_value;
    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
vdpVideoMixerRender(VdpVideoMixer mixer, VdpOutputSurface background_surface,
                    VdpRect const *background_source_rect,
                    VdpVideoMixerPictureStructure current_picture_structure,
                    uint32_t video_surface_past_count, VdpVideoSurface const *video_surface_past,
                    VdpVideoSurface video_surface_current, uint32_t video_surface_future_count,
                    VdpVideoSurface const *video_surface_future, VdpRect const *video_source_rect,
                    VdpOutputSurface destination_surface, VdpRect const *destination_rect,
                    VdpRect const *destination_video_rect, uint32_t layer_count,
                    VdpLayer const *layers)
{
    VdpStatus err_code;
    (void)mixer;    // TODO: mixer should be used to get mixing parameters
    // TODO: current implementation ignores previous and future surfaces, using only current.
    // Is that acceptable for interlaced video? Will VAAPI handle deinterlacing?

    (void)background_surface;   // TODO: background_surface. Is it safe to just ignore it?
    (void)background_source_rect;
    (void)current_picture_structure;
    (void)video_surface_past_count; (void)video_surface_past;
    (void)video_surface_future_count; (void)video_surface_future;
    (void)layer_count; (void)layers;

    VdpVideoSurfaceData *srcSurfData =
        handle_acquire(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    VdpOutputSurfaceData *dstSurfData =
        handle_acquire(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == srcSurfData || NULL == dstSurfData) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }
    if (srcSurfData->deviceData != dstSurfData->deviceData) {
        err_code = VDP_STATUS_HANDLE_DEVICE_MISMATCH;
        goto quit;
    }
    VdpDeviceData *deviceData = srcSurfData->deviceData;

    VdpRect srcVideoRect = {0, 0, srcSurfData->width, srcSurfData->height};
    if (video_source_rect)
        srcVideoRect = *video_source_rect;

    VdpRect dstRect = {0, 0, dstSurfData->width, dstSurfData->height};
    if (destination_rect)
        dstRect = *destination_rect;

    VdpRect dstVideoRect = srcVideoRect;
    if (destination_video_rect)
        dstVideoRect = *destination_video_rect;

    // TODO: dstRect should clip dstVideoRect

    glx_context_push_thread_local(deviceData);

    if (srcSurfData->sync_va_to_glx) {
        VAStatus status;
        if (NULL == srcSurfData->va_glx) {
            status = vaCreateSurfaceGLX(deviceData->va_dpy, GL_TEXTURE_2D, srcSurfData->tex_id,
                                        &srcSurfData->va_glx);
            if (VA_STATUS_SUCCESS != status) {
                glx_context_pop();
                err_code = VDP_STATUS_ERROR;
                goto quit;
            }
        }

        status = vaCopySurfaceGLX(deviceData->va_dpy, srcSurfData->va_glx, srcSurfData->va_surf, 0);
        // TODO: check result of previous call
        srcSurfData->sync_va_to_glx = 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, dstSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dstSurfData->width, 0, dstSurfData->height, -1.0f, 1.0f);
    glViewport(0, 0, dstSurfData->width, dstSurfData->height);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/srcSurfData->width, 1.0f/srcSurfData->height, 1.0f);

    // Clear dstRect area
    glDisable(GL_TEXTURE_2D);
    glColor4f(0, 0, 0, 1);
    glBegin(GL_QUADS);
        glVertex2f(dstRect.x0, dstRect.y0);
        glVertex2f(dstRect.x1, dstRect.y0);
        glVertex2f(dstRect.x1, dstRect.y1);
        glVertex2f(dstRect.x0, dstRect.y1);
    glEnd();

    // Render (maybe scaled) data from video surface
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, srcSurfData->tex_id);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
        glTexCoord2i(srcVideoRect.x0, srcVideoRect.y0);
        glVertex2f(dstVideoRect.x0, dstVideoRect.y0);

        glTexCoord2i(srcVideoRect.x1, srcVideoRect.y0);
        glVertex2f(dstVideoRect.x1, dstVideoRect.y0);

        glTexCoord2i(srcVideoRect.x1, srcVideoRect.y1);
        glVertex2f(dstVideoRect.x1, dstVideoRect.y1);

        glTexCoord2i(srcVideoRect.x0, srcVideoRect.y1);
        glVertex2f(dstVideoRect.x0, dstVideoRect.y1);
    glEnd();
    glFinish();

    GLenum gl_error = glGetError();
    glx_context_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(video_surface_current);
    handle_release(destination_surface);
    return err_code;
}

VdpStatus
vdpVideoMixerSetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                                VdpVideoMixerAttribute const *attributes,
                                void const *const *attribute_values)
{
    (void)mixer; (void)attribute_count; (void)attributes; (void)attribute_values;
    return VDP_STATUS_OK;
}

VdpStatus
vdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count,
                               VdpVideoMixerFeature const *features, VdpBool const *feature_enables)
{
    (void)mixer; (void)feature_count; (void)features; (void)feature_enables;
    return VDP_STATUS_OK;
}
