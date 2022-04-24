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
#include <utility>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/with_modified_locale.hpp>


namespace archon::font {


/// \brief A particular font rendering size.
///
/// An object of this type specifies a particular font rendering size. It is to be
/// understood as the horizontal and vertical number of pixels in the EM-square. Note that
/// the numbers of pixels can be fractional.
///
struct Size {
    using comp_type = double;

    comp_type width, height;

    Size(comp_type = 0);
    Size(comp_type width, comp_type height);
};


template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, font::Size) -> std::basic_ostream<C, T>&;

template<class C, class T> bool parse_value(core::BasicValueParserSource<C, T>&, font::Size&);








// Implementation


inline Size::Size(comp_type val)
    : width(val)
    , height(val)
{
}


inline Size::Size(comp_type width_2, comp_type height_2)
    : width(width_2)
    , height(height_2)
{
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, font::Size size) -> std::basic_ostream<C, T>&
{
    std::array<font::Size::comp_type, 2> components = { size.width, size.height };
    std::size_t min_elems = 1;
    bool copy_last = true;
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    return out << core::with_reverted_numerics(core::as_list_a(components, min_elems, copy_last,
                                                               std::move(config))); // Throws
}


template<class C, class T> inline bool parse_value(core::BasicValueParserSource<C, T>& src, font::Size& size)
{
    std::array<font::Size::comp_type, 2> components = {};
    std::size_t min_elems = 1;
    bool copy_last = true;
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
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
