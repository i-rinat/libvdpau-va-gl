#ifndef VDPAU_SOFT_H_
#define VDPAU_SOFT_H_

#include <vdpau/vdpau.h>

VdpStatus
softVdpDeviceCreateX11(Display *display, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address);

#endif /* VDPAU_SOFT_H_ */
