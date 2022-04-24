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

#ifndef ARCHON_X_IMAGE_X_PALETTE_GENERATOR_HPP
#define ARCHON_X_IMAGE_X_PALETTE_GENERATOR_HPP

/// \file


#include <cstddef>    

#include <archon/core/assert.hpp>    


namespace archon::image {


/// \brief    
///
///    
///
class PaletteGenerator {
public:
    void generate(image::Reader&);

private:
    core::Buffer<word_type> m_colors;
};








// Implementation


void PaletteGenerator::generate(image::Reader& reader)
{
    std::size_t num_words = 1;
    core::int_mul(num_words, num_shannels); // Throws
    m_colors.reset(num_words); // Throws
    // Allocate 1-D buffer of 565 packed format pixels
    // Copy all pixels from image into buffer while converting to the 565 packed format, do this by overlaying memory with image of same size but using the packed format.
    // While palette is not full:
    //   Find bucket with largest number of pixels
    //   Find channel with largest range in bucket
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_PALETTE_GENERATOR_HPP
