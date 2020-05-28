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

#ifndef ARCHON_X_CORE_X_TEXT_FILE_CONFIG_HPP
#define ARCHON_X_CORE_X_TEXT_FILE_CONFIG_HPP

/// \file


#include <cstddef>

#include <archon/core/span.hpp>


namespace archon::core {


/// \brief Primitive text file implementation configuration parameters.
///
/// Objects of this type are used to carry text file configuration parameters pertaining
/// newline translation.
///
struct PrimTextFileImplConfig {
    /// \{
    ///
    /// \brief Newline translation buffer size and memory.
    ///
    /// `newline_codec_buffer_size` specifies the size of the buffer to be used during
    /// newline translation. `newline_codec_buffer_memory` specifies a chunk of memory that
    /// should be used as buffer during newline translation.
    ///
    /// If the text file implementation does not perform newline translation, these
    /// parameters are ignored.
    ///
    /// Otherwise, if the specified size is greater than the amount of specified memory, a
    /// buffer of the specified size will be dynamically allocated.
    ///
    /// Also, if the specified buffer size, or the amount of specified memory is too small
    /// for the text file implementation to operate, a larger buffer will be allocated,
    /// although the size will be kept as small as possible.
    ///
    static constexpr std::size_t default_newline_codec_buffer_size = 4096;
    std::size_t newline_codec_buffer_size = default_newline_codec_buffer_size;
    core::Span<char> newline_codec_buffer_memory;
    /// \}
};



/// \brief Text file implementation configuration parameters.
///
/// Objects of this type are used to carry configuration parameters targetted at text file
/// implementations such as \ref TextFileImpl, \ref PosixTextFileImpl, and \ref
/// WindowsTextFileImpl.
///
/// \tparam D The type of the character codec in use by the text file implementation.
///
template<class D> struct TextFileImplConfig
    : core::PrimTextFileImplConfig {

    /// \{
    ///
    /// \brief Character codec buffer size and memory.
    ///
    /// `char_codec_buffer_size` specifies the size of the buffer to be used during
    /// character encoding and decoding. `char_codec_buffer_memory` specifies a chunk of
    /// memory that should be used as buffer during character encoding and decoding.
    ///
    /// If the text file implementation uses a degenrate character codec (`D::is_degen`),
    /// these parameters are ignored.
    ///
    /// Otherwise, if the specified size is greater than the amount of specified memory, a
    /// buffer of the specified size will be dynamically allocated.
    ///
    /// Also, if the specified buffer size, or the amount of specified memory is too small
    /// for the text file implementation to operate, a larger buffer will be allocated,
    /// although the size will be kept as small as possible.
    ///
    static constexpr std::size_t default_char_codec_buffer_size = 4096;
    std::size_t char_codec_buffer_size = default_char_codec_buffer_size;
    core::Span<char> char_codec_buffer_memory;
    /// \}

    /// \brief Character codec configuration.
    ///
    /// Configuration parameters specific to the selected character codec.
    ///
    typename D::Config char_codec;
};



/// \brief Buffered text file implementation configuration parameters.
///
/// Objects of this type are used to carry configuration parameters targetted at buffered
/// text file implementations such as \ref core::BufferedTextFileImpl, \ref
/// core::BufferedPosixTextFileImpl, and \ref core::BufferedWindowsTextFileImpl.
///
/// \tparam S The sub-implementation type in use by the buffered text file implementation.
///
template<class S> struct BufferedTextFileImplConfig {
    /// \{
    ///
    /// \brief Text file buffer size and memory.
    ///
    /// `buffer_size` specifies the size of the buffer to be used by the buffered text file
    /// implementation. `buffer_memory` specifies a chunk of memory that should be used by
    /// the buffered text file implementation.
    ///
    /// If the specified size is greater than the amount of specified memory, a buffer of
    /// the specified size will be dynamically allocated.
    ///
    /// Also, if the specified buffer size, or the amount of specified memory is too small
    /// for the text file implementation to operate, a larger buffer will be allocated,
    /// although the size will be kept as small as possible.
    ///
    static constexpr std::size_t default_buffer_size = 4096;
    std::size_t buffer_size = default_buffer_size;
    core::Span<typename S::char_type> buffer_memory;
    /// \}

    /// \brief Sub-implementation configuration.
    ///
    /// Configuration parameters specific to the sub-implementation.
    ///
    typename S::Config subimpl;
};


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_FILE_CONFIG_HPP
