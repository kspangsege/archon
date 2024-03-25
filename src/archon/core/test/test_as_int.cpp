// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <type_traits>
#include <array>

#include <archon/core/type_list.hpp>
#include <archon/core/demangle.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/core/as_int.hpp>
#include <archon/check.hpp>


using namespace archon;


namespace {


ARCHON_TEST_VARIANTS(variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


using types = core::TypeList<bool,
                             char,
                             signed char,
                             unsigned char,
                             wchar_t,
                             short,
                             unsigned short,
                             int,
                             unsigned,
                             long,
                             unsigned long,
                             long long,
                             unsigned long long>;


} // unnamed namespace


ARCHON_TEST_BATCH(Core_AsInt_AsFlexInt_Format, variants)
{
    using char_type = test_type;
    using value_formatter_type = core::BasicValueFormatter<char_type>;
    using string_widener_type = core::BasicStringWidener<char_type>;

    std::array<char_type, 8> seed_memory_1;
    value_formatter_type formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 8> seed_memory_2;
    string_widener_type widener(test_context.locale, seed_memory_2);

    auto test = [&, &parent_test_context = test_context](core::Wrap<auto> tag, int) {
        using value_type = decltype(tag)::type;
        ARCHON_TEST_TRAIL(parent_test_context, core::get_type_name<value_type>());
        if constexpr (std::is_unsigned_v<value_type>) {
            if constexpr (std::is_same_v<value_type, bool>) {
                value_type val_1 = false;
                value_type val_2 = true;
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_1)), widener.widen("0"));
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_2)), widener.widen("1"));
                bool format_as_hex = true;
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_1, format_as_hex)), widener.widen("0x0"));
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_2, format_as_hex)), widener.widen("0x1"));
            }
            else {
                value_type val_1 = 0;
                value_type val_2 = 37;
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_1)), widener.widen("0"));
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_2)), widener.widen("37"));
                bool format_as_hex = true;
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_1, format_as_hex)), widener.widen("0x0"));
                ARCHON_CHECK_EQUAL(formatter.format(core::as_flex_int(val_2, format_as_hex)), widener.widen("0x37"));
            }
        }
    };

    core::for_each_type<types>(test);
}
