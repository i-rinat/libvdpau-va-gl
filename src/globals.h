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

#ifndef VA_GL_SRC_GLOBALS_H
#define VA_GL_SRC_GLOBALS_H

#include <pthread.h>

/** @brief place where all shared global variables live */
struct global_data {
    /** @brief tunables */
    struct {
        int buggy_XCloseDisplay;    ///< avoid calling XCloseDisplay
        int show_watermark;         ///< show picture over output
        int log_thread_id;          ///< include thread id into the log output
        int log_call_duration;      ///< measure call duration
        int log_pq_delay;           ///< measure delay between queueing and displaying presentation
                                    ///< queue introduces
        int log_timestamp;          ///< display timestamps
        int avoid_va;               ///< do not use VA-API video decoding acceleration even if
                                    ///< available
    } quirks;
};

extern struct global_data global;

#endif /* VA_GL_SRC_GLOBALS_H */
