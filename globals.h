/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#ifndef __GLOBALS_H
#define __GLOBALS_H

#include <pthread.h>

struct global_data {
    pthread_mutex_t     mutex;
    struct {
        int buggy_XCloseDisplay;
        int show_watermark;
    } quirks;
};

#endif /* __GLOBALS_H */
