/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#include "handle-storage.h"
#include <glib.h>

GPtrArray *vdpHandles;
GHashTable *xdpy_copies;            //< Copies of X Display connections
GHashTable *xdpy_copies_refcount;   //< Reference count of X Display connection copy
void
handle_initialize_storage(void)
{
    vdpHandles = g_ptr_array_new();
    // adding dummy element to ensure all handles start from 1
    g_ptr_array_add(vdpHandles, NULL);

    xdpy_copies = g_hash_table_new(g_direct_hash, g_direct_equal);
    xdpy_copies_refcount = g_hash_table_new(g_direct_hash, g_direct_equal);
}

int
handle_insert(void *data)
{
    g_ptr_array_add(vdpHandles, data);
    return vdpHandles->len - 1;
}

int
handle_is_valid(int handle, HandleType type)
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
handle_acquire(int handle, HandleType type)
{
    if (handle < 1 || handle >= (int)vdpHandles->len) return NULL;
    void *result = g_ptr_array_index(vdpHandles, handle);
    if (!result) return NULL;
    if (HANDLETYPE_ANY == type) return result;
    if (type != ((VdpGenericHandle *)result)->type) result = NULL;
    return result;
}

void
handle_release(int handle)
{
    (void)handle;
    // placeholder
}

void
handle_expunge(int handle)
{
    if (handle_is_valid(handle, HANDLETYPE_ANY)) {
        g_ptr_array_index(vdpHandles, handle) = NULL;
    }
}

void
handle_destory_storage(void)
{
    g_ptr_array_unref(vdpHandles);
    g_hash_table_unref(xdpy_copies);
    g_hash_table_unref(xdpy_copies_refcount);
}

void
handle_execute_for_all(void (*callback)(int idx, void *entry, void *p), void *param)
{
    for (unsigned int k = 0; k < vdpHandles->len; k ++) {
        void *item = g_ptr_array_index(vdpHandles, k);
        if (item)
            callback(k, item, param);
    }
}

void *
handle_xdpy_ref(void *dpy_orig)
{
    Display *dpy = g_hash_table_lookup(xdpy_copies, dpy_orig);
    if (NULL == dpy) {
        dpy = XOpenDisplay(XDisplayString(dpy_orig));
        if (NULL == dpy)
            return NULL;
        g_hash_table_replace(xdpy_copies, dpy_orig, dpy);
        g_hash_table_replace(xdpy_copies_refcount, dpy_orig, GINT_TO_POINTER(1));
    } else {
        int refcount = GPOINTER_TO_INT(g_hash_table_lookup(xdpy_copies_refcount, dpy_orig));
        g_hash_table_replace(xdpy_copies_refcount, dpy_orig, GINT_TO_POINTER(refcount+1));
    }
    return dpy;
}

void
handle_xdpy_unref(void *dpy_orig)
{
    int refcount = GPOINTER_TO_INT(g_hash_table_lookup(xdpy_copies_refcount, dpy_orig));
    refcount = refcount - 1;
    if (0 == refcount) {
        // do close connection, nobody refers it anymore
        Display *dpy = g_hash_table_lookup(xdpy_copies, dpy_orig);
        XCloseDisplay(dpy);
        g_hash_table_remove(xdpy_copies, dpy_orig);
        g_hash_table_remove(xdpy_copies_refcount, dpy_orig);
    } else {
        // just update refcount
        g_hash_table_replace(xdpy_copies_refcount, dpy_orig, GINT_TO_POINTER(refcount));
    }
}
