/*
 * Copyright 2013-2014  Rinat Ibragimov
 *
 * This file is part of libvdpau-va-gl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VA_GL_SRC_CTX_STACK_H
#define VA_GL_SRC_CTX_STACK_H

#include "api.h"

void        glx_ctx_push_global(Display *dpy, Drawable wnd, GLXContext glc);
void        glx_ctx_push_thread_local(VdpDeviceData *deviceData);
void        glx_ctx_pop(void);
void        glx_ctx_ref_glc_hash_table(Display *dpy, int screen);
void        glx_ctx_unref_glc_hash_table(Display *dpy);
GLXContext  glx_ctx_get_root_context(void);

void        glx_ctx_lock(void);
void        glx_ctx_unlock(void);

void        x11_push_eh(void);
int         x11_pop_eh(void);

#endif /* VA_GL_SRC_CTX_STACK_H */
