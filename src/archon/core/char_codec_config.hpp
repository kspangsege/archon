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

#ifndef ARCHON_X_CORE_X_CHAR_CODEC_CONFIG_HPP
#define ARCHON_X_CORE_X_CHAR_CODEC_CONFIG_HPP

/// \file


#include <archon/core/type.hpp>


namespace archon::core {


/// \brief Simple character codec configuration parameters.
///
/// Objects of this type are used to pass configuration parameters to simple character
/// codecs. See \ref core::BasicSimpleCharCodec.
///
template<class C, class T> using SimpleCharCodecConfig = core::Empty;


/// \brief Character codec configuration parameters.
///
/// Objects of this type are used to pass configuration parameters to character codecs. See
/// \ref core::BasicCharCodec.
///
template<class C, class T> struct CharCodecConfig
    : SimpleCharCodecConfig<C, T> {

    /// \brief Enable lenient mode.
    ///
    /// If set to `true`, the character codec will be put into lenient mode. In lenient
    /// mode, invalid byte sequences will be replaced with a replacement character during
    /// decoding, and invalid characters will be replaced with the encoding of a replacement
    /// character during encoding.
    ///
    bool lenient = false;

    /// \brief Use fallback replacement character.
    ///
    /// If set to `true`, the character codec will use the question mark of the ASCII
    /// character set as the replacement character, even in Unicode locales.
    ///
    bool use_fallback_replacement_char = false;
};


} // namespace archon::core

#endif // ARCHON_X_CORE_X_CHAR_CODEC_CONFIG_HPP
