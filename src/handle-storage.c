/*
 * Copyright 2013  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * libvdpau-va-gl is distributed under the terms of the LGPLv3. See COPYING for details.
 */

#define _XOPEN_SOURCE   500
#include "handle-storage.h"
#include <pthread.h>
#include <glib.h>
#include <unistd.h>

static GHashTable *vdp_handles;
static GHashTable *xdpy_copies;            //< Copies of X Display connections
static GHashTable *xdpy_copies_refcount;   //< Reference count of X Display connection copy

static uint32_t next_handle_id;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void
handle_initialize_storage(void)
{
    pthread_mutex_lock(&lock);
    vdp_handles = g_hash_table_new(g_direct_hash, g_direct_equal);
    next_handle_id = 1;

    xdpy_copies = g_hash_table_new(g_direct_hash, g_direct_equal);
    xdpy_copies_refcount = g_hash_table_new(g_direct_hash, g_direct_equal);
    pthread_mutex_unlock(&lock);
}

int
handle_insert(void *data)
{
    int id;

    pthread_mutex_lock(&lock);
    while (g_hash_table_lookup(vdp_handles, GINT_TO_POINTER(next_handle_id)))
        next_handle_id ++;

    id = next_handle_id ++;
    g_hash_table_insert(vdp_handles, GINT_TO_POINTER(id), data);

    pthread_mutex_unlock(&lock);
    return id;
}

// lock unsafe function
static
int
_is_valid(int handle, HandleType type)
{
    VdpGenericHandle *gh;

    gh = g_hash_table_lookup(vdp_handles, GINT_TO_POINTER(handle));
    if (!gh)
         return 0;

    // return true if caller wants any handle type
    if (HANDLETYPE_ANY == type)
        return 1;

    // check handle type
    if (gh->type == type)
        return 1;

    return 0;
}

void *
handle_acquire(int handle, HandleType type)
{
    VdpGenericHandle *res = NULL;

    while (1) {
        pthread_mutex_lock(&lock);
        if (!_is_valid(handle, type)) {
            res = NULL;
            break;
        }
        res = g_hash_table_lookup(vdp_handles, GINT_TO_POINTER(handle));
        if (pthread_mutex_trylock(&res->lock) == 0)
            break;
        pthread_mutex_unlock(&lock);
        usleep(1);
    }

    pthread_mutex_unlock(&lock);
    return res;
}

void
handle_release(int handle)
{
    pthread_mutex_lock(&lock);
    VdpGenericHandle *gh = g_hash_table_lookup(vdp_handles, GINT_TO_POINTER(handle));
    if (gh)
        pthread_mutex_unlock(&gh->lock);
    pthread_mutex_unlock(&lock);
}

void
handle_expunge(int handle)
{
    pthread_mutex_lock(&lock);
    if (_is_valid(handle, HANDLETYPE_ANY)) {
        VdpGenericHandle *gh = g_hash_table_lookup(vdp_handles, GINT_TO_POINTER(handle));
        if (gh)
            pthread_mutex_unlock(&gh->lock);
        g_hash_table_remove(vdp_handles, GINT_TO_POINTER(handle));
    }
    pthread_mutex_unlock(&lock);
}

void
handle_destory_storage(void)
{
    pthread_mutex_lock(&lock);
    g_hash_table_unref(vdp_handles);            vdp_handles = NULL;
    g_hash_table_unref(xdpy_copies);            xdpy_copies = NULL;
    g_hash_table_unref(xdpy_copies_refcount);   xdpy_copies_refcount = NULL;
    pthread_mutex_unlock(&lock);
}

void
handle_execute_for_all(void (*callback)(int idx, void *entry, void *p), void *param)
{
    pthread_mutex_lock(&lock);
    GList *tmp = g_hash_table_get_keys(vdp_handles);
    GList *keys = g_list_copy(tmp);
    g_list_free(tmp);

    GList *ptr = g_list_first(keys);
    while (ptr) {
        HandleType handle = GPOINTER_TO_INT(ptr->data);

        void *item = g_hash_table_lookup(vdp_handles, GINT_TO_POINTER(handle));
        if (item) {
            pthread_mutex_unlock(&lock);
            // TODO: race condition. Supply integer handle instead of pointer to fix.
            callback(handle, item, param);
            pthread_mutex_lock(&lock);
        }
        ptr = g_list_next(ptr);
    }
    g_list_free(keys);
    pthread_mutex_unlock(&lock);
}

void *
handle_xdpy_ref(void *dpy_orig)
{
    pthread_mutex_lock(&lock);
    Display *dpy = g_hash_table_lookup(xdpy_copies, dpy_orig);
    if (NULL == dpy) {
        dpy = XOpenDisplay(XDisplayString(dpy_orig));
        if (!dpy)
            goto quit;
        g_hash_table_replace(xdpy_copies, dpy_orig, dpy);
        g_hash_table_replace(xdpy_copies_refcount, dpy_orig, GINT_TO_POINTER(1));
    } else {
        int refcount = GPOINTER_TO_INT(g_hash_table_lookup(xdpy_copies_refcount, dpy_orig));
        g_hash_table_replace(xdpy_copies_refcount, dpy_orig, GINT_TO_POINTER(refcount+1));
    }
quit:
    pthread_mutex_unlock(&lock);
    return dpy;
}

void
handle_xdpy_unref(void *dpy_orig)
{
    pthread_mutex_lock(&lock);
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
    pthread_mutex_unlock(&lock);
}
