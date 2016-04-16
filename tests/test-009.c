// test-009

// Create and destroy vdp device many times a row.
// Intended to check X resource leakage introduced by library.

#include "tests-common.h"
#include <stdio.h>

int main(void)
{
    for (int k = 0; k < 30; k ++) {
        VdpDevice device = create_vdp_device();
        ASSERT_OK(vdpDeviceDestroy(device));
    }

    printf("pass\n");
    return 0;
}
