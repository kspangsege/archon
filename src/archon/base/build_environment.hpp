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


/// \file

#ifndef ARCHON__BASE__BUILD_ENVIRONMENT_HPP
#define ARCHON__BASE__BUILD_ENVIRONMENT_HPP

#include <locale>
#include <filesystem>

#include <archon/base/config.h>


namespace archon::base {


/// \brief Detect various aspects of build environment.
///
/// This class allows for the detection of various aspects of the build
/// environment, such as the location of the source directory structure.
///
/// In order to work as intended, this class must be instantiated before the
/// current working directory is changed from its value at the start of program
/// execution. A good place to instantiate it, is near the beginning of the
/// `main()` function.
///
/// Example:
///
/// \code{.cpp}
///
///   BuildEnvironment::Config config;
///   config.arv0      = argv[0];
///   config.file_path = __FILE__;
///   config.bin_path  = "archon/foo"; // From "build reflection" of source root
///   config.src_path  = "archon/foo.cpp"; // From source root
///   BuildEnvironment build_env(config);
///   if (build_env.source_root_was_detected())
///       std::cout << build_env.get_relative_source_root() << "\n";
///
/// \endcode
///
class BuildEnvironment {
public:
    struct Config;

    /// \brief Default construction.
    ///
    /// This constructor constructs a build environemnt object where \ref
    /// source_root_was_detected(), \ref project_root_was_detected(), and \ref
    /// file_path_prefix_was_detected() all return `false`.
    ///
    BuildEnvironment() = default;

    /// \brief Detect build environment.
    ///
    /// This constructor attempts to detect the build environment based on the
    /// specified parameters. To know what was, and was not detected, call \ref
    /// source_root_was_detected(), \ref project_root_was_detected(), and \ref
    /// file_path_prefix_was_detected().
    ///
    explicit BuildEnvironment(Config);

    /// \brief Source root directory was detected.
    ///
    /// This function returns `true` if the path to the root of the source
    /// directory structure was detected. Otherwise, it returns `false`.
    ///
    /// When this function returns `true`, \ref get_source_root(), \ref
    /// get_build_root(), \ref get_relative_source_root(), and \ref
    /// get_relative_build_root() return meaningful values.
    ///
    bool source_root_was_detected() const noexcept;

    /// \brief Project root directory was detected.
    ///
    /// This function returns `true` if the path to the root of the project
    /// directory structure was detected. Otherwise, it returns `false`.
    ///
    /// When this function returns `true`, \ref get_project_root() and \ref
    /// get_relative_project_root() return meaningful values.
    ///
    bool project_root_was_detected() const noexcept;

    /// \brief Non-source prefix of `__FILE__` was detected.
    ///
    /// This function returns `true` if the non-source prefix of `__FILE__` was
    /// detected. Otherwise, it returns `false`.
    ///
    /// When this function returns `true`, \ref get_file_path_prefix() returns a
    /// meaningful value.
    ///
    bool file_path_prefix_was_detected() const noexcept;

    /// \brief Get source root directory.
    ///
    /// If \ref source_root_was_detected() returns `true`, this function returns
    /// the absolute path to the root of source directory structure. Otherwise,
    /// it returns the empty path. What is considered to be the root of the
    /// source directory structure follows from the choice in the specification
    /// of \ref Config::source_from_build_path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    /// \sa \ref get_relative_source_root()
    ///
    const std::filesystem::path& get_source_root() const noexcept;

    /// \brief Get "build reflection" of source root directory.
    ///
    /// If \ref source_root_was_detected() returns `true`, this function returns
    /// the absolute path to the "build reflection" of the root of source
    /// directory structure. Otherwise, it returns the empty path. What is
    /// considered to be the root of the source directory structure follows from
    /// the choice in the specification of \ref
    /// Config::source_from_build_path. Also see \ref source_from_build_path for
    /// more on the notion of "build reflection".
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    /// \sa \ref get_relative_build_root()
    ///
    const std::filesystem::path& get_build_root() const noexcept;

    /// \brief Get relative path to source root directory.
    ///
    /// If \ref source_root_was_detected() returns `true`, this function returns
    /// the relative path to the root of the source directory
    /// structure. Otherwise, it returns the empty path. See also \ref
    /// get_source_root().
    ///
    /// The returned path is relative to the directory, that was the current
    /// working directory at the time of construction of the build environment
    /// object. If the two coincide, the returned path will be the empty path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    const std::filesystem::path& get_relative_source_root() const noexcept;

    /// \brief Get relative path to "build reflection" of source root directory.
    ///
    /// This function returns the path to the "build reflection" of the root of
    /// the source directory structure. Otherwise, it returns the empty
    /// path. See also \ref get_build_root().
    ///
    /// The returned path is relative to the directory, that was the current
    /// working directory at the time of construction of the build environment
    /// object. If the two coincide, the returned path will be the empty path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    const std::filesystem::path& get_relative_build_root() const noexcept;

    /// \brief Get project root directory.
    ///
    /// If \ref project_root_was_detected() returns `true`, this function
    /// returns the absolute path to the root of the project directory
    /// structure. Otherwise, it returns the empty path. What is considered to
    /// be the root of the project directory structure follows from the choice
    /// in the specification of \ref Config::src_root, as well as the
    /// specificaztion of \ref Config::source_from_build_path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    /// \sa \ref get_relative_project_root()
    ///
    const std::filesystem::path& get_project_root() const noexcept;

    /// \brief Get relative path to project root directory.
    ///
    /// If \ref project_root_was_detected() returns `true`, this function
    /// returns the relative path to the root of the project directory
    /// structure. Otherwise, it returns the empty path. See also \ref
    /// get_project_root().
    ///
    /// The returned path is relative to the directory, that was the current
    /// working directory at the time of construction of the build environment
    /// object. If the two coincide, the returned path will be the empty path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    const std::filesystem::path& get_relative_project_root() const noexcept;

    /// \brief Get non-source prefix of `__FILE__`.
    ///
    /// If \ref file_path_prefix_was_detected() returns `true`, this function
    /// returns the part of \ref Config::file_path that remains after removing
    /// the part specified by \ref Config::src_path. In other words, it is the
    /// part of the value of `__FILE__` that "falls" outside the root of the
    /// source directory structure.
    ///
    /// The returned path will always have a trailing directory separator (`/`).
    ///
    const std::filesystem::path& get_file_path_prefix() const noexcept;

    /// \brief Remove non-source prefix from value of `__FILE__`.
    ///
    /// For source files where the path specified by `__FILE__` is expressed
    /// relative to the same directory as \ref Config::file_path, this function
    /// can be used to remove a particular prefix from those paths (values of
    /// `__FILE__`). This will change the paths to be expressed relative to the
    /// root of the source directory structure. The removed prefix is the one
    /// returned by \ref get_file_path_prefix().
    ///
    /// If the `__FILE__` path prefix was not detected (\ref
    /// file_path_prefix_was_detected() returns `false`), or if the specifed
    /// path does not have the prefix returned by \ref get_file_path_prefix(),
    /// this function throws an exception of unspecified type.
    ///
    void remove_file_path_prefix(std::filesystem::path& file_path) const;

private:
    bool m_source_root_was_detected      = false;
    bool m_project_root_was_detected     = false;
    bool m_file_path_prefix_was_detected = false;
    std::filesystem::path m_source_root;
    std::filesystem::path m_build_root;
    std::filesystem::path m_relative_source_root;
    std::filesystem::path m_relative_build_root;
    std::filesystem::path m_project_root;
    std::filesystem::path m_relative_project_root;
    std::filesystem::path m_file_path_prefix;
};




/// \brief Build environment detection parameters.
///
/// These are the available parameters for controlling the detection of the
/// build envirenment.
///
struct BuildEnvironment::Config {
    /// \brief Name or path of executing program.
    ///
    /// The value of `argv[0]` where `argv` is the second argument passed to
    /// `main()`. This is supposed to reflect the name of, and possibly the path
    /// within the filesystem to the executing program.
    ///
    std::string_view argv0;

    /// \brief Path specified by `__FILE__` macro.
    ///
    /// The path specified by the `__FILE__` macro for some source file. The
    /// natural choise is to use the source file containing `main()`. In any
    /// case, the choice must be aligned with \ref src_path.
    ///
    std::string_view file_path;

    /// \brief Executable path from "build reflection" of source root.
    ///
    /// The path to the file containing the executing program relative to the
    /// "build reflection" of the root of the source directory strucure. This
    /// path must be specified in the generic path format as defined by
    /// `std::filesystem::path`. The file containing the executing program must
    /// be the one referred to by \ref argv0. What is considered to be the root
    /// of the source directory follows from the choice in the specification of
    /// \ref source_from_build_path. Also see \ref source_from_build_path for
    /// more on the notion of "build reflection".
    ///
    /// On the Windows platform (\ref ARCHON_ WINDOWS), the `.exe` suffix is
    /// implicit, and must therfore not be included in the specified path.
    ///
    std::string_view bin_path;

    /// \brief Source path from source root.
    ///
    /// The path to the source file referred to by \ref file_path, relative to
    /// the root of the source directory structure. This path must be specified
    /// in the generic path format as defined by `std::filesystem::path`. What
    /// is considered to be the root of the source directory follows from the
    /// choice in the specification of \ref source_from_build_path.
    ///
    std::string_view src_path;

    /// \brief Path to source root from project root.
    ///
    /// If the source directory structure is part of a larger project directory
    /// structure, this is the path to the root of the source directory
    /// structure relative to the root of the project directory structure. It
    /// can be specified with, or without a trailing directory separator
    /// (`/`). If it is left empty, this class assumes that the project directory
    /// structure coincides with the source directory structure.
    ///
    std::string_view src_root;

    /// \brief Location of source root relative to its "build reflection".
    ///
    /// The path to the root of the source directory structure (with or without
    /// a trailing slash) relative to its reflection in the build directory
    /// structure, or the empty string if there is no separate build directory
    /// structure. For example, if the root of the source directory structure is
    /// `src/` and its reflection in the build directory structure is
    /// `build/src/`, then `source_from_build_path` should be `../../src/`.
    ///
    std::string_view source_from_build_path = ARCHON_SOURCE_FROM_BUILD_PATH;

    /// \brief Path construction locale.
    ///
    /// This locale will be used for constructing instaces of
    /// `std::filesystem::path`.
    ///
    std::locale locale;
};








// Implementation


inline bool BuildEnvironment::source_root_was_detected() const noexcept
{
    return m_source_root_was_detected;
}


inline bool BuildEnvironment::project_root_was_detected() const noexcept
{
    return m_source_root_was_detected;
}


inline bool BuildEnvironment::file_path_prefix_was_detected() const noexcept
{
    return m_file_path_prefix_was_detected;
}


inline const std::filesystem::path& BuildEnvironment::get_source_root() const noexcept
{
    return m_source_root;
}


inline const std::filesystem::path& BuildEnvironment::get_build_root() const noexcept
{
    return m_build_root;
}


inline const std::filesystem::path& BuildEnvironment::get_relative_source_root() const noexcept
{
    return m_relative_source_root;
}


inline const std::filesystem::path& BuildEnvironment::get_relative_build_root() const noexcept
{
    return m_relative_build_root;
}


inline const std::filesystem::path& BuildEnvironment::get_project_root() const noexcept
{
    return m_project_root;
}


inline const std::filesystem::path& BuildEnvironment::get_relative_project_root() const noexcept
{
    return m_relative_project_root;
}


inline const std::filesystem::path& BuildEnvironment::get_file_path_prefix() const noexcept
{
    return m_file_path_prefix;
}


} // namespace archon::base

#endif // ARCHON__BASE__BUILD_ENVIRONMENT_HPP
