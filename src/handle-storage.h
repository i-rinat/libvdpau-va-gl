/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
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
