#include "vdpau-init.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>


VdpGetErrorString       *vdp_get_error_string = NULL;
VdpGetApiVersion        *vdp_get_api_version = NULL;
VdpGetInformationString *vdp_get_information_string = NULL;
VdpDeviceDestroy        *vdp_device_destroy = NULL;
VdpGenerateCSCMatrix    *vdp_generate_csc_matrix = NULL;

VdpVideoSurfaceQueryCapabilities                *vdp_video_surface_query_capabilities = NULL;
VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities = NULL;
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
VdpPresentationQueueBlockUntilSurfaceIdle *vdp_presentation_queue_block_until_surface_idle = NULL;
VdpPresentationQueueQuerySurfaceStatus  *vdp_presentation_queue_query_surface_status = NULL;

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

#define GET_ADDR(id, ptr)   \
    do { \
        st = vdp_get_proc_address(*device, id, (void **)&ptr); \
        assert(VDP_STATUS_OK==st); \
        assert(NULL != ptr); \
    } while(0)

    GET_ADDR(VDP_FUNC_ID_GET_ERROR_STRING,          vdp_get_error_string);

    GET_ADDR(VDP_FUNC_ID_GET_API_VERSION,           vdp_get_api_version);
    GET_ADDR(VDP_FUNC_ID_GET_INFORMATION_STRING,    vdp_get_information_string);
    GET_ADDR(VDP_FUNC_ID_DEVICE_DESTROY,            vdp_device_destroy);
    GET_ADDR(VDP_FUNC_ID_GENERATE_CSC_MATRIX,       vdp_generate_csc_matrix);

    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES, vdp_video_surface_query_capabilities);
    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES,
                                        vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities);
    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_CREATE, vdp_video_surface_create);
    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, vdp_video_surface_destroy);
    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, vdp_video_surface_get_parameters);
    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, vdp_video_surface_get_bits_y_cb_cr);
    GET_ADDR(VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR, vdp_video_surface_put_bits_y_cb_cr);

    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES, vdp_output_surface_query_capabilities);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES,
                                    vdp_output_surface_query_get_put_bits_native_capabilities);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES,
                                        vdp_output_surface_query_put_bits_indexed_capabilities);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES,
                                        vdp_output_surface_query_put_bits_y_cb_cr_capabilities);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, vdp_output_surface_create);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, vdp_output_surface_destroy);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS, vdp_output_surface_get_parameters);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE, vdp_output_surface_get_bits_native);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE, vdp_output_surface_put_bits_native);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED, vdp_output_surface_put_bits_indexed);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR, vdp_output_surface_put_bits_y_cb_cr);

    GET_ADDR(VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES, vdp_bitmap_surface_query_capabilities);
    GET_ADDR(VDP_FUNC_ID_BITMAP_SURFACE_CREATE, vdp_bitmap_surface_create);
    GET_ADDR(VDP_FUNC_ID_BITMAP_SURFACE_DESTROY, vdp_bitmap_surface_destroy);
    GET_ADDR(VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS, vdp_bitmap_surface_get_parameters);
    GET_ADDR(VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE, vdp_bitmap_surface_put_bits_native);

    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE, vdp_output_surface_render_output_surface);
    GET_ADDR(VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE, vdp_output_surface_render_bitmap_surface);

    // VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA not present?

    GET_ADDR(VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, vdp_decoder_query_capabilities);
    GET_ADDR(VDP_FUNC_ID_DECODER_CREATE, vdp_decoder_create);
    GET_ADDR(VDP_FUNC_ID_DECODER_DESTROY, vdp_decoder_destroy);
    GET_ADDR(VDP_FUNC_ID_DECODER_GET_PARAMETERS, vdp_decoder_get_parameters);
    GET_ADDR(VDP_FUNC_ID_DECODER_RENDER, vdp_decoder_render);

    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT, vdp_video_mixer_query_feature_support);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT, vdp_video_mixer_query_parameter_support);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT, vdp_video_mixer_query_attribute_support);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE, vdp_video_mixer_query_parameter_value_range);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE, vdp_video_mixer_query_attribute_value_range);

    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_CREATE, vdp_video_mixer_create);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES, vdp_video_mixer_set_feature_enables);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES, vdp_video_mixer_set_attribute_values);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT, vdp_video_mixer_get_feature_support);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES, vdp_video_mixer_get_feature_enables);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES, vdp_video_mixer_get_parameter_values);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES, vdp_video_mixer_get_attribute_values);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_DESTROY, vdp_video_mixer_destroy);
    GET_ADDR(VDP_FUNC_ID_VIDEO_MIXER_RENDER, vdp_video_mixer_render);

    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY, vdp_presentation_queue_target_destroy);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE, vdp_presentation_queue_create);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY, vdp_presentation_queue_destroy);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR, vdp_presentation_queue_set_background_color);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR, vdp_presentation_queue_get_background_color);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME, vdp_presentation_queue_get_time);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY, vdp_presentation_queue_display);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE, vdp_presentation_queue_block_until_surface_idle);
    GET_ADDR(VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS, vdp_presentation_queue_query_surface_status);

    GET_ADDR(VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER, vdp_preemption_callback_register);

    return VDP_STATUS_OK;
}
