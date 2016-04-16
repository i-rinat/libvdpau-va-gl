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

#include <exception>


namespace vdp {

class resource_not_found:   public std::exception {};   // VDP_STATUS_INVALID_HANDLE
class generic_error:        public std::exception {};   // VDP_STATUS_ERROR
class invalid_size:         public std::exception {};   // VDP_STATUS_INVALID_SIZE
class invalid_rgba_format:  public std::exception {};   // VDP_STATUS_INVALID_RGBA_FORMAT
class invalid_decoder_profile: public std::exception {};// VDP_INVALID_DECODER_PROFILE
class invalid_chroma_type:  public std::exception {};   // VDP_INVALID_CHROMA_TYPE

} // namespace vdp
