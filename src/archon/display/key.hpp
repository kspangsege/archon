// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_DISPLAY_X_KEY_HPP
#define ARCHON_X_DISPLAY_X_KEY_HPP

/// \file


namespace archon::display {


/// \brief Well-known keys.
///
/// This is a set of "well-known" keys. These enumerants can be used to refer to keys in a
/// manner that is independent of the underlying display implementation.
///
/// A well-known key can be mapped to its corresponding key code (\ref display::KeyCode), if
/// one exists, using \ref display::Connection::try_map_key_to_key_code().
///
/// Likewise, a key code (\ref display::KeyCode) can be mapped to the corresponding
/// well-known key, if one exists, using display::Connection::try_map_key_code_to_key().
///
/// \sa \ref display::KeyCode
///
enum class Key {
    // TTY functions
    backspace,            ///< Backspace
    tab,                  ///< Tab
    line_feed,            ///< Line feed
    clear,                ///< Clear
    return_,              ///< Return
    pause,                ///< Pause
    scroll_lock,          ///< Scroll lock
    sys_req,              ///< System request
    escape,               ///< Escape
    delete_,              ///< Delete

    // Cursor control & motion             
    left,                 ///< Move left, left arrow
    right,                ///< Move right, right arrow
    up,                   ///< Move up, up arrow
    down,                 ///< Move down, down arrow
    page_up,              ///< Page up
    page_down,            ///< Page down
    home,                 ///< Home
    begin,                ///< Beginning of line (BOL)
    end,                  ///< End of line (EOL)
    prior,                ///< Prior, previous
    next,                 ///< Next

    // Misc functions             
    select,               ///< Select, mark
    print,                ///< Print
    execute,              ///< Execute, run, do
    insert,               ///< Insert, insert here
    undo,                 ///< Undo
    redo,                 ///< Redo, again
    menu,                 ///< Menu
    find,                 ///< Find, search
    cancel,               ///< Cancel, stop, abort, exit
    help,                 ///< Help
    break_,               ///< Break
    mode_switch,          ///< Character set switch
    num_lock,             ///< Num lock

    // Keypad             
    keypad_digit_0,       ///< The digit "0" on keypad
    keypad_digit_1,       ///< The digit "1" on keypad
    keypad_digit_2,       ///< The digit "2" on keypad
    keypad_digit_3,       ///< The digit "3" on keypad
    keypad_digit_4,       ///< The digit "4" on keypad
    keypad_digit_5,       ///< The digit "5" on keypad
    keypad_digit_6,       ///< The digit "6" on keypad
    keypad_digit_7,       ///< The digit "7" on keypad
    keypad_digit_8,       ///< The digit "8" on keypad
    keypad_digit_9,       ///< The digit "9" on keypad
/*
#define XK_KP_Space                      0xff80  ///< Space on keypad
#define XK_KP_Tab                        0xff89  ///< 
#define XK_KP_Enter                      0xff8d  ///< Enter on keypad
#define XK_KP_F1                         0xff91  ///< PF1, KP_A, ...
#define XK_KP_F2                         0xff92
#define XK_KP_F3                         0xff93
#define XK_KP_F4                         0xff94
#define XK_KP_Home                       0xff95
#define XK_KP_Left                       0xff96
#define XK_KP_Up                         0xff97
#define XK_KP_Right                      0xff98
#define XK_KP_Down                       0xff99
#define XK_KP_Prior                      0xff9a
#define XK_KP_Page_Up                    0xff9a
#define XK_KP_Next                       0xff9b
#define XK_KP_Page_Down                  0xff9b
#define XK_KP_End                        0xff9c
#define XK_KP_Begin                      0xff9d
#define XK_KP_Insert                     0xff9e
#define XK_KP_Delete                     0xff9f
#define XK_KP_Equal                      0xffbd  ///< Equals
#define XK_KP_Multiply                   0xffaa
#define XK_KP_Add                        0xffab  ///< Plus sign on keypad
#define XK_KP_Separator                  0xffac  ///< Separator, often comma
#define XK_KP_Subtract                   0xffad
#define XK_KP_Decimal                    0xffae
#define XK_KP_Divide                     0xffaf
*/

    keypad_plus_sign,     ///< Plus sign on keypad
    keypad_minus_sign,    ///< Minus sign on keypad
    

    // Function keys             
    

    // Modifier keys
    shift_left,           ///< Left side shift
    shift_right,          ///< Right side shift
    ctrl_left,            ///< Left side control
    ctrl_right,           ///< Right side control
    alt_left,             ///< Left side alt
    alt_right,            ///< Right side alt (or "Alt Gr")
    meta_left,            ///< Left side meta (Windows / Apple / command)
    meta_right,           ///< Right side meta (Windows / Apple / command)
    caps_lock,            ///< Caps lock             
    shift_lock,           ///< Shift lock             

    // Basic Latin
    space,                ///< Space
    exclamation_mark,     ///< Exclamation mark
    quotation_mark,       ///< Quotation mark / double quote
    hash_mark,            ///< Hash mark / number sign
    dollar_sign,          ///< Dollar sign
    percent_sign,         ///< Percent sign
    ampersand,            ///< Ampersand
    apostrophe,           ///< Apostrophe / quote
    left_parenthesis,     ///< Left parenthesis
    right_parenthesis,    ///< Right parenthesis
    asterisk,             ///< Asterisk
    plus_sign,            ///< Plus sign
    comma,                ///< Comma
    minus_sign,           ///< Minus sign / dash
    period,               ///< Period / full stop
    slash,                ///< Slash / solidus
    digit_0,              ///< The digit "0"
    digit_1,              ///< The digit "1"
    digit_2,              ///< The digit "2"
    digit_3,              ///< The digit "3"
    digit_4,              ///< The digit "4"
    digit_5,              ///< The digit "5"
    digit_6,              ///< The digit "6"
    digit_7,              ///< The digit "7"
    digit_8,              ///< The digit "8"
    digit_9,              ///< The digit "9"
    colon,                ///< Colon
    semicolon,            ///< Semicolon
    less_than_sign,       ///< Less-than sign
    equal_sign,           ///< Equal sign
    greater_than_sign,    ///< Greater-than sign
    question_mark,        ///< Question mark
    at_sign,              ///< At sign
    upper_case_a,         ///< Upper case letter "A"
    upper_case_b,         ///< Upper case letter "B"
    upper_case_c,         ///< Upper case letter "C"
    upper_case_d,         ///< Upper case letter "D"
    upper_case_e,         ///< Upper case letter "E"
    upper_case_f,         ///< Upper case letter "F"
    upper_case_g,         ///< Upper case letter "G"
    upper_case_h,         ///< Upper case letter "H"
    upper_case_i,         ///< Upper case letter "I"
    upper_case_j,         ///< Upper case letter "J"
    upper_case_k,         ///< Upper case letter "K"
    upper_case_l,         ///< Upper case letter "L"
    upper_case_m,         ///< Upper case letter "M"
    upper_case_n,         ///< Upper case letter "N"
    upper_case_o,         ///< Upper case letter "O"
    upper_case_p,         ///< Upper case letter "P"
    upper_case_q,         ///< Upper case letter "Q"
    upper_case_r,         ///< Upper case letter "R"
    upper_case_s,         ///< Upper case letter "S"
    upper_case_t,         ///< Upper case letter "T"
    upper_case_u,         ///< Upper case letter "U"
    upper_case_v,         ///< Upper case letter "V"
    upper_case_w,         ///< Upper case letter "W"
    upper_case_x,         ///< Upper case letter "X"
    upper_case_y,         ///< Upper case letter "Y"
    upper_case_z,         ///< Upper case letter "Z"
    left_square_bracket,  ///< Left square bracket
    backslash,            ///< Backslash / reverse solidus
    right_square_bracket, ///< Right square bracket
    circumflex_accent,    ///< Circumflex accent / caret
    underscore,           ///< Underscore
    grave_accent,         ///< Grave accent / backquote
    lower_case_a,         ///< Lower case letter "a"
    lower_case_b,         ///< Lower case letter "b"
    lower_case_c,         ///< Lower case letter "c"
    lower_case_d,         ///< Lower case letter "d"
    lower_case_e,         ///< Lower case letter "e"
    lower_case_f,         ///< Lower case letter "f"
    lower_case_g,         ///< Lower case letter "g"
    lower_case_h,         ///< Lower case letter "h"
    lower_case_i,         ///< Lower case letter "i"
    lower_case_j,         ///< Lower case letter "j"
    lower_case_k,         ///< Lower case letter "k"
    lower_case_l,         ///< Lower case letter "l"
    lower_case_m,         ///< Lower case letter "m"
    lower_case_n,         ///< Lower case letter "n"
    lower_case_o,         ///< Lower case letter "o"
    lower_case_p,         ///< Lower case letter "p"
    lower_case_q,         ///< Lower case letter "q"
    lower_case_r,         ///< Lower case letter "r"
    lower_case_s,         ///< Lower case letter "s"
    lower_case_t,         ///< Lower case letter "t"
    lower_case_u,         ///< Lower case letter "u"
    lower_case_v,         ///< Lower case letter "v"
    lower_case_w,         ///< Lower case letter "w"
    lower_case_x,         ///< Lower case letter "x"
    lower_case_y,         ///< Lower case letter "y"
    lower_case_z,         ///< Lower case letter "z"
    left_curly_bracket,   ///< Left curly bracket
    vertical_bar,         ///< Vertical bar
    right_curly_bracket,  ///< Right curly bracket
    tilde,                ///< Tilde

    // Latin-1 Supplement
    
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_KEY_HPP
