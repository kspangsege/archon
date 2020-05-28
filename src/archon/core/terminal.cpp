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


#include <cstring>
#include <algorithm>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/locale.hpp>

#include <archon/core/terminal.hpp>


using namespace archon;
namespace terminal = core::terminal;


auto terminal::TextAttributes::change(TextAttributes from, TextAttributes to, char* buffer) noexcept -> std::size_t
{
    if (ARCHON_UNLIKELY(to == from))
        return 0;

    char* curr = buffer;
    auto add = [&](const char* data) {
        std::size_t size = std::strlen(data);
        curr = std::copy_n(data, size, curr);
    };
    bool nonfirst_arg = false;
    auto add_arg = [&](const char* arg) {
        if (nonfirst_arg)
            add(";");
        add(arg);
        nonfirst_arg = true;
    };
    auto add_weight = [&] {
        switch (to.get_weight()) {
            case terminal::Weight::normal:
                add_arg("22");
                break;
            case terminal::Weight::bold:
                add_arg("1");
                break;
            case terminal::Weight::faint:
                add_arg("2");
                break;
        }
    };
    auto add_underline = [&] {
        if (!to.get_underline()) {
            add_arg("24");
        }
        else {
            add_arg("4");
        }
    };
    auto add_blink = [&] {
        if (!to.get_blink()) {
            add_arg("25");
        }
        else {
            add_arg("5");
        }
    };
    auto add_reverse = [&] {
        if (!to.get_reverse()) {
            add_arg("27");
        }
        else {
            add_arg("7");
        }
    };
    auto add_color = [&] {
        if (to.has_color()) {
            switch (to.get_color()) {
                case terminal::Color::black:
                    add_arg("30");
                    break;
                case terminal::Color::red:
                    add_arg("31");
                    break;
                case terminal::Color::green:
                    add_arg("32");
                    break;
                case terminal::Color::yellow:
                    add_arg("33");
                    break;
                case terminal::Color::blue:
                    add_arg("34");
                    break;
                case terminal::Color::magenta:
                    add_arg("35");
                    break;
                case terminal::Color::cyan:
                    add_arg("36");
                    break;
                case terminal::Color::white:
                    add_arg("37");
                    break;
            }
        }
        else {
            add_arg("39");
        }
    };
    auto add_background_color = [&] {
        if (to.has_background_color()) {
            switch (to.get_background_color()) {
                case terminal::Color::black:
                    add_arg("40");
                    break;
                case terminal::Color::red:
                    add_arg("41");
                    break;
                case terminal::Color::green:
                    add_arg("42");
                    break;
                case terminal::Color::yellow:
                    add_arg("43");
                    break;
                case terminal::Color::blue:
                    add_arg("44");
                    break;
                case terminal::Color::magenta:
                    add_arg("45");
                    break;
                case terminal::Color::cyan:
                    add_arg("46");
                    break;
                case terminal::Color::white:
                    add_arg("47");
                    break;
            }
        }
        else {
            add_arg("49");
        }
    };
    bool nondefault_weight           = (to.get_weight() != terminal::Weight::normal);
    bool nondefault_underline        = to.get_underline();
    bool nondefault_blink            = to.get_blink();
    bool nondefault_reverse          = to.get_reverse();
    bool nondefault_color            = to.has_color();
    bool nondefault_background_color = to.has_background_color();

    bool change_weight           = (to.get_weight()           != from.get_weight());
    bool change_underline        = (to.get_underline()        != from.get_underline());
    bool change_blink            = (to.get_blink()            != from.get_blink());
    bool change_reverse          = (to.get_reverse()          != from.get_reverse());
    bool change_color            = (to.has_color()            != from.has_color() ||
                                    to.get_color()            != from.get_color());
    bool change_background_color = (to.has_background_color() != from.has_background_color() ||
                                    to.get_background_color() != from.get_background_color());

    int weight_0           = (nondefault_weight           ? 2 : 3); // `1;` or `2;`   else `22;`
    int underline_0        = (nondefault_underline        ? 2 : 3); // `4;`           else `24;`
    int blink_0            = (nondefault_blink            ? 2 : 3); // `5;`           else `25;`
    int reverse_0          = (nondefault_reverse          ? 2 : 3); // `7;`           else `27;`
    int color_0            = (nondefault_color            ? 3 : 3); // `30;` -> `37;` else `39;`
    int background_color_0 = (nondefault_background_color ? 3 : 3); // `40;` -> `47;` else `49;`

    int weight_1           = (change_weight           ? weight_0           : 0);
    int underline_1        = (change_underline        ? underline_0        : 0);
    int blink_1            = (change_blink            ? blink_0            : 0);
    int reverse_1          = (change_reverse          ? reverse_0          : 0);
    int color_1            = (change_color            ? color_0            : 0);
    int background_color_1 = (change_background_color ? background_color_0 : 0);

    int weight_2           = (nondefault_weight           ? 2 : 0); // `1;` or `2;`
    int underline_2        = (nondefault_underline        ? 2 : 0); // `4;`
    int blink_2            = (nondefault_blink            ? 2 : 0); // `5;`
    int reverse_2          = (nondefault_reverse          ? 2 : 0); // `7;`
    int color_2            = (nondefault_color            ? 3 : 0); // `30;` -> `37;`
    int background_color_2 = (nondefault_background_color ? 3 : 0); // `40;` -> `47;`

    // When not starting by resetting all text attributes to their defaults
    int size_1 = weight_1 + underline_1 + blink_1 + reverse_1 + color_1 + background_color_1;

    // When starting by resetting all text attributes to their defaults
    int size_2 = 1 + weight_2 + underline_2 + blink_2 + reverse_2 + color_2 + background_color_2;

    add("\033[");
    if (size_1 <= size_2) {
        if (ARCHON_UNLIKELY(change_weight))
            add_weight();
        if (ARCHON_UNLIKELY(change_underline))
            add_underline();
        if (ARCHON_UNLIKELY(change_blink))
            add_blink();
        if (ARCHON_UNLIKELY(change_reverse))
            add_reverse();
        if (ARCHON_UNLIKELY(change_color))
            add_color();
        if (ARCHON_UNLIKELY(change_background_color))
            add_background_color();
    }
    else {
        // Empty argument is short for `0` which is "reset"
        add_arg("");
        if (ARCHON_UNLIKELY(nondefault_weight))
            add_weight();
        if (ARCHON_UNLIKELY(nondefault_underline))
            add_underline();
        if (ARCHON_UNLIKELY(nondefault_blink))
            add_blink();
        if (ARCHON_UNLIKELY(nondefault_reverse))
            add_reverse();
        if (ARCHON_UNLIKELY(nondefault_color))
            add_color();
        if (ARCHON_UNLIKELY(nondefault_background_color))
            add_background_color();
    }
    add("m");

    return std::size_t(curr - buffer);
}


bool terminal::should_enable_escape_sequences(terminal::When when, bool is_terminal, const std::locale& locale)
{
    switch (when) {
        case terminal::When::auto_:
#if ARCHON_WINDOWS
            // On Windows, the standard Command Prompt (`cmd.exe`) does not support escape
            // sequences, but the Visual Studio Command Prompt does. The MinGW terminal also
            // support escape sequences.
            return false;
#else
            return (is_terminal && core::assume_locale_has_escape(locale)); // Throws
#endif
        case terminal::When::never:
            return false;
        case terminal::When::always:
            return true;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}
