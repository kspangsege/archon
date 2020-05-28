// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_CORE_X_TEXT_FILE_IMPL_HPP
#define ARCHON_X_CORE_X_TEXT_FILE_IMPL_HPP

/// \file


#include <archon/core/char_codec.hpp>
#include <archon/core/impl/prim_text_file_impl.hpp>
#include <archon/core/impl/text_file_impl.hpp>
#include <archon/core/impl/buffered_text_file_impl.hpp>


namespace archon::core {


/// \{
///
/// \brief Text file implementation types.
///
/// These types implement \ref Concept_Archon_Core_TextFileImpl, and can thus be used with
/// \ref core::GenericTextFile and \ref core::GenericTextFileStream as implementation types.
///
/// Those with `Buffered` in their name additionally implement \ref
/// Concept_Archon_Core_BufferedTextFileImpl, and can thus be used with \ref
/// core::GenericBufferedTextFile.
///
/// `WindowsTextFileImpl` and `BufferedWindowsTextFileImpl` encode and decode newline
/// characters in addition to encoding and decoding characters as prescribed by the selected
/// locale. Newline characters are encoded and decoded according to the style used by
/// Microsoft Windows (see \ref newline_codec).
///
/// `PosixTextFileImpl` and `BufferedPosixTextFileImpl` encode and decode characters as
/// prescribed by the `std::codecvt` facet of the selected locale. They do not perform
/// additional encoding and decoding of newline characters. This is usually the wanted
/// behavior on POSIX platforms.
///
/// `TextFileImpl` is an alias of `WindowsTextFileImpl` on the Windows platform. On all
/// other platforms, it is an alias of `PosixTextFileImpl`.
///
/// `BufferedTextFileImpl` is an alias of `BufferedWindowsTextFileImpl` on the Windows
/// platform. On all other platforms, it is an alias of `BufferedPosixTextFileImpl`.
///
/// \tparam C The type used to store unencoded characters.
///
/// \tparam T The character traits type.
///
/// \tparam D The type of the character codec to be used. It must be a type that satisfies
/// \ref Concept_Archon_Core_CharCodec. Note that newline translation is not the job of the
/// character codec.
///
/// \tparam S The sub-implementation type to be used with the buffered text file
/// implementation. It must be a type that satisfies \ref Concept_Archon_Core_TextFileImpl.
///
/// These classes use \ref core::TextFileImplConfig or \ref core::BufferedTextFileImplConfig
/// as their `Config` type depending on whether the implementation is buffered or not.
///
/// These classes generate \ref core::TextFileError::invalid_byte_seq when the character
/// codec reports a decoding error, and they generate \ref core::TextFileError::invalid_char
/// when the character codec reports an encoding error.
///
/// These classes define the type `codec_type` to be equal to `D`.
///
/// `GenericBufferedTextFileImpl` defines the type `subimpl_type` to be equal to `S`.
///
/// Text file implementations of this kind all have a static compile-time constant member,
/// `is_buffered`, which is true if, and only if the implementation is buffered. For a
/// buffered implementation, `Config` is \ref core::BufferedTextFileImplConfig<S> with `S`
/// being `TextFileImpl<C, T, D>`, `PosixTextFileImpl<C, T, D>`, or `WindowsTextFileImpl<C,
/// T, D>`. For a non-buffered implementation, it is \ref core::TextFileImplConfig<D>. A
/// buffered implementation is one where, in the general case, when a single character is
/// written, nothing has to be done other than storing the character in a simple memory
/// buffer. Similarly, in the general case, when a single character is read, nothing has to
/// be done other than fetching it from a simple memory buffer.
///
/// Text file implementations of this kind all have a static compile-time constant member,
/// `has_windows_newline_codec`, which is true if, and only if the implementation transcodes
/// newline characters.
///
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using TextFileImpl =
    impl::TextFileImpl<impl::PrimTextFileImpl, D>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using PosixTextFileImpl =
    impl::TextFileImpl<impl::PrimPosixTextFileImpl, D>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using WindowsTextFileImpl =
    impl::TextFileImpl<impl::PrimWindowsTextFileImpl, D>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BufferedTextFileImpl =
    impl::BufferedTextFileImpl<TextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BufferedPosixTextFileImpl =
    impl::BufferedTextFileImpl<PosixTextFileImpl<C, T, D>>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using BufferedWindowsTextFileImpl =
    impl::BufferedTextFileImpl<WindowsTextFileImpl<C, T, D>>;
template<class S> using GenericBufferedTextFileImpl = impl::BufferedTextFileImpl<S>;
/// \}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_FILE_IMPL_HPP
