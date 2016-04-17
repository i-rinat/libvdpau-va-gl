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

#pragma once

#include "api.hh"
#include "compat-vdpau.hh"
#include "glx-context.hh"
#include "shaders.h"
#include "x-display-ref.hh"
#include <GL/glx.h>
#include <map>
#include <mutex>
#include <va/va_x11.h>


namespace vdp { namespace Device {

struct Resource: public vdp::GenericResource
{
    Resource(Display *a_dpy, int a_screen);

    ~Resource();

    vdp::XDisplayRef    dpy;            ///< own X display connection
    int                 screen;         ///< X screen
    int                 color_depth;    ///< screen color depth
    GLXGlobalContext    glc;            ///< master GL context
    Window              root;           ///< X drawable (root window) used for offscreen drawing
    VADisplay           va_dpy;         ///< VA display
    int                 va_available;   ///< 1 if VA-API available
    int                 va_version_major;
    int                 va_version_minor;
    GLuint              watermark_tex_id;   ///< GL texture id for watermark
    struct {
        GLuint      f_shader;
        GLuint      program;
        struct {
            int     tex_0;
            int     tex_1;
        } uniform;
    } shaders[SHADER_COUNT];
    struct {
        PFNGLXBINDTEXIMAGEEXTPROC       glXBindTexImageEXT;
        PFNGLXRELEASETEXIMAGEEXTPROC    glXReleaseTexImageEXT;
    } fn;

private:
    void
    compile_shaders();

    void
    destroy_shaders();
};


VdpStatus
CreateX11(Display *display, int screen, VdpDevice *device, VdpGetProcAddress **get_proc_address);

VdpDeviceDestroy                Destroy;

} // namespace Device

VdpGetApiVersion                GetApiVersion;
VdpGetErrorString               GetErrorString;
VdpGetInformationString         GetInformationString;
VdpGetProcAddress               GetProcAddress;
VdpPreemptionCallbackRegister   PreemptionCallbackRegister;

} // namespace vdp
