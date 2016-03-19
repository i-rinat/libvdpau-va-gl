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

#ifndef VA_GL_SRC_TRACE_H
#define VA_GL_SRC_TRACE_H

#include <stdio.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>
#include "reverse-constant.h"

void
traceEnableTracing(int flag);

void
traceSetTarget(FILE *target);

void
traceResetTarget(void);

void
traceSetHeader(const char *header, const char *header_blank);

void
traceInfo(const char *buf, ...);

void
traceError(const char *buf, ...);

void
traceSetHook(void (*hook)(void *param1, void *param2, int origin, int after), void *param);

void
traceCallHook(int origin, int after, void *shortterm_param);

VdpStatus
traceVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                         VdpGetProcAddress **get_proc_address);

VdpGetApiVersion traceVdpGetApiVersion;
VdpDecoderQueryCapabilities traceVdpDecoderQueryCapabilities;
VdpDecoderCreate traceVdpDecoderCreate;
VdpDecoderDestroy traceVdpDecoderDestroy;
VdpDecoderGetParameters traceVdpDecoderGetParameters;
VdpDecoderRender traceVdpDecoderRender;
VdpOutputSurfaceQueryCapabilities traceVdpOutputSurfaceQueryCapabilities;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
VdpOutputSurfaceCreate traceVdpOutputSurfaceCreate;
VdpOutputSurfaceDestroy traceVdpOutputSurfaceDestroy;
VdpOutputSurfaceGetParameters traceVdpOutputSurfaceGetParameters;
VdpOutputSurfaceGetBitsNative traceVdpOutputSurfaceGetBitsNative;
VdpOutputSurfacePutBitsNative traceVdpOutputSurfacePutBitsNative;
VdpOutputSurfacePutBitsIndexed traceVdpOutputSurfacePutBitsIndexed;
VdpOutputSurfacePutBitsYCbCr traceVdpOutputSurfacePutBitsYCbCr;
VdpVideoMixerQueryFeatureSupport traceVdpVideoMixerQueryFeatureSupport;
VdpVideoMixerQueryParameterSupport traceVdpVideoMixerQueryParameterSupport;
VdpVideoMixerQueryAttributeSupport traceVdpVideoMixerQueryAttributeSupport;
VdpVideoMixerQueryParameterValueRange traceVdpVideoMixerQueryParameterValueRange;
VdpVideoMixerQueryAttributeValueRange traceVdpVideoMixerQueryAttributeValueRange;
VdpVideoMixerCreate traceVdpVideoMixerCreate;
VdpVideoMixerSetFeatureEnables traceVdpVideoMixerSetFeatureEnables;
VdpVideoMixerSetAttributeValues traceVdpVideoMixerSetAttributeValues;
VdpVideoMixerGetFeatureSupport traceVdpVideoMixerGetFeatureSupport;
VdpVideoMixerGetFeatureEnables traceVdpVideoMixerGetFeatureEnables;
VdpVideoMixerGetParameterValues traceVdpVideoMixerGetParameterValues;
VdpVideoMixerGetAttributeValues traceVdpVideoMixerGetAttributeValues;
VdpVideoMixerDestroy traceVdpVideoMixerDestroy;
VdpVideoMixerRender traceVdpVideoMixerRender;
VdpPresentationQueueTargetDestroy traceVdpPresentationQueueTargetDestroy;
VdpPresentationQueueCreate traceVdpPresentationQueueCreate;
VdpPresentationQueueDestroy traceVdpPresentationQueueDestroy;
VdpPresentationQueueSetBackgroundColor traceVdpPresentationQueueSetBackgroundColor;
VdpPresentationQueueGetBackgroundColor traceVdpPresentationQueueGetBackgroundColor;
VdpPresentationQueueGetTime traceVdpPresentationQueueGetTime;
VdpPresentationQueueDisplay traceVdpPresentationQueueDisplay;
VdpPresentationQueueBlockUntilSurfaceIdle traceVdpPresentationQueueBlockUntilSurfaceIdle;
VdpPresentationQueueQuerySurfaceStatus traceVdpPresentationQueueQuerySurfaceStatus;
VdpVideoSurfaceQueryCapabilities traceVdpVideoSurfaceQueryCapabilities;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
VdpVideoSurfaceCreate traceVdpVideoSurfaceCreate;
VdpVideoSurfaceDestroy traceVdpVideoSurfaceDestroy;
VdpVideoSurfaceGetParameters traceVdpVideoSurfaceGetParameters;
VdpVideoSurfaceGetBitsYCbCr traceVdpVideoSurfaceGetBitsYCbCr;
VdpVideoSurfacePutBitsYCbCr traceVdpVideoSurfacePutBitsYCbCr;
VdpBitmapSurfaceQueryCapabilities traceVdpBitmapSurfaceQueryCapabilities;
VdpBitmapSurfaceCreate traceVdpBitmapSurfaceCreate;
VdpBitmapSurfaceDestroy traceVdpBitmapSurfaceDestroy;
VdpBitmapSurfaceGetParameters traceVdpBitmapSurfaceGetParameters;
VdpBitmapSurfacePutBitsNative traceVdpBitmapSurfacePutBitsNative;
VdpDeviceDestroy traceVdpDeviceDestroy;
VdpGetInformationString traceVdpGetInformationString;
VdpGenerateCSCMatrix traceVdpGenerateCSCMatrix;
VdpOutputSurfaceRenderOutputSurface traceVdpOutputSurfaceRenderOutputSurface;
VdpOutputSurfaceRenderBitmapSurface traceVdpOutputSurfaceRenderBitmapSurface;
VdpPreemptionCallbackRegister traceVdpPreemptionCallbackRegister;
VdpPresentationQueueTargetCreateX11 traceVdpPresentationQueueTargetCreateX11;
VdpGetProcAddress traceVdpGetProcAddress;

#endif /* VA_GL_SRC_TRACE_H */
