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
    // return false if index is invalid
    if (handle < 1 || handle >= vdpHandles->len) return 0;

    // return false if entry was deleted
    if (NULL == g_ptr_array_index(vdpHandles, handle)) return 0;

    // otherwise return true if called want any handle
    if (HANDLETYPE_ANY == type) return 1;

    // else check handle type
    HandleType elementType = ((VdpGenericHandle *)g_ptr_array_index(vdpHandles, handle))->type;
    if (elementType == type)
        return 1;
    else
        return 0;
}

void *
handlestorage_get(int handle, HandleType type)
{

    if (handle < 1 || handle >= vdpHandles->len) return NULL;
    void *result = g_ptr_array_index(vdpHandles, handle);
    if (!result) return NULL;
    if (HANDLETYPE_ANY == type) return result;
    if (type != ((VdpGenericHandle *)result)->type) result = NULL;
    return result;
}

void
handlestorage_expunge(int handle)
{
    if (handlestorage_valid(handle, HANDLETYPE_ANY)) {
        g_ptr_array_index(vdpHandles, handle) = NULL;
    }
}

void
handlestorage_destory(void)
{
    g_ptr_array_unref(vdpHandles);
}
