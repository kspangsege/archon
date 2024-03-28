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
    backspace                   =  100, ///< Backspace
    tab                         =  101, ///< Tab
    line_feed                   =  102, ///< Line feed
    clear                       =  103, ///< Clear
    return_                     =  104, ///< Return
    pause                       =  105, ///< Pause
    scroll_lock                 =  106, ///< Scroll lock
    sys_req                     =  107, ///< System request
    escape                      =  108, ///< Escape
    delete_                     =  109, ///< Delete

    // Cursor control
    left                        =  200, ///< Move left, left arrow
    right                       =  201, ///< Move right, right arrow
    up                          =  202, ///< Move up, up arrow
    down                        =  203, ///< Move down, down arrow
    prior                       =  204, ///< Prior (page up)
    next                        =  205, ///< Next (page down)
    home                        =  206, ///< Home
    begin                       =  207, ///< Beginning of line (BOL)
    end                         =  208, ///< End of line (EOL)

    // Misc functions
    select                      =  300, ///< Select, mark
    print_screen                =  301, ///< Print screen
    execute                     =  302, ///< Execute, run, do
    insert                      =  303, ///< Insert, insert here
    undo                        =  304, ///< Undo
    redo                        =  305, ///< Redo, again
    menu                        =  306, ///< Menu
    find                        =  307, ///< Find, search
    cancel                      =  308, ///< Cancel, stop, abort, exit
    help                        =  309, ///< Help
    break_                      =  310, ///< Break
    mode_switch                 =  311, ///< Character set switch
    num_lock                    =  312, ///< Num lock

    // Keypad
    keypad_add                  =  400, ///< Addition operator on keypad
    keypad_subtract             =  401, ///< Subtraction operator on keypad
    keypad_multiply             =  402, ///< Multiplication operator on keypad
    keypad_divide               =  403, ///< Division operator on keypad
    keypad_left                 =  404, ///< Left arrow on keypad
    keypad_right                =  405, ///< Right arrow on keypad
    keypad_up                   =  406, ///< Up arrow on keypad
    keypad_down                 =  407, ///< Down arrow on keypad
    keypad_prior                =  408, ///< Prior on keypad (page up)
    keypad_next                 =  409, ///< Next on keypad (page down)
    keypad_home                 =  410, ///< Home on keypad
    keypad_begin                =  411, ///< Beginning of line on keypad (BOL)
    keypad_end                  =  412, ///< End of line on keypad (BOL)
    keypad_insert               =  415, ///< Insert on keypad
    keypad_delete               =  416, ///< Delete on keypad
    keypad_enter                =  417, ///< Enter on keypad
    keypad_digit_0              =  418, ///< The digit "0" on keypad
    keypad_digit_1              =  419, ///< The digit "1" on keypad
    keypad_digit_2              =  420, ///< The digit "2" on keypad
    keypad_digit_3              =  421, ///< The digit "3" on keypad
    keypad_digit_4              =  422, ///< The digit "4" on keypad
    keypad_digit_5              =  423, ///< The digit "5" on keypad
    keypad_digit_6              =  424, ///< The digit "6" on keypad
    keypad_digit_7              =  425, ///< The digit "7" on keypad
    keypad_digit_8              =  426, ///< The digit "8" on keypad
    keypad_digit_9              =  427, ///< The digit "9" on keypad
    keypad_decimal_separator    =  428, ///< Decimal separator on keypad (period)
    keypad_thousands_separator  =  429, ///< Thousands separator on keypad (comma)
    keypad_equal_sign           =  430, ///< Equal sign on keypad
    keypad_space                =  431, ///< Space on keypad
    keypad_tab                  =  432, ///< Tab on keypad
    keypad_f1                   =  433, ///< 1st function key on keypad
    keypad_f2                   =  434, ///< 2nd function key on keypad
    keypad_f3                   =  435, ///< 3rd function key on keypad
    keypad_f4                   =  436, ///< 4th function key on keypad

    // Function keys
    f1                          =  500, ///<  1st function key
    f2                          =  501, ///<  2nd function key
    f3                          =  502, ///<  3rd function key
    f4                          =  503, ///<  4th function key
    f5                          =  504, ///<  5th function key
    f6                          =  505, ///<  6th function key
    f7                          =  506, ///<  7th function key
    f8                          =  507, ///<  8th function key
    f9                          =  508, ///<  9th function key
    f10                         =  509, ///< 10th function key
    f11                         =  510, ///< 11th function key
    f12                         =  511, ///< 12th function key
    f13                         =  512, ///< 13th function key
    f14                         =  513, ///< 14th function key
    f15                         =  514, ///< 15th function key
    f16                         =  515, ///< 16th function key
    f17                         =  516, ///< 17th function key
    f18                         =  517, ///< 14th function key
    f19                         =  518, ///< 18th function key
    f20                         =  519, ///< 20th function key
    f21                         =  520, ///< 21th function key
    f22                         =  521, ///< 22th function key
    f23                         =  522, ///< 23th function key
    f24                         =  523, ///< 24th function key
    f25                         =  524, ///< 25th function key
    f26                         =  525, ///< 26th function key
    f27                         =  526, ///< 27th function key
    f28                         =  527, ///< 24th function key
    f29                         =  528, ///< 28th function key
    f30                         =  529, ///< 30th function key
    f31                         =  530, ///< 31th function key
    f32                         =  531, ///< 32th function key
    f33                         =  532, ///< 33th function key
    f34                         =  533, ///< 34th function key
    f35                         =  534, ///< 35th function key

    // Modifier keys
    shift_left                  =  600, ///< Left side shift
    shift_right                 =  601, ///< Right side shift
    ctrl_left                   =  602, ///< Left side control
    ctrl_right                  =  603, ///< Right side control
    alt_left                    =  604, ///< Left side alt
    alt_right                   =  605, ///< Right side alt (or "Alt Gr")
    meta_left                   =  606, ///< Left side meta (Windows / Apple / command)
    meta_right                  =  607, ///< Right side meta (Windows / Apple / command)
    caps_lock                   =  608, ///< Caps lock
    shift_lock                  =  609, ///< Shift lock
    dead_grave                  =  610, ///< Combining grave accent
    dead_acute                  =  611, ///< Combining acute accent
    dead_circumflex             =  612, ///< Combining circumflex
    dead_tilde                  =  613, ///< Combining tilde
    dead_macron                 =  614, ///< Combining macron
    dead_breve                  =  615, ///< Combining breve
    dead_abovedot               =  616, ///< Combining dot above
    dead_diaeresis              =  617, ///< Combining diaeresis
    dead_abovering              =  618, ///< Combining ring above
    dead_doubleacute            =  619, ///< Combining double acute accent
    dead_caron                  =  620, ///< Combining caron
    dead_cedilla                =  621, ///< Combining cedilla
    dead_ogonek                 =  622, ///< Combining ogonek
    dead_iota                   =  623, ///< Combining iota subscript
    dead_voiced_sound           =  624, ///< Combining voiced sound
    dead_semivoiced_sound       =  625, ///< Combining semi-voiced sound
    dead_belowdot               =  626, ///< Combining dot below
    dead_hook                   =  627, ///< Combining hook
    dead_horn                   =  628, ///< Combining horn
    dead_stroke                 =  629, ///< Combining stroke
    dead_psili                  =  630, ///< Combining comma above
    dead_dasia                  =  631, ///< Combining reversed comma above
    dead_doublegrave            =  632, ///< Combining double grave accent
    dead_belowring              =  633, ///< Combining ring below
    dead_belowmacron            =  634, ///< Combining macron below
    dead_belowcircumflex        =  635, ///< Combining circumflex below
    dead_belowtilde             =  636, ///< Combining tilde below
    dead_belowbreve             =  637, ///< Combining breve below
    dead_belowdiaeresis         =  638, ///< Combining diaeresis below
    dead_invertedbreve          =  639, ///< Combining inverted breve
    dead_belowcomma             =  640, ///< Combining comma below
    dead_currency               =  641, ///< Combining currency

    // Basic Latin
    space                       =  700, ///< U+0020 Space
    exclamation_mark            =  701, ///< U+0021 Exclamation mark
    quotation_mark              =  702, ///< U+0022 Quotation mark
    number_sign                 =  703, ///< U+0023 Number sign
    dollar_sign                 =  704, ///< U+0024 Dollar sign
    percent_sign                =  705, ///< U+0025 Percent sign
    ampersand                   =  706, ///< U+0026 Ampersand
    apostrophe                  =  707, ///< U+0027 Apostrophe
    left_parenthesis            =  708, ///< U+0028 Left parenthesis
    right_parenthesis           =  709, ///< U+0029 Right parenthesis
    asterisk                    =  710, ///< U+002A Asterisk
    plus_sign                   =  711, ///< U+002B Plus sign
    comma                       =  712, ///< U+002C Comma
    hyphen_minus                =  713, ///< U+002D Hyphen-minus
    full_stop                   =  714, ///< U+002E Full stop
    solidus                     =  715, ///< U+002F Solidus
    digit_0                     =  716, ///< U+0030 Digit zero
    digit_1                     =  717, ///< U+0031 Digit one
    digit_2                     =  718, ///< U+0032 Digit two
    digit_3                     =  719, ///< U+0033 Digit three
    digit_4                     =  720, ///< U+0034 Digit four
    digit_5                     =  721, ///< U+0035 Digit five
    digit_6                     =  722, ///< U+0036 Digit six
    digit_7                     =  723, ///< U+0037 Digit seven
    digit_8                     =  724, ///< U+0038 Digit eight
    digit_9                     =  725, ///< U+0039 Digit nine
    colon                       =  726, ///< U+003A Colon
    semicolon                   =  727, ///< U+003B Semicolon
    less_than_sign              =  728, ///< U+003C Less-than sign
    equals_sign                 =  729, ///< U+003D Equals sign
    greater_than_sign           =  730, ///< U+003E Greater-than sign
    question_mark               =  731, ///< U+003F Question mark
    commercial_at               =  732, ///< U+0040 Commercial at
    capital_a                   =  733, ///< U+0041 Latin capital letter A
    capital_b                   =  734, ///< U+0042 Latin capital letter B
    capital_c                   =  735, ///< U+0043 Latin capital letter C
    capital_d                   =  736, ///< U+0044 Latin capital letter D
    capital_e                   =  737, ///< U+0045 Latin capital letter E
    capital_f                   =  738, ///< U+0046 Latin capital letter F
    capital_g                   =  739, ///< U+0047 Latin capital letter G
    capital_h                   =  740, ///< U+0048 Latin capital letter H
    capital_i                   =  741, ///< U+0049 Latin capital letter I
    capital_j                   =  742, ///< U+004A Latin capital letter J
    capital_k                   =  743, ///< U+004B Latin capital letter K
    capital_l                   =  744, ///< U+004C Latin capital letter L
    capital_m                   =  745, ///< U+004D Latin capital letter M
    capital_n                   =  746, ///< U+004E Latin capital letter N
    capital_o                   =  747, ///< U+004F Latin capital letter O
    capital_p                   =  748, ///< U+0050 Latin capital letter P
    capital_q                   =  749, ///< U+0051 Latin capital letter Q
    capital_r                   =  750, ///< U+0052 Latin capital letter R
    capital_s                   =  751, ///< U+0053 Latin capital letter S
    capital_t                   =  752, ///< U+0054 Latin capital letter T
    capital_u                   =  753, ///< U+0055 Latin capital letter U
    capital_v                   =  754, ///< U+0056 Latin capital letter V
    capital_w                   =  755, ///< U+0057 Latin capital letter W
    capital_x                   =  756, ///< U+0058 Latin capital letter X
    capital_y                   =  757, ///< U+0059 Latin capital letter Y
    capital_z                   =  758, ///< U+005A Latin capital letter Z
    left_square_bracket         =  759, ///< U+005B Left square bracket
    reverse_solidus             =  760, ///< U+005C Reverse solidus
    right_square_bracket        =  761, ///< U+005D Right square bracket
    circumflex_accent           =  762, ///< U+005E Circumflex accent
    low_line                    =  763, ///< U+005F Low line
    grave_accent                =  764, ///< U+0060 Grave accent
    small_a                     =  765, ///< U+0061 Latin small letter A
    small_b                     =  766, ///< U+0062 Latin small letter B
    small_c                     =  767, ///< U+0063 Latin small letter C
    small_d                     =  768, ///< U+0064 Latin small letter D
    small_e                     =  769, ///< U+0065 Latin small letter E
    small_f                     =  770, ///< U+0066 Latin small letter F
    small_g                     =  771, ///< U+0067 Latin small letter G
    small_h                     =  772, ///< U+0068 Latin small letter H
    small_i                     =  773, ///< U+0069 Latin small letter I
    small_j                     =  774, ///< U+006A Latin small letter J
    small_k                     =  775, ///< U+006B Latin small letter K
    small_l                     =  776, ///< U+006C Latin small letter L
    small_m                     =  777, ///< U+006D Latin small letter M
    small_n                     =  778, ///< U+006E Latin small letter N
    small_o                     =  779, ///< U+006F Latin small letter O
    small_p                     =  780, ///< U+0070 Latin small letter P
    small_q                     =  781, ///< U+0071 Latin small letter Q
    small_r                     =  782, ///< U+0072 Latin small letter R
    small_s                     =  783, ///< U+0073 Latin small letter S
    small_t                     =  784, ///< U+0074 Latin small letter T
    small_u                     =  785, ///< U+0075 Latin small letter U
    small_v                     =  786, ///< U+0076 Latin small letter V
    small_w                     =  787, ///< U+0077 Latin small letter W
    small_x                     =  788, ///< U+0078 Latin small letter X
    small_y                     =  789, ///< U+0079 Latin small letter Y
    small_z                     =  790, ///< U+007A Latin small letter Z
    left_curly_bracket          =  791, ///< U+007B Left curly bracket
    vertical_line               =  792, ///< U+007C Vertical line
    right_curly_bracket         =  793, ///< U+007D Right curly bracket
    tilde                       =  794, ///< U+007E Tilde

    // Latin-1 Supplement
    nobreak_space               =  800, ///< U+00A0 No-break space
    inverted_exclamation_mark   =  801, ///< U+00A1 Inverted exclamation mark
    cent_sign                   =  802, ///< U+00A2 Cent sign
    pound_sign                  =  803, ///< U+00A3 Pound sign
    currency_sign               =  804, ///< U+00A4 Currency sign
    yen_sign                    =  805, ///< U+00A5 Yen sign
    broken_bar                  =  806, ///< U+00A6 Broken bar
    section_sign                =  807, ///< U+00A7 Section sign
    diaeresis                   =  808, ///< U+00A8 Diaeresis
    copyright_sign              =  809, ///< U+00A9 Copyright sign
    feminine_ordinal_indicator  =  810, ///< U+00AA Feminine ordinal indicator
    left_guillemet              =  811, ///< U+00AB Left-pointing double angle quotation mark
    not_sign                    =  812, ///< U+00AC Not sign
    soft_hyphen                 =  813, ///< U+00AD Soft hyphen
    registered_sign             =  814, ///< U+00AE Registered sign
    macron                      =  815, ///< U+00AF Macron
    degree_sign                 =  816, ///< U+00B0 Degree sign
    plus_minus_sign             =  817, ///< U+00B1 Plus-minus sign
    superscript_two             =  818, ///< U+00B2 Superscript two
    superscript_three           =  819, ///< U+00B3 Superscript three
    acute_accent                =  820, ///< U+00B4 Acute accent
    micro_sign                  =  821, ///< U+00B5 Micro sign
    pilcrow_sign                =  822, ///< U+00B6 Pilcrow sign
    middle_dot                  =  823, ///< U+00B7 Middle dot
    cedilla                     =  824, ///< U+00B8 Cedilla
    superscript_one             =  825, ///< U+00B9 Superscript one
    masculine_ordinal_indicator =  826, ///< U+00BA Masculine ordinal indicator
    right_guillemet             =  827, ///< U+00BB Right-pointing double angle quotation mark
    one_quarter                 =  828, ///< U+00BC Vulgar fraction one quarter
    one_half                    =  829, ///< U+00BD Vulgar fraction one half
    three_quarters              =  830, ///< U+00BE Vulgar fraction three quarters
    inverted_question_mark      =  831, ///< U+00BF Inverted question mark
    capital_a_grave             =  832, ///< U+00C0 Latin capital letter A with grave
    capital_a_acute             =  833, ///< U+00C1 Latin capital letter A with acute
    capital_a_circumflex        =  834, ///< U+00C2 Latin capital letter A with circumflex
    capital_a_tilde             =  835, ///< U+00C3 Latin capital letter A with tilde
    capital_a_diaeresis         =  836, ///< U+00C4 Latin capital letter A with diaeresis
    capital_a_ring              =  837, ///< U+00C5 Latin capital letter A with ring above
    capital_ae_ligature         =  838, ///< U+00C6 Latin capital letter AE
    capital_c_cedilla           =  839, ///< U+00C7 Latin capital letter C with cedilla
    capital_e_grave             =  840, ///< U+00C8 Latin capital letter E with grave
    capital_e_acute             =  841, ///< U+00C9 Latin capital letter E with acute
    capital_e_circumflex        =  842, ///< U+00CA Latin capital letter E with circumflex
    capital_e_diaeresis         =  843, ///< U+00CB Latin capital letter E with diaeresis
    capital_i_grave             =  844, ///< U+00CC Latin capital letter I with grave
    capital_i_acute             =  845, ///< U+00CD Latin capital letter I with acute
    capital_i_circumflex        =  846, ///< U+00CE Latin capital letter I with circumflex
    capital_i_diaeresis         =  847, ///< U+00CF Latin capital letter I with diaeresis
    capital_eth                 =  848, ///< U+00D0 Latin capital letter ETH
    capital_n_tilde             =  849, ///< U+00D1 Latin capital letter N with tilde
    capital_o_grave             =  850, ///< U+00D2 Latin capital letter O with grave
    capital_o_acute             =  851, ///< U+00D3 Latin capital letter O with acute
    capital_o_circumflex        =  852, ///< U+00D4 Latin capital letter O with circumflex
    capital_o_tilde             =  853, ///< U+00D5 Latin capital letter O with tilde
    capital_o_diaeresis         =  854, ///< U+00D6 Latin capital letter O with diaeresis
    multiplication_sign         =  855, ///< U+00D7 Multiplication sign
    capital_o_stroke            =  856, ///< U+00D8 Latin capital letter O with stroke
    capital_u_grave             =  857, ///< U+00D9 Latin capital letter U with grave
    capital_u_acute             =  858, ///< U+00DA Latin capital letter U with acute
    capital_u_circumflex        =  859, ///< U+00DB Latin capital letter U with circumflex
    capital_u_diaeresis         =  860, ///< U+00DC Latin capital letter U with diaeresis
    capital_y_acute             =  861, ///< U+00DD Latin capital letter Y with acute
    capital_thorn               =  862, ///< U+00DE Latin capital letter THORN
    sharp_s                     =  863, ///< U+00DF Latin small letter SHARP S
    small_a_grave               =  864, ///< U+00E0 Latin small letter A with grave
    small_a_acute               =  865, ///< U+00E1 Latin small letter A with acute
    small_a_circumflex          =  866, ///< U+00E2 Latin small letter A with circumflex
    small_a_tilde               =  867, ///< U+00E3 Latin small letter A with tilde
    small_a_diaeresis           =  868, ///< U+00E4 Latin small letter A with diaeresis
    small_a_ring                =  869, ///< U+00E5 Latin small letter A with ring above
    small_ae_ligature           =  870, ///< U+00E6 Latin small letter AE
    small_c_cedilla             =  871, ///< U+00E7 Latin small letter C with cedilla
    small_e_grave               =  872, ///< U+00E8 Latin small letter E with grave
    small_e_acute               =  873, ///< U+00E9 Latin small letter E with acute
    small_e_circumflex          =  874, ///< U+00EA Latin small letter E with circumflex
    small_e_diaeresis           =  875, ///< U+00EB Latin small letter E with diaeresis
    small_i_grave               =  876, ///< U+00EC Latin small letter I with grave
    small_i_acute               =  877, ///< U+00ED Latin small letter I with acute
    small_i_circumflex          =  878, ///< U+00EE Latin small letter I with circumflex
    small_i_diaeresis           =  879, ///< U+00EF Latin small letter I with diaeresis
    small_eth                   =  880, ///< U+00F0 Latin small letter ETH
    small_n_tilde               =  881, ///< U+00F1 Latin small letter N with tilde
    small_o_grave               =  882, ///< U+00F2 Latin small letter O with grave
    small_o_acute               =  883, ///< U+00F3 Latin small letter O with acute
    small_o_circumflex          =  884, ///< U+00F4 Latin small letter O with circumflex
    small_o_tilde               =  885, ///< U+00F5 Latin small letter O with tilde
    small_o_diaeresis           =  886, ///< U+00F6 Latin small letter O with diaeresis
    division_sign               =  887, ///< U+00F7 Division sign
    small_o_stroke              =  888, ///< U+00F8 Latin small letter O with stroke
    small_u_grave               =  889, ///< U+00F9 Latin small letter U with grave
    small_u_acute               =  890, ///< U+00FA Latin small letter U with acute
    small_u_circumflex          =  891, ///< U+00FB Latin small letter U with circumflex
    small_u_diaeresis           =  892, ///< U+00FC Latin small letter U with diaeresis
    small_y_acute               =  893, ///< U+00FD Latin small letter Y with acute
    small_thorn                 =  894, ///< U+00FE Latin small letter THORN
    small_y_diaeresis           =  895, ///< U+00FF Latin small letter Y with diaeresis
};


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_KEY_HPP
