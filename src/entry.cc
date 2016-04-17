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

#define _XOPEN_SOURCE   500
#include "api.hh"
#include "compat-vdpau.hh"
#include "compat.hh"
#include "globals.hh"
#include "handle-storage.hh"
#include "trace.hh"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


static
void
initialize_quirks()
{
    global.quirks.buggy_XCloseDisplay = 0;
    global.quirks.show_watermark = 0;
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
void
va_gl_library_constructor()
{
    // Initialize global data
    initialize_quirks();
}

extern "C"
__attribute__ ((visibility("default")))
VdpStatus
vdp_imp_device_create_x11(Display *display, int screen, VdpDevice *device,
                          VdpGetProcAddress **get_proc_address)
{
    return vdp::Device::CreateX11(display, screen, device, get_proc_address);
}
