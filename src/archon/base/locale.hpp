// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_LOCALE_HPP
#define ARCHON_X_BASE_X_LOCALE_HPP

/// \file


#include <string_view>
#include <locale>

#include <archon/base/string.hpp>

/// \def ARCHON_ASSUME_UTF8_LOCALE
///
/// \brief Select how to detect UTF-8 locales.
///
/// The value assigned to this macro controls the behavior of \ref
/// assume_utf8_locale(). The default value (if none is specified), is `1`.
///
///   | Value  | Meaning
///   |--------|------------------------------------------------------------
///   | 0 (<1) | Do not assume that any locales are UTF-8 locales.
///   | 1      | Auto-detect (see \ref assume_utf8_locale()).
///   | 2 (>1) | Assume that all locales are UTF-8 locales.
///

#if !defined ARCHON_ASSUME_UTF8_LOCALE
#  define ARCHON_ASSUME_UTF8_LOCALE 1
#endif


namespace archon::base {


/// \brief Check validity of system locale name.
///
/// This function checks whether a locale with the specified name is available
/// on this platform. On POSIX systems, the check is done using `newlocale()`.
///
/// The same thing can be checked by passing the name to a constructor of
/// `std::locale` and being prepared to catch any std::runtime
/// exception. However, this approach is not as robust (as such an exception
/// could be thrown for other reasons), and the use of exceptions can be
/// problematic from a debugging perspective.
///
bool has_locale(const char* name);



/// \brief Detect UTF-8 locale.
///
/// Depending on the value of \ref ARCHON_ASSUME_UTF8_LOCALE, this function
/// either returns `false` for all locales, `true` for all locales, or attempts
/// to automatically detect whether the specified locale is a UTF-8 locale
/// (generally, if its name ends with `.UTF-8`). Here, a UTF-8 locale is to be
/// understood as one whose external (multi-byte) encoding is UTF-8.
///
/// \sa \ref ARCHON_ASSUME_UTF8_LOCALE.
///
bool assume_utf8_locale(const std::locale&);








// Implementation


inline bool assume_utf8_locale(const std::locale& locale)
{
    static_cast<void>(locale);
#if ARCHON_ASSUME_UTF8_LOCALE < 1
    return false;
#elif ARCHON_ASSUME_UTF8_LOCALE > 1
    return true;
#else
    std::string name = locale.name(); // Throws
    std::string_view name_2 = name;
    // The ".UTF8" form is for Windows compatibility.
    return (base::ends_with(name_2, ".UTF-8") || base::ends_with(name_2, ".UTF8")); // Throws
#endif
}

} // namespace archon::base

#endif // ARCHON_X_BASE_X_LOCALE_HPP
