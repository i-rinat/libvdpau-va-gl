#include <X11/Xlib.h>
#include <stdio.h>

__attribute__((constructor))
static
void
library_constructor(void)
{
    XInitThreads();
    printf("XInitThreads()\n");
}
