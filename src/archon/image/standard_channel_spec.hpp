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

#ifndef ARCHON_X_IMAGE_X_STANDARD_CHANNEL_SPEC_HPP
#define ARCHON_X_IMAGE_X_STANDARD_CHANNEL_SPEC_HPP

/// \file


#include <archon/image/color_space.hpp>


namespace archon::image {


/// \brief Channel specification based on standard color space.
///
/// An instantiation of this template implements \ref Concept_Archon_Image_ChannelSpec, and
/// can thus be used with \ref image::IntegerPixelFormat and friends.
///
/// \tparam C The tag for the standard color space is use in this channel specifiaction.
///
/// \tparam A If `true`, an alpha channel is present.
///
template<image::ColorSpace::Tag C, bool A> class StandardChannelSpec {
public:
    static constexpr image::ColorSpace::Tag color_space_tag = C;
    static constexpr bool has_alpha_channel = A;

    static constexpr int num_channels = image::get_num_channels(C) + int(A);

    auto get_color_space() const noexcept -> const image::ColorSpace&;
};


using ChannelSpec_Lum  = image::StandardChannelSpec<image::ColorSpace::Tag::lum, false>;
using ChannelSpec_LumA = image::StandardChannelSpec<image::ColorSpace::Tag::lum, true>;
using ChannelSpec_RGB  = image::StandardChannelSpec<image::ColorSpace::Tag::rgb, false>;
using ChannelSpec_RGBA = image::StandardChannelSpec<image::ColorSpace::Tag::rgb, true>;








// Implementation


template<image::ColorSpace::Tag C, bool A>
inline auto StandardChannelSpec<C, A>::get_color_space() const noexcept -> const image::ColorSpace&
{
    return image::get_color_space(color_space_tag);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_STANDARD_CHANNEL_SPEC_HPP
