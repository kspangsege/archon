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


} // namespace archon::base

#endif // ARCHON_X_BASE_X_LOCALE_HPP
