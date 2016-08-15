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

/**
 * \file
 *
 * \author Kristian Spangsege
 *
 * \todo FIXME: Must use Autoconf to check if \c iconv is available.
 */

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


using namespace std;
using namespace archon::Core;
using namespace archon::Util;

namespace
{
  /**
   * It is crucial that neither \c SourceChar not \c TargetChar is set
   * to anything else than \c char unless it can be guranteed that the
   * character encoding uses exactly the same number of bytes per
   * characters as the size of the corresponding character type.
   */
  template<typename SourceChar, typename TargetChar> struct TranscoderImpl
  {
    typedef SourceChar source_char_type;
    typedef TargetChar target_char_type;

    // We assume that no character requires more than 64 bytes in any
    // encoding.
    static int const min_source_buffer_size =
      (64 + sizeof(SourceChar)-1) / sizeof(SourceChar);

    static int const min_target_buffer_size =
      (64 + sizeof(TargetChar)-1) / sizeof(TargetChar);

    struct State;

    TranscoderImpl(string source_encoding, string target_encoding, bool fail):
      source_encoding(source_encoding), target_encoding(target_encoding),
      fail(fail) {}

    string const source_encoding, target_encoding;
    bool const fail;
  };



  // FIXME: The core stuff that is not concerned with character types
  // is the bulk of what happens here, it it should therefore be moved
  // into a nontemplate base class.
  template<typename SourceChar, typename TargetChar>
  struct TranscoderImpl<SourceChar, TargetChar>::State: BasicTranscoder<SourceChar, TargetChar>
  {
    bool conv(SourceChar const *&in, SourceChar const *in_end,
              TargetChar *&out, TargetChar *out_end, bool eoi)
    {
      char const *in_begin  = reinterpret_cast<char const *>(in);
      char       *out_begin = reinterpret_cast<char       *>(out);

      bool r = byte_conv(in_begin,  reinterpret_cast<char const *>(in_end),
                         out_begin, reinterpret_cast<char       *>(out_end), eoi);

      in  = reinterpret_cast<SourceChar const *>(in_begin);
      out = reinterpret_cast<TargetChar       *>(out_begin);

      return r;
    }


    // Implementing BasicTranscoder::transcode()
    virtual bool transcode(SourceChar const *&in, SourceChar const *in_end,
                           TargetChar *&out, TargetChar *out_end, bool eoi)
    {
      return conv(in, in_end, out, out_end, eoi);
    }


    /**
     * \todo Currently there will be one replacement character per
     * byte of invalid input. this is suboptimal. An achievable
     * improvement will be to distinguish between "invalid input" and
     * "unrepresentible in output". For invalid input we should
     * collapse any number of consecutive invalid bytes into a single
     * replacement character. For "unrepresentible in output" we
     * should substitute individual input characters with replacement
     * characters. The propper number of input bytes can be eaten by a
     * transcoder to UCS-4 by limiting the output buffer to 4
     * bytes. This can also be used for discriminating between the two
     * types of error.
     *
     * \todo An even better scheme for handling invalid input would be
     * to utilize the fact that UTF-8 input octets where the most
     * significant bit is set and the next most significant bit is
     * cleared should be skipped when synchronizing to the start of a
     * character. Similarly for UTF-16 where surrogate elements should
     * be skipped. However, this will only work for these special
     * cases.
     */
    bool byte_conv(char const *&in, char const *in_end,
                   char *&out, char *out_end, bool eoi)
    {
      size_t inbytes = in_end - in, outbytes = out_end - out;
      for(;;)
      {
        if(emit_reset)
        {
          size_t n = iconv(iconv_state, 0, 0, &out, &outbytes);
          if(n==static_cast<size_t>(-1))
          {
            int errnum = errno;
            if(errnum == E2BIG) return false; // Not enought room in out buffer

            // Unexpected error
            throw runtime_error("'iconv' failed: "+Sys::error(errnum));
          }

          if(eoi) return true; // End of conversion
          emit_reset = dirty = false;
        }

        if(emit_replacement)
        {
          prepare_replacement();
          size_t n = replacement_character.size();
          if(outbytes < n) return false; // Not enought room in out buffer
          replacement_character.copy(out, n);
          outbytes -= n;
          out += n;
          emit_replacement = false;
        }

        // Skip iconv if possible
        if(!inbytes)
        {
          if(!eoi || !dirty) return true; // Input exhausted or end of
                                          // conversion

          // We need a reset sequence to finish off the conversion
          emit_reset = true;
          continue;
        }

	size_t n = iconv(iconv_state, const_cast<char **>(&in),
                         &inbytes, &out, &outbytes);
        dirty = true; // Reset sequence is now required
	if(n != static_cast<size_t>(-1))
        {
          if(!eoi) return true; // Input exhausted

          // We need a reset sequence to finish off the conversion
          emit_reset = true;
          continue;
        }

        int errnum = errno;
        if(errnum == E2BIG) return false; // Not enought room in out buffer
        if(errnum == EINVAL)
        {
          // Incomplete character in input
          if(!eoi) return true; // Get some more input
          errnum = EILSEQ; // Interpret as invalid input
        }
        if(errnum != EILSEQ) // Unexpected error
          throw runtime_error("'iconv' failed: "+Sys::error(errnum));

        if(fail) throw IncConvException(Sys::error(errnum));

        // Throw away one byte of input
        ++in;
        --inbytes;

        // Request a reset sequence if required, then a replacement
        // character
        if(dirty) emit_reset = true;
        emit_replacement = true;
      }
    }

    void prepare_replacement()
    {
      if(!need_replacement) return;
      try
      {
        replacement_character =
          ::transcode("\xEF\xBF\xBD", transcode_UTF_8, target_encoding, true);
      }
      catch(TranscodeException &)
      {
        try
        {
          replacement_character = ::transcode("?", transcode_US_ASCII, target_encoding, true);
        }
        catch(TranscodeException &)
        {
          try
          {
            replacement_character = ::transcode(" ", transcode_US_ASCII, target_encoding, true);
          }
          catch(TranscodeException &)
          {
            replacement_character = "";
          }
        }
      }
      need_replacement = false;
    }

    State(TranscoderImpl<SourceChar, TargetChar> const &t):
      target_encoding(t.target_encoding),
      fail(t.fail), dirty(false), emit_reset(false), emit_replacement(false),
      need_replacement(true)
    {
      if(numeric_limits<unsigned char>::digits != 8)
	throw runtime_error("Unsupported number of bits in char: " +
                            Text::print(numeric_limits<unsigned char>::digits));
      string source_encoding = t.source_encoding;
      iconv_state = iconv_open(target_encoding.c_str(), source_encoding.c_str());
      if(iconv_state==reinterpret_cast<iconv_t>(-1))
      {
        if(errno == EINVAL)
          throw TranscoderNotAvailableException("iconv cannot convert from '"+
                                                source_encoding+"' to '"+
                                                target_encoding+"'");
        int errnum = errno;
	throw runtime_error("Unexpected error from 'iconv_open': " +
                            Sys::error(errnum));
      }
    }

    ~State()
    {
      iconv_close(iconv_state);
    }

    string const target_encoding;
    bool const fail;
    iconv_t iconv_state;
    bool dirty, emit_reset, emit_replacement;
    string replacement_character;
    bool need_replacement;
  };



  struct UcsEcodingDecider
  {
    UcsEcodingDecider()
    {
      if (numeric_limits<unsigned char>::digits != 8)
        throw runtime_error("Unsupported number of bits in byte");

      bool wide = false;
      if(sizeof(wchar_t) == 4) wide = true;
      else if(sizeof(wchar_t) != 2)
        throw runtime_error("Unsupported size of wchar_t");

      bool big_endian = false;
      if(wide)
      {
        if(native_endianness[0] && native_endianness[1]) big_endian = true;
        else if(native_endianness[0] || native_endianness[1])
          throw runtime_error("Unsupported mixed endianness detected");
      }
      else big_endian = native_endianness[0];

      encoding = wide ?
        big_endian ? "UCS-4BE" : "UCS-4LE" :
        big_endian ? "UCS-2BE" : "UCS-2LE";
    }

    string encoding;
  };



  inline string get_ucs_encoding()
  {
    static UcsEcodingDecider decider;
    return decider.encoding;
  }



  struct Utf16EcodingDecider
  {
    Utf16EcodingDecider()
    {
      if (numeric_limits<unsigned char>::digits != 8)
        throw runtime_error("Unsupported number of bits in byte");

      if(sizeof(CharUtf16) != 2)
        throw runtime_error("Unsupported size of Core::CharUtf16");

      bool const big_endian = native_endianness[0];

      encoding = big_endian ? "UTF-16BE" : "UTF-16LE";
    }

    string encoding;
  };



  inline string get_utf16_encoding()
  {
    static Utf16EcodingDecider decider;
    return decider.encoding;
  }
}




namespace archon
{
  namespace Util
  {
    string const transcode_US_ASCII     = "US-ASCII";
    string const transcode_ISO_8859_1   = "ISO-8859-1";
    string const transcode_ISO_8859_15  = "ISO-8859-15";
    string const transcode_UTF_8        = "UTF-8";
    string const transcode_UTF_16LE     = "UTF-16LE";
    string const transcode_UTF_16BE     = "UTF-16BE";
    string const transcode_UTF_32LE     = "UTF-32LE";
    string const transcode_UTF_32BE     = "UTF-32BE";
    string const transcode_WINDOWS_1252 = "WINDOWS-1252";



    string transcode(string s, string source_encoding,
                     string target_encoding, bool fail)
    {
      TranscoderImpl<char, char> t(source_encoding, target_encoding, fail);
      return inc_convert(t, s);
    }


    UniquePtr<InputStream> get_transcoding_input_stream(InputStream &in,
                                                        string source_encoding,
                                                        string target_encoding,
                                                        bool fail)
    {
      TranscoderImpl<char, char> t(source_encoding, target_encoding, fail);
      UniquePtr<InputStream> s(make_inc_conv_in_stream(t, in).release());
      return s;
    }


    UniquePtr<OutputStream> get_transcoding_output_stream(OutputStream &out,
                                                          string source_encoding,
                                                          string target_encoding,
                                                          bool fail)
    {
      TranscoderImpl<char, char> t(source_encoding, target_encoding, fail);
      UniquePtr<OutputStream> s(make_inc_conv_out_stream(t, out).release());
      return s;
    }


    UniquePtr<WideCodec const> get_transcoding_codec(string encoding, bool fail)
    {
      typedef TranscoderImpl<wchar_t, char> Enc;
      typedef TranscoderImpl<char, wchar_t> Dec;
      Enc enc(get_ucs_encoding(), encoding, fail);
      Dec dec(encoding, get_ucs_encoding(), fail);
      UniquePtr<WideCodec const> c(new IncConvCodec<Enc, Dec>(enc, dec));
      return c;
    }


    UniquePtr<Transcoder> get_transcoder(string input_encoding, string output_encoding, bool fail)
    {
      TranscoderImpl<char, char> desc(input_encoding, output_encoding, fail);
      UniquePtr<Transcoder> t(new TranscoderImpl<char, char>::State(desc));
      return t;
    }


    UniquePtr<TranscoderToUtf16> get_transcoder_to_utf16(string input_encoding, bool fail)
    {
      TranscoderImpl<char, CharUtf16> desc(input_encoding, get_utf16_encoding(), fail);
      UniquePtr<TranscoderToUtf16> t(new TranscoderImpl<char, CharUtf16>::State(desc));
      return t;
    }
  }
}
