/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#define _XOPEN_SOURCE   500
#define _GNU_SOURCE
#include <ctype.h>
#include <vdpau/vdpau.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handle-storage.h"
#include "vdpau-soft.h"
#include "vdpau-locking.h"
#include "vdpau-trace.h"
#include "globals.h"

#include <sys/syscall.h>
#include <unistd.h>

void
trc_hk(void *longterm_param, void *shortterm_param, int origin, int after)
{
    (void)longterm_param;
    (void)origin;
    int before = !after;

    if (global.quirks.log_call_duration) {
        static __thread struct timespec start_ts = {0, 0};
        if (before) {
            clock_gettime(CLOCK_MONOTONIC, &start_ts);
        }

        if (after) {
            struct timespec end_ts;
            clock_gettime(CLOCK_MONOTONIC, &end_ts);
            double diff = (end_ts.tv_sec - start_ts.tv_sec) +
                          (end_ts.tv_nsec - start_ts.tv_nsec) / 1.0e9;
            printf("Duration %7.5f secs, %s, %s\n",
                diff, reverse_func_id(origin), reverse_status((VdpStatus)shortterm_param));
        }
    }

    if (before && global.quirks.log_thread_id) {
        printf("[%5d] ", (pid_t)syscall(__NR_gettid));
    }
}

static
void
initialize_quirks(void)
{
    global.quirks.buggy_XCloseDisplay = 0;
    global.quirks.show_watermark = 0;
    global.quirks.log_thread_id = 0;
    global.quirks.log_call_duration = 0;
    global.quirks.avoid_va = 0;

    const char *value = getenv("VDPAU_QUIRKS");
    if (!value)
        return;

    char *value_lc = strdup(value);
    if (NULL == value_lc)
        return;

    for (int k = 0; value_lc[k] != 0; k ++)
        value_lc[k] = tolower(value_lc[k]);

    // tokenize string
    const char delimiter = ',';
    char *item_start = value_lc;
    char *ptr = item_start;
    while (1) {
        int last = (0 == *ptr);
        if (delimiter == *ptr || 0 == *ptr) {
            *ptr = 0;

            if (!strcmp("xclosedisplay", item_start)) {
                global.quirks.buggy_XCloseDisplay = 1;
            } else
            if (!strcmp("showwatermark", item_start)) {
                global.quirks.show_watermark = 1;
            } else
            if (!strcmp("logthreadid", item_start)) {
                global.quirks.log_thread_id = 1;
            } else
            if (!strcmp("logcallduration", item_start)) {
                global.quirks.log_call_duration = 1;
            } else
            if (!strcmp("avoidva", item_start)) {
                global.quirks.avoid_va = 1;
            }

            item_start = ptr + 1;
        }
        ptr ++;
        if (last)
            break;
    }

    free(value_lc);
}

__attribute__((constructor))
static
void
library_constructor(void)
{
    handlestorage_initialize();

    // Initialize global data
    pthread_mutex_init(&global.mutex, NULL);
    pthread_mutex_init(&global.glx_ctx_stack_mutex, NULL);
    initialize_quirks();

    // initialize tracer
    traceSetTarget(stdout);
    traceSetHook(trc_hk, NULL);
    traceInfo("Software VDPAU backend library initialized\n");
#ifdef NDEBUG
    traceEnableTracing(0);
#else
    traceEnableTracing(1);
#endif
    const char *value = getenv("VDPAU_LOG");
    if (value) {
        // enable tracing when variable present
        traceEnableTracing(1);
        char *value_lc = strdup(value); // convert to lowercase
        for (int k = 0; value_lc[k] != 0; k ++) value_lc[k] = tolower(value_lc[k]);
        // and disable tracing when variable value equals one of the following values
        if (!strcmp(value_lc, "0") ||
            !strcmp(value_lc, "false") ||
            !strcmp(value_lc, "off") ||
            !strcmp(value_lc, "disable") ||
            !strcmp(value_lc, "disabled"))
        {
            traceEnableTracing(0);
        }
        free(value_lc);
    }
}

__attribute__((destructor))
static
void
library_destructor(void)
{
    handlestorage_destory();
}


__attribute__ ((visibility("default")))
VdpStatus
vdp_imp_device_create_x11(Display *display, int screen, VdpDevice *device,
                          VdpGetProcAddress **get_proc_address)
{
    // Select appropriate sub-backend. For now only software backend available.
    if (1) {
        return lockedVdpDeviceCreateX11(display, screen, device, get_proc_address);
    }
}
