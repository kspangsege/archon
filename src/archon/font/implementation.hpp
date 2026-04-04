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

#ifndef ARCHON_X_FONT_X_IMPLEMENTATION_HPP
#define ARCHON_X_FONT_X_IMPLEMENTATION_HPP

/// \file


#include <memory>
#include <string_view>
#include <locale>

#include <archon/core/filesystem.hpp>
#include <archon/font/loader.hpp>


namespace archon::font {


/// \brief Particular implementation of font API.
///
/// An instance of this class represents a particular implementation of the font API. A
/// particular implementation can be available or unavailable (\ref is_available()). The
/// complete set of available and unavailable implementations can be iterated over, or
/// searched using \ref font::get_num_implementations(), \ref font::get_implementation(),
/// and \ref font::lookup_implementation(). The default implementation is available as \ref
/// font::get_default_implementation().
///
/// \sa \ref font::get_default_implementation()
/// \sa \ref font::get_num_implementations(), \ref font::get_implementation()
/// \sa \ref font::lookup_implementation()
/// \sa \ref font::get_fallback_implementation(), \ref font::get_freetype_implementation()
///
class implementation {
public:
    /// \brief Identifier for font implementation.
    ///
    /// This function returns the identifier for this font implementation. Each font
    /// implementation is supposed to return a unique identifier. An implementation can be
    /// looked up by identifier using \ref font::lookup_implementation().
    ///
    virtual auto get_ident() const noexcept -> std::string_view = 0;

    /// \brief Implementation description.
    ///
    /// This function returns the description of this font implementation. The description
    /// is supposed to be a short text that serves to identify the implementation in a
    /// broader context.
    ///
    virtual auto get_descr() const noexcept -> std::string_view = 0;

    /// \brief Whether implementation is available.
    ///
    /// This function returns true when and only when this implementation is available. An
    /// implementation is generally available if it was available and enabled during the
    /// building of the Archon Font Library.
    ///
    virtual bool is_available() const noexcept = 0;

    /// \brief Create new font loader.
    ///
    /// This function creates a new font loader that is tied to the font implementation. The
    /// caller must ensure that the lifetime of the returned loader does not extend beyond
    /// the lifetime of the implementation. This is trivially ensured for built in
    /// implementations. The returned loader object must be used by at most one thread at a
    /// time. Destruction counts as use.
    ///
    /// For applications that link against an installed font library, the specified resource
    /// directory path (\p resource_dir) must point to the directory in which font resources
    /// were installed (possibly `/usr/local/share/archon/font`). For applications linking
    /// against an uninstalled font library, such as during testing, the specified resource
    /// directory path must point to the `font` directory in the source tree (use \ref
    /// core::BuildEnvironment and \ref core::archon_source_from_build_path).
    ///
    /// FIXME: Clarify how to determine the resource directory path.
    ///
    virtual auto new_loader(core::FilesystemPathRef resource_dir, const std::locale& locale,
                            const font::loader::config& config = {}) const -> std::unique_ptr<font::loader> = 0;

    virtual ~implementation() noexcept = default;
};


/// \brief Get default font implementation.
///
/// This function retrieves the default font implementation. The default font implementation
/// is the first one in the built-in list of implementations that is available (\ref
/// font::implementation::is_available()). Since the fallback font implementation is always
/// available, there is always a default implementation. The built-in list is the one that
/// is accessed using \ref font::get_num_implementations() and \ref
/// font::get_implementation().
///
/// \sa \ref font::get_num_implementations(), \ref font::get_implementation()
/// \sa \ref font::implementation::is_available()
///
auto get_default_implementation() noexcept -> const font::implementation&;


/// \brief Number of font implementations.
///
/// This function returns the number of built-in font implementations (\ref
/// font::implementation). Each one can be retrieved using \ref font::get_implementation().
///
/// \sa \ref font::implementation
/// \sa \ref font::get_implementation()
///
int get_num_implementations() noexcept;


/// \brief Get font implementation by index.
///
/// This function returns the specified built-in font implementation (\ref
/// font::implementation). The implementation is specified in terms of its index within the
/// built-in list of available and unavailable implementations (\ref
/// font::implementation::is_available()). The number of implementations in the list can be
/// obtained by calling \ref font::get_num_implementations(). The order of implementations
/// in the list is fixed with the fallback implementation (\ref
/// font::get_fallback_implementation()) always coming last.
///
/// \sa \ref font::implementation
/// \sa \ref font::get_num_implementations()
///
auto get_implementation(int index) -> const font::implementation&;


/// \brief Lookup font implementation by identifier.
///
/// If the specified identifier matches one of the built-in font implementations (\ref
/// font::implementation), then this function returns that implementation regardless of
/// whether it is available (\ref font::implementation::is_available()). If the specified
/// identifier does not mach any implementations, this function returns null. The built-in
/// list of implementations is the one that is accessed using \ref
/// font::get_num_implementations() and \ref font::get_implementation().
///
/// \sa \ref font::implementation
/// \sa \ref font::get_num_implementations(), \ref font::get_implementation()
/// \sa \ref font::implementation::get_ident()
///
auto lookup_implementation(std::string_view ident) noexcept -> const font::implementation*;


} // namespace archon::font

#endif // ARCHON_X_FONT_X_IMPLEMENTATION_HPP
