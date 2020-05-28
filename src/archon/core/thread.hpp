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

#ifndef ARCHON_X_CORE_X_THREAD_HPP
#define ARCHON_X_CORE_X_THREAD_HPP

/// \file


#include <string_view>
#include <string>
#include <locale>


namespace archon::core {


/// \brief Set name of calling thread.
///
/// If supported by the platform, this function sets the name of the calling thread and
/// returns `true`. Otherwise this function returns `false`. When supported, the name will
/// be silently clamped to whatever limit the platform places on these names. Linux places a
/// limit of 15 characters for these names.
///
bool set_thread_name(std::string_view name, const std::locale& = {});


/// \brief Get name of calling thread.
///
/// If supported by the platform, this function fetches the name of the calling thread and
/// assigns it to \p name, then returns `true`. Otherwise this function returns `false` and
/// leaves \p name untouched.
///
bool get_thread_name(std::string& name, const std::locale& = {});


} // namespace archon::core

#endif // ARCHON_X_CORE_X_THREAD_HPP
