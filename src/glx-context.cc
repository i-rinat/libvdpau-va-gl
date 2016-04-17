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

/*
 *  GLX context related helpers
 */

#include "api-device.hh"
#include "compat.hh"
#include "globals.hh"
#include "glx-context.hh"
#include "trace.hh"
#include <assert.h>
#include <map>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <vector>


using std::vector;


namespace {

std::map<thread_id_t, vdp::GLXManagedContext> g_glc_map;
std::recursive_mutex    g_glc_mutex;
GLXContext              g_root_glc;
int                     g_root_glc_refcnt;
XVisualInfo            *g_root_vi;

int             x11_error_code = 0;

} // anonymouse namespace

namespace vdp {

GLXManagedContext &
GLXManagedContext::operator=(GLXManagedContext &&that)
{
    if (this == &that)
        return *this;

    glc_ = that.glc_;
    that.glc_ = nullptr;

    return *this;
}

GLXManagedContext::GLXManagedContext(GLXManagedContext &&other)
    : dpy_{}
    , glc_{other.glc_}
{
    other.glc_ = nullptr;
}

void
GLXManagedContext::destroy()
{
    if (glc_ == nullptr)
        return;

    if (glc_ == glXGetCurrentContext())
        glXMakeCurrent(dpy_.get(), None, nullptr);

    glXDestroyContext(dpy_.get(), glc_);

    glc_ = nullptr;
}

GLXThreadLocalContext::GLXThreadLocalContext(std::shared_ptr<vdp::Device::Resource> device,
                                             bool restore_previous_context)
    : GLXThreadLocalContext(device->root, restore_previous_context)
{
}

GLXThreadLocalContext::GLXThreadLocalContext(Window wnd, bool restore_previous_context)
    : restore_previous_context_(restore_previous_context)
{
    g_glc_mutex.lock();

    XDisplayRef       dpy_ref{};
    Display *const    dpy = dpy_ref.get();
    const thread_id_t thread_id = get_current_thread_id();

    prev_dpy_ = glXGetCurrentDisplay();
    if (!prev_dpy_)
        prev_dpy_ = dpy;

    prev_wnd_ = glXGetCurrentDrawable();
    prev_glc_ = glXGetCurrentContext();

    GLXContext glc;
    auto val = g_glc_map.find(thread_id);
    if (val == g_glc_map.end()) {
        glc = glXCreateContext(dpy, g_root_vi, g_root_glc, GL_TRUE);
        assert(glc);

        g_glc_map.emplace(thread_id, GLXManagedContext(glc));

        // find which threads are not alive already
        vector<thread_id_t> dead_threads;
        for (const auto &it: g_glc_map)
            if (not thread_is_alive(it.first))
                dead_threads.push_back(it.first);

        // and delete every context associated with them
        for (const auto &it: dead_threads)
            g_glc_map.erase(it);

    } else {
        glc = val->second.get();
    }

    glXMakeCurrent(dpy, wnd, glc);
}

GLXThreadLocalContext::~GLXThreadLocalContext()
{
    if (restore_previous_context_)
        glXMakeCurrent(prev_dpy_, prev_wnd_, prev_glc_);
    else
        glXMakeCurrent(prev_dpy_, None, nullptr);

    g_glc_mutex.unlock();
}

GLXLockGuard::GLXLockGuard()
{
    g_glc_mutex.lock();
}

GLXLockGuard::~GLXLockGuard()
{
    g_glc_mutex.unlock();
}

GLXGlobalContext::GLXGlobalContext(Display *dpy, int screen)
    : dpy_{dpy}
{
    std::unique_lock<decltype(g_glc_mutex)> lock{g_glc_mutex};

    g_root_glc_refcnt += 1;
    if (g_root_glc_refcnt > 1)
        return;

    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

    g_root_vi = glXChooseVisual(dpy, screen, att);
    if (!g_root_vi) {
        traceError("GLXGlobalContext::GLXGlobalContext: glXChooseVisual failed\n");
        throw std::bad_alloc();
    }

    g_root_glc = glXCreateContext(dpy, g_root_vi, NULL, GL_TRUE);
    if (!g_root_glc)
        throw std::bad_alloc();
}

GLXGlobalContext::~GLXGlobalContext()
{
    try {
        std::unique_lock<decltype(g_glc_mutex)> lock{g_glc_mutex};

        g_root_glc_refcnt -= 1;

        if (g_root_glc_refcnt <= 0) {
            // destroying global GL context
            glXMakeCurrent(dpy_, None, nullptr);
            glXDestroyContext(dpy_, g_root_glc);
            XFree(g_root_vi);

            // destroying all per-thread GL contexts
            g_glc_map.clear();
        }

    } catch (...) {
        traceError("GLXGlobalContext::~GLXGlobalContext(): caught exception\n");
    }
}

GLXContext
GLXGlobalContext::get() const
{
    std::unique_lock<decltype(g_glc_mutex)> lock{g_glc_mutex};

    if (g_root_glc_refcnt > 0)
        return g_root_glc;
    else
        return nullptr;
}

} // namespace vdp

static
int
x11_error_handler(Display *, XErrorEvent *ee)
{
    x11_error_code = ee->error_code;
    return 0;
}

void
x11_push_eh()
{
    x11_error_code = 0;
    XSetErrorHandler(&x11_error_handler);
}

int
x11_pop_eh()
{
    // restoring error handler is too risky, leave it as is
    return x11_error_code;
}
