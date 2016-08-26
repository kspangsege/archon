/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_DISPLAY_KEYSYMS_HPP
#define ARCHON_DISPLAY_KEYSYMS_HPP

namespace archon {
namespace display {

/// A KeySym identifies a symbol (or pictogram) on a key (on the keyboard).
///
/// Although the KeySym values are chosen to match those defined by
/// Xlib, you must not rely on any such correspondance.
using KeySym = int;

constexpr KeySym KeySym_None = 0;

/// @{ TTY Functions, chosen to match ASCII.
constexpr KeySym KeySym_BackSpace   = 0xFF08; // back space, back char
constexpr KeySym KeySym_Tab         = 0xFF09;
constexpr KeySym KeySym_Linefeed    = 0xFF0A; // Linefeed, LF
constexpr KeySym KeySym_Clear       = 0xFF0B;
constexpr KeySym KeySym_Return      = 0xFF0D; // Return, enter
constexpr KeySym KeySym_Pause       = 0xFF13; // Pause, hold
constexpr KeySym KeySym_Scroll_Lock = 0xFF14;
constexpr KeySym KeySym_Sys_Req     = 0xFF15;
constexpr KeySym KeySym_Escape      = 0xFF1B;
constexpr KeySym KeySym_Delete      = 0xFFFF; // Delete, rubout
/// @}

/// @{ International & multi-key character composition.
constexpr KeySym KeySym_Multi_key         = 0xFF20; // Multi-key character compose
constexpr KeySym KeySym_Codeinput         = 0xFF37;
constexpr KeySym KeySym_SingleCandidate   = 0xFF3C;
constexpr KeySym KeySym_MultipleCandidate = 0xFF3D;
constexpr KeySym KeySym_PreviousCandidate = 0xFF3E;
/// @}

/// @{ Japanese keyboard support.
constexpr KeySym KeySym_Kanji             = 0xFF21; // Kanji, Kanji convert
constexpr KeySym KeySym_Muhenkan          = 0xFF22; // Cancel Conversion
constexpr KeySym KeySym_Henkan_Mode       = 0xFF23; // Start/Stop Conversion
constexpr KeySym KeySym_Henkan            = 0xFF23; // Alias for Henkan_Mode
constexpr KeySym KeySym_Romaji            = 0xFF24; // to Romaji
constexpr KeySym KeySym_Hiragana          = 0xFF25; // to Hiragana
constexpr KeySym KeySym_Katakana          = 0xFF26; // to Katakana
constexpr KeySym KeySym_Hiragana_Katakana = 0xFF27; // Hiragana/Katakana toggle
constexpr KeySym KeySym_Zenkaku           = 0xFF28; // to Zenkaku
constexpr KeySym KeySym_Hankaku           = 0xFF29; // to Hankaku
constexpr KeySym KeySym_Zenkaku_Hankaku   = 0xFF2A; // Zenkaku/Hankaku toggle
constexpr KeySym KeySym_Touroku           = 0xFF2B; // Add to Dictionary
constexpr KeySym KeySym_Massyo            = 0xFF2C; // Delete from Dictionary
constexpr KeySym KeySym_Kana_Lock         = 0xFF2D; // Kana Lock
constexpr KeySym KeySym_Kana_Shift        = 0xFF2E; // Kana Shift
constexpr KeySym KeySym_Eisu_Shift        = 0xFF2F; // Alphanumeric Shift
constexpr KeySym KeySym_Eisu_toggle       = 0xFF30; // Alphanumeric toggle
constexpr KeySym KeySym_Kanji_Bangou      = 0xFF37; // Codeinput (Alias for Codeinput)
constexpr KeySym KeySym_Zen_Koho          = 0xFF3D; // Multiple/All Candidate(s) (Alias for MultipleCandidate)
constexpr KeySym KeySym_Mae_Koho          = 0xFF3E; // Alias for PreviousCandidate
/// @}

// 0xFF31 thru 0xFF3F are under Korean

/// @{ Cursor control & motion.
constexpr KeySym KeySym_Home      = 0xFF50;
constexpr KeySym KeySym_Left      = 0xFF51; // Move left, left arrow
constexpr KeySym KeySym_Up        = 0xFF52; // Move up, up arrow
constexpr KeySym KeySym_Right     = 0xFF53; // Move right, right arrow
constexpr KeySym KeySym_Down      = 0xFF54; // Move down, down arrow
constexpr KeySym KeySym_Prior     = 0xFF55; // Prior, previous
constexpr KeySym KeySym_Page_Up   = 0xFF55; // Alias for Prior
constexpr KeySym KeySym_Next      = 0xFF56; // Next
constexpr KeySym KeySym_Page_Down = 0xFF56; // Alias for Next
constexpr KeySym KeySym_End       = 0xFF57; // EOL
constexpr KeySym KeySym_Begin     = 0xFF58; // BOL
/// @}

/// @{ Misc Functions.
constexpr KeySym KeySym_Select        = 0xFF60; // Select, mark
constexpr KeySym KeySym_Print         = 0xFF61;
constexpr KeySym KeySym_Execute       = 0xFF62; // Execute, run, do
constexpr KeySym KeySym_Insert        = 0xFF63; // Insert, insert here
constexpr KeySym KeySym_Undo          = 0xFF65; // Undo, oops
constexpr KeySym KeySym_Redo          = 0xFF66; // redo, again
constexpr KeySym KeySym_Menu          = 0xFF67;
constexpr KeySym KeySym_Find          = 0xFF68; // Find, search
constexpr KeySym KeySym_Cancel        = 0xFF69; // Cancel, stop, abort, exit
constexpr KeySym KeySym_Help          = 0xFF6A; // Help
constexpr KeySym KeySym_Break         = 0xFF6B;
constexpr KeySym KeySym_Mode_switch   = 0xFF7E; // Character set switch
constexpr KeySym KeySym_script_switch = 0xFF7E; // Alias for mode_switch
constexpr KeySym KeySym_Num_Lock      = 0xFF7F;
/// @}

/// @{ Keypad Functions, keypad numbers cleverly chosen to map to ascii.
constexpr KeySym KeySym_KP_Space     = 0xFF80; // space
constexpr KeySym KeySym_KP_Tab       = 0xFF89;
constexpr KeySym KeySym_KP_Enter     = 0xFF8D; // enter
constexpr KeySym KeySym_KP_F1        = 0xFF91; // PF1, KP_A, ...
constexpr KeySym KeySym_KP_F2        = 0xFF92;
constexpr KeySym KeySym_KP_F3        = 0xFF93;
constexpr KeySym KeySym_KP_F4        = 0xFF94;
constexpr KeySym KeySym_KP_Home      = 0xFF95;
constexpr KeySym KeySym_KP_Left      = 0xFF96;
constexpr KeySym KeySym_KP_Up        = 0xFF97;
constexpr KeySym KeySym_KP_Right     = 0xFF98;
constexpr KeySym KeySym_KP_Down      = 0xFF99;
constexpr KeySym KeySym_KP_Prior     = 0xFF9A;
constexpr KeySym KeySym_KP_Page_Up   = 0xFF9A; // Alias for KP_Prior
constexpr KeySym KeySym_KP_Next      = 0xFF9B;
constexpr KeySym KeySym_KP_Page_Down = 0xFF9B; // Alias for KP_Next
constexpr KeySym KeySym_KP_End       = 0xFF9C;
constexpr KeySym KeySym_KP_Begin     = 0xFF9D;
constexpr KeySym KeySym_KP_Insert    = 0xFF9E;
constexpr KeySym KeySym_KP_Delete    = 0xFF9F;
constexpr KeySym KeySym_KP_Equal     = 0xFFBD; // equals
constexpr KeySym KeySym_KP_Multiply  = 0xFFAA;
constexpr KeySym KeySym_KP_Add       = 0xFFAB;
constexpr KeySym KeySym_KP_Separator = 0xFFAC; // separator, often comma
constexpr KeySym KeySym_KP_Subtract  = 0xFFAD;
constexpr KeySym KeySym_KP_Decimal   = 0xFFAE;
constexpr KeySym KeySym_KP_Divide    = 0xFFAF;

constexpr KeySym KeySym_KP_0         = 0xFFB0;
constexpr KeySym KeySym_KP_1         = 0xFFB1;
constexpr KeySym KeySym_KP_2         = 0xFFB2;
constexpr KeySym KeySym_KP_3         = 0xFFB3;
constexpr KeySym KeySym_KP_4         = 0xFFB4;
constexpr KeySym KeySym_KP_5         = 0xFFB5;
constexpr KeySym KeySym_KP_6         = 0xFFB6;
constexpr KeySym KeySym_KP_7         = 0xFFB7;
constexpr KeySym KeySym_KP_8         = 0xFFB8;
constexpr KeySym KeySym_KP_9         = 0xFFB9;
/// @}

/// @{ Auxilliary Functions; note the duplicate definitions for left and right
/// function keys; Sun keyboards and a few other manufactures have such function
/// key groups on the left and/or right sides of the keyboard.  We've not found
/// a keyboard with more than 35 function keys total.
constexpr KeySym KeySym_F1  = 0xFFBE;
constexpr KeySym KeySym_F2  = 0xFFBF;
constexpr KeySym KeySym_F3  = 0xFFC0;
constexpr KeySym KeySym_F4  = 0xFFC1;
constexpr KeySym KeySym_F5  = 0xFFC2;
constexpr KeySym KeySym_F6  = 0xFFC3;
constexpr KeySym KeySym_F7  = 0xFFC4;
constexpr KeySym KeySym_F8  = 0xFFC5;
constexpr KeySym KeySym_F9  = 0xFFC6;
constexpr KeySym KeySym_F10 = 0xFFC7;
constexpr KeySym KeySym_F11 = 0xFFC8;
constexpr KeySym KeySym_L1  = 0xFFC8; // Alias for F11
constexpr KeySym KeySym_F12 = 0xFFC9;
constexpr KeySym KeySym_L2  = 0xFFC9; // Alias for F12
constexpr KeySym KeySym_F13 = 0xFFCA;
constexpr KeySym KeySym_L3  = 0xFFCA; // Alias for F13
constexpr KeySym KeySym_F14 = 0xFFCB;
constexpr KeySym KeySym_L4  = 0xFFCB; // Alias for F14
constexpr KeySym KeySym_F15 = 0xFFCC;
constexpr KeySym KeySym_L5  = 0xFFCC; // Alias for F15
constexpr KeySym KeySym_F16 = 0xFFCD;
constexpr KeySym KeySym_L6  = 0xFFCD; // Alias for F16
constexpr KeySym KeySym_F17 = 0xFFCE;
constexpr KeySym KeySym_L7  = 0xFFCE; // Alias for F17
constexpr KeySym KeySym_F18 = 0xFFCF;
constexpr KeySym KeySym_L8  = 0xFFCF; // Alias for F18
constexpr KeySym KeySym_F19 = 0xFFD0;
constexpr KeySym KeySym_L9  = 0xFFD0; // Alias for F19
constexpr KeySym KeySym_F20 = 0xFFD1;
constexpr KeySym KeySym_L10 = 0xFFD1; // Alias for F20
constexpr KeySym KeySym_F21 = 0xFFD2;
constexpr KeySym KeySym_R1  = 0xFFD2; // Alias for F21
constexpr KeySym KeySym_F22 = 0xFFD3;
constexpr KeySym KeySym_R2  = 0xFFD3; // Alias for F22
constexpr KeySym KeySym_F23 = 0xFFD4;
constexpr KeySym KeySym_R3  = 0xFFD4; // Alias for F23
constexpr KeySym KeySym_F24 = 0xFFD5;
constexpr KeySym KeySym_R4  = 0xFFD5; // Alias for F24
constexpr KeySym KeySym_F25 = 0xFFD6;
constexpr KeySym KeySym_R5  = 0xFFD6; // Alias for F25
constexpr KeySym KeySym_F26 = 0xFFD7;
constexpr KeySym KeySym_R6  = 0xFFD7; // Alias for F26
constexpr KeySym KeySym_F27 = 0xFFD8;
constexpr KeySym KeySym_R7  = 0xFFD8; // Alias for F27
constexpr KeySym KeySym_F28 = 0xFFD9;
constexpr KeySym KeySym_R8  = 0xFFD9; // Alias for F28
constexpr KeySym KeySym_F29 = 0xFFDA;
constexpr KeySym KeySym_R9  = 0xFFDA; // Alias for F29
constexpr KeySym KeySym_F30 = 0xFFDB;
constexpr KeySym KeySym_R10 = 0xFFDB; // Alias for F30
constexpr KeySym KeySym_F31 = 0xFFDC;
constexpr KeySym KeySym_R11 = 0xFFDC; // Alias for F31
constexpr KeySym KeySym_F32 = 0xFFDD;
constexpr KeySym KeySym_R12 = 0xFFDD; // Alias for F32
constexpr KeySym KeySym_F33 = 0xFFDE;
constexpr KeySym KeySym_R13 = 0xFFDE; // Alias for F33
constexpr KeySym KeySym_F34 = 0xFFDF;
constexpr KeySym KeySym_R14 = 0xFFDF; // Alias for F34
constexpr KeySym KeySym_F35 = 0xFFE0;
constexpr KeySym KeySym_R15 = 0xFFE0; // Alias for F35
/// @}

/// @{ Modifiers.
constexpr KeySym KeySym_Shift_L    = 0xFFE1; // Left shift
constexpr KeySym KeySym_Shift_R    = 0xFFE2; // Right shift
constexpr KeySym KeySym_Control_L  = 0xFFE3; // Left control
constexpr KeySym KeySym_Control_R  = 0xFFE4; // Right control
constexpr KeySym KeySym_Caps_Lock  = 0xFFE5; // Caps lock
constexpr KeySym KeySym_Shift_Lock = 0xFFE6; // Shift lock

constexpr KeySym KeySym_Meta_L     = 0xFFE7; // Left meta
constexpr KeySym KeySym_Meta_R     = 0xFFE8; // Right meta
constexpr KeySym KeySym_Alt_L      = 0xFFE9; // Left alt
constexpr KeySym KeySym_Alt_R      = 0xFFEA; // Right alt
constexpr KeySym KeySym_Super_L    = 0xFFEB; // Left super
constexpr KeySym KeySym_Super_R    = 0xFFEC; // Right super
constexpr KeySym KeySym_Hyper_L    = 0xFFED; // Left hyper
constexpr KeySym KeySym_Hyper_R    = 0xFFEE; // Right hyper
/// @}

/// @{ ISO 9995 Function and Modifier Keys.
///
/// Byte 3 = 0xFE
constexpr KeySym KeySym_ISO_Lock             = 0xFE01;
constexpr KeySym KeySym_ISO_Level2_Latch     = 0xFE02;
constexpr KeySym KeySym_ISO_Level3_Shift     = 0xFE03;
constexpr KeySym KeySym_ISO_Level3_Latch     = 0xFE04;
constexpr KeySym KeySym_ISO_Level3_Lock      = 0xFE05;
constexpr KeySym KeySym_ISO_Group_Shift      = 0xFF7E; // Alias for mode_switch
constexpr KeySym KeySym_ISO_Group_Latch      = 0xFE06;
constexpr KeySym KeySym_ISO_Group_Lock       = 0xFE07;
constexpr KeySym KeySym_ISO_Next_Group       = 0xFE08;
constexpr KeySym KeySym_ISO_Next_Group_Lock  = 0xFE09;
constexpr KeySym KeySym_ISO_Prev_Group       = 0xFE0A;
constexpr KeySym KeySym_ISO_Prev_Group_Lock  = 0xFE0B;
constexpr KeySym KeySym_ISO_First_Group      = 0xFE0C;
constexpr KeySym KeySym_ISO_First_Group_Lock = 0xFE0D;
constexpr KeySym KeySym_ISO_Last_Group       = 0xFE0E;
constexpr KeySym KeySym_ISO_Last_Group_Lock  = 0xFE0F;

constexpr KeySym KeySym_ISO_Left_Tab                = 0xFE20;
constexpr KeySym KeySym_ISO_Move_Line_Up            = 0xFE21;
constexpr KeySym KeySym_ISO_Move_Line_Down          = 0xFE22;
constexpr KeySym KeySym_ISO_Partial_Line_Up         = 0xFE23;
constexpr KeySym KeySym_ISO_Partial_Line_Down       = 0xFE24;
constexpr KeySym KeySym_ISO_Partial_Space_Left      = 0xFE25;
constexpr KeySym KeySym_ISO_Partial_Space_Right     = 0xFE26;
constexpr KeySym KeySym_ISO_Set_Margin_Left         = 0xFE27;
constexpr KeySym KeySym_ISO_Set_Margin_Right        = 0xFE28;
constexpr KeySym KeySym_ISO_Release_Margin_Left     = 0xFE29;
constexpr KeySym KeySym_ISO_Release_Margin_Right    = 0xFE2A;
constexpr KeySym KeySym_ISO_Release_Both_Margins    = 0xFE2B;
constexpr KeySym KeySym_ISO_Fast_Cursor_Left        = 0xFE2C;
constexpr KeySym KeySym_ISO_Fast_Cursor_Right       = 0xFE2D;
constexpr KeySym KeySym_ISO_Fast_Cursor_Up          = 0xFE2E;
constexpr KeySym KeySym_ISO_Fast_Cursor_Down        = 0xFE2F;
constexpr KeySym KeySym_ISO_Continuous_Underline    = 0xFE30;
constexpr KeySym KeySym_ISO_Discontinuous_Underline = 0xFE31;
constexpr KeySym KeySym_ISO_Emphasize               = 0xFE32;
constexpr KeySym KeySym_ISO_Center_Object           = 0xFE33;
constexpr KeySym KeySym_ISO_Enter                   = 0xFE34;

constexpr KeySym KeySym_dead_grave            = 0xFE50;
constexpr KeySym KeySym_dead_acute            = 0xFE51;
constexpr KeySym KeySym_dead_circumflex       = 0xFE52;
constexpr KeySym KeySym_dead_tilde            = 0xFE53;
constexpr KeySym KeySym_dead_macron           = 0xFE54;
constexpr KeySym KeySym_dead_breve            = 0xFE55;
constexpr KeySym KeySym_dead_abovedot         = 0xFE56;
constexpr KeySym KeySym_dead_diaeresis        = 0xFE57;
constexpr KeySym KeySym_dead_abovering        = 0xFE58;
constexpr KeySym KeySym_dead_doubleacute      = 0xFE59;
constexpr KeySym KeySym_dead_caron            = 0xFE5A;
constexpr KeySym KeySym_dead_cedilla          = 0xFE5B;
constexpr KeySym KeySym_dead_ogonek           = 0xFE5C;
constexpr KeySym KeySym_dead_iota             = 0xFE5D;
constexpr KeySym KeySym_dead_voiced_sound     = 0xFE5E;
constexpr KeySym KeySym_dead_semivoiced_sound = 0xFE5F;
constexpr KeySym KeySym_dead_belowdot         = 0xFE60;
constexpr KeySym KeySym_dead_hook             = 0xFE61;
constexpr KeySym KeySym_dead_horn             = 0xFE62;

constexpr KeySym KeySym_First_Virtual_Screen  = 0xFED0;
constexpr KeySym KeySym_Prev_Virtual_Screen   = 0xFED1;
constexpr KeySym KeySym_Next_Virtual_Screen   = 0xFED2;
constexpr KeySym KeySym_Last_Virtual_Screen   = 0xFED4;
constexpr KeySym KeySym_Terminate_Server      = 0xFED5;

constexpr KeySym KeySym_AccessX_Enable          = 0xFE70;
constexpr KeySym KeySym_AccessX_Feedback_Enable = 0xFE71;
constexpr KeySym KeySym_RepeatKeys_Enable       = 0xFE72;
constexpr KeySym KeySym_SlowKeys_Enable         = 0xFE73;
constexpr KeySym KeySym_BounceKeys_Enable       = 0xFE74;
constexpr KeySym KeySym_StickyKeys_Enable       = 0xFE75;
constexpr KeySym KeySym_MouseKeys_Enable        = 0xFE76;
constexpr KeySym KeySym_MouseKeys_Accel_Enable  = 0xFE77;
constexpr KeySym KeySym_Overlay1_Enable         = 0xFE78;
constexpr KeySym KeySym_Overlay2_Enable         = 0xFE79;
constexpr KeySym KeySym_AudibleBell_Enable      = 0xFE7A;

constexpr KeySym KeySym_Pointer_Left          = 0xFEE0;
constexpr KeySym KeySym_Pointer_Right         = 0xFEE1;
constexpr KeySym KeySym_Pointer_Up            = 0xFEE2;
constexpr KeySym KeySym_Pointer_Down          = 0xFEE3;
constexpr KeySym KeySym_Pointer_UpLeft        = 0xFEE4;
constexpr KeySym KeySym_Pointer_UpRight       = 0xFEE5;
constexpr KeySym KeySym_Pointer_DownLeft      = 0xFEE6;
constexpr KeySym KeySym_Pointer_DownRight     = 0xFEE7;
constexpr KeySym KeySym_Pointer_Button_Dflt   = 0xFEE8;
constexpr KeySym KeySym_Pointer_Button1       = 0xFEE9;
constexpr KeySym KeySym_Pointer_Button2       = 0xFEEA;
constexpr KeySym KeySym_Pointer_Button3       = 0xFEEB;
constexpr KeySym KeySym_Pointer_Button4       = 0xFEEC;
constexpr KeySym KeySym_Pointer_Button5       = 0xFEED;
constexpr KeySym KeySym_Pointer_DblClick_Dflt = 0xFEEE;
constexpr KeySym KeySym_Pointer_DblClick1     = 0xFEEF;
constexpr KeySym KeySym_Pointer_DblClick2     = 0xFEF0;
constexpr KeySym KeySym_Pointer_DblClick3     = 0xFEF1;
constexpr KeySym KeySym_Pointer_DblClick4     = 0xFEF2;
constexpr KeySym KeySym_Pointer_DblClick5     = 0xFEF3;
constexpr KeySym KeySym_Pointer_Drag_Dflt     = 0xFEF4;
constexpr KeySym KeySym_Pointer_Drag1         = 0xFEF5;
constexpr KeySym KeySym_Pointer_Drag2         = 0xFEF6;
constexpr KeySym KeySym_Pointer_Drag3         = 0xFEF7;
constexpr KeySym KeySym_Pointer_Drag4         = 0xFEF8;
constexpr KeySym KeySym_Pointer_Drag5         = 0xFEFD;

constexpr KeySym KeySym_Pointer_EnableKeys    = 0xFEF9;
constexpr KeySym KeySym_Pointer_Accelerate    = 0xFEFA;
constexpr KeySym KeySym_Pointer_DfltBtnNext   = 0xFEFB;
constexpr KeySym KeySym_Pointer_DfltBtnPrev   = 0xFEFC;
/// @}

/// @{ 3270 Terminal Keys.
///
/// Byte 3 = 0xFD
constexpr KeySym KeySym_3270_Duplicate    = 0xFD01;
constexpr KeySym KeySym_3270_FieldMark    = 0xFD02;
constexpr KeySym KeySym_3270_Right2       = 0xFD03;
constexpr KeySym KeySym_3270_Left2        = 0xFD04;
constexpr KeySym KeySym_3270_BackTab      = 0xFD05;
constexpr KeySym KeySym_3270_EraseEOF     = 0xFD06;
constexpr KeySym KeySym_3270_EraseInput   = 0xFD07;
constexpr KeySym KeySym_3270_Reset        = 0xFD08;
constexpr KeySym KeySym_3270_Quit         = 0xFD09;
constexpr KeySym KeySym_3270_PA1          = 0xFD0A;
constexpr KeySym KeySym_3270_PA2          = 0xFD0B;
constexpr KeySym KeySym_3270_PA3          = 0xFD0C;
constexpr KeySym KeySym_3270_Test         = 0xFD0D;
constexpr KeySym KeySym_3270_Attn         = 0xFD0E;
constexpr KeySym KeySym_3270_CursorBlink  = 0xFD0F;
constexpr KeySym KeySym_3270_AltCursor    = 0xFD10;
constexpr KeySym KeySym_3270_KeyClick     = 0xFD11;
constexpr KeySym KeySym_3270_Jump         = 0xFD12;
constexpr KeySym KeySym_3270_Ident        = 0xFD13;
constexpr KeySym KeySym_3270_Rule         = 0xFD14;
constexpr KeySym KeySym_3270_Copy         = 0xFD15;
constexpr KeySym KeySym_3270_Play         = 0xFD16;
constexpr KeySym KeySym_3270_Setup        = 0xFD17;
constexpr KeySym KeySym_3270_Record       = 0xFD18;
constexpr KeySym KeySym_3270_ChangeScreen = 0xFD19;
constexpr KeySym KeySym_3270_DeleteWord   = 0xFD1A;
constexpr KeySym KeySym_3270_ExSelect     = 0xFD1B;
constexpr KeySym KeySym_3270_CursorSelect = 0xFD1C;
constexpr KeySym KeySym_3270_PrintScreen  = 0xFD1D;
constexpr KeySym KeySym_3270_Enter        = 0xFD1E;
/// @}

/// @{ Latin 1.
///
/// Byte 3 = 0
constexpr KeySym KeySym_space               = 0x020;
constexpr KeySym KeySym_exclam              = 0x021;
constexpr KeySym KeySym_quotedbl            = 0x022;
constexpr KeySym KeySym_numbersign          = 0x023;
constexpr KeySym KeySym_dollar              = 0x024;
constexpr KeySym KeySym_percent             = 0x025;
constexpr KeySym KeySym_ampersand           = 0x026;
constexpr KeySym KeySym_apostrophe          = 0x027;
constexpr KeySym KeySym_parenleft           = 0x028;
constexpr KeySym KeySym_parenright          = 0x029;
constexpr KeySym KeySym_asterisk            = 0x02a;
constexpr KeySym KeySym_plus                = 0x02b;
constexpr KeySym KeySym_comma               = 0x02c;
constexpr KeySym KeySym_minus               = 0x02d;
constexpr KeySym KeySym_period              = 0x02e;
constexpr KeySym KeySym_slash               = 0x02f;
constexpr KeySym KeySym_0                   = 0x030;
constexpr KeySym KeySym_1                   = 0x031;
constexpr KeySym KeySym_2                   = 0x032;
constexpr KeySym KeySym_3                   = 0x033;
constexpr KeySym KeySym_4                   = 0x034;
constexpr KeySym KeySym_5                   = 0x035;
constexpr KeySym KeySym_6                   = 0x036;
constexpr KeySym KeySym_7                   = 0x037;
constexpr KeySym KeySym_8                   = 0x038;
constexpr KeySym KeySym_9                   = 0x039;
constexpr KeySym KeySym_colon               = 0x03a;
constexpr KeySym KeySym_semicolon           = 0x03b;
constexpr KeySym KeySym_less                = 0x03c;
constexpr KeySym KeySym_equal               = 0x03d;
constexpr KeySym KeySym_greater             = 0x03e;
constexpr KeySym KeySym_question            = 0x03f;
constexpr KeySym KeySym_at                  = 0x040;
constexpr KeySym KeySym_A                   = 0x041;
constexpr KeySym KeySym_B                   = 0x042;
constexpr KeySym KeySym_C                   = 0x043;
constexpr KeySym KeySym_D                   = 0x044;
constexpr KeySym KeySym_E                   = 0x045;
constexpr KeySym KeySym_F                   = 0x046;
constexpr KeySym KeySym_G                   = 0x047;
constexpr KeySym KeySym_H                   = 0x048;
constexpr KeySym KeySym_I                   = 0x049;
constexpr KeySym KeySym_J                   = 0x04a;
constexpr KeySym KeySym_K                   = 0x04b;
constexpr KeySym KeySym_L                   = 0x04c;
constexpr KeySym KeySym_M                   = 0x04d;
constexpr KeySym KeySym_N                   = 0x04e;
constexpr KeySym KeySym_O                   = 0x04f;
constexpr KeySym KeySym_P                   = 0x050;
constexpr KeySym KeySym_Q                   = 0x051;
constexpr KeySym KeySym_R                   = 0x052;
constexpr KeySym KeySym_S                   = 0x053;
constexpr KeySym KeySym_T                   = 0x054;
constexpr KeySym KeySym_U                   = 0x055;
constexpr KeySym KeySym_V                   = 0x056;
constexpr KeySym KeySym_W                   = 0x057;
constexpr KeySym KeySym_X                   = 0x058;
constexpr KeySym KeySym_Y                   = 0x059;
constexpr KeySym KeySym_Z                   = 0x05a;
constexpr KeySym KeySym_bracketleft         = 0x05b;
constexpr KeySym KeySym_backslash           = 0x05c;
constexpr KeySym KeySym_bracketright        = 0x05d;
constexpr KeySym KeySym_asciicircum         = 0x05e;
constexpr KeySym KeySym_underscore          = 0x05f;
constexpr KeySym KeySym_grave               = 0x060;
constexpr KeySym KeySym_a                   = 0x061;
constexpr KeySym KeySym_b                   = 0x062;
constexpr KeySym KeySym_c                   = 0x063;
constexpr KeySym KeySym_d                   = 0x064;
constexpr KeySym KeySym_e                   = 0x065;
constexpr KeySym KeySym_f                   = 0x066;
constexpr KeySym KeySym_g                   = 0x067;
constexpr KeySym KeySym_h                   = 0x068;
constexpr KeySym KeySym_i                   = 0x069;
constexpr KeySym KeySym_j                   = 0x06a;
constexpr KeySym KeySym_k                   = 0x06b;
constexpr KeySym KeySym_l                   = 0x06c;
constexpr KeySym KeySym_m                   = 0x06d;
constexpr KeySym KeySym_n                   = 0x06e;
constexpr KeySym KeySym_o                   = 0x06f;
constexpr KeySym KeySym_p                   = 0x070;
constexpr KeySym KeySym_q                   = 0x071;
constexpr KeySym KeySym_r                   = 0x072;
constexpr KeySym KeySym_s                   = 0x073;
constexpr KeySym KeySym_t                   = 0x074;
constexpr KeySym KeySym_u                   = 0x075;
constexpr KeySym KeySym_v                   = 0x076;
constexpr KeySym KeySym_w                   = 0x077;
constexpr KeySym KeySym_x                   = 0x078;
constexpr KeySym KeySym_y                   = 0x079;
constexpr KeySym KeySym_z                   = 0x07a;
constexpr KeySym KeySym_braceleft           = 0x07b;
constexpr KeySym KeySym_bar                 = 0x07c;
constexpr KeySym KeySym_braceright          = 0x07d;
constexpr KeySym KeySym_asciitilde          = 0x07e;

constexpr KeySym KeySym_nobreakspace        = 0x0a0;
constexpr KeySym KeySym_exclamdown          = 0x0a1;
constexpr KeySym KeySym_cent                = 0x0a2;
constexpr KeySym KeySym_sterling            = 0x0a3;
constexpr KeySym KeySym_currency            = 0x0a4;
constexpr KeySym KeySym_yen                 = 0x0a5;
constexpr KeySym KeySym_brokenbar           = 0x0a6;
constexpr KeySym KeySym_section             = 0x0a7;
constexpr KeySym KeySym_diaeresis           = 0x0a8;
constexpr KeySym KeySym_copyright           = 0x0a9;
constexpr KeySym KeySym_ordfeminine         = 0x0aa;
constexpr KeySym KeySym_guillemotleft       = 0x0ab; // left angle quotation mark
constexpr KeySym KeySym_notsign             = 0x0ac;
constexpr KeySym KeySym_hyphen              = 0x0ad;
constexpr KeySym KeySym_registered          = 0x0ae;
constexpr KeySym KeySym_macron              = 0x0af;
constexpr KeySym KeySym_degree              = 0x0b0;
constexpr KeySym KeySym_plusminus           = 0x0b1;
constexpr KeySym KeySym_twosuperior         = 0x0b2;
constexpr KeySym KeySym_threesuperior       = 0x0b3;
constexpr KeySym KeySym_acute               = 0x0b4;
constexpr KeySym KeySym_mu                  = 0x0b5;
constexpr KeySym KeySym_paragraph           = 0x0b6;
constexpr KeySym KeySym_periodcentered      = 0x0b7;
constexpr KeySym KeySym_cedilla             = 0x0b8;
constexpr KeySym KeySym_onesuperior         = 0x0b9;
constexpr KeySym KeySym_masculine           = 0x0ba;
constexpr KeySym KeySym_guillemotright      = 0x0bb; // right angle quotation mark
constexpr KeySym KeySym_onequarter          = 0x0bc;
constexpr KeySym KeySym_onehalf             = 0x0bd;
constexpr KeySym KeySym_threequarters       = 0x0be;
constexpr KeySym KeySym_questiondown        = 0x0bf;
constexpr KeySym KeySym_Agrave              = 0x0c0;
constexpr KeySym KeySym_Aacute              = 0x0c1;
constexpr KeySym KeySym_Acircumflex         = 0x0c2;
constexpr KeySym KeySym_Atilde              = 0x0c3;
constexpr KeySym KeySym_Adiaeresis          = 0x0c4;
constexpr KeySym KeySym_Aring               = 0x0c5;
constexpr KeySym KeySym_AE                  = 0x0c6;
constexpr KeySym KeySym_Ccedilla            = 0x0c7;
constexpr KeySym KeySym_Egrave              = 0x0c8;
constexpr KeySym KeySym_Eacute              = 0x0c9;
constexpr KeySym KeySym_Ecircumflex         = 0x0ca;
constexpr KeySym KeySym_Ediaeresis          = 0x0cb;
constexpr KeySym KeySym_Igrave              = 0x0cc;
constexpr KeySym KeySym_Iacute              = 0x0cd;
constexpr KeySym KeySym_Icircumflex         = 0x0ce;
constexpr KeySym KeySym_Idiaeresis          = 0x0cf;
constexpr KeySym KeySym_ETH                 = 0x0d0;
constexpr KeySym KeySym_Ntilde              = 0x0d1;
constexpr KeySym KeySym_Ograve              = 0x0d2;
constexpr KeySym KeySym_Oacute              = 0x0d3;
constexpr KeySym KeySym_Ocircumflex         = 0x0d4;
constexpr KeySym KeySym_Otilde              = 0x0d5;
constexpr KeySym KeySym_Odiaeresis          = 0x0d6;
constexpr KeySym KeySym_multiply            = 0x0d7;
constexpr KeySym KeySym_Ooblique            = 0x0d8;
constexpr KeySym KeySym_Oslash              = KeySym_Ooblique; // symbol alias
constexpr KeySym KeySym_Ugrave              = 0x0d9;
constexpr KeySym KeySym_Uacute              = 0x0da;
constexpr KeySym KeySym_Ucircumflex         = 0x0db;
constexpr KeySym KeySym_Udiaeresis          = 0x0dc;
constexpr KeySym KeySym_Yacute              = 0x0dd;
constexpr KeySym KeySym_THORN               = 0x0de;
constexpr KeySym KeySym_ssharp              = 0x0df;
constexpr KeySym KeySym_agrave              = 0x0e0;
constexpr KeySym KeySym_aacute              = 0x0e1;
constexpr KeySym KeySym_acircumflex         = 0x0e2;
constexpr KeySym KeySym_atilde              = 0x0e3;
constexpr KeySym KeySym_adiaeresis          = 0x0e4;
constexpr KeySym KeySym_aring               = 0x0e5;
constexpr KeySym KeySym_ae                  = 0x0e6;
constexpr KeySym KeySym_ccedilla            = 0x0e7;
constexpr KeySym KeySym_egrave              = 0x0e8;
constexpr KeySym KeySym_eacute              = 0x0e9;
constexpr KeySym KeySym_ecircumflex         = 0x0ea;
constexpr KeySym KeySym_ediaeresis          = 0x0eb;
constexpr KeySym KeySym_igrave              = 0x0ec;
constexpr KeySym KeySym_iacute              = 0x0ed;
constexpr KeySym KeySym_icircumflex         = 0x0ee;
constexpr KeySym KeySym_idiaeresis          = 0x0ef;
constexpr KeySym KeySym_eth                 = 0x0f0;
constexpr KeySym KeySym_ntilde              = 0x0f1;
constexpr KeySym KeySym_ograve              = 0x0f2;
constexpr KeySym KeySym_oacute              = 0x0f3;
constexpr KeySym KeySym_ocircumflex         = 0x0f4;
constexpr KeySym KeySym_otilde              = 0x0f5;
constexpr KeySym KeySym_odiaeresis          = 0x0f6;
constexpr KeySym KeySym_division            = 0x0f7;
constexpr KeySym KeySym_ooblique            = 0x0f8;
constexpr KeySym KeySym_oslash              = KeySym_ooblique; // symbol alias
constexpr KeySym KeySym_ugrave              = 0x0f9;
constexpr KeySym KeySym_uacute              = 0x0fa;
constexpr KeySym KeySym_ucircumflex         = 0x0fb;
constexpr KeySym KeySym_udiaeresis          = 0x0fc;
constexpr KeySym KeySym_yacute              = 0x0fd;
constexpr KeySym KeySym_thorn               = 0x0fe;
constexpr KeySym KeySym_ydiaeresis          = 0x0ff;
/// @}

/// @{ Latin 2.
///
/// Byte 3 = 1
constexpr KeySym KeySym_Aogonek             = 0x1a1;
constexpr KeySym KeySym_breve               = 0x1a2;
constexpr KeySym KeySym_Lstroke             = 0x1a3;
constexpr KeySym KeySym_Lcaron              = 0x1a5;
constexpr KeySym KeySym_Sacute              = 0x1a6;
constexpr KeySym KeySym_Scaron              = 0x1a9;
constexpr KeySym KeySym_Scedilla            = 0x1aa;
constexpr KeySym KeySym_Tcaron              = 0x1ab;
constexpr KeySym KeySym_Zacute              = 0x1ac;
constexpr KeySym KeySym_Zcaron              = 0x1ae;
constexpr KeySym KeySym_Zabovedot           = 0x1af;
constexpr KeySym KeySym_aogonek             = 0x1b1;
constexpr KeySym KeySym_ogonek              = 0x1b2;
constexpr KeySym KeySym_lstroke             = 0x1b3;
constexpr KeySym KeySym_lcaron              = 0x1b5;
constexpr KeySym KeySym_sacute              = 0x1b6;
constexpr KeySym KeySym_caron               = 0x1b7;
constexpr KeySym KeySym_scaron              = 0x1b9;
constexpr KeySym KeySym_scedilla            = 0x1ba;
constexpr KeySym KeySym_tcaron              = 0x1bb;
constexpr KeySym KeySym_zacute              = 0x1bc;
constexpr KeySym KeySym_doubleacute         = 0x1bd;
constexpr KeySym KeySym_zcaron              = 0x1be;
constexpr KeySym KeySym_zabovedot           = 0x1bf;
constexpr KeySym KeySym_Racute              = 0x1c0;
constexpr KeySym KeySym_Abreve              = 0x1c3;
constexpr KeySym KeySym_Lacute              = 0x1c5;
constexpr KeySym KeySym_Cacute              = 0x1c6;
constexpr KeySym KeySym_Ccaron              = 0x1c8;
constexpr KeySym KeySym_Eogonek             = 0x1ca;
constexpr KeySym KeySym_Ecaron              = 0x1cc;
constexpr KeySym KeySym_Dcaron              = 0x1cf;
constexpr KeySym KeySym_Dstroke             = 0x1d0;
constexpr KeySym KeySym_Nacute              = 0x1d1;
constexpr KeySym KeySym_Ncaron              = 0x1d2;
constexpr KeySym KeySym_Odoubleacute        = 0x1d5;
constexpr KeySym KeySym_Rcaron              = 0x1d8;
constexpr KeySym KeySym_Uring               = 0x1d9;
constexpr KeySym KeySym_Udoubleacute        = 0x1db;
constexpr KeySym KeySym_Tcedilla            = 0x1de;
constexpr KeySym KeySym_racute              = 0x1e0;
constexpr KeySym KeySym_abreve              = 0x1e3;
constexpr KeySym KeySym_lacute              = 0x1e5;
constexpr KeySym KeySym_cacute              = 0x1e6;
constexpr KeySym KeySym_ccaron              = 0x1e8;
constexpr KeySym KeySym_eogonek             = 0x1ea;
constexpr KeySym KeySym_ecaron              = 0x1ec;
constexpr KeySym KeySym_dcaron              = 0x1ef;
constexpr KeySym KeySym_dstroke             = 0x1f0;
constexpr KeySym KeySym_nacute              = 0x1f1;
constexpr KeySym KeySym_ncaron              = 0x1f2;
constexpr KeySym KeySym_odoubleacute        = 0x1f5;
constexpr KeySym KeySym_udoubleacute        = 0x1fb;
constexpr KeySym KeySym_rcaron              = 0x1f8;
constexpr KeySym KeySym_uring               = 0x1f9;
constexpr KeySym KeySym_tcedilla            = 0x1fe;
constexpr KeySym KeySym_abovedot            = 0x1ff;
/// @}

/// @{ Latin 3.
///
/// Byte 3 = 2
constexpr KeySym KeySym_Hstroke             = 0x2a1;
constexpr KeySym KeySym_Hcircumflex         = 0x2a6;
constexpr KeySym KeySym_Iabovedot           = 0x2a9;
constexpr KeySym KeySym_Gbreve              = 0x2ab;
constexpr KeySym KeySym_Jcircumflex         = 0x2ac;
constexpr KeySym KeySym_hstroke             = 0x2b1;
constexpr KeySym KeySym_hcircumflex         = 0x2b6;
constexpr KeySym KeySym_idotless            = 0x2b9;
constexpr KeySym KeySym_gbreve              = 0x2bb;
constexpr KeySym KeySym_jcircumflex         = 0x2bc;
constexpr KeySym KeySym_Cabovedot           = 0x2c5;
constexpr KeySym KeySym_Ccircumflex         = 0x2c6;
constexpr KeySym KeySym_Gabovedot           = 0x2d5;
constexpr KeySym KeySym_Gcircumflex         = 0x2d8;
constexpr KeySym KeySym_Ubreve              = 0x2dd;
constexpr KeySym KeySym_Scircumflex         = 0x2de;
constexpr KeySym KeySym_cabovedot           = 0x2e5;
constexpr KeySym KeySym_ccircumflex         = 0x2e6;
constexpr KeySym KeySym_gabovedot           = 0x2f5;
constexpr KeySym KeySym_gcircumflex         = 0x2f8;
constexpr KeySym KeySym_ubreve              = 0x2fd;
constexpr KeySym KeySym_scircumflex         = 0x2fe;
/// @}

/// @{ Latin 4.
///
/// Byte 3 = 3
constexpr KeySym KeySym_kra                 = 0x3a2;
constexpr KeySym KeySym_Rcedilla            = 0x3a3;
constexpr KeySym KeySym_Itilde              = 0x3a5;
constexpr KeySym KeySym_Lcedilla            = 0x3a6;
constexpr KeySym KeySym_Emacron             = 0x3aa;
constexpr KeySym KeySym_Gcedilla            = 0x3ab;
constexpr KeySym KeySym_Tslash              = 0x3ac;
constexpr KeySym KeySym_rcedilla            = 0x3b3;
constexpr KeySym KeySym_itilde              = 0x3b5;
constexpr KeySym KeySym_lcedilla            = 0x3b6;
constexpr KeySym KeySym_emacron             = 0x3ba;
constexpr KeySym KeySym_gcedilla            = 0x3bb;
constexpr KeySym KeySym_tslash              = 0x3bc;
constexpr KeySym KeySym_ENG                 = 0x3bd;
constexpr KeySym KeySym_eng                 = 0x3bf;
constexpr KeySym KeySym_Amacron             = 0x3c0;
constexpr KeySym KeySym_Iogonek             = 0x3c7;
constexpr KeySym KeySym_Eabovedot           = 0x3cc;
constexpr KeySym KeySym_Imacron             = 0x3cf;
constexpr KeySym KeySym_Ncedilla            = 0x3d1;
constexpr KeySym KeySym_Omacron             = 0x3d2;
constexpr KeySym KeySym_Kcedilla            = 0x3d3;
constexpr KeySym KeySym_Uogonek             = 0x3d9;
constexpr KeySym KeySym_Utilde              = 0x3dd;
constexpr KeySym KeySym_Umacron             = 0x3de;
constexpr KeySym KeySym_amacron             = 0x3e0;
constexpr KeySym KeySym_iogonek             = 0x3e7;
constexpr KeySym KeySym_eabovedot           = 0x3ec;
constexpr KeySym KeySym_imacron             = 0x3ef;
constexpr KeySym KeySym_ncedilla            = 0x3f1;
constexpr KeySym KeySym_omacron             = 0x3f2;
constexpr KeySym KeySym_kcedilla            = 0x3f3;
constexpr KeySym KeySym_uogonek             = 0x3f9;
constexpr KeySym KeySym_utilde              = 0x3fd;
constexpr KeySym KeySym_umacron             = 0x3fe;
/// @}

/// @{ Latin-8.
///
/// Byte 3 = 18
constexpr KeySym KeySym_Babovedot           = 0x12a1;
constexpr KeySym KeySym_babovedot           = 0x12a2;
constexpr KeySym KeySym_Dabovedot           = 0x12a6;
constexpr KeySym KeySym_Wgrave              = 0x12a8;
constexpr KeySym KeySym_Wacute              = 0x12aa;
constexpr KeySym KeySym_dabovedot           = 0x12ab;
constexpr KeySym KeySym_Ygrave              = 0x12ac;
constexpr KeySym KeySym_Fabovedot           = 0x12b0;
constexpr KeySym KeySym_fabovedot           = 0x12b1;
constexpr KeySym KeySym_Mabovedot           = 0x12b4;
constexpr KeySym KeySym_mabovedot           = 0x12b5;
constexpr KeySym KeySym_Pabovedot           = 0x12b7;
constexpr KeySym KeySym_wgrave              = 0x12b8;
constexpr KeySym KeySym_pabovedot           = 0x12b9;
constexpr KeySym KeySym_wacute              = 0x12ba;
constexpr KeySym KeySym_Sabovedot           = 0x12bb;
constexpr KeySym KeySym_ygrave              = 0x12bc;
constexpr KeySym KeySym_Wdiaeresis          = 0x12bd;
constexpr KeySym KeySym_wdiaeresis          = 0x12be;
constexpr KeySym KeySym_sabovedot           = 0x12bf;
constexpr KeySym KeySym_Wcircumflex         = 0x12d0;
constexpr KeySym KeySym_Tabovedot           = 0x12d7;
constexpr KeySym KeySym_Ycircumflex         = 0x12de;
constexpr KeySym KeySym_wcircumflex         = 0x12f0;
constexpr KeySym KeySym_tabovedot           = 0x12f7;
constexpr KeySym KeySym_ycircumflex         = 0x12fe;
/// @}

/// @{ Latin-9 (a.k.a. Latin-0).
///
/// Byte 3 = 19
constexpr KeySym KeySym_OE                  = 0x13bc;
constexpr KeySym KeySym_oe                  = 0x13bd;
constexpr KeySym KeySym_Ydiaeresis          = 0x13be;
/// @}

/// @{ Katakana.
///
/// Byte 3 = 4
constexpr KeySym KeySym_overline                                    = 0x47e;
constexpr KeySym KeySym_kana_fullstop                               = 0x4a1;
constexpr KeySym KeySym_kana_openingbracket                         = 0x4a2;
constexpr KeySym KeySym_kana_closingbracket                         = 0x4a3;
constexpr KeySym KeySym_kana_comma                                  = 0x4a4;
constexpr KeySym KeySym_kana_conjunctive                            = 0x4a5;
constexpr KeySym KeySym_kana_WO                                     = 0x4a6;
constexpr KeySym KeySym_kana_a                                      = 0x4a7;
constexpr KeySym KeySym_kana_i                                      = 0x4a8;
constexpr KeySym KeySym_kana_u                                      = 0x4a9;
constexpr KeySym KeySym_kana_e                                      = 0x4aa;
constexpr KeySym KeySym_kana_o                                      = 0x4ab;
constexpr KeySym KeySym_kana_ya                                     = 0x4ac;
constexpr KeySym KeySym_kana_yu                                     = 0x4ad;
constexpr KeySym KeySym_kana_yo                                     = 0x4ae;
constexpr KeySym KeySym_kana_tsu                                    = 0x4af;
constexpr KeySym KeySym_prolongedsound                              = 0x4b0;
constexpr KeySym KeySym_kana_A                                      = 0x4b1;
constexpr KeySym KeySym_kana_I                                      = 0x4b2;
constexpr KeySym KeySym_kana_U                                      = 0x4b3;
constexpr KeySym KeySym_kana_E                                      = 0x4b4;
constexpr KeySym KeySym_kana_O                                      = 0x4b5;
constexpr KeySym KeySym_kana_KA                                     = 0x4b6;
constexpr KeySym KeySym_kana_KI                                     = 0x4b7;
constexpr KeySym KeySym_kana_KU                                     = 0x4b8;
constexpr KeySym KeySym_kana_KE                                     = 0x4b9;
constexpr KeySym KeySym_kana_KO                                     = 0x4ba;
constexpr KeySym KeySym_kana_SA                                     = 0x4bb;
constexpr KeySym KeySym_kana_SHI                                    = 0x4bc;
constexpr KeySym KeySym_kana_SU                                     = 0x4bd;
constexpr KeySym KeySym_kana_SE                                     = 0x4be;
constexpr KeySym KeySym_kana_SO                                     = 0x4bf;
constexpr KeySym KeySym_kana_TA                                     = 0x4c0;
constexpr KeySym KeySym_kana_CHI                                    = 0x4c1;
constexpr KeySym KeySym_kana_TSU                                    = 0x4c2;
constexpr KeySym KeySym_kana_TE                                     = 0x4c3;
constexpr KeySym KeySym_kana_TO                                     = 0x4c4;
constexpr KeySym KeySym_kana_NA                                     = 0x4c5;
constexpr KeySym KeySym_kana_NI                                     = 0x4c6;
constexpr KeySym KeySym_kana_NU                                     = 0x4c7;
constexpr KeySym KeySym_kana_NE                                     = 0x4c8;
constexpr KeySym KeySym_kana_NO                                     = 0x4c9;
constexpr KeySym KeySym_kana_HA                                     = 0x4ca;
constexpr KeySym KeySym_kana_HI                                     = 0x4cb;
constexpr KeySym KeySym_kana_FU                                     = 0x4cc;
constexpr KeySym KeySym_kana_HE                                     = 0x4cd;
constexpr KeySym KeySym_kana_HO                                     = 0x4ce;
constexpr KeySym KeySym_kana_MA                                     = 0x4cf;
constexpr KeySym KeySym_kana_MI                                     = 0x4d0;
constexpr KeySym KeySym_kana_MU                                     = 0x4d1;
constexpr KeySym KeySym_kana_ME                                     = 0x4d2;
constexpr KeySym KeySym_kana_MO                                     = 0x4d3;
constexpr KeySym KeySym_kana_YA                                     = 0x4d4;
constexpr KeySym KeySym_kana_YU                                     = 0x4d5;
constexpr KeySym KeySym_kana_YO                                     = 0x4d6;
constexpr KeySym KeySym_kana_RA                                     = 0x4d7;
constexpr KeySym KeySym_kana_RI                                     = 0x4d8;
constexpr KeySym KeySym_kana_RU                                     = 0x4d9;
constexpr KeySym KeySym_kana_RE                                     = 0x4da;
constexpr KeySym KeySym_kana_RO                                     = 0x4db;
constexpr KeySym KeySym_kana_WA                                     = 0x4dc;
constexpr KeySym KeySym_kana_N                                      = 0x4dd;
constexpr KeySym KeySym_voicedsound                                 = 0x4de;
constexpr KeySym KeySym_semivoicedsound                             = 0x4df;
constexpr KeySym KeySym_kana_switch                                 = 0xFF7E; // Alias for mode_switch
/// @}

/// \{ Arabic.
///
/// Byte 3 = 5
constexpr KeySym KeySym_Farsi_0                                     = 0x590;
constexpr KeySym KeySym_Farsi_1                                     = 0x591;
constexpr KeySym KeySym_Farsi_2                                     = 0x592;
constexpr KeySym KeySym_Farsi_3                                     = 0x593;
constexpr KeySym KeySym_Farsi_4                                     = 0x594;
constexpr KeySym KeySym_Farsi_5                                     = 0x595;
constexpr KeySym KeySym_Farsi_6                                     = 0x596;
constexpr KeySym KeySym_Farsi_7                                     = 0x597;
constexpr KeySym KeySym_Farsi_8                                     = 0x598;
constexpr KeySym KeySym_Farsi_9                                     = 0x599;
constexpr KeySym KeySym_Arabic_percent                              = 0x5a5;
constexpr KeySym KeySym_Arabic_superscript_alef                     = 0x5a6;
constexpr KeySym KeySym_Arabic_tteh                                 = 0x5a7;
constexpr KeySym KeySym_Arabic_peh                                  = 0x5a8;
constexpr KeySym KeySym_Arabic_tcheh                                = 0x5a9;
constexpr KeySym KeySym_Arabic_ddal                                 = 0x5aa;
constexpr KeySym KeySym_Arabic_rreh                                 = 0x5ab;
constexpr KeySym KeySym_Arabic_comma                                = 0x5ac;
constexpr KeySym KeySym_Arabic_fullstop                             = 0x5ae;
constexpr KeySym KeySym_Arabic_0                                    = 0x5b0;
constexpr KeySym KeySym_Arabic_1                                    = 0x5b1;
constexpr KeySym KeySym_Arabic_2                                    = 0x5b2;
constexpr KeySym KeySym_Arabic_3                                    = 0x5b3;
constexpr KeySym KeySym_Arabic_4                                    = 0x5b4;
constexpr KeySym KeySym_Arabic_5                                    = 0x5b5;
constexpr KeySym KeySym_Arabic_6                                    = 0x5b6;
constexpr KeySym KeySym_Arabic_7                                    = 0x5b7;
constexpr KeySym KeySym_Arabic_8                                    = 0x5b8;
constexpr KeySym KeySym_Arabic_9                                    = 0x5b9;
constexpr KeySym KeySym_Arabic_semicolon                            = 0x5bb;
constexpr KeySym KeySym_Arabic_question_mark                        = 0x5bf;
constexpr KeySym KeySym_Arabic_hamza                                = 0x5c1;
constexpr KeySym KeySym_Arabic_maddaonalef                          = 0x5c2;
constexpr KeySym KeySym_Arabic_hamzaonalef                          = 0x5c3;
constexpr KeySym KeySym_Arabic_hamzaonwaw                           = 0x5c4;
constexpr KeySym KeySym_Arabic_hamzaunderalef                       = 0x5c5;
constexpr KeySym KeySym_Arabic_hamzaonyeh                           = 0x5c6;
constexpr KeySym KeySym_Arabic_alef                                 = 0x5c7;
constexpr KeySym KeySym_Arabic_beh                                  = 0x5c8;
constexpr KeySym KeySym_Arabic_tehmarbuta                           = 0x5c9;
constexpr KeySym KeySym_Arabic_teh                                  = 0x5ca;
constexpr KeySym KeySym_Arabic_theh                                 = 0x5cb;
constexpr KeySym KeySym_Arabic_jeem                                 = 0x5cc;
constexpr KeySym KeySym_Arabic_hah                                  = 0x5cd;
constexpr KeySym KeySym_Arabic_khah                                 = 0x5ce;
constexpr KeySym KeySym_Arabic_dal                                  = 0x5cf;
constexpr KeySym KeySym_Arabic_thal                                 = 0x5d0;
constexpr KeySym KeySym_Arabic_ra                                   = 0x5d1;
constexpr KeySym KeySym_Arabic_zain                                 = 0x5d2;
constexpr KeySym KeySym_Arabic_seen                                 = 0x5d3;
constexpr KeySym KeySym_Arabic_sheen                                = 0x5d4;
constexpr KeySym KeySym_Arabic_sad                                  = 0x5d5;
constexpr KeySym KeySym_Arabic_dad                                  = 0x5d6;
constexpr KeySym KeySym_Arabic_tah                                  = 0x5d7;
constexpr KeySym KeySym_Arabic_zah                                  = 0x5d8;
constexpr KeySym KeySym_Arabic_ain                                  = 0x5d9;
constexpr KeySym KeySym_Arabic_ghain                                = 0x5da;
constexpr KeySym KeySym_Arabic_tatweel                              = 0x5e0;
constexpr KeySym KeySym_Arabic_feh                                  = 0x5e1;
constexpr KeySym KeySym_Arabic_qaf                                  = 0x5e2;
constexpr KeySym KeySym_Arabic_kaf                                  = 0x5e3;
constexpr KeySym KeySym_Arabic_lam                                  = 0x5e4;
constexpr KeySym KeySym_Arabic_meem                                 = 0x5e5;
constexpr KeySym KeySym_Arabic_noon                                 = 0x5e6;
constexpr KeySym KeySym_Arabic_ha                                   = 0x5e7;
constexpr KeySym KeySym_Arabic_waw                                  = 0x5e8;
constexpr KeySym KeySym_Arabic_alefmaksura                          = 0x5e9;
constexpr KeySym KeySym_Arabic_yeh                                  = 0x5ea;
constexpr KeySym KeySym_Arabic_fathatan                             = 0x5eb;
constexpr KeySym KeySym_Arabic_dammatan                             = 0x5ec;
constexpr KeySym KeySym_Arabic_kasratan                             = 0x5ed;
constexpr KeySym KeySym_Arabic_fatha                                = 0x5ee;
constexpr KeySym KeySym_Arabic_damma                                = 0x5ef;
constexpr KeySym KeySym_Arabic_kasra                                = 0x5f0;
constexpr KeySym KeySym_Arabic_shadda                               = 0x5f1;
constexpr KeySym KeySym_Arabic_sukun                                = 0x5f2;
constexpr KeySym KeySym_Arabic_madda_above                          = 0x5f3;
constexpr KeySym KeySym_Arabic_hamza_above                          = 0x5f4;
constexpr KeySym KeySym_Arabic_hamza_below                          = 0x5f5;
constexpr KeySym KeySym_Arabic_jeh                                  = 0x5f6;
constexpr KeySym KeySym_Arabic_veh                                  = 0x5f7;
constexpr KeySym KeySym_Arabic_keheh                                = 0x5f8;
constexpr KeySym KeySym_Arabic_gaf                                  = 0x5f9;
constexpr KeySym KeySym_Arabic_noon_ghunna                          = 0x5fa;
constexpr KeySym KeySym_Arabic_heh_doachashmee                      = 0x5fb;
constexpr KeySym KeySym_Farsi_yeh                                   = 0x5fc;
constexpr KeySym KeySym_Arabic_farsi_yeh                            = KeySym_Farsi_yeh; // symbol alias
constexpr KeySym KeySym_Arabic_yeh_baree                            = 0x5fd;
constexpr KeySym KeySym_Arabic_heh_goal                             = 0x5fe;
constexpr KeySym KeySym_Arabic_switch                               = 0xFF7E; // Alias for mode_switch
/// @}

/// @{ Cyrillic.
///
/// Byte 3 = 6
constexpr KeySym KeySym_Cyrillic_GHE_bar                            = 0x680;
constexpr KeySym KeySym_Cyrillic_ghe_bar                            = 0x690;
constexpr KeySym KeySym_Cyrillic_ZHE_descender                      = 0x681;
constexpr KeySym KeySym_Cyrillic_zhe_descender                      = 0x691;
constexpr KeySym KeySym_Cyrillic_KA_descender                       = 0x682;
constexpr KeySym KeySym_Cyrillic_ka_descender                       = 0x692;
constexpr KeySym KeySym_Cyrillic_KA_vertstroke                      = 0x683;
constexpr KeySym KeySym_Cyrillic_ka_vertstroke                      = 0x693;
constexpr KeySym KeySym_Cyrillic_EN_descender                       = 0x684;
constexpr KeySym KeySym_Cyrillic_en_descender                       = 0x694;
constexpr KeySym KeySym_Cyrillic_U_straight                         = 0x685;
constexpr KeySym KeySym_Cyrillic_u_straight                         = 0x695;
constexpr KeySym KeySym_Cyrillic_U_straight_bar                     = 0x686;
constexpr KeySym KeySym_Cyrillic_u_straight_bar                     = 0x696;
constexpr KeySym KeySym_Cyrillic_HA_descender                       = 0x687;
constexpr KeySym KeySym_Cyrillic_ha_descender                       = 0x697;
constexpr KeySym KeySym_Cyrillic_CHE_descender                      = 0x688;
constexpr KeySym KeySym_Cyrillic_che_descender                      = 0x698;
constexpr KeySym KeySym_Cyrillic_CHE_vertstroke                     = 0x689;
constexpr KeySym KeySym_Cyrillic_che_vertstroke                     = 0x699;
constexpr KeySym KeySym_Cyrillic_SHHA                               = 0x68a;
constexpr KeySym KeySym_Cyrillic_shha                               = 0x69a;

constexpr KeySym KeySym_Cyrillic_SCHWA                              = 0x68c;
constexpr KeySym KeySym_Cyrillic_schwa                              = 0x69c;
constexpr KeySym KeySym_Cyrillic_I_macron                           = 0x68d;
constexpr KeySym KeySym_Cyrillic_i_macron                           = 0x69d;
constexpr KeySym KeySym_Cyrillic_O_bar                              = 0x68e;
constexpr KeySym KeySym_Cyrillic_o_bar                              = 0x69e;
constexpr KeySym KeySym_Cyrillic_U_macron                           = 0x68f;
constexpr KeySym KeySym_Cyrillic_u_macron                           = 0x69f;

constexpr KeySym KeySym_Serbian_dje                                 = 0x6a1;
constexpr KeySym KeySym_Macedonia_gje                               = 0x6a2;
constexpr KeySym KeySym_Cyrillic_io                                 = 0x6a3;
constexpr KeySym KeySym_Ukrainian_ie                                = 0x6a4;
constexpr KeySym KeySym_Macedonia_dse                               = 0x6a5;
constexpr KeySym KeySym_Ukrainian_i                                 = 0x6a6;
constexpr KeySym KeySym_Ukrainian_yi                                = 0x6a7;
constexpr KeySym KeySym_Cyrillic_je                                 = 0x6a8;
constexpr KeySym KeySym_Cyrillic_lje                                = 0x6a9;
constexpr KeySym KeySym_Cyrillic_nje                                = 0x6aa;
constexpr KeySym KeySym_Serbian_tshe                                = 0x6ab;
constexpr KeySym KeySym_Macedonia_kje                               = 0x6ac;
constexpr KeySym KeySym_Ukrainian_ghe_with_upturn                   = 0x6ad;
constexpr KeySym KeySym_Byelorussian_shortu                         = 0x6ae;
constexpr KeySym KeySym_Cyrillic_dzhe                               = 0x6af;
constexpr KeySym KeySym_numerosign                                  = 0x6b0;
constexpr KeySym KeySym_Serbian_DJE                                 = 0x6b1;
constexpr KeySym KeySym_Macedonia_GJE                               = 0x6b2;
constexpr KeySym KeySym_Cyrillic_IO                                 = 0x6b3;
constexpr KeySym KeySym_Ukrainian_IE                                = 0x6b4;
constexpr KeySym KeySym_Macedonia_DSE                               = 0x6b5;
constexpr KeySym KeySym_Ukrainian_I                                 = 0x6b6;
constexpr KeySym KeySym_Ukrainian_YI                                = 0x6b7;
constexpr KeySym KeySym_Cyrillic_JE                                 = 0x6b8;
constexpr KeySym KeySym_Cyrillic_LJE                                = 0x6b9;
constexpr KeySym KeySym_Cyrillic_NJE                                = 0x6ba;
constexpr KeySym KeySym_Serbian_TSHE                                = 0x6bb;
constexpr KeySym KeySym_Macedonia_KJE                               = 0x6bc;
constexpr KeySym KeySym_Ukrainian_GHE_WITH_UPTURN                   = 0x6bd;
constexpr KeySym KeySym_Byelorussian_SHORTU                         = 0x6be;
constexpr KeySym KeySym_Cyrillic_DZHE                               = 0x6bf;
constexpr KeySym KeySym_Cyrillic_yu                                 = 0x6c0;
constexpr KeySym KeySym_Cyrillic_a                                  = 0x6c1;
constexpr KeySym KeySym_Cyrillic_be                                 = 0x6c2;
constexpr KeySym KeySym_Cyrillic_tse                                = 0x6c3;
constexpr KeySym KeySym_Cyrillic_de                                 = 0x6c4;
constexpr KeySym KeySym_Cyrillic_ie                                 = 0x6c5;
constexpr KeySym KeySym_Cyrillic_ef                                 = 0x6c6;
constexpr KeySym KeySym_Cyrillic_ghe                                = 0x6c7;
constexpr KeySym KeySym_Cyrillic_ha                                 = 0x6c8;
constexpr KeySym KeySym_Cyrillic_i                                  = 0x6c9;
constexpr KeySym KeySym_Cyrillic_shorti                             = 0x6ca;
constexpr KeySym KeySym_Cyrillic_ka                                 = 0x6cb;
constexpr KeySym KeySym_Cyrillic_el                                 = 0x6cc;
constexpr KeySym KeySym_Cyrillic_em                                 = 0x6cd;
constexpr KeySym KeySym_Cyrillic_en                                 = 0x6ce;
constexpr KeySym KeySym_Cyrillic_o                                  = 0x6cf;
constexpr KeySym KeySym_Cyrillic_pe                                 = 0x6d0;
constexpr KeySym KeySym_Cyrillic_ya                                 = 0x6d1;
constexpr KeySym KeySym_Cyrillic_er                                 = 0x6d2;
constexpr KeySym KeySym_Cyrillic_es                                 = 0x6d3;
constexpr KeySym KeySym_Cyrillic_te                                 = 0x6d4;
constexpr KeySym KeySym_Cyrillic_u                                  = 0x6d5;
constexpr KeySym KeySym_Cyrillic_zhe                                = 0x6d6;
constexpr KeySym KeySym_Cyrillic_ve                                 = 0x6d7;
constexpr KeySym KeySym_Cyrillic_softsign                           = 0x6d8;
constexpr KeySym KeySym_Cyrillic_yeru                               = 0x6d9;
constexpr KeySym KeySym_Cyrillic_ze                                 = 0x6da;
constexpr KeySym KeySym_Cyrillic_sha                                = 0x6db;
constexpr KeySym KeySym_Cyrillic_e                                  = 0x6dc;
constexpr KeySym KeySym_Cyrillic_shcha                              = 0x6dd;
constexpr KeySym KeySym_Cyrillic_che                                = 0x6de;
constexpr KeySym KeySym_Cyrillic_hardsign                           = 0x6df;
constexpr KeySym KeySym_Cyrillic_YU                                 = 0x6e0;
constexpr KeySym KeySym_Cyrillic_A                                  = 0x6e1;
constexpr KeySym KeySym_Cyrillic_BE                                 = 0x6e2;
constexpr KeySym KeySym_Cyrillic_TSE                                = 0x6e3;
constexpr KeySym KeySym_Cyrillic_DE                                 = 0x6e4;
constexpr KeySym KeySym_Cyrillic_IE                                 = 0x6e5;
constexpr KeySym KeySym_Cyrillic_EF                                 = 0x6e6;
constexpr KeySym KeySym_Cyrillic_GHE                                = 0x6e7;
constexpr KeySym KeySym_Cyrillic_HA                                 = 0x6e8;
constexpr KeySym KeySym_Cyrillic_I                                  = 0x6e9;
constexpr KeySym KeySym_Cyrillic_SHORTI                             = 0x6ea;
constexpr KeySym KeySym_Cyrillic_KA                                 = 0x6eb;
constexpr KeySym KeySym_Cyrillic_EL                                 = 0x6ec;
constexpr KeySym KeySym_Cyrillic_EM                                 = 0x6ed;
constexpr KeySym KeySym_Cyrillic_EN                                 = 0x6ee;
constexpr KeySym KeySym_Cyrillic_O                                  = 0x6ef;
constexpr KeySym KeySym_Cyrillic_PE                                 = 0x6f0;
constexpr KeySym KeySym_Cyrillic_YA                                 = 0x6f1;
constexpr KeySym KeySym_Cyrillic_ER                                 = 0x6f2;
constexpr KeySym KeySym_Cyrillic_ES                                 = 0x6f3;
constexpr KeySym KeySym_Cyrillic_TE                                 = 0x6f4;
constexpr KeySym KeySym_Cyrillic_U                                  = 0x6f5;
constexpr KeySym KeySym_Cyrillic_ZHE                                = 0x6f6;
constexpr KeySym KeySym_Cyrillic_VE                                 = 0x6f7;
constexpr KeySym KeySym_Cyrillic_SOFTSIGN                           = 0x6f8;
constexpr KeySym KeySym_Cyrillic_YERU                               = 0x6f9;
constexpr KeySym KeySym_Cyrillic_ZE                                 = 0x6fa;
constexpr KeySym KeySym_Cyrillic_SHA                                = 0x6fb;
constexpr KeySym KeySym_Cyrillic_E                                  = 0x6fc;
constexpr KeySym KeySym_Cyrillic_SHCHA                              = 0x6fd;
constexpr KeySym KeySym_Cyrillic_CHE                                = 0x6fe;
constexpr KeySym KeySym_Cyrillic_HARDSIGN                           = 0x6ff;
/// @}

/// @{ Greek.
///
/// Byte 3 = 7
constexpr KeySym KeySym_Greek_ALPHAaccent                           = 0x7a1;
constexpr KeySym KeySym_Greek_EPSILONaccent                         = 0x7a2;
constexpr KeySym KeySym_Greek_ETAaccent                             = 0x7a3;
constexpr KeySym KeySym_Greek_IOTAaccent                            = 0x7a4;
constexpr KeySym KeySym_Greek_IOTAdieresis                          = 0x7a5;
constexpr KeySym KeySym_Greek_OMICRONaccent                         = 0x7a7;
constexpr KeySym KeySym_Greek_UPSILONaccent                         = 0x7a8;
constexpr KeySym KeySym_Greek_UPSILONdieresis                       = 0x7a9;
constexpr KeySym KeySym_Greek_OMEGAaccent                           = 0x7ab;
constexpr KeySym KeySym_Greek_accentdieresis                        = 0x7ae;
constexpr KeySym KeySym_Greek_horizbar                              = 0x7af;
constexpr KeySym KeySym_Greek_alphaaccent                           = 0x7b1;
constexpr KeySym KeySym_Greek_epsilonaccent                         = 0x7b2;
constexpr KeySym KeySym_Greek_etaaccent                             = 0x7b3;
constexpr KeySym KeySym_Greek_iotaaccent                            = 0x7b4;
constexpr KeySym KeySym_Greek_iotadieresis                          = 0x7b5;
constexpr KeySym KeySym_Greek_iotaaccentdieresis                    = 0x7b6;
constexpr KeySym KeySym_Greek_omicronaccent                         = 0x7b7;
constexpr KeySym KeySym_Greek_upsilonaccent                         = 0x7b8;
constexpr KeySym KeySym_Greek_upsilondieresis                       = 0x7b9;
constexpr KeySym KeySym_Greek_upsilonaccentdieresis                 = 0x7ba;
constexpr KeySym KeySym_Greek_omegaaccent                           = 0x7bb;
constexpr KeySym KeySym_Greek_ALPHA                                 = 0x7c1;
constexpr KeySym KeySym_Greek_BETA                                  = 0x7c2;
constexpr KeySym KeySym_Greek_GAMMA                                 = 0x7c3;
constexpr KeySym KeySym_Greek_DELTA                                 = 0x7c4;
constexpr KeySym KeySym_Greek_EPSILON                               = 0x7c5;
constexpr KeySym KeySym_Greek_ZETA                                  = 0x7c6;
constexpr KeySym KeySym_Greek_ETA                                   = 0x7c7;
constexpr KeySym KeySym_Greek_THETA                                 = 0x7c8;
constexpr KeySym KeySym_Greek_IOTA                                  = 0x7c9;
constexpr KeySym KeySym_Greek_KAPPA                                 = 0x7ca;
constexpr KeySym KeySym_Greek_LAMDA                                 = 0x7cb; // Alias for Greek_LAMBDA
constexpr KeySym KeySym_Greek_LAMBDA                                = 0x7cb;
constexpr KeySym KeySym_Greek_MU                                    = 0x7cc;
constexpr KeySym KeySym_Greek_NU                                    = 0x7cd;
constexpr KeySym KeySym_Greek_XI                                    = 0x7ce;
constexpr KeySym KeySym_Greek_OMICRON                               = 0x7cf;
constexpr KeySym KeySym_Greek_PI                                    = 0x7d0;
constexpr KeySym KeySym_Greek_RHO                                   = 0x7d1;
constexpr KeySym KeySym_Greek_SIGMA                                 = 0x7d2;
constexpr KeySym KeySym_Greek_TAU                                   = 0x7d4;
constexpr KeySym KeySym_Greek_UPSILON                               = 0x7d5;
constexpr KeySym KeySym_Greek_PHI                                   = 0x7d6;
constexpr KeySym KeySym_Greek_CHI                                   = 0x7d7;
constexpr KeySym KeySym_Greek_PSI                                   = 0x7d8;
constexpr KeySym KeySym_Greek_OMEGA                                 = 0x7d9;
constexpr KeySym KeySym_Greek_alpha                                 = 0x7e1;
constexpr KeySym KeySym_Greek_beta                                  = 0x7e2;
constexpr KeySym KeySym_Greek_gamma                                 = 0x7e3;
constexpr KeySym KeySym_Greek_delta                                 = 0x7e4;
constexpr KeySym KeySym_Greek_epsilon                               = 0x7e5;
constexpr KeySym KeySym_Greek_zeta                                  = 0x7e6;
constexpr KeySym KeySym_Greek_eta                                   = 0x7e7;
constexpr KeySym KeySym_Greek_theta                                 = 0x7e8;
constexpr KeySym KeySym_Greek_iota                                  = 0x7e9;
constexpr KeySym KeySym_Greek_kappa                                 = 0x7ea;
constexpr KeySym KeySym_Greek_lamda                                 = 0x7eb; // Alias for Greek_lambda
constexpr KeySym KeySym_Greek_lambda                                = 0x7eb;
constexpr KeySym KeySym_Greek_mu                                    = 0x7ec;
constexpr KeySym KeySym_Greek_nu                                    = 0x7ed;
constexpr KeySym KeySym_Greek_xi                                    = 0x7ee;
constexpr KeySym KeySym_Greek_omicron                               = 0x7ef;
constexpr KeySym KeySym_Greek_pi                                    = 0x7f0;
constexpr KeySym KeySym_Greek_rho                                   = 0x7f1;
constexpr KeySym KeySym_Greek_sigma                                 = 0x7f2;
constexpr KeySym KeySym_Greek_finalsmallsigma                       = 0x7f3;
constexpr KeySym KeySym_Greek_tau                                   = 0x7f4;
constexpr KeySym KeySym_Greek_upsilon                               = 0x7f5;
constexpr KeySym KeySym_Greek_phi                                   = 0x7f6;
constexpr KeySym KeySym_Greek_chi                                   = 0x7f7;
constexpr KeySym KeySym_Greek_psi                                   = 0x7f8;
constexpr KeySym KeySym_Greek_omega                                 = 0x7f9;
constexpr KeySym KeySym_Greek_switch                                = 0xFF7E; // Alias for mode_switch
/// @}

/// @{ Technical.
///
/// Byte 3 = 8
constexpr KeySym KeySym_leftradical                                 = 0x8a1;
constexpr KeySym KeySym_topleftradical                              = 0x8a2;
constexpr KeySym KeySym_horizconnector                              = 0x8a3;
constexpr KeySym KeySym_topintegral                                 = 0x8a4;
constexpr KeySym KeySym_botintegral                                 = 0x8a5;
constexpr KeySym KeySym_vertconnector                               = 0x8a6;
constexpr KeySym KeySym_topleftsqbracket                            = 0x8a7;
constexpr KeySym KeySym_botleftsqbracket                            = 0x8a8;
constexpr KeySym KeySym_toprightsqbracket                           = 0x8a9;
constexpr KeySym KeySym_botrightsqbracket                           = 0x8aa;
constexpr KeySym KeySym_topleftparens                               = 0x8ab;
constexpr KeySym KeySym_botleftparens                               = 0x8ac;
constexpr KeySym KeySym_toprightparens                              = 0x8ad;
constexpr KeySym KeySym_botrightparens                              = 0x8ae;
constexpr KeySym KeySym_leftmiddlecurlybrace                        = 0x8af;
constexpr KeySym KeySym_rightmiddlecurlybrace                       = 0x8b0;
constexpr KeySym KeySym_topleftsummation                            = 0x8b1;
constexpr KeySym KeySym_botleftsummation                            = 0x8b2;
constexpr KeySym KeySym_topvertsummationconnector                   = 0x8b3;
constexpr KeySym KeySym_botvertsummationconnector                   = 0x8b4;
constexpr KeySym KeySym_toprightsummation                           = 0x8b5;
constexpr KeySym KeySym_botrightsummation                           = 0x8b6;
constexpr KeySym KeySym_rightmiddlesummation                        = 0x8b7;
constexpr KeySym KeySym_lessthanequal                               = 0x8bc;
constexpr KeySym KeySym_notequal                                    = 0x8bd;
constexpr KeySym KeySym_greaterthanequal                            = 0x8be;
constexpr KeySym KeySym_integral                                    = 0x8bf;
constexpr KeySym KeySym_therefore                                   = 0x8c0;
constexpr KeySym KeySym_variation                                   = 0x8c1;
constexpr KeySym KeySym_infinity                                    = 0x8c2;
constexpr KeySym KeySym_nabla                                       = 0x8c5;
constexpr KeySym KeySym_approximate                                 = 0x8c8;
constexpr KeySym KeySym_similarequal                                = 0x8c9;
constexpr KeySym KeySym_ifonlyif                                    = 0x8cd;
constexpr KeySym KeySym_implies                                     = 0x8ce;
constexpr KeySym KeySym_identical                                   = 0x8cf;
constexpr KeySym KeySym_radical                                     = 0x8d6;
constexpr KeySym KeySym_includedin                                  = 0x8da;
constexpr KeySym KeySym_includes                                    = 0x8db;
constexpr KeySym KeySym_intersection                                = 0x8dc;
constexpr KeySym KeySym_union                                       = 0x8dd;
constexpr KeySym KeySym_logicaland                                  = 0x8de;
constexpr KeySym KeySym_logicalor                                   = 0x8df;
constexpr KeySym KeySym_partialderivative                           = 0x8ef;
constexpr KeySym KeySym_function                                    = 0x8f6;
constexpr KeySym KeySym_leftarrow                                   = 0x8fb;
constexpr KeySym KeySym_uparrow                                     = 0x8fc;
constexpr KeySym KeySym_rightarrow                                  = 0x8fd;
constexpr KeySym KeySym_downarrow                                   = 0x8fe;
/// @}

/// @{ Special.
///
/// Byte 3 = 9
constexpr KeySym KeySym_blank                                       = 0x9df;
constexpr KeySym KeySym_soliddiamond                                = 0x9e0;
constexpr KeySym KeySym_checkerboard                                = 0x9e1;
constexpr KeySym KeySym_ht                                          = 0x9e2;
constexpr KeySym KeySym_ff                                          = 0x9e3;
constexpr KeySym KeySym_cr                                          = 0x9e4;
constexpr KeySym KeySym_lf                                          = 0x9e5;
constexpr KeySym KeySym_nl                                          = 0x9e8;
constexpr KeySym KeySym_vt                                          = 0x9e9;
constexpr KeySym KeySym_lowrightcorner                              = 0x9ea;
constexpr KeySym KeySym_uprightcorner                               = 0x9eb;
constexpr KeySym KeySym_upleftcorner                                = 0x9ec;
constexpr KeySym KeySym_lowleftcorner                               = 0x9ed;
constexpr KeySym KeySym_crossinglines                               = 0x9ee;
constexpr KeySym KeySym_horizlinescan1                              = 0x9ef;
constexpr KeySym KeySym_horizlinescan3                              = 0x9f0;
constexpr KeySym KeySym_horizlinescan5                              = 0x9f1;
constexpr KeySym KeySym_horizlinescan7                              = 0x9f2;
constexpr KeySym KeySym_horizlinescan9                              = 0x9f3;
constexpr KeySym KeySym_leftt                                       = 0x9f4;
constexpr KeySym KeySym_rightt                                      = 0x9f5;
constexpr KeySym KeySym_bott                                        = 0x9f6;
constexpr KeySym KeySym_topt                                        = 0x9f7;
constexpr KeySym KeySym_vertbar                                     = 0x9f8;
/// @}

/// @{ Publishing.
///
/// Byte 3 = a
constexpr KeySym KeySym_emspace                                     = 0xaa1;
constexpr KeySym KeySym_enspace                                     = 0xaa2;
constexpr KeySym KeySym_em3space                                    = 0xaa3;
constexpr KeySym KeySym_em4space                                    = 0xaa4;
constexpr KeySym KeySym_digitspace                                  = 0xaa5;
constexpr KeySym KeySym_punctspace                                  = 0xaa6;
constexpr KeySym KeySym_thinspace                                   = 0xaa7;
constexpr KeySym KeySym_hairspace                                   = 0xaa8;
constexpr KeySym KeySym_emdash                                      = 0xaa9;
constexpr KeySym KeySym_endash                                      = 0xaaa;
constexpr KeySym KeySym_signifblank                                 = 0xaac;
constexpr KeySym KeySym_ellipsis                                    = 0xaae;
constexpr KeySym KeySym_doubbaselinedot                             = 0xaaf;
constexpr KeySym KeySym_onethird                                    = 0xab0;
constexpr KeySym KeySym_twothirds                                   = 0xab1;
constexpr KeySym KeySym_onefifth                                    = 0xab2;
constexpr KeySym KeySym_twofifths                                   = 0xab3;
constexpr KeySym KeySym_threefifths                                 = 0xab4;
constexpr KeySym KeySym_fourfifths                                  = 0xab5;
constexpr KeySym KeySym_onesixth                                    = 0xab6;
constexpr KeySym KeySym_fivesixths                                  = 0xab7;
constexpr KeySym KeySym_careof                                      = 0xab8;
constexpr KeySym KeySym_figdash                                     = 0xabb;
constexpr KeySym KeySym_leftanglebracket                            = 0xabc;
constexpr KeySym KeySym_decimalpoint                                = 0xabd;
constexpr KeySym KeySym_rightanglebracket                           = 0xabe;
constexpr KeySym KeySym_marker                                      = 0xabf;
constexpr KeySym KeySym_oneeighth                                   = 0xac3;
constexpr KeySym KeySym_threeeighths                                = 0xac4;
constexpr KeySym KeySym_fiveeighths                                 = 0xac5;
constexpr KeySym KeySym_seveneighths                                = 0xac6;
constexpr KeySym KeySym_trademark                                   = 0xac9;
constexpr KeySym KeySym_signaturemark                               = 0xaca;
constexpr KeySym KeySym_trademarkincircle                           = 0xacb;
constexpr KeySym KeySym_leftopentriangle                            = 0xacc;
constexpr KeySym KeySym_rightopentriangle                           = 0xacd;
constexpr KeySym KeySym_emopencircle                                = 0xace;
constexpr KeySym KeySym_emopenrectangle                             = 0xacf;
constexpr KeySym KeySym_leftsinglequotemark                         = 0xad0;
constexpr KeySym KeySym_rightsinglequotemark                        = 0xad1;
constexpr KeySym KeySym_leftdoublequotemark                         = 0xad2;
constexpr KeySym KeySym_rightdoublequotemark                        = 0xad3;
constexpr KeySym KeySym_prescription                                = 0xad4;
constexpr KeySym KeySym_minutes                                     = 0xad6;
constexpr KeySym KeySym_seconds                                     = 0xad7;
constexpr KeySym KeySym_latincross                                  = 0xad9;
constexpr KeySym KeySym_hexagram                                    = 0xada;
constexpr KeySym KeySym_filledrectbullet                            = 0xadb;
constexpr KeySym KeySym_filledlefttribullet                         = 0xadc;
constexpr KeySym KeySym_filledrighttribullet                        = 0xadd;
constexpr KeySym KeySym_emfilledcircle                              = 0xade;
constexpr KeySym KeySym_emfilledrect                                = 0xadf;
constexpr KeySym KeySym_enopencircbullet                            = 0xae0;
constexpr KeySym KeySym_enopensquarebullet                          = 0xae1;
constexpr KeySym KeySym_openrectbullet                              = 0xae2;
constexpr KeySym KeySym_opentribulletup                             = 0xae3;
constexpr KeySym KeySym_opentribulletdown                           = 0xae4;
constexpr KeySym KeySym_openstar                                    = 0xae5;
constexpr KeySym KeySym_enfilledcircbullet                          = 0xae6;
constexpr KeySym KeySym_enfilledsqbullet                            = 0xae7;
constexpr KeySym KeySym_filledtribulletup                           = 0xae8;
constexpr KeySym KeySym_filledtribulletdown                         = 0xae9;
constexpr KeySym KeySym_leftpointer                                 = 0xaea;
constexpr KeySym KeySym_rightpointer                                = 0xaeb;
constexpr KeySym KeySym_club                                        = 0xaec;
constexpr KeySym KeySym_diamond                                     = 0xaed;
constexpr KeySym KeySym_heart                                       = 0xaee;
constexpr KeySym KeySym_maltesecross                                = 0xaf0;
constexpr KeySym KeySym_dagger                                      = 0xaf1;
constexpr KeySym KeySym_doubledagger                                = 0xaf2;
constexpr KeySym KeySym_checkmark                                   = 0xaf3;
constexpr KeySym KeySym_ballotcross                                 = 0xaf4;
constexpr KeySym KeySym_musicalsharp                                = 0xaf5;
constexpr KeySym KeySym_musicalflat                                 = 0xaf6;
constexpr KeySym KeySym_malesymbol                                  = 0xaf7;
constexpr KeySym KeySym_femalesymbol                                = 0xaf8;
constexpr KeySym KeySym_telephone                                   = 0xaf9;
constexpr KeySym KeySym_telephonerecorder                           = 0xafa;
constexpr KeySym KeySym_phonographcopyright                         = 0xafb;
constexpr KeySym KeySym_caret                                       = 0xafc;
constexpr KeySym KeySym_singlelowquotemark                          = 0xafd;
constexpr KeySym KeySym_doublelowquotemark                          = 0xafe;
constexpr KeySym KeySym_cursor                                      = 0xaff;
/// @}

/// @{ APL.
///
/// Byte 3 = b
constexpr KeySym KeySym_leftcaret                                   = 0xba3;
constexpr KeySym KeySym_rightcaret                                  = 0xba6;
constexpr KeySym KeySym_downcaret                                   = 0xba8;
constexpr KeySym KeySym_upcaret                                     = 0xba9;
constexpr KeySym KeySym_overbar                                     = 0xbc0;
constexpr KeySym KeySym_downtack                                    = 0xbc2;
constexpr KeySym KeySym_upshoe                                      = 0xbc3;
constexpr KeySym KeySym_downstile                                   = 0xbc4;
constexpr KeySym KeySym_underbar                                    = 0xbc6;
constexpr KeySym KeySym_jot                                         = 0xbca;
constexpr KeySym KeySym_quad                                        = 0xbcc;
constexpr KeySym KeySym_uptack                                      = 0xbce;
constexpr KeySym KeySym_circle                                      = 0xbcf;
constexpr KeySym KeySym_upstile                                     = 0xbd3;
constexpr KeySym KeySym_downshoe                                    = 0xbd6;
constexpr KeySym KeySym_rightshoe                                   = 0xbd8;
constexpr KeySym KeySym_leftshoe                                    = 0xbda;
constexpr KeySym KeySym_lefttack                                    = 0xbdc;
constexpr KeySym KeySym_righttack                                   = 0xbfc;
/// @}

/// @{ Hebrew.
///
/// Byte 3 = c
constexpr KeySym KeySym_hebrew_doublelowline                        = 0xcdf;
constexpr KeySym KeySym_hebrew_aleph                                = 0xce0;
constexpr KeySym KeySym_hebrew_bet                                  = 0xce1;
constexpr KeySym KeySym_hebrew_gimel                                = 0xce2;
constexpr KeySym KeySym_hebrew_dalet                                = 0xce3;
constexpr KeySym KeySym_hebrew_he                                   = 0xce4;
constexpr KeySym KeySym_hebrew_waw                                  = 0xce5;
constexpr KeySym KeySym_hebrew_zain                                 = 0xce6;
constexpr KeySym KeySym_hebrew_chet                                 = 0xce7;
constexpr KeySym KeySym_hebrew_tet                                  = 0xce8;
constexpr KeySym KeySym_hebrew_yod                                  = 0xce9;
constexpr KeySym KeySym_hebrew_finalkaph                            = 0xcea;
constexpr KeySym KeySym_hebrew_kaph                                 = 0xceb;
constexpr KeySym KeySym_hebrew_lamed                                = 0xcec;
constexpr KeySym KeySym_hebrew_finalmem                             = 0xced;
constexpr KeySym KeySym_hebrew_mem                                  = 0xcee;
constexpr KeySym KeySym_hebrew_finalnun                             = 0xcef;
constexpr KeySym KeySym_hebrew_nun                                  = 0xcf0;
constexpr KeySym KeySym_hebrew_samech                               = 0xcf1;
constexpr KeySym KeySym_hebrew_ayin                                 = 0xcf2;
constexpr KeySym KeySym_hebrew_finalpe                              = 0xcf3;
constexpr KeySym KeySym_hebrew_pe                                   = 0xcf4;
constexpr KeySym KeySym_hebrew_finalzade                            = 0xcf5;
constexpr KeySym KeySym_hebrew_zade                                 = 0xcf6;
constexpr KeySym KeySym_hebrew_qoph                                 = 0xcf7;
constexpr KeySym KeySym_hebrew_resh                                 = 0xcf8;
constexpr KeySym KeySym_hebrew_shin                                 = 0xcf9;
constexpr KeySym KeySym_hebrew_taw                                  = 0xcfa;
constexpr KeySym KeySym_Hebrew_switch                               = 0xFF7E; // Alias for mode_switch
/// @}

/// @{ Thai.
///
/// Byte 3 = d
constexpr KeySym KeySym_Thai_kokai             = 0xda1;
constexpr KeySym KeySym_Thai_khokhai           = 0xda2;
constexpr KeySym KeySym_Thai_khokhuat          = 0xda3;
constexpr KeySym KeySym_Thai_khokhwai          = 0xda4;
constexpr KeySym KeySym_Thai_khokhon           = 0xda5;
constexpr KeySym KeySym_Thai_khorakhang        = 0xda6;
constexpr KeySym KeySym_Thai_ngongu            = 0xda7;
constexpr KeySym KeySym_Thai_chochan           = 0xda8;
constexpr KeySym KeySym_Thai_choching          = 0xda9;
constexpr KeySym KeySym_Thai_chochang          = 0xdaa;
constexpr KeySym KeySym_Thai_soso              = 0xdab;
constexpr KeySym KeySym_Thai_chochoe           = 0xdac;
constexpr KeySym KeySym_Thai_yoying            = 0xdad;
constexpr KeySym KeySym_Thai_dochada           = 0xdae;
constexpr KeySym KeySym_Thai_topatak           = 0xdaf;
constexpr KeySym KeySym_Thai_thothan           = 0xdb0;
constexpr KeySym KeySym_Thai_thonangmontho     = 0xdb1;
constexpr KeySym KeySym_Thai_thophuthao        = 0xdb2;
constexpr KeySym KeySym_Thai_nonen             = 0xdb3;
constexpr KeySym KeySym_Thai_dodek             = 0xdb4;
constexpr KeySym KeySym_Thai_totao             = 0xdb5;
constexpr KeySym KeySym_Thai_thothung          = 0xdb6;
constexpr KeySym KeySym_Thai_thothahan         = 0xdb7;
constexpr KeySym KeySym_Thai_thothong          = 0xdb8;
constexpr KeySym KeySym_Thai_nonu              = 0xdb9;
constexpr KeySym KeySym_Thai_bobaimai          = 0xdba;
constexpr KeySym KeySym_Thai_popla             = 0xdbb;
constexpr KeySym KeySym_Thai_phophung          = 0xdbc;
constexpr KeySym KeySym_Thai_fofa              = 0xdbd;
constexpr KeySym KeySym_Thai_phophan           = 0xdbe;
constexpr KeySym KeySym_Thai_fofan             = 0xdbf;
constexpr KeySym KeySym_Thai_phosamphao        = 0xdc0;
constexpr KeySym KeySym_Thai_moma              = 0xdc1;
constexpr KeySym KeySym_Thai_yoyak             = 0xdc2;
constexpr KeySym KeySym_Thai_rorua             = 0xdc3;
constexpr KeySym KeySym_Thai_ru                = 0xdc4;
constexpr KeySym KeySym_Thai_loling            = 0xdc5;
constexpr KeySym KeySym_Thai_lu                = 0xdc6;
constexpr KeySym KeySym_Thai_wowaen            = 0xdc7;
constexpr KeySym KeySym_Thai_sosala            = 0xdc8;
constexpr KeySym KeySym_Thai_sorusi            = 0xdc9;
constexpr KeySym KeySym_Thai_sosua             = 0xdca;
constexpr KeySym KeySym_Thai_hohip             = 0xdcb;
constexpr KeySym KeySym_Thai_lochula           = 0xdcc;
constexpr KeySym KeySym_Thai_oang              = 0xdcd;
constexpr KeySym KeySym_Thai_honokhuk          = 0xdce;
constexpr KeySym KeySym_Thai_paiyannoi         = 0xdcf;
constexpr KeySym KeySym_Thai_saraa             = 0xdd0;
constexpr KeySym KeySym_Thai_maihanakat        = 0xdd1;
constexpr KeySym KeySym_Thai_saraaa            = 0xdd2;
constexpr KeySym KeySym_Thai_saraam            = 0xdd3;
constexpr KeySym KeySym_Thai_sarai             = 0xdd4;
constexpr KeySym KeySym_Thai_saraii            = 0xdd5;
constexpr KeySym KeySym_Thai_saraue            = 0xdd6;
constexpr KeySym KeySym_Thai_sarauee           = 0xdd7;
constexpr KeySym KeySym_Thai_sarau             = 0xdd8;
constexpr KeySym KeySym_Thai_sarauu            = 0xdd9;
constexpr KeySym KeySym_Thai_phinthu           = 0xdda;
constexpr KeySym KeySym_Thai_maihanakat_maitho = 0xdde;
constexpr KeySym KeySym_Thai_baht              = 0xddf;
constexpr KeySym KeySym_Thai_sarae             = 0xde0;
constexpr KeySym KeySym_Thai_saraae            = 0xde1;
constexpr KeySym KeySym_Thai_sarao             = 0xde2;
constexpr KeySym KeySym_Thai_saraaimaimuan     = 0xde3;
constexpr KeySym KeySym_Thai_saraaimaimalai    = 0xde4;
constexpr KeySym KeySym_Thai_lakkhangyao       = 0xde5;
constexpr KeySym KeySym_Thai_maiyamok          = 0xde6;
constexpr KeySym KeySym_Thai_maitaikhu         = 0xde7;
constexpr KeySym KeySym_Thai_maiek             = 0xde8;
constexpr KeySym KeySym_Thai_maitho            = 0xde9;
constexpr KeySym KeySym_Thai_maitri            = 0xdea;
constexpr KeySym KeySym_Thai_maichattawa       = 0xdeb;
constexpr KeySym KeySym_Thai_thanthakhat       = 0xdec;
constexpr KeySym KeySym_Thai_nikhahit          = 0xded;
constexpr KeySym KeySym_Thai_leksun            = 0xdf0;
constexpr KeySym KeySym_Thai_leknung           = 0xdf1;
constexpr KeySym KeySym_Thai_leksong           = 0xdf2;
constexpr KeySym KeySym_Thai_leksam            = 0xdf3;
constexpr KeySym KeySym_Thai_leksi             = 0xdf4;
constexpr KeySym KeySym_Thai_lekha             = 0xdf5;
constexpr KeySym KeySym_Thai_lekhok            = 0xdf6;
constexpr KeySym KeySym_Thai_lekchet           = 0xdf7;
constexpr KeySym KeySym_Thai_lekpaet           = 0xdf8;
constexpr KeySym KeySym_Thai_lekkao            = 0xdf9;
/// @}

/// @{ Korean.
///
/// Byte 3 = e
constexpr KeySym KeySym_Hangul                     = 0xff31; // Hangul start/stop(toggle)
constexpr KeySym KeySym_Hangul_Start               = 0xff32; // Hangul start
constexpr KeySym KeySym_Hangul_End                 = 0xff33; // Hangul end, English start
constexpr KeySym KeySym_Hangul_Hanja               = 0xff34; // Start Hangul->Hanja Conversion
constexpr KeySym KeySym_Hangul_Jamo                = 0xff35; // Hangul Jamo mode
constexpr KeySym KeySym_Hangul_Romaja              = 0xff36; // Hangul Romaja mode
constexpr KeySym KeySym_Hangul_Codeinput           = 0xff37; // Hangul code input mode (Alias for Codeinput)
constexpr KeySym KeySym_Hangul_Jeonja              = 0xff38; // Jeonja mode
constexpr KeySym KeySym_Hangul_Banja               = 0xff39; // Banja mode
constexpr KeySym KeySym_Hangul_PreHanja            = 0xff3a; // Pre Hanja conversion
constexpr KeySym KeySym_Hangul_PostHanja           = 0xff3b; // Post Hanja conversion
constexpr KeySym KeySym_Hangul_SingleCandidate     = 0xff3c; // Alias for SingleCandidate
constexpr KeySym KeySym_Hangul_MultipleCandidate   = 0xff3d; // Alias for MultipleCandidate
constexpr KeySym KeySym_Hangul_PreviousCandidate   = 0xff3e; // Alias for PreviousCandidate
constexpr KeySym KeySym_Hangul_Special             = 0xff3f; // Special symbols
constexpr KeySym KeySym_Hangul_switch              = 0xFF7E; // Alias for mode_switch
// Hangul Consonant Characters
constexpr KeySym KeySym_Hangul_Kiyeog              = 0xea1;
constexpr KeySym KeySym_Hangul_SsangKiyeog         = 0xea2;
constexpr KeySym KeySym_Hangul_KiyeogSios          = 0xea3;
constexpr KeySym KeySym_Hangul_Nieun               = 0xea4;
constexpr KeySym KeySym_Hangul_NieunJieuj          = 0xea5;
constexpr KeySym KeySym_Hangul_NieunHieuh          = 0xea6;
constexpr KeySym KeySym_Hangul_Dikeud              = 0xea7;
constexpr KeySym KeySym_Hangul_SsangDikeud         = 0xea8;
constexpr KeySym KeySym_Hangul_Rieul               = 0xea9;
constexpr KeySym KeySym_Hangul_RieulKiyeog         = 0xeaa;
constexpr KeySym KeySym_Hangul_RieulMieum          = 0xeab;
constexpr KeySym KeySym_Hangul_RieulPieub          = 0xeac;
constexpr KeySym KeySym_Hangul_RieulSios           = 0xead;
constexpr KeySym KeySym_Hangul_RieulTieut          = 0xeae;
constexpr KeySym KeySym_Hangul_RieulPhieuf         = 0xeaf;
constexpr KeySym KeySym_Hangul_RieulHieuh          = 0xeb0;
constexpr KeySym KeySym_Hangul_Mieum               = 0xeb1;
constexpr KeySym KeySym_Hangul_Pieub               = 0xeb2;
constexpr KeySym KeySym_Hangul_SsangPieub          = 0xeb3;
constexpr KeySym KeySym_Hangul_PieubSios           = 0xeb4;
constexpr KeySym KeySym_Hangul_Sios                = 0xeb5;
constexpr KeySym KeySym_Hangul_SsangSios           = 0xeb6;
constexpr KeySym KeySym_Hangul_Ieung               = 0xeb7;
constexpr KeySym KeySym_Hangul_Jieuj               = 0xeb8;
constexpr KeySym KeySym_Hangul_SsangJieuj          = 0xeb9;
constexpr KeySym KeySym_Hangul_Cieuc               = 0xeba;
constexpr KeySym KeySym_Hangul_Khieuq              = 0xebb;
constexpr KeySym KeySym_Hangul_Tieut               = 0xebc;
constexpr KeySym KeySym_Hangul_Phieuf              = 0xebd;
constexpr KeySym KeySym_Hangul_Hieuh               = 0xebe;
// Hangul Vowel Characters
constexpr KeySym KeySym_Hangul_A                   = 0xebf;
constexpr KeySym KeySym_Hangul_AE                  = 0xec0;
constexpr KeySym KeySym_Hangul_YA                  = 0xec1;
constexpr KeySym KeySym_Hangul_YAE                 = 0xec2;
constexpr KeySym KeySym_Hangul_EO                  = 0xec3;
constexpr KeySym KeySym_Hangul_E                   = 0xec4;
constexpr KeySym KeySym_Hangul_YEO                 = 0xec5;
constexpr KeySym KeySym_Hangul_YE                  = 0xec6;
constexpr KeySym KeySym_Hangul_O                   = 0xec7;
constexpr KeySym KeySym_Hangul_WA                  = 0xec8;
constexpr KeySym KeySym_Hangul_WAE                 = 0xec9;
constexpr KeySym KeySym_Hangul_OE                  = 0xeca;
constexpr KeySym KeySym_Hangul_YO                  = 0xecb;
constexpr KeySym KeySym_Hangul_U                   = 0xecc;
constexpr KeySym KeySym_Hangul_WEO                 = 0xecd;
constexpr KeySym KeySym_Hangul_WE                  = 0xece;
constexpr KeySym KeySym_Hangul_WI                  = 0xecf;
constexpr KeySym KeySym_Hangul_YU                  = 0xed0;
constexpr KeySym KeySym_Hangul_EU                  = 0xed1;
constexpr KeySym KeySym_Hangul_YI                  = 0xed2;
constexpr KeySym KeySym_Hangul_I                   = 0xed3;
// Hangul syllable-final (JongSeong) Characters
constexpr KeySym KeySym_Hangul_J_Kiyeog            = 0xed4;
constexpr KeySym KeySym_Hangul_J_SsangKiyeog       = 0xed5;
constexpr KeySym KeySym_Hangul_J_KiyeogSios        = 0xed6;
constexpr KeySym KeySym_Hangul_J_Nieun             = 0xed7;
constexpr KeySym KeySym_Hangul_J_NieunJieuj        = 0xed8;
constexpr KeySym KeySym_Hangul_J_NieunHieuh        = 0xed9;
constexpr KeySym KeySym_Hangul_J_Dikeud            = 0xeda;
constexpr KeySym KeySym_Hangul_J_Rieul             = 0xedb;
constexpr KeySym KeySym_Hangul_J_RieulKiyeog       = 0xedc;
constexpr KeySym KeySym_Hangul_J_RieulMieum        = 0xedd;
constexpr KeySym KeySym_Hangul_J_RieulPieub        = 0xede;
constexpr KeySym KeySym_Hangul_J_RieulSios         = 0xedf;
constexpr KeySym KeySym_Hangul_J_RieulTieut        = 0xee0;
constexpr KeySym KeySym_Hangul_J_RieulPhieuf       = 0xee1;
constexpr KeySym KeySym_Hangul_J_RieulHieuh        = 0xee2;
constexpr KeySym KeySym_Hangul_J_Mieum             = 0xee3;
constexpr KeySym KeySym_Hangul_J_Pieub             = 0xee4;
constexpr KeySym KeySym_Hangul_J_PieubSios         = 0xee5;
constexpr KeySym KeySym_Hangul_J_Sios              = 0xee6;
constexpr KeySym KeySym_Hangul_J_SsangSios         = 0xee7;
constexpr KeySym KeySym_Hangul_J_Ieung             = 0xee8;
constexpr KeySym KeySym_Hangul_J_Jieuj             = 0xee9;
constexpr KeySym KeySym_Hangul_J_Cieuc             = 0xeea;
constexpr KeySym KeySym_Hangul_J_Khieuq            = 0xeeb;
constexpr KeySym KeySym_Hangul_J_Tieut             = 0xeec;
constexpr KeySym KeySym_Hangul_J_Phieuf            = 0xeed;
constexpr KeySym KeySym_Hangul_J_Hieuh             = 0xeee;
// Ancient Hangul Consonant Characters
constexpr KeySym KeySym_Hangul_RieulYeorinHieuh    = 0xeef;
constexpr KeySym KeySym_Hangul_SunkyeongeumMieum   = 0xef0;
constexpr KeySym KeySym_Hangul_SunkyeongeumPieub   = 0xef1;
constexpr KeySym KeySym_Hangul_PanSios             = 0xef2;
constexpr KeySym KeySym_Hangul_KkogjiDalrinIeung   = 0xef3;
constexpr KeySym KeySym_Hangul_SunkyeongeumPhieuf  = 0xef4;
constexpr KeySym KeySym_Hangul_YeorinHieuh         = 0xef5;
// Ancient Hangul Vowel Characters
constexpr KeySym KeySym_Hangul_AraeA               = 0xef6;
constexpr KeySym KeySym_Hangul_AraeAE              = 0xef7;
// Ancient Hangul syllable-final (JongSeong) Characters
constexpr KeySym KeySym_Hangul_J_PanSios           = 0xef8;
constexpr KeySym KeySym_Hangul_J_KkogjiDalrinIeung = 0xef9;
constexpr KeySym KeySym_Hangul_J_YeorinHieuh       = 0xefa;
// Korean currency symbol
constexpr KeySym KeySym_Korean_Won                 = 0xeff;
/// @}

/// @{ Armenian.
///
/// Byte 3 = 0x14
constexpr KeySym KeySym_Armenian_eternity        = 0x14a1;
constexpr KeySym KeySym_Armenian_ligature_ew     = 0x14a2;
constexpr KeySym KeySym_Armenian_full_stop       = 0x14a3;
constexpr KeySym KeySym_Armenian_verjaket        = 0x14a3; // Alias for Armenian_full_stop
constexpr KeySym KeySym_Armenian_parenright      = 0x14a4;
constexpr KeySym KeySym_Armenian_parenleft       = 0x14a5;
constexpr KeySym KeySym_Armenian_guillemotright  = 0x14a6;
constexpr KeySym KeySym_Armenian_guillemotleft   = 0x14a7;
constexpr KeySym KeySym_Armenian_em_dash         = 0x14a8;
constexpr KeySym KeySym_Armenian_dot             = 0x14a9;
constexpr KeySym KeySym_Armenian_mijaket         = 0x14a9; // Alias for Armenian_dot
constexpr KeySym KeySym_Armenian_separation_mark = 0x14aa;
constexpr KeySym KeySym_Armenian_but             = 0x14aa; // Alias for Armenian_separation_mark
constexpr KeySym KeySym_Armenian_comma           = 0x14ab;
constexpr KeySym KeySym_Armenian_en_dash         = 0x14ac;
constexpr KeySym KeySym_Armenian_hyphen          = 0x14ad;
constexpr KeySym KeySym_Armenian_yentamna        = 0x14ad; // Alias for Armenian_hyphen
constexpr KeySym KeySym_Armenian_ellipsis        = 0x14ae;
constexpr KeySym KeySym_Armenian_exclam          = 0x14af;
constexpr KeySym KeySym_Armenian_amanak          = 0x14af; // Alias for Armenian_exclam
constexpr KeySym KeySym_Armenian_accent          = 0x14b0;
constexpr KeySym KeySym_Armenian_shesht          = 0x14b0; // Alias for Armenian_accent
constexpr KeySym KeySym_Armenian_question        = 0x14b1;
constexpr KeySym KeySym_Armenian_paruyk          = 0x14b1; // Alias for Armenian_question
constexpr KeySym KeySym_Armenian_AYB             = 0x14b2;
constexpr KeySym KeySym_Armenian_ayb             = 0x14b3;
constexpr KeySym KeySym_Armenian_BEN             = 0x14b4;
constexpr KeySym KeySym_Armenian_ben             = 0x14b5;
constexpr KeySym KeySym_Armenian_GIM             = 0x14b6;
constexpr KeySym KeySym_Armenian_gim             = 0x14b7;
constexpr KeySym KeySym_Armenian_DA              = 0x14b8;
constexpr KeySym KeySym_Armenian_da              = 0x14b9;
constexpr KeySym KeySym_Armenian_YECH            = 0x14ba;
constexpr KeySym KeySym_Armenian_yech            = 0x14bb;
constexpr KeySym KeySym_Armenian_ZA              = 0x14bc;
constexpr KeySym KeySym_Armenian_za              = 0x14bd;
constexpr KeySym KeySym_Armenian_E               = 0x14be;
constexpr KeySym KeySym_Armenian_e               = 0x14bf;
constexpr KeySym KeySym_Armenian_AT              = 0x14c0;
constexpr KeySym KeySym_Armenian_at              = 0x14c1;
constexpr KeySym KeySym_Armenian_TO              = 0x14c2;
constexpr KeySym KeySym_Armenian_to              = 0x14c3;
constexpr KeySym KeySym_Armenian_ZHE             = 0x14c4;
constexpr KeySym KeySym_Armenian_zhe             = 0x14c5;
constexpr KeySym KeySym_Armenian_INI             = 0x14c6;
constexpr KeySym KeySym_Armenian_ini             = 0x14c7;
constexpr KeySym KeySym_Armenian_LYUN            = 0x14c8;
constexpr KeySym KeySym_Armenian_lyun            = 0x14c9;
constexpr KeySym KeySym_Armenian_KHE             = 0x14ca;
constexpr KeySym KeySym_Armenian_khe             = 0x14cb;
constexpr KeySym KeySym_Armenian_TSA             = 0x14cc;
constexpr KeySym KeySym_Armenian_tsa             = 0x14cd;
constexpr KeySym KeySym_Armenian_KEN             = 0x14ce;
constexpr KeySym KeySym_Armenian_ken             = 0x14cf;
constexpr KeySym KeySym_Armenian_HO              = 0x14d0;
constexpr KeySym KeySym_Armenian_ho              = 0x14d1;
constexpr KeySym KeySym_Armenian_DZA             = 0x14d2;
constexpr KeySym KeySym_Armenian_dza             = 0x14d3;
constexpr KeySym KeySym_Armenian_GHAT            = 0x14d4;
constexpr KeySym KeySym_Armenian_ghat            = 0x14d5;
constexpr KeySym KeySym_Armenian_TCHE            = 0x14d6;
constexpr KeySym KeySym_Armenian_tche            = 0x14d7;
constexpr KeySym KeySym_Armenian_MEN             = 0x14d8;
constexpr KeySym KeySym_Armenian_men             = 0x14d9;
constexpr KeySym KeySym_Armenian_HI              = 0x14da;
constexpr KeySym KeySym_Armenian_hi              = 0x14db;
constexpr KeySym KeySym_Armenian_NU              = 0x14dc;
constexpr KeySym KeySym_Armenian_nu              = 0x14dd;
constexpr KeySym KeySym_Armenian_SHA             = 0x14de;
constexpr KeySym KeySym_Armenian_sha             = 0x14df;
constexpr KeySym KeySym_Armenian_VO              = 0x14e0;
constexpr KeySym KeySym_Armenian_vo              = 0x14e1;
constexpr KeySym KeySym_Armenian_CHA             = 0x14e2;
constexpr KeySym KeySym_Armenian_cha             = 0x14e3;
constexpr KeySym KeySym_Armenian_PE              = 0x14e4;
constexpr KeySym KeySym_Armenian_pe              = 0x14e5;
constexpr KeySym KeySym_Armenian_JE              = 0x14e6;
constexpr KeySym KeySym_Armenian_je              = 0x14e7;
constexpr KeySym KeySym_Armenian_RA              = 0x14e8;
constexpr KeySym KeySym_Armenian_ra              = 0x14e9;
constexpr KeySym KeySym_Armenian_SE              = 0x14ea;
constexpr KeySym KeySym_Armenian_se              = 0x14eb;
constexpr KeySym KeySym_Armenian_VEV             = 0x14ec;
constexpr KeySym KeySym_Armenian_vev             = 0x14ed;
constexpr KeySym KeySym_Armenian_TYUN            = 0x14ee;
constexpr KeySym KeySym_Armenian_tyun            = 0x14ef;
constexpr KeySym KeySym_Armenian_RE              = 0x14f0;
constexpr KeySym KeySym_Armenian_re              = 0x14f1;
constexpr KeySym KeySym_Armenian_TSO             = 0x14f2;
constexpr KeySym KeySym_Armenian_tso             = 0x14f3;
constexpr KeySym KeySym_Armenian_VYUN            = 0x14f4;
constexpr KeySym KeySym_Armenian_vyun            = 0x14f5;
constexpr KeySym KeySym_Armenian_PYUR            = 0x14f6;
constexpr KeySym KeySym_Armenian_pyur            = 0x14f7;
constexpr KeySym KeySym_Armenian_KE              = 0x14f8;
constexpr KeySym KeySym_Armenian_ke              = 0x14f9;
constexpr KeySym KeySym_Armenian_O               = 0x14fa;
constexpr KeySym KeySym_Armenian_o               = 0x14fb;
constexpr KeySym KeySym_Armenian_FE              = 0x14fc;
constexpr KeySym KeySym_Armenian_fe              = 0x14fd;
constexpr KeySym KeySym_Armenian_apostrophe      = 0x14fe;
constexpr KeySym KeySym_Armenian_section_sign    = 0x14ff;
/// @}

/// @{ Georgian.
///
/// Byte 3 = 0x15
constexpr KeySym KeySym_Georgian_an      = 0x15d0;
constexpr KeySym KeySym_Georgian_ban     = 0x15d1;
constexpr KeySym KeySym_Georgian_gan     = 0x15d2;
constexpr KeySym KeySym_Georgian_don     = 0x15d3;
constexpr KeySym KeySym_Georgian_en      = 0x15d4;
constexpr KeySym KeySym_Georgian_vin     = 0x15d5;
constexpr KeySym KeySym_Georgian_zen     = 0x15d6;
constexpr KeySym KeySym_Georgian_tan     = 0x15d7;
constexpr KeySym KeySym_Georgian_in      = 0x15d8;
constexpr KeySym KeySym_Georgian_kan     = 0x15d9;
constexpr KeySym KeySym_Georgian_las     = 0x15da;
constexpr KeySym KeySym_Georgian_man     = 0x15db;
constexpr KeySym KeySym_Georgian_nar     = 0x15dc;
constexpr KeySym KeySym_Georgian_on      = 0x15dd;
constexpr KeySym KeySym_Georgian_par     = 0x15de;
constexpr KeySym KeySym_Georgian_zhar    = 0x15df;
constexpr KeySym KeySym_Georgian_rae     = 0x15e0;
constexpr KeySym KeySym_Georgian_san     = 0x15e1;
constexpr KeySym KeySym_Georgian_tar     = 0x15e2;
constexpr KeySym KeySym_Georgian_un      = 0x15e3;
constexpr KeySym KeySym_Georgian_phar    = 0x15e4;
constexpr KeySym KeySym_Georgian_khar    = 0x15e5;
constexpr KeySym KeySym_Georgian_ghan    = 0x15e6;
constexpr KeySym KeySym_Georgian_qar     = 0x15e7;
constexpr KeySym KeySym_Georgian_shin    = 0x15e8;
constexpr KeySym KeySym_Georgian_chin    = 0x15e9;
constexpr KeySym KeySym_Georgian_can     = 0x15ea;
constexpr KeySym KeySym_Georgian_jil     = 0x15eb;
constexpr KeySym KeySym_Georgian_cil     = 0x15ec;
constexpr KeySym KeySym_Georgian_char    = 0x15ed;
constexpr KeySym KeySym_Georgian_xan     = 0x15ee;
constexpr KeySym KeySym_Georgian_jhan    = 0x15ef;
constexpr KeySym KeySym_Georgian_hae     = 0x15f0;
constexpr KeySym KeySym_Georgian_he      = 0x15f1;
constexpr KeySym KeySym_Georgian_hie     = 0x15f2;
constexpr KeySym KeySym_Georgian_we      = 0x15f3;
constexpr KeySym KeySym_Georgian_har     = 0x15f4;
constexpr KeySym KeySym_Georgian_hoe     = 0x15f5;
constexpr KeySym KeySym_Georgian_fi      = 0x15f6;
/// @}

/// @{ Azeri (and other Turkic or Caucasian languages of ex-USSR).
///
/// Byte 3 = 0x16

// latin
constexpr KeySym KeySym_Ccedillaabovedot = 0x16a2;
constexpr KeySym KeySym_Xabovedot        = 0x16a3;
constexpr KeySym KeySym_Qabovedot        = 0x16a5;
constexpr KeySym KeySym_Ibreve           = 0x16a6;
constexpr KeySym KeySym_IE               = 0x16a7;
constexpr KeySym KeySym_UO               = 0x16a8;
constexpr KeySym KeySym_Zstroke          = 0x16a9;
constexpr KeySym KeySym_Gcaron           = 0x16aa;
constexpr KeySym KeySym_Obarred          = 0x16af;
constexpr KeySym KeySym_ccedillaabovedot = 0x16b2;
constexpr KeySym KeySym_xabovedot        = 0x16b3;
constexpr KeySym KeySym_Ocaron           = 0x16b4;
constexpr KeySym KeySym_qabovedot        = 0x16b5;
constexpr KeySym KeySym_ibreve           = 0x16b6;
constexpr KeySym KeySym_ie               = 0x16b7;
constexpr KeySym KeySym_uo               = 0x16b8;
constexpr KeySym KeySym_zstroke          = 0x16b9;
constexpr KeySym KeySym_gcaron           = 0x16ba;
constexpr KeySym KeySym_ocaron           = 0x16bd;
constexpr KeySym KeySym_obarred          = 0x16bf;
constexpr KeySym KeySym_SCHWA            = 0x16c6;
constexpr KeySym KeySym_schwa            = 0x16f6;
// Those are not really Caucasus, but I put them here for now
// For Inupiak
constexpr KeySym KeySym_Lbelowdot        = 0x16d1;
constexpr KeySym KeySym_Lstrokebelowdot  = 0x16d2;
constexpr KeySym KeySym_lbelowdot        = 0x16e1;
constexpr KeySym KeySym_lstrokebelowdot  = 0x16e2;
// For Guarani
constexpr KeySym KeySym_Gtilde           = 0x16d3;
constexpr KeySym KeySym_gtilde           = 0x16e3;
/// @}

/// @{ Vietnamese.
///
/// Byte 3 = 0x1e
constexpr KeySym KeySym_Abelowdot           = 0x1ea0;
constexpr KeySym KeySym_abelowdot           = 0x1ea1;
constexpr KeySym KeySym_Ahook               = 0x1ea2;
constexpr KeySym KeySym_ahook               = 0x1ea3;
constexpr KeySym KeySym_Acircumflexacute    = 0x1ea4;
constexpr KeySym KeySym_acircumflexacute    = 0x1ea5;
constexpr KeySym KeySym_Acircumflexgrave    = 0x1ea6;
constexpr KeySym KeySym_acircumflexgrave    = 0x1ea7;
constexpr KeySym KeySym_Acircumflexhook     = 0x1ea8;
constexpr KeySym KeySym_acircumflexhook     = 0x1ea9;
constexpr KeySym KeySym_Acircumflextilde    = 0x1eaa;
constexpr KeySym KeySym_acircumflextilde    = 0x1eab;
constexpr KeySym KeySym_Acircumflexbelowdot = 0x1eac;
constexpr KeySym KeySym_acircumflexbelowdot = 0x1ead;
constexpr KeySym KeySym_Abreveacute         = 0x1eae;
constexpr KeySym KeySym_abreveacute         = 0x1eaf;
constexpr KeySym KeySym_Abrevegrave         = 0x1eb0;
constexpr KeySym KeySym_abrevegrave         = 0x1eb1;
constexpr KeySym KeySym_Abrevehook          = 0x1eb2;
constexpr KeySym KeySym_abrevehook          = 0x1eb3;
constexpr KeySym KeySym_Abrevetilde         = 0x1eb4;
constexpr KeySym KeySym_abrevetilde         = 0x1eb5;
constexpr KeySym KeySym_Abrevebelowdot      = 0x1eb6;
constexpr KeySym KeySym_abrevebelowdot      = 0x1eb7;
constexpr KeySym KeySym_Ebelowdot           = 0x1eb8;
constexpr KeySym KeySym_ebelowdot           = 0x1eb9;
constexpr KeySym KeySym_Ehook               = 0x1eba;
constexpr KeySym KeySym_ehook               = 0x1ebb;
constexpr KeySym KeySym_Etilde              = 0x1ebc;
constexpr KeySym KeySym_etilde              = 0x1ebd;
constexpr KeySym KeySym_Ecircumflexacute    = 0x1ebe;
constexpr KeySym KeySym_ecircumflexacute    = 0x1ebf;
constexpr KeySym KeySym_Ecircumflexgrave    = 0x1ec0;
constexpr KeySym KeySym_ecircumflexgrave    = 0x1ec1;
constexpr KeySym KeySym_Ecircumflexhook     = 0x1ec2;
constexpr KeySym KeySym_ecircumflexhook     = 0x1ec3;
constexpr KeySym KeySym_Ecircumflextilde    = 0x1ec4;
constexpr KeySym KeySym_ecircumflextilde    = 0x1ec5;
constexpr KeySym KeySym_Ecircumflexbelowdot = 0x1ec6;
constexpr KeySym KeySym_ecircumflexbelowdot = 0x1ec7;
constexpr KeySym KeySym_Ihook               = 0x1ec8;
constexpr KeySym KeySym_ihook               = 0x1ec9;
constexpr KeySym KeySym_Ibelowdot           = 0x1eca;
constexpr KeySym KeySym_ibelowdot           = 0x1ecb;
constexpr KeySym KeySym_Obelowdot           = 0x1ecc;
constexpr KeySym KeySym_obelowdot           = 0x1ecd;
constexpr KeySym KeySym_Ohook               = 0x1ece;
constexpr KeySym KeySym_ohook               = 0x1ecf;
constexpr KeySym KeySym_Ocircumflexacute    = 0x1ed0;
constexpr KeySym KeySym_ocircumflexacute    = 0x1ed1;
constexpr KeySym KeySym_Ocircumflexgrave    = 0x1ed2;
constexpr KeySym KeySym_ocircumflexgrave    = 0x1ed3;
constexpr KeySym KeySym_Ocircumflexhook     = 0x1ed4;
constexpr KeySym KeySym_ocircumflexhook     = 0x1ed5;
constexpr KeySym KeySym_Ocircumflextilde    = 0x1ed6;
constexpr KeySym KeySym_ocircumflextilde    = 0x1ed7;
constexpr KeySym KeySym_Ocircumflexbelowdot = 0x1ed8;
constexpr KeySym KeySym_ocircumflexbelowdot = 0x1ed9;
constexpr KeySym KeySym_Ohornacute          = 0x1eda;
constexpr KeySym KeySym_ohornacute          = 0x1edb;
constexpr KeySym KeySym_Ohorngrave          = 0x1edc;
constexpr KeySym KeySym_ohorngrave          = 0x1edd;
constexpr KeySym KeySym_Ohornhook           = 0x1ede;
constexpr KeySym KeySym_ohornhook           = 0x1edf;
constexpr KeySym KeySym_Ohorntilde          = 0x1ee0;
constexpr KeySym KeySym_ohorntilde          = 0x1ee1;
constexpr KeySym KeySym_Ohornbelowdot       = 0x1ee2;
constexpr KeySym KeySym_ohornbelowdot       = 0x1ee3;
constexpr KeySym KeySym_Ubelowdot           = 0x1ee4;
constexpr KeySym KeySym_ubelowdot           = 0x1ee5;
constexpr KeySym KeySym_Uhook               = 0x1ee6;
constexpr KeySym KeySym_uhook               = 0x1ee7;
constexpr KeySym KeySym_Uhornacute          = 0x1ee8;
constexpr KeySym KeySym_uhornacute          = 0x1ee9;
constexpr KeySym KeySym_Uhorngrave          = 0x1eea;
constexpr KeySym KeySym_uhorngrave          = 0x1eeb;
constexpr KeySym KeySym_Uhornhook           = 0x1eec;
constexpr KeySym KeySym_uhornhook           = 0x1eed;
constexpr KeySym KeySym_Uhorntilde          = 0x1eee;
constexpr KeySym KeySym_uhorntilde          = 0x1eef;
constexpr KeySym KeySym_Uhornbelowdot       = 0x1ef0;
constexpr KeySym KeySym_uhornbelowdot       = 0x1ef1;
constexpr KeySym KeySym_Ybelowdot           = 0x1ef4;
constexpr KeySym KeySym_ybelowdot           = 0x1ef5;
constexpr KeySym KeySym_Yhook               = 0x1ef6;
constexpr KeySym KeySym_yhook               = 0x1ef7;
constexpr KeySym KeySym_Ytilde              = 0x1ef8;
constexpr KeySym KeySym_ytilde              = 0x1ef9;
constexpr KeySym KeySym_Ohorn               = 0x1efa; // U+01a0
constexpr KeySym KeySym_ohorn               = 0x1efb; // U+01a1
constexpr KeySym KeySym_Uhorn               = 0x1efc; // U+01af
constexpr KeySym KeySym_uhorn               = 0x1efd; // U+01b0
/// @}

constexpr KeySym KeySym_combining_tilde     = 0x1e9f; // U+0303
constexpr KeySym KeySym_combining_grave     = 0x1ef2; // U+0300
constexpr KeySym KeySym_combining_acute     = 0x1ef3; // U+0301
constexpr KeySym KeySym_combining_hook      = 0x1efe; // U+0309
constexpr KeySym KeySym_combining_belowdot  = 0x1eff; // U+0323

/// @{ Currency.
constexpr KeySym KeySym_EcuSign       = 0x20a0;
constexpr KeySym KeySym_ColonSign     = 0x20a1;
constexpr KeySym KeySym_CruzeiroSign  = 0x20a2;
constexpr KeySym KeySym_FFrancSign    = 0x20a3;
constexpr KeySym KeySym_LiraSign      = 0x20a4;
constexpr KeySym KeySym_MillSign      = 0x20a5;
constexpr KeySym KeySym_NairaSign     = 0x20a6;
constexpr KeySym KeySym_PesetaSign    = 0x20a7;
constexpr KeySym KeySym_RupeeSign     = 0x20a8;
constexpr KeySym KeySym_WonSign       = 0x20a9;
constexpr KeySym KeySym_NewSheqelSign = 0x20aa;
constexpr KeySym KeySym_DongSign      = 0x20ab;
constexpr KeySym KeySym_EuroSign      = 0x20ac;
/// @}

} // namespace display
} // namespace archon

#endif // ARCHON_DISPLAY_KEYSYMS_HPP
