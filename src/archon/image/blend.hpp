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

#ifndef ARCHON_X_IMAGE_X_BLEND_HPP
#define ARCHON_X_IMAGE_X_BLEND_HPP

/// \file


#include <archon/image/comp_types.hpp>


namespace archon::image {


/// \brief    
///
///    
///
enum class BlendMode {
    over
};



/// \brief    
///
/// If the specified blend mode (\p mode) is OVER, this function computes `a OVER b` and stores the result in `c`.
///
/// FIXME: Explain: \p num_channels is including the alpha channel.    
///
/// FIXME: Explain: Colors are assumed to have an alpha components, and they must be represented according to the floating point component representation scheme (all channels are expressed in terms of linear intensity and alpha channel is pre-multiplied).                
///
/// FIXME: Explain: `c` is allowed to alias `a` or `b` (or both)        
///
void blend(const image::float_type* a, const image::float_type* b, image::float_type* c, int num_channels,
           image::BlendMode mode) noexcept;








// Implementation


inline void blend(const image::float_type* a, const image::float_type* b, image::float_type* c, int num_channels,
                  image::BlendMode mode) noexcept
{
    switch (mode) {
        case image::BlendMode::over:
            image::float_type alpha = a[num_channels - 1];
            auto beta = 1.0 - alpha;
            for (int i = 0; i < num_channels; ++i)
                c[i] = image::float_type(a[i] + beta * b[i]);
    }
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_BLEND_HPP
