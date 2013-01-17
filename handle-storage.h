#pragma once
#ifndef HANDLE_STORAGE_H_
#define HANDLE_STORAGE_H_

#include <cairo.h>
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

typedef struct {
    HandleType type;
} VdpGenericHandle;

typedef struct {
    HandleType type;
    Display *display;
    int screen;
} VdpDeviceData;

typedef struct {
    HandleType type;
    int device;
    Drawable drawable;
} VdpPresentationQueueTargetData;

typedef struct {
    HandleType type;
    int device;
    int presentation_queue_target;
    XShmSegmentInfo shminfo;
    XImage *image;
    uint32_t prev_width;
    uint32_t prev_height;
} VdpPresentationQueueData;

typedef struct {
    HandleType type;
    VdpDevice device;
} VdpVideoMixerData;

typedef struct {
    HandleType type;
    VdpDevice device;
    VdpRGBAFormat rgba_format;
    cairo_surface_t *cairo_surface;
} VdpOutputSurfaceData;

typedef struct {
    HandleType type;
    VdpDevice device;
    VdpChromaType chroma_type;
    uint32_t width;
    uint32_t stride;
    uint32_t height;
    void *y_plane;
    void *v_plane;
    void *u_plane;
} VdpVideoSurfaceData;

typedef struct {
    HandleType type;
    VdpDevice device;
    VdpRGBAFormat rgba_format;
    cairo_surface_t *cairo_surface;
} VdpBitmapSurfaceData;

void handlestorage_initialize(void);
int handlestorage_add(void *data);
int handlestorage_valid(int handle, HandleType type);
void * handlestorage_get(int handle, HandleType type);
void handlestorage_expunge(int handle);
void handlestorage_destory(void);


#endif /* HANDLE_STORAGE_H_ */
