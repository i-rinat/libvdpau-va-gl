// test-004
//
// Large scale (> 500 pixels) smooth test for VdpOutputSurfaceRenderBitmapSurface and
// VdpOutputSurfaceRenderOutputSurface.
// Rendering the same pattern via both paths and then comparing results. Using opaque copy,
// only source matters.

#include "tests-common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH       509
#define HEIGHT      601

int main(void)
{
    Display *dpy = get_dpy();
    int err_code = 0;
    VdpDevice device;
    ASSERT_OK(vdpDeviceCreateX11(dpy, 0, &device, NULL));

    VdpOutputSurface out_surface_in;
    VdpOutputSurface out_surface_out;
    VdpBitmapSurface bmp_surface;
    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, WIDTH, HEIGHT, &out_surface_in));
    ASSERT_OK(vdpOutputSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, WIDTH, HEIGHT, &out_surface_out));
    ASSERT_OK(vdpBitmapSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, WIDTH, HEIGHT, 1, &bmp_surface));

    uint32_t *src = malloc(4 * WIDTH * HEIGHT);
    uint32_t *dst = malloc(4 * WIDTH * HEIGHT);

    assert (NULL != src || NULL != dst);

    for (int k = 0; k < WIDTH * HEIGHT; k ++) {
        src[k] = ((k & 0xff) << 8) + (0xff << 24);  // green pixel pattern
    }

    const void * const source_data[] = { src };
    void * const destination_data[] = { dst };
    uint32_t source_pitches[] = { 4 * WIDTH };
    uint32_t destination_pitches[] = { 4 * WIDTH };

    ASSERT_OK(vdpOutputSurfacePutBitsNative(out_surface_in, source_data, source_pitches, NULL));
    ASSERT_OK(vdpBitmapSurfacePutBitsNative(bmp_surface, source_data, source_pitches, NULL));

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
    ASSERT_OK(vdpOutputSurfaceRenderOutputSurface(out_surface_out, NULL, out_surface_in, NULL,
                NULL, &blend_state_opaque_copy, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));

    // check result of vdpOutputSurfaceRenderOutputSurface
    ASSERT_OK(vdpOutputSurfaceGetBitsNative(out_surface_out, NULL, destination_data, destination_pitches));
    if (calc_difference_r8g8b8a8(src, dst, WIDTH * HEIGHT) > 2) {
        printf("fail / vdpOutputSurfaceRenderOutputSurface\n");
        err_code = 1;
        goto free_resources_and_exit;
    }

    // check vdpOutputSurfaceRenderBitmapSurface
    ASSERT_OK(vdpOutputSurfaceRenderBitmapSurface(out_surface_out, NULL, bmp_surface, NULL,
                NULL, &blend_state_opaque_copy, VDP_OUTPUT_SURFACE_RENDER_ROTATE_0));
    ASSERT_OK(vdpOutputSurfaceGetBitsNative(out_surface_out, NULL, destination_data, destination_pitches));
    if (calc_difference_r8g8b8a8(src, dst, WIDTH * HEIGHT) > 2) {
        printf("fail / vdpOutputSurfaceRenderBitmapSurface\n");
        err_code = 2;
        goto free_resources_and_exit;
    }

    printf("pass\n");
free_resources_and_exit:
    free(src);
    free(dst);
    return err_code;
}
