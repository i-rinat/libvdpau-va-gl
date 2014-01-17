#ifdef NDEBUG
#undef NDEBUG
#endif

#define CHECK(expr) if (VDP_STATUS_OK != (expr)) assert(0);

#include <assert.h>
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// force linking library constructor
void va_gl_library_constructor();
void *dummy_ptr = va_gl_library_constructor;

int
main(int argc, char *argv[])
{

    const int           width = 720;
    const int           height = 480;
    VdpGetProcAddress  *get_proc_address;
    VdpDevice           vdp_device;
    VdpVideoSurface     vdp_video_surface;
    VdpVideoMixer       vdp_video_mixer;
    VdpOutputSurface    vdp_output_surface;
    Display            *dpy;

    dpy = XOpenDisplay(NULL);
    assert(dpy);

    CHECK(vdpDeviceCreateX11(dpy, 0, &vdp_device, &get_proc_address));
    CHECK(vdpVideoSurfaceCreate(vdp_device, VDP_CHROMA_TYPE_420, width, height,
                                &vdp_video_surface));
    CHECK(vdpOutputSurfaceCreate(vdp_device, VDP_RGBA_FORMAT_B8G8R8A8, width, height,
                                 &vdp_output_surface));
    CHECK(vdpVideoMixerCreate(vdp_device, 0, NULL, 0, NULL, NULL, &vdp_video_mixer));

    char *y_plane = malloc(width * height);
    char *u_plane = malloc((width/2) * (height/2));
    char *v_plane = malloc((width/2) * (height/2));
    const void *source_planes[4] = { y_plane, u_plane, v_plane, NULL };
    uint32_t source_pitches[4] = { width, width/2, width/2, 0 };

    assert(y_plane);
    assert(u_plane);
    assert(v_plane);

    memset(y_plane, 128, width * height);
    memset(u_plane, 200, (width/2) * (height/2));
    memset(v_plane, 95, (width/2) * (height/2));

    struct timespec t_start, t_end;
    int rep_count = 3000;
    if (argc >= 2)
        rep_count = atoi(argv[1]);

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    for (int k = 0; k < rep_count; k ++) {
        CHECK(vdpVideoSurfacePutBitsYCbCr(vdp_video_surface, VDP_YCBCR_FORMAT_YV12,
                                          source_planes, source_pitches));
        CHECK(vdpVideoMixerRender(vdp_video_mixer, -1, NULL,
                                  VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
                                  0, NULL, vdp_video_surface, 0, NULL,
                                  NULL, vdp_output_surface, NULL, NULL, 0, NULL));
    }
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double duration = t_end.tv_sec - t_start.tv_sec + (t_end.tv_nsec - t_start.tv_nsec) / 1.0e9;

    printf("%d repetitions in %f secs, %f per sec\n", rep_count, duration, rep_count / duration);

    CHECK(vdpOutputSurfaceDestroy(vdp_output_surface));
    CHECK(vdpVideoMixerDestroy(vdp_video_mixer));
    CHECK(vdpVideoSurfaceDestroy(vdp_video_surface));
    CHECK(vdpDeviceDestroy(vdp_device));
    return 0;
}
