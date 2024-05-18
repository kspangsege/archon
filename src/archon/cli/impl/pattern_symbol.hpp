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

#ifndef ARCHON_X_CLI_X_IMPL_X_PATTERN_SYMBOL_HPP
#define ARCHON_X_CLI_X_IMPL_X_PATTERN_SYMBOL_HPP


#include <cstddef>


namespace archon::cli::impl {


// If the `type` is Type::keyword, `index` identifies the keyword. If `type` is
// Type::option, `index` identifies the option. If `type` is Type::value_slot, `index` must
// be zero.
class PatternSymbol {
public:
    enum class Type { keyword, proto_option, value_slot };

    Type type;
    std::size_t index;

    static auto keyword(std::size_t keyword_index) noexcept -> PatternSymbol;
    static auto proto_option(std::size_t proto_index) noexcept -> PatternSymbol;
    static auto value_slot() noexcept -> PatternSymbol;

    bool operator==(PatternSymbol) const noexcept;
    bool operator!=(PatternSymbol) const noexcept;
    bool operator<(PatternSymbol) const noexcept;
};








// Implementation


inline auto PatternSymbol::keyword(std::size_t keyword_index) noexcept -> PatternSymbol
{
    return { Type::keyword, keyword_index };
}


inline auto PatternSymbol::proto_option(std::size_t proto_index) noexcept -> PatternSymbol
{
    return { Type::proto_option, proto_index };
}


inline auto PatternSymbol::value_slot() noexcept -> PatternSymbol
{
    return { Type::value_slot, 0 };
}


inline bool PatternSymbol::operator==(PatternSymbol s) const noexcept
{
    return (type == s.type && index == s.index);
}


inline bool PatternSymbol::operator!=(PatternSymbol s) const noexcept
{
    return (type != s.type || index != s.index);
}


inline bool PatternSymbol::operator<(PatternSymbol s) const noexcept
{
    return (type < s.type || (type == s.type && index < s.index));
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_PATTERN_SYMBOL_HPP
