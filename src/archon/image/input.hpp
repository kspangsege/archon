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

#ifndef ARCHON_X_IMAGE_X_INPUT_HPP
#define ARCHON_X_IMAGE_X_INPUT_HPP

/// \file


#include <string_view>

#include <archon/core/source.hpp>


namespace archon::image {


/// \brief Reference to image byte source and optional file format hints.
///
/// An object of this type refers to a byte source, generally one that holds an image stored
/// according to a particular image file format. This object can optionally carry hints
/// about the file format with the purpose of aiding the automatic file format detection
/// scheme of the loading process. See \ref image::try_load_a().
///
/// \sa \ref image::try_load_a()
///
class Input {
public:
    /// \brief Reference to byte source.
    ///
    /// This is a reference to the byte source, which is generally supposed to be the
    /// contents of an image file.
    ///
    core::Source& source;

    /// \brief Optional MIME type hint.
    ///
    /// When nonempty, the specified string will be interpreted as a MIME type, and it will
    /// be taken into account during automatic image file format detection as described in
    /// the documentation of \ref image::try_load_a().
    ///
    std::string_view mime_type;

    /// \brief Optional filename extension hint.
    ///
    /// When nonempty, the specified string will be interpreted as a filename extension, and
    /// it will be taken into account during automatic image file format detection as
    /// described in the documentation of \ref image::try_load_a().
    ///
    /// \note The filename extension must always be specified with the leading dot (`.`)
    /// included. For example, for the PNG file format, it must be `".png"`. If the leading
    /// dot is not included, the hint will be without effect.
    ///
    std::string_view filename_extension;

    /// \brief Construct input object from byte source.
    ///
    Input(core::Source&) noexcept;
};








// Implementation


inline Input::Input(core::Source& s) noexcept
    : source(s)
{
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_INPUT_HPP
