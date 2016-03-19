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

#ifndef VA_GL_SRC_HANDLE_STORAGE_H
#define VA_GL_SRC_HANDLE_STORAGE_H

#include <vdpau/vdpau_x11.h>
#include <glib.h>

typedef uint32_t    HandleType;
#define HANDLETYPE_ANY                         (HandleType)0
#define HANDLETYPE_DEVICE                      (HandleType)1
#define HANDLETYPE_PRESENTATION_QUEUE_TARGET   (HandleType)2
#define HANDLETYPE_PRESENTATION_QUEUE          (HandleType)3
#define HANDLETYPE_VIDEO_MIXER                 (HandleType)4
#define HANDLETYPE_OUTPUT_SURFACE              (HandleType)5
#define HANDLETYPE_VIDEO_SURFACE               (HandleType)6
#define HANDLETYPE_BITMAP_SURFACE              (HandleType)7
#define HANDLETYPE_DECODER                     (HandleType)8

void    handle_initialize_storage(void);
int     handle_insert(void *data);
void   *handle_acquire(int handle, HandleType type);
void    handle_release(int handle);
void    handle_expunge(int handle);
void    handle_destory_storage(void);
void    handle_execute_for_all(void (*callback)(int idx, void *entry, void *p), void *param);
void   *handle_xdpy_ref(void *dpy_orig);
void    handle_xdpy_unref(void *dpy_orig);

static inline
void
free_list_push(int32_t *free_list, int32_t *free_list_head, int32_t value)
{
    free_list[value] = *free_list_head;
    *free_list_head = value;
}

static inline
int32_t
free_list_pop(int32_t *free_list, int32_t *free_list_head)
{
    int32_t value = *free_list_head;
    if (value >= 0)
        *free_list_head = free_list[value];
    return value;
}

#endif /* VA_GL_SRC_HANDLE_STORAGE_H */
