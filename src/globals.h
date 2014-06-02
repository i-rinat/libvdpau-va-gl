/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
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
