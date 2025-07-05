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

#ifndef ARCHON_X_IMAGE_X_IMAGE_PROVIDER_HPP
#define ARCHON_X_IMAGE_X_IMAGE_PROVIDER_HPP

/// \file


#include <archon/image/geom.hpp>
#include <archon/image/image.hpp>


namespace archon::image {


/// \brief    
///
///    
///
class ImageProvider {
public:
    /// \brief    
    ///
    ///    
    ///
    virtual bool try_provide_image(image::Size image_size, image::Image::TransferInfo, image::Image*&, image::Pos&,
                                   std::error_code&) = 0;

    virtual ~ImageProvider() noexcept = default;
};


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_IMAGE_PROVIDER_HPP
