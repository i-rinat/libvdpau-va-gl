/*
 * Copyright 2013-2014  Rinat Ibragimov
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
#include <va/va_x11.h>
#include <vdpau/vdpau.h>
#include <string.h>
#include "api.h"
#include "trace.h"


static
void
_free_video_mixer_pixmaps(VdpVideoMixerData *mixerData)
{
    Display *dpy = mixerData->deviceData->display;

    if (mixerData->glx_pixmap != None) {
        glXDestroyGLXPixmap(dpy, mixerData->glx_pixmap);
        mixerData->glx_pixmap = None;
    }
    if (mixerData->pixmap != None) {
        XFreePixmap(dpy, mixerData->pixmap);
        mixerData->pixmap = None;
    }
}

static
void
_render_va_surf_to_texture(VdpVideoMixerData *videoMixerData, VdpVideoSurfaceData *srcSurfData)
{
    VdpDeviceData *deviceData = videoMixerData->deviceData;
    Display *dpy = deviceData->display;

    if (srcSurfData->width != videoMixerData->pixmap_width ||
        srcSurfData->height != videoMixerData->pixmap_height)
    {
        _free_video_mixer_pixmaps(videoMixerData);
        videoMixerData->pixmap = XCreatePixmap(dpy, deviceData->root, srcSurfData->width,
                                               srcSurfData->height, deviceData->color_depth);

        int fbconfig_attrs[] = {
            GLX_DRAWABLE_TYPE,  GLX_PIXMAP_BIT,
            GLX_RENDER_TYPE,    GLX_RGBA_BIT,
            GLX_X_RENDERABLE,   GL_TRUE,
            GLX_Y_INVERTED_EXT, GL_TRUE,
            GLX_RED_SIZE,       8,
            GLX_GREEN_SIZE,     8,
            GLX_BLUE_SIZE,      8,
            GLX_ALPHA_SIZE,     8,
            GLX_DEPTH_SIZE,     16,
            GLX_BIND_TO_TEXTURE_RGBA_EXT,     GL_TRUE,
            GL_NONE
        };

        int nconfigs;
        GLXFBConfig *fbconfig = glXChooseFBConfig(deviceData->display, deviceData->screen,
                                                  fbconfig_attrs, &nconfigs);
        int pixmap_attrs[] = {
            GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
            GLX_MIPMAP_TEXTURE_EXT, GL_FALSE,
            GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
            GL_NONE
        };

        videoMixerData->glx_pixmap = glXCreatePixmap(dpy, fbconfig[0], videoMixerData->pixmap,
                                                     pixmap_attrs);
        free(fbconfig);
        videoMixerData->pixmap_width = srcSurfData->width;
        videoMixerData->pixmap_height = srcSurfData->height;
    }

    glBindTexture(GL_TEXTURE_2D, videoMixerData->tex_id);
    deviceData->fn.glXBindTexImageEXT(deviceData->display, videoMixerData->glx_pixmap,
                                      GLX_FRONT_EXT, NULL);
    XSync(deviceData->display, False);

    vaPutSurface(deviceData->va_dpy, srcSurfData->va_surf, videoMixerData->pixmap,
                 0, 0, srcSurfData->width, srcSurfData->height,
                 0, 0, srcSurfData->width, srcSurfData->height,
                 NULL, 0, VA_FRAME_PICTURE);

    glBindFramebuffer(GL_FRAMEBUFFER, srcSurfData->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, srcSurfData->width, 0, srcSurfData->height, -1.0, 1.0);
    glViewport(0, 0, srcSurfData->width, srcSurfData->height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glDisable(GL_BLEND);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(srcSurfData->width, 0);
        glTexCoord2f(1, 1); glVertex2f(srcSurfData->width, srcSurfData->height);
        glTexCoord2f(0, 1); glVertex2f(0, srcSurfData->height);
    glEnd();
    glFinish();

    deviceData->fn.glXReleaseTexImageEXT(deviceData->display, videoMixerData->glx_pixmap,
                                         GLX_FRONT_EXT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

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
    data->pixmap = None;
    data->glx_pixmap = None;
    data->pixmap_width = (uint32_t)(-1);    // set knowingly invalid geometry
    data->pixmap_height = (uint32_t)(-1);   // to force pixmap recreation

    glx_ctx_push_thread_local(deviceData);
    glGenTextures(1, &data->tex_id);
    glBindTexture(GL_TEXTURE_2D, data->tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLenum gl_error = glGetError();
    glx_ctx_pop();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        free(data);
        goto quit;
    }

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
    VdpStatus err_code;
    VdpVideoMixerData *videoMixerData = handle_acquire(mixer, HANDLETYPE_VIDEO_MIXER);
    if (NULL == videoMixerData)
        return VDP_STATUS_INVALID_HANDLE;
    VdpDeviceData *deviceData = videoMixerData->deviceData;

    glx_ctx_lock();
    _free_video_mixer_pixmaps(videoMixerData);
    glx_ctx_unlock();
    glx_ctx_push_thread_local(deviceData);
    glDeleteTextures(1, &videoMixerData->tex_id);
    GLenum gl_error = glGetError();
    glx_ctx_pop();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    unref_device(deviceData);
    handle_expunge(mixer);
    free(videoMixerData);
    return err_code;
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
    uint32_t uint32_value;

    switch (parameter) {
    case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH: // TODO: get actual limits
        uint32_value = 16;
        memcpy(min_value, &uint32_value, sizeof(uint32_value));
        uint32_value = 4096;
        memcpy(max_value, &uint32_value, sizeof(uint32_value));
        return VDP_STATUS_OK;

    case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT: // TODO: get actual limits
        uint32_value = 16;
        memcpy(min_value, &uint32_value, sizeof(uint32_value));
        uint32_value = 4096;
        memcpy(max_value, &uint32_value, sizeof(uint32_value));
        return VDP_STATUS_OK;

    case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE: // TODO
    case VDP_VIDEO_MIXER_PARAMETER_LAYERS:      // TODO
    default:
        return VDP_STATUS_NO_IMPLEMENTATION;
    }
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

    VdpVideoMixerData *mixerData = handle_acquire(mixer, HANDLETYPE_VIDEO_MIXER);
    VdpVideoSurfaceData *srcSurfData =
        handle_acquire(video_surface_current, HANDLETYPE_VIDEO_SURFACE);
    VdpOutputSurfaceData *dstSurfData =
        handle_acquire(destination_surface, HANDLETYPE_OUTPUT_SURFACE);
    if (NULL == mixerData || NULL == srcSurfData || NULL == dstSurfData) {
        err_code = VDP_STATUS_INVALID_HANDLE;
        goto quit;
    }
    if (srcSurfData->deviceData != dstSurfData->deviceData ||
        srcSurfData->deviceData != mixerData->deviceData)
    {
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

    glx_ctx_push_thread_local(deviceData);

    if (srcSurfData->sync_va_to_glx) {
        _render_va_surf_to_texture(mixerData, srcSurfData);
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
    glx_ctx_pop();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (%s): gl error %d\n", __func__, gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    err_code = VDP_STATUS_OK;
quit:
    handle_release(video_surface_current);
    handle_release(destination_surface);
    handle_release(mixer);
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
