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

#ifndef ARCHON_X_CHECK_X_PATTERN_BASED_TEST_ORDER_HPP
#define ARCHON_X_CHECK_X_PATTERN_BASED_TEST_ORDER_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <locale>

#include <archon/core/span.hpp>
#include <archon/core/string_matcher.hpp>
#include <archon/check/test_config.hpp>


namespace archon::check {


/// \brief Order test cases using sequence of wildcard patterns.
///
/// This is a test case ordering comparator that uses wildcard patterns to rank the test
/// cases. Test cases whose name match the first specified pattern will be ordered before
/// test cases that do not match it. Of those that do not match the first pattern, those
/// that match the second pattern will be ordered before those that do not match it, and so
/// forth.
///
/// The specified wildcard patterns are assumes to be expressed in the 'wildcard' syntax as
/// defined in \ref core::StringMatcherBase::PatternType.
///
/// If the specified list of patterns is empty, behavior is as if a single match everything
/// pattern (`*`) was specified.
///
class PatternBasedTestOrder
    : public TestOrder {
public:
    explicit PatternBasedTestOrder(core::Span<const std::string_view> patterns, const std::locale& = {});

    bool less(const TestDetails&, const TestDetails&) const noexcept override;

private:
    core::StringMatcher m_matcher;

    auto get_pattern_index(const TestDetails&) const noexcept -> std::size_t;
};








// Implementation


inline auto PatternBasedTestOrder::get_pattern_index(const TestDetails& details) const noexcept -> std::size_t
{
    std::size_t index = std::size_t(-1);
    m_matcher.match(details.name, index);
    return index;
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_PATTERN_BASED_TEST_ORDER_HPP
