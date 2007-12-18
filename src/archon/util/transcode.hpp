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
 * Provides a number of utilities for converting strings and streams
 * between various character encodings. Encodings are specified using
 * the names registered by IANA. Please see the IANA registry for the
 * complete list of character encodings. Not all encodings may be
 * available on any specific platform, but the ones that are mentioned
 * explicitely, and all conversions between them, are guaranteed to be
 * available.
 *
 * All functions take a \c fail argument. Pass true if you want an
 * exception when an input character could not be converted. This
 * could either be because the input contains bytes that does not
 * constitute a valid character according to the specified input
 * encoding, or it could be because a character is unrepresentable in
 * the output encoding. The default is to simply output a replacement
 * character in these cases. On request, the transcode function will
 * throw a <tt>TranscodeException</tt>, the streams will throw a \c
 * Core::ReadException or \c Core::WriteException depending on
 * direction, and the \c encode and \c decode methods of the codec
 * will throw Core::EncodeException or \c Core::DecodeException
 * depending on direction.
 *
 * \note The implementation is currently based on the \c iconv
 * transcoding facility, but unfortunately the POSIX specification of
 * \c iconv does not guarantee that the application is notified when a
 * character from the input is unrepresentable in the output
 * encoding. According to POSIX the result is undefined in this
 * case. The linux version, however, will let us know, and in this
 * case we can rely on getting a TranscodeException (or similar), if
 * we asked for it.
 *
 * Thread safety: All functions are thread safe. The returned
 * transcoders are guaranteed to be weakly thread-safe, that is, each
 * thread must use a separate instance (see \ref ThreadSafety).
 *
 * \sa http://www.iana.org/assignments/character-sets
 */

#ifndef ARCHON_UTIL_TRANSCODE_HPP
#define ARCHON_UTIL_TRANSCODE_HPP

#include <cwchar>
#include <stdexcept>
#include <string>

#include <archon/core/codec.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/core/stream.hpp>
#include <archon/core/utf16.hpp>


namespace Archon
{
  namespace Util
  {
    /**
     * Classical American 7-bit encoding.
     */
    extern std::string const transcode_US_ASCII;

    /**
     * ISO Latin 1 encoding.
     */
    extern std::string const transcode_ISO_8859_1;

    /**
     * ISO Latin 1 encoding with Euro sign.
     */
    extern std::string const transcode_ISO_8859_15;

    /**
     * ISO 8-bit variable length Unicode (UCS) encoding.
     */
    extern std::string const transcode_UTF_8;

    /**
     * ISO 16-bit little-endian variable length Unicode (UCS)
     * encoding.
     *
     * This one always stores a UTF-16 unit as two consecutive
     * bytes, even if a byte has 16 bits on the target platform.
     */
    extern std::string const transcode_UTF_16LE;

    /**
     * ISO 16-bit big-endian variable length Unicode (UCS) encoding.
     *
     * This one always stores a UTF-16 unit as two consecutive
     * bytes, even if a byte has 16 bits on the target platform.
     */
    extern std::string const transcode_UTF_16BE;

    /**
     * ISO 32-bit little-endian fixed length Unicode (UCS) encoding.
     *
     * This one always stores a UTF-32 unit as four consecutive
     * bytes, even if a byte has 16 (or even 32) bits on the target
     * platform.
     */
    extern std::string const transcode_UTF_32LE;

    /**
     * ISO 32-bit big-endian fixed length Unicode (UCS) encoding.
     *
     * This one always stores a UTF-32 unit as four consecutive
     * bytes, even if a byte has 16 (or even 32) bits on the target
     * platform.
     */
    extern std::string const transcode_UTF_32BE;

    /**
     * MS Windows expansion of US-ASCII which is incompatible with
     * ISO Latin 1
     */
    extern std::string const transcode_WINDOWS_1252;



    struct TranscodeException;
    struct TranscoderNotAvailableException;



    /**
     * Transcode the specified string. Both input and output are to
     * be thought of as strings of bytes rather than strings of
     * characters. A single character can consist of multiple bytes
     * in both the input and the output.
     *
     * Illegal charcters in the input will be converted to the
     * Unicode replacement character or another suitable replacement
     * character available in the target encoding, unless \c fail is
     * <tt>true</tt>.
     *
     * \param s The source string to be converted to the target
     * encoding. This string is in general understood as a sequence
     * of bytes, rather than characters.
     *
     * \param source_encoding The input characters are considered
     * encoded using this character encoding.
     *
     * \param target_encoding The output characters will be encoded
     * using this character encoding.
     *
     * \param fail Pass true if you want a \c TranscodeException
     * when an input character could not be converted. The default
     * is to output a replacement character in these cases.
     *
     * \return The result of the conversion. This string is to be
     * understood as a sequence of bytes rather than a sequence of
     * characters.
     *
     * \throw TranscodeException When \c fail is true and an input
     * character could not be converted. This will either be when
     * the input is malformed according to the specified source
     * encoding or when an input character cannot be represented in
     * the target encoding. If \c fail is false, it will never be
     * thrown.
     *
     * \throw TranscoderNotAvailableException If no transcoder is
     * available for the specified conversion.
     *
     * \sa http://www.iana.org/assignments/character-sets
     */
    std::string transcode(std::string s,
                          std::string source_encoding,
                          std::string target_encoding,
                          bool fail = false);



    /**
     * Construct a byte input stream that transcodes the characters
     * it receives from the wrapped byte input stream before
     * returning them.
     *
     * \param in The wrapped byte input stream whose characters will
     * be transcoded.
     *
     * \param source_encoding The characters returned from the
     * wrapped stream are considered encoded using this character
     * encoding.
     *
     * \param target_encoding The characters returned from the new
     * stream will be encoded using this character encoding.
     *
     * \param fail Pass true if you want the \c read method of the
     * new stream to throw a \c Core::ReadException when an input
     * character could not be converted. The default is to convert
     * it silently to a replacement character.
     *
     * \return The new byte stream that returns characters encoded
     * according to the target encoding.
     *
     * \throw TranscoderNotAvailableException If no transcoder is
     * available for the specified conversion.
     *
     * \sa http://www.iana.org/assignments/character-sets
     */
    Core::UniquePtr<Core::InputStream>
    get_transcoding_input_stream(Core::InputStream &in,
                                 std::string source_encoding,
                                 std::string target_encoding, bool fail = false);



    /**
     * Construct a byte output stream that transcodes the characters
     * that are written to it before writing them to the wrapped
     * byte output stream.
     *
     * \param out The wrapped output stream to which the transcoded
     * characters will be writtin.
     *
     * \param source_encoding The characters written to the new
     * stream are considered encoded using this character encoding.
     *
     * \param target_encoding The characters written to the wrapped
     * stream will be encoded using this character encoding.
     *
     * \param fail Pass true if you want the \c write method of the
     * new stream to throw a \c Core::WriteException when an output
     * character could not be converted. The default is to convert
     * it silently to a replacement character.
     *
     * \return The new byte stream that accepts characters encoded
     * according to the source encoding.
     *
     * \throw TranscoderNotAvailableException If no transcoder is
     * available for the specified conversion.
     *
     * \sa http://www.iana.org/assignments/character-sets
     */
    Core::UniquePtr<Core::OutputStream>
    get_transcoding_output_stream(Core::OutputStream &out,
                                  std::string source_encoding,
                                  std::string target_encoding, bool fail = false);



    /**
     * Get a codec object that assumes the specified external
     * character encoding, and assumes that UCS is used as the
     * internal character representation. Thus, the \c encode method
     * of the returned codec will encode a wide character string
     * using the specified encoding.
     *
     * \param encoding The desired character encoding for encoded
     * strings.
     *
     * \param fail Pass true if you want an exception when an input
     * character could not be converted. The default is to output a
     * replacement character in these cases.
     *
     * \return The requested codec object.
     */
    Core::UniquePtr<Core::WideCodec const>
    get_transcoding_codec(std::string encoding, bool fail = false);



    template<class InChar, class OutChar> struct BasicTranscoder
    {
      /**
       * This method is supposed to be called repeatedly to
       * progrssively transcode a stream of data.
       *
       * Conversion can stop for three reasons:
       *
       * 1) There is not enough data left in the input chunk to
       *    continue.
       *
       * 2) There is not enough free space left in the output chunk to
       *    continue.
       *
       * 3) The input data is invalid.
       *
       * If it runs out of input, it must return true. If it needs
       * more space for output, it must return false. Otherwise, in
       * case of invalid input, it must throw a
       * <tt>IncConvException</tt>.
       *
       * \param in_begin, in_end Specifies a chunk of input data to be
       *   transcoded. At entry \a in_begin must point to the first
       *   available input character, and \a in_end must point one
       *   beyond the last available input character. Before it
       *   returns, the conversion function updates the former to
       *   reflect the extent of successful conversion. The conversion
       *   function is able to handle an empty input chunk.
       *
       * \param out_begin, out_end Specifies a chunk of free space in
       *   which converted data can be stored. At entry \a out_begin
       *   must point to the first character of this chunk, and \a
       *   out_end must point one beyond the last. Before it returns,
       *   the conversion function will update the former to reflect
       *   the extent of successful conversion. The conversion
       *   function is able to handle an empty output chunk.
       *
       * \param eoi A flag that signals to the conversion function
       *   that the specified input chunk contains the last byte of
       *   input. The caller must eventually set this flag to
       *   true. The caller does not need to set it true, just because
       *   the last byte is in the input chunk, but it must eventually
       *   recognize this, and set the flag to true, and keep it set
       *   to tru for any remaining calls to the conversion
       *   function. See below for further details.
       *
       * The conversion function guarantees that its conversion state
       * is strictly advanced if at the same time the size of the
       * input and output chunks have a minimum size of 64 bytes. This
       * is a conservative estimate that no encoding scheme uses more
       * than 64 bytes per logical character. At the end of input
       * (when \c eoi is true), it guarantees strict advancement as
       * long as the output chunk has a minimum size of 64 bytes. This
       * is important for preventing an infinite loop in the caller.
       *
       * When the \a eoi argument is true, a return value of true must
       * be interpreted as 'successful completion of conversion', and
       * in this case the conversion function should not be called
       * again. If it is, the \a eoi argument must be set to true
       * again.
       *
       * \throw TranscodeException When \c fail was set to true when
       * the transcoder was constructed, and an input character could
       * not be converted. This will either be when the input is
       * malformed according to the specified input encoding or when
       * an input character cannot be represented in the output
       * encoding. If \c fail was set to false, it will never be
       * thrown.
       */
      virtual bool transcode(InChar const *&in_begin, InChar const *in_end,
                             OutChar *&out_begin, OutChar *out_end, bool eoi) = 0;



      virtual ~BasicTranscoder() {}
    };


    typedef BasicTranscoder<char, char>            Transcoder;
    typedef BasicTranscoder<char, Core::CharUtf16> TranscoderToUtf16;



    /**
     * Get a transcoder for doing low-level transcoding.
     *
     * Illegal charcters in the input will be converted to the Unicode
     * replacement character or another suitable replacement character
     * available in the output encoding, unless \c fail is
     * <tt>true</tt>.
     *
     * \param input_encoding The input characters will be assumed as
     * having been encoded using this character encoding scheme.
     *
     * \param output_encoding The ouput characters will be encoded
     * using this character encoding scheme.
     *
     * \param fail Pass true if you want a \c TranscodeException when
     * an input character could not be converted. The default is to
     * output a replacement character in these cases.
     *
     * \return A transcoder object on which you should invoke the
     * transcode() method.
     *
     * \throw TranscoderNotAvailableException If no transcoder is
     * available for the specified conversion.
     *
     * \sa http://www.iana.org/assignments/character-sets
     */
    Core::UniquePtr<Transcoder> get_transcoder(std::string input_encoding,
                                               std::string output_encoding,
                                               bool fail = false);



    /**
     * A convenience function for transcoding to UTF-16 when the
     * result is needed as a sequence of elements of type
     * Core::CharUtf16.
     *
     * Same as get_transcoder() but delivers result as a sequence of
     * characters of type Core::CharUtf16.
     */
    Core::UniquePtr<TranscoderToUtf16> get_transcoder_to_utf16(std::string input_encoding,
                                                               bool fail = false);






    struct TranscodeException: std::runtime_error
    {
      TranscodeException(std::string m): std::runtime_error(m) {}
    };

    struct TranscoderNotAvailableException: std::runtime_error
    {
      TranscoderNotAvailableException(std::string m): std::runtime_error(m) {}
    };
  }
}

#endif // ARCHON_UTIL_TRANSCODE_HPP
