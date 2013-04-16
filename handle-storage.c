/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl distributed under the terms of LGPLv3. See COPYING for details.
 */

#include "handle-storage.h"
#include <glib.h>

GPtrArray *vdpHandles;
GHashTable *xdpy_copies;     //< Copies of X Display connections

void
handlestorage_initialize(void)
{
    vdpHandles = g_ptr_array_new();
    // adding dummy element to ensure all handles start from 1
    g_ptr_array_add(vdpHandles, NULL);

    xdpy_copies = g_hash_table_new(g_direct_hash, g_direct_equal);
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
    if (handle < 1 || handle >= (int)vdpHandles->len) return 0;

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

    if (handle < 1 || handle >= (int)vdpHandles->len) return NULL;
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
    g_hash_table_unref(xdpy_copies);
}

void
handlestorage_execute_for_all(void (*callback)(int idx, void *entry, void *p), void *param)
{
    for (unsigned int k = 0; k < vdpHandles->len; k ++) {
        void *item = g_ptr_array_index(vdpHandles, k);
        if (item)
            callback(k, item, param);
    }
}

void *
handlestorage_get_cached_xdpy_copy(void *original)
{
    return g_hash_table_lookup(xdpy_copies, original);
}

void
handlestorage_push_xdpy_copy(void *original, void *copy)
{
    g_hash_table_insert(xdpy_copies, original, copy);
}

void
handlestorage_expunge_xdpy_copy(void *original)
{
    g_hash_table_remove(xdpy_copies, original);
}
