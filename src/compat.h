/*
 * Copyright 2013-2014  Rinat Ibragimov
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

#ifndef VA_GL_SRC_COMPAT_H
#define VA_GL_SRC_COMPAT_H

#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <unistd.h>
#include <sys/syscall.h>
#ifdef __FreeBSD__
#include <sys/thr.h>
#endif

#if (LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51, 42, 0)) || \
    (LIBAVUTIL_VERSION_INT == AV_VERSION_INT(51, 73, 101))

#define AV_PIX_FMT_NONE         PIX_FMT_NONE
#define AV_PIX_FMT_NV12         PIX_FMT_NV12
#define AV_PIX_FMT_YUV420P      PIX_FMT_YUV420P
#define AV_PIX_FMT_UYVY422      PIX_FMT_UYVY422
#define AV_PIX_FMT_YUYV422      PIX_FMT_YUYV422

#endif // old libavutil version


#if defined(__linux__)
typedef int thread_id_t;
#elif defined(__FreeBSD__)
typedef long thread_id_t;
#else
#error Unknown OS
#endif

static inline thread_id_t
get_current_thread_id(void)
{
#if defined(__linux__)
    return syscall(__NR_gettid);
#elif defined(__FreeBSD__)
    long thread_id;
    thr_self(&thread_id);
    return thread_id;
#endif
}

static inline size_t
thread_is_alive(thread_id_t tid)
{
#if defined(__linux__)
    return kill(tid, 0) == 0;
#elif defined(__FreeBSD__)
    return thr_kill(tid, 0) == 0;
#endif
}


#endif // VA_GL_SRC_COMPAT_H
