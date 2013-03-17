#include <assert.h>
#include <stdio.h>
#include <vdpau/vdpau.h>
#include "vdpau-init.h"

int main(void)
{
    VdpDevice device;
    VdpStatus st = vdpau_init_functions(&device);
    assert (VDP_STATUS_OK == st);

    printf("hello from test.\n");
}
