// test-003
//
// Testing blending A8 bitmap surface into B8G8R8A8 output with following blend parameters:
// source/destination colors : src alpha / 1 - src alpha
// source/destination alpha  : one / src alpha
// blend equation for color / alpha : add / add
//
// coloring with color {0, 1, 0, 1}. This should be green with alpha == 1.

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "vdpau-init.h"


int main(void)
{

    VdpDevice device;
    VdpBitmapSurface bmp_surface;
    VdpOutputSurface out_surface;

    const uint8_t bmp_1[] = {
        0x00, 0x01, 0x02, 0x03,
        0x14, 0x15, 0x16, 0x17,
        0x28, 0x29, 0x2a, 0x2b,
        0x3c, 0x3d, 0x3e, 0x3f
    };
    const void * const source_data_bmp[] = { bmp_1 };
    uint32_t source_pitches_bmp[] = { 4 };

    const uint32_t black_4x4[] = {
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0xff000000, 0xff000000, 0xff000000, 0xff000000
    };
    const void * const source_data_black[] = { black_4x4 };
    uint32_t source_pitches_black[] = { 4 * 4 };

    ASSERT_OK(vdpau_init_functions(&device));
    // create surfaces
    ASSERT_OK(vdp_bitmap_surface_create(device, VDP_RGBA_FORMAT_A8, 4, 4, 1, &bmp_surface));
    ASSERT_OK(vdp_output_surface_create(device, VDP_RGBA_FORMAT_B8G8R8A8, 4, 4, &out_surface));
    // upload data
    ASSERT_OK(vdp_bitmap_surface_put_bits_native(bmp_surface, source_data_bmp, source_pitches_bmp, NULL));
    ASSERT_OK(vdp_output_surface_put_bits_native(out_surface, source_data_black, source_pitches_black, NULL));

    VdpOutputSurfaceRenderBlendState blend_state = {
        .blend_factor_source_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
        .blend_factor_destination_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .blend_factor_source_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_destination_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
        .blend_equation_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_equation_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_constant = {0, 0, 0, 0}
    };

    VdpColor color[] = {{0, 1.0, 0, 1.0}};
    ASSERT_OK(vdp_output_surface_render_bitmap_surface(out_surface, NULL, bmp_surface, NULL,
                color, &blend_state, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));

    const uint32_t expected_result[] = {
        0x00000000, 0x02000100, 0x04000200, 0x06000300,
        0x28001400, 0x2a001500, 0x2c001600, 0x2e001700,
        0x50002800, 0x52002900, 0x54002a00, 0x56002b00,
        0x78003c00, 0x7a003d00, 0x7c003e00, 0x7e003f00
    };

    uint32_t result[16];
    void * const dest_data[] = { result };
    ASSERT_OK(vdp_output_surface_get_bits_native(out_surface, NULL, dest_data, source_pitches_black));

    printf("=== expected ===\n");
    for (int k = 0; k < 16; k ++) {
        printf(" %08x", expected_result[k]);
        if (k % 4 == 3) printf("\n");
    }
    printf("--- actual ---\n");
    for (int k = 0; k < 16; k ++) {
        printf(" %08x", result[k]);
        if (k % 4 == 3) printf("\n");
    }
    printf("==========\n");

    if (memcmp(expected_result, result, sizeof(expected_result))) {
        printf("fail\n");
        return 1;
    }

    printf("pass\n");
    return 0;
}
