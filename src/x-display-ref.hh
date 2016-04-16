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

#include <mutex>
#include <X11/Xlib.h>


namespace vdp {

class XDisplayRef {
public:
    XDisplayRef(bool one_more_ref = false)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);

        const bool ref_cnt_was_zero = (ref_cnt_ == 0);

        ref_cnt_ += 1;
        if (one_more_ref)
            ref_cnt_ += 1;

        if (ref_cnt_was_zero) {
            dpy_ = XOpenDisplay(nullptr);

            // TODO: do we need to throw if (dpy_ == nullptr)?
        }
    }

    ~XDisplayRef()
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);

        ref_cnt_ -= 1;
        if (ref_cnt_ <= 0) {
            XCloseDisplay(dpy_);
            dpy_ = nullptr;
        }
    }

    XDisplayRef(const XDisplayRef &ref) = delete;

    XDisplayRef &
    operator=(const XDisplayRef &that) = delete;

    Display *
    get() const { return dpy_; }

private:
    static Display    *dpy_;
    static std::mutex  mtx_;
    static int         ref_cnt_;
};

} // namespace vdp
