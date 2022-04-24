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

#ifndef ARCHON_X_FONT_X_LOADER_HPP
#define ARCHON_X_FONT_X_LOADER_HPP

/// \file


#include <memory>
#include <utility>
#include <string_view>
#include <locale>

#include <archon/core/filesystem.hpp>
#include <archon/log/logger.hpp>
#include <archon/font/size.hpp>
#include <archon/font/face.hpp>


namespace archon::font {


class Loader {
public:
    struct Config;
    class Implementation;

    static auto new_default_loader(core::FilesystemPathRef resource_dir, const std::locale&) ->
        std::unique_ptr<Loader>;
    static auto new_default_loader(core::FilesystemPathRef resource_dir, const std::locale&, Config) ->
        std::unique_ptr<Loader>;

    virtual ~Loader() noexcept = default;

    /// \brief     
    ///
    ///    
    ///
    /// The caller must ensure that the returned font face object is destroyed before the
    /// font loader object is destroyed.
    ///
    /// Note that while a single font loader object can safely be accessed concurrently by
    /// multiple threads, behavior is undefined if multiple threads access a single font
    /// face object concurrently. I.e., font face objects are not thread-safe.
    ///
    virtual auto load_default_face() const -> std::unique_ptr<font::Face> = 0;

    virtual auto get_implementation() const noexcept -> const Implementation& = 0;

    static auto get_default_implementation() noexcept -> const Implementation&;
    static int get_num_implementations() noexcept;
    static auto get_implementation(int index) -> const Implementation&;
    static auto lookup_implementation(std::string_view ident) noexcept -> const Implementation*;
};


/// \brief Font loader configuration parameters.
///
/// These are the available parameters for configuring the the operation of a font loader
/// (\ref font::Loader).
///
struct Loader::Config {
    /// \brief Log through alternative logger.
    ///
    /// If a logger is specified, log messages will be routed through that logger.
    ///
    /// If a logger is not specified, messages will be routed to STDOUT.
    ///
    /// If a logger is specified, it must use a locale that is compatible with the locale
    /// that is specified during font loader construction (\ref
    /// Implementation::new_loader()). The important thing is that the character encodings
    /// agree (`std::codecvt` facet).
    ///
    /// The specified logger must be thread-safe.
    ///
    log::Logger* logger = nullptr;
};


class Loader::Implementation {
public:
    virtual auto ident() const noexcept -> std::string_view = 0;
    virtual auto new_loader(core::FilesystemPathRef resource_dir, const std::locale&, Config = {}) const ->
        std::unique_ptr<Loader> = 0;

    virtual ~Implementation() noexcept = default;
};








// Implementation


inline auto Loader::new_default_loader(core::FilesystemPathRef resource_dir,
                                       const std::locale& loc) -> std::unique_ptr<Loader>
{
    return new_default_loader(resource_dir, loc, {}); // Throws
}


inline auto Loader::new_default_loader(core::FilesystemPathRef resource_dir, const std::locale& loc,
                                       Config config) -> std::unique_ptr<Loader>
{
    return get_default_implementation().new_loader(resource_dir, loc, std::move(config)); // Throws
}


} // namespace archon::font

#endif // ARCHON_X_FONT_X_LOADER_HPP
