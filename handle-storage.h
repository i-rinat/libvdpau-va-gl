/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#pragma once
#ifndef HANDLE_STORAGE_H_
#define HANDLE_STORAGE_H_

#include <vdpau/vdpau_x11.h>
#include <glib.h>
#include <X11/extensions/XShm.h>

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

/** @brief Generic handle struct.

    Every other handle struct has same members at same place so it's possible
    to use type casting to determine handle type and parent.
*/
typedef struct {
    HandleType type;        ///< handle type
    void      *parent;      ///< link to parent
} VdpGenericHandle;

void handlestorage_initialize(void);
int handlestorage_add(void *data);
int handlestorage_valid(int handle, HandleType type);
void * handlestorage_get(int handle, HandleType type);
void handlestorage_expunge(int handle);
void handlestorage_destory(void);
void handlestorage_execute_for_all(void (*callback)(int idx, void *entry, void *p), void *param);
void *handlestorage_xdpy_copy_ref(void *dpy_orig);
void handlestorage_xdpy_copy_unref(void *dpy_orig);

#endif /* HANDLE_STORAGE_H_ */
