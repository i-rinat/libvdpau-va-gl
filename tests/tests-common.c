#include "tests-common.h"


Display *
get_dpy(void)
{
    static Display *cached_dpy = NULL;

    if (cached_dpy)
        return cached_dpy;
    cached_dpy = XOpenDisplay(NULL);
    return cached_dpy;
}

Window
get_wnd(void)
{
    Display *dpy = get_dpy();
    Window root = XDefaultRootWindow(dpy);
    Window wnd = XCreateSimpleWindow(dpy, root, 0, 0, 300, 300, 0, 0, 0);
    XSync(dpy, False);
    return wnd;
}


// force linking library constructor
void va_gl_library_constructor();
void *dummy_ptr = va_gl_library_constructor;
