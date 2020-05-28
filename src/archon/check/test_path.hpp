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

#ifndef ARCHON_X_CHECK_X_TEST_PATH_HPP
#define ARCHON_X_CHECK_X_TEST_PATH_HPP

/// \file


#include <archon/core/filesystem.hpp>
#include <archon/check/test_context.hpp>


/// \brief Declaration of test file guard.
///
/// This macro expands to a variable declaration with the type of the variable being \ref
/// archon::check::TestFileGuard, and the name of it being \p var_name.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
#define ARCHON_TEST_FILE(var_name)                                      \
    X_ARCHON_TEST_PATH(archon::check::TestFileGuard, var_name, "")


/// \brief Declaration of test file guard with custom file name extension.
///
/// This macro expands to a variable declaration with the type of the variable being \ref
/// archon::check::TestFileGuard, and the name of it being \p var_name. The file name with
/// have the specified extension.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
#define ARCHON_TEST_FILE_EX(var_name, file_name_extension)              \
    X_ARCHON_TEST_PATH(archon::check::TestFileGuard, var_name, "." file_name_extension)


/// \brief Declaration of test directory guard.
///
/// This macro expands to a variable declaration with the type of the variable being \ref
/// archon::check::TestDirGuard, and the name of it being \p var_name.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
#define ARCHON_TEST_DIR(var_name)                                       \
    X_ARCHON_TEST_PATH(archon::check::TestDirGuard, var_name, "")



namespace archon::check {



/// \brief Basics for managing test files.
///
/// This class is a convenience base for \ref TestFile and \ref TestDir, and can be used as
/// base of other classes too.
///
class TestPath {
public:
    TestPath(std::filesystem::path) noexcept;
    ~TestPath() noexcept = default;

    /// \brief The represented filesystem path.
    ///
    /// This is one of the required functions for any template argument of \ref
    /// TestPathGuard. It returns the represented filesystem path.
    ///
    auto path() const noexcept -> const std::filesystem::path&;

    /// \brief No-op preparation.
    ///
    /// This is one of the required functions for any template argument of \ref
    /// TestPathGuard. In this case, the function does nothing. It exists to free subclasses
    /// from having to define it when there is nothing to do as preperation.
    ///
    void prepare(const check::TestContext&);

    /// \brief Remove test file or directory.
    ///
    /// This is one of the required functions for any template argument of \ref
    /// TestPathGuard. It removes whatever \ref path() refers to, be that a file or a
    /// directory. Removal occurs as if by `std::filesystem::remove_all()`.
    ///
    void cleanup(const check::TestContext&);

private:
    std::filesystem::path m_path;
};


/// \brief Manage test files.
///
/// This class is used to manage test files, and is supposed to be passed as template
/// argument for \ref TestPathGuard.
///
/// \sa \ref TestFileGuard.
/// \sa \ref TestDir.
///
class TestFile
    : public check::TestPath {
public:
    using check::TestPath::TestPath;
};


/// \brief Manage test directories.
///
/// This class is used to manage test directories, and is supposed to be passed as template
/// argument for \ref TestPathGuard.
///
/// \sa \ref TestDirGuard.
/// \sa \ref TestFile.
///
class TestDir
    : public check::TestPath {
public:
    using check::TestPath::TestPath;

    /// \brief Create directory.
    ///
    /// This function creates the represented directory.
    ///
    void prepare(const check::TestContext&);
};




/// \brief Test file guard.
///
/// An instance of this class guards the preperation and cleanup of a test file, or a set of
/// test files. It does this by tying the preperation operation to the construction of the
/// guard object, and the cleanup operation to the destruction of it. Cleanup is also
/// carried out during construction for the cases where files are left over from a previous
/// execution of the test case.
///
/// This class is agnostic as to what exactly the preperation and cleanup operations
/// do. Those operations are determined by the specified test path type, \p P. A guard
/// object contains a instance of \p P, which is constructed during the construction of the
/// guard object. The guard object constructor then calls \ref P::cleanup() and \ref
/// P::prepare(), in that order. The guard object destructor calls \ref P::cleanup() unless
/// \ref TestContext::keep_test_files() returns true.
///
/// The specified test path type, \p P, must define the following three functions:
///
///   - `path()`, which must return the represented filesystem path as a reference to an
///     object of type `std::filesystem::path`.
///
///   - `prepare(const TestContext&)`, which must perform preparations associated with the
///     represented test file, or set of test files. It will be called during construction
///     of the guard object. It can assume that `P::cleanup()` has just been executed, so it
///     does not itself have to deal with cleanup. The passed test context object is the one
///     that was passed to the contructor of the guard object.
///
///   - `cleanup(const TestContext&)`, which must remove the represented test file, or set
///     of test files. It will be called during construction of the guard object, before
///     `P::prepare()` is called, and again during destruction of the guard object. The
///     passed test context object is the one that was passed to the contructor of the guard
///     object.
///
/// A guard object can be implicitly converted to a filesystem path (`const
/// std::filesystem::path&` or \ref core::FilesystemPathRef).
///
/// A guard object is movable, but not copyable.
///
/// Due to the cleaup operation carried out during destruction of a guard object, the
/// destructor can, in general, throw exceptions.
///
/// The life of a guard object must not extend beyond the end of execution of the test case,
/// i.e., the test case in the context of which the guard object was constructed. Here, the
/// life of a moved guard in a move-construction or a move-assigment is considered to
/// continue, and the life of as replaced guard in a move-assignment is considered to end.
///
/// \sa \ref TestFileGuard and \ref TestDirGuard.
///
template<class P> class TestPathGuard {
public:
    using test_path_type = P;

    /// \brief Construct test path guard.
    ///
    /// Construct a guard object for a test file, or a set of test files. The filesystem
    /// path will have the specified suffix (\p suffix), e.g., `.txt` (note that the dot
    /// must be included in the specified string). The exact filesystem path is determined
    /// by an invocation of \ref TestContext::make_test_path().
    ///
    /// Additional arguments (\p args) will be forwarded to the constructor of the contained
    /// test path object of type \p P.
    ///
    template<class... A> TestPathGuard(const check::TestContext&, std::string_view suffix, A&&... args);

    ~TestPathGuard() noexcept(false);

    operator const std::filesystem::path&() const noexcept;
    operator core::FilesystemPathRef() const noexcept;

    TestPathGuard(const TestPathGuard&) = delete;
    auto operator=(const TestPathGuard&) -> TestPathGuard& = delete;

    TestPathGuard(TestPathGuard&&) noexcept;
    auto operator=(TestPathGuard&&) -> TestPathGuard&;

    /// \brief Ths represented filesystem path.
    ///
    /// This function returns a reference to the contained test path object.
    ///
    auto path() const noexcept -> const P&;

private:
    const check::TestContext* m_test_context;
    P m_path;

    void destroy();
};


/// \brief Guard for single test file.
///
/// The file is removed, if it exists, during the construction of the guard, and also
/// removed during the destruction of the guard.
///
/// \sa \ref ARCHON_TEST_FILE() and \ref ARCHON_TEST_FILE_EX()
///
using TestFileGuard = check::TestPathGuard<check::TestFile>;


/// \brief Guard for test directory.
///
/// During construction of the guard, the directory and its contents is removed, then the
/// directory is recreated empty. During destruction, the directory and its contents is
/// removed.
///
/// \sa \ref ARCHON_TEST_DIR()
///
using TestDirGuard  = check::TestPathGuard<check::TestDir>;








// Implementation


#define X_ARCHON_TEST_PATH(guard_class_name, var_name, suffix)          \
    guard_class_name var_name(test_context, "." #var_name suffix)



// ============================ TestPath ============================


inline TestPath::TestPath(std::filesystem::path path) noexcept
    : m_path(std::move(path))
{
}


inline auto TestPath::path() const noexcept -> const std::filesystem::path&
{
    return m_path;
}


inline void TestPath::prepare(const check::TestContext&)
{
    // Intentional no-op
}


inline void TestPath::cleanup(const check::TestContext&)
{
    namespace fs = std::filesystem;
    fs::remove_all(m_path); // Throws
}



// ============================ TestDir ============================


inline void TestDir::prepare(const check::TestContext&)
{
    namespace fs = std::filesystem;
    fs::create_directory(path()); // Throws
}



// ============================ TestPathGuard ============================


template<class P>
template<class... A>
inline TestPathGuard<P>::TestPathGuard(const check::TestContext& test_context, std::string_view suffix, A&&... args)
    : m_test_context(&test_context)
    , m_path(test_context.make_test_path(suffix), std::forward<A>(args)...) // Throws
{
    m_path.cleanup(test_context); // Throws
    m_path.prepare(test_context); // Throws
}


template<class P>
inline TestPathGuard<P>::~TestPathGuard() noexcept(false)
{
    destroy(); // Throws
}


template<class P>
inline TestPathGuard<P>::operator const std::filesystem::path&() const noexcept
{
    return m_path.path();
}


template<class P>
inline TestPathGuard<P>::operator core::FilesystemPathRef() const noexcept
{
    return m_path.path();
}


template<class P>
inline TestPathGuard<P>::TestPathGuard(TestPathGuard&& other) noexcept
    : m_test_context(other.m_test_context)
    , m_path(std::move(other.m_path))
{
    other.m_test_context = nullptr;
}


template<class P>
inline auto TestPathGuard<P>::operator=(TestPathGuard&& other) -> TestPathGuard&
{
    destroy(); // Throws
    m_test_context = other.m_test_context;
    other.m_test_context = nullptr;
    m_path = std::move(other.m_path);
    return *this;
}


template<class P>
inline auto TestPathGuard<P>::path() const noexcept -> const P&
{
    return m_path;
}


template<class P>
inline void TestPathGuard<P>::destroy()
{
    if (m_test_context && !m_test_context->keep_test_files())
        m_path.cleanup(*m_test_context); // Throws
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_TEST_PATH_HPP
