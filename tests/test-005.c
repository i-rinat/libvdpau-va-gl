// test-005
//
// rendering A8 bitmap to non-black output surface.
// source/destination colors : src alpha / 1 - src alpha
// source/destination alpha  : one / src alpha
// blend equation for color / alpha : add / add
//
// target surface filled with {1, 0, 0, 1}
//
// coloring with color {0, 1, 0, 1}. This should be green with alpha == 1.

#include "tests-common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(void)
{
    Display *dpy = get_dpy();
    VdpDevice device;
    ASSERT_OK(vdpDeviceCreateX11(dpy, 0, &device, NULL));

    VdpBitmapSurface bmp_surface;
    VdpOutputSurface out_surface;
    ASSERT_OK(vdpBitmapSurfaceCreate(device, VDP_RGBA_FORMAT_A8, 5, 5, 1, &bmp_surface));
    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 7, 7, &out_surface));

    const uint8_t bmp_data[5 * 5] = {
    /*   1     2     3     4     5  */
/* 1 */ 0x00, 0x1e, 0x1f, 0x20, 0x21,
/* 2 */ 0x01, 0x3e, 0x3f, 0x40, 0x41,
/* 3 */ 0x02, 0x5e, 0x5f, 0x60, 0x61,
/* 4 */ 0x03, 0x8e, 0x7f, 0xff, 0xff,
/* 5 */ 0x04, 0xce, 0x7f, 0xff, 0xff
    };
    const void * const source_data_bmp[] = { bmp_data };
    uint32_t source_pitches_bmp[] = { 5 * 1 };

    uint32_t green_screen[7 * 7];
    const void * const source_data[] = { green_screen };
    uint32_t source_pitches[] = { 7 * 4 };
    for (int k = 0; k < 7 * 7; k ++) {
        green_screen[k] = 0xff00ff00;
    }

    ASSERT_OK(vdpOutputSurfacePutBitsNative(out_surface, source_data, source_pitches, NULL));
    ASSERT_OK(vdpBitmapSurfacePutBitsNative(bmp_surface, source_data_bmp, source_pitches_bmp, NULL));

    VdpOutputSurfaceRenderBlendState blend_state = {
        .blend_factor_source_color =        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
        .blend_factor_destination_color =   VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .blend_factor_source_alpha =        VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE,
        .blend_factor_destination_alpha =   VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA,
        .blend_equation_color =             VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_equation_alpha =             VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD,
        .blend_constant =                   {0, 0, 0, 0}
    };
    VdpColor color[] = {{0.7, 0.3, 0.1, 0.6}};

    VdpRect dest_rect = {1, 1, 6, 6};
    ASSERT_OK(vdpOutputSurfaceRenderBitmapSurface(out_surface, &dest_rect, bmp_surface, NULL,
                color, &blend_state, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));

    uint32_t result_buf[7 * 7];
    void * const dest_data[] = { result_buf };
    ASSERT_OK(vdpOutputSurfaceGetBitsNative(out_surface, NULL, dest_data, source_pitches));

    printf("--- actual ---\n");
    for (int k = 0; k < 7 * 7; k ++) {
        printf(" %08x", result_buf[k]);
        if (k % 7 == 7 - 1) printf("\n");
    }

    uint32_t expected[7 * 7];

    // compute expected result
    for (int x = 0; x < 7 * 7; x ++) expected[x] = green_screen[x];
    for (int y = 0; y < 5; y ++) {
        for (int x = 0; x < 5; x ++) {
            float src_r = 1.0 * color[0].red;
            float src_g = 1.0 * color[0].green;
            float src_b = 1.0 * color[0].blue;
            float src_a = bmp_data[y*5+x]/255.0 * color[0].alpha;

            uint32_t dst_bgra = expected[(y+1)*7 + (x+1)];
            float dst_a = ((dst_bgra >> 24) & 0xff) / 255.0;
            float dst_r = ((dst_bgra >> 16) & 0xff) / 255.0;
            float dst_g = ((dst_bgra >>  8) & 0xff) / 255.0;
            float dst_b = ((dst_bgra >>  0) & 0xff) / 255.0;

            float res_r = src_r * src_a + dst_r * (1.0 - src_a);
            float res_g = src_g * src_a + dst_g * (1.0 - src_a);
            float res_b = src_b * src_a + dst_b * (1.0 - src_a);
            float res_a = src_a * 1.0 + dst_a * src_a;

            uint32_t r = (res_r * 255.0);
            uint32_t g = (res_g * 255.0);
            uint32_t b = (res_b * 255.0);
            uint32_t a = (res_a * 255.0);
            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;
            if (a > 255) a = 255;

            expected[(y+1)*7 + (x+1)] = (a << 24) | (r << 16) | (g << 8) | (b);
        }
    }

    printf("--- expected ---\n");
    for (int k = 0; k < 7 * 7; k ++) {
        printf(" %08x", expected[k]);
        if (k % 7 == 7 - 1) printf("\n");
    }
    printf("=================\n");
    printf("--- difference --- \n");
    uint32_t max_diff = 0;
    for (int k = 0; k < 7 * 7; k ++) {
        uint32_t diff_a = abs(((expected[k] >> 24) & 0xff) - ((result_buf[k] >> 24) & 0xff));
        uint32_t diff_r = abs(((expected[k] >> 16) & 0xff) - ((result_buf[k] >> 16) & 0xff));
        uint32_t diff_g = abs(((expected[k] >>  8) & 0xff) - ((result_buf[k] >>  8) & 0xff));
        uint32_t diff_b = abs(((expected[k] >>  0) & 0xff) - ((result_buf[k] >>  0) & 0xff));

        printf(" %08x", (diff_a << 24) + (diff_r << 16) + (diff_g << 8) + (diff_b));
        if (k % 7 == 7 - 1) printf("\n");

        if (diff_a > max_diff) max_diff = diff_a;
        if (diff_r > max_diff) max_diff = diff_r;
        if (diff_g > max_diff) max_diff = diff_g;
        if (diff_b > max_diff) max_diff = diff_b;
    }
    printf("=================\n");

    if (max_diff > 1) {
        printf("fail\n");
        return 1;
    }

    printf("pass\n");
    return 0;
}
