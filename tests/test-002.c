// test-002

// Test alignment issues of output surface {get,put}bits.
// Uploads 5x5 square of A8 samples, thus breaking 4-byte alignment. Then downloads and
// compares. Buffers should contain identical data.
//
// Bitmap surfaces checked too. But since there is no way to download data directly from
// bitmap surface, we doing this via rendering to output surface.

#include <stdio.h>
#include <string.h>
#include "vdpau-init.h"


int main(void)
{
    VdpDevice device;
    VdpOutputSurface out_surface;
    VdpBitmapSurface bmp_surface;
    uint8_t twenty_five[] = {
        0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14,
        0x15, 0x16, 0x17, 0x18, 0x19
    };
    uint8_t out_buf[25];
    const void * const source_data[] = { twenty_five };
    void * const destination_data[] = { out_buf };
    uint32_t source_pitches[] = { 5 };
    uint32_t destination_pitches[] = { 5 };

    ASSERT_OK(vdpau_init_functions(&device, NULL, 0));
    ASSERT_OK(vdp_output_surface_create(device, VDP_RGBA_FORMAT_A8, 5, 5, &out_surface));

    // upload image to surface, download image from surface
    ASSERT_OK(vdp_output_surface_put_bits_native(out_surface, source_data, source_pitches, NULL));
    ASSERT_OK(vdp_output_surface_get_bits_native(out_surface, NULL, destination_data, destination_pitches));

    printf("outputsurface\n");
    for (int k = 0; k < 25; k ++) {
        printf(" %02x", twenty_five[k]);
        if (k % 5 == 4) printf("\n");
    }
    printf("----------\n");
    for (int k = 0; k < 25; k ++) {
        printf(" %02x", out_buf[k]);
        if (k % 5 == 4) printf("\n");
    }
    printf("==========\n");

    if (memcmp(out_buf, twenty_five, 25)) {
        printf("failure\n");
        return 1;
    }

    // Do check bitmap surface
    ASSERT_OK(vdp_bitmap_surface_create(device, VDP_RGBA_FORMAT_A8, 5, 5, 1, &bmp_surface));
    ASSERT_OK(vdp_bitmap_surface_put_bits_native(bmp_surface, source_data, source_pitches, NULL));

    // draw alpha channel as color
    VdpOutputSurfaceRenderBlendState blend_state = {
        .struct_version = VDP_OUTPUT_SURFACE_RENDER_BLEND_STATE_VERSION,
        .blend_factor_source_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
        .blend_factor_source_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_destination_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
        .blend_factor_destination_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO,
        .blend_equation_color = VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_equation_alpha = VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_constant = {0, 0, 0, 0}
    };
    ASSERT_OK(vdp_output_surface_render_bitmap_surface(out_surface, NULL, bmp_surface, NULL, NULL,
                &blend_state, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));
    ASSERT_OK(vdp_output_surface_get_bits_native(out_surface, NULL, destination_data, destination_pitches));

    printf("bitmapsurface\n");
    for (int k = 0; k < 25; k ++) {
        printf(" %02x", twenty_five[k]);
        if (k % 5 == 4) printf("\n");
    }
    printf("----------\n");
    for (int k = 0; k < 25; k ++) {
        printf(" %02x", out_buf[k]);
        if (k % 5 == 4) printf("\n");
    }
    printf("==========\n");

    if (memcmp(out_buf, twenty_five, 25)) {
        printf("failure\n");
        return 2;
    }

    printf("pass\n");
    return 0;
}
