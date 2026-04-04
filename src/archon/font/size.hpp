// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_FONT_X_SIZE_HPP
#define ARCHON_X_FONT_X_SIZE_HPP

/// \file


#include <cstddef>
#include <compare>
#include <utility>
#include <array>
#include <ostream>

#include <archon/core/features.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/core/as_list.hpp>


namespace archon::font {


/// \brief A particular font rendering size.
///
/// An object of this type specifies a particular font rendering size. It is to be
/// understood as the horizontal and vertical number of pixels in the EM-square. Note that
/// the numbers of pixels can be fractional.
///
/// Font rendering sizes are comparable. Comparison is lexicographical over its two
/// components.
///
/// Font rendering sizes can be formatted (written to an output stream), and can be parsed
/// through a value parser (\ref core::BasicValueParserSource).
///
/// When a font rendering size is formatted, if the two components are equal, only one
/// component is shown. For example, the size `{ 16, 16 }` is formatted as just `16`. When
/// the two components are different, both components are shown and are separated by an
/// `x`. For example, the size `{ 16, 17 }` is formatted as `16x17`.
///
/// When a font rendering size is parsed, if there is only one value, that value is used for
/// both components. If there are two values, they must be separated by an `x`.
///
class size {
public:
    using comp_type = double;

    comp_type width  = 0;
    comp_type height = 0;

    constexpr size() noexcept = default;
    constexpr size(comp_type size) noexcept;
    constexpr size(comp_type width, comp_type height) noexcept;

    constexpr auto operator<=>(const size&) const noexcept = default;
};


template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, font::size) -> std::basic_ostream<C, T>&;

template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>&, font::size&);








// Implementation


constexpr size::size(comp_type size_2) noexcept
    : size(size_2, size_2)
{
}


constexpr size::size(comp_type width_2, comp_type height_2) noexcept
    : width(width_2)
    , height(height_2)
{
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, font::size size) -> std::basic_ostream<C, T>&
{
    std::array<font::size::comp_type, 2> components = { size.width, size.height };
    std::size_t min_elems = 1;
    bool copy_last = true;
    core::AsListConfig config;
    config.separator = 'x';
    config.space = core::AsListSpace::none;
    return out << core::with_reverted_numerics(core::as_list_a(components, min_elems, copy_last,
                                                               std::move(config))); // Throws
}


template<class C, class T> inline bool parse_value(core::BasicValueParserSource<C, T>& src, font::size& size)
{
    std::array<font::size::comp_type, 2> components = {};
    std::size_t min_elems = 1;
    bool copy_last = true;
    core::AsListConfig config;
    config.separator = 'x';
    config.space = core::AsListSpace::none;
    bool success = src.delegate(core::with_reverted_numerics(core::as_list_a(components, min_elems, copy_last,
                                                                             std::move(config)))); // Throws
    if (ARCHON_LIKELY(success)) {
        size = { components[0], components[1] };
        return true;
    }
    return false;
}


} // namespace archon::font

#endif // ARCHON_X_FONT_X_SIZE_HPP
