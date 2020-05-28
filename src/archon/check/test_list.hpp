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

#ifndef ARCHON_X_CHECK_X_TEST_LIST_HPP
#define ARCHON_X_CHECK_X_TEST_LIST_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <list>

#include <archon/check/test_details.hpp>
#include <archon/check/test_context.hpp>


namespace archon::check {


/// \brief List of test cases to be considered for execution.
///
/// A list of test cases that can can be passed to \ref check::run() via \ref
/// TestConfig::test_list. Test cases can be added to the list using \ref add(), and the the
/// list can be iterated over using \ref begin() and \ref end(). Assume that an invocation
/// of \ref add() invalidates all interators. The iterators should be assumed to be of the
/// 'forward' type (`std::forward_iterator_tag`).
///
/// Ordinarily, when using macros such as \ref ARCHON_TEST() and \ref ARCHON_TEST_EX(), test
/// cases are added automatically to a test list. In the case of \ref ARCHON_TEST(), the
/// test case will be added to the default list, which is the one that is accessible via
/// \ref get_default_list().
///
class TestList {
public:
    struct Entry;

    using RunFunc = void(TestContext&);
    using IsEnabledFunc = bool();
    using iterator = std::list<Entry>::const_iterator;

    /// \brief Register function as test case.
    ///
    /// This function registers the specified function (\p func) as a test case. It is
    /// called automatically when you use the \ref ARCHON_TEST() macro (or one of its
    /// friends).
    ///
    /// The caller must ensure that the memory, pointed to by \p name and \p file_path, is
    /// not destroyed while the test list is still in use. Here, "in use" covers any
    /// invocation of a member function, other than the destructor, as well as any access to
    /// contained entries (\ref Entry). It is, on the other hand, safe for the memory,
    /// pointed to by \p name and \p file_path, to be destroyed before the destruction of
    /// the list, as long as the previously mentioned rule is adhered to.
    ///
    void add(std::string_view name, const char* file_path, long line_number, RunFunc* func, IsEnabledFunc* = nullptr,
             bool allow_concur = true);

    /// \brief Number of test cases in list.
    ///
    /// The number of test cases currently in this test list.
    ///
    auto size() const noexcept -> std::size_t;

    /// \{
    ///
    /// \brief Iterate over test cases in list.
    ///
    /// These functions return the beginning and end iterators respectively. Assume that
    /// these iterators are invalidated by invocations of \ref add().
    ///
    auto begin() const noexcept -> iterator;
    auto end() const noexcept -> iterator;
    /// \}

    /// \brief The default test list.
    ///
    /// This function returns a reference to the default test list. This is the list to
    /// which tests are added when using \ref ARCHON_TEST().
    ///
    static auto get_default_list() noexcept -> TestList&;

private:
    std::list<Entry> m_entries;

    void do_add(std::string_view name, Location, RunFunc*, IsEnabledFunc*, bool allow_concur);

    static TestList s_default_list;
};



/// \brief Test list entry.
///
/// Every test case in a test list is represented by an entry of this type.
///
struct TestList::Entry {
    /// \brief Test case function.
    ///
    /// The function that is the test case.
    ///
    RunFunc* run_func;

    /// \brief Function deciding whether test case is enabled.
    ///
    /// A function that decides whether this test case is enabled or disabled. The test case
    /// is enabled if, and only if this function returns `true`.
    ///
    IsEnabledFunc* is_enabled_func;

    /// \brief Whether test case can execute concurrently with others.
    ///
    /// If set to `true`, this test case will be allowed to execute concurrently with the
    /// other test cases in the list that also set \p allow_concur to `true`. If set to
    /// `false`, this test case will not be allowed to execute concurrently with any other
    /// test cases in the list.
    ///
    bool allow_concur;

    /// \brief Description of this test case.
    ///
    /// This is a description of the test case including its name.
    ///
    TestDetails details;
};








// Implementation


inline void TestList::add(std::string_view name, const char* file_path, long line_number, RunFunc* run_func,
                          IsEnabledFunc* is_enabled_func, bool allow_concur)
{
    Location location = { file_path, line_number };
    do_add(name, location, run_func, is_enabled_func, allow_concur); // Throws
}


inline std::size_t TestList::size() const noexcept
{
    return m_entries.size();
}


inline auto TestList::begin() const noexcept -> iterator
{
    return m_entries.begin();
}


inline auto TestList::end() const noexcept -> iterator
{
    return m_entries.end();
}


inline auto TestList::get_default_list() noexcept -> TestList&
{
    return s_default_list;
}


inline TestList TestList::s_default_list;


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_TEST_LIST_HPP
