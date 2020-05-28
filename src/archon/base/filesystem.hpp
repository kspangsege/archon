// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_FILESYSTEM_HPP
#define ARCHON_X_BASE_X_FILESYSTEM_HPP

/// \file


#include <string_view>
#include <utility>
#include <locale>
#include <filesystem>

#include <archon/base/features.h>


namespace archon::base {


inline const std::filesystem::path fs_dot_path = ".";
inline const std::filesystem::path fs_dot_dot_path = "..";



/// \brief Replacement type of filesystem path arguments.
///
/// This class is intended to be used as a replacement for argument types that
/// would otherwise be `std::filesystem::path`. Its purpose is to avoid implicit
/// conversion from a string type to `std::filesystem::path`. Such a conversion
/// is generally unwanted as it uses the global locale rather than the
/// appropriate locale of the context in which the conversion would happen.
///
/// \sa \ref FilesystemPathRef.
///
class FilesystemPath {
public:
    FilesystemPath(std::filesystem::path&&) noexcept;

    operator std::filesystem::path&&() && noexcept;

private:
    std::filesystem::path m_path;
};



/// \brief Replacement type of filesystem path arguments.
///
/// This class is intended to be used as a replacement for argument types that
/// would otherwise be `const std::filesystem::path&`. Its purpose is to avoid
/// implicit conversion from a string type to `std::filesystem::path`. Such a
/// conversion is generally unwanted as it uses the global locale rather than
/// the appropriate locale of the context in which the conversion would happen.
///
/// \sa \ref FilesystemPath.
///
class FilesystemPathRef {
public:
    using value_type = std::filesystem::path::value_type;

    FilesystemPathRef(const std::filesystem::path&) noexcept;

    operator const std::filesystem::path&() const noexcept;

    const value_type* c_str() const noexcept;

private:
    const std::filesystem::path& m_path;
};



/// \{
///
/// \brief Construct filesystem path.
///
/// These functions are shorthands for calling \ref make_fs_path() with the
/// corresponding format argument.
///
std::filesystem::path make_fs_path_generic(std::string_view path, const std::locale&);
std::filesystem::path make_fs_path_native(std::string_view path, const std::locale&);
std::filesystem::path make_fs_path_auto(std::string_view path, const std::locale&);
/// \}



/// \brief Construct filesystem path from specified format.
///
/// The purpose of this function is to hide a number of problems with direct use
/// of the corresponding `std::filesystem::path` constructor in LLVM libc++
/// (LLVM 10.0).
///
std::filesystem::path make_fs_path(std::string_view path, const std::locale&,
                                   std::filesystem::path::format);



/// \{
///
/// \brief Convert filesystem path to string.
///
/// These functions convert a filesystem path to a string representation using
/// either the generic or the native path syntax.
///
std::string path_to_string_generic(base::FilesystemPathRef, const std::locale&);
std::string path_to_string_native(base::FilesystemPathRef, const std::locale&);
/// \}



/// \brief Check for trailing directory separator.
///
/// This function returns `true` if, and only if the specified filesystem path
/// has a trailing directory separator (`/`). A directory separator that is part
/// of a root path (`std::filesystem::path::root_path()`) is not considered a
/// trailing directory separator by this function.
///
bool has_trailing_slash(base::FilesystemPathRef);



/// \brief Remove trailing directory separator.
///
/// If the specified filesystem path has a trailing directory separator (\ref
/// has_trailing_slash()), this function removes it.
///
void remove_trailing_slash(std::filesystem::path&);



/// \brief Replace dot with empty filesystem path.
///
/// If the specified filesystem path is a single dot (`.`), this function
/// replaces it with the empty filesystem path.
///
void dot_to_empty(std::filesystem::path&) noexcept;








// Implementation


// ============================ FilesystemPath ============================


inline FilesystemPath::FilesystemPath(std::filesystem::path&& path) noexcept :
    m_path(std::move(path))
{
}


inline FilesystemPath::operator std::filesystem::path&&() && noexcept
{
    return std::move(m_path);
}



// ============================ FilesystemPathRef ============================


inline FilesystemPathRef::FilesystemPathRef(const std::filesystem::path& path) noexcept :
    m_path(path)
{
}


inline FilesystemPathRef::operator const std::filesystem::path&() const noexcept
{
    return m_path;
}


inline auto FilesystemPathRef::c_str() const noexcept -> const value_type*
{
    return m_path.c_str();
}



// ============================ * * * ============================


inline std::filesystem::path make_fs_path_generic(std::string_view path, const std::locale& locale)
{
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped
    // enumeration, but LLVM libc++ (LLVM 10.0) makes it a scoped enumeration,
    // so we are forced to use extra qualification here.
    return make_fs_path(path, locale, fs::path::format::generic_format);
}


inline std::filesystem::path make_fs_path_native(std::string_view path, const std::locale& locale)
{
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped
    // enumeration, but LLVM libc++ (LLVM 10.0) makes it a scoped enumeration,
    // so we are forced to use extra qualification here.
    return make_fs_path(path, locale, fs::path::format::native_format);
}


inline std::filesystem::path make_fs_path_auto(std::string_view path, const std::locale& locale)
{
    namespace fs = std::filesystem;
    // According to the C++ standard, `fs::path::format` is an unscoped
    // enumeration, but LLVM libc++ (LLVM 10.0) makes it a scoped enumeration,
    // so we are forced to use extra qualification here.
    return make_fs_path(path, locale, fs::path::format::auto_format);
}


inline std::filesystem::path make_fs_path(std::string_view path, const std::locale& locale,
                                          std::filesystem::path::format format)
{
    namespace fs = std::filesystem;
#if ARCHON_LLVM_LIBCXX && !ARCHON_WINDOWS
    // A bug in LLVM libc++ (LLVM 10.0) causes the natural constructor
    // (`fs::path(std::string_view, const std::locale&, fs::path::format)`) to
    // not be found at link time. Fortunately, on POSIX platforms, the locale
    // argument is redundant, so we can use the simpler constructor in those
    // cases.
    //
    static_cast<void>(locale);
    return fs::path(path, format); // Throws
#else
    return fs::path(path, locale, format); // Throws
#endif
}


inline std::string path_to_string_generic(base::FilesystemPathRef path, const std::locale& locale)
{
    // FIXME: Why is it impossible to specify a locale for the conversion when a
    // locale can be specified for the reverse conversion (see
    // make_fs_path_generic())?
    static_cast<void>(locale);                         
    const std::filesystem::path& path_2 = path;
    return path_2.generic_string(); // Throws
}


inline std::string path_to_string_native(base::FilesystemPathRef path, const std::locale& locale)
{
    // FIXME: Why is it impossible to specify a locale for the conversion when a
    // locale can be specified for the reverse conversion (see
    // make_fs_path_native())?
    static_cast<void>(locale);                         
    const std::filesystem::path& path_2 = path;
    return path_2.string(); // Throws
}


inline bool has_trailing_slash(base::FilesystemPathRef path)
{
    const std::filesystem::path& path_2 = path;
    return (path_2.has_relative_path() && !path_2.has_filename());
}


inline void remove_trailing_slash(std::filesystem::path& path)
{
    if (base::has_trailing_slash(path))
        path = path.parent_path();
}


inline void dot_to_empty(std::filesystem::path& path) noexcept
{
    if (path == base::fs_dot_path)
        path = std::filesystem::path();
}



} // namespace archon::base

#endif // ARCHON_X_BASE_X_FILESYSTEM_HPP
