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

#ifndef ARCHON_X_DISPLAY_X_TYPES_HPP
#define ARCHON_X_DISPLAY_X_TYPES_HPP

/// \file


#include <ostream>

#include <archon/core/format.hpp>
#include <archon/core/with_modified_locale.hpp>
#include <archon/image/geom.hpp>


namespace archon::display {


/// \brief    
///
///    
///
using Size = image::Size;


/// \brief    
///
///    
///
using Pos = image::Pos;


/// \brief    
///
///    
///
using Box = image::Box;



/// \brief Horizontal and vertical screen resolution.
///
/// An object of this type specifies the horizontal and vertical resolutions of a screen.
///
/// To get the resolution in pixels per inch, multiply by 2.54 cm/in.
///
/// A resolution object can be formatted, i.e., it can be written to an output stream. The
/// format is `<horizontal>,<vertical>`. Within the two components, a dot (`.`) is used as
/// decimal point.
///
struct Resolution {
    /// \brief Pixels per centimeter in horizontal direction.
    ///
    /// This field specifies the number of pixels per centimeter in the horizontal
    /// direction.
    ///
    double horz_ppcm;

    /// \brief Pixels per centimeter in vertical direction.
    ///
    /// This field specifies the number of pixels per centimeter in the vertical direction.
    ///
    double vert_ppcm;
};


template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, display::Resolution) ->
    std::basic_ostream<C, T>&;








// Implementation


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, display::Resolution resl) -> std::basic_ostream<C, T>&
{
    out << core::with_reverted_numerics(core::formatted("%s,%s", resl.horz_ppcm, resl.vert_ppcm)); // Throws
    return out;
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_TYPES_HPP
