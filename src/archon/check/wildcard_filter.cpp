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


#include <archon/core/string.hpp>

#include <archon/check/wildcard_filter.hpp>


using namespace archon;
using check::WildcardFilter;


WildcardFilter::WildcardFilter(std::string_view filter, const std::locale& locale)
{
    core::StringMatcher::Builder::Config config;
    config.locale = locale;
    config.allow_interpattern_ambiguity = true;
    core::StringMatcher::Builder builder(config); // Throws
    auto make_include_matcher = [&] {
        // Include everything if no includes were specified.
        if (builder.empty())
            builder.add_pattern(core::StringMatcher::PatternType::wildcard, "*"); // Throws
        builder.build(m_include_matcher); // Throws
    };
    core::for_each_word(filter, [&](std::string_view word) {
        if (word != "-") {
            builder.add_pattern(core::StringMatcher::PatternType::wildcard, word); // Throws
        }
        else {
            make_include_matcher(); // Throws
            builder.clear();
        }
    }); // Throws
    if (m_include_matcher.is_degenerate()) {
        make_include_matcher(); // Throws
    }
    else {
        builder.build(m_exclude_matcher); // Throws
    }
}


bool WildcardFilter::include(const check::TestDetails& test_details) const noexcept
{
    return (m_include_matcher.match(test_details.name) && !m_exclude_matcher.match(test_details.name));
}
