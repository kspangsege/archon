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

#ifndef ARCHON_X_LOG_X_LIMIT_HPP
#define ARCHON_X_LOG_X_LIMIT_HPP

/// \file


#include <archon/log/log_level.hpp>


namespace archon::log {


class Limit {
public:
    virtual int get_fixed_limit() const noexcept = 0;
    virtual auto get_level_limit() const noexcept -> log::LogLevel = 0;

protected:
    ~Limit() noexcept = default;
};




class RootLimit
    : public log::Limit {
public:
    // Overriding functions from log::Limit
    int get_fixed_limit() const noexcept override final;
    auto get_level_limit() const noexcept -> log::LogLevel override final;
};




class NullLimit
    : public log::Limit {
public:
    // Overriding functions from log::Limit
    int get_fixed_limit() const noexcept override final;
    auto get_level_limit() const noexcept -> log::LogLevel override final;
};








// Implementation


// ============================ RootLimit ============================


inline int RootLimit::get_fixed_limit() const noexcept
{
    return int(log::LogLevel::all);
}


inline auto RootLimit::get_level_limit() const noexcept -> log::LogLevel
{
    return log::LogLevel::all;
}



// ============================ NullLimit ============================


inline int NullLimit::get_fixed_limit() const noexcept
{
    return int(log::LogLevel::off);
}


inline auto NullLimit::get_level_limit() const noexcept -> log::LogLevel
{
    return log::LogLevel::off;
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_LIMIT_HPP
