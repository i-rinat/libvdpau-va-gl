#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include "tests-common.h"

#ifndef DRIVER_NAME
#error no DRIVER_NAME
#endif

Display *
get_dpy(void)
{
    static Display *cached_dpy = NULL;

    if (cached_dpy)
        return cached_dpy;
    cached_dpy = XOpenDisplay(NULL);
    return cached_dpy;
}

Window
get_wnd(void)
{
    Display *dpy = get_dpy();
    Window root = XDefaultRootWindow(dpy);
    Window wnd = XCreateSimpleWindow(dpy, root, 0, 0, 300, 300, 0, 0, 0);
    XSync(dpy, False);
    return wnd;
}

static inline
int
max2(int a, int b)
{
    return (a > b) ? a : b;
}

int
calc_difference_a8(uint8_t *src1, uint8_t *src2, int count)
{
    int max_diff = 0;
    for (int k = 0; k < count; k ++)
        max_diff = max2(max_diff, abs(src1[k] - src2[k]));
    return max_diff;
}

int
calc_difference_r8g8b8a8(uint32_t *src1, uint32_t *src2, int count)
{
    int max_diff = 0;
    for (int k = 0; k < count; k ++) {
        const uint8_t r1 = (src1[k] >> 24) & 0xffu;
        const uint8_t g1 = (src1[k] >> 16) & 0xffu;
        const uint8_t b1 = (src1[k] >>  8) & 0xffu;
        const uint8_t a1 = (src1[k] >>  0) & 0xffu;
        const uint8_t r2 = (src2[k] >> 24) & 0xffu;
        const uint8_t g2 = (src2[k] >> 16) & 0xffu;
        const uint8_t b2 = (src2[k] >>  8) & 0xffu;
        const uint8_t a2 = (src2[k] >>  0) & 0xffu;

        const int tmp1 = max2(abs(r1 - r2), abs(g1 - g2));
        const int tmp2 = max2(abs(b1 - b2), abs(a1 - a2));
        max_diff = max2(max_diff, max2(tmp1, tmp2));
    }
    return max_diff;
}

VdpDevice
create_vdp_device(void)
{
    void *dl = dlopen(DRIVER_NAME, RTLD_NOW);
    if (!dl) {
        printf("failed to open %s\n", DRIVER_NAME);
        exit(1);
    }

    vdpDeviceCreateX11 = dlsym(dl, "vdp_imp_device_create_x11");
    if (!vdpDeviceCreateX11) {
        fprintf(stderr, "no vdp_imp_device_create_x11 in %s\n", DRIVER_NAME);
        exit(1);
    }

    VdpGetProcAddress *get_proc_address;
    VdpDevice          device;

    ASSERT_OK(vdpDeviceCreateX11(get_dpy(), 0, &device, &get_proc_address));

#define GET_FUNC(macroname, funcptr)                    \
    ASSERT_OK(get_proc_address(device, VDP_FUNC_ID_##macroname, (void **)&funcptr));


    GET_FUNC(GET_ERROR_STRING,       vdpGetErrorString);
    GET_FUNC(GET_API_VERSION,        vdpGetApiVersion);
    GET_FUNC(GET_INFORMATION_STRING, vdpGetInformationString);
    GET_FUNC(DEVICE_DESTROY,         vdpDeviceDestroy);
    GET_FUNC(GENERATE_CSC_MATRIX,    vdpGenerateCSCMatrix);
    GET_FUNC(VIDEO_SURFACE_QUERY_CAPABILITIES,  vdpVideoSurfaceQueryCapabilities);
    GET_FUNC(VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES,
             vdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities);
    GET_FUNC(VIDEO_SURFACE_CREATE,              vdpVideoSurfaceCreate);
    GET_FUNC(VIDEO_SURFACE_DESTROY,             vdpVideoSurfaceDestroy);
    GET_FUNC(VIDEO_SURFACE_GET_PARAMETERS,      vdpVideoSurfaceGetParameters);
    GET_FUNC(VIDEO_SURFACE_GET_BITS_Y_CB_CR,    vdpVideoSurfaceGetBitsYCbCr);
    GET_FUNC(VIDEO_SURFACE_PUT_BITS_Y_CB_CR,    vdpVideoSurfacePutBitsYCbCr);
    GET_FUNC(OUTPUT_SURFACE_QUERY_CAPABILITIES, vdpOutputSurfaceQueryCapabilities);

    GET_FUNC(OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES,
             vdpOutputSurfaceQueryGetPutBitsNativeCapabilities);
    GET_FUNC(OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES,
             vdpOutputSurfaceQueryPutBitsIndexedCapabilities);

    GET_FUNC(OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES,
             vdpOutputSurfaceQueryPutBitsYCbCrCapabilities);
    GET_FUNC(OUTPUT_SURFACE_CREATE,             vdpOutputSurfaceCreate);
    GET_FUNC(OUTPUT_SURFACE_DESTROY,            vdpOutputSurfaceDestroy);
    GET_FUNC(OUTPUT_SURFACE_GET_PARAMETERS,     vdpOutputSurfaceGetParameters);
    GET_FUNC(OUTPUT_SURFACE_GET_BITS_NATIVE,    vdpOutputSurfaceGetBitsNative);
    GET_FUNC(OUTPUT_SURFACE_PUT_BITS_NATIVE,    vdpOutputSurfacePutBitsNative);
    GET_FUNC(OUTPUT_SURFACE_PUT_BITS_INDEXED,   vdpOutputSurfacePutBitsIndexed);
    GET_FUNC(OUTPUT_SURFACE_PUT_BITS_Y_CB_CR,   vdpOutputSurfacePutBitsYCbCr);
    GET_FUNC(BITMAP_SURFACE_QUERY_CAPABILITIES, vdpBitmapSurfaceQueryCapabilities);
    GET_FUNC(BITMAP_SURFACE_CREATE,             vdpBitmapSurfaceCreate);
    GET_FUNC(BITMAP_SURFACE_DESTROY,            vdpBitmapSurfaceDestroy);
    GET_FUNC(BITMAP_SURFACE_GET_PARAMETERS,     vdpBitmapSurfaceGetParameters);
    GET_FUNC(BITMAP_SURFACE_PUT_BITS_NATIVE,    vdpBitmapSurfacePutBitsNative);
    GET_FUNC(OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, vdpOutputSurfaceRenderOutputSurface);
    GET_FUNC(OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, vdpOutputSurfaceRenderBitmapSurface);
    GET_FUNC(DECODER_QUERY_CAPABILITIES, vdpDecoderQueryCapabilities);
    GET_FUNC(DECODER_CREATE,             vdpDecoderCreate);
    GET_FUNC(DECODER_DESTROY,            vdpDecoderDestroy);
    GET_FUNC(DECODER_GET_PARAMETERS,     vdpDecoderGetParameters);
    GET_FUNC(DECODER_RENDER,             vdpDecoderRender);
    GET_FUNC(VIDEO_MIXER_QUERY_FEATURE_SUPPORT,   vdpVideoMixerQueryFeatureSupport);
    GET_FUNC(VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, vdpVideoMixerQueryParameterSupport);
    GET_FUNC(VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, vdpVideoMixerQueryAttributeSupport);
    GET_FUNC(VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, vdpVideoMixerQueryParameterValueRange);
    GET_FUNC(VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, vdpVideoMixerQueryAttributeValueRange);
    GET_FUNC(VIDEO_MIXER_CREATE,               vdpVideoMixerCreate);
    GET_FUNC(VIDEO_MIXER_SET_FEATURE_ENABLES,  vdpVideoMixerSetFeatureEnables);
    GET_FUNC(VIDEO_MIXER_SET_ATTRIBUTE_VALUES, vdpVideoMixerSetAttributeValues);
    GET_FUNC(VIDEO_MIXER_GET_FEATURE_SUPPORT,  vdpVideoMixerGetFeatureSupport);
    GET_FUNC(VIDEO_MIXER_GET_FEATURE_ENABLES,  vdpVideoMixerGetFeatureEnables);
    GET_FUNC(VIDEO_MIXER_GET_PARAMETER_VALUES, vdpVideoMixerGetParameterValues);
    GET_FUNC(VIDEO_MIXER_GET_ATTRIBUTE_VALUES, vdpVideoMixerGetAttributeValues);
    GET_FUNC(VIDEO_MIXER_DESTROY, vdpVideoMixerDestroy);
    GET_FUNC(VIDEO_MIXER_RENDER,  vdpVideoMixerRender);
    GET_FUNC(PRESENTATION_QUEUE_TARGET_DESTROY, vdpPresentationQueueTargetDestroy);
    GET_FUNC(PRESENTATION_QUEUE_CREATE,         vdpPresentationQueueCreate);
    GET_FUNC(PRESENTATION_QUEUE_DESTROY,        vdpPresentationQueueDestroy);
    GET_FUNC(PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, vdpPresentationQueueSetBackgroundColor);
    GET_FUNC(PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, vdpPresentationQueueGetBackgroundColor);
    GET_FUNC(PRESENTATION_QUEUE_GET_TIME, vdpPresentationQueueGetTime);
    GET_FUNC(PRESENTATION_QUEUE_DISPLAY,  vdpPresentationQueueDisplay);
    GET_FUNC(PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE,
             vdpPresentationQueueBlockUntilSurfaceIdle);
    GET_FUNC(PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, vdpPresentationQueueQuerySurfaceStatus);
    GET_FUNC(PREEMPTION_CALLBACK_REGISTER,            vdpPreemptionCallbackRegister);
    GET_FUNC(PRESENTATION_QUEUE_TARGET_CREATE_X11,    vdpPresentationQueueTargetCreateX11);

    return device;
}


VdpBitmapSurfaceCreate *
vdpBitmapSurfaceCreate;

VdpBitmapSurfaceDestroy *
vdpBitmapSurfaceDestroy;

VdpBitmapSurfaceGetParameters *
vdpBitmapSurfaceGetParameters;

VdpBitmapSurfacePutBitsNative *
vdpBitmapSurfacePutBitsNative;

VdpBitmapSurfaceQueryCapabilities *
vdpBitmapSurfaceQueryCapabilities;

VdpDecoderCreate *
vdpDecoderCreate;

VdpDecoderDestroy *
vdpDecoderDestroy;

VdpDecoderGetParameters *
vdpDecoderGetParameters;

VdpDecoderQueryCapabilities *
vdpDecoderQueryCapabilities;

VdpDecoderRender *
vdpDecoderRender;

VdpDeviceCreateX11 *
vdpDeviceCreateX11;

VdpDeviceDestroy *
vdpDeviceDestroy;

VdpGenerateCSCMatrix *
vdpGenerateCSCMatrix;

VdpGetApiVersion *
vdpGetApiVersion;

VdpGetErrorString *
vdpGetErrorString;

VdpGetInformationString *
vdpGetInformationString;

VdpOutputSurfaceCreate *
vdpOutputSurfaceCreate;

VdpOutputSurfaceDestroy *
vdpOutputSurfaceDestroy;

VdpOutputSurfaceGetBitsNative *
vdpOutputSurfaceGetBitsNative;

VdpOutputSurfaceGetParameters *
vdpOutputSurfaceGetParameters;

VdpOutputSurfacePutBitsIndexed *
vdpOutputSurfacePutBitsIndexed;

VdpOutputSurfacePutBitsNative *
vdpOutputSurfacePutBitsNative;

VdpOutputSurfacePutBitsYCbCr *
vdpOutputSurfacePutBitsYCbCr;

VdpOutputSurfaceQueryCapabilities *
vdpOutputSurfaceQueryCapabilities;

VdpOutputSurfaceQueryGetPutBitsNativeCapabilities *
vdpOutputSurfaceQueryGetPutBitsNativeCapabilities;

VdpOutputSurfaceQueryPutBitsIndexedCapabilities *
vdpOutputSurfaceQueryPutBitsIndexedCapabilities;

VdpOutputSurfaceQueryPutBitsYCbCrCapabilities *
vdpOutputSurfaceQueryPutBitsYCbCrCapabilities;

VdpOutputSurfaceRenderBitmapSurface *
vdpOutputSurfaceRenderBitmapSurface;

VdpOutputSurfaceRenderOutputSurface *
vdpOutputSurfaceRenderOutputSurface;

VdpPreemptionCallbackRegister *
vdpPreemptionCallbackRegister;

VdpPresentationQueueBlockUntilSurfaceIdle *
vdpPresentationQueueBlockUntilSurfaceIdle;

VdpPresentationQueueCreate *
vdpPresentationQueueCreate;

VdpPresentationQueueDestroy *
vdpPresentationQueueDestroy;

VdpPresentationQueueDisplay *
vdpPresentationQueueDisplay;

VdpPresentationQueueGetBackgroundColor *
vdpPresentationQueueGetBackgroundColor;

VdpPresentationQueueGetTime *
vdpPresentationQueueGetTime;

VdpPresentationQueueQuerySurfaceStatus *
vdpPresentationQueueQuerySurfaceStatus;

VdpPresentationQueueSetBackgroundColor *
vdpPresentationQueueSetBackgroundColor;

VdpPresentationQueueTargetCreateX11 *
vdpPresentationQueueTargetCreateX11;

VdpPresentationQueueTargetDestroy *
vdpPresentationQueueTargetDestroy;

VdpVideoMixerCreate *
vdpVideoMixerCreate;

VdpVideoMixerDestroy *
vdpVideoMixerDestroy;

VdpVideoMixerGetAttributeValues *
vdpVideoMixerGetAttributeValues;

VdpVideoMixerGetFeatureEnables *
vdpVideoMixerGetFeatureEnables;

VdpVideoMixerGetFeatureSupport *
vdpVideoMixerGetFeatureSupport;

VdpVideoMixerGetParameterValues *
vdpVideoMixerGetParameterValues;

VdpVideoMixerQueryAttributeSupport *
vdpVideoMixerQueryAttributeSupport;

VdpVideoMixerQueryAttributeValueRange *
vdpVideoMixerQueryAttributeValueRange;

VdpVideoMixerQueryFeatureSupport *
vdpVideoMixerQueryFeatureSupport;

VdpVideoMixerQueryParameterSupport *
vdpVideoMixerQueryParameterSupport;

VdpVideoMixerQueryParameterValueRange *
vdpVideoMixerQueryParameterValueRange;

VdpVideoMixerRender *
vdpVideoMixerRender;

VdpVideoMixerSetAttributeValues *
vdpVideoMixerSetAttributeValues;

VdpVideoMixerSetFeatureEnables *
vdpVideoMixerSetFeatureEnables;

VdpVideoSurfaceCreate *
vdpVideoSurfaceCreate;

VdpVideoSurfaceDestroy *
vdpVideoSurfaceDestroy;

VdpVideoSurfaceGetBitsYCbCr *
vdpVideoSurfaceGetBitsYCbCr;

VdpVideoSurfaceGetParameters *
vdpVideoSurfaceGetParameters;

VdpVideoSurfacePutBitsYCbCr *
vdpVideoSurfacePutBitsYCbCr;

VdpVideoSurfaceQueryCapabilities *
vdpVideoSurfaceQueryCapabilities;

VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *
vdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
