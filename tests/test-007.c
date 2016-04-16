// test-007

// TOUCHES: VdpBitmapSurfaceCreate
// TOUCHES: VdpBitmapSurfaceDestroy
// TOUCHES: VdpBitmapSurfaceGetParameters
// TOUCHES: VdpBitmapSurfaceQueryCapabilities

#include "tests-common.h"
#include <stdio.h>

int main(void)
{
    VdpDevice device = create_vdp_device();
    VdpBitmapSurface bmp_surf1;
    VdpBitmapSurface bmp_surf2;

    // invalid device handle
    assert(VDP_STATUS_INVALID_HANDLE ==
            vdpBitmapSurfaceCreate(device + 1, VDP_RGBA_FORMAT_A8, 13, 13, 1, &bmp_surf1));

    // invalid rgba format
    assert(VDP_STATUS_INVALID_RGBA_FORMAT ==
            vdpBitmapSurfaceCreate(device, -2, 13, 13, 1, &bmp_surf1));

    // normal paratemers
    ASSERT_OK(vdpBitmapSurfaceCreate(device, VDP_RGBA_FORMAT_B8G8R8A8, 123, 234, 1, &bmp_surf1));
    ASSERT_OK(vdpBitmapSurfaceCreate(device, VDP_RGBA_FORMAT_R8G8B8A8, 345, 456, 0, &bmp_surf2));

    uint32_t width, height;
    VdpBool fa;
    VdpRGBAFormat rgba_f;

    // test that getParameters get actual supplied parameters
    ASSERT_OK(vdpBitmapSurfaceGetParameters(bmp_surf1, &rgba_f, &width, &height, &fa));
    assert(VDP_RGBA_FORMAT_B8G8R8A8 == rgba_f);
    assert(width == 123);
    assert(height == 234);
    assert(fa == 1);

    // test with other surface
    ASSERT_OK(vdpBitmapSurfaceGetParameters(bmp_surf2, &rgba_f, &width, &height, &fa));
    assert(VDP_RGBA_FORMAT_R8G8B8A8 == rgba_f);
    assert(width == 345);
    assert(height == 456);
    assert(fa == 0);

    // test getParameters with NULLs
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceGetParameters(bmp_surf1, NULL, &width, &height, &fa));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceGetParameters(bmp_surf1, &rgba_f, NULL, &height, &fa));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceGetParameters(bmp_surf1, &rgba_f, &width, NULL, &fa));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceGetParameters(bmp_surf1, &rgba_f, &width, &height, NULL));

    // test with invalid bitmap handle
    assert(VDP_STATUS_INVALID_HANDLE ==
            vdpBitmapSurfaceGetParameters(device, &rgba_f, &width, &height, &fa));

    VdpBool is_supported;
    // testing query capabilities
    assert(VDP_STATUS_INVALID_HANDLE ==
            vdpBitmapSurfaceQueryCapabilities(device+1, VDP_RGBA_FORMAT_A8, &is_supported,
                &width, &height));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_A8, NULL,
                &width, &height));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_A8, &is_supported,
                NULL, &height));
    assert(VDP_STATUS_INVALID_POINTER ==
            vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_A8, &is_supported,
                &width, NULL));

    // querying various formats
    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_B8G8R8A8, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_R8G8B8A8, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_R10G10B10A2, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_B10G10R10A2, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, VDP_RGBA_FORMAT_A8, &is_supported,
                &width, &height));
    assert(is_supported);
    assert(width > 0);
    assert(height > 0);

    // query wrong format
    ASSERT_OK(vdpBitmapSurfaceQueryCapabilities(device, 9000, &is_supported, &width, &height));
    assert (0 == is_supported);

    // try to destroy wrong surface
    assert (VDP_STATUS_INVALID_HANDLE == vdpBitmapSurfaceDestroy(-2));
    assert (VDP_STATUS_INVALID_HANDLE == vdpBitmapSurfaceDestroy(device));
    assert (VDP_STATUS_INVALID_HANDLE == vdpBitmapSurfaceDestroy(bmp_surf1 + 43000));

    // really destroy surfaces
    ASSERT_OK(vdpBitmapSurfaceDestroy(bmp_surf1));
    ASSERT_OK(vdpBitmapSurfaceDestroy(bmp_surf2));

    ASSERT_OK(vdpDeviceDestroy(device));

    printf("pass\n");
    return 0;
}
