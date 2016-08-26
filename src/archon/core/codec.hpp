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

#ifndef ARCHON_CORE_CODEC_HPP
#define ARCHON_CORE_CODEC_HPP

#include <cwchar>
#include <stdexcept>
#include <string>

#include <archon/core/stream.hpp>


namespace archon {
namespace core {

class CodecException;
class EncodeException;
class DecodeException;

/// Represents a specific codec, with the ability to encode and decode
/// individual strings and create encoding or decoding stream wrappers.
///
/// Thread safety: All methods must be thread safe.
template<class Ch> class BasicCodec {
public:
    using CharType = Ch;
    using StringType = std::basic_string<CharType>;
    using InputStreamType = BasicInputStream<CharType>;
    using OutputStreamType = BasicOutputStream<CharType>;

    /// Encode the specified string to a string of bytes.
    ///
    /// \throw EncodeException When the string could not be encoded.
    virtual std::string encode(const StringType&) const = 0;

    /// Decode the specified byte string string.
    ///
    /// \throw DecodeException When the string could not be encoded.
    virtual StringType decode(const std::string&) const = 0;

    /// Create a character output stream that accepts un-encoded characters, and
    /// writes the encoded characters to the specified byte output stream.
    virtual std::unique_ptr<OutputStreamType> get_enc_out_stream(OutputStream&) const = 0;

    /// Create a character input stream that returns unencoded characters, and
    /// reads the encoded characters from the specified byte input stream.
    virtual std::unique_ptr<InputStreamType> get_dec_in_stream(InputStream&) const = 0;

    /// Create a byte input stream that returns encoded characters, and reads
    /// the un-encoded characters from the specified character input stream.
    virtual std::unique_ptr<InputStream> get_enc_in_stream(InputStreamType&) const = 0;

    /// Create a byte output stream that accepts encoded characters, and writes
    /// the un-encoded characters to the specified character output stream.
    virtual std::unique_ptr<OutputStream> get_dec_out_stream(OutputStreamType&) const = 0;


    virtual std::unique_ptr<OutputStreamType>
    get_enc_out_stream(const std::shared_ptr<OutputStream>&) const = 0;

    virtual std::unique_ptr<InputStreamType>
    get_dec_in_stream(const std::shared_ptr<InputStream>&) const = 0;

    virtual std::unique_ptr<InputStream>
    get_enc_in_stream(const std::shared_ptr<InputStreamType>&) const = 0;

    virtual std::unique_ptr<OutputStream>
    get_dec_out_stream(const std::shared_ptr<OutputStreamType>&) const = 0;

    virtual ~BasicCodec() {}
};


using Codec = BasicCodec<char>;
using WideCodec = BasicCodec<wchar_t>;




class CodecException: public std::runtime_error {
public:
    CodecException(const std::string& message):
        std::runtime_error(message)
    {
    }
};

class EncodeException: public CodecException {
public:
    EncodeException(const std::string& message):
        CodecException(message)
    {
    }
};

class DecodeException: public CodecException {
public:
    DecodeException(const std::string& message):
        CodecException(message)
    {
    }
};

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_CODEC_HPP
