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


#include <cstddef>
#include <array>
#include <string>

#include <archon/core/char_mapper.hpp>
#include <archon/core/string_matcher.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST_BATCH(Core_StringMatcher_WithoutInterpatternAmbiguityAndNotAllowed, char_variants)
{
    using char_type            = test_type;
    using traits_type          = std::char_traits<char_type>;
    using matcher_type         = core::BasicStringMatcher<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    typename matcher_type::Builder::Config config;
    config.locale = test_context.locale;
    config.allow_interpattern_ambiguity = false;
    typename matcher_type::Builder builder(config);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_*", 1);
    builder.add_pattern(matcher_type::PatternType::wildcard, "bar_*", 2);
    builder.add_pattern(matcher_type::PatternType::wildcard, "baz_*", 3);

    matcher_type matcher = builder.build();
    std::array<char_type, 16> seed_memory;
    string_widener_type widener(test_context.locale, seed_memory);
    std::size_t ident = 0;
    ARCHON_CHECK_NOT(matcher.match(widener.widen("x"), ident));
    ARCHON_CHECK(matcher.match(widener.widen("foo_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 1);
    ARCHON_CHECK(matcher.match(widener.widen("bar_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 2);
    ARCHON_CHECK(matcher.match(widener.widen("baz_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 3);
}


ARCHON_TEST_BATCH(Core_StringMatcher_WithoutInterpatternAmbiguityButAllowed, char_variants)
{
    using char_type            = test_type;
    using traits_type          = std::char_traits<char_type>;
    using matcher_type         = core::BasicStringMatcher<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    typename matcher_type::Builder::Config config;
    config.locale = test_context.locale;
    config.allow_interpattern_ambiguity = true;
    typename matcher_type::Builder builder(config);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_*", 1);
    builder.add_pattern(matcher_type::PatternType::wildcard, "bar_*", 2);
    builder.add_pattern(matcher_type::PatternType::wildcard, "baz_*", 3);

    matcher_type matcher = builder.build();
    std::array<char_type, 16> seed_memory;
    string_widener_type widener(test_context.locale, seed_memory);
    std::size_t ident = 0;
    ARCHON_CHECK_NOT(matcher.match(widener.widen("x"), ident));
    ARCHON_CHECK(matcher.match(widener.widen("foo_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 1);
    ARCHON_CHECK(matcher.match(widener.widen("bar_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 2);
    ARCHON_CHECK(matcher.match(widener.widen("baz_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 3);
}


ARCHON_TEST_BATCH(Core_StringMatcher_WithInterpatternAmbiguity, char_variants)
{
    using char_type            = test_type;
    using traits_type          = std::char_traits<char_type>;
    using matcher_type         = core::BasicStringMatcher<char_type, traits_type>;
    using string_widener_type  = core::BasicStringWidener<char_type, traits_type>;

    typename matcher_type::Builder::Config config;
    config.locale = test_context.locale;
    config.allow_interpattern_ambiguity = true;
    typename matcher_type::Builder builder(config);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_bar_baz_*", 1);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_bar_*",     2);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_*",         3);

    matcher_type matcher = builder.build();
    std::array<char_type, 16> seed_memory;
    string_widener_type widener(test_context.locale, seed_memory);
    std::size_t ident = 0;
    ARCHON_CHECK_NOT(matcher.match(widener.widen("x"), ident));
    ARCHON_CHECK(matcher.match(widener.widen("foo_bar_baz_x"), ident)) && ARCHON_CHECK_EQUAL(ident, 1);
    ARCHON_CHECK(matcher.match(widener.widen("foo_bar_x"),     ident)) && ARCHON_CHECK_EQUAL(ident, 2);
    ARCHON_CHECK(matcher.match(widener.widen("foo_x"),         ident)) && ARCHON_CHECK_EQUAL(ident, 3);
}


ARCHON_TEST_BATCH(Core_StringMatcher_FailOnInterpatternAmbiguity, char_variants)
{
    using char_type    = test_type;
    using traits_type  = std::char_traits<char_type>;
    using matcher_type = core::BasicStringMatcher<char_type, traits_type>;

    typename matcher_type::Builder::Config config;
    config.locale = test_context.locale;
    config.allow_interpattern_ambiguity = false;
    typename matcher_type::Builder builder(config);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_bar_baz_*", 1);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_bar_*",     2);
    builder.add_pattern(matcher_type::PatternType::wildcard, "foo_*",         3);
    ARCHON_CHECK_THROW_ANY(builder.build()); // FIXME: This check should be more precise                      
}
