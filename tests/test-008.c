// test-008

// Progressively create larger and larger bitmap surfaces, up to maximum
// allowed size. First square ones, then stretched in both directions
// in turn

// TOUCHES: VdpBitmapSurfaceCreate
// TOUCHES: VdpBitmapSurfaceQueryCapabilities

#include "tests-common.h"
#include <stdio.h>


void
test_bitmaps_of_format(VdpDevice device, int fmt, const char *fmt_name,
                       uint32_t max_width, uint32_t max_height)
{
    VdpBitmapSurface bmp_surf1;
    const uint32_t max_square_size = MIN(max_width, max_height);
    const uint32_t step = 128;

    // trying square surface
    for (uint32_t k = 0; k < max_square_size + step; (k < step) ? (k++) : (k+=step)) {
        for (uint32_t freq = 0; freq <= 1; freq ++) {
            const uint32_t size = MAX(1, MIN(k, max_square_size));
            printf("trying square %s bitmap %d x %d (%d)\n", fmt_name, size, size, freq);
            ASSERT_OK(vdpBitmapSurfaceCreate(device, fmt, size, size, freq, &bmp_surf1));
            ASSERT_OK(vdpBitmapSurfaceDestroy(bmp_surf1));
        }
    }

    // width stretched
    for (uint32_t k = 0; k < max_width + step; (k < step) ? (k++) : (k+=step)) {
        for (uint32_t freq = 0; freq <= 1; freq ++) {
            const uint32_t size = MAX(1, MIN(k, max_width));
            printf("trying width stretched %s bitmap %d x %d (%d)\n", fmt_name, size, 128, freq);
            ASSERT_OK(vdpBitmapSurfaceCreate(device, fmt, size, 128, freq, &bmp_surf1));
            ASSERT_OK(vdpBitmapSurfaceDestroy(bmp_surf1));
        }
    }

    // height stretched
    for (uint32_t k = 0; k < max_height + step; (k < step) ? (k++) : (k+=step)) {
        for (uint32_t freq = 0; freq <= 1; freq ++) {
            const uint32_t size = MAX(1, MIN(k, max_height));
            printf("trying height stretched %s bitmap %d x %d (%d)\n", fmt_name, 128, size, freq);
            ASSERT_OK(vdpBitmapSurfaceCreate(device, fmt, 128, size, freq, &bmp_surf1));
            ASSERT_OK(vdpBitmapSurfaceDestroy(bmp_surf1));
        }
    }
}


int main(void)
{
    Display *dpy = get_dpy();
    VdpDevice device;

    ASSERT_OK(vdpDeviceCreateX11(dpy, 0, &device, NULL));

    uint32_t max_width, max_height;
    VdpBool is_supported;

    // querying max_size
    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_B8G8R8A8, &is_supported,
                                                &max_width, &max_height));
    assert(is_supported);
    assert(max_width > 0);
    assert(max_height > 0);

    test_bitmaps_of_format(device, VDP_RGBA_FORMAT_B8G8R8A8, "VDP_RGBA_FORMAT_B8G8R8A8",
                           max_width, max_height);

    test_bitmaps_of_format(device, VDP_RGBA_FORMAT_R8G8B8A8, "VDP_RGBA_FORMAT_R8G8B8A8",
                           max_width, max_height);

    test_bitmaps_of_format(device, VDP_RGBA_FORMAT_R10G10B10A2, "VDP_RGBA_FORMAT_R10G10B10A2",
                           max_width, max_height);

    test_bitmaps_of_format(device, VDP_RGBA_FORMAT_B10G10R10A2, "VDP_RGBA_FORMAT_B10G10R10A2",
                           max_width, max_height);

    test_bitmaps_of_format(device, VDP_RGBA_FORMAT_A8, "VDP_RGBA_FORMAT_A8",
                           max_width, max_height);

    ASSERT_OK(vdpDeviceDestroy(device));

    printf("pass\n");
    return 0;
}
