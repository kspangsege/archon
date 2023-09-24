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

#ifndef ARCHON_X_CORE_X_MISC_ERRORS_HPP
#define ARCHON_X_CORE_X_MISC_ERRORS_HPP

/// \file


#include <system_error>

#include <archon/core/features.h>


namespace archon::core {


/// \brief Miscellaneous error codes.
///
/// This is a set of miscellaneous error codes intended to have wide applicability.
///
/// \sa \ref core::make_error_code(MiscError)
///
enum class MiscError {
    /// Unknow type of error. Placeholder for errors where there is no other approriate
    /// error code.
    other = 1,

    /// Operation not supported. An error that is generated when a requested functionality
    /// is not supported.
    operation_not_supported = 2,

    /// Premature end of input. An error that is generated when the end of input is reached
    /// prematurely.
    premature_end_of_input = 3,

    /// Delimiter not found. An error that is generated when a specific delimiter was not
    /// found in a case where it should have been found.
    delim_not_found = 4,
};


/// \brief Construct "miscellaneous" error code objects.
///
/// This function constructs error code objects from "miscellaneous" error codes (\ref
/// core::MiscError).
///
/// Together with the specialization of `std::is_error_code_enum<T>` for \ref
/// core::MiscError, this allows for implicit conversion from an enum value to an error code
/// object.
///
auto make_error_code(core::MiscError) noexcept -> std::error_code;








// Implementation


} // namespace archon::core

namespace std {


template<> class is_error_code_enum<archon::core::MiscError> {
public:
    static constexpr bool value = true;
};


} // namespace std

namespace archon::core {

namespace impl {


class MiscErrorCategory
    : public std::error_category {
public:
    auto name() const noexcept -> const char* override final;
    auto message(int) const -> std::string override final;
};


inline constinit const impl::MiscErrorCategory g_misc_error_category;


} // namespace impl


inline auto make_error_code(core::MiscError err) noexcept -> std::error_code
{
    return std::error_code(int(err), impl::g_misc_error_category);
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_MISC_ERRORS_HPP
