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
#include "api-bitmap-surface.hh"
#include "api-csc-matrix.hh"
#include "api-decoder.hh"
#include "api-device.hh"
#include "api-output-surface.hh"
#include "api-presentation-queue.hh"
#include "api-video-mixer.hh"
#include "api-video-surface.hh"
#include "globals.hh"
#include "glx-context.hh"
#include "handle-storage.hh"
#include "reverse-constant.hh"
#include "trace.hh"
#include "watermark.hh"
#include <GL/gl.h>
#include <map>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <va/va_x11.h>
#include <vdpau/vdpau_x11.h>
#include <vector>


using std::map;
using std::mutex;
using std::pair;
using std::string;

namespace vdp {

class shader_compilation_failed: public std::exception
{
};

const string kImplemetationDescriptionString{"OpenGL/VAAPI backend for VDPAU"};

} // namespace vdp


namespace vdp {

VdpStatus
GetApiVersion(uint32_t *api_version)
{
    if (api_version)
        *api_version = VDPAU_VERSION;

    return VDP_STATUS_OK;
}

const char *
GetErrorString(VdpStatus status)
{
    return reverse_status(status);
}

VdpStatus
GetInformationString(char const **information_string)
{
    if (information_string)
        *information_string = kImplemetationDescriptionString.c_str();

    return VDP_STATUS_OK;
}

VdpStatus
GetProcAddress(VdpDevice, VdpFuncId function_id, void **function_pointer)
{
    if (!function_pointer)
        return VDP_STATUS_INVALID_POINTER;

    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = reinterpret_cast<void *>(&vdp::GetErrorString);
        break;

    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = reinterpret_cast<void *>(&vdp::GetProcAddress);
        break;

    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = reinterpret_cast<void *>(&vdp::GetApiVersion);
        break;

    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = reinterpret_cast<void *>(&vdp::GetInformationString);
        break;

    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::Device::Destroy);
        break;

    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = reinterpret_cast<void *>(&vdp::GenerateCSCMatrix);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoSurface::QueryCapabilities);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(
                            &vdp::VideoSurface::QueryGetPutBitsYCbCrCapabilities);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoSurface::Create);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoSurface::Destroy);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoSurface::GetParameters);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoSurface::GetBitsYCbCr);
        break;

    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoSurface::PutBitsYCbCr);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::QueryCapabilities);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(&
                            vdp::OutputSurface::QueryGetPutBitsNativeCapabilities);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(
                            &vdp::OutputSurface::QueryPutBitsIndexedCapabilities);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(
                            &vdp::OutputSurface::QueryPutBitsYCbCrCapabilities);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::Create);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::Destroy);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::GetParameters);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::GetBitsNative);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::PutBitsNative);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::PutBitsIndexed);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::PutBitsYCbCr);
        break;

    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(&vdp::BitmapSurface::QueryCapabilities);
        break;

    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = reinterpret_cast<void *>(&vdp::BitmapSurface::Create);
        break;

    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::BitmapSurface::Destroy);
        break;

    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = reinterpret_cast<void *>(&vdp::BitmapSurface::GetParameters);
        break;

    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = reinterpret_cast<void *>(&vdp::BitmapSurface::PutBitsNative);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::RenderOutputSurface);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = reinterpret_cast<void *>(&vdp::OutputSurface::RenderBitmapSurface);
        break;

    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // deprecated by the spec
        *function_pointer = nullptr;
        break;

    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = reinterpret_cast<void *>(&vdp::Decoder::QueryCapabilities);
        break;

    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = reinterpret_cast<void *>(&vdp::Decoder::Create);
        break;

    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::Decoder::Destroy);
        break;

    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = reinterpret_cast<void *>(&vdp::Decoder::GetParameters);
        break;

    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = reinterpret_cast<void *>(&vdp::Decoder::Render);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::QueryFeatureSupport);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::QueryParameterSupport);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::QueryAttributeSupport);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::QueryParameterValueRange);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::QueryAttributeValueRange);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::Create);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::SetFeatureEnables);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::SetAttributeValues);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::GetFeatureSupport);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::GetFeatureEnables);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::GetParameterValues);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::GetAttributeValues);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::Destroy);
        break;

    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = reinterpret_cast<void *>(&vdp::VideoMixer::Render);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::TargetDestroy);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::Create);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::Destroy);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::SetBackgroundColor);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::GetBackgroundColor);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::GetTime);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::Display);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = reinterpret_cast<void *>(
                            &vdp::PresentationQueue::BlockUntilSurfaceIdle);
        break;

    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::QuerySurfaceStatus);
        break;

    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = reinterpret_cast<void *>(&vdp::PreemptionCallbackRegister);
        break;

    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = reinterpret_cast<void *>(&vdp::PresentationQueue::TargetCreateX11);
        break;

    default:
        *function_pointer = nullptr;
        break;
    } // switch

    if (*function_pointer == nullptr)
        return VDP_STATUS_INVALID_FUNC_ID;

    return VDP_STATUS_OK;
}

VdpStatus
PreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    std::ignore = device;
    std::ignore = callback;
    std::ignore = context;

    return VDP_STATUS_OK;
}

namespace Device {

Resource::Resource(Display *a_display, int a_screen)
    : dpy{not not global.quirks.buggy_XCloseDisplay}
    , screen{a_screen}
    , glc{dpy.get(), screen}
{
    {
        GLXLockGuard glx_lock_guard;

        root = DefaultRootWindow(dpy.get());

        XWindowAttributes wnd_attrs;
        XGetWindowAttributes(dpy.get(), root, &wnd_attrs);
        color_depth = wnd_attrs.depth;

        fn.glXBindTexImageEXT =
            (PFNGLXBINDTEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *)"glXBindTexImageEXT");
        fn.glXReleaseTexImageEXT =
            (PFNGLXRELEASETEXIMAGEEXTPROC)glXGetProcAddress((GLubyte *)"glXReleaseTexImageEXT");
    }

    if (!fn.glXBindTexImageEXT || !fn.glXReleaseTexImageEXT) {
        traceError("error (%s): can't get glXBindTexImageEXT address\n");
        throw std::bad_alloc();
    }

    GLXThreadLocalContext glc_guard{root};

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // initialize VAAPI
    va_available = 0;
    if (global.quirks.avoid_va) {
        // pretend there is no VA-API available
        va_dpy = nullptr;
    } else {
        va_dpy = vaGetDisplay(dpy.get());

        VAStatus status = vaInitialize(va_dpy, &va_version_major, &va_version_minor);
        if (status == VA_STATUS_SUCCESS)
            va_available = 1;
    }

    compile_shaders();

    glGenTextures(1, &watermark_tex_id);
    glBindTexture(GL_TEXTURE_2D, watermark_tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, watermark_width, watermark_height, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, watermark_data);
    glFinish();

    const auto gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        traceError("Device::Resource::Resource(): gl error %d\n", gl_error);
        throw vdp::generic_error();
    }
}

template<typename T>
void
destroy_orphaned_resources(VdpDevice device_id)
{
    for (const auto res_id: ResourceStorage<T>::instance().enumerate()) {
        try {
            ResourceRef<T> res{res_id};

            if (res->device->id == device_id)
                ResourceStorage<T>::instance().drop(res_id);

        } catch (const vdp::resource_not_found &) {
            // ignore missing resources
        }
    }
}

Resource::~Resource()
{
    try {
        // cleaup libva
        vaTerminate(va_dpy);

        {
            GLXThreadLocalContext guard{root};

            glDeleteTextures(1, &watermark_tex_id);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            destroy_shaders();
        }

        {
            GLXLockGuard guard;
            glXMakeCurrent(dpy.get(), None, nullptr);
        }

        const auto gl_error = glGetError();
        if (gl_error != GL_NO_ERROR)
            traceError("Device::Resource::~Resource(): gl error %d\n", gl_error);

    } catch (...) {
        traceError("Device::Resource::~Resource(): caught exception\n");
    }
}

void
Resource::compile_shaders()
{
    for (int k = 0; k < SHADER_COUNT; k ++) {
        struct shader_s *s = &glsl_shaders[k];
        int ok;

        const GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(f_shader, 1, &s->body, &s->len);
        glCompileShader(f_shader);
        glGetShaderiv(f_shader, GL_COMPILE_STATUS, &ok);

        if (!ok) {
            GLint errmsg_len;
            glGetShaderiv(f_shader, GL_INFO_LOG_LENGTH, &errmsg_len);

            std::vector<char> errmsg(errmsg_len);
            glGetShaderInfoLog(f_shader, errmsg.size(), nullptr, errmsg.data());
            traceError("Device::Resource::compile_shaders(): compilation of shader #%d failed with "
                       "'%s'\n", k, errmsg.data());
            glDeleteShader(f_shader);
            throw shader_compilation_failed();
        }

        const GLuint program = glCreateProgram();
        glAttachShader(program, f_shader);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &ok);

        if (!ok) {
            GLint errmsg_len;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errmsg_len);

            std::vector<char> errmsg(errmsg_len);
            glGetProgramInfoLog(program, errmsg.size(), nullptr, errmsg.data());
            traceError("Device::Resource::compile_shaders(): linking of shader #%d failed with "
                       "'%s'\n", k, errmsg.data());
            glDeleteProgram(program);
            glDeleteShader(f_shader);
            throw shader_compilation_failed();
        }

        shaders[k].f_shader = f_shader;
        shaders[k].program = program;

        switch (k) {
        case glsl_YV12_RGBA:
        case glsl_NV12_RGBA:
            shaders[k].uniform.tex_0 = glGetUniformLocation(program, "tex[0]");
            shaders[k].uniform.tex_1 = glGetUniformLocation(program, "tex[1]");
            break;

        case glsl_red_to_alpha_swizzle:
            shaders[k].uniform.tex_0 = glGetUniformLocation(program, "tex_0");
            break;
        }
    }
}

void
Resource::destroy_shaders()
{
    for (int k = 0; k < SHADER_COUNT; k ++) {
        glDeleteProgram(shaders[k].program);
        glDeleteShader(shaders[k].f_shader);
    }
}

VdpStatus
CreateX11Impl(Display *display_orig, int screen, VdpDevice *device,
              VdpGetProcAddress **get_proc_address)
{
    if (!display_orig || !device)
        return VDP_STATUS_INVALID_POINTER;

    auto data = std::make_shared<Resource>(display_orig, screen);

    *device = vdp::ResourceStorage<Resource>::instance().insert(data);

    if (get_proc_address)
        *get_proc_address = &vdp::GetProcAddress;

    return VDP_STATUS_OK;
}

VdpStatus
CreateX11(Display *display_orig, int screen, VdpDevice *device,
          VdpGetProcAddress **get_proc_address)
{
    return check_for_exceptions(CreateX11Impl, display_orig, screen, device, get_proc_address);
}

VdpStatus
DestroyImpl(VdpDevice device_id)
{
    ResourceRef<Resource> device{device_id};

    ResourceStorage<Resource>::instance().drop(device_id);

    destroy_orphaned_resources<vdp::BitmapSurface::Resource>(device_id);
    destroy_orphaned_resources<vdp::Decoder::Resource>(device_id);
    destroy_orphaned_resources<vdp::OutputSurface::Resource>(device_id);
    destroy_orphaned_resources<vdp::PresentationQueue::Resource>(device_id);
    destroy_orphaned_resources<vdp::PresentationQueue::TargetResource>(device_id);
    destroy_orphaned_resources<vdp::VideoMixer::Resource>(device_id);
    destroy_orphaned_resources<vdp::VideoSurface::Resource>(device_id);

    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpDevice device_id)
{
    return check_for_exceptions(DestroyImpl, device_id);
}

} } // namespace vdp::Device
