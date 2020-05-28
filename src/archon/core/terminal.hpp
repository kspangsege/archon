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

#ifndef ARCHON_X_CORE_X_TERMINAL_HPP
#define ARCHON_X_CORE_X_TERMINAL_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <locale>

#include <archon/core/enum.hpp>


/// \brief Text terminal functionality.
///
/// This namespace provides functionality relating to text terminals, especially the
/// generation of ANSI escape sequences that control text color and style.
///
namespace archon::core::terminal {


/// \brief Available weights for terminal text.
///
/// These are the weights (degrees of boldness) that are availble for terminal text. See
/// \ref terminal::seq::set_weight() and terminal::TextAttributes::set_weight().
///
enum class Weight { normal, bold, faint };


/// \brief Available colors for terminal text.
///
/// These are the colors that are availble as text and background color. See \ref
/// terminal::seq::set_color() and terminal::TextAttributes::set_color().
///
enum class Color { black, red, green, yellow, blue, magenta, cyan, white };


/// \brief Simple escape sequences.
///
/// This namespace holds a set of functions that return various escape sequences
/// corresponding to simple changes of text rendition style. See \ref
/// terminal::seq::set_weight().
///
namespace seq {

/// \{
///
/// \brief Escape sequences to control various text attributes.
///
/// These function return ANSI escape sequences that control various text attributes. Only
/// SGR (Select Graphic Rendition) type escape sequences will be emitted.
///
/// Normally, such escape sequences should only be emitted in output that is sent to a text
/// terminal, and only if the text terminal is compatible with SGR escape sequences as
/// defined by ANSI.
///
/// CAUTION: Given a particular target locale, the returned sequences are useful only if
/// that locale has the escape character in the sense defined by \ref
/// core::assume_locale_has_escape(). On the other hand, if \ref
/// core::assume_locale_has_escape() returns `true`, the returned escape sequences are
/// widenable using `std::ctype<wchar_t>::widen()`.
///
/// \sa \ref terminal::TextAttributes::change()
///
constexpr auto reset() noexcept -> const char*;
constexpr auto set_weight(terminal::Weight) noexcept -> const char*;
constexpr auto set_underline(bool) noexcept -> const char*;
constexpr auto set_blink(bool) noexcept -> const char*;
constexpr auto set_reverse(bool) noexcept -> const char*;
constexpr auto set_color(terminal::Color) noexcept -> const char*;
constexpr auto set_background_color(terminal::Color) noexcept -> const char*;
constexpr auto reset_color() noexcept -> const char*;
constexpr auto reset_background_color() noexcept -> const char*;
/// \}

} // namespace seq



/// \brief Full specification of text rendition style.
///
/// An object of this type fully specifies a text rendition style of the terminal. Given two
/// such objects, one can call \ref change() in order to generate an escape sequence that
/// switches between them.
///
class TextAttributes {
public:
    void set_weight(terminal::Weight);
    void set_underline(bool);
    void set_blink(bool);
    void set_reverse(bool);
    void set_color(terminal::Color);
    void unset_color();
    void set_background_color(terminal::Color);
    void unset_background_color();

    auto get_weight() const noexcept -> terminal::Weight;
    bool get_underline() const noexcept;
    bool get_blink() const noexcept;
    bool get_reverse() const noexcept;
    bool has_color() const noexcept;
    auto get_color() const noexcept -> terminal::Color;
    bool has_background_color() const noexcept;
    auto get_background_color() const noexcept -> terminal::Color;

    /// \brief Generate escape sequence for swithcing between styles.
    ///
    /// This function generates an escape sequence that can be used to switch from one style
    /// (\p from) to another (\p to). Only SGR (Select Graphic Rendition) type escape
    /// sequences will be generated. The generated escape sequence is stored in the
    /// specified buffer (\p buffer), and the size of the sequence is returned.
    ///
    /// Normally, such escape sequences should only be emitted in output that is sent to a
    /// text terminal, and only if the text terminal is compatible with SGR escape sequences
    /// as defined by ANSI.
    ///
    /// The size of the specified buffer must be greater than, or equal to the value
    /// returned by \ref min_change_buffer_size().
    ///
    /// The result is meaningful only if \p from corresponds to what is actually the curren
    /// text rendition style of the target terminal.
    ///
    /// A default constructed text attributes object corresponds to the default text
    /// rendition style of the terminal. This is also the style obtained after emitting the
    /// "reset" escape sequence (\ref terminal::seq::reset()).
    ///
    /// CAUTION: Given a particular target locale, the result of this function is reliable
    /// only if that locale has the escape character in the sense defined by \ref
    /// core::assume_locale_has_escape(). On the other hand, if \ref
    /// core::assume_locale_has_escape() returns `true`, the returned escape sequences are
    /// widenable using `std::ctype<wchar_t>::widen()`.
    ///
    /// \sa \ref terminal::seq::set_weight() and friends.
    ///
    static auto change(TextAttributes from, TextAttributes to, char* buffer) noexcept -> std::size_t;

    /// \brief Minimum size of change buffer.
    ///
    /// See \ref change().
    ///
    static constexpr auto min_change_buffer_size() noexcept -> std::size_t;

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



/// \brief When to enable escape sequences.
///
/// This enumeration can be used, for example in a command-line interface, to specify when
/// ANSI escape sequences should be enabled. Autodetection can be carried out by \ref
/// terminal::should_enable_escape_sequences().
///
enum class When {
    auto_,  ///< Autodetect.
    never,  ///< Never enable ANSI escape sequences.
    always, ///< Always enable ANSI escape sequences.
};


/// \brief Decide whether to enable escape sequences.
///
/// This function decides whether to enable ANSI escape sequences in text output based on \p
/// when, whether the output is sent to a text terminal (\p is_terminal), the locale in
/// effect, and the target platform.
///
bool should_enable_escape_sequences(terminal::When when, bool is_terminal, const std::locale&);








// Implementation


constexpr auto seq::reset() noexcept -> const char*
{
    return "\033[m";
}


constexpr auto seq::set_weight(Weight value) noexcept -> const char*
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


constexpr auto seq::set_underline(bool value) noexcept -> const char*
{
    return (value ? "\033[4m" : "\033[24m");
}


constexpr auto seq::set_blink(bool value) noexcept -> const char*
{
    return (value ? "\033[5m" : "\033[25m");
}


constexpr auto seq::set_reverse(bool value) noexcept -> const char*
{
    return (value ? "\033[7m" : "\033[27m");
}


constexpr auto seq::set_color(Color value) noexcept -> const char*
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


constexpr auto seq::set_background_color(Color value) noexcept -> const char*
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


constexpr auto seq::reset_color() noexcept -> const char*
{
    return "\033[39m";
}


constexpr auto seq::reset_background_color() noexcept -> const char*
{
    return "\033[49m";
}


inline void TextAttributes::set_weight(terminal::Weight value)
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


inline void TextAttributes::set_background_color(terminal::Color value)
{
    m_fields.has_background_color = uint(true);
    m_fields.background_color     = uint(value);
}


inline void TextAttributes::unset_background_color()
{
    m_fields.has_background_color = uint(false);
    m_fields.background_color     = uint(Color::black);
}


inline auto TextAttributes::get_weight() const noexcept -> terminal::Weight
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


inline auto TextAttributes::get_color() const noexcept -> terminal::Color
{
    return Color(m_fields.color);
}


inline bool TextAttributes::has_background_color() const noexcept
{
    return bool(m_fields.has_background_color);
}


inline auto TextAttributes::get_background_color() const noexcept -> terminal::Color
{
    return Color(m_fields.background_color);
}


constexpr auto TextAttributes::min_change_buffer_size() noexcept -> std::size_t
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


} // namespace archon::core::terminal

namespace archon::core {

template<> struct EnumTraits<terminal::When> {
    static constexpr bool is_specialized = true;
    struct Spec {
        static constexpr core::EnumAssoc map[] = {
            { int(terminal::When::auto_),  "auto"   },
            { int(terminal::When::never),  "never"  },
            { int(terminal::When::always), "always" },
        };
    };
    static constexpr bool ignore_case = false;
};

} // namespace archon::core

#endif // ARCHON_X_CORE_X_TERMINAL_HPP
