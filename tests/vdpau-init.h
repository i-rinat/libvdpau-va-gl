#ifndef __VDPAU_INIT_H
#define __VDPAU_INIT_H
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <vdpau/vdpau.h>
#include <vdpau/vdpau_x11.h>

#define ASSERT_OK(expr) \
    do { \
        VdpStatus status = expr; \
        assert (VDP_STATUS_OK == status); \
    } while (0)

extern VdpGetErrorString       *vdp_get_error_string;
extern VdpGetApiVersion        *vdp_get_api_version;
extern VdpGetInformationString *vdp_get_information_string;
extern VdpDeviceDestroy        *vdp_device_destroy;
extern VdpGenerateCSCMatrix    *vdp_generate_csc_matrix;

extern VdpVideoSurfaceQueryCapabilities                *vdp_video_surface_query_capabilities;
extern VdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities *vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities;
extern VdpVideoSurfaceCreate                           *vdp_video_surface_create;
extern VdpVideoSurfaceDestroy                          *vdp_video_surface_destroy;
extern VdpVideoSurfaceGetParameters                    *vdp_video_surface_get_parameters;
extern VdpVideoSurfaceGetBitsYCbCr                     *vdp_video_surface_get_bits_y_cb_cr;
extern VdpVideoSurfacePutBitsYCbCr                     *vdp_video_surface_put_bits_y_cb_cr;

extern VdpOutputSurfaceQueryCapabilities               *vdp_output_surface_query_capabilities;
extern VdpOutputSurfaceQueryGetPutBitsNativeCapabilities *vdp_output_surface_query_get_put_bits_native_capabilities;
extern VdpOutputSurfaceQueryPutBitsIndexedCapabilities *vdp_output_surface_query_put_bits_indexed_capabilities;
extern VdpOutputSurfaceQueryPutBitsYCbCrCapabilities   *vdp_output_surface_query_put_bits_y_cb_cr_capabilities;
extern VdpOutputSurfaceCreate                          *vdp_output_surface_create;
extern VdpOutputSurfaceDestroy                         *vdp_output_surface_destroy;
extern VdpOutputSurfaceGetParameters                   *vdp_output_surface_get_parameters;
extern VdpOutputSurfaceGetBitsNative                   *vdp_output_surface_get_bits_native;
extern VdpOutputSurfacePutBitsNative                   *vdp_output_surface_put_bits_native;
extern VdpOutputSurfacePutBitsIndexed                  *vdp_output_surface_put_bits_indexed;
extern VdpOutputSurfacePutBitsYCbCr                    *vdp_output_surface_put_bits_y_cb_cr;

extern VdpBitmapSurfaceQueryCapabilities   *vdp_bitmap_surface_query_capabilities;
extern VdpBitmapSurfaceCreate              *vdp_bitmap_surface_create;
extern VdpBitmapSurfaceDestroy             *vdp_bitmap_surface_destroy;
extern VdpBitmapSurfaceGetParameters       *vdp_bitmap_surface_get_parameters;
extern VdpBitmapSurfacePutBitsNative       *vdp_bitmap_surface_put_bits_native;

extern VdpOutputSurfaceRenderOutputSurface *vdp_output_surface_render_output_surface;
extern VdpOutputSurfaceRenderBitmapSurface *vdp_output_surface_render_bitmap_surface;

extern VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities;
extern VdpDecoderCreate            *vdp_decoder_create;
extern VdpDecoderDestroy           *vdp_decoder_destroy;
extern VdpDecoderGetParameters     *vdp_decoder_get_parameters;
extern VdpDecoderRender            *vdp_decoder_render;

extern VdpVideoMixerQueryFeatureSupport        *vdp_video_mixer_query_feature_support;
extern VdpVideoMixerQueryParameterSupport      *vdp_video_mixer_query_parameter_support;
extern VdpVideoMixerQueryAttributeSupport      *vdp_video_mixer_query_attribute_support;
extern VdpVideoMixerQueryParameterValueRange   *vdp_video_mixer_query_parameter_value_range;
extern VdpVideoMixerQueryAttributeValueRange   *vdp_video_mixer_query_attribute_value_range;

extern VdpVideoMixerCreate             *vdp_video_mixer_create;
extern VdpVideoMixerSetFeatureEnables  *vdp_video_mixer_set_feature_enables;
extern VdpVideoMixerSetAttributeValues *vdp_video_mixer_set_attribute_values;
extern VdpVideoMixerGetFeatureSupport  *vdp_video_mixer_get_feature_support;
extern VdpVideoMixerGetFeatureEnables  *vdp_video_mixer_get_feature_enables;
extern VdpVideoMixerGetParameterValues *vdp_video_mixer_get_parameter_values;
extern VdpVideoMixerGetAttributeValues *vdp_video_mixer_get_attribute_values;
extern VdpVideoMixerDestroy            *vdp_video_mixer_destroy;
extern VdpVideoMixerRender             *vdp_video_mixer_render;

extern VdpPresentationQueueTargetDestroy       *vdp_presentation_queue_target_destroy;
extern VdpPresentationQueueCreate              *vdp_presentation_queue_create;
extern VdpPresentationQueueDestroy             *vdp_presentation_queue_destroy;
extern VdpPresentationQueueSetBackgroundColor  *vdp_presentation_queue_set_background_color;
extern VdpPresentationQueueGetBackgroundColor  *vdp_presentation_queue_get_background_color;
extern VdpPresentationQueueGetTime             *vdp_presentation_queue_get_time;
extern VdpPresentationQueueDisplay             *vdp_presentation_queue_display;
extern VdpPresentationQueueBlockUntilSurfaceIdle *vdp_presentation_queue_block_until_surface_idle;
extern VdpPresentationQueueQuerySurfaceStatus  *vdp_presentation_queue_query_surface_status;

extern VdpPreemptionCallbackRegister   *vdp_preemption_callback_register;

extern VdpGetProcAddress   *vdp_get_proc_address;


VdpStatus vdpau_init_functions(VdpDevice *device);

#endif /* __VDPAU_INIT_H */
