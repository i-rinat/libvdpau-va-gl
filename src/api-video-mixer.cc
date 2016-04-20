/*
 * Copyright 2013-2016  Rinat Ibragimov
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
#include "api-device.hh"
#include "api-output-surface.hh"
#include "api-video-mixer.hh"
#include "api-video-surface.hh"
#include "glx-context.hh"
#include "handle-storage.hh"
#include "trace.hh"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <string.h>
#include <va/va_x11.h>
#include <vdpau/vdpau.h>


using std::make_shared;
using std::shared_ptr;


namespace vdp { namespace VideoMixer {

void
Resource::free_video_mixer_pixmaps()
{
    Display *dpy = device->dpy.get();

    if (glx_pixmap != None) {
        glXDestroyGLXPixmap(dpy, glx_pixmap);
        glx_pixmap = None;
    }

    if (pixmap != None) {
        XFreePixmap(dpy, pixmap);
        pixmap = None;
    }
}

void
render_va_surf_to_texture(shared_ptr<Resource> mixer,
                          shared_ptr<vdp::VideoSurface::Resource> src_surf)
{
    auto deviceData = mixer->device;
    Display *dpy = mixer->device->dpy.get();

    if (src_surf->width != mixer->pixmap_width || src_surf->height != mixer->pixmap_height) {
        mixer->free_video_mixer_pixmaps();
        mixer->pixmap = XCreatePixmap(dpy, mixer->device->root, src_surf->width, src_surf->height,
                                      mixer->device->color_depth);

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
        GLXFBConfig *fbconfig = glXChooseFBConfig(dpy, mixer->device->screen, fbconfig_attrs,
                                                  &nconfigs);
        int pixmap_attrs[] = {
            GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
            GLX_MIPMAP_TEXTURE_EXT, GL_FALSE,
            GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
            GL_NONE
        };

        mixer->glx_pixmap = glXCreatePixmap(dpy, fbconfig[0], mixer->pixmap, pixmap_attrs);
        free(fbconfig);
        mixer->pixmap_width = src_surf->width;
        mixer->pixmap_height = src_surf->height;
    }

    glBindTexture(GL_TEXTURE_2D, mixer->tex_id);
    mixer->device->fn.glXBindTexImageEXT(dpy, mixer->glx_pixmap, GLX_FRONT_EXT, NULL);
    XSync(dpy, False); // TODO: avoid XSync

    vaPutSurface(mixer->device->va_dpy, src_surf->va_surf, mixer->pixmap,
                 0, 0, src_surf->width, src_surf->height,
                 0, 0, src_surf->width, src_surf->height,
                 nullptr, 0, VA_FRAME_PICTURE);

    glBindFramebuffer(GL_FRAMEBUFFER, src_surf->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, src_surf->width, 0, src_surf->height, -1.0, 1.0);
    glViewport(0, 0, src_surf->width, src_surf->height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glDisable(GL_BLEND);

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0,               0);
        glTexCoord2f(1, 0); glVertex2f(src_surf->width, 0);
        glTexCoord2f(1, 1); glVertex2f(src_surf->width, src_surf->height);
        glTexCoord2f(0, 1); glVertex2f(0,               src_surf->height);
    glEnd();
    glFinish();

    mixer->device->fn.glXReleaseTexImageEXT(dpy, mixer->glx_pixmap, GLX_FRONT_EXT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Resource::Resource(shared_ptr<vdp::Device::Resource> a_device, uint32_t a_feature_count,
                   VdpVideoMixerFeature const *a_features, uint32_t a_parameter_count,
                   VdpVideoMixerParameter const *a_parameters,
                   void const *const *a_parameter_values)
{
    std::ignore = a_feature_count;
    std::ignore = a_features;             // TODO: mixer features
    std::ignore = a_parameter_count;
    std::ignore = a_parameters;
    std::ignore = a_parameter_values;     // TODO: mixer parameters

    device =        a_device;
    pixmap =        None;
    glx_pixmap =    None;
    pixmap_width =  (uint32_t)(-1); // set knowingly invalid geometry
    pixmap_height = (uint32_t)(-1); // to force pixmap recreation

    {
        GLXThreadLocalContext guard{device};

        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        const auto gl_error = glGetError();
        if (gl_error != GL_NO_ERROR) {
            traceError("VideoMixer::Resource::Resource(): gl error %d\n", gl_error);
            throw vdp::generic_error();
        }
    }
}

Resource::~Resource()
{
    try {
        {
            GLXLockGuard guard;
            free_video_mixer_pixmaps();
        }

        {
            GLXThreadLocalContext guard{device};

            glDeleteTextures(1, &tex_id);

            const auto gl_error = glGetError();
            if (gl_error != GL_NO_ERROR)
                traceError("VideoMixer::Resource::~Resource(): gl error %d\n", gl_error);
        }

    } catch (...) {
        traceError("VideoMixer::Resource::~Resource(): caught exception\n");
    }
}

VdpStatus
CreateImpl(VdpDevice device_id, uint32_t feature_count, VdpVideoMixerFeature const *features,
           uint32_t parameter_count, VdpVideoMixerParameter const *parameters,
           void const *const *parameter_values, VdpVideoMixer *mixer)
{
    if (!mixer)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

    auto data = make_shared<Resource>(device, feature_count, features, parameter_count,
                                      parameters, parameter_values);

    *mixer = ResourceStorage<Resource>::instance().insert(data);
    return VDP_STATUS_OK;
}

VdpStatus
Create(VdpDevice device_id, uint32_t feature_count, VdpVideoMixerFeature const *features,
       uint32_t parameter_count, VdpVideoMixerParameter const *parameters,
       void const *const *parameter_values, VdpVideoMixer *mixer)
{
    return check_for_exceptions(CreateImpl, device_id, feature_count, features, parameter_count,
                                parameters, parameter_values, mixer);
}

VdpStatus
DestroyImpl(VdpVideoMixer mixer_id)
{
    ResourceRef<Resource> mixer{mixer_id};

    ResourceStorage<Resource>::instance().drop(mixer_id);
    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpVideoMixer mixer_id)
{
    return check_for_exceptions(DestroyImpl, mixer_id);
}

VdpStatus
GetAttributeValuesImpl(VdpVideoMixer mixer, uint32_t attribute_count,
                       VdpVideoMixerAttribute const *attributes, void *const *attribute_values)
{
    std::ignore = mixer;
    std::ignore = attribute_count;
    std::ignore = attributes;
    std::ignore = attribute_values;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
GetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                   VdpVideoMixerAttribute const *attributes, void *const *attribute_values)
{
    return check_for_exceptions(GetAttributeValuesImpl, mixer, attribute_count, attributes,
                                attribute_values);
}

VdpStatus
GetFeatureEnablesImpl(VdpVideoMixer mixer, uint32_t feature_count,
                      VdpVideoMixerFeature const *features, VdpBool *feature_enables)
{
    std::ignore = mixer;
    std::ignore = feature_count;
    std::ignore = features;
    std::ignore = feature_enables;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
GetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count, VdpVideoMixerFeature const *features,
                  VdpBool *feature_enables)
{
    return check_for_exceptions(GetFeatureEnablesImpl, mixer, feature_count, features,
                                feature_enables);
}

VdpStatus
GetFeatureSupportImpl(VdpVideoMixer mixer, uint32_t feature_count,
                      VdpVideoMixerFeature const *features, VdpBool *feature_supports)
{
    std::ignore = mixer;
    std::ignore = feature_count;
    std::ignore = features;
    std::ignore = feature_supports;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
GetFeatureSupport(VdpVideoMixer mixer, uint32_t feature_count, VdpVideoMixerFeature const *features,
                  VdpBool *feature_supports)
{
    return check_for_exceptions(GetFeatureSupportImpl, mixer, feature_count, features,
                                feature_supports);
}

VdpStatus
GetParameterValuesImpl(VdpVideoMixer mixer, uint32_t parameter_count,
                       VdpVideoMixerParameter const *parameters, void *const *parameter_values)
{
    std::ignore = mixer;
    std::ignore = parameter_count;
    std::ignore = parameters;
    std::ignore = parameter_values;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
GetParameterValues(VdpVideoMixer mixer, uint32_t parameter_count,
                   VdpVideoMixerParameter const *parameters, void *const *parameter_values)
{
    return check_for_exceptions(GetParameterValuesImpl, mixer, parameter_count, parameters,
                                parameter_values);
}

VdpStatus
QueryAttributeSupportImpl(VdpDevice device, VdpVideoMixerAttribute attribute, VdpBool *is_supported)
{
    std::ignore = device;
    std::ignore = attribute;
    std::ignore = is_supported;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryAttributeSupport(VdpDevice device, VdpVideoMixerAttribute attribute, VdpBool *is_supported)
{
    return check_for_exceptions(QueryAttributeSupportImpl, device, attribute, is_supported);
}

VdpStatus
QueryAttributeValueRangeImpl(VdpDevice device, VdpVideoMixerAttribute attribute, void *min_value,
                             void *max_value)
{
    std::ignore = device;
    std::ignore = attribute;
    std::ignore = min_value;
    std::ignore = max_value;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryAttributeValueRange(VdpDevice device, VdpVideoMixerAttribute attribute, void *min_value,
                         void *max_value)
{
    return check_for_exceptions(QueryAttributeValueRangeImpl, device, attribute, min_value,
                                max_value);
}

VdpStatus
QueryFeatureSupportImpl(VdpDevice device, VdpVideoMixerFeature feature, VdpBool *is_supported)
{
    std::ignore = device;
    std::ignore = feature;
    std::ignore = is_supported;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryFeatureSupport(VdpDevice device, VdpVideoMixerFeature feature, VdpBool *is_supported)
{
    return check_for_exceptions(QueryFeatureSupportImpl, device, feature, is_supported);
}

VdpStatus
QueryParameterSupportImpl(VdpDevice device, VdpVideoMixerParameter parameter, VdpBool *is_supported)
{
    std::ignore = device;
    std::ignore = parameter;
    std::ignore = is_supported;

    return VDP_STATUS_NO_IMPLEMENTATION;
}

VdpStatus
QueryParameterSupport(VdpDevice device, VdpVideoMixerParameter parameter, VdpBool *is_supported)
{
    return check_for_exceptions(QueryParameterSupportImpl, device, parameter, is_supported);
}

VdpStatus
QueryParameterValueRangeImpl(VdpDevice device, VdpVideoMixerParameter parameter, void *min_value,
                             void *max_value)
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
QueryParameterValueRange(VdpDevice device, VdpVideoMixerParameter parameter, void *min_value,
                         void *max_value)
{
    return check_for_exceptions(QueryParameterValueRangeImpl, device, parameter, min_value,
                                max_value);
}

VdpStatus
RenderImpl(VdpVideoMixer mixer_id, VdpOutputSurface background_surface,
           VdpRect const *background_source_rect,
           VdpVideoMixerPictureStructure current_picture_structure,
           uint32_t video_surface_past_count, VdpVideoSurface const *video_surface_past,
           VdpVideoSurface video_surface_current, uint32_t video_surface_future_count,
           VdpVideoSurface const *video_surface_future, VdpRect const *video_source_rect,
           VdpOutputSurface destination_surface, VdpRect const *destination_rect,
           VdpRect const *destination_video_rect, uint32_t layer_count, VdpLayer const *layers)
{
    std::ignore = mixer_id;    // TODO: mixer should be used to get mixing parameters
    // TODO: current implementation ignores previous and future surfaces, using only current.
    // Is that acceptable for interlaced video? Will VAAPI handle deinterlacing?

    std::ignore = background_surface;   // TODO: background_surface. Is it safe to just ignore it?
    std::ignore = background_source_rect;
    std::ignore = current_picture_structure;
    std::ignore = video_surface_past_count;
    std::ignore = video_surface_past;
    std::ignore = video_surface_future_count;
    std::ignore = video_surface_future;
    std::ignore = layer_count;
    std::ignore = layers;

    ResourceRef<Resource> mixer{mixer_id};
    ResourceRef<vdp::VideoSurface::Resource> src_surf{video_surface_current};
    ResourceRef<vdp::OutputSurface::Resource> dst_surf{destination_surface};

    if (src_surf->device->id != dst_surf->device->id ||
        src_surf->device->id != mixer->device->id)
    {
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
    }

    VdpRect srcVideoRect = {0, 0, src_surf->width, src_surf->height};
    if (video_source_rect)
        srcVideoRect = *video_source_rect;

    VdpRect dstRect = {0, 0, dst_surf->width, dst_surf->height};
    if (destination_rect)
        dstRect = *destination_rect;

    VdpRect dstVideoRect = srcVideoRect;
    if (destination_video_rect)
        dstVideoRect = *destination_video_rect;

    // TODO: dstRect should clip dstVideoRect

    GLXThreadLocalContext guard{mixer->device};

    if (src_surf->sync_va_to_glx) {
        render_va_surf_to_texture(mixer, src_surf);
        src_surf->sync_va_to_glx = false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, dst_surf->fbo_id);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, dst_surf->width, 0, dst_surf->height, -1.0f, 1.0f);
    glViewport(0, 0, dst_surf->width, dst_surf->height);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0f/src_surf->width, 1.0f/src_surf->height, 1.0f);

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
    glBindTexture(GL_TEXTURE_2D, src_surf->tex_id);
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

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("VideoMixer::RenderImpl(): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
Render(VdpVideoMixer mixer_id, VdpOutputSurface background_surface,
       VdpRect const *background_source_rect,
       VdpVideoMixerPictureStructure current_picture_structure, uint32_t video_surface_past_count,
       VdpVideoSurface const *video_surface_past, VdpVideoSurface video_surface_current,
       uint32_t video_surface_future_count, VdpVideoSurface const *video_surface_future,
       VdpRect const *video_source_rect, VdpOutputSurface destination_surface,
       VdpRect const *destination_rect, VdpRect const *destination_video_rect, uint32_t layer_count,
       VdpLayer const *layers)
{
    return check_for_exceptions(RenderImpl, mixer_id, background_surface, background_source_rect,
                                current_picture_structure, video_surface_past_count,
                                video_surface_past, video_surface_current,
                                video_surface_future_count, video_surface_future, video_source_rect,
                                destination_surface, destination_rect, destination_video_rect,
                                layer_count, layers);
}

VdpStatus
SetAttributeValuesImpl(VdpVideoMixer mixer, uint32_t attribute_count,
                       VdpVideoMixerAttribute const *attributes,
                       void const *const *attribute_values)
{
    std::ignore = mixer;
    std::ignore = attribute_count;
    std::ignore = attributes;
    std::ignore = attribute_values;

    return VDP_STATUS_OK;
}

VdpStatus
SetAttributeValues(VdpVideoMixer mixer, uint32_t attribute_count,
                   VdpVideoMixerAttribute const *attributes, void const *const *attribute_values)
{
    return check_for_exceptions(SetAttributeValuesImpl, mixer, attribute_count, attributes,
                                attribute_values);
}

VdpStatus
SetFeatureEnablesImpl(VdpVideoMixer mixer, uint32_t feature_count,
                      VdpVideoMixerFeature const *features, VdpBool const *feature_enables)
{
    std::ignore = mixer;
    std::ignore = feature_count;
    std::ignore = features;
    std::ignore = feature_enables;

    return VDP_STATUS_OK;
}

VdpStatus
SetFeatureEnables(VdpVideoMixer mixer, uint32_t feature_count, VdpVideoMixerFeature const *features,
                  VdpBool const *feature_enables)
{
    return check_for_exceptions(SetFeatureEnablesImpl, mixer, feature_count, features,
                                feature_enables);
}

} } // namespace vdp::VideoMixer
