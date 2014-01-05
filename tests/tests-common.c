#include "tests-common.h"
#include <stdlib.h>

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

static inline
int
max2(int a, int b)
{
    return (a > b) ? a : b;
}

int
calc_difference_a8(uint8_t *src1, uint8_t *src2, int count)
{
    int max_diff = 0;
    for (int k = 0; k < count; k ++)
        max_diff = max2(max_diff, abs(src1[k] - src2[k]));
    return max_diff;
}

int
calc_difference_r8g8b8a8(uint32_t *src1, uint32_t *src2, int count)
{
    int max_diff = 0;
    for (int k = 0; k < count; k ++) {
        const uint8_t r1 = (src1[k] >> 24) & 0xffu;
        const uint8_t g1 = (src1[k] >> 16) & 0xffu;
        const uint8_t b1 = (src1[k] >>  8) & 0xffu;
        const uint8_t a1 = (src1[k] >>  0) & 0xffu;
        const uint8_t r2 = (src2[k] >> 24) & 0xffu;
        const uint8_t g2 = (src2[k] >> 16) & 0xffu;
        const uint8_t b2 = (src2[k] >>  8) & 0xffu;
        const uint8_t a2 = (src2[k] >>  0) & 0xffu;

        const int tmp1 = max2(abs(r1 - r2), abs(g1 - g2));
        const int tmp2 = max2(abs(b1 - b2), abs(a1 - a2));
        max_diff = max2(max_diff, max2(tmp1, tmp2));
    }
    return max_diff;
}

// force linking library constructor
void va_gl_library_constructor();
void *dummy_ptr = va_gl_library_constructor;
