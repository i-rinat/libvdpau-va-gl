/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef VDPAU_SOFT_H_
#define VDPAU_SOFT_H_

#include <GL/glx.h>
#include <pthread.h>
#include <vdpau/vdpau.h>
#include <va/va.h>
#include "handle-storage.h"

#define MAX_RENDER_TARGETS          21
#define NUM_RENDER_TARGETS_H264     21

#define PRESENTATION_QUEUE_LENGTH   10

/** @brief VdpDevice object parameters */
typedef struct {
    HandleType      type;           ///< common type field
    void           *self;           ///< link to device. For VdpDeviceData this is link to itself
    pthread_mutex_t lock;
    int             refcount;
    Display        *display;        ///< own X display connection
    Display        *display_orig;   ///< supplied X display connection
    int             screen;         ///< X screen
    GLXContext      root_glc;       ///< master GL context
    Window          root;           ///< X drawable (root window) used for offscreen drawing
    VADisplay       va_dpy;         ///< VA display
    int             va_available;   ///< 1 if VA-API available
    int             va_version_major;
    int             va_version_minor;
    GLuint          watermark_tex_id;   ///< GL texture id for watermark
} VdpDeviceData;

/** @brief VdpVideoMixer object parameters */
typedef struct {
    HandleType      type;       ///< handle type
    VdpDeviceData  *device;     ///< link to parent
    pthread_mutex_t lock;
} VdpVideoMixerData;

/** @brief VdpOutputSurface object parameters */
typedef struct {
    HandleType      type;               ///< handle type
    VdpDeviceData  *device;             ///< link to parent
    pthread_mutex_t lock;
    VdpRGBAFormat   rgba_format;        ///< RGBA format of data stored
    GLuint          tex_id;             ///< associated GL texture id
    GLuint          fbo_id;             ///< framebuffer object id
    uint32_t        width;
    uint32_t        height;
    GLuint          gl_internal_format; ///< GL texture format: internal format
    GLuint          gl_format;          ///< GL texture format: preferred external format
    GLuint          gl_type;            ///< GL texture format: pixel type
    unsigned int    bytes_per_pixel;    ///< number of bytes per pixel
    VdpTime         first_presentation_time;    ///< first displayed time in queue
    VdpPresentationQueueStatus  status; ///< status in presentation queue
    VdpTime         queued_at;
} VdpOutputSurfaceData;

/** @brief VdpPresentationQueueTarget object parameters */
typedef struct {
    HandleType      type;           ///< handle type
    VdpDeviceData  *device;         ///< link to parent
    pthread_mutex_t lock;
    int             refcount;
    Drawable        drawable;       ///< X drawable to output to
    GLXContext      glc;            ///< GL context used for output
} VdpPresentationQueueTargetData;

/** @brief VdpPresentationQueue object parameters */
typedef struct {
    HandleType                      type;       ///< handle type
    VdpDeviceData                  *device;     ///< link to parent
    pthread_mutex_t                 lock;
    VdpPresentationQueueTargetData *target;
    VdpColor                        bg_color;   ///< background color

    struct {
        int head;
        int used;
        int firstfree;
        int freelist[PRESENTATION_QUEUE_LENGTH];
        struct {
            VdpTime             t;      ///< earliest_presentation_time
            int                 next;
            uint32_t            clip_width;
            uint32_t            clip_height;
            VdpOutputSurface    surface;
        } item[PRESENTATION_QUEUE_LENGTH];
    } queue;

    pthread_t           worker_thread;
    pthread_mutex_t     queue_mutex;
    pthread_cond_t      new_work_available;
} VdpPresentationQueueData;

/** @brief VdpVideoSurface object parameters */
typedef struct {
    HandleType      type;           ///< handle type
    VdpDeviceData  *device;         ///< link to parent
    pthread_mutex_t lock;
    VdpChromaType   chroma_type;    ///< video chroma type
    uint32_t        width;
    uint32_t        stride;         ///< distance between first pixels of two consecutive rows (in pixels)
    uint32_t        height;
    void           *y_plane;        ///< luma data (software)
    void           *v_plane;        ///< chroma data (software)
    void           *u_plane;        ///< chroma data (software)
    VASurfaceID     va_surf;        ///< VA-API surface
    void           *va_glx;         ///< handle for VA-API/GLX interaction
    GLuint          tex_id;         ///< GL texture id (RGBA)
} VdpVideoSurfaceData;

/** @brief VdpBitmapSurface object parameters */
typedef struct {
    HandleType      type;               ///< handle type
    VdpDeviceData  *device;             ///< link to parent
    pthread_mutex_t lock;
    VdpRGBAFormat   rgba_format;        ///< RGBA format of data stored
    GLuint          tex_id;             ///< GL texture id
    uint32_t        width;
    uint32_t        height;
    VdpBool         frequently_accessed;///< 1 if surface should be optimized for frequent access
    unsigned int    bytes_per_pixel;    ///< number of bytes per bitmap pixel
    GLuint          gl_internal_format; ///< GL texture format: internal format
    GLuint          gl_format;          ///< GL texture format: preferred external format
    GLuint          gl_type;            ///< GL texture format: pixel type
    char           *bitmap_data;        ///< system-memory buffer for frequently accessed bitmaps
    int             dirty;              ///< dirty flag. True if system-memory buffer contains data
                                        ///< newer than GPU texture contents
} VdpBitmapSurfaceData;

/** @brief VdpDecoder object parameters */
typedef struct {
    HandleType          type;           ///< handle type
    VdpDeviceData      *device;         ///< link to parent
    pthread_mutex_t     lock;
    VdpDecoderProfile   profile;        ///< decoder profile
    uint32_t            width;
    uint32_t            height;
    uint32_t            max_references; ///< maximum count of reference frames
    VAConfigID          config_id;      ///< VA-API config id
    VASurfaceID         render_targets[MAX_RENDER_TARGETS]; ///< spare VA surfaces
    uint32_t            num_render_targets;
    uint32_t            next_surface_idx;   ///< next free surface in render_targets
    VAContextID         context_id;     ///< VA-API context id
} VdpDecoderData;


VdpStatus
softVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address);

VdpGetApiVersion softVdpGetApiVersion;
VdpDecoderQueryCapabilities softVdpDecoderQueryCapabilities;
VdpDecoderCreate softVdpDecoderCreate;
VdpDecoderDestroy softVdpDecoderDestroy;
VdpDecoderGetParameters softVdpDecoderGetParameters;
VdpDecoderRender softVdpDecoderRender;
VdpOutputSurfaceQueryCapabilities softVdpOutputSurfaceQueryCapabilities;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities softVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities softVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities softVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
VdpOutputSurfaceCreate softVdpOutputSurfaceCreate;
VdpOutputSurfaceDestroy softVdpOutputSurfaceDestroy;
VdpOutputSurfaceGetParameters softVdpOutputSurfaceGetParameters;
VdpOutputSurfaceGetBitsNative softVdpOutputSurfaceGetBitsNative;
VdpOutputSurfacePutBitsNative softVdpOutputSurfacePutBitsNative;
VdpOutputSurfacePutBitsIndexed softVdpOutputSurfacePutBitsIndexed;
VdpOutputSurfacePutBitsYCbCr softVdpOutputSurfacePutBitsYCbCr;
VdpVideoMixerQueryFeatureSupport softVdpVideoMixerQueryFeatureSupport;
VdpVideoMixerQueryParameterSupport softVdpVideoMixerQueryParameterSupport;
VdpVideoMixerQueryAttributeSupport softVdpVideoMixerQueryAttributeSupport;
VdpVideoMixerQueryParameterValueRange softVdpVideoMixerQueryParameterValueRange;
VdpVideoMixerQueryAttributeValueRange softVdpVideoMixerQueryAttributeValueRange;
VdpVideoMixerCreate softVdpVideoMixerCreate;
VdpVideoMixerSetFeatureEnables softVdpVideoMixerSetFeatureEnables;
VdpVideoMixerSetAttributeValues softVdpVideoMixerSetAttributeValues;
VdpVideoMixerGetFeatureSupport softVdpVideoMixerGetFeatureSupport;
VdpVideoMixerGetFeatureEnables softVdpVideoMixerGetFeatureEnables;
VdpVideoMixerGetParameterValues softVdpVideoMixerGetParameterValues;
VdpVideoMixerGetAttributeValues softVdpVideoMixerGetAttributeValues;
VdpVideoMixerDestroy softVdpVideoMixerDestroy;
VdpVideoMixerRender softVdpVideoMixerRender;
VdpPresentationQueueTargetDestroy softVdpPresentationQueueTargetDestroy;
VdpPresentationQueueCreate softVdpPresentationQueueCreate;
VdpPresentationQueueDestroy softVdpPresentationQueueDestroy;
VdpPresentationQueueSetBackgroundColor softVdpPresentationQueueSetBackgroundColor;
VdpPresentationQueueGetBackgroundColor softVdpPresentationQueueGetBackgroundColor;
VdpPresentationQueueGetTime softVdpPresentationQueueGetTime;
VdpPresentationQueueDisplay softVdpPresentationQueueDisplay;
VdpPresentationQueueBlockUntilSurfaceIdle softVdpPresentationQueueBlockUntilSurfaceIdle;
VdpPresentationQueueQuerySurfaceStatus softVdpPresentationQueueQuerySurfaceStatus;
VdpVideoSurfaceQueryCapabilities softVdpVideoSurfaceQueryCapabilities;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities softVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
VdpVideoSurfaceCreate softVdpVideoSurfaceCreate;
VdpVideoSurfaceDestroy softVdpVideoSurfaceDestroy;
VdpVideoSurfaceGetParameters softVdpVideoSurfaceGetParameters;
VdpVideoSurfaceGetBitsYCbCr softVdpVideoSurfaceGetBitsYCbCr;
VdpVideoSurfacePutBitsYCbCr softVdpVideoSurfacePutBitsYCbCr;
VdpBitmapSurfaceQueryCapabilities softVdpBitmapSurfaceQueryCapabilities;
VdpBitmapSurfaceCreate softVdpBitmapSurfaceCreate;
VdpBitmapSurfaceDestroy softVdpBitmapSurfaceDestroy;
VdpBitmapSurfaceGetParameters softVdpBitmapSurfaceGetParameters;
VdpBitmapSurfacePutBitsNative softVdpBitmapSurfacePutBitsNative;
VdpDeviceDestroy softVdpDeviceDestroy;
VdpGetInformationString softVdpGetInformationString;
VdpGenerateCSCMatrix softVdpGenerateCSCMatrix;
VdpOutputSurfaceRenderOutputSurface softVdpOutputSurfaceRenderOutputSurface;
VdpOutputSurfaceRenderBitmapSurface softVdpOutputSurfaceRenderBitmapSurface;
VdpPreemptionCallbackRegister softVdpPreemptionCallbackRegister;
VdpPresentationQueueTargetCreateX11 softVdpPresentationQueueTargetCreateX11;
VdpGetProcAddress softVdpGetProcAddress;

#endif /* VDPAU_SOFT_H_ */
