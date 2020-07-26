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

#ifndef ARCHON_X_UNIT_TEST_X_PATTERN_BASED_TEST_ORDER_HPP
#define ARCHON_X_UNIT_TEST_X_PATTERN_BASED_TEST_ORDER_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <locale>

#include <archon/base/span.hpp>
#include <archon/base/string_matcher.hpp>
#include <archon/unit_test/test_config.hpp>


namespace archon::unit_test {


/// \brief Order unit tests using sequence of wildcard patterns.
///
/// This is a unit test ordering comparator that uses wildcard patterns to rank
/// the unit tests. Unit tests whose name match the first specified pattern will
/// be ordered before unit tests that do not match it. Of those that do not
/// match the first pattern, those that match the second pattern will be ordered
/// before those that do not match it, and so forth.
///
/// The specified wildcard patterns are assumes to be expressed in the
/// 'wildcard' syntax as defined in \ref base::StringMatcherBase::PatternType.
///
class PatternBasedTestOrder :
        public TestOrder {
public:
    explicit PatternBasedTestOrder(base::Span<std::string_view> patterns, const std::locale& = {});

    bool less(const TestDetails&, const TestDetails&) const noexcept override;

private:
    base::StringMatcher m_matcher;

    std::size_t get_pattern_index(const TestDetails&) const noexcept;
};








// Implementation


inline std::size_t PatternBasedTestOrder::get_pattern_index(const TestDetails& details) const
    noexcept
{
    std::size_t index = std::size_t(-1);
    m_matcher.match(details.name, index);
    return index;
}


} // namespace archon::unit_test

#endif // ARCHON_X_UNIT_TEST_X_PATTERN_BASED_TEST_ORDER_HPP
