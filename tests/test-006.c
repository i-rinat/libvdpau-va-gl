// test-006
//
// initializing/finalizing number of times a row with some drawing between.
// This test is to reveal thread-safety failure inside VDPAU library.
//
// Initialization function executed once, but then THREAD_COUNT threads try to do the same work
// with rendering simultaneously.

#include "tests-common.h"
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT    50


VdpDevice device;
Window window;

void *thread_1_func(void *p)
{
    (void)p;    // unused
    VdpPresentationQueueTarget pq_target;
    VdpPresentationQueue pq;
    VdpOutputSurface out_surface;
    VdpOutputSurface out_surface_2;
    VdpBitmapSurface bmp_surface;

    ASSERT_OK(vdpPresentationQueueTargetCreateX11(device, window, &pq_target));
    ASSERT_OK(vdpPresentationQueueCreate(device, pq_target, &pq));
    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 300, 150, &out_surface));
    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 300, 150, &out_surface_2));
    ASSERT_OK(vdpBitmapSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 300, 150, 1, &bmp_surface));

    uint32_t buf[300*150];
    const void * const source_data[] = { buf };
    uint32_t source_pitches[] = { 4 * 300 };
    for (int k = 0; k < 300*150; k ++) { buf[k] = 0xff000000 + (k & 0xffffff); }
    ASSERT_OK(vdpBitmapSurfacePutBitsNative(bmp_surface, source_data, source_pitches, NULL));
    VdpTime vdpTime = 0;
    ASSERT_OK(vdpPresentationQueueBlockUntilSurfaceIdle(pq, out_surface, &vdpTime));
    ASSERT_OK(vdpPresentationQueueGetTime(pq, &vdpTime));

    VdpOutputSurfaceRenderBlendState blend_state = {
       .blend_factor_source_color=VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
       .blend_factor_destination_color=VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
       .blend_factor_source_alpha=VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
       .blend_factor_destination_alpha=VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
       .blend_equation_color=VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
       .blend_equation_alpha=VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
       .blend_constant = { 0, 0, 0, 0}
    };
    VdpRect source_rect = {0, 0, 300, 150};
    VdpRect destination_rect = {0, 0, 300, 150};
    ASSERT_OK(vdpOutputSurfaceRenderBitmapSurface(out_surface, &destination_rect, bmp_surface,
                &source_rect, NULL, &blend_state, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));

    ASSERT_OK(vdpPresentationQueueDisplay(pq, out_surface, 0, 0, 0));

    ASSERT_OK(vdpOutputSurfaceDestroy(out_surface));
    ASSERT_OK(vdpOutputSurfaceDestroy(out_surface_2));
    ASSERT_OK(vdpPresentationQueueDestroy(pq));
    ASSERT_OK(vdpPresentationQueueTargetDestroy(pq_target));
    ASSERT_OK(vdpBitmapSurfaceDestroy(bmp_surface));

    return NULL;
}

int main(void)
{
    window = get_wnd();
    pthread_t pt[THREAD_COUNT];

    device = create_vdp_device();

    for (int k = 0; k < THREAD_COUNT; k ++)
        pthread_create(&pt[k], NULL, thread_1_func, NULL);

    for (int k = 0; k < THREAD_COUNT; k ++)
        pthread_join(pt[k], NULL);

    ASSERT_OK(vdpDeviceDestroy(device));

    return 0;
}
