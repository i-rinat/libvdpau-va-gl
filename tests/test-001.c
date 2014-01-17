// Create two output surfaces (B8G8R8A8) of 4x4, fill first with opaque black
// and second with black and two red dots (opaque too).
// Render second into first. Check that red dots do not get smoothed.
// The dot at (1, 1) checks for smoothing, one at (3,3) checks for edge condition.

#include "tests-common.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>


int main(void)
{
    VdpDevice device;
    Display *dpy = get_dpy();

    ASSERT_OK(vdpDeviceCreateX11(dpy, 0, &device, NULL));

    VdpOutputSurface out_surface_1;
    VdpOutputSurface out_surface_2;

    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 4, 4, &out_surface_1));
    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 4, 4, &out_surface_2));

    uint32_t black_box[] = {
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000
    };

    uint32_t two_red_dots[] = {
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xffff0000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xffff0000
    };

    const void * const source_data_1[] = {black_box};
    const void * const source_data_2[] = {two_red_dots};
    uint32_t source_pitches[] = { 4 * 4 };

    // upload data
    ASSERT_OK(vdpOutputSurfacePutBitsNative(out_surface_1, source_data_1, source_pitches, NULL));
    ASSERT_OK(vdpOutputSurfacePutBitsNative(out_surface_2, source_data_2, source_pitches, NULL));

    // render
    VdpOutputSurfaceRenderBlendState blend_state = {
        .struct_version =                   VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION,
        .blend_factor_source_color =        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_source_alpha =        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_destination_color =   VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
        .blend_factor_destination_alpha =   VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
        .blend_equation_color =             VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_equation_alpha =             VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_constant =                   {0, 0, 0, 0}
    };

    ASSERT_OK(vdpOutputSurfaceRenderOutputSurface(out_surface_1, NULL, out_surface_2, NULL,
                NULL, &blend_state, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));

    // get data back
    uint32_t receive_buf[16];
    void * const dest_data[] = {receive_buf};
    ASSERT_OK(vdpOutputSurfaceGetBitsNative(out_surface_1, NULL, dest_data, source_pitches));

    printf("output surface\n");
    for (int k = 0; k < 16; k ++) {
        printf("%x ", receive_buf[k]);
        if (3 == k % 4) printf("\n");
    }
    printf("----------\n");
    for (int k = 0; k < 16; k ++) {
        printf("%x ", two_red_dots[k]);
        if (3 == k % 4) printf("\n");
    }

    // compare recieve_buf with two_red_dots
    if (memcmp(receive_buf, two_red_dots, 4*4*4)) {
        printf("fail\n");
        return 1;
    }

    // Check bitmap surface rendering smoothing issue
    VdpBitmapSurface bmp_surface;
    ASSERT_OK(vdpBitmapSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 4, 4, 1, &bmp_surface));
    ASSERT_OK(vdpBitmapSurfacePutBitsNative(bmp_surface, source_data_2, source_pitches, NULL));
    VdpOutputSurfaceRenderBlendState blend_state_opaque_copy = {
        .struct_version =                   VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION,
        .blend_factor_source_color =        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_source_alpha =        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_destination_color =   VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
        .blend_factor_destination_alpha =   VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
        .blend_equation_color =             VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_equation_alpha =             VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_constant =                   {0, 0, 0, 0}
    };
    ASSERT_OK(vdpOutputSurfaceRenderBitmapSurface(out_surface_1, NULL, bmp_surface, NULL,
                NULL, &blend_state_opaque_copy, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));
    ASSERT_OK(vdpOutputSurfaceGetBitsNative(out_surface_1, NULL, dest_data, source_pitches));

    printf("bitmap surface\n");
    for (int k = 0; k < 16; k ++) {
        printf("%x ", receive_buf[k]);
        if (3 == k % 4) printf("\n");
    }
    printf("----------\n");
    for (int k = 0; k < 16; k ++) {
        printf("%x ", two_red_dots[k]);
        if (3 == k % 4) printf("\n");
    }

    if (memcmp(receive_buf, two_red_dots, 4*4*4)) {
        printf("fail\n");
        return 2;
    }

    printf("pass\n");
    return 0;
}
