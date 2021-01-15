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


#include <utility>
#include <memory>
#include <functional>

#include <archon/base/type_traits.hpp>
#include <archon/base/assert.hpp>
#include <archon/cli/command_line.hpp>


namespace archon::cli::detail {


template<class C, class T> class PatternAction {
public:
    using char_type   = C;
    using traits_type = T;

    const bool is_deleg;

    virtual ~PatternAction() noexcept = default;

    virtual int enact();
    virtual int deleg(const CommandLine&);

protected:
    PatternAction(bool is_deleg) noexcept;
};



template<class C, class T, class F> std::unique_ptr<PatternAction<C, T>> make_pattern_action(F&& func);








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


template<class C, class T> inline PatternAction<C, T>::PatternAction(bool d) noexcept :
    is_deleg(d)
{
}



// ============================ FuncPatternExecAction ============================


template<class C, class T, class F> class FuncPatternExecAction;


template<class C, class T> class FuncPatternExecAction<C, T, void()> :
        public PatternAction<C, T> {
public:
    using func_type = std::function<void()>;

    FuncPatternExecAction(func_type func) :
        PatternAction<C, T>(false),
        m_func(std::move(func))
    {
    }

    int enact() override final
    {
        m_func(); // Throws
        return 0;
    }

private:
    const func_type m_func;
};


template<class C, class T, class R> class FuncPatternExecAction<C, T, R()> :
        public PatternAction<C, T> {
public:
    using func_type = std::function<R()>;

    FuncPatternExecAction(func_type func) :
        PatternAction<C, T>(false),
        m_func(std::move(func))
    {
    }

    int enact() override final
    {
        return m_func(); // Throws
    }

private:
    const func_type m_func;
};



// ============================ DelegPatternAction ============================


template<class C, class T, class R> class DelegPatternAction;


template<class C, class T> class DelegPatternAction<C, T, void> :
        public PatternAction<C, T> {
public:
    using func_type = std::function<void(const CommandLine&)>;

    DelegPatternAction(func_type func) :
        PatternAction<C, T>(true),
        m_func(std::move(func))
    {
    }

    int deleg(const CommandLine& command_line) override final
    {
        m_func(command_line); // Throws
        return 0;
    }

private:
    const func_type m_func;
};


template<class C, class T> class DelegPatternAction<C, T, int> :
    public PatternAction<C, T> {
public:
    using func_type = std::function<int(const CommandLine&)>;

    DelegPatternAction(func_type func) :
        PatternAction<C, T>(true),
        m_func(std::move(func))
    {
    }

    int deleg(const CommandLine& command_line) override final
    {
        return m_func(command_line); // Throws
    }

private:
    const func_type m_func;
};



// ============================ MakePatternAction ============================


template<class C, class T, class G> struct MakePatternAction {
    template<class F> static auto make(F&& func)
    {
        return std::make_unique<FuncPatternExecAction<C, T, G>>(std::forward<F>(func)); // Throws
    }
};


template<class C, class T, class R> struct MakePatternAction<C, T, R(const CommandLine&)> {
    template<class F> static auto make(F&& func)
    {
        return std::make_unique<DelegPatternAction<C, T, R>>(std::forward<F>(func)); // Throws
    }
};



// ============================ make_pattern_action() ============================


template<class C, class T, class F>
std::unique_ptr<PatternAction<C, T>> make_pattern_action(F&& func)
{
    using G = base::FuncDecay<F>;
    return MakePatternAction<C, T, G>::make(std::forward<F>(func)); // Throws
}


} // namespace archon::cli::detail

#endif // ARCHON_X_CLI_X_DETAIL_X_PATTERN_ACTION_HPP
