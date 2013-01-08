#include "handle-storage.h"
#include <glib.h>

GPtrArray *vdpDeviceHandles;

void
initialize_handle_arrays(void)
{
    vdpDeviceHandles = g_ptr_array_new();
}

void
destroy_handle_arrays(void)
{
    g_ptr_array_unref(vdpDeviceHandles);
}
