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

#ifndef ARCHON__BASE__TERMINAL_HPP
#define ARCHON__BASE__TERMINAL_HPP

#include <cstddef>
#include <cstdint>


namespace archon::base::terminal {


enum class Weight { normal, bold, faint };


enum class Color { black, red, green, yellow, blue, magenta, cyan, white };


namespace seq {

constexpr const char* reset() noexcept;
constexpr const char* set_weight(Weight) noexcept;
constexpr const char* set_underline(bool) noexcept;
constexpr const char* set_blink(bool) noexcept;
constexpr const char* set_reverse(bool) noexcept;
constexpr const char* set_color(Color) noexcept;
constexpr const char* set_background_color(Color) noexcept;
constexpr const char* reset_color() noexcept;
constexpr const char* reset_background_color() noexcept;

} // namespace seq



class TextAttributes {
public:
    void set_weight(Weight);
    void set_underline(bool);
    void set_blink(bool);
    void set_reverse(bool);
    void set_color(Color);
    void unset_color();
    void set_background_color(Color);
    void unset_background_color();

    Weight get_weight() const noexcept;
    bool get_underline() const noexcept;
    bool get_blink() const noexcept;
    bool get_reverse() const noexcept;
    bool has_color() const noexcept;
    Color get_color() const noexcept;
    bool has_background_color() const noexcept;
    Color get_background_color() const noexcept;

    static std::size_t change(TextAttributes from, TextAttributes to, char* buffer) noexcept;

    static constexpr std::size_t min_change_buffer_size() noexcept;

    TextAttributes() noexcept;
    bool operator==(const TextAttributes&) const noexcept;
    bool operator!=(const TextAttributes&) const noexcept;

private:
    using uint = std::uint_least16_t;

    struct Fields {
        uint weight               : 2;
        uint underline            : 1;
        uint blink                : 1;
        uint reverse              : 1;
        uint has_color            : 1;
        uint color                : 3;
        uint has_background_color : 1;
        uint background_color     : 3;
    };

    union {
        uint m_mem;
        Fields m_fields;
    };

    static_assert(sizeof m_mem == sizeof m_fields);
};








// Implementation


constexpr const char* seq::reset() noexcept
{
    return "\033[m";
}


constexpr const char* seq::set_weight(Weight value) noexcept
{
    switch (value) {
        case Weight::normal:
            return "\033[22m";
        case Weight::bold:
            return "\033[1m";
        case Weight::faint:
            return "\033[2m";
    }
    return nullptr;
}


constexpr const char* seq::set_underline(bool value) noexcept
{
    return (value ? "\033[4m" : "\033[24m");
}


constexpr const char* seq::set_blink(bool value) noexcept
{
    return (value ? "\033[5m" : "\033[25m");
}


constexpr const char* seq::set_reverse(bool value) noexcept
{
    return (value ? "\033[7m" : "\033[27m");
}


constexpr const char* seq::set_color(Color value) noexcept
{
    switch (value) {
        case Color::black:
            return "\033[30m";
        case Color::red:
            return "\033[31m";
        case Color::green:
            return "\033[32m";
        case Color::yellow:
            return "\033[33m";
        case Color::blue:
            return "\033[34m";
        case Color::magenta:
            return "\033[35m";
        case Color::cyan:
            return "\033[36m";
        case Color::white:
            return "\033[37m";
    }
    return nullptr;
}


constexpr const char* seq::set_background_color(Color value) noexcept
{
    switch (value) {
        case Color::black:
            return "\033[40m";
        case Color::red:
            return "\033[41m";
        case Color::green:
            return "\033[42m";
        case Color::yellow:
            return "\033[43m";
        case Color::blue:
            return "\033[44m";
        case Color::magenta:
            return "\033[45m";
        case Color::cyan:
            return "\033[46m";
        case Color::white:
            return "\033[47m";
    }
    return nullptr;
}


constexpr const char* seq::reset_color() noexcept
{
    return "\033[39m";
}


constexpr const char* seq::reset_background_color() noexcept
{
    return "\033[49m";
}


inline void TextAttributes::set_weight(Weight value)
{
    m_fields.weight = uint(value);
}


inline void TextAttributes::set_underline(bool value)
{
    m_fields.underline = uint(value);
}


inline void TextAttributes::set_blink(bool value)
{
    m_fields.blink = uint(value);
}


inline void TextAttributes::set_reverse(bool value)
{
    m_fields.reverse = uint(value);
}


inline void TextAttributes::set_color(Color value)
{
    m_fields.has_color = uint(true);
    m_fields.color     = uint(value);
}


inline void TextAttributes::unset_color()
{
    m_fields.has_color = uint(false);
    m_fields.color     = uint(Color::black);
}


inline void TextAttributes::set_background_color(Color value)
{
    m_fields.has_background_color = uint(true);
    m_fields.background_color     = uint(value);
}


inline void TextAttributes::unset_background_color()
{
    m_fields.has_background_color = uint(false);
    m_fields.background_color     = uint(Color::black);
}


inline Weight TextAttributes::get_weight() const noexcept
{
    return Weight(m_fields.weight);
}


inline bool TextAttributes::get_underline() const noexcept
{
    return bool(m_fields.underline);
}


inline bool TextAttributes::get_blink() const noexcept
{
    return bool(m_fields.blink);
}


inline bool TextAttributes::get_reverse() const noexcept
{
    return bool(m_fields.reverse);
}


inline bool TextAttributes::has_color() const noexcept
{
    return bool(m_fields.has_color);
}


inline Color TextAttributes::get_color() const noexcept
{
    return Color(m_fields.color);
}


inline bool TextAttributes::has_background_color() const noexcept
{
    return bool(m_fields.has_background_color);
}


inline Color TextAttributes::get_background_color() const noexcept
{
    return Color(m_fields.background_color);
}


constexpr std::size_t TextAttributes::min_change_buffer_size() noexcept
{
    int weight           = 3; // `1;`, `2;`, or `22;`
    int underline        = 3; // `4;` or `24;`
    int blink            = 3; // `5;` or `25;`
    int reverse          = 3; // `7;` or `27;`
    int color            = 3; // `30;` -> `37;` else `39;`
    int background_color = 3; // `40;` -> `47;` else `49;`
    return std::size_t(3 +  // leading and trailing (`\033[...m`)
                       1 +  // reset (`;`)
                       weight +
                       underline +
                       blink +
                       reverse +
                       color +
                       background_color +
                       -1); // regret last semicolon
}


inline TextAttributes::TextAttributes() noexcept
{
    m_mem = 0;
}


inline bool TextAttributes::operator==(const TextAttributes& attr) const noexcept
{
    return (m_mem == attr.m_mem);
}


inline bool TextAttributes::operator!=(const TextAttributes& attr) const noexcept
{
    return (m_mem != attr.m_mem);
}


} // namespace archon::base::terminal

#endif // ARCHON__BASE__TERMINAL_HPP
