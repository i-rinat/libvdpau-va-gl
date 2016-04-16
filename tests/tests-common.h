#pragma once

#undef NDEBUG
#include <assert.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

#define ASSERT_OK(expr) \
    do { \
        VdpStatus status = (expr); \
        assert(status == VDP_STATUS_OK); \
    } while (0)

#define MIN(a, b) ({ typeof(a) _a = a; typeof(b) _b = b; _a < _b ? _a : _b; })
#define MAX(a, b) ({ typeof(a) _a = a; typeof(b) _b = b; _a > _b ? _a : _b; })


Display *
get_dpy(void);

Window
get_wnd(void);

int
calc_difference_a8(uint8_t *src1, uint8_t *src2, int count);

int
calc_difference_r8g8b8a8(uint32_t *src1, uint32_t *src2, int count);

VdpDevice
create_vdp_device(void);

// API

extern VdpBitmapSurfaceCreate *
vdpBitmapSurfaceCreate;

extern VdpBitmapSurfaceDestroy *
vdpBitmapSurfaceDestroy;

extern VdpBitmapSurfaceGetParameters *
vdpBitmapSurfaceGetParameters;

extern VdpBitmapSurfacePutBitsNative *
vdpBitmapSurfacePutBitsNative;

extern VdpBitmapSurfaceQueryCapabilities *
vdpBitmapSurfaceQueryCapabilities;

extern VdpDecoderCreate *
vdpDecoderCreate;

extern VdpDecoderDestroy *
vdpDecoderDestroy;

extern VdpDecoderGetParameters *
vdpDecoderGetParameters;

extern VdpDecoderQueryCapabilities *
vdpDecoderQueryCapabilities;

extern VdpDecoderRender *
vdpDecoderRender;

extern VdpDeviceCreateX11 *
vdpDeviceCreateX11;

extern VdpDeviceDestroy *
vdpDeviceDestroy;

extern VdpGenerateCSCMatrix *
vdpGenerateCSCMatrix;

extern VdpGetApiVersion *
vdpGetApiVersion;

extern VdpGetErrorString *
vdpGetErrorString;

extern VdpGetInformationString *
vdpGetInformationString;

extern VdpOutputSurfaceCreate *
vdpOutputSurfaceCreate;

extern VdpOutputSurfaceDestroy *
vdpOutputSurfaceDestroy;

extern VdpOutputSurfaceGetBitsNative *
vdpOutputSurfaceGetBitsNative;

extern VdpOutputSurfaceGetParameters *
vdpOutputSurfaceGetParameters;

extern VdpOutputSurfacePutBitsIndexed *
vdpOutputSurfacePutBitsIndexed;

extern VdpOutputSurfacePutBitsNative *
vdpOutputSurfacePutBitsNative;

extern VdpOutputSurfacePutBitsYCbCr *
vdpOutputSurfacePutBitsYCbCr;

extern VdpOutputSurfaceQueryCapabilities *
vdpOutputSurfaceQueryCapabilities;

extern VdpOutputSurfaceQueryGetPutBitsNativeCapabilities *
vdpOutputSurfaceQueryGetPutBitsNativeCapabilities;

extern VdpOutputSurfaceQueryPutBitsIndexedCapabilities *
vdpOutputSurfaceQueryPutBitsIndexedCapabilities;

extern VdpOutputSurfaceQueryPutBitsYCbCrCapabilities *
vdpOutputSurfaceQueryPutBitsYCbCrCapabilities;

extern VdpOutputSurfaceRenderBitmapSurface *
vdpOutputSurfaceRenderBitmapSurface;

extern VdpOutputSurfaceRenderOutputSurface *
vdpOutputSurfaceRenderOutputSurface;

extern VdpPreemptionCallbackRegister *
vdpPreemptionCallbackRegister;

extern VdpPresentationQueueBlockUntilSurfaceIdle *
vdpPresentationQueueBlockUntilSurfaceIdle;

extern VdpPresentationQueueCreate *
vdpPresentationQueueCreate;

extern VdpPresentationQueueDestroy *
vdpPresentationQueueDestroy;

extern VdpPresentationQueueDisplay *
vdpPresentationQueueDisplay;

extern VdpPresentationQueueGetBackgroundColor *
vdpPresentationQueueGetBackgroundColor;

extern VdpPresentationQueueGetTime *
vdpPresentationQueueGetTime;

extern VdpPresentationQueueQuerySurfaceStatus *
vdpPresentationQueueQuerySurfaceStatus;

extern VdpPresentationQueueSetBackgroundColor *
vdpPresentationQueueSetBackgroundColor;

extern VdpPresentationQueueTargetCreateX11 *
vdpPresentationQueueTargetCreateX11;

extern VdpPresentationQueueTargetDestroy *
vdpPresentationQueueTargetDestroy;

extern VdpVideoMixerCreate *
vdpVideoMixerCreate;

extern VdpVideoMixerDestroy *
vdpVideoMixerDestroy;

extern VdpVideoMixerGetAttributeValues *
vdpVideoMixerGetAttributeValues;

extern VdpVideoMixerGetFeatureEnables *
vdpVideoMixerGetFeatureEnables;

extern VdpVideoMixerGetFeatureSupport *
vdpVideoMixerGetFeatureSupport;

extern VdpVideoMixerGetParameterValues *
vdpVideoMixerGetParameterValues;

extern VdpVideoMixerQueryAttributeSupport *
vdpVideoMixerQueryAttributeSupport;

extern VdpVideoMixerQueryAttributeValueRange *
vdpVideoMixerQueryAttributeValueRange;

extern VdpVideoMixerQueryFeatureSupport *
vdpVideoMixerQueryFeatureSupport;

extern VdpVideoMixerQueryParameterSupport *
vdpVideoMixerQueryParameterSupport;

extern VdpVideoMixerQueryParameterValueRange *
vdpVideoMixerQueryParameterValueRange;

extern VdpVideoMixerRender *
vdpVideoMixerRender;

extern VdpVideoMixerSetAttributeValues *
vdpVideoMixerSetAttributeValues;

extern VdpVideoMixerSetFeatureEnables *
vdpVideoMixerSetFeatureEnables;

extern VdpVideoSurfaceCreate *
vdpVideoSurfaceCreate;

extern VdpVideoSurfaceDestroy *
vdpVideoSurfaceDestroy;

extern VdpVideoSurfaceGetBitsYCbCr *
vdpVideoSurfaceGetBitsYCbCr;

extern VdpVideoSurfaceGetParameters *
vdpVideoSurfaceGetParameters;

extern VdpVideoSurfacePutBitsYCbCr *
vdpVideoSurfacePutBitsYCbCr;

extern VdpVideoSurfaceQueryCapabilities *
vdpVideoSurfaceQueryCapabilities;

extern VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *
vdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
