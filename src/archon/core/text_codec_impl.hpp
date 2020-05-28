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

#ifndef ARCHON_X_CORE_X_TEXT_CODEC_IMPL_HPP
#define ARCHON_X_CORE_X_TEXT_CODEC_IMPL_HPP

/// \file


#include <archon/core/impl/text_codec_impl.hpp>


namespace archon::core {


/// \{
///
/// \brief Text codec implementation types.
///
/// These types implement \ref Concept_Archon_Core_TextCodecImpl, and can thus be used with
/// \ref core::GenericTextCodec as implementation types.
///
/// `WindowsTextCodecImpl` encodes and decodes newline characters in addition to encoding
/// and decoding characters as prescribed by the selected locale. Newline characters are
/// encoded and decoded according to the style used by Microsoft Windows (see \ref
/// newline_codec).
///
/// `PosixTextCodecImpl` encodes and decodes characters as prescribed by the `std::codecvt`
/// facet of the selected locale. It does not perform additional encoding and decoding of
/// newline characters. This is usually the wanted behavior on POSIX platforms.
///
/// `TextCodecImpl` is an alias of `WindowsTextCodecImpl` on the Windows platform. On all
/// other platforms, it is an alias of `PosixTextCodecImpl`.
///
/// \tparam C The type used to store unencoded characters.
///
/// \tparam T The character traits type.
///
/// \tparam D The type of the character codec to be used. It must be a type that satisfies
/// \ref Concept_Archon_Core_CharCodec. Note that newline translation is not the job of the
/// character codec.
///
/// These classes are empty (`std::is_empty`) when `D::is_degen` is true.
///
/// These classes use `D::Config` as their `Config` type, so with the default type for `D`,
/// they use \ref core::CharCodecConfig.
///
/// These classes define the type `char_codec_type` to be equal to `D`.
///
/// \sa \ref Concept_Archon_Core_TextCodecImpl.
///
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using TextCodecImpl =
    impl::TextCodecImpl<impl::PrimTextCodecImpl, D>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using PosixTextCodecImpl =
    impl::TextCodecImpl<impl::PrimPosixTextCodecImpl, D>;
template<class C, class T = std::char_traits<C>, class D = core::BasicCharCodec<C, T>> using WindowsTextCodecImpl =
    impl::TextCodecImpl<impl::PrimWindowsTextCodecImpl, D>;
/// \}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_CODEC_IMPL_HPP
