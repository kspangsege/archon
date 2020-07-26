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


/// \file

#ifndef ARCHON__UNIT_TEST__WILDCARD_FILTER_HPP
#define ARCHON__UNIT_TEST__WILDCARD_FILTER_HPP

#include <string_view>
#include <locale>

#include <archon/base/string_matcher.hpp>
#include <archon/unit_test/test_details.hpp>
#include <archon/unit_test/test_config.hpp>


namespace archon::unit_test {


/// \brief Filter unit tests using wildcard patterns.
///
/// A test is included if its name is matched by one of the specified "include"
/// patterns unless is also matched by one of the specified "exclude" patterns.
///
/// EBNF:
///
///     filter = { include-pattern }, [ '-', { exclude-pattern } ]
///     include-pattern = pattern
///     exclude-pattern = pattern
///
/// Each pattern is a string containing no white-space, and optionally containg
/// `*` wildcards. Each `*` matches zero or more arbitrary characters.
///
/// An empty filter is functionally equivalent to `*` and a filter on the form
/// `- ...` is equivalent to `* - ...`.
///
/// Note that the empty string, `*`, `* -`, and `-` all mean
/// "everything". Likewise, both `- *` and `* - *` means "nothing".
///
/// For example, `Foo Bar*` will inlcude only the `Foo` test and those whose
/// names start with `Bar`. Another example is `Foo* - Foo2 *X`, which will
/// include all tests whose names start with `Foo`, except `Foo2` and those
/// whose names end with an `X`.
///
class WildcardFilter :
        public Filter {
public:
    explicit WildcardFilter(std::string_view filter, const std::locale& = {});

    bool include(const TestDetails&) const noexcept override final;

private:
    base::StringMatcher m_include_matcher;
    base::StringMatcher m_exclude_matcher;
};


} // namespace archon::unit_test

#endif // ARCHON__UNIT_TEST__WILDCARD_FILTER_HPP
