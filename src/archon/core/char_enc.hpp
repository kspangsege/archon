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


namespace archon {
namespace core {

/*
char_enc_Native,  ///< The multi-byte encoding native to the specified locale.
char_enc_ASCII,   ///< The ASCII encoding which is also known as US-ASCII, ANSI_X3.4-1986, and ISO646-US.
char_enc_UTF_8,   ///< The UTF-8 encoding.
char_enc_UTF_16,  ///< The UTF-16 encoding.
char_enc_UTF_32   ///< The UTF-32 encoding (also known as UCS-4).
*/

/// A character string transcoder. The target encoding of the encode() method,
/// and the origin encoding of the decode() method is determined by the template
/// argument.
///
/// The origin encoding of the encode() method that takes a narrow first
/// argument, and the target encoding of the decode() method that takes a narrow
/// second argument, is the narrow encoding of the specified locale.
///
/// The origin encoding of the encode() method that takes a wide first argument
/// is the wide ecoding of the specified locale.
///
/// A codec for narrow and/or wide character strings. The intention is that each
/// wide character represents a single logical character. The narrow and wide
/// character encodings are expected to be as described by the specified
/// locale. The encoded form is determined entirely by the template parameter
/// which also determines the type of the elements of the encoded string.
template<class Enc = char> class CharEnc {
public:
    CharEnc(const std::locale&);

    bool is_available() { return false; }

    using enc_char_type = Enc;
    using enc_string_type = std::basic_string<Enc>;

    /// The encoding of the input string is the narrow (or multi-byte) encoding
    /// of the specified locale. The encoding of the output is the encoding
    /// selected by the template argument. This method assumes that the input
    /// string only contains characters from the portable character set, and as
    /// a consequence of this, that each input character is represented by a
    /// single byte in the input string. Note that by definition, characters
    /// from the portable character set can always be represented in the
    /// character encodings associated with any of the available locales.
    void encode_narrow(const std::string& in, enc_string_type& out) const;

    /// The encoding of the input string is the wide encoding of the specified
    /// locale. The encoding of the output is the encoding selected by the
    /// template argument.
    bool encode(const std::wstring& in, enc_string_type& out) const;

    /// The encoding of the input string is the encoding selected by the
    /// template argument. The encoding of the output is the narrow (or
    /// multi-byte) encoding of the specified locale. However, this method will
    /// fail (return false) if any of the converted characters could not be
    /// represented as a single byte in the output string.
    bool decode_narrow(const enc_string_type& in, std::string& out) const;

    /// The encoding of the input string is the encoding selected by the
    /// template argument. The encoding of the output is the wide encoding of
    /// the specified locale.
    bool decode(const enc_string_type& in, std::wstring& out) const;

    /// The encoding of the input string is the encoding selected by the
    /// template argument. The encoding of the returned string is the wide
    /// encoding of the specified locale.
    ///
    /// \param replacement The replacement character to use in place of
    /// characters that cannot be decoded. Regardless of the specified locale,
    /// this character is always assumed to be encoded according to the wide
    /// character encoding of the classic locale. Specify L'\uFFFD' to get the
    /// Unicode replacement character.
    std::wstring decode(const enc_string_type& in, wchar_t replacement) const;
};


template<class Enc>
inline CharEnc<Enc>::CharEnc(const std::locale&)
{
}

template<class Enc>
inline bool CharEnc<Enc>::encode(const std::wstring&, enc_string_type&) const
{
    throw std::runtime_error("Unavailable charcter codec");
}

template<class Enc>
inline bool CharEnc<Enc>::decode(const enc_string_type&, std::wstring&) const
{
    throw std::runtime_error("Unavailable charcter codec");
}

template<class Enc>
inline std::wstring CharEnc<Enc>::decode(const enc_string_type&, wchar_t) const
{
    throw std::runtime_error("Unavailable charcter codec");
}


#ifdef ARCHON_WCHAR_ENC_IS_UCS

template<> class CharEnc<CharUtf16> {
public:
    using enc_char_type = CharUtf16;
    using enc_string_type = std::basic_string<CharUtf16>;

    CharEnc(const std::locale& l):
        loc(l),
        ctype2(std::use_facet<std::ctype<wchar_t>>(l))
    {
    }

    bool is_available()
    {
        return true;
    }

private:
    using wide_traits = std::char_traits<wchar_t>;
    using wide_int_type = wide_traits::int_type;
    using enc_traits = std::char_traits<CharUtf16>;
    using enc_int_type = enc_traits::int_type;

    const std::locale loc;
    const std::ctype<wchar_t>& ctype2;

public:
    void encode_narrow(const std::string& in, enc_string_type& out) const
    {
        enc_string_type str;
        str.reserve(in.size());
        for (char c: in) {
            wide_int_type v = wide_traits::to_int_type(ctype2.widen(c));
            // The portable character set is a subset of the printable ASCII
            // characters
            if (int_less_than(v, 0x20) || int_less_than_equal(0x7F, v)) {
                throw std::runtime_error("Unexpected failue while encoding narrow string as "
                                         "UTF-16");
            }
            str += enc_traits::to_char_type(v);
        }
        out = str;
    }

    bool encode(const std::wstring& in, enc_string_type& out) const
    {
        enc_string_type str;
        str.reserve(in.size());
        for (wchar_t c: in) {
            wide_int_type v = wide_traits::to_int_type(c);
            if (int_less_than(v, 0) || int_less_than_equal(0x110000, v))
                return false; // Cannot be encoded by UTF-16
            if (0xD800 <= v && v < 0xE000)
                return false; // Forbidden surrogate range
            if (v < 0x10000) {
                str += enc_traits::to_char_type(v);
            }
            else {
                v -= 0x10000;
                wide_int_type v1 = 0xD800 + (v >> 10);
                wide_int_type v2 = 0xDC00 + (v & 0x3FF);
                str += enc_traits::to_char_type(v1);
                str += enc_traits::to_char_type(v2);
            }
        }
        out = str;
        return true;
    }

    bool decode_narrow(const enc_string_type& in, std::string& out) const
    {
        std::string str;
        str.reserve(in.size());
        auto end = in.end();
        for (auto i = in.begin(); i != end; ++i) {
            enc_int_type v = enc_traits::to_int_type(*i);
            if (v == 0xFFFE || v == 0xFFFF)
                return false; // Illegal UTF-16
            if (0xDC00 <= v && v < 0xE000)
                return false; // Unexpected high surrogate
            wide_int_type w;
            if (0xD800 <= v && v < 0xDC00) {
                // Combine UTF-16 surrogates
                if (++i == end)
                    return false; // Incomplete surrogate pair
                enc_int_type v2 = enc_traits::to_int_type(*i);
                if (v2 < 0xDC00 || 0xE000 <= v2)
                    return false; // Invalid high surrogate
                w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
            }
            else {
                w = v;
            }
            wchar_t wide = wide_traits::to_char_type(w);
            char narrow = ctype2.narrow(wide, '\0');
            if (narrow == '\0' && ctype2.narrow(wide, 'x') == 'x')
                return false;
            str += narrow;
        }
        out = str;
        return true;
    }

    bool decode(const enc_string_type& in, std::wstring& out) const
    {
        std::wstring str;
        str.reserve(in.size());
        auto end = in.end();
        for (auto i = in.begin(); i != end; ++i) {
            enc_int_type v = enc_traits::to_int_type(*i);
            if (v == 0xFFFE || v == 0xFFFF)
                return false; // Illegal UTF-16
            if (0xDC00 <= v && v < 0xE000)
                return false; // Unexpected high surrogate
            wide_int_type w;
            if (0xD800 <= v && v < 0xDC00) {
                // Combine UTF-16 surrogates
                if (++i == end)
                    return false; // Incomplete surrogate pair
                enc_int_type v2 = enc_traits::to_int_type(*i);
                if (v2 < 0xDC00 || 0xE000 <= v2)
                    return false; // Invalid high surrogate
                w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
            }
            else {
                w = v;
            }
            str += wide_traits::to_char_type(w);
        }
        out = str;
        return true;
    }

    std::wstring decode(const enc_string_type& in, wchar_t replacement) const
    {
        std::wstring str;
        str.reserve(in.size());
        auto end = in.end();
        for (auto i = in.begin(); i != end; ++i) {
            enc_int_type v = enc_traits::to_int_type(*i);
            if (v == 0xFFFE || v == 0xFFFF)
                goto replace; // Illegal UTF-16
            if (0xDC00 <= v && v < 0xE000)
                goto replace; // Unexpected high surrogate
            wide_int_type w;
            if (0xD800 <= v && v < 0xDC00) {
                // Combine UTF-16 surrogates
                if (++i == end) { // Incomplete surrogate pair
                    --i;
                    goto replace;
                }
                enc_int_type v2 = enc_traits::to_int_type(*i);
                if (v2 < 0xDC00 || 0xE000 <= v2)
                    goto replace; // Invalid high surrogate
                w = 0x10000 + (wide_int_type(v-0xD800)<<10) + (v2-0xDC00);
            }
            else {
                w = v;
            }
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



/*
/// Encode the specified wide character string using the ASCII character
/// encoding.
///
/// \todo FIXME: This is only true if the POSIX locale uses ASCII as the
/// character encoding.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::string ascii_encode(const std::basic_string<Ch>&);

/// Decode the specified byte sequence assuming it contains charcters encoded
/// using the ASCII character encoding.
///
/// \todo FIXME: This is only true if the POSIX locale uses ASCII as the
/// character encoding.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::basic_string<Ch> ascii_decode(const std::string&);
*/


/// Encode the specified wide character string using the external character
/// encoding as specified by the locale given by the execution environment.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::string env_encode(const std::basic_string<Ch>&);

/// Decode the specified (byte) string using the external character encoding as
/// specified by the locale given by the execution environment.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::basic_string<Ch> env_decode(const std::string&);


/*
/// Transform the specified wide character string into a string of conventional
/// characters assuming that ASCII is used as the character representation in
/// both the source and the target string.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
 template<class Ch> std::string ascii_narrow(const std::basic_string<Ch>&);

/// Transform the specified string of conventional characters into a wide
/// character string assuming that ASCII is used as the character representation
/// in both the source and the target string.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::basic_string<Ch> ascii_widen(const std::string&);
*/


/// Transform the specified wide character string into a string of conventional
/// characters applying any required character set conversion as specified by
/// the execution environment given locale. Any wide character that cannot be
/// represented as a conventional characters is replaced by a question mark.
///
/// \note This is a conversion between two internal character representations
/// both of which use one entry in the string per logical character. Often, but
/// not always, the two representations are the same. For example, in the C
/// locale, both are ASCII.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::string env_narrow(const std::basic_string<Ch>&);

/// Transform the specified string of conventional characters into a wide
/// character string applying any required character set conversion as specified
/// by the execution environment given locale.
///
/// \note This is a conversion between two internal character representations
/// both of which use one entry in the string per logical character. Often, but
/// not always, the two representations are the same. For example, in the C
/// locale, both are ASCII.
///
/// \todo FIXME: This function seems to suggest that the character
/// representation used in the wide character string is in some way locale
/// neutral. This is not the case. It depends on the environment specified
/// locale. In particular it is not necessarily UCS-4, it may be for some
/// locales, for all locales, or for none at all. If __STDC_ISO_10646__ is
/// defined, then wchar_t is treated by the standard library as containing a
/// Unicode character, so in that case, the wide character string can be thought
/// of as acting as a locale neutral representation.
template<class Ch> std::basic_string<Ch> env_widen(const std::string&);



/*
template<class Ch> std::basic_string<Ch> ascii_toupper(const std::basic_string<Ch>&);
template<class Ch> std::basic_string<Ch> ascii_tolower(const std::basic_string<Ch>&);
*/

template<class Ch> std::basic_string<Ch> env_toupper(const std::basic_string<Ch>&);
template<class Ch> std::basic_string<Ch> env_tolower(const std::basic_string<Ch>&);



template<class Ch> class LocaleCodecTraitsBase {
public:
    typedef Ch                                        CharType;
    typedef typename std::char_traits<Ch>::state_type StateType;
    typedef std::basic_string<CharType>               StringType;

    static std::string degen_encode(const StringType&);
    static StringType degen_decode(const std::string&);
    static void degen_encode(std::string&, const CharType*, const CharType*);
    static void degen_decode(StringType&, const char*, const char*);
};

template<class Ch> class LocaleCodecTraits: public LocaleCodecTraitsBase<Ch> {
public:
    static constexpr Ch replacement_char = '?';
};



/// \todo Implement the stream creators. This is probably best done by moving
/// "inc_conv.hpp" from "util" to "core", and then basing it on
/// <tt>util::IncConvCodec</tt>.
template<class Ch, class Tr = LocaleCodecTraits<Ch>>
class BasicLocaleCodec: public BasicCodec<Ch> {
public:
    typedef Ch                                      CharType;
    typedef Tr                                      TraitsType;
    typedef typename TraitsType::StateType          StateType;
    typedef std::codecvt<CharType, char, StateType> CodecvtType;
    typedef std::ctype<CharType>                    CtypeType;
    typedef std::basic_string<CharType>             StringType;

    BasicLocaleCodec(bool fail = true, std::locale loc = std::locale());

    std::string encode(const StringType&) const;
    StringType decode(const std::string&) const;

    std::unique_ptr<BasicOutputStream<Ch>> get_enc_out_stream(OutputStream&) const;

    std::unique_ptr<BasicInputStream<Ch>> get_dec_in_stream(InputStream&) const;

    std::unique_ptr<InputStream> get_enc_in_stream(BasicInputStream<Ch>&) const;

    std::unique_ptr<OutputStream> get_dec_out_stream(BasicOutputStream<Ch>&) const;

    std::unique_ptr<BasicOutputStream<Ch>> get_enc_out_stream(const std::shared_ptr<OutputStream>&) const;

    std::unique_ptr<BasicInputStream<Ch>> get_dec_in_stream(const std::shared_ptr<InputStream>&) const;

    std::unique_ptr<InputStream> get_enc_in_stream(const std::shared_ptr<BasicInputStream<Ch>>&) const;

    std::unique_ptr<OutputStream> get_dec_out_stream(const std::shared_ptr<BasicOutputStream<Ch>>&) const;

    std::locale getloc() const
    {
        return loc;
    }

private:
    static constexpr unsigned buffer_size = 512;
    const bool fail;
    const std::locale loc;
    const CodecvtType& cvt;
    const CharType replacement_char;
};

using LocaleCodec = BasicLocaleCodec<char>;
using WideLocaleCodec = BasicLocaleCodec<wchar_t>;



class NarrowException;

template<class Ch> class BasicLocaleCharMapper {
public:
    typedef Ch                             CharType;
    typedef std::basic_string<CharType>    StringType;
    typedef std::ctype<CharType>           CtypeType;
    typedef typename StringType::size_type SizeType;

    BasicLocaleCharMapper(const std::locale& loc = std::locale());

    char narrow(CharType c) const;
    CharType widen(char c) const;

    /// \throw NarrowException if the specified character cannot be represented
    /// in an ordinary \c char given the selected locale.
    char narrow_checked(CharType c) const;

    std::string narrow(const StringType& s) const;
    StringType widen(const std::string& s) const;

    CharType toupper(CharType c) const;
    CharType tolower(CharType c) const;

    StringType toupper(const StringType& s) const;
    StringType tolower(const StringType& s) const;

    /// Check whether the specified character belongs to the specified class.
    bool is(CharType c, typename std::ctype_base::mask m) const;

    /// Check whether any of the characters of the specified string belong to
    /// the specified class.
    ///
    /// The logical negative of this method is true if, and only if none of the
    /// characters are of the specified class.
    bool are_any(StringType s, typename std::ctype_base::mask m) const;

    /// Check whether all of the characters of the specified string belong to
    /// the specified class.
    ///
    /// The logical negative of this method is true if, and only if at least one
    /// character is not of the specified class.
    bool are_all(StringType s, typename std::ctype_base::mask m) const;

    /// Find the position in the specified string of the first character
    /// belonging to the specified class.
    ///
    /// \return The index of the first character in the specified string that
    /// belongs to the class indicated by the specified mask. If no such
    /// character is found, \c StringType::npos is returned.
    SizeType scan_is(StringType s, typename std::ctype_base::mask m) const;

    /// Find the position in the specified string of the first character not
    /// belonging to the specified class.
    ///
    /// \return The index of the first character in the specified string that
    /// does not belongs to the class indicated by the specified mask. If no
    /// such character is found, \c StringType::npos is returned.
    SizeType scan_not(StringType s, typename std::ctype_base::mask m) const;

private:
    const std::locale loc;
    const CtypeType& ctype;
};

using LocaleCharMapper = BasicLocaleCharMapper<char>;
using WideLocaleCharMapper = BasicLocaleCharMapper<wchar_t>;




// Implementation

/*
template<class Wide, class Enc>
inline bool encode(const std::basic_string<Wide>& in, std::basic_string<Enc>& out,
                   const std::locale& loc, CharEnc enc)
{
  if (enc == char_enc_Native)
      return 
  // If __STDC_ISO_10646__ and enc == utf-32:
  //   If Wide == Enc: return 
}
*/

template<class Ch> inline std::string LocaleCodecTraitsBase<Ch>::
degen_encode(const StringType&)
{
    throw std::runtime_error("Forbidden call");
}

template<class Ch> inline std::basic_string<Ch> LocaleCodecTraitsBase<Ch>::
degen_decode(const std::string&)
{
    throw std::runtime_error("Forbidden call");
}

template<class Ch> inline void LocaleCodecTraitsBase<Ch>::
degen_encode(std::string&, const CharType*, const CharType*)
{
    throw std::runtime_error("Forbidden call");
}

template<class Ch> inline void LocaleCodecTraitsBase<Ch>::
degen_decode(StringType&, const char*, const char*)
{
    throw std::runtime_error("Forbidden call");
}

template<> inline std::string LocaleCodecTraitsBase<char>::
degen_encode(const std::string& s)
{
    return s;
}

template<> inline std::string LocaleCodecTraitsBase<char>::
degen_decode(const std::string& s)
{
    return s;
}

template<> inline void LocaleCodecTraitsBase<char>::
degen_encode(std::string& s, const char* b, const char* e)
{
    s.append(b, e);
}

template<> inline void LocaleCodecTraitsBase<char>::
degen_decode(std::string& s, const char* b, const char* e)
{
    s.append(b, e);
}

template<> class LocaleCodecTraits<wchar_t>: public LocaleCodecTraitsBase<wchar_t> {
public:
    static constexpr wchar_t replacement_char = L'\uFFFD';
};



template<class Ch, class Tr>
inline BasicLocaleCodec<Ch, Tr>::BasicLocaleCodec(bool fail, std::locale l):
    fail(fail),
    loc(l),
    cvt(std::use_facet<CodecvtType>(loc)),
    replacement_char(TraitsType::replacement_char)
{
}

template<class Ch, class Tr>
std::string BasicLocaleCodec<Ch, Tr>::encode(const StringType& s) const
{
    // Check for degenerate codecs that are always one-to-one. According to DR19
    // (TC) this implies that the internal and external character types are
    // identical, thus we may use the direct copy method from the codec traits
    // class.
    //
    // See also http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#19.
    if (cvt.always_noconv())
        return TraitsType::degen_encode(s);

    std::string result;
    char buffer[buffer_size];
    const CharType* from_begin = s.data();
    const CharType* from_end = from_begin + s.size();
    const CharType* from_pos;
    char* to_begin = buffer;
    char* to_end = buffer + buffer_size;
    char* to_pos;
    StateType state = StateType(); // Produce an initialized state

    while (from_begin < from_end) {
        switch (cvt.out(state, from_begin, from_end, from_pos, to_begin, to_end, to_pos)) {
            case std::codecvt_base::ok:
                result.append(to_begin, to_pos);
                goto done;

            case std::codecvt_base::partial:
                result.append(to_begin, to_pos);
                from_begin = from_pos;
                continue;

            case std::codecvt_base::error:
                if (fail)
                    throw EncodeException("");
                result.append(to_begin, to_pos);
                from_begin = from_pos + 1;
                // First attempt to write a wide replacement character. If this
                // fails, fall back to ASCII '?'.
                if (cvt.out(state, &replacement_char, &replacement_char+1, from_pos,
                            to_begin, to_end, to_pos) != std::codecvt_base::ok) {
                    CharType r = '?';
                    if (cvt.out(state, &r, &r+1, from_pos,
                                to_begin, to_end, to_pos) != std::codecvt_base::ok)
                        throw std::runtime_error("Failed to convert replacement character");
                }
                result.append(to_begin, to_pos);
                continue;

            case std::codecvt_base::noconv:
                // According to DR19 (TC) this can only happen if internal and
                // external character types are identical. In this case we may
                // use the direct copy method from the codec traits class.
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
                if (from_pos == from_begin)
                    from_pos = from_end;
                TraitsType::degen_encode(result, from_begin, from_pos);
        }
    }

  done:
    // Output reset sequence for stateful encodings
    for (;;) {
        switch (cvt.unshift(state, to_begin, to_end, to_pos)) {
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
}

template<class Ch, class Tr>
inline typename BasicLocaleCodec<Ch, Tr>::StringType
BasicLocaleCodec<Ch, Tr>::decode(const std::string& s) const
{
    // Check for degenerate codecs that are always one-to-one. According to DR19
    // (TC) this implies that the internal and external character types are
    // identical, thus we may use the direct copy method from the codec traits
    // class.
    //
    // See also http://www.open-std.org/jtc1/sc22/wg21/docs/lwg-defects.html#19.
    if (cvt.always_noconv())
        return TraitsType::degen_decode(s);

    std::basic_string<Ch> result;
    CharType buffer[buffer_size];
    const char* from_begin = s.data();
    const char* from_end = from_begin + s.size();
    const char* from_pos;
    CharType* to_begin = buffer;
    CharType* to_end = buffer + buffer_size;
    CharType* to_pos;
    StateType state = StateType(); // Produce an initialized state

    while (from_begin < from_end) {
        switch (cvt.in(state, from_begin, from_end, from_pos, to_begin, to_end, to_pos)) {
            case std::codecvt_base::ok:
                result.append(to_begin, to_pos);
                goto done;

            case std::codecvt_base::partial:
                result.append(to_begin, to_pos);
                from_begin = from_pos;
                continue;

            case std::codecvt_base::error:
                if (fail)
                    throw EncodeException("");
                result.append(to_begin, to_pos);
                result.append(1, replacement_char);
                from_begin = from_pos + 1;
                continue;

            case std::codecvt_base::noconv:
                // According to DR19 (TC) this can only happen if internal and
                // external character types are identical. In this case we may
                // use the direct copy method from the codec traits class.
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
                if (from_pos == from_begin)
                    from_pos = from_end;
                TraitsType::degen_decode(result, from_begin, from_pos);
        }
    }

  done:
    return result;
}

template<class Ch, class Tr> inline std::unique_ptr<BasicOutputStream<Ch>>
BasicLocaleCodec<Ch, Tr>::get_enc_out_stream(OutputStream&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<BasicInputStream<Ch>>
BasicLocaleCodec<Ch, Tr>::get_dec_in_stream(InputStream&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<InputStream>
BasicLocaleCodec<Ch, Tr>::get_enc_in_stream(BasicInputStream<Ch>&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<OutputStream>
BasicLocaleCodec<Ch, Tr>::get_dec_out_stream(BasicOutputStream<Ch>&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<BasicOutputStream<Ch>>
BasicLocaleCodec<Ch, Tr>::get_enc_out_stream(const std::shared_ptr<OutputStream>&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<BasicInputStream<Ch>>
BasicLocaleCodec<Ch, Tr>::get_dec_in_stream(const std::shared_ptr<InputStream>&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<InputStream>
BasicLocaleCodec<Ch, Tr>::get_enc_in_stream(const std::shared_ptr<BasicInputStream<Ch>>&) const
{
    throw std::runtime_error("Method not implemented");
}

template<class Ch, class Tr> inline std::unique_ptr<OutputStream>
BasicLocaleCodec<Ch, Tr>::get_dec_out_stream(const std::shared_ptr<BasicOutputStream<Ch>>&) const
{
    throw std::runtime_error("Method not implemented");
}


template<class Ch>
inline BasicLocaleCharMapper<Ch>::BasicLocaleCharMapper(const std::locale& l):
    loc(l),
    ctype(std::use_facet<CtypeType>(loc))
{
}

template<class Ch> inline char BasicLocaleCharMapper<Ch>::narrow(Ch c) const
{
    return ctype.narrow(c, '?');
}

template<class Ch> inline Ch BasicLocaleCharMapper<Ch>::widen(char c) const
{
    return ctype.widen(c);
}

class NarrowException: public std::runtime_error {
public:
    NarrowException(const std::string& message):
        std::runtime_error(message)
    {
    }
};

template<class Ch> inline char BasicLocaleCharMapper<Ch>::narrow_checked(Ch c) const
{
    char d = ctype.narrow(c, '\0');
    if (d == '\0' && ctype.narrow(c, 'c') == 'c')
        throw NarrowException("Unrepresentable character");
    return d;
}

template<class Ch> inline std::string
BasicLocaleCharMapper<Ch>::narrow(const StringType& s) const
{
    std::string t;
    t.reserve(s.size());
    using This = BasicLocaleCharMapper<Ch>;
    std::transform(s.begin(), s.end(), std::back_inserter(t),
                   std::bind1st(std::mem_fun<char, This, Ch>(&This::narrow), this));
    return t;
}

template<class Ch> inline typename BasicLocaleCharMapper<Ch>::StringType
BasicLocaleCharMapper<Ch>::widen(const std::string& s) const
{
    StringType t;
    t.reserve(s.size());
    std::transform(s.begin(), s.end(), std::back_inserter(t),
                   std::bind1st(std::mem_fun(&CtypeType::widen), &ctype));
    return t;
}

template<class Ch> inline Ch BasicLocaleCharMapper<Ch>::toupper(Ch c) const
{
    return ctype.toupper(c);
}

template<class Ch> inline Ch BasicLocaleCharMapper<Ch>::tolower(Ch c) const
{
    return ctype.tolower(c);
}

template<class Ch> inline typename BasicLocaleCharMapper<Ch>::StringType
BasicLocaleCharMapper<Ch>::toupper(const StringType& s) const
{
    StringType t;
    t.reserve(s.size());
    std::transform(s.begin(), s.end(), std::back_inserter(t),
                   std::bind1st(std::mem_fun(&CtypeType::toupper), &ctype));
    return t;
}

template<class Ch> inline typename BasicLocaleCharMapper<Ch>::StringType
BasicLocaleCharMapper<Ch>::tolower(const StringType& s) const
{
    StringType t;
    t.reserve(s.size());
    std::transform(s.begin(), s.end(), std::back_inserter(t),
                   std::bind1st(std::mem_fun(&CtypeType::tolower), &ctype));
    return t;
}

template<class Ch> inline bool
BasicLocaleCharMapper<Ch>::is(CharType c, typename std::ctype_base::mask m) const
{
    return ctype.is(m,c);
}

template<class Ch> inline bool
BasicLocaleCharMapper<Ch>::are_any(StringType s, typename std::ctype_base::mask m) const
{
    return scan_is(s,m) != StringType::npos;
}

template<class Ch> inline bool
BasicLocaleCharMapper<Ch>::are_all(StringType s, typename std::ctype_base::mask m) const
{
    return scan_not(s,m) == StringType::npos;
}

template<class Ch> inline typename BasicLocaleCharMapper<Ch>::SizeType
BasicLocaleCharMapper<Ch>::scan_is(StringType s, typename std::ctype_base::mask m) const
{
    CharType* a = s.data();
    CharType* b = a + s.size();
    CharType* c = ctype.scan_is(m,a,b);
    return (c == b ? StringType::npos : c - a);
}

template<class Ch> inline typename BasicLocaleCharMapper<Ch>::SizeType
BasicLocaleCharMapper<Ch>::scan_not(StringType s, typename std::ctype_base::mask m) const
{
    CharType* a = s.data();
    CharType* b = a + s.size();
    CharType* c = ctype.scan_not(m,a,b);
    return (c == b ? StringType::npos : c - a);
}




namespace _impl {

template<class Ch> inline const BasicLocaleCodec<Ch>& get_ascii_codec()
{
    static BasicLocaleCodec<Ch> c(false, std::locale::classic());
    return c;
}
template<class Ch> inline const BasicLocaleCodec<Ch>& get_env_codec()
{
    static BasicLocaleCodec<Ch> c(false, std::locale(""));
    return c;
}

} // namespace _impl

template<class Ch> inline std::string ascii_encode(const std::basic_string<Ch>& s)
{
    return _impl::get_ascii_codec<Ch>().encode(s);
}

template<> inline std::string ascii_encode(const std::string& s)
{
    return s;
}

template<class Ch> inline std::basic_string<Ch> ascii_decode(const std::string& s)
{
    return _impl::get_ascii_codec<Ch>().decode(s);
}

template<> inline std::string ascii_decode(const std::string& s)
{
    return s;
}

template<class Ch> inline std::string env_encode(const std::basic_string<Ch>& s)
{
    return _impl::get_env_codec<Ch>().encode(s);
}

template<> inline std::string env_encode(const std::string& s)
{
    return s;
}

template<class Ch> inline std::basic_string<Ch> env_decode(const std::string& s)
{
    return _impl::get_env_codec<Ch>().decode(s);
}

template<> inline std::string env_decode(const std::string& s)
{
    return s;
}



namespace _impl {

template<class Ch> inline const BasicLocaleCharMapper<Ch>& get_ascii_mapper()
{
    static BasicLocaleCharMapper<Ch> m(std::locale::classic());
    return m;
}
template<class Ch> inline const BasicLocaleCharMapper<Ch>& get_env_mapper()
{
    static BasicLocaleCharMapper<Ch> m(std::locale(""));
    return m;
}

} // namespace _impl

template<class Ch>
inline std::string ascii_narrow(const std::basic_string<Ch>& s)
{
    return _impl::get_ascii_mapper<Ch>().narrow(s);
}

template<>
inline std::string ascii_narrow(const std::string& s)
{
    return s;
}

template<class Ch>
inline std::basic_string<Ch> ascii_widen(const std::string& s)
{
    return _impl::get_ascii_mapper<Ch>().widen(s);
}

template<>
inline std::string ascii_widen(const std::string& s)
{
    return s;
}

template<class Ch>
inline std::string env_narrow(const std::basic_string<Ch>& s)
{
    return _impl::get_env_mapper<Ch>().narrow(s);
}

template<>
inline std::string env_narrow(const std::string& s)
{
    return s;
}

template<class Ch>
inline std::basic_string<Ch> env_widen(const std::string& s)
{
    return _impl::get_env_mapper<Ch>().widen(s);
}

template<>
inline std::string env_widen(const std::string& s)
{
    return s;
}



template<class Ch>
inline std::basic_string<Ch> ascii_toupper(const std::basic_string<Ch>& s)
{
    return _impl::get_ascii_mapper<Ch>().toupper(s);
}

template<class Ch>
inline std::basic_string<Ch> ascii_tolower(const std::basic_string<Ch>& s)
{
    return _impl::get_ascii_mapper<Ch>().tolower(s);
}

template<class Ch>
inline std::basic_string<Ch> env_toupper(const std::basic_string<Ch>& s)
{
    return _impl::get_env_mapper<Ch>().toupper(s);
}

template<class Ch>
inline std::basic_string<Ch> env_tolower(const std::basic_string<Ch>& s)
{
    return _impl::get_env_mapper<Ch>().tolower(s);
}

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_CHAR_ENC_HPP
