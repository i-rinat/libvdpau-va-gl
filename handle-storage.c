#include "handle-storage.h"
#include <glib.h>

GPtrArray *vdpHandles;

void
handlestorage_initialize(void)
{
    vdpHandles = g_ptr_array_new();
    // adding dummy element to ensure all handles start from 1
    g_ptr_array_add(vdpHandles, NULL);
}

int
handlestorage_add(void *data)
{
    g_ptr_array_add(vdpHandles, data);
    return vdpHandles->len - 1;
}

int
handlestorage_valid(int handle, HandleType type)
{
    if (handle < 1 || handle >= vdpHandles->len)
        return 0;
    if (HANDLE_TYPE_ANY == type)
        return 1;
    HandleType elementType = ((VdpGenericHandle *)g_ptr_array_index(vdpHandles, handle))->type;
    if (elementType == type)
        return 1;
    else
        return 0;
}

void *
handlestorage_get(int handle)
{
    if (handlestorage_valid(handle, HANDLE_TYPE_ANY))
        return g_ptr_array_index(vdpHandles, handle);
    else
        return NULL;
}

void
handlestorage_destory(void)
{
    g_ptr_array_unref(vdpHandles);
}
