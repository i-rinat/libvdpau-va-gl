/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#define _XOPEN_SOURCE   500
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

extern struct global_data global;

static
void
initialize_quirks(void)
{
    global.quirks.buggy_XCloseDisplay = 0;

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
    initialize_quirks();

    // initialize tracer
    traceSetTarget(stdout);
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
