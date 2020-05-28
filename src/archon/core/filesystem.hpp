// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_FILESYSTEM_HPP
#define ARCHON_X_CORE_X_FILESYSTEM_HPP

/// \file


#include <array>
#include <utility>
#include <string_view>
#include <string>
#include <locale>
#include <ostream>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/string_codec.hpp>
#include <archon/core/value_parser.hpp>


namespace archon::core {


/// \brief Replacement type of file system path arguments.
///
/// This class is intended to be used as a replacement for argument types that would
/// otherwise be `std::filesystem::path`. Its purpose is to avoid implicit conversion from a
/// string type to `std::filesystem::path`. Such a conversion is generally unwanted as it
/// uses the global locale rather than the appropriate locale of the context in which the
/// conversion would happen.
///
/// \sa \ref core::FilesystemPathRef.
///
class FilesystemPath {
public:
    FilesystemPath(std::filesystem::path&&) noexcept;

    operator std::filesystem::path&&() && noexcept;

private:
    std::filesystem::path m_path;
};



/// \brief Replacement type of file system path arguments.
///
/// This class is intended to be used as a replacement for argument types that would
/// otherwise be `const std::filesystem::path&`. Its purpose is to avoid implicit conversion
/// from a string type to `std::filesystem::path`. Such a conversion is generally unwanted
/// as it uses the global locale rather than the appropriate locale of the context in which
/// the conversion would happen.
///
/// \sa \ref core::FilesystemPath.
///
class FilesystemPathRef {
public:
    using value_type = std::filesystem::path::value_type;

    FilesystemPathRef(const std::filesystem::path&) noexcept;

    operator const std::filesystem::path&() const noexcept;

    auto get() const noexcept -> const std::filesystem::path&;

    auto c_str() const noexcept -> const value_type*;

private:
    const std::filesystem::path& m_path;
};



/// \{
///
/// \brief Construct file system path.
///
/// These functions are shorthands for calling \ref core::make_fs_path() with the
/// corresponding format argument.
///
auto make_fs_path_generic(std::string_view path, const std::locale&) -> std::filesystem::path;
auto make_fs_path_native(std::string_view path, const std::locale&)  -> std::filesystem::path;
auto make_fs_path_auto(std::string_view path, const std::locale&)    -> std::filesystem::path;
/// \}



/// \brief Construct file system path from specified format.
///
/// The purpose of this function is to hide a number of problems with direct use of the
/// corresponding `std::filesystem::path` constructor in LLVM libc++ (LLVM 10.0).
///
auto make_fs_path(std::string_view path, const std::locale&, std::filesystem::path::format) -> std::filesystem::path;



/// \{
///
/// \brief Convert file system path to string.
///
/// These functions convert a file system path to a string representation using either the
/// generic or the native path syntax.
///
auto path_to_string_generic(core::FilesystemPathRef, const std::locale&) -> std::string;
auto path_to_string_native(core::FilesystemPathRef, const std::locale&)  -> std::string;
/// \}



/// \{
///
/// \brief Format and parse file system paths.
///
/// If the referenced path object is not `const`, the object returned by these functions can
/// be used for both formatting and parsing. Formatting happens when the returned object is
/// passed to the stream output operator (`operator<<` on `std::basic_ostream`). Parsing
/// happens when it is passed to \ref core::BasicValueParser::parse().
///
/// If the referenced path object is `const`, only formatting is possible.
///
/// `as_generic_path()` will format the path according to the generic syntax
/// (`std::filesystem::path::native_format`).
///
/// `as_native_path()` will format the path according to the native syntax
/// (`std::filesystem::path::native_format`).
///
/// If \p lenient is true, and the returned object is used for parsing, syntax
/// auto-detection will be enabled (`std::filesystem::path::auto_format`).
///
auto as_generic_path(std::filesystem::path& path, bool lenient = false) noexcept;
auto as_generic_path(const std::filesystem::path& path, bool lenient = false) noexcept;
auto as_native_path(std::filesystem::path& path, bool lenient = false) noexcept;
auto as_native_path(const std::filesystem::path& path, bool lenient = false) noexcept;
/// \}



/// \brief Check for trailing directory separator.
///
/// This function returns `true` if, and only if the specified file system path has a
/// trailing directory separator (`/`). A directory separator that is part of a root path
/// (`std::filesystem::path::root_path()`) is not considered a trailing directory separator
/// by this function.
///
bool has_trailing_slash(core::FilesystemPathRef);



/// \brief Remove trailing directory separator.
///
/// If the specified file system path has a trailing directory separator (\ref
/// core::has_trailing_slash()), this function removes it.
///
void remove_trailing_slash(std::filesystem::path&);



/// \brief Remove trailing directory separator.
///
/// If the specified file system path does not have a trailing directory separator (\ref
/// core::has_trailing_slash()), this function adds one. If the specified path was empty,
/// this function first changes it to `.`.
///
void add_trailing_slash(std::filesystem::path&);



/// \brief Replace dot with empty file system path.
///
/// If the specified file system path is a single dot (`.`), this function replaces it with
/// the empty file system path.
///
void dot_to_empty(std::filesystem::path&);



/// \{
///
/// \brief Get dot and dot-dot file system paths.
///
/// These functions return the `.` and `..` file system paths respectively.
///
auto get_fs_dot_path() -> const std::filesystem::path&;
auto get_fs_dot_dot_path() -> const std::filesystem::path&;
/// \}








// Implementation


// ============================ FilesystemPath ============================


inline FilesystemPath::FilesystemPath(std::filesystem::path&& path) noexcept
    : m_path(std::move(path))
{
}


inline FilesystemPath::operator std::filesystem::path&&() && noexcept
{
    return std::move(m_path);
}



// ============================ FilesystemPathRef ============================


inline FilesystemPathRef::FilesystemPathRef(const std::filesystem::path& path) noexcept
    : m_path(path)
{
}


inline FilesystemPathRef::operator const std::filesystem::path&() const noexcept
{
    return m_path;
}


inline auto FilesystemPathRef::get() const noexcept -> const std::filesystem::path&
{
    return m_path;
}


inline auto FilesystemPathRef::c_str() const noexcept -> const value_type*
{
    return m_path.c_str();
}



// ============================ * * * ============================


inline auto make_fs_path_generic(std::string_view path, const std::locale& locale) -> std::filesystem::path
{
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped enumeration, but
    // LLVM libc++ (LLVM 10.0) makes it a scoped enumeration, so we are forced to use extra
    // qualification here.
    return make_fs_path(path, locale, fs::path::format::generic_format);
}


inline auto make_fs_path_native(std::string_view path, const std::locale& locale) -> std::filesystem::path
{
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped enumeration, but
    // LLVM libc++ (LLVM 10.0) makes it a scoped enumeration, so we are forced to use extra
    // qualification here.
    return make_fs_path(path, locale, fs::path::format::native_format);
}


inline auto make_fs_path_auto(std::string_view path, const std::locale& locale) -> std::filesystem::path
{
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped enumeration, but
    // LLVM libc++ (LLVM 10.0) makes it a scoped enumeration, so we are forced to use extra
    // qualification here.
    return make_fs_path(path, locale, fs::path::format::auto_format);
}


inline auto make_fs_path(std::string_view path, const std::locale& locale,
                         std::filesystem::path::format format) -> std::filesystem::path
{
    namespace fs = std::filesystem;
#if ARCHON_WINDOWS
    std::wstring path_2 = core::decode_string<wchar_t>(path, locale); // Throws
    return fs::path(path_2, format); // Throws
#else
    // Assume POSIX
    static_cast<void>(locale);
    return fs::path(path, format); // Throws
#endif
}


inline auto path_to_string_generic(core::FilesystemPathRef path, const std::locale& locale) -> std::string
{
    const std::filesystem::path& path_2 = path;
#if ARCHON_WINDOWS
    std::wstring path_3 = path_2.generic_wstring(); // Throws
    return core::encode_string(std::wstring_view(path_3), locale); // Throws
#else
    // Assume POSIX
    static_cast<void>(locale);
    return path_2.generic_string(); // Throws
#endif
}


inline auto path_to_string_native(core::FilesystemPathRef path, const std::locale& locale) -> std::string
{
    const std::filesystem::path& path_2 = path;
#if ARCHON_WINDOWS
    std::wstring path_3 = path_2.wstring(); // Throws
    return core::encode_string(std::wstring_view(path_3), locale); // Throws
#else
    // Assume POSIX
    static_cast<void>(locale);
    return path_2.string(); // Throws
#endif
}


namespace impl {


template<class P> struct AsPath {
    P& path;
    bool native;
    bool lenient;
};


template<class C, class T, class P>
inline auto operator<<(std::basic_ostream<C, T>& out, const impl::AsPath<P>& pod) -> std::basic_ostream<C, T>&
{
    std::locale loc = out.getloc(); // Throws
    std::string str;
    if (pod.native) {
        str = core::path_to_string_native(pod.path, loc); // Throws
    }
    else {
        str = core::path_to_string_generic(pod.path, loc); // Throws
    }
    std::array<C, 128> seed_memory;
    core::BasicStringDecoder<C, T> decoder(loc, seed_memory); // Throws
    return out << decoder.decode_sc(str); // Throws
}


template<class C, class T, class P>
inline bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::AsPath<P>& pod)
{
    std::locale loc = src.getloc(); // Throws
    std::array<char, 128> seed_memory;
    core::BasicStringEncoder<C, T> encoder(loc, seed_memory); // Throws
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped enumeration, but
    // LLVM libc++ (LLVM 10.0) makes it a scoped enumeration, so we are forced to use extra
    // qualification here.
    fs::path::format fmt = fs::path::format::auto_format;
    if (!pod.lenient)
        fmt = (pod.native ? fs::path::format::native_format : fs::path::format::generic_format);
    pod.path = core::make_fs_path(encoder.encode_sc(src.string()), loc, fmt); // Throws
    return true;
}


} // namespace impl


inline auto as_generic_path(std::filesystem::path& path, bool lenient) noexcept
{
    bool native = false;
    return impl::AsPath<std::filesystem::path> { path, native, lenient };
}


inline auto as_generic_path(const std::filesystem::path& path, bool lenient) noexcept
{
    bool native = false;
    return impl::AsPath<const std::filesystem::path> { path, native, lenient };
}


inline auto as_native_path(std::filesystem::path& path, bool lenient) noexcept
{
    bool native = true;
    return impl::AsPath<std::filesystem::path> { path, native, lenient };
}


inline auto as_native_path(const std::filesystem::path& path, bool lenient) noexcept
{
    bool native = true;
    return impl::AsPath<const std::filesystem::path> { path, native, lenient };
}


inline bool has_trailing_slash(core::FilesystemPathRef path)
{
    const std::filesystem::path& path_2 = path;
    return (path_2.has_relative_path() && !path_2.has_filename());
}


namespace impl {


struct DotPaths {
    std::filesystem::path dot, dot_dot;
    DotPaths()
    {
        dot     = "."; // Throws
        dot_dot = ".."; // Throws
    }
};

inline auto get_dot_paths() -> const impl::DotPaths&
{
    static impl::DotPaths paths; // Throws
    return paths;
}


} // namespace impl


inline auto get_fs_dot_path() -> const std::filesystem::path&
{
    return impl::get_dot_paths().dot; // Throws
}


inline auto get_fs_dot_dot_path() -> const std::filesystem::path&
{
    return impl::get_dot_paths().dot_dot; // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FILESYSTEM_HPP
