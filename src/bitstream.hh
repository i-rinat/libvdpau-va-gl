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

#include <stdint.h>
#include <unistd.h>
#include <vector>
#include <stdexcept>


namespace vdp {

/// Raw byte sequence payload state
///
/// throws ByteReader::error()

class RBSPState
{
public:
    class error: public std::logic_error
    {
    public:
        error(const char *descr)
            : std::logic_error(descr)
        {}
    };

private:
    /// encapsulates handling of emulation prevention bytes (EPB)
    class ByteReader
    {
    public:
        ByteReader(const std::vector<uint8_t> &buffer)
            : data_{buffer}
            , byte_ofs_{0}
            , zeros_in_row_{0}
        {}

        ByteReader(const ByteReader &other)
            : data_{other.data_}
            , byte_ofs_{other.byte_ofs_}
            , zeros_in_row_{other.zeros_in_row_}
        {}

        uint8_t
        get_byte()
        {
            if (byte_ofs_ >= data_.size())
                throw error("ByteReader: trying to read beyond bounds");

            const uint8_t current_byte = data_[byte_ofs_ ++];

            if (zeros_in_row_ >= 2 && current_byte == 3) {
                if (byte_ofs_ >= data_.size())
                    throw error("ByteReader: trying to read beyond bounds");

                const uint8_t another_byte = data_[byte_ofs_ ++];
                zeros_in_row_ = (another_byte == 0) ? 1 : 0;

                return another_byte;
            }

            if (current_byte == 0) {
                zeros_in_row_ += 1;
            } else {
                zeros_in_row_ = 0;
            }

            return current_byte;
        }

        size_t
        get_ofs() const
        {
            return byte_ofs_;
        }

        /// rewind to the next NAL unit begin marker (0x00 0x00 0x01)
        int64_t
        navigate_to_nal_unit()
        {
            const size_t prev_ofs = byte_ofs_;

            uint32_t window = ~0u;
            do {
                if (byte_ofs_ >= data_.size())
                    throw error("ByteReader: no more bytes");

                const uint32_t c = data_[byte_ofs_++];
                window = (window << 8) | c;

            } while ((window & 0xffffff) != 0x000001);

            return byte_ofs_ - prev_ofs;
        }

    private:
        ByteReader &
        operator=(const ByteReader &) = delete;

        const std::vector<uint8_t>  &data_;
        size_t byte_ofs_;
        size_t zeros_in_row_;
    };

public:
    RBSPState(const std::vector<uint8_t> &buffer)
        : byte_reader_{buffer}
        , bits_eaten_{0}
        , current_byte_{0}
        , bit_ofs_{7}
    {}

    ~RBSPState() = default;

    RBSPState(const RBSPState &other)
        : byte_reader_{other.byte_reader_}
        , bits_eaten_{other.bits_eaten_}
        , current_byte_{other.current_byte_}
        , bit_ofs_{other.bit_ofs_}
    {}

    int64_t
    navigate_to_nal_unit()
    {
        // reset bit position to ensure next read will fetch a fresh byte from byte_reader_
        bit_ofs_ = 7;

        return byte_reader_.navigate_to_nal_unit();
    }

    void
    reset_bit_counter()
    {
        bits_eaten_ = 0;
    }

    size_t
    bits_eaten() const
    {
        return bits_eaten_;
    }

    uint32_t
    get_u(size_t bitcount)
    {
        uint32_t res = 0;

        for (size_t k = 0; k < bitcount; k ++)
            res = (res << 1) + get_bit();

        return res;
    }

    uint32_t
    get_uev()
    {
        size_t zeros = 0;

        while (get_bit() == 0)
            zeros ++;

        if (zeros == 0)
            return 0;

        return (1 << zeros) - 1 + get_u(zeros);
    }

    int32_t
    get_sev()
    {
        size_t zeros = 0;

        while (get_bit() == 0)
            zeros ++;

        if (zeros == 0)
            return 0;

        const int32_t val = (1 << zeros) + get_u(zeros);

        if (val & 1)
            return -(val / 2);

        return val / 2;
    }

private:
    RBSPState &
    operator=(const RBSPState &) = delete;

    uint32_t
    get_bit()
    {
        if (bit_ofs_ == 7)
            current_byte_ = byte_reader_.get_byte();

        const uint32_t val = (current_byte_ >> bit_ofs_) & 1;

        if (bit_ofs_ > 0) {
            bit_ofs_ -= 1;
        } else {
            bit_ofs_ = 7; // most significant bit in a 8-bit byte
        }

        bits_eaten_ += 1;

        return val;
    }

    ByteReader  byte_reader_;
    size_t      bits_eaten_;
    uint8_t     current_byte_;
    uint8_t     bit_ofs_;
};

} // namespace vdp
