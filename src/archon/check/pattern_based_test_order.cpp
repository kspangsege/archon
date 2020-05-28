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


#include <archon/check/pattern_based_test_order.hpp>


using namespace archon;
using check::PatternBasedTestOrder;


PatternBasedTestOrder::PatternBasedTestOrder(core::Span<const std::string_view> patterns, const std::locale& locale)
{
    core::StringMatcher::Builder::Config config;
    config.locale = locale;
    config.allow_interpattern_ambiguity = true;
    core::StringMatcher::Builder builder(config); // Throws
    std::size_t n = patterns.size();
    for (std::size_t i = 0; i < n; ++i)
        builder.add_pattern(core::StringMatcher::PatternType::wildcard, patterns[i], i); // Throws
    m_matcher = builder.build(); // Throws
}


bool PatternBasedTestOrder::less(const check::TestDetails& a, const check::TestDetails& b) const noexcept
{
    std::size_t i = get_pattern_index(a);
    std::size_t j = get_pattern_index(b);
    return (i < j);
}
