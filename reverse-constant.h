#pragma once
#ifndef REVERSE_CONSTANT_H_
#define REVERSE_CONSTANT_H_

#include <vdpau/vdpau.h>

const char * reverse_func_id(VdpFuncId func_id);
const char * reverse_video_mixer_feature(VdpVideoMixerFeature mixer_feature);
const char * reverse_video_mixer_attributes(VdpVideoMixerAttribute attr);

#endif /* REVERSE_CONSTANT_H_ */
