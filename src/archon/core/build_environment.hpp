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

#ifndef ARCHON_X_CORE_X_BUILD_ENVIRONMENT_HPP
#define ARCHON_X_CORE_X_BUILD_ENVIRONMENT_HPP

/// \file


#include <array>
#include <string>
#include <locale>
#include <filesystem>
#include <ostream>

#include <archon/core/impl/config.h>
#include <archon/core/string_codec.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_list.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/filesystem.hpp>


namespace archon::core {


/// \brief Detect various aspects of build environment.
///
/// This class allows for the detection of various aspects of the build environment, such as
/// the location of the source directory structure.
///
/// In order to work as intended, this class must be instantiated before the current working
/// directory is changed from its value at the start of program execution. A good place to
/// instantiate it, is near the beginning of the `main()` function.
///
/// Here is a simple example for an executable named `foo` built from source file `foo.cpp`:
///
/// \code{.cpp}
///
///   archon::core::BuildEnvironment::Params params;
///   params.file_path = __FILE__;
///   params.bin_path  = "foo";     // From "build reflection" of source root
///   params.src_path  = "foo.cpp"; // From source root
///   archon::core::BuildEnvironment build_env(argv[0], params);
///   if (build_env.source_root_was_detected())
///       std::cout << build_env.get_relative_source_root() << "\n";
///
/// \endcode
///
/// \note The executable name must be specified without the `.exe` suffix on the Windows
/// platform (see \ref Params::bin_path).
///
/// Here is a more complex example where the root of the source tree is in subdirectory
/// `src/`, where the root of the build tree (reflection of structure in source tree) is in
/// `build/`, and where the executable, `bar`, is in subdirectory `foo/` relative to the
/// root of the source tree:
///
/// \code{.cpp}
///
///   archon::core::BuildEnvironment::Params params;
///   params.file_path = __FILE__;
///   params.bin_path  = "foo/bar";     // From "build reflection" of source root
///   params.src_path  = "foo/bar.cpp"; // From source root
///   params.src_root  = "src/";        // Relative to project root
///   params.source_from_build_path = "../src/";
///   archon::core::BuildEnvironment build_env(argv[0], params);
///   if (build_env.source_root_was_detected())
///       std::cout << build_env.get_relative_source_root() << "\n";
///
/// \endcode
///
/// If a build environment object is written to an output stream, a description of the
/// detected build environment will be written to that output stream. See \ref
/// operator<<(std::basic_ostream<C, T>&, const core::BuildEnvironment&).
///
class BuildEnvironment {
public:
    struct Params;

    /// \brief Default construction.
    ///
    /// This constructor constructs a build environment object where \ref
    /// source_root_was_detected(), \ref project_root_was_detected(), and \ref
    /// file_path_prefix_was_detected() all return `false`.
    ///
    BuildEnvironment() = default;

    /// \brief Detect build environment.
    ///
    /// This constructor attempts to detect the build environment based on the specified
    /// parameters. To know what was, and was not detected, call \ref
    /// source_root_was_detected(), \ref project_root_was_detected(), and \ref
    /// file_path_prefix_was_detected().
    ///
    /// \param argv0 The first entry in the array of arguments passed to `main()` by the
    /// operating system. This is supposed to reflect the name of, and possibly the path
    /// within the file system to the executing program. The path information, including
    /// whether it is present, is important to this class.
    ///
    explicit BuildEnvironment(std::string_view argv0, Params, const std::locale& = {});

    /// \brief Source root directory was detected.
    ///
    /// This function returns `true` if the path to the root of the source directory
    /// structure was detected. Otherwise, it returns `false`.
    ///
    /// When this function returns `true`, \ref get_source_root(), \ref get_build_root(),
    /// \ref get_relative_source_root(), and \ref get_relative_build_root() return
    /// meaningful values.
    ///
    bool source_root_was_detected() const noexcept;

    /// \brief Project root directory was detected.
    ///
    /// This function returns `true` if the path to the root of the project directory
    /// structure was detected. Otherwise, it returns `false`.
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
    /// When this function returns `true`, \ref get_file_path_prefix() returns a meaningful
    /// value.
    ///
    bool file_path_prefix_was_detected() const noexcept;

    /// \brief Get source root directory.
    ///
    /// If \ref source_root_was_detected() returns `true`, this function returns the
    /// absolute path to the root of source directory structure. Otherwise, it returns the
    /// empty path. What is considered to be the root of the source directory structure
    /// follows from the choice in the specification of \ref Params::source_from_build_path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    /// \sa \ref get_relative_source_root()
    ///
    auto get_source_root() const noexcept -> const std::filesystem::path&;

    /// \brief Get "build reflection" of source root directory.
    ///
    /// If \ref source_root_was_detected() returns `true`, this function returns the
    /// absolute path to the "build reflection" of the root of source directory
    /// structure. Otherwise, it returns the empty path. What is considered to be the root
    /// of the source directory structure follows from the choice in the specification of
    /// \ref Params::source_from_build_path. Also see \ref source_from_build_path for more
    /// on the notion of "build reflection".
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    /// \sa \ref get_relative_build_root()
    ///
    auto get_build_root() const noexcept -> const std::filesystem::path&;

    /// \brief Get relative path to source root directory.
    ///
    /// If \ref source_root_was_detected() returns `true`, this function returns the
    /// relative path to the root of the source directory structure. Otherwise, it returns
    /// the empty path. See also \ref get_source_root().
    ///
    /// The returned path is relative to the directory, that was the current working
    /// directory at the time of construction of the build environment object. If the two
    /// coincide, the returned path will be the empty path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    auto get_relative_source_root() const noexcept -> const std::filesystem::path&;

    /// \brief Get relative path to "build reflection" of source root directory.
    ///
    /// This function returns the path to the "build reflection" of the root of the source
    /// directory structure. Otherwise, it returns the empty path. See also \ref
    /// get_build_root().
    ///
    /// The returned path is relative to the directory, that was the current working
    /// directory at the time of construction of the build environment object. If the two
    /// coincide, the returned path will be the empty path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    auto get_relative_build_root() const noexcept -> const std::filesystem::path&;

    /// \brief Get project root directory.
    ///
    /// If \ref project_root_was_detected() returns `true`, this function returns the
    /// absolute path to the root of the project directory structure. Otherwise, it returns
    /// the empty path. What is considered to be the root of the project directory structure
    /// follows from the choice in the specification of \ref Params::src_root, as well as
    /// the specification of \ref Params::source_from_build_path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    /// \sa \ref get_relative_project_root()
    ///
    auto get_project_root() const noexcept -> const std::filesystem::path&;

    /// \brief Get relative path to project root directory.
    ///
    /// If \ref project_root_was_detected() returns `true`, this function returns the
    /// relative path to the root of the project directory structure. Otherwise, it returns
    /// the empty path. See also \ref get_project_root().
    ///
    /// The returned path is relative to the directory, that was the current working
    /// directory at the time of construction of the build environment object. If the two
    /// coincide, the returned path will be the empty path.
    ///
    /// The returned path will never have a trailing directory separator (`/`).
    ///
    auto get_relative_project_root() const noexcept -> const std::filesystem::path&;

    /// \brief Get non-source prefix of `__FILE__`.
    ///
    /// If \ref file_path_prefix_was_detected() returns `true`, this function returns the
    /// part of \ref Params::file_path that remains after removing the part specified by
    /// \ref Params::src_path. In other words, it is the part of the value of `__FILE__`
    /// that "falls" outside the root of the source directory structure.
    ///
    /// The returned path will always have a trailing directory separator (`/`).
    ///
    /// \sa \ref remove_file_path_prefix()
    ///
    auto get_file_path_prefix() const noexcept -> const std::filesystem::path&;

    /// \brief Remove non-source prefix from value of `__FILE__`.
    ///
    /// For source files where the path specified by `__FILE__` is expressed relative to the
    /// same directory as \ref Params::file_path, this function can be used to remove a
    /// particular prefix from those paths (values of `__FILE__`). This will change the
    /// paths to be expressed relative to the root of the source directory structure. The
    /// removed prefix is the one returned by \ref get_file_path_prefix().
    ///
    /// If the `__FILE__` path prefix was not detected (\ref file_path_prefix_was_detected()
    /// returns `false`), or if the specified path does not have the prefix returned by \ref
    /// get_file_path_prefix(), this function throws an exception of unspecified type.
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


/// \brief Write description of detected build environment to output stream.
///
/// This stream output operator writes a description of the detected build environment (\p
/// env) to the specified output stream (\p out).
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>& out, const core::BuildEnvironment& env) ->
    std::basic_ostream<C, T>&;




/// \brief Build environment detection parameters.
///
/// These are the available parameters for controlling the detection of the build
/// environment.
///
struct BuildEnvironment::Params {
    /// \brief Path specified by `__FILE__` macro.
    ///
    /// The path specified by the `__FILE__` macro for some source file. The natural choice
    /// is to use the source file containing `main()`. In any case, the choice must be
    /// aligned with \ref src_path.
    ///
    std::string_view file_path;

    /// \brief Executable path from "build reflection" of source root.
    ///
    /// The path to the file containing the executing program relative to the "build
    /// reflection" of the root of the source directory structure. This path must be
    /// specified in the generic path format as defined by `std::filesystem::path`. The file
    /// containing the executing program must be the one referred to by \ref argv0. What is
    /// considered to be the root of the source directory follows from the choice in the
    /// specification of \ref source_from_build_path. Also see \ref source_from_build_path
    /// for more on the notion of "build reflection".
    ///
    /// On the Windows platform (\ref ARCHON_ WINDOWS), the `.exe` suffix is implicit, and
    /// must therefore not be included in the specified path.
    ///
    std::string_view bin_path;

    /// \brief Source path from source root.
    ///
    /// The path to the source file referred to by \ref file_path, relative to the root of
    /// the source directory structure. This path must be specified in the generic path
    /// format as defined by `std::filesystem::path`. What is considered to be the root of
    /// the source directory follows from the choice in the specification of \ref
    /// source_from_build_path.
    ///
    std::string_view src_path;

    /// \brief Path to source root from project root.
    ///
    /// If the source directory structure is part of a larger project directory structure,
    /// this is the path to the root of the source directory structure relative to the root
    /// of the project directory structure. It can be specified with, or without a trailing
    /// directory separator (`/`). If it is left empty, this class assumes that the project
    /// directory structure coincides with the source directory structure.
    ///
    std::string_view src_root;

    /// \brief Location of source root relative to its "build reflection".
    ///
    /// The path to the root of the source directory structure (with or without a trailing
    /// slash) relative to its reflection in the build directory structure, or the empty
    /// string if there is no separate build directory structure. For example, if the root
    /// of the source directory structure is `src/` and its reflection in the build
    /// directory structure is `build/src/`, then `source_from_build_path` should be
    /// `../../src/`.
    ///
    /// For use cases inside the Archon project, use \ref
    /// core::archon_source_from_build_path.
    ///
    std::string_view source_from_build_path;
};




/// \brief Path to Archon project source root from build root.
///
/// This is the file system path to the root of the source tree of the Archon project,
/// relative to the root of the build tree ("build reflection" of source tree). See \ref
/// core::BuildEnvironment::Params::source_from_build_path.
///
constexpr std::string_view archon_source_from_build_path = ARCHON_SOURCE_FROM_BUILD_PATH;








// Implementation


inline bool BuildEnvironment::source_root_was_detected() const noexcept
{
    return m_source_root_was_detected;
}


inline bool BuildEnvironment::project_root_was_detected() const noexcept
{
    return m_project_root_was_detected;
}


inline bool BuildEnvironment::file_path_prefix_was_detected() const noexcept
{
    return m_file_path_prefix_was_detected;
}


inline auto BuildEnvironment::get_source_root() const noexcept -> const std::filesystem::path&
{
    return m_source_root;
}


inline auto BuildEnvironment::get_build_root() const noexcept -> const std::filesystem::path&
{
    return m_build_root;
}


inline auto BuildEnvironment::get_relative_source_root() const noexcept -> const std::filesystem::path&
{
    return m_relative_source_root;
}


inline auto BuildEnvironment::get_relative_build_root() const noexcept -> const std::filesystem::path&
{
    return m_relative_build_root;
}


inline auto BuildEnvironment::get_project_root() const noexcept -> const std::filesystem::path&
{
    return m_project_root;
}


inline auto BuildEnvironment::get_relative_project_root() const noexcept -> const std::filesystem::path&
{
    return m_relative_project_root;
}


inline auto BuildEnvironment::get_file_path_prefix() const noexcept -> const std::filesystem::path&
{
    return m_file_path_prefix;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const core::BuildEnvironment& env) -> std::basic_ostream<C, T>&
{
    struct Entry {
        std::string_view label;
        const std::filesystem::path* value;
    };
    Entry entries[] = {
        {
            "project root",
            (env.project_root_was_detected() ? &env.get_relative_project_root() : nullptr),
        },
        {
            "source root",
            (env.source_root_was_detected() ? &env.get_relative_source_root() : nullptr),
        },
        {
            "build root",
            (env.source_root_was_detected() ? &env.get_relative_build_root() : nullptr),
        },
        {
            "file path prefix",
            (env.file_path_prefix_was_detected() ? &env.get_file_path_prefix() : nullptr),
        },
    };
    std::locale loc = out.getloc(); // Throws
    std::array<C, 64> seed_mem;
    core::BasicStringDecoder<C, T> decoder(loc, seed_mem); // Throws
    out << core::as_list(entries, [&](const Entry& entry) {
        return core::as_format_func([&](std::basic_ostream<C, T>& out) {
            if (entry.value) {
                namespace fs = std::filesystem;
                fs::path path = *entry.value;
                core::add_trailing_slash(path); // Throws
                std::string str = core::path_to_string_generic(path, loc); // Throws
                out << core::formatted("%s is %s", entry.label, decoder.decode_sc(str)); // Throws
            }
            else {
                out << core::formatted("%s was not detected", entry.label); // Throws
            }
        }); // Throws
    }); // Throws
    return out;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_BUILD_ENVIRONMENT_HPP
