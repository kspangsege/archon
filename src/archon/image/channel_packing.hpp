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

#ifndef ARCHON_X_IMAGE_X_CHANNEL_PACKING_HPP
#define ARCHON_X_IMAGE_X_CHANNEL_PACKING_HPP

/// \file


#include <archon/image/bit_field.hpp>


namespace archon::image {


/// \brief Channel packing specification for 2-channel pixel format.
///
/// An instantiation of this class template specifies a particular scheme for packing 2
/// channels into a single integer word. It implements \ref
/// Concept_Archon_Image_ChannelPacking, and can thus be used with \ref
/// image::PackedPixelFormat.
///
/// \sa \ref image::ThreeChannelPacking and \ref image::FourChannelPacking
///
template<int A, int B, int C, int D> class TwoChannelPacking {
public:
    static constexpr int width_1 = A;
    static constexpr int gap_1   = B;
    static constexpr int width_2 = C;
    static constexpr int gap_2   = D;

    static constexpr int num_fields = 2;

    static constexpr image::BitField fields[num_fields] = {
        { width_1, gap_1 },
        { width_2, gap_2 },
    };
};

using ChannelPacking_88  = image::TwoChannelPacking<8, 0, 8, 0>;


/// \brief Channel packing specification for 3-channel pixel format.
///
/// An instantiation of this class template specifies a particular scheme for packing 3
/// channels into a single integer word. It implements \ref
/// Concept_Archon_Image_ChannelPacking, and can thus be used with \ref
/// image::PackedPixelFormat.
///
/// \sa \ref image::TwoChannelPacking and \ref image::FourChannelPacking
///
template<int A, int B, int C, int D, int E, int F> class ThreeChannelPacking {
public:
    static constexpr int width_1 = A;
    static constexpr int gap_1   = B;
    static constexpr int width_2 = C;
    static constexpr int gap_2   = D;
    static constexpr int width_3 = E;
    static constexpr int gap_3   = F;

    static constexpr int num_fields = 3;

    static constexpr image::BitField fields[num_fields] = {
        { width_1, gap_1 },
        { width_2, gap_2 },
        { width_3, gap_3 },
    };
};

using ChannelPacking_332  = image::ThreeChannelPacking<3, 0, 3, 0, 2, 0>;
using ChannelPacking_233  = image::ThreeChannelPacking<2, 0, 3, 0, 3, 0>;
using ChannelPacking_555  = image::ThreeChannelPacking<5, 0, 5, 0, 5, 0>;
using ChannelPacking_565  = image::ThreeChannelPacking<5, 0, 6, 0, 5, 0>;
using ChannelPacking_888  = image::ThreeChannelPacking<8, 0, 8, 0, 8, 0>;


/// \brief Channel packing specification for 4-channel pixel format.
///
/// An instantiation of this class template specifies a particular scheme for packing 4
/// channels into a single integer word. It implements \ref
/// Concept_Archon_Image_ChannelPacking, and can thus be used with \ref
/// image::PackedPixelFormat.
///
/// \sa \ref image::TwoChannelPacking and \ref image::ThreeChannelPacking
///
template<int A, int B, int C, int D, int E, int F, int G, int H> class FourChannelPacking {
public:
    static constexpr int width_1 = A;
    static constexpr int gap_1   = B;
    static constexpr int width_2 = C;
    static constexpr int gap_2   = D;
    static constexpr int width_3 = E;
    static constexpr int gap_3   = F;
    static constexpr int width_4 = G;
    static constexpr int gap_4   = H;

    static constexpr int num_fields = 4;

    static constexpr image::BitField fields[num_fields] = {
        { width_1, gap_1 },
        { width_2, gap_2 },
        { width_3, gap_3 },
        { width_4, gap_4 },
    };
};

using ChannelPacking_4444 = image::FourChannelPacking<4, 0, 4, 0, 4, 0, 4, 0>;
using ChannelPacking_5551 = image::FourChannelPacking<5, 0, 5, 0, 5, 0, 1, 0>;
using ChannelPacking_8888 = image::FourChannelPacking<8, 0, 8, 0, 8, 0, 8, 0>;


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_CHANNEL_PACKING_HPP
