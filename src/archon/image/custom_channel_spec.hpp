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

#ifndef ARCHON_X_IMAGE_X_CUSTOM_CHANNEL_SPEC_HPP
#define ARCHON_X_IMAGE_X_CUSTOM_CHANNEL_SPEC_HPP

/// \file


#include <archon/image/color_space.hpp>


namespace archon::image {


/// \brief Channel specification based on any color space.
///
/// An instantiation of this template implements \ref Concept_Archon_Image_ChannelSpec, and
/// can thus be used with \ref image::IntegerPixelFormat and friends.
///
/// \tparam N The number of color channels in the color space passed to the constructor.
///
/// \tparam A If `true`, an alpha channel is present.
///
template<int N, bool A> class CustomChannelSpec {
public:
    static constexpr int num_color_channels = N;
    static constexpr bool has_alpha_channel = A;

    static constexpr int num_channels = num_color_channels + bool(has_alpha_channel);

    /// \brief Construct custon channel specification based on specified color space.
    ///
    /// This constructor constructs a custon channel specification based on the specified
    /// color space. The number of channels in that color space must match the number passed
    /// for template parameter \p N. If it does not, behavior is undefined.
    ///
    constexpr CustomChannelSpec(const image::ColorSpace& color_space) noexcept;

    constexpr auto get_color_space() const noexcept -> const image::ColorSpace&;

private:
    const image::ColorSpace& m_color_space;
};








// Implementation


template<int N, bool A>
constexpr auto CustomChannelSpec<N, A>::get_color_space() const noexcept -> const image::ColorSpace&
{
    return m_color_space;
}


template<int N, bool A>
constexpr CustomChannelSpec<N, A>::CustomChannelSpec(const image::ColorSpace& color_space) noexcept
    : m_color_space(color_space)
{
    ARCHON_ASSERT(m_color_space.get_num_channels() == num_color_channels);
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_CUSTOM_CHANNEL_SPEC_HPP
