// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_FONT_X_LOADER_HPP
#define ARCHON_X_FONT_X_LOADER_HPP

/// \file


#include <memory>
#include <locale>

#include <archon/core/typed_object_registry.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/log/logger.hpp>
#include <archon/font/face.hpp>


namespace archon::font {


/// \brief Font loader tied to specific implementation.
///
/// An instance of this class represents an ordered collection of font faces. The number of
/// faces is obtained by calling \ref get_num_faces() and each face can be loaded using \ref
/// load_face(). The loader designates a particular face as the default face. This implies
/// that there is always at least one face in the collection. The default font face is
/// loaded using \ref load_default_face().
///
/// New font loader objects are created by, or in the context of a particular font
/// implementation (\ref font::implementation). A font loader is tied to that particular
/// font implementation. A font loader object must never outlive its implementation object.
///
/// A font loader aggregate, which is the loader object together with any font face objects
/// tied to it, must never be accessed by more than one thread at a time. On the other hand,
/// two threads can safely work with separate font loader aggregates. Destruction of a
/// loader or face object counts as aggregate access.
///
/// \sa \ref font::implementation::new_loader()
/// \sa \ref font::new_default_loader()
/// \sa \ref font::new_freetype_loader_from_font_file()
///
class loader {
public:
    struct config;

    /// \brief Load default font face.
    ///
    /// This function loads the default font face. The caller must ensure that the lifetime
    /// of the returned face object does not extend beyond the lifetime of the loader.
    ///
    /// \sa \ref load_face()
    ///
    virtual auto load_default_face() -> std::unique_ptr<font::face> = 0;

    /// \brief Number of available font faces.
    ///
    /// This function returns the number of font faces in the collection represented by the
    /// font loader. Each of those font faces can be loaded using \ref load_face(). There is
    /// always at least one face in the collection.
    ///
    virtual int get_num_faces() = 0;

    /// \brief Load font face at particular index.
    ///
    /// This function loads the font face at the specified index. The index refers to the
    /// position of the font face in the ordered collection represented by the font
    /// loader. Number of available faces is returned by \ref get_num_faces(). The caller
    /// must ensure that the lifetime of the returned face object does not extend beyond the
    /// lifetime of the loader.
    ///
    /// \sa \ref load_default_face()
    ///
    virtual auto load_face(int face_index) -> std::unique_ptr<font::face> = 0;

    virtual ~loader() noexcept = default;
};


/// \brief Font loader configuration parameters.
///
/// These are the available parameters for configuring the the operation of a font loader
/// (\ref font::loader).
///
struct loader::config {
    /// \brief Log through alternative logger.
    ///
    /// If a logger is specified, log messages will be routed through that logger. Otherwise
    /// all logging will be inhibited.
    ///
    /// If a logger is specified, it must use a locale that is compatible with the locale
    /// that is specified during font loader construction (\ref
    /// font::implementation::new_loader()). The important thing is that the character
    /// encodings agree (`std::codecvt` facet).
    ///
    log::Logger* logger = nullptr;

    /// \brief Base class for sub-configurations.
    ///
    /// Base class for sub-configurations. See \ref sub.
    ///
    struct subconfig {};

    /// \brief Registry of implementation-specific sub-configurations.
    ///
    /// Registry of sub-configurations for particular implementations. For example, the
    /// application can pass FreeType-specific parameters to the loader constructor as
    /// follows:
    ///
    /// \code{.cpp}
    ///
    ///   archon::font::freetype_subconfig freetype_subconfig;
    ///   // set parameters in freetype_subconfig...
    ///   archon::font::loader::config config;
    ///   config.sub.register_(freetype_subconfig);
    ///   loader = archon::font::new_default_loader(resource_dir, locale, config);
    ///
    /// \endcode
    ///
    /// Note that the application is responsible for keeping the sub-configuration object
    /// alive for as long as the configuration object remains in use. The constructed loader
    /// object copies the configuration it needs. It does not keep a reference to the
    /// configuration object.
    ///
    /// Note that FreeType-specific parameters have effect only if the FreeType-based
    /// implementation is selected by \ref font::new_default_loader(). If a different
    /// implementation is selected, the FreeType-specific parameters will be ignored.
    ///
    /// \sa \ref font::freetype_subconfig
    /// \sa \ref core::TypedObjectRegistry::register_()
    ///
    core::TypedObjectRegistry<subconfig, 4> sub;
};


/// \brief Create font loader for default implementation.
///
/// This function is shorthand for calling \ref font::implementation::new_load() on the
/// implementation obtained by calling \ref font::get_default_implementation().
///
auto new_default_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                        const font::loader::config& config = {}) -> std::unique_ptr<font::loader>;


} // namespace archon::font

#endif // ARCHON_X_FONT_X_LOADER_HPP
