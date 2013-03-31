// test-007

// TOUCHES: VdpBitmapSurfaceCreate
// TOUCHES: VdpBitmapSurfaceDestroy
// TOUCHES: VdpBitmapSurfaceGetParameters
// TOUCHES: VdpBitmapSurfaceQueryCapabilities

#include "vdpau-init.h"
#include <stdio.h>

int main(void)
{
    VdpDevice device;
    VdpBitmapSurface bmp_surf1;
    VdpBitmapSurface bmp_surf2;
    ASSERT_OK(vdpau_init_functions(&device, NULL, 0));

    // invalid device handle
    assert(VDP_STATUS_INVALID_HANDLE ==
            vdp_bitmap_surface_create(device+1, VDP_RGBA_FORMAT_A8, 13, 13, 1, &bmp_surf1));

    // invalid rgba format
    assert(VDP_STATUS_INVALID_RGBA_FORMAT ==
            vdp_bitmap_surface_create(device, -2, 13, 13, 1, &bmp_surf1));

    // normal paratemers
    ASSERT_OK(vdp_bitmap_surface_create(device, VDP_RGBA_FORMAT_B8G8R8A8, 123, 234, 1, &bmp_surf1));
    ASSERT_OK(vdp_bitmap_surface_create(device, VDP_RGBA_FORMAT_R8G8B8A8, 345, 456, 0, &bmp_surf2));

    uint32_t width, height;
    VdpBool fa;
    VdpRGBAFormat rgba_f;

    // test that getParameters get actual supplied parameters
    ASSERT_OK(vdp_bitmap_surface_get_parameters(bmp_surf1, &rgba_f, &width, &height, &fa));
    assert(VDP_RGBA_FORMAT_B8G8R8A8 == rgba_f);
    assert(123 == width);
    assert(234 == height);
    assert(1 == fa);

    // test with other surface
    ASSERT_OK(vdp_bitmap_surface_get_parameters(bmp_surf2, &rgba_f, &width, &height, &fa));
    assert(VDP_RGBA_FORMAT_R8G8B8A8 == rgba_f);
    assert(345 == width);
    assert(456 == height);
    assert(0 == fa);

    // test getParameters with NULLs
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_get_parameters(bmp_surf1, NULL, &width, &height, &fa));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_get_parameters(bmp_surf1, &rgba_f, NULL, &height, &fa));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_get_parameters(bmp_surf1, &rgba_f, &width, NULL, &fa));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_get_parameters(bmp_surf1, &rgba_f, &width, &height, NULL));

    // test with invalid bitmap handle
    assert(VDP_STATUS_INVALID_HANDLE ==
            vdp_bitmap_surface_get_parameters(device, &rgba_f, &width, &height, &fa));

    VdpBool is_supported;
    // testing query capabilities
    assert(VDP_STATUS_INVALID_HANDLE ==
            vdp_bitmap_surface_query_capabilities(device+1, VDP_RGBA_FORMAT_A8, &is_supported,
                &width, &height));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_A8, NULL,
                &width, &height));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_A8, &is_supported,
                NULL, &height));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_A8, &is_supported,
                &width, NULL));

    // querying various formats
    ASSERT_OK(vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_B8G8R8A8, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_R8G8B8A8, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_R10G10B10A2, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_B10G10R10A2, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdp_bitmap_surface_query_capabilities(device, VDP_RGBA_FORMAT_A8, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    // query wrong format
    ASSERT_OK(vdp_bitmap_surface_query_capabilities(device, 9000, &is_supported, &width, &height));
    assert (0 == is_supported);

    // try to destroy wrong surface
    assert (VDP_STATUS_INVALID_HANDLE == vdp_bitmap_surface_destroy(-2));
    assert (VDP_STATUS_INVALID_HANDLE == vdp_bitmap_surface_destroy(device));
    assert (VDP_STATUS_INVALID_HANDLE == vdp_bitmap_surface_destroy(bmp_surf1 + 43000));

    // really destroy surfaces
    ASSERT_OK(vdp_bitmap_surface_destroy(bmp_surf1));
    ASSERT_OK(vdp_bitmap_surface_destroy(bmp_surf2));

    ASSERT_OK(vdp_device_destroy(device));

    printf("pass\n");
    return 0;
}
