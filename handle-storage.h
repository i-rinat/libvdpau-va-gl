#pragma once
#ifndef HANDLE_STORAGE_H_
#define HANDLE_STORAGE_H_

#include <vdpau/vdpau_x11.h>
#include <glib.h>

typedef struct {
    Display *display;
    int screen;
} VdpDeviceData;

extern GPtrArray *vdpDeviceHandles;

void
initialize_handle_arrays(void);

#endif /* HANDLE_STORAGE_H_ */
