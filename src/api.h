/*
 * Copyright 2013-2014  Rinat Ibragimov
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
#include "shaders.h"

#define DESCRIBE(xparam, format)    fprintf(stderr, #xparam " = %" #format "\n", xparam)

#define MAX_RENDER_TARGETS          21
#define NUM_RENDER_TARGETS_H264     21

#define VDP_GENERIC_HANDLE_FIELDS                   \
    struct {                                        \
        HandleType      type;   /**< handle type */ \
        VdpDevice       device;                     \
        VdpDeviceData  *deviceData;                 \
        pthread_mutex_t lock;                       \
    }

typedef struct VdpDeviceData VdpDeviceData;

/** @brief Generic handle struct.

    Every other handle struct has same members at same place so it's possible
    to use type casting to determine handle type and parent.
*/
typedef struct {
    VDP_GENERIC_HANDLE_FIELDS;      ///< base struct
} VdpGenericData;

/** @brief VdpDevice object parameters */
struct VdpDeviceData {
    VDP_GENERIC_HANDLE_FIELDS;      ///< base struct
    int             refcount;
    pthread_mutex_t refcount_mutex;
    Display        *display;        ///< own X display connection
    Display        *display_orig;   ///< supplied X display connection
    int             screen;         ///< X screen
    int             color_depth;    ///< screen color depth
    GLXContext      root_glc;       ///< master GL context
    Window          root;           ///< X drawable (root window) used for offscreen drawing
    VADisplay       va_dpy;         ///< VA display
    int             va_available;   ///< 1 if VA-API available
    int             va_version_major;
    int             va_version_minor;
    GLuint          watermark_tex_id;   ///< GL texture id for watermark
    struct {
        GLuint      f_shader;
        GLuint      program;
        struct {
            int     tex_0;
            int     tex_1;
        } uniform;
    } shaders[SHADER_COUNT];
    struct {
        PFNGLXBINDTEXIMAGEEXTPROC       glXBindTexImageEXT;
        PFNGLXRELEASETEXIMAGEEXTPROC    glXReleaseTexImageEXT;
    } fn;
};

/** @brief VdpVideoMixer object parameters */
typedef struct {
    VDP_GENERIC_HANDLE_FIELDS;
    uint32_t        pixmap_width;       ///< last seen width
    uint32_t        pixmap_height;      ///< last seen height
    Pixmap          pixmap;             ///< target pixmap for vaPutSurface
    GLXPixmap       glx_pixmap;         ///< associated glx pixmap for texture-from-pixmap
    GLuint          tex_id;             ///< texture for texture-from-pixmap
} VdpVideoMixerData;

/** @brief VdpOutputSurface object parameters */
typedef struct {
    VDP_GENERIC_HANDLE_FIELDS;          ///< base struct
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
    VDP_GENERIC_HANDLE_FIELDS;      ///< base struct
    int             refcount;
    pthread_mutex_t refcount_mutex;
    Drawable        drawable;       ///< X drawable to output to
    unsigned int    drawable_width; ///< last seen drawable width
    unsigned int    drawable_height;///< last seen drawable height
    Pixmap          pixmap;         ///< draw buffer
    GLXPixmap       glx_pixmap;     ///< GLX pixmap proxy
    GC              plain_copy_gc;  ///< X GC for displaying buffer content
    GLXContext      glc;            ///< GL context used for output
    XVisualInfo    *xvi;
} VdpPresentationQueueTargetData;

/** @brief VdpPresentationQueue object parameters */
typedef struct {
    VDP_GENERIC_HANDLE_FIELDS;                  ///< base struct
    VdpPresentationQueueTargetData *targetData;
    VdpPresentationQueueTarget      target;
    VdpColor                        bg_color;   ///< background color
} VdpPresentationQueueData;

/** @brief VdpVideoSurface object parameters */
typedef struct {
    VDP_GENERIC_HANDLE_FIELDS;      ///< base struct
    VdpChromaType   chroma_type;    ///< video chroma type
    VdpYCbCrFormat  format;         ///< current data format
    uint32_t        width;
    uint32_t        height;
    uint32_t        stride;
    uint32_t        chroma_width;
    uint32_t        chroma_height;
    uint32_t        chroma_stride;
    VASurfaceID     va_surf;        ///< VA-API surface
    int             sync_va_to_glx; ///< whenever VA-API surface should be converted to GL texture
    GLuint          tex_id;         ///< GL texture id (RGBA)
    GLuint          fbo_id;         ///< framebuffer object id
    VdpDecoder      decoder;        ///< associated VdpDecoder
    int32_t         rt_idx;         ///< index in VdpDecoder's render_targets
    uint8_t        *y_plane;
    uint8_t        *u_plane;
    uint8_t        *v_plane;
} VdpVideoSurfaceData;

/** @brief VdpBitmapSurface object parameters */
typedef struct {
    VDP_GENERIC_HANDLE_FIELDS;          ///< base struct
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
    VDP_GENERIC_HANDLE_FIELDS;          ///< base struct
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


static inline
int
ref_device(VdpDeviceData *deviceData)
{
    pthread_mutex_lock(&deviceData->refcount_mutex);
    int retval = ++deviceData->refcount;
    pthread_mutex_unlock(&deviceData->refcount_mutex);
    return retval;
}

static inline
int
unref_device(VdpDeviceData *deviceData)
{
    pthread_mutex_lock(&deviceData->refcount_mutex);
    int retval = --deviceData->refcount;
    pthread_mutex_unlock(&deviceData->refcount_mutex);
    return retval;
}

static inline
int
ref_pq_target(VdpPresentationQueueTargetData *pqTargetData)
{
    pthread_mutex_lock(&pqTargetData->refcount_mutex);
    int retval = ++pqTargetData->refcount;
    pthread_mutex_unlock(&pqTargetData->refcount_mutex);
    return retval;
}

static inline
int
unref_pq_target(VdpPresentationQueueTargetData *pqTargetData)
{
    pthread_mutex_lock(&pqTargetData->refcount_mutex);
    int retval = --pqTargetData->refcount;
    pthread_mutex_unlock(&pqTargetData->refcount_mutex);
    return retval;
}

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
