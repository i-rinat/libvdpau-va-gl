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

#include "x-display-ref.hh"
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <memory>


namespace vdp {

namespace Device {

struct Resource;

} // namespace Device

class GLXManagedContext
{
public:
    explicit
    GLXManagedContext(GLXContext glc)
        : dpy_{}
        , glc_{glc}
    {}

    GLXManagedContext(GLXManagedContext &&other);

    ~GLXManagedContext()
    {
        destroy();
    }

    GLXManagedContext &
    operator=(GLXManagedContext &&that);

    GLXContext
    get() const { return glc_; }

private:
    XDisplayRef dpy_;
    GLXContext  glc_;

    void
    destroy();
};

class GLXThreadLocalContext
{
public:
    explicit
    GLXThreadLocalContext(Window wnd, bool restore_previous_context = true);

    explicit
    GLXThreadLocalContext(std::shared_ptr<vdp::Device::Resource> device,
                          bool restore_previous_context = true);

    ~GLXThreadLocalContext();

    GLXThreadLocalContext(const GLXThreadLocalContext &) = delete;

    GLXThreadLocalContext &
    operator=(const GLXThreadLocalContext &) = delete;

private:
    Display   *prev_dpy_;
    Window     prev_wnd_;
    GLXContext prev_glc_;
    bool       restore_previous_context_;
};

class GLXLockGuard
{
public:
    GLXLockGuard();
    ~GLXLockGuard();
};

class GLXGlobalContext
{
public:
    GLXGlobalContext(Display *dpy, int screen);

    ~GLXGlobalContext();

    GLXContext
    get() const;

private:
    GLXGlobalContext &
    operator=(const GLXGlobalContext &that) = delete;

    Display    *dpy_;
};

} // namespace vdp

void
x11_push_eh();

int
x11_pop_eh();
