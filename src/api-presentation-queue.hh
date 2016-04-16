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
#include <GL/glx.h>
#include <memory>
#include <thread>
#include <vdpau/vdpau_x11.h>


namespace vdp { namespace PresentationQueue {

struct TargetResource: public vdp::GenericResource
{
    TargetResource(std::shared_ptr<vdp::Device::Resource> device, Drawable drawable);

    ~TargetResource();

    void
    recreate_pixmaps_if_geometry_changed();

    Drawable        drawable;       ///< X drawable to output to
    uint32_t        drawable_width; ///< last seen drawable width
    uint32_t        drawable_height;///< last seen drawable height
    Pixmap          pixmap;         ///< draw buffer
    GLXPixmap       glx_pixmap;     ///< GLX pixmap proxy
    GC              plain_copy_gc;  ///< X GC for displaying buffer content
    GLXContext      glc;            ///< GL context used for output
    XVisualInfo    *xvi;

private:
    void
    free_glx_pixmaps();
};

class PresentationQueueThreadRef
{
public:
    PresentationQueueThreadRef();

    ~PresentationQueueThreadRef();

private:
    static void
    thread_body();

    void
    do_start_thread();

    static std::thread  t_;
    static int          thread_refs_;   // amount of users that want thread alive
};

struct Resource: public vdp::GenericResource
{
    Resource(std::shared_ptr<vdp::Device::Resource> a_device,
             std::shared_ptr<TargetResource> a_target);

    ~Resource();

    std::shared_ptr<TargetResource> target;
    VdpColor                        bg_color;   ///< background color

private:
    PresentationQueueThreadRef      presentation_thread_reference;
};

VdpPresentationQueueCreate                  Create;
VdpPresentationQueueDestroy                 Destroy;
VdpPresentationQueueSetBackgroundColor      SetBackgroundColor;
VdpPresentationQueueGetBackgroundColor      GetBackgroundColor;
VdpPresentationQueueGetTime                 GetTime;
VdpPresentationQueueDisplay                 Display;
VdpPresentationQueueBlockUntilSurfaceIdle   BlockUntilSurfaceIdle;
VdpPresentationQueueQuerySurfaceStatus      QuerySurfaceStatus;
VdpPresentationQueueTargetCreateX11         TargetCreateX11;
VdpPresentationQueueTargetDestroy           TargetDestroy;

} } // namespace vdp::PresentationQueue
