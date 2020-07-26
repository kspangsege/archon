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

#ifndef ARCHON__UNIT_TEST__TEST_LIST_HPP
#define ARCHON__UNIT_TEST__TEST_LIST_HPP

#include <cstddef>
#include <string_view>
#include <list>

#include <archon/unit_test/test_details.hpp>
#include <archon/unit_test/test_context.hpp>


namespace archon::unit_test {


/// \brief List of unit tests to be considered for execution.
///
/// A list of unit tests that can can be passed to \ref unit_test::run() via
/// \ref TestConfig::test_list. Unit tests can be added to the list using \ref
/// add(), and the the list can be iterated over using \ref begin() and \ref
/// end(). Assume that an invocation of \ref add() invalidates all
/// interators. The iterators should be assumed to be of the 'forward' type
/// (`std::forward_iterator_tag`).
///
/// Ordinarily, when using macros such as \ref ARCHON_TEST() and \ref
/// ARCHON_TEST_EX(), unit tests are added automatically to a test list. In the
/// case of \ref ARCHON_TEST(), the unit test will be added to the default list,
/// which is the one that is accessible via \ref get_default_list().
///
class TestList {
public:
    struct Entry;

    using RunFunc = void(TestContext&);
    using IsEnabledFunc = bool();
    using iterator = std::list<Entry>::const_iterator;

    /// \brief Register function as unit test.
    ///
    /// This function registers a function as a unit test. It is called
    /// automatically when you use the \ref ARCHON_TEST() macro (or one of its
    /// friends).
    ///
    void add(std::string_view name, const char* file_path, long line_number, RunFunc*,
             IsEnabledFunc* = nullptr, bool allow_concur = true);

    /// \brief Number of unit tests in list.
    ///
    /// The number of unit tests currently in this test list.
    ///
    std::size_t size() const noexcept;

    /// \{
    ///
    /// \brief Iterate over unit tests in list.
    ///
    /// These functions return the beginning and end iterators
    /// respectively. Assume that these iterators are invalidated by invocations
    /// of \ref add().
    ///
    iterator begin() const noexcept;
    iterator end() const noexcept;
    /// \}

    /// \brief The default test list.
    ///
    /// This function returns a reference to the default test list. This is the
    /// list to which tests are added when using \ref ARCHON_TEST().
    ///
    static TestList& get_default_list() noexcept;

private:
    std::list<Entry> m_entries;

    void do_add(std::string_view name, Location, RunFunc*, IsEnabledFunc*, bool allow_concur);

    static TestList s_default_list;
};



/// \brief Test list entry.
///
/// Every unit test in a test list is represented by an entry of this type.
///
struct TestList::Entry {
    /// \brief Unit test function.
    ///
    /// The function that is the unit test.
    ///
    RunFunc* run_func;

    /// \brief Function deciding whether unit test is enabled.
    ///
    /// A function that decides whether this unit test is enabled or
    /// disabled. The test is enabled if, and only if this function returns
    /// `true`.
    ///
    IsEnabledFunc* is_enabled_func;

    /// \brief Whether unit test can execute concurrently with others.
    ///
    /// If set to `true`, this unit test will be allowed to execute concurrently
    /// with other unit tests in te list, that also set \p allow_concur to
    /// `true`. If set to `false`, this unit test will not be allowed to execute
    /// concurrently with any other unit test in the list.
    ///
    bool allow_concur;

    /// \brief Description of this unit test.
    ///
    /// This is a description of the unit test including its name.
    ///
    TestDetails details;
};








// Implementation


inline void TestList::add(std::string_view name, const char* file_path, long line_number,
                          RunFunc* run_func, IsEnabledFunc* is_enabled_func, bool allow_concur)
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


inline TestList& TestList::get_default_list() noexcept
{
    return s_default_list;
}


inline TestList TestList::s_default_list;


} // namespace archon::unit_test

#endif // ARCHON__UNIT_TEST__TEST_LIST_HPP
