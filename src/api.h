/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#ifndef VA_GL_SRC_API_H
#define VA_GL_SRC_API_H

#include <GL/glx.h>
#include <pthread.h>
#include <vdpau/vdpau.h>
#include <va/va.h>
#include "handle-storage.h"

#define DESCRIBE(xparam, format)    fprintf(stderr, #xparam " = %" #format "\n", xparam)

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
    Pixmap          pixmap;
    GLXPixmap       glx_pixmap;
    GC              plain_copy_gc;  ///< X GC for displaying buffer content
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
    uint32_t        height;
    VASurfaceID     va_surf;        ///< VA-API surface
    void           *va_glx;         ///< handle for VA-API/GLX interaction
    int             sync_va_to_glx; ///< whenever VA-API surface should be converted to GL texture
    GLuint          tex_id;         ///< GL texture id (RGBA)
    GLuint          fbo_id;         ///< framebuffer object id
    VdpDecoder      decoder;        ///< associated VdpDecoder
    int32_t         rt_idx;         ///< index in VdpDecoder's render_targets
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
    int32_t             free_list_head;
    int32_t             free_list[MAX_RENDER_TARGETS];
    uint32_t            num_render_targets;
    VAContextID         context_id;     ///< VA-API context id
} VdpDecoderData;


VdpStatus
vdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address);

VdpGetApiVersion            vdpGetApiVersion;

VdpDecoderQueryCapabilities vdpDecoderQueryCapabilities;
VdpDecoderCreate            vdpDecoderCreate;
VdpDecoderDestroy           vdpDecoderDestroy;
VdpDecoderGetParameters     vdpDecoderGetParameters;
VdpDecoderRender            vdpDecoderRender;

VdpOutputSurfaceQueryCapabilities               vdpOutputSurfaceQueryCapabilities;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities vdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities vdpOutputSurfaceQueryPutBitsIndexedCapabilities;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities   vdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
VdpOutputSurfaceCreate          vdpOutputSurfaceCreate;
VdpOutputSurfaceDestroy         vdpOutputSurfaceDestroy;
VdpOutputSurfaceGetParameters   vdpOutputSurfaceGetParameters;
VdpOutputSurfaceGetBitsNative   vdpOutputSurfaceGetBitsNative;
VdpOutputSurfacePutBitsNative   vdpOutputSurfacePutBitsNative;
VdpOutputSurfacePutBitsIndexed  vdpOutputSurfacePutBitsIndexed;
VdpOutputSurfacePutBitsYCbCr    vdpOutputSurfacePutBitsYCbCr;

VdpVideoMixerQueryFeatureSupport        vdpVideoMixerQueryFeatureSupport;
VdpVideoMixerQueryParameterSupport      vdpVideoMixerQueryParameterSupport;
VdpVideoMixerQueryAttributeSupport      vdpVideoMixerQueryAttributeSupport;
VdpVideoMixerQueryParameterValueRange   vdpVideoMixerQueryParameterValueRange;
VdpVideoMixerQueryAttributeValueRange   vdpVideoMixerQueryAttributeValueRange;
VdpVideoMixerCreate                     vdpVideoMixerCreate;
VdpVideoMixerSetFeatureEnables          vdpVideoMixerSetFeatureEnables;
VdpVideoMixerSetAttributeValues         vdpVideoMixerSetAttributeValues;
VdpVideoMixerGetFeatureSupport          vdpVideoMixerGetFeatureSupport;
VdpVideoMixerGetFeatureEnables          vdpVideoMixerGetFeatureEnables;
VdpVideoMixerGetParameterValues         vdpVideoMixerGetParameterValues;
VdpVideoMixerGetAttributeValues         vdpVideoMixerGetAttributeValues;
VdpVideoMixerDestroy                    vdpVideoMixerDestroy;
VdpVideoMixerRender                     vdpVideoMixerRender;

VdpPresentationQueueTargetDestroy           vdpPresentationQueueTargetDestroy;
VdpPresentationQueueCreate                  vdpPresentationQueueCreate;
VdpPresentationQueueDestroy                 vdpPresentationQueueDestroy;
VdpPresentationQueueSetBackgroundColor      vdpPresentationQueueSetBackgroundColor;
VdpPresentationQueueGetBackgroundColor      vdpPresentationQueueGetBackgroundColor;
VdpPresentationQueueGetTime                 vdpPresentationQueueGetTime;
VdpPresentationQueueDisplay                 vdpPresentationQueueDisplay;
VdpPresentationQueueBlockUntilSurfaceIdle   vdpPresentationQueueBlockUntilSurfaceIdle;
VdpPresentationQueueQuerySurfaceStatus      vdpPresentationQueueQuerySurfaceStatus;

VdpVideoSurfaceQueryCapabilities                vdpVideoSurfaceQueryCapabilities;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities vdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
VdpVideoSurfaceCreate                           vdpVideoSurfaceCreate;
VdpVideoSurfaceDestroy                          vdpVideoSurfaceDestroy;
VdpVideoSurfaceGetParameters                    vdpVideoSurfaceGetParameters;
VdpVideoSurfaceGetBitsYCbCr                     vdpVideoSurfaceGetBitsYCbCr;
VdpVideoSurfacePutBitsYCbCr                     vdpVideoSurfacePutBitsYCbCr;

VdpBitmapSurfaceQueryCapabilities   vdpBitmapSurfaceQueryCapabilities;
VdpBitmapSurfaceCreate              vdpBitmapSurfaceCreate;
VdpBitmapSurfaceDestroy             vdpBitmapSurfaceDestroy;
VdpBitmapSurfaceGetParameters       vdpBitmapSurfaceGetParameters;
VdpBitmapSurfacePutBitsNative       vdpBitmapSurfacePutBitsNative;

VdpDeviceDestroy            vdpDeviceDestroy;
VdpGetInformationString     vdpGetInformationString;
VdpGenerateCSCMatrix        vdpGenerateCSCMatrix;

VdpOutputSurfaceRenderOutputSurface vdpOutputSurfaceRenderOutputSurface;
VdpOutputSurfaceRenderBitmapSurface vdpOutputSurfaceRenderBitmapSurface;

VdpPreemptionCallbackRegister       vdpPreemptionCallbackRegister;
VdpPresentationQueueTargetCreateX11 vdpPresentationQueueTargetCreateX11;
VdpGetProcAddress                   vdpGetProcAddress;

#endif /* VA_GL_SRC_API_H */