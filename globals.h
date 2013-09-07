/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef __GLOBALS_H
#define __GLOBALS_H

#include <pthread.h>

/** @brief place where all shared global variables live */
struct global_data {
    pthread_mutex_t     mutex;
    pthread_mutex_t     glx_ctx_stack_mutex;    ///< mutex for GLX context management functions

    /** @brief tunables */
    struct {
        int buggy_XCloseDisplay;    ///< avoid calling XCloseDisplay
        int show_watermark;         ///< show picture over output
        int log_thread_id;          ///< include thread id into the log output
        int log_call_duration;      ///< measure call duration
        int log_pq_delay;           ///< measure delay between queueing and displaying presentation queue
                                    ///< introduces
        int avoid_va;               ///< do not use VA-API video decoding acceleration even if available
    } quirks;
};

extern struct global_data global;

#endif /* __GLOBALS_H */
