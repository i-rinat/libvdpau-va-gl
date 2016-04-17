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
#include "api-output-surface.hh"
#include "api-presentation-queue.hh"
#include "compat-vdpau.hh"
#include "globals.hh"
#include "glx-context.hh"
#include "handle-storage.hh"
#include "trace.hh"
#include "watermark.hh"
#include <GL/gl.h>
#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <errno.h>
#include <queue>
#include <set>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>


using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::make_shared;
using std::shared_ptr;

namespace {

struct Task
{
    Task()
        : when{}
        , clip_width{0}
        , clip_height{0}
        , wipe_tasks{false}
        , shutdown_thread{false}
        , pq_id{VDP_INVALID_HANDLE}
        , surface_id{VDP_INVALID_HANDLE}
    {}

    timespec        when;
    uint32_t        clip_width;
    uint32_t        clip_height;
    bool            wipe_tasks;
    bool            shutdown_thread;
    VdpPresentationQueue    pq_id;
    VdpOutputSurface        surface_id;

    bool
    operator<(const Task &that) const
    {
        if (this->when.tv_sec < that.when.tv_sec)
            return true;
        if (this->when.tv_sec > that.when.tv_sec)
            return false;
        if (this->when.tv_nsec < that.when.tv_nsec)
            return true;

        return false;
    }
};

std::queue<Task>        g_task_queue;
std::mutex              g_task_queue_mtx;
std::condition_variable g_task_queue_cv;

} // anonymous namespace

namespace vdp { namespace PresentationQueue {

void
PresentationQueueThreadRef::do_start_thread()
{
    t_ = std::thread([this] () {
        thread_body();
    });
}

PresentationQueueThreadRef::PresentationQueueThreadRef()
{
    vdp::GLXLockGuard guard;

    if (thread_refs_ == 0)
        do_start_thread();

    thread_refs_ += 1;
}

PresentationQueueThreadRef::~PresentationQueueThreadRef()
{
    try {
        {
            vdp::GLXLockGuard guard;

            if (thread_refs_ > 1) {
                thread_refs_ -= 1;

                // there are still other users, do nothing now
                return;
            }
        }

        // here thread_refs_ equals 1, which means this instance is the only user left. Ask worker
        // thread to stop.

        {
            std::unique_lock<decltype(g_task_queue_mtx)> lock{g_task_queue_mtx};

            Task task;
            task.shutdown_thread = true;

            g_task_queue.push(task);
            g_task_queue_cv.notify_one();
        }

        t_.join();

        {
            vdp::GLXLockGuard guard;

            thread_refs_ -= 1;

            if (thread_refs_ > 0) {
                // there is at least one another instance which increased reference count. But since
                // we were holding one reference, that instance didn't start the thread.
                // Start it now.

                do_start_thread();
            }
        }

    } catch (...) {
        traceError("PresentationQueueThreadRef::~PresentationQueueThreadRef(): caught exception\n");
    }
}

std::thread
PresentationQueueThreadRef::t_;

int
PresentationQueueThreadRef::thread_refs_ = 0;

VdpTime
timespec2vdptime(struct timespec t)
{
    return (uint64_t)t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
}

struct timespec
vdptime2timespec(VdpTime t)
{
    struct timespec res;
    res.tv_sec = t / (1000*1000*1000);
    res.tv_nsec = t % (1000*1000*1000);
    return res;
}

void
do_presentation_queue_display(const Task &task)
{
    try {
        ResourceRef<vdp::PresentationQueue::Resource> pq{task.pq_id};
        ResourceRef<vdp::OutputSurface::Resource> surface{task.surface_id};

        const uint32_t clip_width = task.clip_width;
        const uint32_t clip_height = task.clip_height;

        vdp::GLXLockGuard guard;

        pq->target->recreate_pixmaps_if_geometry_changed();
        glXMakeCurrent(pq->device->dpy.get(), pq->target->glx_pixmap, pq->target->glc);

        const uint32_t target_width  = (clip_width > 0)  ? clip_width  : surface->width;
        const uint32_t target_height = (clip_height > 0) ? clip_height : surface->height;

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, target_width, target_height, 0, -1.0, 1.0);
        glViewport(0, 0, target_width, target_height);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f/surface->width, 1.0f/surface->height, 1.0f);

        glEnable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D, surface->tex_id);
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
            glBindTexture(GL_TEXTURE_2D, pq->device->watermark_tex_id);

            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();

            glColor4f(1.0, 1.0, 1.0, 0.2);
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

        glFinish();

        x11_push_eh();
        XCopyArea(pq->device->dpy.get(), pq->target->pixmap, pq->target->drawable,
                  pq->target->plain_copy_gc, 0, 0, target_width, target_height, 0, 0);
        XSync(pq->device->dpy.get(), False);

        int x11_err = x11_pop_eh();
        if (x11_err != Success) {
            char buf[200] = { 0 };
            XGetErrorText(pq->device->dpy.get(), x11_err, buf, sizeof(buf));
            traceError("PresentationQueue::do_presentation_queue_display(): caught X11 error %s\n",
                       buf);
        }

        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        surface->first_presentation_time = timespec2vdptime(now);
        surface->status = VDP_PRESENTATION_QUEUE_STATUS_IDLE;

        const auto gl_error = glGetError();
        if (gl_error != GL_NO_ERROR) {
            traceError("PresentationQueue::do_presentation_queue_display(): gl error %d\n",
                       gl_error);
        }

    } catch (const vdp::resource_not_found &) {
        // just ignoring
        return;
    }
}

void
PresentationQueueThreadRef::thread_body()
{
    std::set<Task> int_q;   // internal queue of task, always sorted

    while (true) {
        int64_t timeout;
        Task task;

        if (int_q.size() != 0) {
            // internal queue have a task
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);

            task = *int_q.begin();
            timeout = (task.when.tv_sec - now.tv_sec) * 1000 * 1000 +
                      (task.when.tv_nsec - now.tv_nsec) / 1000;

            if (timeout <= 0) {
                // task is ready to go, run it
                do_presentation_queue_display(task);

                // remove it from queue
                int_q.erase(int_q.begin());
                continue;
            }
        } else {
            // no tasks in queue, sleep for a while
            timeout = 1000 * 1000; // one second
        }

        {
            std::unique_lock<decltype(g_task_queue_mtx)> lock(g_task_queue_mtx);

            if (g_task_queue.size() == 0) {
                g_task_queue_cv.wait_for(lock, microseconds(timeout));
                continue;
            } else {
                task = g_task_queue.front();
                g_task_queue.pop();
            }
        }

        if (task.shutdown_thread)
            return;

        if (task.wipe_tasks) {
            // drop all tasks with the same queue id
            decltype(int_q) new_q;

            for (const auto &t: int_q) {
                if (t.pq_id != task.pq_id)
                    new_q.insert(t);
            }

            std::swap(int_q, new_q);
            continue;
        }

        int_q.insert(task);
    }
}

VdpStatus
BlockUntilSurfaceIdleImpl(VdpPresentationQueue presentation_queue, VdpOutputSurface surface_id,
                          VdpTime *first_presentation_time)

{
    // ensure presentation_queue is valid;
    {
        ResourceRef<Resource> pq{presentation_queue};
    }

    // TODO: use locking instead of busy loop
    while (true) {
        ResourceRef<vdp::OutputSurface::Resource> surface{surface_id};

        if (surface->status == VDP_PRESENTATION_QUEUE_STATUS_IDLE)
            break;

        usleep(1000);
    }

    if (first_presentation_time) {
        ResourceRef<vdp::OutputSurface::Resource> surface{surface_id};
        *first_presentation_time = surface->first_presentation_time;
    }

    return VDP_STATUS_OK;
}

VdpStatus
BlockUntilSurfaceIdle(VdpPresentationQueue presentation_queue, VdpOutputSurface surface_id,
                      VdpTime *first_presentation_time)
{
    return check_for_exceptions(BlockUntilSurfaceIdleImpl, presentation_queue, surface_id,
                                first_presentation_time);
}

VdpStatus
QuerySurfaceStatusImpl(VdpPresentationQueue presentation_queue, VdpOutputSurface surface_id,
                       VdpPresentationQueueStatus *status, VdpTime *first_presentation_time)
{
    ResourceRef<Resource> pq{presentation_queue};
    ResourceRef<vdp::OutputSurface::Resource> surface{surface_id};

    if (status)
        *status = surface->status;

    if (first_presentation_time)
        *first_presentation_time = surface->first_presentation_time;

    return VDP_STATUS_OK;
}

VdpStatus
QuerySurfaceStatus(VdpPresentationQueue presentation_queue, VdpOutputSurface surface_id,
                   VdpPresentationQueueStatus *status, VdpTime *first_presentation_time)
{
    return check_for_exceptions(QuerySurfaceStatusImpl, presentation_queue, surface_id, status,
                                first_presentation_time);
}

TargetResource::TargetResource(std::shared_ptr<vdp::Device::Resource> a_device,
                               Drawable a_drawable)
{
    device = a_device;
    drawable = a_drawable;

    // emulate geometry change. Hope there will be no drawables of such size
    drawable_width =  (uint32_t)(-1);
    drawable_height = (uint32_t)(-1);
    pixmap =          None;

    {
        GLXLockGuard  guard;
        auto         *dpy =    device->dpy.get();

        // No double buffering since we are going to render to glx pixmap
        GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
        xvi = glXChooseVisual(dpy, device->screen, att);
        if (!xvi) {
            traceError("PresentationQueue::TargetResource::TargetResource(): "
                       "glXChooseVisual failed\n");
            throw vdp::generic_error();
        }

        recreate_pixmaps_if_geometry_changed();

        // create context for dislaying result (can share display lists with device->glc)
        glc = glXCreateContext(dpy, xvi, device->glc.get(), GL_TRUE);
    }
}

TargetResource::~TargetResource()
{
    try {
        // drawable may be destroyed already, so it's a global context that should be activated
        {
            GLXThreadLocalContext guard{device, false}; // keep that context set afterwards
            glXDestroyContext(device->dpy.get(), glc);  // since previous was just destroyed
            free_glx_pixmaps();

            const auto gl_error = glGetError();

            if (gl_error != GL_NO_ERROR) {
                traceError("PresentationQueue::TargetResource::~TargetResource(): gl error %d\n",
                           gl_error);
            }
        }

        XFree(xvi);

    } catch (...) {
        traceError("PresentationQueue::TargetResource::~TargetResource(): caught exception\n");
    }
}

void
TargetResource::free_glx_pixmaps()
{
    auto *dpy = device->dpy.get();

    // if pixmap is None, nothing was allocated
    if (pixmap == None)
        return;

    glXDestroyGLXPixmap(dpy, glx_pixmap);
    XFreeGC(dpy, plain_copy_gc);
    XFreePixmap(dpy, pixmap);
    pixmap = None;
}

// create new pixmap, glx pixmap, GC if size has changed.
// This function relies on external serializing Xlib access
void
TargetResource::recreate_pixmaps_if_geometry_changed()
{
    Window          root_wnd;
    int             xpos, ypos;
    unsigned int    width, height, border_width, depth;
    auto           *dpy = device->dpy.get();

    XGetGeometry(dpy, drawable, &root_wnd, &xpos, &ypos, &width, &height, &border_width, &depth);

    if (width != drawable_width || height != drawable_height) {
        free_glx_pixmaps();
        drawable_width = width;
        drawable_height = height;

        pixmap = XCreatePixmap(dpy, device->root, drawable_width, drawable_height, depth);

        XGCValues gc_values = {};
        gc_values.function = GXcopy;
        gc_values.graphics_exposures = True;

        plain_copy_gc = XCreateGC(dpy, pixmap, GCFunction | GCGraphicsExposures, &gc_values);
        glx_pixmap = glXCreateGLXPixmap(dpy, xvi, pixmap);
        XSync(dpy, False);
    }
}

Resource::Resource(shared_ptr<vdp::Device::Resource> a_device, shared_ptr<TargetResource> a_target)
{
    device = a_device;
    target = a_target;

    bg_color.red = 0.0;
    bg_color.green = 0.0;
    bg_color.blue = 0.0;
    bg_color.alpha = 0.0;
}

Resource::~Resource()
{
    try {
        Task task;
        task.when =       vdptime2timespec(0);   // as early as possible
        task.pq_id =      id;
        task.wipe_tasks = true;

        {
            std::unique_lock<decltype(g_task_queue_mtx)> lock{g_task_queue_mtx};

            g_task_queue.push(task);
            g_task_queue_cv.notify_one();
        }

    } catch (...) {
        traceError("PresentationQueue::Resource::~Resource(): caught exception\n");
    }
}

VdpStatus
CreateImpl(VdpDevice device_id, VdpPresentationQueueTarget presentation_queue_target,
           VdpPresentationQueue *presentation_queue)
{
    if (!presentation_queue)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};
    ResourceRef<TargetResource> target{presentation_queue_target};

    auto data = make_shared<Resource>(device, target);

    *presentation_queue = ResourceStorage<Resource>::instance().insert(data);

    return VDP_STATUS_OK;
}

VdpStatus
Create(VdpDevice device_id, VdpPresentationQueueTarget presentation_queue_target,
       VdpPresentationQueue *presentation_queue)
{
    return check_for_exceptions(CreateImpl, device_id, presentation_queue_target,
                                presentation_queue);
}

VdpStatus
DestroyImpl(VdpPresentationQueue presentation_queue)
{
    ResourceRef<Resource> pq{presentation_queue};

    ResourceStorage<Resource>::instance().drop(presentation_queue);
    return VDP_STATUS_OK;
}

VdpStatus
Destroy(VdpPresentationQueue presentation_queue)
{
    return check_for_exceptions(DestroyImpl, presentation_queue);
}

VdpStatus
SetBackgroundColorImpl(VdpPresentationQueue presentation_queue, VdpColor *const background_color)
{
    ResourceRef<Resource> pq{presentation_queue};

    if (background_color) {
        pq->bg_color = *background_color;
    } else {
        pq->bg_color.red = 0.0;
        pq->bg_color.green = 0.0;
        pq->bg_color.blue = 0.0;
        pq->bg_color.alpha = 0.0;
    }

    return VDP_STATUS_OK;
}

VdpStatus
SetBackgroundColor(VdpPresentationQueue presentation_queue, VdpColor *const background_color)
{
    return check_for_exceptions(SetBackgroundColorImpl, presentation_queue, background_color);
}

VdpStatus
GetBackgroundColorImpl(VdpPresentationQueue presentation_queue, VdpColor *background_color)
{
    ResourceRef<Resource> pq{presentation_queue};

    if (background_color)
        *background_color = pq->bg_color;

    return VDP_STATUS_OK;
}

VdpStatus
GetBackgroundColor(VdpPresentationQueue presentation_queue, VdpColor *background_color)
{
    return check_for_exceptions(GetBackgroundColorImpl, presentation_queue, background_color);
}

VdpStatus
GetTime(VdpPresentationQueue, VdpTime *current_time)
{
    struct timespec now;

    clock_gettime(CLOCK_REALTIME, &now);
    if (current_time)
        *current_time = timespec2vdptime(now);

    return VDP_STATUS_OK;
}

VdpStatus
DisplayImpl(VdpPresentationQueue presentation_queue, VdpOutputSurface surface_id,
            uint32_t clip_width, uint32_t clip_height, VdpTime earliest_presentation_time)
{
    ResourceRef<Resource> pq{presentation_queue};
    ResourceRef<vdp::OutputSurface::Resource> surface{surface_id};

    if (pq->device->id != surface->device->id)
        return VDP_STATUS_HANDLE_DEVICE_MISMATCH;

    Task task;

    task.when =        vdptime2timespec(earliest_presentation_time);
    task.clip_width =  clip_width;
    task.clip_height = clip_height;
    task.surface_id =  surface_id;
    task.pq_id =       presentation_queue;

    surface->first_presentation_time = 0;
    surface->status = VDP_PRESENTATION_QUEUE_STATUS_QUEUED;

    {
        std::unique_lock<decltype(g_task_queue_mtx)> lock{g_task_queue_mtx};

        g_task_queue.push(task);
        g_task_queue_cv.notify_one();
    }

    return VDP_STATUS_OK;
}

VdpStatus
Display(VdpPresentationQueue presentation_queue, VdpOutputSurface surface_id, uint32_t clip_width,
        uint32_t clip_height, VdpTime earliest_presentation_time)
{
    return check_for_exceptions(DisplayImpl, presentation_queue, surface_id, clip_width,
                                clip_height, earliest_presentation_time);
}

VdpStatus
TargetCreateX11Impl(VdpDevice device_id, Drawable drawable, VdpPresentationQueueTarget *target)
{
    if (!target)
        return VDP_STATUS_INVALID_POINTER;

    ResourceRef<vdp::Device::Resource> device{device_id};

    auto data = make_shared<TargetResource>(device, drawable);

    *target = ResourceStorage<TargetResource>::instance().insert(data);

    return VDP_STATUS_OK;
}

VdpStatus
TargetCreateX11(VdpDevice device_id, Drawable drawable, VdpPresentationQueueTarget *target)
{
    return check_for_exceptions(TargetCreateX11Impl, device_id, drawable, target);
}

VdpStatus
TargetDestroyImpl(VdpPresentationQueueTarget presentation_queue_target)
{
    ResourceRef<TargetResource> target{presentation_queue_target};

    ResourceStorage<TargetResource>::instance().drop(presentation_queue_target);
    return VDP_STATUS_OK;
}

VdpStatus
TargetDestroy(VdpPresentationQueueTarget presentation_queue_target)
{
    return check_for_exceptions(TargetDestroyImpl, presentation_queue_target);
}

} } // namespace vdp::PresentationQueue
