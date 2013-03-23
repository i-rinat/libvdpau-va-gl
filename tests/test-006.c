// test-006
//
// initializing/finalizing three times a row

#include "vdpau-init.h"

int main(void)
{
    VdpDevice device;
    vdp_device_destroy(device);
    ASSERT_OK(vdpau_init_functions(&device, &window, 1));

    ASSERT_OK(vdpau_init_functions(&device));
    vdp_device_destroy(device);

    ASSERT_OK(vdpau_init_functions(&device));
    vdp_device_destroy(device);

    return 0;
}
