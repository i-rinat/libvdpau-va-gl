// test-009

// Create and destroy vdp device many times a row.
// Intended to check X resource leakage introduced by library.

#include "vdpau-init.h"
#include <stdio.h>

int main(void)
{
    VdpDevice device;

    for (int k = 0; k < 3000; k ++) {
        ASSERT_OK(vdpau_init_functions(&device, NULL, 0));
        ASSERT_OK(vdp_device_destroy(device));
    }

    printf("pass\n");
    return 0;
}
