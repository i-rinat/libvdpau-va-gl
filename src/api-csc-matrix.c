#include <vdpau/vdpau.h>

VdpStatus
softVdpGenerateCSCMatrix(VdpProcamp *procamp, VdpColorStandard standard, VdpCSCMatrix *csc_matrix)
{
    if (!csc_matrix)
        return VDP_STATUS_INVALID_POINTER;
    if (procamp && VDP_PROCAMP_VERSION != procamp->struct_version)
        return VDP_STATUS_INVALID_VALUE;

    // TODO: do correct matricies calculation
    VdpCSCMatrix *m = csc_matrix;
    switch (standard) {
    case VDP_COLOR_STANDARD_ITUR_BT_601:
        (*m)[0][0] = 1.164f; (*m)[0][1] =  0.0f;   (*m)[0][2] =  1.596f; (*m)[0][3] = -222.9f;
        (*m)[1][0] = 1.164f; (*m)[1][1] = -0.392f; (*m)[1][2] = -0.813f; (*m)[1][3] =  135.6f;
        (*m)[2][0] = 1.164f; (*m)[2][1] =  2.017f; (*m)[2][2] =  0.0f;   (*m)[2][3] = -276.8f;
        break;
    case VDP_COLOR_STANDARD_ITUR_BT_709:
        (*m)[0][0] =  1.0f; (*m)[0][1] =  0.0f;   (*m)[0][2] =  1.402f; (*m)[0][3] = -179.4f;
        (*m)[1][0] =  1.0f; (*m)[1][1] = -0.344f; (*m)[1][2] = -0.714f; (*m)[1][3] =  135.5f;
        (*m)[2][0] =  1.0f; (*m)[2][1] =  1.772f; (*m)[2][2] =  0.0f;   (*m)[2][3] = -226.8f;
        break;
    case VDP_COLOR_STANDARD_SMPTE_240M:
        (*m)[0][0] =  0.581f; (*m)[0][1] = -0.764f; (*m)[0][2] =  1.576f; (*m)[0][3] = 0.0f;
        (*m)[1][0] =  0.581f; (*m)[1][1] = -0.991f; (*m)[1][2] = -0.477f; (*m)[1][3] = 0.0f;
        (*m)[2][0] =  0.581f; (*m)[2][1] =  1.062f; (*m)[2][2] =  0.000f; (*m)[2][3] = 0.0f;
        break;
    default:
        return VDP_STATUS_INVALID_COLOR_STANDARD;
    }

    return VDP_STATUS_OK;
}
