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
#include <algorithm>
#include <sstream>

#include <archon/util/base64.hpp>
#include <archon/util/transcode.hpp>
#include <archon/web/yber_codec.hpp>


using namespace archon::core;
using namespace archon::util;


namespace {

class CodecImpl: public Codec {
public:
    std::string encode(const std::string&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::string decode(const std::string& s) const
    {
        try {
            std::ostringstream t;
            static const char d[] = { '~', '!', '$', '.', '*' };
            auto n = s.size();
            std::string::size_type i = 0;
            do {
                char c = s[i++];
                int e = base64::lookup_symbol(c);
                if (0 <= e) {
                    t << '\0' << c;
                    continue;
                }
                std::size_t f = std::find(d, d + sizeof d, c) - d;
                if (f == sizeof d)
                    throw DecodeException("Illegal character");
                if (i == n)
                    throw DecodeException("Unterminated escape sequence");
                if (f < 3) {
                    e = base64::value_from_symbol(s[i++]);
                    t << '\0' << static_cast<unsigned char>(e+(f?f<2?128:192:e<45?0:e<47?1:e<54?11:e<58?37:e<59?38:64));
                    continue;
                }
                std::string::size_type m;
                if (f<4) {
                    m = 2;
                }
                else {
                    if (i == n)
                        throw DecodeException("Unterminated UTF-16 escape sequence");
                    m = base64::value_from_symbol(s[i++]) + 3;
                }
                std::string::size_type j = (m*4u + 2u) / 3u;
                if (n < i+j)
                    throw DecodeException("Unterminated UTF-16 escape sequence");
                std::string l = base64::decode(s.substr(i, j));
                char p = l[0];
                for (std::string::size_type k = 1; k < m; ++k)
                    t << p << l[k];
                i += j;
            }
            while (i<n);
            return transcode(t.str(), transcode_UTF_16BE, transcode_UTF_8);
        }
        catch (base64::BadCharException&) {
            throw DecodeException("Found bad base-64 character");
        }
        catch (TranscodeException& e) {
            throw DecodeException("Failed to transcode UTF-16 to UTF-8: "+std::string(e.what()));
        }
        catch (TranscoderNotAvailableException& e) {
            throw std::runtime_error(e.what()); // Should never happen
        }
    }


    std::unique_ptr<OutputStream> get_enc_out_stream(OutputStream&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::unique_ptr<InputStream> get_dec_in_stream(InputStream&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::unique_ptr<InputStream> get_enc_in_stream(InputStream&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::unique_ptr<OutputStream> get_dec_out_stream(OutputStream&) const
    {
        throw std::runtime_error("Method not implemented");
    }


    std::unique_ptr<OutputStream> get_enc_out_stream(const std::shared_ptr<OutputStream>&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::unique_ptr<InputStream> get_dec_in_stream(const std::shared_ptr<InputStream>&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::unique_ptr<InputStream> get_enc_in_stream(const std::shared_ptr<InputStream>&) const
    {
        throw std::runtime_error("Method not implemented");
    }

    std::unique_ptr<OutputStream> get_dec_out_stream(const std::shared_ptr<OutputStream>&) const
    {
        throw std::runtime_error("Method not implemented");
    }
};

} // unnamed namespace


namespace archon {
namespace web {

std::unique_ptr<const Codec> get_yber_codec()
{
    return std::make_unique<CodecImpl>(); // Throws
}

} // namespace web
} // namespace archon
