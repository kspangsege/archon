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

#ifndef ARCHON_X_CLI_X_DETAIL_X_PATTERN_ACTION_HPP
#define ARCHON_X_CLI_X_DETAIL_X_PATTERN_ACTION_HPP

/// \file


#include <archon/base/assert.hpp>
#include <archon/cli/command_line.hpp>


namespace archon::cli::detail {


template<class C, class T> class PatternAction {
public:
    using char_type         = C;
    using traits_type       = T;

    const bool is_deleg;

    virtual ~PatternAction() noexcept = default;

    virtual int enact();
    virtual int deleg(const CommandLine&);

protected:
    PatternAction(bool is_deleg);
};








// Implementation


// ============================ PatternAction ============================


template<class C, class T> inline int PatternAction<C, T>::enact()
{
    ARCHON_ASSERT(false);
}


template<class C, class T> inline int PatternAction<C, T>::deleg(const CommandLine&)
{
    ARCHON_ASSERT(false);
}


template<class C, class T> inline PatternAction<C, T>::PatternAction(bool d) :
    is_deleg(d)
{
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_PATTERN_ACTION_HPP
