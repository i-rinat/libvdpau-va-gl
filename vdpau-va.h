#ifndef VDPAU_VA_H_
#define VDPAU_VA_H_

#include <vdpau/vdpau.h>

VdpStatus
vaVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                     VdpGetProcAddress **get_proc_address);

#endif /* VDPAU_VA_H_ */
