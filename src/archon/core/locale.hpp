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

#ifndef ARCHON_X_CORE_X_LOCALE_HPP
#define ARCHON_X_CORE_X_LOCALE_HPP

/// \file


#include <string_view>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/string.hpp>


/// \def ARCHON_ASSUME_UNICODE_LOCALE
///
/// \brief Select how to detect Unicode locales.
///
/// The value assigned to this macro controls the behavior of \ref
/// archon::core::assume_unicode_locale(). The default value (if none is specified), is `1`.
///
///   | Value  | Meaning
///   |--------|---------------------------------------------------------------
///   | 0 (<1) | Do not assume that any locales are Unicode locales.
///   | 1      | Auto-detect (see \ref archon::core::assume_unicode_locale()).
///   | 2 (>1) | Assume that all locales are Unicode locales.
///
/// Be careful not to confuse this macro with \ref ARCHON_ASSUME_UTF8_LOCALE.
///
#if !defined ARCHON_ASSUME_UNICODE_LOCALE
#  define ARCHON_ASSUME_UNICODE_LOCALE 1
#endif


/// \def ARCHON_ASSUME_UTF8_LOCALE
///
/// \brief Select how to detect UTF-8 locales.
///
/// The value assigned to this macro controls the behavior of \ref
/// archon::core::assume_utf8_locale(). The default value (if none is specified), is `1`.
///
///   | Value  | Meaning
///   |--------|------------------------------------------------------------
///   | 0 (<1) | Do not assume that any locales are UTF-8 locales.
///   | 1      | Auto-detect (see \ref archon::core::assume_utf8_locale()).
///   | 2 (>1) | Assume that all locales are UTF-8 locales.
///
/// Be careful not to confuse this macro with \ref ARCHON_ASSUME_UNICODE_LOCALE.
///
#if !defined ARCHON_ASSUME_UTF8_LOCALE
#  define ARCHON_ASSUME_UTF8_LOCALE 1
#endif


/// \def ARCHON_ASSUME_LOCALE_HAS_ESCAPE
///
/// \brief Select how to detect whether locale has escape character.
///
/// The value assigned to this macro controls the behavior of \ref
/// archon::core::assume_locale_has_escape(). The default value (if none is specified), is
/// `1`.
///
///   | Value  | Meaning
///   |--------|------------------------------------------------------------------
///   | 0 (<1) | Do not assume that any locales have the escape character.
///   | 1      | Auto-detect (see \ref archon::core::assume_locale_has_escape()).
///   | 2 (>1) | Assume that all locales have the escape character.
///
/// See \ref archon::core::assume_locale_has_escape() for an explanation of what exactly it
/// means for a locale to have the escape character.
///
#if !defined ARCHON_ASSUME_LOCALE_HAS_ESCAPE
#  define ARCHON_ASSUME_LOCALE_HAS_ESCAPE 1
#endif



namespace archon::core {


/// \brief Check validity of system locale name.
///
/// This function checks whether a locale with the specified name is available on this
/// platform. On POSIX systems, the check is done using `newlocale()`.
///
/// The same thing can be checked by passing the name to a constructor of `std::locale` and
/// being prepared to catch any `std::runtime` exception. However, this approach is not as
/// robust (as such an exception could be thrown for other reasons), and the use of
/// exceptions can be problematic from a debugging perspective.
///
/// Unfortunately, this function is affected by race conditions / data races in the
/// implementation of newlocale() and/or freelocale() on several platforms. Specifically,
/// the thread sanitizer reports a data race when constructing a locale from the locale name
/// `C.UTF-8` when using Glibc 2.31 (https://sourceware.org/bugzilla/show_bug.cgi?id=28206),
/// and on macos, race conditions are detected by a malloc-level debugging feature
/// (https://feedbackassistant.apple.com/feedback/9461552).
///
/// Also, since the C++ locale system is generally built on top of newlocale() and
/// freelocale(), the mentioned problems also affect applications the use the C++ locale
/// system.
///
/// A workaround for the mentioned race condition problems is to arrange for all invocations
/// of has_locale(), and all constructions of locale objects from locale names to occur
/// early in the program, and before any additional threads are launched.
///
bool has_locale(const char* name);



/// \brief Detect Unicode locale.
///
/// Depending on the value of \ref ARCHON_ASSUME_UNICODE_LOCALE, this function either
/// returns `false` for all locales, `true` for all locales, or attempts to automatically
/// detect whether the specified locale is a Unicode locale. Here, a Unicode locale is to be
/// understood as one whose internal (wide character) encoding is UCS (Unicode). With
/// automatic detection, all locales will be considered to be Unicode locales if \ref
/// ARCHON_WCHAR_IS_UNICODE is true. Otherwise, on some platforms, a locale will still be
/// considered to be a Unicode locale if its name has a certain form.
///
/// Be careful not to confuse this function with \ref assume_utf8_locale().
///
/// \sa \ref ARCHON_ASSUME_UNICODE_LOCALE.
///
bool assume_unicode_locale(const std::locale&);



/// \brief Detect UTF-8 locale.
///
/// Depending on the value of \ref ARCHON_ASSUME_UTF8_LOCALE, this function either returns
/// `false` for all locales, `true` for all locales, or attempts to automatically detect
/// whether the specified locale is a UTF-8 locale (generally, if its name ends with
/// `.UTF-8`). Here, a UTF-8 locale is to be understood as one whose external (multi-byte)
/// encoding is UTF-8.
///
/// Be careful not to confuse this function with \ref assume_unicode_locale().
///
/// \sa \ref ARCHON_ASSUME_UTF8_LOCALE.
///
bool assume_utf8_locale(const std::locale&);



/// \brief Detect whether locale has escape character.
///
/// Depending on the value of \ref ARCHON_ASSUME_LOCALE_HAS_ESCAPE, this function either
/// returns `false` for all locales, `true` for all locales, or attempts to automatically
/// detect whether the specified locale has the escape character. Automatic detection
/// succeds precisely when both \ref core::assume_unicode_locale() and \ref
/// core::assume_utf8_locale() return `true` for the specified locale.
///
/// From the point of view of this function, no locale is understood as having the esape
/// character unless the character set used in the ordinary literal encoding contains the
/// escape character as defined by ASCII, and encoded in the same way as by ASCII (single
/// code unit of value 27). Subject to that condition, a locale is understood as having the
/// escape character if both the non-wide and wide execution character sets associated with
/// that locale contain the escape character, as it is defined by ASCII; the non-wide
/// encoding of the escape character agrees with ASCII; and the wide encoding can be
/// obtained by widening (i.e., by invocation of `std::ctype<wchar_t>::widen()`).
///
/// \sa \ref ARCHON_ASSUME_LOCALE_HAS_ESCAPE.
///
bool assume_locale_has_escape(const std::locale&);







// Implementation


inline bool assume_unicode_locale(const std::locale& loc)
{
    static_cast<void>(loc);
#if ARCHON_ASSUME_UNICODE_LOCALE < 1
    return false;
#elif ARCHON_ASSUME_UNICODE_LOCALE > 1
    return true;
#elif ARCHON_WCHAR_IS_UNICODE
    return true;
#elif ARCHON_APPLE
    return assume_utf8_locale(loc); // Throws
#else
    return false;
#endif
}


inline bool assume_utf8_locale(const std::locale& loc)
{
    static_cast<void>(loc);
#if ARCHON_ASSUME_UTF8_LOCALE < 1
    return false;
#elif ARCHON_ASSUME_UTF8_LOCALE > 1
    return true;
#else
    std::string name = loc.name(); // Throws
    std::string_view name_2 = name;
    // The ".UTF8" form is for Windows compatibility.
    using namespace std::literals;
    return (name_2.ends_with(".UTF-8"sv) || name_2.ends_with(".UTF8"sv));
#endif
}


inline bool assume_locale_has_escape(const std::locale& loc)
{
    static_cast<void>(loc);
#if ARCHON_ASSUME_LOCALE_HAS_ESCAPE < 1
    return false;
#elif ARCHON_ASSUME_LOCALE_HAS_ESCAPE > 1
    return true;
#else
    return (assume_unicode_locale(loc) && assume_utf8_locale(loc)); // Throws
#endif
}

} // namespace archon::core

#endif // ARCHON_X_CORE_X_LOCALE_HPP
