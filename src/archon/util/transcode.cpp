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
///
/// \todo FIXME: Must use Autoconf to check if \c iconv is available.

#include <iconv.h>
#include <cerrno>
#include <cstring>
#include <limits>
#include <algorithm>

#include <archon/core/sys.hpp>
#include <archon/core/text.hpp>
#include <archon/core/memory.hpp>
#include <archon/util/inc_conv.hpp>
#include <archon/util/transcode.hpp>


using namespace archon::core;
using namespace archon::util;

namespace {

/// It is crucial that neither \c SourceChar not \c TargetChar is set to
/// anything else than \c char unless it can be guranteed that the character
/// encoding uses exactly the same number of bytes per characters as the size of
/// the corresponding character type.
template<class SourceChar, class TargetChar> class TranscoderImpl {
public:
    using source_char_type = SourceChar;
    using target_char_type = TargetChar;

    // We assume that no character requires more than 64 bytes in any encoding.
    static constexpr int min_source_buffer_size =
        (64 + sizeof (SourceChar) - 1) / sizeof (SourceChar);

    static constexpr int min_target_buffer_size =
        (64 + sizeof (TargetChar) - 1) / sizeof (TargetChar);

    class State;

    TranscoderImpl(std::string source_encoding, std::string target_encoding, bool fail):
        source_encoding(source_encoding),
        target_encoding(target_encoding),
        fail(fail)
    {
    }

    const std::string source_encoding, target_encoding;
    const bool fail;
};



// FIXME: The core stuff that is not concerned with character types is the bulk
// of what happens here, it it should therefore be moved into a nontemplate base
// class.
template<class SourceChar, class TargetChar>
class TranscoderImpl<SourceChar, TargetChar>::State:
        public BasicTranscoder<SourceChar, TargetChar> {
public:
    bool conv(const SourceChar*& in, const SourceChar* in_end,
              TargetChar*& out, TargetChar* out_end, bool eoi)
    {
        const char* in_begin = reinterpret_cast<const char*>(in);
        char* out_begin = reinterpret_cast<char*>(out);

        bool r = byte_conv(in_begin, reinterpret_cast<const char*>(in_end),
                           out_begin, reinterpret_cast<char*>(out_end), eoi);

        in = reinterpret_cast<const SourceChar*>(in_begin);
        out = reinterpret_cast<TargetChar*>(out_begin);

        return r;
    }


    // Implementing BasicTranscoder::transcode()
    bool transcode(const SourceChar*& in, const SourceChar* in_end,
                   TargetChar*& out, TargetChar* out_end, bool eoi) override
    {
        return conv(in, in_end, out, out_end, eoi);
    }


    /// \todo Currently there will be one replacement character per byte of
    /// invalid input. this is suboptimal. An achievable improvement will be to
    /// distinguish between "invalid input" and "unrepresentible in output". For
    /// invalid input we should collapse any number of consecutive invalid bytes
    /// into a single replacement character. For "unrepresentible in output" we
    /// should substitute individual input characters with replacement
    /// characters. The propper number of input bytes can be eaten by a
    /// transcoder to UCS-4 by limiting the output buffer to 4 bytes. This can
    /// also be used for discriminating between the two types of error.
    ///
    /// \todo An even better scheme for handling invalid input would be to
    /// utilize the fact that UTF-8 input octets where the most significant bit
    /// is set and the next most significant bit is cleared should be skipped
    /// when synchronizing to the start of a character. Similarly for UTF-16
    /// where surrogate elements should be skipped. However, this will only work
    /// for these special cases.
    bool byte_conv(const char*& in, const char* in_end,
                   char*& out, char* out_end, bool eoi)
    {
        std::size_t inbytes = in_end - in, outbytes = out_end - out;
        for (;;) {
            if (emit_reset) {
                std::size_t n = iconv(iconv_state, 0, 0, &out, &outbytes);
                if (n == std::size_t(-1))
                {
                    int errnum = errno;
                    if (errnum == E2BIG)
                        return false; // Not enought room in out buffer

                    // Unexpected error
                    throw std::runtime_error("'iconv' failed: "+sys::error(errnum));
                }

                if (eoi)
                    return true; // End of conversion
                emit_reset = dirty = false;
            }

            if (emit_replacement) {
                prepare_replacement();
                std::size_t n = replacement_character.size();
                if (outbytes < n)
                    return false; // Not enought room in out buffer
                replacement_character.copy(out, n);
                outbytes -= n;
                out += n;
                emit_replacement = false;
            }

            // Skip iconv if possible
            if (!inbytes) {
                if (!eoi || !dirty)
                    return true; // Input exhausted or end of
                // conversion

                // We need a reset sequence to finish off the conversion
                emit_reset = true;
                continue;
            }

            std::size_t n = iconv(iconv_state, const_cast<char**>(&in),
                                  &inbytes, &out, &outbytes);
            dirty = true; // Reset sequence is now required
            if (n != std::size_t(-1)) {
                if(!eoi) return true; // Input exhausted

                // We need a reset sequence to finish off the conversion
                emit_reset = true;
                continue;
            }

            int errnum = errno;
            if (errnum == E2BIG)
                return false; // Not enought room in out buffer
            if (errnum == EINVAL) {
                // Incomplete character in input
                if (!eoi)
                    return true; // Get some more input
                errnum = EILSEQ; // Interpret as invalid input
            }
            if (errnum != EILSEQ) // Unexpected error
                throw std::runtime_error("'iconv' failed: "+sys::error(errnum));

            if (fail)
                throw IncConvException(sys::error(errnum));

            // Throw away one byte of input
            ++in;
            --inbytes;

            // Request a reset sequence if required, then a replacement
            // character
            if (dirty)
                emit_reset = true;
            emit_replacement = true;
        }
    }

    void prepare_replacement()
    {
        if (!need_replacement)
            return;
        try {
            replacement_character =
                ::transcode("\xEF\xBF\xBD", transcode_UTF_8, target_encoding, true);
        }
        catch (TranscodeException &) {
            try {
                replacement_character = ::transcode("?", transcode_US_ASCII, target_encoding, true);
            }
            catch (TranscodeException&) {
                try {
                    replacement_character = ::transcode(" ", transcode_US_ASCII, target_encoding, true);
                }
                catch (TranscodeException&) {
                    replacement_character = "";
                }
            }
        }
        need_replacement = false;
    }

    State(const TranscoderImpl<SourceChar, TargetChar>& t):
        target_encoding(t.target_encoding),
        fail(t.fail),
        dirty(false),
        emit_reset(false),
        emit_replacement(false),
        need_replacement(true)
    {
        if (std::numeric_limits<unsigned char>::digits != 8)
            throw std::runtime_error("Unsupported number of bits in char: " +
                                     Text::print(std::numeric_limits<unsigned char>::digits));
        std::string source_encoding = t.source_encoding;
        iconv_state = iconv_open(target_encoding.c_str(), source_encoding.c_str());
        if (iconv_state==reinterpret_cast<iconv_t>(-1)) {
            if (errno == EINVAL)
                throw TranscoderNotAvailableException("iconv cannot convert from '"+
                                                      source_encoding+"' to '"+
                                                      target_encoding+"'");
            int errnum = errno;
            throw std::runtime_error("Unexpected error from 'iconv_open': " +
                                     sys::error(errnum));
        }
    }

    ~State()
    {
        iconv_close(iconv_state);
    }

    const std::string target_encoding;
    const bool fail;
    iconv_t iconv_state;
    bool dirty, emit_reset, emit_replacement;
    std::string replacement_character;
    bool need_replacement;
};



class UcsEcodingDecider {
public:
    UcsEcodingDecider()
    {
        if (std::numeric_limits<unsigned char>::digits != 8)
            throw std::runtime_error("Unsupported number of bits in byte");

        bool wide = false;
        if (sizeof (wchar_t) == 4) {
            wide = true;
        }
        else if (sizeof (wchar_t) != 2) {
            throw std::runtime_error("Unsupported size of wchar_t");
        }

        bool big_endian = false;
        if (wide) {
            if (native_endianness[0] && native_endianness[1]) {
                big_endian = true;
            }
            else if(native_endianness[0] || native_endianness[1]) {
                throw std::runtime_error("Unsupported mixed endianness detected");
            }
        }
        else {
            big_endian = native_endianness[0];
        }

        encoding = (wide ?
                    (big_endian ? "UCS-4BE" : "UCS-4LE") :
                    (big_endian ? "UCS-2BE" : "UCS-2LE"));
    }

    std::string encoding;
};



inline std::string get_ucs_encoding()
{
    static UcsEcodingDecider decider;
    return decider.encoding;
}



class Utf16EcodingDecider {
public:
    Utf16EcodingDecider()
    {
        if (std::numeric_limits<unsigned char>::digits != 8)
            throw std::runtime_error("Unsupported number of bits in byte");

        if (sizeof (CharUtf16) != 2)
            throw std::runtime_error("Unsupported size of core::CharUtf16");

        bool big_endian = native_endianness[0];

        encoding = (big_endian ? "UTF-16BE" : "UTF-16LE");
    }

    std::string encoding;
};



inline std::string get_utf16_encoding()
{
    static Utf16EcodingDecider decider;
    return decider.encoding;
}

} // unnamed namespace


namespace archon {
namespace util {

const std::string transcode_US_ASCII     = "US-ASCII";
const std::string transcode_ISO_8859_1   = "ISO-8859-1";
const std::string transcode_ISO_8859_15  = "ISO-8859-15";
const std::string transcode_UTF_8        = "UTF-8";
const std::string transcode_UTF_16LE     = "UTF-16LE";
const std::string transcode_UTF_16BE     = "UTF-16BE";
const std::string transcode_UTF_32LE     = "UTF-32LE";
const std::string transcode_UTF_32BE     = "UTF-32BE";
const std::string transcode_WINDOWS_1252 = "WINDOWS-1252";



std::string transcode(std::string s, std::string source_encoding,
                      std::string target_encoding, bool fail)
{
    TranscoderImpl<char, char> t(source_encoding, target_encoding, fail);
    return inc_convert(t, s);
}


std::unique_ptr<InputStream> get_transcoding_input_stream(InputStream& in,
                                                          std::string source_encoding,
                                                          std::string target_encoding,
                                                          bool fail)
{
    TranscoderImpl<char, char> t{source_encoding, target_encoding, fail};
    return make_inc_conv_in_stream(t, in); // Throws
}


std::unique_ptr<OutputStream> get_transcoding_output_stream(OutputStream& out,
                                                            std::string source_encoding,
                                                            std::string target_encoding,
                                                            bool fail)
{
    TranscoderImpl<char, char> t{source_encoding, target_encoding, fail};
    return make_inc_conv_out_stream(t, out); // Throws
}


std::unique_ptr<const WideCodec> get_transcoding_codec(std::string encoding, bool fail)
{
    using Enc = TranscoderImpl<wchar_t, char>;
    using Dec = TranscoderImpl<char, wchar_t>;
    Enc enc{get_ucs_encoding(), encoding, fail};
    Dec dec{encoding, get_ucs_encoding(), fail};
    return std::make_unique<IncConvCodec<Enc, Dec>>(enc, dec); // Throws
}


std::unique_ptr<Transcoder> get_transcoder(std::string input_encoding,
                                           std::string output_encoding,
                                           bool fail)
{
    TranscoderImpl<char, char> desc{input_encoding, output_encoding, fail};
    return std::make_unique<TranscoderImpl<char, char>::State>(desc); // Throws
}


std::unique_ptr<TranscoderToUtf16> get_transcoder_to_utf16(std::string input_encoding, bool fail)
{
    TranscoderImpl<char, CharUtf16> desc{input_encoding, get_utf16_encoding(), fail};
    return std::make_unique<TranscoderImpl<char, CharUtf16>::State>(desc); // Throws
}

} // namespace util
} // namespace archon
