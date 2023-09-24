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

#ifndef ARCHON_X_CORE_X_TEXT_FILE_ERROR_HPP
#define ARCHON_X_CORE_X_TEXT_FILE_ERROR_HPP

/// \file


#include <system_error>

#include <archon/core/features.h>


namespace archon::core {


/// \brief Errors that can be generated through the use of text files.
///
/// These are errors that can be generated through the use of text files that are based on
/// \ref core::TextFileImpl. An example of such text file is \ref core::TextFile.
///
/// \sa \ref core::make_error_code(TextFileError)
///
enum class TextFileError {
    /// Invalid byte sequence while trying to decode character. A character could not be
    /// decoded, because the presented byte sequence was not a valid encoding of any
    /// character.
    invalid_byte_seq = 1,

    /// Invalid character value while trying to encode character. A character could not be
    /// encoded, because its value was outside the range of valid character values.
    invalid_char = 2,
};


/// \brief Make text file error code object.
///
/// This function makes a text file error code object. Together with the specialization of
/// `std::is_error_code_enum<T>` for \ref core::TextFileError, this allows for implicit
/// conversion from an enum value to an error code object.
///
auto make_error_code(core::TextFileError) noexcept -> std::error_code;








// Implementation


} // namespace archon::core

namespace std {


template<> class is_error_code_enum<archon::core::TextFileError> {
public:
    static constexpr bool value = true;
};


} // namespace std

namespace archon::core {

namespace impl {


class TextFileErrorCategory
    : public std::error_category {
public:
    auto name() const noexcept -> const char* override final;
    auto message(int) const -> std::string override final;
};


inline constinit const impl::TextFileErrorCategory g_text_file_error_category;


} // namespace impl


inline auto make_error_code(core::TextFileError err) noexcept -> std::error_code
{
    return std::error_code(int(err), impl::g_text_file_error_category);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_FILE_ERROR_HPP
