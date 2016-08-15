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
 */

#ifndef ARCHON_CORE_CHAR_ENC_HPP
#define ARCHON_CORE_CHAR_ENC_HPP

#include <cwchar>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <locale>
#include <string>

#include <archon/config_pub.h>
#include <archon/core/codec.hpp>
#include <archon/core/utf16.hpp>


namespace archon
{
  namespace core
  {
//     char_enc_Native,  ///< The multi-byte encoding native to the specified locale.
//     char_enc_ASCII,   ///< The ASCII encoding which is also known as US-ASCII, ANSI_X3.4-1986, and ISO646-US.
//     char_enc_UTF_8,   ///< The UTF-8 encoding.
//     char_enc_UTF_16,  ///< The UTF-16 encoding.
//     char_enc_UTF_32   ///< The UTF-32 encoding (also known as UCS-4).


    /**
     * A character string transcoder. The target encoding of the
     * encode() method, and the origin encoding of the decode() method
     * is determined by the template argument.
     *
     * The origin encoding of the encode() method that takes a narrow
     * first argument, and the target encoding of the decode() method
     * that takes a narrow second argument, is the narrow encoding of
     * the specified locale.
     *
     * The origin encoding of the encode() method that takes a wide
     * first argument is the wide ecoding of the specified locale.
     *
     * A codec for narrow and/or wide character strings. The intention
     * is that each wide character represents a single logical
     * character. The narrow and wide character encodings are expected
     * to be as described by the specified locale. The encoded form is
     * determined entirely by the template parameter which also
     * determines the type of the elements of the encoded string.
     */
    template<class Enc = char> struct CharEnc
    {
      CharEnc(std::locale const &loc);

      bool is_available() { return false; }

      typedef Enc                    enc_char_type;
      typedef std::basic_string<Enc> enc_string_type;

      /**
       * The encoding of the input string is the narrow (or
       * multi-byte) encoding of the specified locale. The encoding of
       * the output is the encoding selected by the template
       * argument. This method assumes that the input string only
       * contains characters from the portable character set, and as a
       * consequence of this, that each input character is represented
       * by a single byte in the input string. Note that by
       * definition, characters from the portable character set can
       * always be represented in the character encodings associated
       * with any of the available locales.
       */
      void encode_narrow(std::string const &in, enc_string_type &out) const;

      /**
       * The encoding of the input string is the wide encoding of the
       * specified locale. The encoding of the output is the encoding
       * selected by the template argument.
       */
      bool encode(std::wstring const &in, enc_string_type &out) const;

      /**
       * The encoding of the input string is the encoding selected by
       * the template argument. The encoding of the output is the
       * narrow (or multi-byte) encoding of the specified
       * locale. However, this method will fail (return false) if any
       * of the converted characters could not be represented as a
       * single byte in the output string.
       */
      bool decode_narrow(enc_string_type const &in, std::string &out) const;

      /**
       * The encoding of the input string is the encoding selected by
       * the template argument. The encoding of the output is the wide
       * encoding of the specified locale.
       */
      bool decode(enc_string_type const &in, std::wstring &out) const;

      /**
       * The encoding of the input string is the encoding selected by
       * the template argument. The encoding of the returned string is
       * the wide encoding of the specified locale.
       *
       * \param replacement The replacement character to use in place
       * of characters that cannot be decoded. Regardless of the
       * specified locale, this character is always assumed to be
       * encoded according to the wide character encoding of the
       * classic locale. Specify L'\uFFFD' to get the Unicode
       * replacement character.
       */
      std::wstring decode(enc_string_type const &in, wchar_t replacement) const;
    };


    template<class Enc>
    inline CharEnc<Enc>::CharEnc(std::locale const &) {}

    template<class Enc>
    inline bool CharEnc<Enc>::encode(std::wstring const &, enc_string_type &) const
    {
      throw std::runtime_error("Unavailable charcter codec");
    }

    template<class Enc>
    inline bool CharEnc<Enc>::decode(enc_string_type const &, std::wstring &) const
    {
      throw std::runtime_error("Unavailable charcter codec");
    }

    template<class Enc>
    inline std::wstring CharEnc<Enc>::decode(enc_string_type const &, wchar_t replacement) const
    {
      throw std::runtime_error("Unavailable charcter codec");
    }


#ifdef ARCHON_WCHAR_ENC_IS_UCS
    template<> struct CharEnc<CharUtf16>
    {
      CharEnc(std::locale const &l): loc(l), ctype2(std::use_facet<std::ctype<wchar_t> >(l)) {}

      bool is_available() { return true; }

      typedef CharUtf16                    enc_char_type;
      typedef std::basic_string<CharUtf16> enc_string_type;

    private:
      typedef std::char_traits<wchar_t>    wide_traits;
      typedef wide_traits::int_type        wide_int_type;
      typedef std::char_traits<CharUtf16>  enc_traits;
      typedef enc_traits::int_type         enc_int_type;

      std::locale const loc;
      std::ctype<wchar_t> const &ctype2;

    public:
      void encode_narrow(std::string const &in, enc_string_type &out) const
      {
        enc_string_type str;
        str.reserve(in.size());
        typedef std::string::const_iterator iter;
        iter const end = in.end();
        for (iter i=in.begin(); i!=end; ++i) {
          wide_int_type v = wide_traits::to_int_type(ctype2.widen(*i));
          // The portable character set is a subset of the printable
          // ASCII characters
          if (int_less_than(v, 0x20) || int_less_than_equal(0x7F, v))
            throw std::runtime_error("Unexpected failue while encoding narrow string as UTF-16");
          str += enc_traits::to_char_type(v);
        }
        out = str;
      }

      bool encode(std::wstring const &in, enc_string_type &out) const
      {
        enc_string_type str;
        str.reserve(in.size());
        typedef std::wstring::const_iterator iter;
        iter const end = in.end();
        for (iter i=in.begin(); i!=end; ++i) {
          wide_int_type v = wide_traits::to_int_type(*i);
          if (int_less_than(v, 0) || int_less_than_equal(0x110000, v))
            return false; // Cannot be encoded by UTF-16
          if (0xD800 <= v && v < 0xE000) return false; // Forbidden surrogate range
          if (v < 0x10000) str += enc_traits::to_char_type(v);
          else {
            v -= 0x10000;
            wide_int_type const v1 = 0xD800 + (v >> 10);
            wide_int_type const v2 = 0xDC00 + (v & 0x3FF);
            str += enc_traits::to_char_type(v1);
            str += enc_traits::to_char_type(v2);
          }
        }
        out = str;
        return true;
      }

      bool decode_narrow(enc_string_type const &in, std::string &out) const
      {
        std::string str;
        str.reserve(in.size());
        typedef enc_string_type::const_iterator iter;
        iter const end = in.end();
        for (iter i=in.begin(); i!=end; ++i) {
          enc_int_type const v = enc_traits::to_int_type(*i);
          if (v == 0xFFFE || v == 0xFFFF) return false; // Illegal UTF-16
          if (0xDC00 <= v && v < 0xE000) return false; // Unexpected high surrogate
          wide_int_type w;
          if (0xD800 <= v && v < 0xDC00) {
            // Combine UTF-16 surrogates
            if (++i == end) return false; // Incomplete surrogate pair
            enc_int_type const v2 = enc_traits::to_int_type(*i);
            if (v2 < 0xDC00 || 0xE000 <= v2) return false; // Invalid high surrogate
            w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
          }
          else w = v;
          wchar_t const wide = wide_traits::to_char_type(w);
          char const narrow = ctype2.narrow(wide, '\0');
          if (narrow == '\0' && ctype2.narrow(wide, 'x') == 'x') return false;
          str += narrow;
        }
        out = str;
        return true;
      }

      bool decode(enc_string_type const &in, std::wstring &out) const
      {
        std::wstring str;
        str.reserve(in.size());
        typedef enc_string_type::const_iterator iter;
        iter const end = in.end();
        for (iter i=in.begin(); i!=end; ++i) {
          enc_int_type const v = enc_traits::to_int_type(*i);
          if (v == 0xFFFE || v == 0xFFFF) return false; // Illegal UTF-16
          if (0xDC00 <= v && v < 0xE000) return false; // Unexpected high surrogate
          wide_int_type w;
          if (0xD800 <= v && v < 0xDC00) {
            // Combine UTF-16 surrogates
            if (++i == end) return false; // Incomplete surrogate pair
            enc_int_type const v2 = enc_traits::to_int_type(*i);
            if (v2 < 0xDC00 || 0xE000 <= v2) return false; // Invalid high surrogate
            w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
          }
          else w = v;
          str += wide_traits::to_char_type(w);
        }
        out = str;
        return true;
      }

      std::wstring decode(enc_string_type const &in, wchar_t replacement) const
      {
        std::wstring str;
        str.reserve(in.size());
        typedef enc_string_type::const_iterator iter;
        iter const end = in.end();
        for (iter i=in.begin(); i!=end; ++i) {
          enc_int_type const v = enc_traits::to_int_type(*i);
          if (v == 0xFFFE || v == 0xFFFF) goto replace; // Illegal UTF-16
          if (0xDC00 <= v && v < 0xE000) goto replace; // Unexpected high surrogate
          wide_int_type w;
          if (0xD800 <= v && v < 0xDC00) {
            // Combine UTF-16 surrogates
            if (++i == end) { // Incomplete surrogate pair
              --i;
              goto replace;
            }
            enc_int_type const v2 = enc_traits::to_int_type(*i);
            if (v2 < 0xDC00 || 0xE000 <= v2) goto replace; // Invalid high surrogate
            w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
          }
          else w = v;
          str += wide_traits::to_char_type(w);
          continue;
        replace:
          str += replacement;
        }
        return str;
      }
    };
#else
#  error Unexpected wide character encoding
#endif // ARCHON_WCHAR_ENC_IS_UCS




    /**
     * Encode the specified wide character string using the ASCII
     * character encoding.
     *
     * \todo FIXME: This is only true if the POSIX locale uses ASCII
     * as the character encoding.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
//    template<typename Ch> std::string ascii_encode(std::basic_string<Ch> const &);

    /**
     * Decode the specified byte sequence assuming it contains
     * charcters encoded using the ASCII character encoding.
     *
     * \todo FIXME: This is only true if the POSIX locale uses ASCII
     * as the character encoding.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
//    template<typename Ch> std::basic_string<Ch> ascii_decode(std::string const &);



    /**
     * Encode the specified wide character string using the external
     * character encoding as specified by the locale given by the
     * execution environment.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
    template<typename Ch> std::string env_encode(std::basic_string<Ch> const &);

    /**
     * Decode the specified (byte) string using the external character
     * encoding as specified by the locale given by the execution
     * environment.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
    template<typename Ch> std::basic_string<Ch> env_decode(std::string const &);



    /**
     * Transform the specified wide character string into a string of
     * conventional characters assuming that ASCII is used as the
     * character representation in both the source and the target
     * string.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
//    template<typename Ch> std::string ascii_narrow(std::basic_string<Ch> const &);

    /**
     * Transform the specified string of conventional characters into
     * a wide character string assuming that ASCII is used as the
     * character representation in both the source and the target
     * string.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
//    template<typename Ch> std::basic_string<Ch> ascii_widen(std::string const &);



    /**
     * Transform the specified wide character string into a string of
     * conventional characters applying any required character set
     * conversion as specified by the execution environment given
     * locale. Any wide character that cannot be represented as a
     * conventional characters is replaced by a question mark.
     *
     * \note This is a conversion between two internal character
     * representations both of which use one entry in the string per
     * logical character. Often, but not always, the two
     * representations are the same. For example, in the C locale,
     * both are ASCII.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
    template<typename Ch> std::string env_narrow(std::basic_string<Ch> const &);

    /**
     * Transform the specified string of conventional characters into
     * a wide character string applying any required character set
     * conversion as specified by the execution environment given
     * locale.
     *
     * \note This is a conversion between two internal character
     * representations both of which use one entry in the string per
     * logical character. Often, but not always, the two
     * representations are the same. For example, in the C locale,
     * both are ASCII.
     *
     * \todo FIXME: This function seems to suggest that the character
     * representation used in the wide character string is in some way
     * locale neutral. This is not the case. It depends on the
     * environment specified locale. In particular it is not
     * necessarily UCS-4, it may be for some locales, for all locales,
     * or for none at all. If __STDC_ISO_10646__ is defined, then
     * wchar_t is treated by the standard library as containing a
     * Unicode character, so in that case, the wide character string
     * can be thought of as acting as a locale neutral representation.
     */
    template<typename Ch> std::basic_string<Ch> env_widen(std::string const &);



//    template<typename Ch> std::basic_string<Ch> ascii_toupper(std::basic_string<Ch> const &);
//    template<typename Ch> std::basic_string<Ch> ascii_tolower(std::basic_string<Ch> const &);

    template<typename Ch> std::basic_string<Ch> env_toupper(std::basic_string<Ch> const &);
    template<typename Ch> std::basic_string<Ch> env_tolower(std::basic_string<Ch> const &);



    template<typename Ch> struct LocaleCodecTraitsBase
    {
      typedef Ch                                        CharType;
      typedef typename std::char_traits<Ch>::state_type StateType;
      typedef std::basic_string<CharType>               StringType;

      static std::string degen_encode(StringType const &);
      static StringType degen_decode(std::string const &);
      static void degen_encode(std::string &, CharType const *, CharType const *);
      static void degen_decode(StringType &, char const *, char const *);
    };

    template<typename Ch> struct LocaleCodecTraits: LocaleCodecTraitsBase<Ch>
    {
      static Ch const replacement_char = '?';
    };



    /**
     * \todo Implement the stream creators. This is probably best done
     * by moving "inc_conv.hpp" from "util" to "core", and then basing
     * it on <tt>Util::IncConvCodec</tt>.
     */
    template<typename Ch, typename Tr = LocaleCodecTraits<Ch> >
    struct BasicLocaleCodec: BasicCodec<Ch>
    {
      typedef Ch                                      CharType;
      typedef Tr                                      TraitsType;
      typedef typename TraitsType::StateType          StateType;
      typedef std::codecvt<CharType, char, StateType> CodecvtType;
      typedef std::ctype<CharType>                    CtypeType;
      typedef std::basic_string<CharType>             StringType;

      BasicLocaleCodec(bool fail = true, std::locale loc = std::locale());

      std::string encode(StringType const &) const;
      StringType decode(std::string const &) const;

      UniquePtr<BasicOutputStream<Ch> > get_enc_out_stream(OutputStream &) const;

      UniquePtr<BasicInputStream<Ch> > get_dec_in_stream(InputStream &) const;

      UniquePtr<InputStream> get_enc_in_stream(BasicInputStream<Ch> &) const;

      UniquePtr<OutputStream> get_dec_out_stream(BasicOutputStream<Ch> &) const;

      UniquePtr<BasicOutputStream<Ch> > get_enc_out_stream(SharedPtr<OutputStream> const &) const;

      UniquePtr<BasicInputStream<Ch> > get_dec_in_stream(SharedPtr<InputStream> const &) const;

      UniquePtr<InputStream> get_enc_in_stream(SharedPtr<BasicInputStream<Ch> > const &) const;

      UniquePtr<OutputStream> get_dec_out_stream(SharedPtr<BasicOutputStream<Ch> > const &) const;

      std::locale getloc() const { return loc; }

    private:
      static unsigned const buffer_size = 512;
      bool const fail;
      std::locale const loc;
      CodecvtType const &cvt;
      CharType const replacement_char;
    };

    typedef BasicLocaleCodec<char>    LocaleCodec;
    typedef BasicLocaleCodec<wchar_t> WideLocaleCodec;



    struct NarrowException;

    template<typename Ch>
    struct BasicLocaleCharMapper
    {
      typedef Ch                             CharType;
      typedef std::basic_string<CharType>    StringType;
      typedef std::ctype<CharType>           CtypeType;
      typedef typename StringType::size_type SizeType;

      BasicLocaleCharMapper(std::locale const &loc = std::locale());

      char narrow(CharType c) const;
      CharType widen(char c) const;

      /**
       * \throw NarrowException if the specified character cannot be
       * represented in an ordinary \c char given the selected locale.
       */
      char narrow_checked(CharType c) const;

      std::string narrow(StringType const &s) const;
      StringType widen(std::string const &s) const;

      CharType toupper(CharType c) const;
      CharType tolower(CharType c) const;

      StringType toupper(StringType const &s) const;
      StringType tolower(StringType const &s) const;

      /**
       * Check whether the specified character belongs to the
       * specified class.
       */
      bool is(CharType c, typename std::ctype_base::mask m) const;

      /**
       * Check whether any of the characters of the specified string
       * belong to the specified class.
       *
       * The logical negative of this method is true if, and only if
       * none of the characters are of the specified class.
       */
      bool are_any(StringType s, typename std::ctype_base::mask m) const;

      /**
       * Check whether all of the characters of the specified string
       * belong to the specified class.
       *
       * The logical negative of this method is true if, and only if
       * at least one character is not of the specified class.
       */
      bool are_all(StringType s, typename std::ctype_base::mask m) const;

      /**
       * Find the position in the specified string of the first
       * character belonging to the specified class.
       *
       * \return The index of the first character in the specified
       * string that belongs to the class indicated by the specified
       * mask. If no such character is found, \c StringType::npos is
       * returned.
       */
      SizeType scan_is(StringType s, typename std::ctype_base::mask m) const;

      /**
       * Find the position in the specified string of the first
       * character not belonging to the specified class.
       *
       * \return The index of the first character in the specified
       * string that does not belongs to the class indicated by the
       * specified mask. If no such character is found, \c
       * StringType::npos is returned.
       */
      SizeType scan_not(StringType s, typename std::ctype_base::mask m) const;

    private:
      std::locale const loc;
      CtypeType const &ctype;
    };

    typedef BasicLocaleCharMapper<char>    LocaleCharMapper;
    typedef BasicLocaleCharMapper<wchar_t> WideLocaleCharMapper;








    // Implementation:

//     template<class Wide, class Enc>
//     inline bool encode(std::basic_string<Wide> const &in, std::basic_string<Enc> &out,
//                        std::locale const &loc, CharEnc enc)
//     {
//       if (enc == char_enc_Native) return 
//       // If __STDC_ISO_10646__ and enc == utf-32:
//       //   If Wide == Enc: return 
//     }


    template<typename Ch> inline std::string LocaleCodecTraitsBase<Ch>::
    degen_encode(StringType const &)
    {
      throw std::runtime_error("Forbidden call");
    }

    template<typename Ch> inline std::basic_string<Ch> LocaleCodecTraitsBase<Ch>::
    degen_decode(std::string const &)
    {
      throw std::runtime_error("Forbidden call");
    }

    template<typename Ch> inline void LocaleCodecTraitsBase<Ch>::
    degen_encode(std::string &, CharType const *, CharType const *)
    {
      throw std::runtime_error("Forbidden call");
    }

    template<typename Ch> inline void LocaleCodecTraitsBase<Ch>::
    degen_decode(StringType &, char const *, char const *)
    {
      throw std::runtime_error("Forbidden call");
    }

    template<> inline std::string LocaleCodecTraitsBase<char>::
    degen_encode(std::string const &s)
    {
      return s;
    }

    template<> inline std::string LocaleCodecTraitsBase<char>::
    degen_decode(std::string const &s)
    {
      return s;
    }

    template<> inline void LocaleCodecTraitsBase<char>::
    degen_encode(std::string &s, char const *b, char const *e)
    {
      s.append(b, e);
    }

    template<> inline void LocaleCodecTraitsBase<char>::
    degen_decode(std::string &s, char const *b, char const *e)
    {
      s.append(b, e);
    }

    template<>
    struct LocaleCodecTraits<wchar_t>: LocaleCodecTraitsBase<wchar_t>
    {
      static wchar_t const replacement_char = L'\uFFFD';
    };



    template<typename Ch, typename Tr>
    inline BasicLocaleCodec<Ch, Tr>::BasicLocaleCodec(bool fail, std::locale l):
      fail(fail), loc(l), cvt(std::use_facet<CodecvtType>(loc)),
      replacement_char(TraitsType::replacement_char)
    {
    }

    template<typename Ch, typename Tr>
    std::string BasicLocaleCodec<Ch, Tr>::encode(StringType const &s) const
    {
      // Check for degenerate codecs that are always
      // one-to-one. According to DR19 (TC) this implies that the
      // internal and external character types are identical, thus we
      // may use the direct copy method from the codec traits class.
      //
      // See also
      // http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#19.
      if(cvt.always_noconv()) return TraitsType::degen_encode(s);

      std::string result;
      char buffer[buffer_size];
      CharType const *from_begin = s.data(), *const from_end = from_begin + s.size(), *from_pos;
      char *const to_begin = buffer, *const to_end = buffer + buffer_size, *to_pos;
      StateType state = StateType(); // Produce an initialized state

      while(from_begin < from_end)
      {
	switch(cvt.out(state, from_begin, from_end, from_pos,
		       to_begin, to_end, to_pos))
	{
	case std::codecvt_base::ok:
	  result.append(to_begin, to_pos);
	  goto done;

	case std::codecvt_base::partial:
	  result.append(to_begin, to_pos);
	  from_begin = from_pos;
	  continue;

	case std::codecvt_base::error:
	  if(fail) throw EncodeException("");
	  result.append(to_begin, to_pos);
	  from_begin = from_pos + 1;
	  // First attempt to write a wide replacement character. If
	  // this fails, fall back to ASCII '?'.
	  if(cvt.out(state, &replacement_char, &replacement_char+1, from_pos,
		     to_begin, to_end, to_pos) != std::codecvt_base::ok)
	  {
	    CharType r = '?';
	    if(cvt.out(state, &r, &r+1, from_pos,
		       to_begin, to_end, to_pos) != std::codecvt_base::ok)
	      throw std::runtime_error("Failed to convert replacement character");
	  }
	  result.append(to_begin, to_pos);
	  continue;

	case std::codecvt_base::noconv:
	  // According to DR19 (TC) this can only happen if internal
	  // and external character types are identical. In this case
	  // we may use the direct copy method from the codec traits
	  // class.
	  //
	  // According to DR19 when 'out' (or 'in') returns 'noconv',
	  // 'from_pos' shall indicate the end of the initial input
	  // sequence that need no conversion. However, there is some
	  // confusion about this, and GNUs libstdc++ has adopted a
	  // different notion where 'from_pos' is always set equal to
	  // 'from_begin', so we need a workaround for this case.
	  //
	  // See also
	  // http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#19.
	  if(from_pos == from_begin) from_pos = from_end;
	  TraitsType::degen_encode(result, from_begin, from_pos);
	}
      }

    done:
      // Output reset sequence for stateful encodings
      for(;;) switch(cvt.unshift(state, to_begin, to_end, to_pos))
      {
      case std::codecvt_base::ok:
	result.append(to_begin, to_pos);
	return result;

      case std::codecvt_base::partial:
	result.append(to_begin, to_pos);
	continue;

      case std::codecvt_base::error:
	throw std::runtime_error("Invalid encoding state");

      case std::codecvt_base::noconv:
	return result;
      }
    }

    template<typename Ch, typename Tr>
    inline typename BasicLocaleCodec<Ch, Tr>::StringType
    BasicLocaleCodec<Ch, Tr>::decode(std::string const &s) const
    {
      // Check for degenerate codecs that are always
      // one-to-one. According to DR19 (TC) this implies that the
      // internal and external character types are identical, thus we
      // may use the direct copy method from the codec traits class.
      //
      // See also
      // http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#19.
      if(cvt.always_noconv()) return TraitsType::degen_decode(s);

      std::basic_string<Ch> result;
      CharType buffer[buffer_size];
      char const *from_begin = s.data(), *const from_end = from_begin + s.size(), *from_pos;
      CharType *const to_begin = buffer, *const to_end = buffer + buffer_size, *to_pos;
      StateType state = StateType(); // Produce an initialized state

      while(from_begin < from_end)
      {
	switch(cvt.in(state, from_begin, from_end, from_pos,
                      to_begin, to_end, to_pos))
	{
	case std::codecvt_base::ok:
	  result.append(to_begin, to_pos);
	  goto done;

	case std::codecvt_base::partial:
	  result.append(to_begin, to_pos);
	  from_begin = from_pos;
	  continue;

	case std::codecvt_base::error:
	  if(fail) throw EncodeException("");
	  result.append(to_begin, to_pos);
	  result.append(1, replacement_char);
	  from_begin = from_pos + 1;
	  continue;

	case std::codecvt_base::noconv:
	  // According to DR19 (TC) this can only happen if internal
	  // and external character types are identical. In this case
	  // we may use the direct copy method from the codec traits
	  // class.
	  //
	  // According to DR19 when 'out' (or 'in') returns 'noconv',
	  // 'from_pos' shall indicate the end of the initial input
	  // sequence that need no conversion. However, there is some
	  // confusion about this, and GNUs libstdc++ has adopted a
	  // different notion where 'from_pos' is always set equal to
	  // 'from_begin', so we need a workaround for this case.
	  //
	  // See also
	  // http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#19.
	  if(from_pos == from_begin) from_pos = from_end;
	  TraitsType::degen_decode(result, from_begin, from_pos);
	}
      }

    done:
      return result;
    }

    template<typename Ch, typename Tr> inline UniquePtr<BasicOutputStream<Ch> >
    BasicLocaleCodec<Ch, Tr>::get_enc_out_stream(OutputStream &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<BasicInputStream<Ch> >
    BasicLocaleCodec<Ch, Tr>::get_dec_in_stream(InputStream &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<InputStream>
    BasicLocaleCodec<Ch, Tr>::get_enc_in_stream(BasicInputStream<Ch> &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<OutputStream>
    BasicLocaleCodec<Ch, Tr>::get_dec_out_stream(BasicOutputStream<Ch> &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<BasicOutputStream<Ch> >
    BasicLocaleCodec<Ch, Tr>::get_enc_out_stream(SharedPtr<OutputStream> const &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<BasicInputStream<Ch> >
    BasicLocaleCodec<Ch, Tr>::get_dec_in_stream(SharedPtr<InputStream> const &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<InputStream>
    BasicLocaleCodec<Ch, Tr>::get_enc_in_stream(SharedPtr<BasicInputStream<Ch> > const &) const
    {
      throw std::runtime_error("Method not implemented");
    }

    template<typename Ch, typename Tr> inline UniquePtr<OutputStream>
    BasicLocaleCodec<Ch, Tr>::get_dec_out_stream(SharedPtr<BasicOutputStream<Ch> > const &) const
    {
      throw std::runtime_error("Method not implemented");
    }


    template<typename Ch>
    inline BasicLocaleCharMapper<Ch>::BasicLocaleCharMapper(std::locale const &l):
      loc(l), ctype(std::use_facet<CtypeType>(loc)) {}

    template<typename Ch> inline char BasicLocaleCharMapper<Ch>::narrow(Ch c) const
    {
      return ctype.narrow(c, '?');
    }

    template<typename Ch> inline Ch BasicLocaleCharMapper<Ch>::widen(char c) const
    {
      return ctype.widen(c);
    }

    struct NarrowException: std::runtime_error
    {
      NarrowException(std::string s): std::runtime_error(s) {}
    };

    template<typename Ch> inline char BasicLocaleCharMapper<Ch>::narrow_checked(Ch c) const
    {
      char const d = ctype.narrow(c, '\0');
      if(d == '\0' && ctype.narrow(c, 'c') == 'c')
        throw NarrowException("Unrepresentable character");
      return d;
    }

    template<typename Ch> inline std::string
    BasicLocaleCharMapper<Ch>::narrow(StringType const &s) const
    {
      std::string t;
      t.reserve(s.size());
      typedef BasicLocaleCharMapper<Ch> This;
      std::transform(s.begin(), s.end(), std::back_inserter(t),
		     std::bind1st(std::mem_fun<char, This, Ch>(&This::narrow), this));
      return t;
    }

    template<typename Ch> inline typename BasicLocaleCharMapper<Ch>::StringType
    BasicLocaleCharMapper<Ch>::widen(std::string const &s) const
    {
      StringType t;
      t.reserve(s.size());
      std::transform(s.begin(), s.end(), std::back_inserter(t),
		     std::bind1st(std::mem_fun(&CtypeType::widen), &ctype));
      return t;
    }

    template<typename Ch> inline Ch BasicLocaleCharMapper<Ch>::toupper(Ch c) const
    {
      return ctype.toupper(c);
    }

    template<typename Ch> inline Ch BasicLocaleCharMapper<Ch>::tolower(Ch c) const
    {
      return ctype.tolower(c);
    }

    template<typename Ch> inline typename BasicLocaleCharMapper<Ch>::StringType
    BasicLocaleCharMapper<Ch>::toupper(StringType const &s) const
    {
      StringType t;
      t.reserve(s.size());
      std::transform(s.begin(), s.end(), std::back_inserter(t),
		     std::bind1st(std::mem_fun(&CtypeType::toupper), &ctype));
      return t;
    }

    template<typename Ch> inline typename BasicLocaleCharMapper<Ch>::StringType
    BasicLocaleCharMapper<Ch>::tolower(StringType const &s) const
    {
      StringType t;
      t.reserve(s.size());
      std::transform(s.begin(), s.end(), std::back_inserter(t),
		     std::bind1st(std::mem_fun(&CtypeType::tolower), &ctype));
      return t;
    }

    template<typename Ch> inline bool
    BasicLocaleCharMapper<Ch>::is(CharType c, typename std::ctype_base::mask m) const
    {
      return ctype.is(m,c);
    }

    template<typename Ch> inline bool
    BasicLocaleCharMapper<Ch>::are_any(StringType s, typename std::ctype_base::mask m) const
    {
      return scan_is(s,m) != StringType::npos;
    }

    template<typename Ch> inline bool
    BasicLocaleCharMapper<Ch>::are_all(StringType s, typename std::ctype_base::mask m) const
    {
      return scan_not(s,m) == StringType::npos;
    }

    template<typename Ch> inline typename BasicLocaleCharMapper<Ch>::SizeType
    BasicLocaleCharMapper<Ch>::scan_is(StringType s, typename std::ctype_base::mask m) const
    {
      CharType *const a = s.data();
      CharType *const b = a + s.size();
      CharType *const c = ctype.scan_is(m,a,b);
      return c == b ? StringType::npos : c - a;
    }

    template<typename Ch> inline typename BasicLocaleCharMapper<Ch>::SizeType
    BasicLocaleCharMapper<Ch>::scan_not(StringType s, typename std::ctype_base::mask m) const
    {
      CharType *const a = s.data();
      CharType *const b = a + s.size();
      CharType *const c = ctype.scan_not(m,a,b);
      return c == b ? StringType::npos : c - a;
    }




    namespace _Impl
    {
      template<typename Ch> inline BasicLocaleCodec<Ch> const &get_ascii_codec()
      {
        static BasicLocaleCodec<Ch> c(false, std::locale::classic());
        return c;
      }
      template<typename Ch> inline BasicLocaleCodec<Ch> const &get_env_codec()
      {
        static BasicLocaleCodec<Ch> c(false, std::locale(""));
        return c;
      }
    }

    template<typename Ch> inline std::string ascii_encode(std::basic_string<Ch> const &s)
    {
      return _Impl::get_ascii_codec<Ch>().encode(s);
    }

    template<> inline std::string ascii_encode(std::string const &s)
    {
      return s;
    }

    template<typename Ch> inline std::basic_string<Ch> ascii_decode(std::string const &s)
    {
      return _Impl::get_ascii_codec<Ch>().decode(s);
    }

    template<> inline std::string ascii_decode(std::string const &s)
    {
      return s;
    }

    template<typename Ch> inline std::string env_encode(std::basic_string<Ch> const &s)
    {
      return _Impl::get_env_codec<Ch>().encode(s);
    }

    template<> inline std::string env_encode(std::string const &s)
    {
      return s;
    }

    template<typename Ch> inline std::basic_string<Ch> env_decode(std::string const &s)
    {
      return _Impl::get_env_codec<Ch>().decode(s);
    }

    template<> inline std::string env_decode(std::string const &s)
    {
      return s;
    }



    namespace _Impl
    {
      template<typename Ch> inline BasicLocaleCharMapper<Ch> const &get_ascii_mapper()
      {
        static BasicLocaleCharMapper<Ch> m(std::locale::classic());
        return m;
      }
      template<typename Ch> inline BasicLocaleCharMapper<Ch> const &get_env_mapper()
      {
        static BasicLocaleCharMapper<Ch> m(std::locale(""));
        return m;
      }
    }

    template<typename Ch>
    inline std::string ascii_narrow(std::basic_string<Ch> const &s)
    {
      return _Impl::get_ascii_mapper<Ch>().narrow(s);
    }

    template<>
    inline std::string ascii_narrow(std::string const &s)
    {
      return s;
    }

    template<typename Ch>
    inline std::basic_string<Ch> ascii_widen(std::string const &s)
    {
      return _Impl::get_ascii_mapper<Ch>().widen(s);
    }

    template<>
    inline std::string ascii_widen(std::string const &s)
    {
      return s;
    }

    template<typename Ch>
    inline std::string env_narrow(std::basic_string<Ch> const &s)
    {
      return _Impl::get_env_mapper<Ch>().narrow(s);
    }

    template<>
    inline std::string env_narrow(std::string const &s)
    {
      return s;
    }

    template<typename Ch>
    inline std::basic_string<Ch> env_widen(std::string const &s)
    {
      return _Impl::get_env_mapper<Ch>().widen(s);
    }

    template<>
    inline std::string env_widen(std::string const &s)
    {
      return s;
    }



    template<typename Ch>
    inline std::basic_string<Ch> ascii_toupper(std::basic_string<Ch> const &s)
    {
      return _Impl::get_ascii_mapper<Ch>().toupper(s);
    }

    template<typename Ch>
    inline std::basic_string<Ch> ascii_tolower(std::basic_string<Ch> const &s)
    {
      return _Impl::get_ascii_mapper<Ch>().tolower(s);
    }

    template<typename Ch>
    inline std::basic_string<Ch> env_toupper(std::basic_string<Ch> const &s)
    {
      return _Impl::get_env_mapper<Ch>().toupper(s);
    }

    template<typename Ch>
    inline std::basic_string<Ch> env_tolower(std::basic_string<Ch> const &s)
    {
      return _Impl::get_env_mapper<Ch>().tolower(s);
    }
  }
}

#endif // ARCHON_CORE_CHAR_ENC_HPP
