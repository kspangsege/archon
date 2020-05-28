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

#ifndef ARCHON_X_CHECK_X_FAIL_CONTEXT_HPP
#define ARCHON_X_CHECK_X_FAIL_CONTEXT_HPP

/// \file


#include <string_view>

#include <archon/check/test_details.hpp>
#include <archon/check/test_context.hpp>


namespace archon::check {


/// \brief Context of failure.
///
/// This is the part of the test case execution context that describes a particular failure
/// to the configured reporter. See \ref Reporter::fail().
///
class FailContext {
public:
    /// \brief Test case-specific execution context.
    ///
    /// This is the part of the execution context that is specific to a particular execution
    /// of a particular test case.
    ///
    const TestContext& test_context;

    /// \brief Source code location of failed check.
    ///
    /// This is the location within the source code of the failed check, or if this was not
    /// a failed check, the location of the failed test case.
    ///
    const Location& location;

    /// \brief Mapped path to file in which failure occurred.
    ///
    /// If a source path mapper is installed (\ref TestConfig::source_path_mapper), this is
    /// the result of the mapping of the path specified by `location.file_path` (\ref
    /// Location::file_path). Otherwise it is the same as `location.file_path`.
    ///
    std::string_view mapped_file_path;

private:
    FailContext(const TestContext&, const Location&, std::string_view mapped_file_path) noexcept;

    friend class impl::ThreadContextImpl;
};








// Implementation

inline FailContext::FailContext(const TestContext& tc, const Location& l, std::string_view mfp) noexcept
    : test_context(tc)
    , location(l)
    , mapped_file_path(mfp)
{
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_FAIL_CONTEXT_HPP
