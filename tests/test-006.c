// test-006
//
// initializing/finalizing two times a row
// with some drawing between

#include "vdpau-init.h"
#include <stdlib.h>

int main(void)
{
    VdpDevice device;
    Window window;
    VdpPresentationQueueTarget pq_target;
    VdpPresentationQueue pq;
    VdpOutputSurface out_surface;
    VdpOutputSurface out_surface_2;
    VdpBitmapSurface bmp_surface;
    ASSERT_OK(vdpau_init_functions(&device, &window, 1));
    ASSERT_OK(vdp_presentation_queue_target_create_x11(device, window, &pq_target));
    ASSERT_OK(vdp_presentation_queue_create(device, pq_target, &pq));
    ASSERT_OK(vdp_output_surface_create(device, VDP_RGBA_FORMAT_B8G8R8A8, 300, 150, &out_surface));
    ASSERT_OK(vdp_output_surface_create(device, VDP_RGBA_FORMAT_B8G8R8A8, 300, 150, &out_surface_2));
    ASSERT_OK(vdp_bitmap_surface_create(device, VDP_RGBA_FORMAT_B8G8R8A8, 300, 150, 1, &bmp_surface));

    uint32_t buf[300*150];
    const void * const source_data[] = { buf };
    uint32_t source_pitches[] = { 4 * 300 };
    for (int k = 0; k < 300*150; k ++) { buf[k] = 0xff000000 + (k) & 0xffffff; }
    ASSERT_OK(vdp_bitmap_surface_put_bits_native(bmp_surface, source_data, source_pitches, NULL));
    VdpTime vdpTime = 0;
    ASSERT_OK(vdp_presentation_queue_block_until_surface_idle(pq, out_surface, &vdpTime));
    ASSERT_OK(vdp_presentation_queue_get_time(pq, &vdpTime));

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
    ASSERT_OK(vdp_output_surface_render_bitmap_surface(out_surface, &destination_rect, bmp_surface,
                &source_rect, NULL, &blend_state, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));

    ASSERT_OK(vdp_presentation_queue_display(pq, out_surface, 0, 0, 0));
    ASSERT_OK(vdp_output_surface_destroy(out_surface));
    ASSERT_OK(vdp_output_surface_destroy(out_surface_2));
    ASSERT_OK(vdp_presentation_queue_destroy(pq));
    ASSERT_OK(vdp_presentation_queue_target_destroy(pq_target));
    ASSERT_OK(vdp_bitmap_surface_destroy(bmp_surface));
    ASSERT_OK(vdp_device_destroy(device));

    ASSERT_OK(vdpau_init_functions(&device, &window, 1));
    ASSERT_OK(vdp_presentation_queue_target_create_x11(device, window, &pq_target));
    ASSERT_OK(vdp_presentation_queue_create(device, pq_target, &pq));
    ASSERT_OK(vdp_device_destroy(device));

    return 0;
}
