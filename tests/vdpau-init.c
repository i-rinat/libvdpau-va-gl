#include "vdpau-init.h"


VdpGetErrorString       *vdp_get_error_string = NULL;
VdpGetApiVersion        *vdp_get_api_version = NULL;
VdpGetInformationString *vdp_get_information_string = NULL;
VdpDeviceDestroy        *vdp_device_destroy = NULL;
VdpGenerateCSCMatrix    *vdp_generate_csc_matrix = NULL;

VdpVideoSurfaceQueryCapabilities                *vdp_video_surface_query_capabilities = NULL;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *vdp_video_surface_query_get_putBits_y_cb_cr_capabilities = NULL;
VdpVideoSurfaceCreate                           *vdp_video_surface_create = NULL;
VdpVideoSurfaceDestroy                          *vdp_video_surface_destroy = NULL;
VdpVideoSurfaceGetParameters                    *vdp_video_surface_get_parameters = NULL;
VdpVideoSurfaceGetBitsYCbCr                     *vdp_video_surface_get_bits_y_cb_cr = NULL;
VdpVideoSurfacePutBitsYCbCr                     *vdp_video_surface_put_bits_y_cb_cr = NULL;

VdpOutputSurfaceQueryCapabilities               *vdp_output_surface_query_capabilities = NULL;
VdpOutputSurfaceQueryGetPutBitsNativeCapabilities *vdp_output_surface_query_get_put_bits_native_capabilities = NULL;
VdpOutputSurfaceQueryPutBitsIndexedCapabilities *vdp_output_surface_query_put_bits_indexed_capabilities = NULL;
VdpOutputSurfaceQueryPutBitsYCbCrCapabilities   *vdp_output_surface_query_put_bits_y_cb_cr_capabilities = NULL;
VdpOutputSurfaceCreate                          *vdp_output_surface_create = NULL;
VdpOutputSurfaceDestroy                         *vdp_output_surface_destroy = NULL;
VdpOutputSurfaceGetParameters                   *vdp_output_surface_get_parameters = NULL;
VdpOutputSurfaceGetBitsNative                   *vdp_output_surface_get_bits_native = NULL;
VdpOutputSurfacePutBitsNative                   *vdp_output_surface_put_bits_native = NULL;
VdpOutputSurfacePutBitsIndexed                  *vdp_output_surface_put_bits_indexed = NULL;
VdpOutputSurfacePutBitsYCbCr                    *vdp_output_surface_put_bits_y_cb_cr = NULL;

VdpBitmapSurfaceQueryCapabilities   *vdp_bitmap_surface_query_capabilities = NULL;
VdpBitmapSurfaceCreate              *vdp_bitmap_surface_create = NULL;
VdpBitmapSurfaceDestroy             *vdp_bitmap_surface_destroy = NULL;
VdpBitmapSurfaceGetParameters       *vdp_bitmap_surface_get_parameters = NULL;
VdpBitmapSurfacePutBitsNative       *vdp_bitmap_surface_put_bits_native = NULL;

VdpOutputSurfaceRenderOutputSurface *vdp_output_surface_render_output_surface = NULL;
VdpOutputSurfaceRenderBitmapSurface *vdp_output_surface_render_bitmap_surface = NULL;

VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities = NULL;
VdpDecoderCreate            *vdp_decoder_create = NULL;
VdpDecoderDestroy           *vdp_decoder_destroy = NULL;
VdpDecoderGetParameters     *vdp_decoder_get_parameters = NULL;
VdpDecoderRender            *vdp_decoder_render = NULL;

VdpVideoMixerQueryFeatureSupport        *vdp_video_mixer_query_feature_support = NULL;
VdpVideoMixerQueryParameterSupport      *vdp_video_mixer_query_parameter_support = NULL;
VdpVideoMixerQueryAttributeSupport      *vdp_video_mixer_query_attribute_support = NULL;
VdpVideoMixerQueryParameterValueRange   *vdp_video_mixer_query_parameter_value_range = NULL;
VdpVideoMixerQueryAttributeValueRange   *vdp_video_mixer_query_attribute_value_range = NULL;

VdpVideoMixerCreate             *vdp_video_mixer_create = NULL;
VdpVideoMixerSetFeatureEnables  *vdp_video_mixer_set_feature_enables = NULL;
VdpVideoMixerSetAttributeValues *vdp_video_mixer_set_attribute_values = NULL;
VdpVideoMixerGetFeatureSupport  *vdp_video_mixer_get_feature_support = NULL;
VdpVideoMixerGetFeatureEnables  *vdp_video_mixer_get_feature_enables = NULL;
VdpVideoMixerGetParameterValues *vdp_video_mixer_get_parameter_values = NULL;
VdpVideoMixerGetAttributeValues *vdp_video_mixer_get_attribute_values = NULL;
VdpVideoMixerDestroy            *vdp_video_mixer_destroy = NULL;
VdpVideoMixerRender             *vdp_video_mixer_render = NULL;

VdpPresentationQueueTargetDestroy       *vdp_presentation_queue_target_destroy = NULL;
VdpPresentationQueueCreate              *vdp_presentation_queue_create = NULL;
VdpPresentationQueueDestroy             *vdp_presentation_queue_destroy = NULL;
VdpPresentationQueueSetBackgroundColor  *vdp_presentation_queue_set_background_color = NULL;
VdpPresentationQueueGetBackgroundColor  *vdp_presentation_queue_get_background_color = NULL;
VdpPresentationQueueGetTime             *vdp_presentation_queue_get_time = NULL;
VdpPresentationQueueDisplay             *vdp_presentation_queue_display = NULL;
VdpPresentationQueueBlockUntilSurfaceIdle *vdpPresentation_queue_block_until_surface_idle = NULL;
VdpPresentationQueueQuerySurfaceStatus  *vdp_presentation_queue_query_surface_status = NULL;

VdpPreemptionCallback           *vdp_preemption_callback = NULL;
VdpPreemptionCallbackRegister   *vdp_preemption_callback_register = NULL;

VdpGetProcAddress   *vdp_get_proc_address = NULL;

VdpStatus
vdpau_init_functions(VdpDevice *device)
{
    Display *dpy = XOpenDisplay(NULL);
    // Window root = XDefaultRootWindow(dpy);
    // Window window = XCreateSimpleWindow(dpy, root, 0, 0, 300, 300, 0, 0, 0);
    // XMapWindow(dpy, window);
    XSync(dpy, 0);

    VdpStatus st = vdp_device_create_x11(dpy, 0, device, &vdp_get_proc_address);
    if (VDP_STATUS_OK != st)
        return st;
    if (!vdp_get_proc_address)
        return VDP_STATUS_ERROR;

    return VDP_STATUS_OK;
}
