/*
 * Copyright 2013-2016  Rinat Ibragimov
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

#include "api-bitmap-surface.hh"
#include "api-decoder.hh"
#include "api-output-surface.hh"
#include "api-presentation-queue.hh"
#include "api-video-mixer.hh"
#include "api-video-surface.hh"
#include "api.hh"
#include "handle-storage.hh"
#include <atomic>


namespace {

struct {
    vdp::ResourceStorage<vdp::BitmapSurface::Resource>  bitmap_surface;
    vdp::ResourceStorage<vdp::Device::Resource>         device;

    vdp::ResourceStorage<vdp::OutputSurface::Resource>           output_surface;
    vdp::ResourceStorage<vdp::PresentationQueue::Resource>       presentation_queue;
    vdp::ResourceStorage<vdp::PresentationQueue::TargetResource> presentation_queue_target;

    vdp::ResourceStorage<vdp::Decoder::Resource>      video_decoder;
    vdp::ResourceStorage<vdp::VideoMixer::Resource>   video_mixer;
    vdp::ResourceStorage<vdp::VideoSurface::Resource> video_surface;
} storage;

} // anonymous namespace


namespace vdp {

uint32_t
get_resource_id()
{
    static std::atomic<uint32_t> id{300000};
    return ++id;
}

template<>
vdp::ResourceStorage<vdp::BitmapSurface::Resource> &
vdp::ResourceStorage<vdp::BitmapSurface::Resource>::instance()
{
    return storage.bitmap_surface;
}

template<>
vdp::ResourceStorage<vdp::Device::Resource> &
vdp::ResourceStorage<vdp::Device::Resource>::instance()
{
    return storage.device;
}

template<>
vdp::ResourceStorage<vdp::OutputSurface::Resource> &
vdp::ResourceStorage<vdp::OutputSurface::Resource>::instance()
{
    return storage.output_surface;
}

template<>
vdp::ResourceStorage<vdp::PresentationQueue::Resource> &
vdp::ResourceStorage<vdp::PresentationQueue::Resource>::instance()
{
    return storage.presentation_queue;
}

template<>
vdp::ResourceStorage<vdp::PresentationQueue::TargetResource> &
vdp::ResourceStorage<vdp::PresentationQueue::TargetResource>::instance()
{
    return storage.presentation_queue_target;
}

template<>
vdp::ResourceStorage<vdp::Decoder::Resource> &
vdp::ResourceStorage<vdp::Decoder::Resource>::instance()
{
    return storage.video_decoder;
}

template<>
vdp::ResourceStorage<vdp::VideoMixer::Resource> &
vdp::ResourceStorage<vdp::VideoMixer::Resource>::instance()
{
    return storage.video_mixer;
}

template<>
vdp::ResourceStorage<vdp::VideoSurface::Resource> &
vdp::ResourceStorage<vdp::VideoSurface::Resource>::instance()
{
    return storage.video_surface;
}

} // namespace vdp
