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

#ifndef ARCHON_X_DISPLAY_X_KEYSYMS_HPP
#define ARCHON_X_DISPLAY_X_KEYSYMS_HPP

/// \file


#include <archon/display/event.hpp>


namespace archon::display {


/// \brief Placeholder identifier for unknown key.
///
/// This keyboard key identifier is generated when a key event (\ref display::KeyEvent)
/// occurs for key that is unknown to the Archon Display Library.
///
constexpr display::KeyEvent::KeySym key_Unknown = 0;


/// \{
///
/// \brief TTY function key identifiers.
///
/// These are the identifiers for the keyboard keys that correspond to the TTY functions of
/// ASCII.
///
constexpr display::KeyEvent::KeySym key_BackSpace  = 0x08;
constexpr display::KeyEvent::KeySym key_Tab        = 0x09;
constexpr display::KeyEvent::KeySym key_LineFeed   = 0x0A;
constexpr display::KeyEvent::KeySym key_Clear      = 0x0B;
constexpr display::KeyEvent::KeySym key_Return     = 0x0D;
constexpr display::KeyEvent::KeySym key_Pause      = 0x13;
constexpr display::KeyEvent::KeySym key_ScrollLock = 0x14;
constexpr display::KeyEvent::KeySym key_SysReq     = 0x15;
constexpr display::KeyEvent::KeySym key_Escape     = 0x1B;
constexpr display::KeyEvent::KeySym key_Delete     = 0xFF;
/// \}


/// \{
///
/// \brief Basic Latin (ASCII) key identifiers.
///
/// These are the identifiers for the keyboard keys that, when no affected by modifier keys,
/// produce symbols correspond to the Basic Latin (ASCII) code block of Unicode.
///
constexpr display::KeyEvent::KeySym key_Space              = 0x20;
constexpr display::KeyEvent::KeySym key_ExclamationMark    = 0x21;
constexpr display::KeyEvent::KeySym key_QuotationMark      = 0x22;
constexpr display::KeyEvent::KeySym key_NumberSign         = 0x23;
constexpr display::KeyEvent::KeySym key_DollarSign         = 0x24;
constexpr display::KeyEvent::KeySym key_PercentSign        = 0x25;
constexpr display::KeyEvent::KeySym key_Ampersand          = 0x26;
constexpr display::KeyEvent::KeySym key_Apostrophe         = 0x27;
constexpr display::KeyEvent::KeySym key_LeftParenthesis    = 0x28;
constexpr display::KeyEvent::KeySym key_RightParenthesis   = 0x29;
constexpr display::KeyEvent::KeySym key_Asterisk           = 0x2A;
constexpr display::KeyEvent::KeySym key_Plus               = 0x2B;
constexpr display::KeyEvent::KeySym key_Comma              = 0x2C;
constexpr display::KeyEvent::KeySym key_Minus              = 0x2D;
constexpr display::KeyEvent::KeySym key_Period             = 0x2E;
constexpr display::KeyEvent::KeySym key_Slash              = 0x2F;
constexpr display::KeyEvent::KeySym key_Digit0             = 0x30;
constexpr display::KeyEvent::KeySym key_Digit1             = 0x31;
constexpr display::KeyEvent::KeySym key_Digit2             = 0x32;
constexpr display::KeyEvent::KeySym key_Digit3             = 0x33;
constexpr display::KeyEvent::KeySym key_Digit4             = 0x34;
constexpr display::KeyEvent::KeySym key_Digit5             = 0x35;
constexpr display::KeyEvent::KeySym key_Digit6             = 0x36;
constexpr display::KeyEvent::KeySym key_Digit7             = 0x37;
constexpr display::KeyEvent::KeySym key_Digit8             = 0x38;
constexpr display::KeyEvent::KeySym key_Digit9             = 0x39;
constexpr display::KeyEvent::KeySym key_Colon              = 0x3A;
constexpr display::KeyEvent::KeySym key_Semicolon          = 0x3B;
constexpr display::KeyEvent::KeySym key_LessThanSign       = 0x3C;
constexpr display::KeyEvent::KeySym key_EqualSign          = 0x3D;
constexpr display::KeyEvent::KeySym key_GreaterThanSign    = 0x3E;
constexpr display::KeyEvent::KeySym key_QuestionMark       = 0x3F;
constexpr display::KeyEvent::KeySym key_At                 = 0x40;
constexpr display::KeyEvent::KeySym key_CapitalLetterA     = 0x41;
constexpr display::KeyEvent::KeySym key_CapitalLetterB     = 0x42;
constexpr display::KeyEvent::KeySym key_CapitalLetterC     = 0x43;
constexpr display::KeyEvent::KeySym key_CapitalLetterD     = 0x44;
constexpr display::KeyEvent::KeySym key_CapitalLetterE     = 0x45;
constexpr display::KeyEvent::KeySym key_CapitalLetterF     = 0x46;
constexpr display::KeyEvent::KeySym key_CapitalLetterG     = 0x47;
constexpr display::KeyEvent::KeySym key_CapitalLetterH     = 0x48;
constexpr display::KeyEvent::KeySym key_CapitalLetterI     = 0x49;
constexpr display::KeyEvent::KeySym key_CapitalLetterJ     = 0x4A;
constexpr display::KeyEvent::KeySym key_CapitalLetterK     = 0x4B;
constexpr display::KeyEvent::KeySym key_CapitalLetterL     = 0x4C;
constexpr display::KeyEvent::KeySym key_CapitalLetterM     = 0x4D;
constexpr display::KeyEvent::KeySym key_CapitalLetterN     = 0x4E;
constexpr display::KeyEvent::KeySym key_CapitalLetterO     = 0x4F;
constexpr display::KeyEvent::KeySym key_CapitalLetterP     = 0x50;
constexpr display::KeyEvent::KeySym key_CapitalLetterQ     = 0x51;
constexpr display::KeyEvent::KeySym key_CapitalLetterR     = 0x52;
constexpr display::KeyEvent::KeySym key_CapitalLetterS     = 0x53;
constexpr display::KeyEvent::KeySym key_CapitalLetterT     = 0x54;
constexpr display::KeyEvent::KeySym key_CapitalLetterU     = 0x55;
constexpr display::KeyEvent::KeySym key_CapitalLetterV     = 0x56;
constexpr display::KeyEvent::KeySym key_CapitalLetterW     = 0x57;
constexpr display::KeyEvent::KeySym key_CapitalLetterX     = 0x58;
constexpr display::KeyEvent::KeySym key_CapitalLetterY     = 0x59;
constexpr display::KeyEvent::KeySym key_CapitalLetterZ     = 0x5A;
constexpr display::KeyEvent::KeySym key_LeftSquareBracket  = 0x5B;
constexpr display::KeyEvent::KeySym key_Backslash          = 0x5C;
constexpr display::KeyEvent::KeySym key_RightSquareBracket = 0x5D;
constexpr display::KeyEvent::KeySym key_CircumflexAccent   = 0x5E;
constexpr display::KeyEvent::KeySym key_Underscore         = 0x5F;
constexpr display::KeyEvent::KeySym key_GraveAccent        = 0x60;
constexpr display::KeyEvent::KeySym key_SmallLetterA       = 0x61;
constexpr display::KeyEvent::KeySym key_SmallLetterB       = 0x62;
constexpr display::KeyEvent::KeySym key_SmallLetterC       = 0x63;
constexpr display::KeyEvent::KeySym key_SmallLetterD       = 0x64;
constexpr display::KeyEvent::KeySym key_SmallLetterE       = 0x65;
constexpr display::KeyEvent::KeySym key_SmallLetterF       = 0x66;
constexpr display::KeyEvent::KeySym key_SmallLetterG       = 0x67;
constexpr display::KeyEvent::KeySym key_SmallLetterH       = 0x68;
constexpr display::KeyEvent::KeySym key_SmallLetterI       = 0x69;
constexpr display::KeyEvent::KeySym key_SmallLetterJ       = 0x6A;
constexpr display::KeyEvent::KeySym key_SmallLetterK       = 0x6B;
constexpr display::KeyEvent::KeySym key_SmallLetterL       = 0x6C;
constexpr display::KeyEvent::KeySym key_SmallLetterM       = 0x6D;
constexpr display::KeyEvent::KeySym key_SmallLetterN       = 0x6E;
constexpr display::KeyEvent::KeySym key_SmallLetterO       = 0x6F;
constexpr display::KeyEvent::KeySym key_SmallLetterP       = 0x70;
constexpr display::KeyEvent::KeySym key_SmallLetterQ       = 0x71;
constexpr display::KeyEvent::KeySym key_SmallLetterR       = 0x72;
constexpr display::KeyEvent::KeySym key_SmallLetterS       = 0x73;
constexpr display::KeyEvent::KeySym key_SmallLetterT       = 0x74;
constexpr display::KeyEvent::KeySym key_SmallLetterU       = 0x75;
constexpr display::KeyEvent::KeySym key_SmallLetterV       = 0x76;
constexpr display::KeyEvent::KeySym key_SmallLetterW       = 0x77;
constexpr display::KeyEvent::KeySym key_SmallLetterX       = 0x78;
constexpr display::KeyEvent::KeySym key_SmallLetterY       = 0x79;
constexpr display::KeyEvent::KeySym key_SmallLetterZ       = 0x7A;
constexpr display::KeyEvent::KeySym key_LeftCurlyBracket   = 0x7B;
constexpr display::KeyEvent::KeySym key_VerticalBar        = 0x7C;
constexpr display::KeyEvent::KeySym key_RightCurlyBracket  = 0x7D;
constexpr display::KeyEvent::KeySym key_Tilde              = 0x7E;
/// \}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_KEYSYMS_HPP
