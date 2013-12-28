#define GL_GLEXT_PROTOTYPES
#include "ctx-stack.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "globals.h"
#include "vdpau-soft.h"
#include "shaders.h"
#include <stdlib.h>
#include "trace.h"
#include <va/va_glx.h>
#include <vdpau/vdpau.h>
#include "watermark.h"


static char const *
implemetation_description_string = "OpenGL/VAAPI/libswscale backend for VDPAU";


void
print_handle_type(int handle, void *item, void *p)
{
    VdpGenericHandle *gh = item;
    struct {
        int cnt;
        int total_cnt;
        VdpDeviceData *deviceData;
    } *pp = p;
    pp->total_cnt ++;

    if (gh) {
        if (pp->deviceData == gh->parent) {
            traceError("handle %d type = %d\n", handle, gh->type);
            pp->cnt ++;
        }
    }
}

static
void
destroy_child_objects(int handle, void *item, void *p)
{
    const void *parent = p;
    VdpGenericHandle *gh = item;
    if (gh) {
        if (parent == gh->parent) {
            switch (gh->type) {
            case HANDLETYPE_DEVICE:
                // do nothing
                break;
            case HANDLETYPE_PRESENTATION_QUEUE_TARGET:
                softVdpPresentationQueueDestroy(handle);
                break;
            case HANDLETYPE_PRESENTATION_QUEUE:
                softVdpPresentationQueueDestroy(handle);
                break;
            case HANDLETYPE_VIDEO_MIXER:
                softVdpVideoMixerDestroy(handle);
                break;
            case HANDLETYPE_OUTPUT_SURFACE:
                softVdpOutputSurfaceDestroy(handle);
                break;
            case HANDLETYPE_VIDEO_SURFACE:
                softVdpVideoSurfaceDestroy(handle);
                break;
            case HANDLETYPE_BITMAP_SURFACE:
                softVdpBitmapSurfaceDestroy(handle);
                break;
            case HANDLETYPE_DECODER:
                softVdpDecoderDestroy(handle);
                break;
            default:
                traceError("warning (destroy_child_objects): unknown handle type %d\n", gh->type);
                break;
            }
        }
    }
}

static
VdpStatus
compile_shaders(void)
{
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    const int shader_count = sizeof(glsl_shaders)/sizeof(glsl_shaders[0]);
    VdpStatus retval = VDP_STATUS_ERROR;

    pthread_mutex_lock(&lock);
    for (int k = 0; k < shader_count; k ++) {
        struct shader_s *s = &glsl_shaders[k];
        if (!s->program) {
            GLint errmsg_len;
            int ok;

            s->f_shader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(s->f_shader, 1, &s->body, &s->len);
            glCompileShader(s->f_shader);
            glGetShaderiv(s->f_shader, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                glGetShaderiv(s->f_shader, GL_INFO_LOG_LENGTH, &errmsg_len);
                char *errmsg = malloc(errmsg_len);
                glGetShaderInfoLog(s->f_shader, errmsg_len, NULL, errmsg);
                traceError("error (%s): compilation of shader #%d failed with '%s'\n", __func__, k,
                           errmsg);
                free(errmsg);
                glDeleteShader(s->f_shader);
                goto err;
            }

            s->program = glCreateProgram();
            glAttachShader(s->program, s->f_shader);
            glLinkProgram(s->program);
            glGetProgramiv(s->program, GL_LINK_STATUS, &ok);
            if (!ok) {
                glGetProgramiv(s->program, GL_INFO_LOG_LENGTH, &errmsg_len);
                char *errmsg = malloc(errmsg_len);
                glGetProgramInfoLog(s->program, errmsg_len, NULL, errmsg);
                traceError("error (%s): linking of shader #%d failed with '%s'\n", __func__, k,
                           errmsg);
                free(errmsg);
                glDeleteProgram(s->program);
                glDeleteShader(s->f_shader);
                goto err;
            }

            switch (k) {
            case glsl_YV12_RGBA:
            case glsl_NV12_RGBA:
                s->uniform.tex_0 = glGetUniformLocation(s->program, "tex[0]");
                s->uniform.tex_1 = glGetUniformLocation(s->program, "tex[1]");
                break;
            default:
                /* nothing */
                break;
            }
        }
    }

    retval = VDP_STATUS_OK;
err:
    pthread_mutex_unlock(&lock);
    return retval;
}

VdpStatus
softVdpDeviceCreateX11(Display *display_orig, int screen, VdpDevice *device,
                       VdpGetProcAddress **get_proc_address)
{
    if (!display_orig || !device || !get_proc_address)
        return VDP_STATUS_INVALID_POINTER;

    // Let's get own connection to the X server
    Display *display = handle_xdpy_ref(display_orig);
    if (NULL == display)
        return VDP_STATUS_ERROR;

    if (global.quirks.buggy_XCloseDisplay) {
        // XCloseDisplay could segfault on fglrx. To avoid calling XCloseDisplay,
        // make one more reference to xdpy copy.
        handle_xdpy_ref(display_orig);
    }

    VdpDeviceData *data = calloc(1, sizeof(VdpDeviceData));
    if (NULL == data)
        return VDP_STATUS_RESOURCES;

    data->type = HANDLETYPE_DEVICE;
    data->display = display;
    data->display_orig = display_orig;   // save supplied pointer too
    data->screen = screen;
    data->refcount = 0;
    data->root = DefaultRootWindow(display);

    // create master GLX context to share data between further created ones
    glx_context_ref_glc_hash_table(display, screen);
    data->root_glc = glx_context_get_root_context();

    glx_context_push_thread_local(data);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // initialize VAAPI
    if (global.quirks.avoid_va) {
        // pretend there is no VA-API available
        data->va_available = 0;
    } else {
        data->va_dpy = vaGetDisplayGLX(display);
        data->va_available = 0;

        VAStatus status = vaInitialize(data->va_dpy, &data->va_version_major,
                                       &data->va_version_minor);
        if (VA_STATUS_SUCCESS == status) {
            data->va_available = 1;
            traceInfo("libva (version %d.%d) library initialized\n",
                      data->va_version_major, data->va_version_minor);
        } else {
            data->va_available = 0;
            traceInfo("warning: failed to initialize libva. "
                      "No video decode acceleration available.\n");
        }
    }

    compile_shaders();

    glGenTextures(1, &data->watermark_tex_id);
    glBindTexture(GL_TEXTURE_2D, data->watermark_tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, watermark_width, watermark_height, 0, GL_RED,
                 GL_UNSIGNED_BYTE, watermark_data);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFinish();

    *device = handle_insert(data);
    *get_proc_address = &softVdpGetProcAddress;

    GLenum gl_error = glGetError();
    glx_context_pop();

    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpDeviceCreateX11): gl error %d\n", gl_error);
        return VDP_STATUS_ERROR;
    }

    return VDP_STATUS_OK;
}

VdpStatus
softVdpDeviceDestroy(VdpDevice device)
{
    VdpStatus err_code;
    VdpDeviceData *data = handle_acquire(device, HANDLETYPE_DEVICE);
    if (NULL == data)
        return VDP_STATUS_INVALID_HANDLE;

    if (0 != data->refcount) {
        // Buggy client forgot to destroy dependend objects or decided that destroying
        // VdpDevice destroys all child object. Let's try to mitigate and prevent leakage.
        traceError("warning (softVdpDeviceDestroy): non-zero reference count (%d). "
                   "Trying to free child objects.\n", data->refcount);
        void *parent_object = data;
        handle_execute_for_all(destroy_child_objects, parent_object);
    }

    if (0 != data->refcount) {
        traceError("error (softVdpDeviceDestroy): still non-zero reference count (%d)\n",
                   data->refcount);
        traceError("Here is the list of objects:\n");
        struct {
            int cnt;
            int total_cnt;
            VdpDeviceData *deviceData;
        } state = { .cnt = 0, .total_cnt = 0, .deviceData = data };

        handle_execute_for_all(print_handle_type, &state);
        traceError("Objects leaked: %d\n", state.cnt);
        traceError("Objects visited during scan: %d\n", state.total_cnt);
        err_code = VDP_STATUS_ERROR;
        goto quit;
    }

    // cleaup libva
    if (data->va_available)
        vaTerminate(data->va_dpy);

    glx_context_push_thread_local(data);
    glDeleteTextures(1, &data->watermark_tex_id);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glx_context_pop();

    pthread_mutex_lock(&global.glx_ctx_stack_mutex);
    glXMakeCurrent(data->display, None, NULL);
    pthread_mutex_unlock(&global.glx_ctx_stack_mutex);

    glx_context_unref_glc_hash_table(data->display);

    handle_xdpy_unref(data->display_orig);
    handle_expunge(device);
    free(data);

    GLenum gl_error = glGetError();
    if (GL_NO_ERROR != gl_error) {
        traceError("error (VdpDeviceDestroy): gl error %d\n", gl_error);
        err_code = VDP_STATUS_ERROR;
        goto quit_skip_release;
    }

    return VDP_STATUS_OK;

quit:
    handle_release(device);
quit_skip_release:
    return err_code;
}

VdpStatus
softVdpGetApiVersion(uint32_t *api_version)
{
    if (!api_version)
        return VDP_STATUS_INVALID_POINTER;
    *api_version = VDPAU_VERSION;
    return VDP_STATUS_OK;
}

static
const char *
softVdpGetErrorString(VdpStatus status)
{
    return reverse_status(status);
}

VdpStatus
softVdpGetInformationString(char const **information_string)
{
    if (!information_string)
        return VDP_STATUS_INVALID_POINTER;
    *information_string = implemetation_description_string;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpGetProcAddress(VdpDevice device, VdpFuncId function_id, void **function_pointer)
{
    (void)device;   // there is no difference between various devices. All have same procedures
    if (!function_pointer)
        return VDP_STATUS_INVALID_POINTER;
    switch (function_id) {
    case VDP_FUNC_ID_GET_ERROR_STRING:
        *function_pointer = &softVdpGetErrorString;
        break;
    case VDP_FUNC_ID_GET_PROC_ADDRESS:
        *function_pointer = &softVdpGetProcAddress;
        break;
    case VDP_FUNC_ID_GET_API_VERSION:
        *function_pointer = &traceVdpGetApiVersion;
        break;
    case VDP_FUNC_ID_GET_INFORMATION_STRING:
        *function_pointer = &traceVdpGetInformationString;
        break;
    case VDP_FUNC_ID_DEVICE_DESTROY:
        *function_pointer = &traceVdpDeviceDestroy;
        break;
    case VDP_FUNC_ID_GENERATE_CSC_MATRIX:
        *function_pointer = &traceVdpGenerateCSCMatrix;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpVideoSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &traceVdpVideoSurfaceQueryGetPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_CREATE:
        *function_pointer = &traceVdpVideoSurfaceCreate;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_DESTROY:
        *function_pointer = &traceVdpVideoSurfaceDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS:
        *function_pointer = &traceVdpVideoSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR:
        *function_pointer = &traceVdpVideoSurfaceGetBitsYCbCr;
        break;
    case VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &traceVdpVideoSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryGetPutBitsNativeCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryPutBitsIndexedCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES:
        *function_pointer = &traceVdpOutputSurfaceQueryPutBitsYCbCrCapabilities;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_CREATE:
        *function_pointer = &traceVdpOutputSurfaceCreate;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY:
        *function_pointer = &traceVdpOutputSurfaceDestroy;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS:
        *function_pointer = &traceVdpOutputSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE:
        *function_pointer = &traceVdpOutputSurfaceGetBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &traceVdpOutputSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED:
        *function_pointer = &traceVdpOutputSurfacePutBitsIndexed;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR:
        *function_pointer = &traceVdpOutputSurfacePutBitsYCbCr;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpBitmapSurfaceQueryCapabilities;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_CREATE:
        *function_pointer = &traceVdpBitmapSurfaceCreate;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_DESTROY:
        *function_pointer = &traceVdpBitmapSurfaceDestroy;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS:
        *function_pointer = &traceVdpBitmapSurfaceGetParameters;
        break;
    case VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE:
        *function_pointer = &traceVdpBitmapSurfacePutBitsNative;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE:
        *function_pointer = &traceVdpOutputSurfaceRenderOutputSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE:
        *function_pointer = &traceVdpOutputSurfaceRenderBitmapSurface;
        break;
    case VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_VIDEO_SURFACE_LUMA:
        // *function_pointer = &traceVdpOutputSurfaceRenderVideoSurfaceLuma;
        *function_pointer = NULL;
        break;
    case VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES:
        *function_pointer = &traceVdpDecoderQueryCapabilities;
        break;
    case VDP_FUNC_ID_DECODER_CREATE:
        *function_pointer = &traceVdpDecoderCreate;
        break;
    case VDP_FUNC_ID_DECODER_DESTROY:
        *function_pointer = &traceVdpDecoderDestroy;
        break;
    case VDP_FUNC_ID_DECODER_GET_PARAMETERS:
        *function_pointer = &traceVdpDecoderGetParameters;
        break;
    case VDP_FUNC_ID_DECODER_RENDER:
        *function_pointer = &traceVdpDecoderRender;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT:
        *function_pointer = &traceVdpVideoMixerQueryFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT:
        *function_pointer = &traceVdpVideoMixerQueryParameterSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT:
        *function_pointer = &traceVdpVideoMixerQueryAttributeSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE:
        *function_pointer = &traceVdpVideoMixerQueryParameterValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE:
        *function_pointer = &traceVdpVideoMixerQueryAttributeValueRange;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_CREATE:
        *function_pointer = &traceVdpVideoMixerCreate;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES:
        *function_pointer = &traceVdpVideoMixerSetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES:
        *function_pointer = &traceVdpVideoMixerSetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT:
        *function_pointer = &traceVdpVideoMixerGetFeatureSupport;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES:
        *function_pointer = &traceVdpVideoMixerGetFeatureEnables;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES:
        *function_pointer = &traceVdpVideoMixerGetParameterValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES:
        *function_pointer = &traceVdpVideoMixerGetAttributeValues;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_DESTROY:
        *function_pointer = &traceVdpVideoMixerDestroy;
        break;
    case VDP_FUNC_ID_VIDEO_MIXER_RENDER:
        *function_pointer = &traceVdpVideoMixerRender;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY:
        *function_pointer = &traceVdpPresentationQueueTargetDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE:
        *function_pointer = &traceVdpPresentationQueueCreate;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY:
        *function_pointer = &traceVdpPresentationQueueDestroy;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR:
        *function_pointer = &traceVdpPresentationQueueSetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR:
        *function_pointer = &traceVdpPresentationQueueGetBackgroundColor;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME:
        *function_pointer = &traceVdpPresentationQueueGetTime;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY:
        *function_pointer = &traceVdpPresentationQueueDisplay;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE:
        *function_pointer = &traceVdpPresentationQueueBlockUntilSurfaceIdle;
        break;
    case VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS:
        *function_pointer = &traceVdpPresentationQueueQuerySurfaceStatus;
        break;
    case VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER:
        *function_pointer = &traceVdpPreemptionCallbackRegister;
        break;
    case VDP_FUNC_ID_BASE_WINSYS:
        *function_pointer = &traceVdpPresentationQueueTargetCreateX11;
        break;
    default:
        *function_pointer = NULL;
        break;
    } // switch

    if (NULL == *function_pointer)
        return VDP_STATUS_INVALID_FUNC_ID;
    return VDP_STATUS_OK;
}

VdpStatus
softVdpPreemptionCallbackRegister(VdpDevice device, VdpPreemptionCallback callback, void *context)
{
    (void)device; (void)callback; (void)context;
    return VDP_STATUS_OK;
}
