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

#pragma once

#include "api-device.hh"
#include "exceptions.hh"
#include <map>
#include <memory>
#include <mutex>
#include <unistd.h>
#include <vdpau/vdpau_x11.h>
#include <vector>


namespace vdp {

template<class callable, typename... Args>
VdpStatus
check_for_exceptions(callable fwd, Args... args)
{
    try {
        return fwd(args...);

    } catch (const std::bad_alloc &) {
        return VDP_STATUS_RESOURCES;

    } catch (const vdp::resource_not_found &) {
        return VDP_STATUS_INVALID_HANDLE;

    } catch (const vdp::generic_error &) {
        return VDP_STATUS_ERROR;

    } catch (const vdp::invalid_size &) {
        return VDP_STATUS_INVALID_SIZE;

    } catch (const vdp::invalid_rgba_format &) {
        return VDP_STATUS_INVALID_RGBA_FORMAT;

    } catch (const vdp::invalid_decoder_profile &) {
        return VDP_STATUS_INVALID_DECODER_PROFILE;

    } catch (const vdp::invalid_chroma_type &) {
        return VDP_STATUS_INVALID_CHROMA_TYPE;

    } catch (...) {
        return VDP_STATUS_ERROR;
    }
}

// TODO: what to do when it flips over uint32_t limit?
uint32_t
get_resource_id();

template <class T>
class ResourceStorage
{
public:
    uint32_t
    insert(std::shared_ptr<T> res)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        auto id = get_resource_id();
        res->id = id;
        map_.insert(std::make_pair(id, res));
        return id;
    }

    std::shared_ptr<T>
    find(uint32_t handle)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);

        auto res = map_.find(handle);
        if (res == map_.end())
            throw vdp::resource_not_found();

        return res->second;
    }

    void
    drop(uint32_t handle)
    {
        std::unique_lock<decltype(mtx_)> lock(mtx_);
        map_.erase(handle);
    }

    std::vector<uint32_t>
    enumerate()
    {
        std::vector<uint32_t> v;
        for (const auto &it: map_)
            v.push_back(it.first);
        return v;
    }

    static ResourceStorage<T> &
    instance();

private:
    std::recursive_mutex                    mtx_;
    std::map<uint32_t, std::shared_ptr<T>>  map_;
};

template<class T>
class ResourceRef
{
public:
    explicit ResourceRef(uint32_t handle)
    {
        auto &storage = ResourceStorage<T>::instance();
        while (true) {
            auto res = storage.find(handle);
            if (res->mtx.try_lock()) {
                res_ = res;
                return;
            }
            usleep(1);
        }
    }

    ~ResourceRef()
    {
        res_->mtx.unlock();
    }

    ResourceRef &
    operator=(const ResourceRef &) = delete;

    std::shared_ptr<T>
    get_ref() const { return res_; }

    T *
    operator->() const { return res_.get(); }

    operator std::shared_ptr<T>() const { return res_; }

private:
    std::shared_ptr<T> res_;
};

} // namespace vdp
