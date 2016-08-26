/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#include <stdexcept>

#include <archon/util/codec.hpp>


using namespace archon::core;

namespace {

class BlockDecodeInputStream: public InputStream {
public:
    std::size_t read(char_type* b, std::size_t n) override
    {
        if (!n || eoi)
            return 0;

        std::size_t n_orig = n;
        for (;;) {
            // Transfer remaining bytes of current block
            while (left) {
                std::size_t m = std::min(n, left);
                std::size_t r = in.read(b, m);
                if (!r)
                    throw ReadException("BlockDecodeInputStream: Premature end of input");
                left -= r;
                n -= r;
                if (!n)
                    return n_orig;
                if (b)
                    b += r;
            }

            // Get size of next block
            {
                char c;
                std::size_t r = in.read(&c, 1);
                if (!r)
                    throw ReadException("BlockDecodeInputStream: Premature end of input");
                if (!c) {
                    eoi = true;
                    return n_orig - n;
                }
                left = static_cast<unsigned char>(c);
            }
        }
    }

    BlockDecodeInputStream(InputStream& i, const std::shared_ptr<InputStream>& owner):
        in(i),
        in_owner(owner)
    {
    }

    InputStream &in;
    const std::shared_ptr<InputStream> in_owner;

    std::size_t left = 0;
    bool eoi = false;
};


class BlockEncodeOutputStream: public OutputStream {
public:
    void write(const char* b, std::size_t n) override
    {
        if (!chunk)
            throw WriteException("Write after flush is not supported");
        for (;;) {
            // Transfer callers data to buffer
            std::size_t m = std::min(n, left);
            const char* e = b + m;
            std::copy(b, e, s_chunk_size - left + chunk);
            left -= m;
            n -= m;
            if (!n)
                return;
            b = e;

            // Close the filled chunk, and go to next
            chunk[0] = '\xFF';
            chunk += s_chunk_size;
            left = s_chunk_size-1;

            // Write to wrapped stream if buffer is full
            if (chunk == buffer + sizeof buffer) {
                out.write(buffer, sizeof buffer);
                chunk = buffer;
            }
        }
    }

    void flush() override
    {
        if (!chunk)
            return;
        flush2();
        buffer[0] = '\0';
        out.write(buffer, 1);
        chunk = 0; // Force a fail on next write
    }

    void flush2()
    {
        std::size_t n = s_chunk_size-left-1;
        // If n is zero, the buffer is empty
        if (n != 0) {
            chunk[0] = static_cast<unsigned char>(n);
            out.write(buffer, chunk+(256-left) - buffer);
            left = s_chunk_size-1;
            chunk = buffer;
        }
    }

    BlockEncodeOutputStream(OutputStream& o, const std::shared_ptr<OutputStream>& owner):
        out(o),
        out_owner(owner)
    {
    }

    ~BlockEncodeOutputStream()
    {
        try {
            flush();
        }
        catch (...) {
        }
    }

    static constexpr std::size_t s_chunk_size = 256; // Must never exceed 256

    OutputStream& out;
    const std::shared_ptr<OutputStream> out_owner;

    char buffer[4*s_chunk_size]; // Must be an integer multiple of chunk_size
    char* chunk = buffer;
    std::size_t left = s_chunk_size - 1;
};


class BlockCodec: public Codec {
public:
    std::string encode(const std::string&) const
    {
        throw std::runtime_error("Not implemented");
    }

    std::string decode(const std::string&) const
    {
        throw std::runtime_error("Not implemented");
    }

    std::unique_ptr<OutputStream> get_enc_out_stream(OutputStream& out) const
    {
        return std::make_unique<BlockEncodeOutputStream>(out, nullptr); // Throws
    }

    std::unique_ptr<InputStream> get_dec_in_stream(InputStream& in) const
    {
        return std::make_unique<BlockDecodeInputStream>(in, nullptr); // Throws
    }

    std::unique_ptr<InputStream> get_enc_in_stream(InputStream&) const
    {
        throw std::runtime_error("Not implemented");
    }

    std::unique_ptr<OutputStream> get_dec_out_stream(OutputStream&) const
    {
        throw std::runtime_error("Not implemented");
    }

    std::unique_ptr<OutputStream> get_enc_out_stream(const std::shared_ptr<OutputStream>& out) const
    {
        return std::make_unique<BlockEncodeOutputStream>(*out, out); // Throws
    }

    std::unique_ptr<InputStream> get_dec_in_stream(const std::shared_ptr<InputStream>& in) const
    {
        return std::make_unique<BlockDecodeInputStream>(*in, in); // Throws
    }

    std::unique_ptr<InputStream> get_enc_in_stream(const std::shared_ptr<InputStream>&) const
    {
        throw std::runtime_error("Not implemented");
    }

    std::unique_ptr<OutputStream> get_dec_out_stream(const std::shared_ptr<OutputStream>&) const
    {
        throw std::runtime_error("Not implemented");
    }
};

} // unnamed namespace


namespace archon {
namespace util {

std::unique_ptr<const Codec> get_block_codec()
{
    return std::make_unique<BlockCodec>();
}

} // namespace util
} // namespace archon
