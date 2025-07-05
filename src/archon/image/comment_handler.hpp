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

#ifndef ARCHON_X_IMAGE_X_COMMENT_HANDLER_HPP
#define ARCHON_X_IMAGE_X_COMMENT_HANDLER_HPP

/// \file


#include <string_view>


namespace archon::image {


/// \brief Interface for handlers of image file comments.
///
/// This class specifies the interface of a handler of image file comments. These are
/// comments that reside alongside image data inside image files. Only some image file
/// formats support this. In order to discover such comments during the loading of an image,
/// the application must instantiate an implementing subclass and pass it to \ref
/// image::load(), \ref image::try_load(), or \ref image::try_load_a() through \ref
/// image::FileFormat::LoadConfig::comment_handler.
///
/// A single image may contain multiple comments, so the application must be prepared to
/// receive multiple comments during the loading of a single image.
///
/// \sa \ref image::load(), \ref image::try_load(), \ref image::try_load_a()
/// \sa \ref image::FileFormat::LoadConfig::comment_handler
///
class CommentHandler {
public:
    /// \brief Handle single comment.
    ///
    /// This function is called for each comment in a loaded image file.
    ///
    /// The character encoding in the passed comment will be the native multi-byte encoding
    /// of the locale that was passed to to \ref image::load(), \ref image::try_load(), or
    /// \ref image::try_load_a().
    ///
    virtual void handle_comment(std::string_view comment) = 0;

    virtual ~CommentHandler() noexcept = default;
};


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_COMMENT_HANDLER_HPP
