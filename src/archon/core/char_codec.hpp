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

#ifndef ARCHON_X_CORE_X_CHAR_CODEC_HPP
#define ARCHON_X_CORE_X_CHAR_CODEC_HPP

/// \file


#include <string>

#include <archon/core/impl/char_codec.hpp>


namespace archon::core {


/// \brief Locale-based character codec with optional leniency mode.
///
/// Based on \ref core::BasicSimpleCharCodec, and adds support for leniency mode.
///
/// Implements \ref Concept_Archon_Core_CharCodec.
///
/// Uses \ref core::CharCodecConfig<C, T> as `Config` type.
///
/// When the lenient mode is selected (\ref core::CharCodecConfig::lenient), neither the
/// decode nor the encode operation will ever report an error. This means that when any of
/// the two functions return `false` they are guaranteed to set \p error to false.
///
template<class C, class T = std::char_traits<C>> using BasicCharCodec = impl::CharCodec<C, T>;


using CharCodec     = BasicCharCodec<char>;
using WideCharCodec = BasicCharCodec<wchar_t>;



/// \brief Simple locale-based character codec.
///
/// This class is a simple wrapper around `std::codecvt`, which covers up many of its
/// quirks.
///
/// Implements \ref Concept_Archon_Core_CharCodec.
///
/// Uses \ref core::SimpleCharCodecConfig<C, T> as `Config` type.
///
template<class C, class T = std::char_traits<C>> using BasicSimpleCharCodec = impl::SimpleCharCodec<C, T>;


using SimpleCharCodec     = BasicSimpleCharCodec<char>;
using WideSimpleCharCodec = BasicSimpleCharCodec<wchar_t>;


} // namespace archon::core

#endif // ARCHON_X_CORE_X_CHAR_CODEC_HPP
