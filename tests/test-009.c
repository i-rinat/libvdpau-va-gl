// test-009

// Create and destroy vdp device many times a row.
// Intended to check X resource leakage introduced by library.

#include "tests-common.h"
#include <stdio.h>

int main(void)
{
    VdpDevice device;
    Display *dpy = get_dpy();

    for (int k = 0; k < 300; k ++) {
        ASSERT_OK(vdpDeviceCreateX11(dpy, 0, &device, NULL));
        ASSERT_OK(vdpDeviceDestroy(device));
    }

    printf("pass\n");
    return 0;
}
