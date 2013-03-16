#include <vdpau/vdpau.h>
#include <stdio.h>
#include <stdlib.h>
#include "handle-storage.h"
#include "vdpau-soft.h"
#include "vdpau-trace.h"

__attribute__((constructor))
static
void
library_constructor(void)
{
    handlestorage_initialize();

    // initialize tracer
    traceSetTarget(stdout);
    traceTrace("Software VDPAU backend library initialized\n");
#ifdef NDEBUG
    traceEnableTracing(0);
#else
    traceEnableTracing(1);
#endif
    if (getenv("VDPAU_VA_GL_NOLOG"))
        traceEnableTracing(0);
}

__attribute__ ((visibility("default")))
VdpStatus
vdp_imp_device_create_x11(Display *display, int screen, VdpDevice *device,
                          VdpGetProcAddress **get_proc_address)
{
    // Select appropriate sub-backend. For now only software backend available.
    if (1) {
        return softVdpDeviceCreateX11(display, screen, device, get_proc_address);
    }
}
