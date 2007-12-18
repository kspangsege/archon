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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_DISPLAY_KEYSYMS_HPP
#define ARCHON_DISPLAY_KEYSYMS_HPP

namespace Archon
{
  namespace Display
  {
    /**
     * A KeySym identifies a symbol (or pictogram) on a key (on the keyboard).
     *
     * Although the KeySym values are chosen to match those defined by
     * Xlib, you must not rely on any such correspondance.
     */
    typedef int KeySym;

    KeySym const KeySym_None = 0x0;

    /*
     * TTY Functions, chosen to match ASCII.
     */
    KeySym const KeySym_BackSpace   = 0xFF08; // back space, back char
    KeySym const KeySym_Tab         = 0xFF09;
    KeySym const KeySym_Linefeed    = 0xFF0A; // Linefeed, LF
    KeySym const KeySym_Clear       = 0xFF0B;
    KeySym const KeySym_Return      = 0xFF0D; // Return, enter
    KeySym const KeySym_Pause       = 0xFF13; // Pause, hold
    KeySym const KeySym_Scroll_Lock = 0xFF14;
    KeySym const KeySym_Sys_Req     = 0xFF15;
    KeySym const KeySym_Escape      = 0xFF1B;
    KeySym const KeySym_Delete      = 0xFFFF; // Delete, rubout


    /*
     * International & multi-key character composition
     */
    KeySym const KeySym_Multi_key         = 0xFF20; // Multi-key character compose
    KeySym const KeySym_Codeinput         = 0xFF37;
    KeySym const KeySym_SingleCandidate   = 0xFF3C;
    KeySym const KeySym_MultipleCandidate = 0xFF3D;
    KeySym const KeySym_PreviousCandidate = 0xFF3E;


    /*
     * Japanese keyboard support
     */
    KeySym const KeySym_Kanji             = 0xFF21; // Kanji, Kanji convert
    KeySym const KeySym_Muhenkan          = 0xFF22; // Cancel Conversion
    KeySym const KeySym_Henkan_Mode       = 0xFF23; // Start/Stop Conversion
    KeySym const KeySym_Henkan            = 0xFF23; // Alias for Henkan_Mode
    KeySym const KeySym_Romaji            = 0xFF24; // to Romaji
    KeySym const KeySym_Hiragana          = 0xFF25; // to Hiragana
    KeySym const KeySym_Katakana          = 0xFF26; // to Katakana
    KeySym const KeySym_Hiragana_Katakana = 0xFF27; // Hiragana/Katakana toggle
    KeySym const KeySym_Zenkaku           = 0xFF28; // to Zenkaku
    KeySym const KeySym_Hankaku           = 0xFF29; // to Hankaku
    KeySym const KeySym_Zenkaku_Hankaku   = 0xFF2A; // Zenkaku/Hankaku toggle
    KeySym const KeySym_Touroku           = 0xFF2B; // Add to Dictionary
    KeySym const KeySym_Massyo            = 0xFF2C; // Delete from Dictionary
    KeySym const KeySym_Kana_Lock         = 0xFF2D; // Kana Lock
    KeySym const KeySym_Kana_Shift        = 0xFF2E; // Kana Shift
    KeySym const KeySym_Eisu_Shift        = 0xFF2F; // Alphanumeric Shift
    KeySym const KeySym_Eisu_toggle       = 0xFF30; // Alphanumeric toggle
    KeySym const KeySym_Kanji_Bangou      = 0xFF37; // Codeinput (Alias for Codeinput)
    KeySym const KeySym_Zen_Koho          = 0xFF3D; // Multiple/All Candidate(s) (Alias for MultipleCandidate)
    KeySym const KeySym_Mae_Koho          = 0xFF3E; // Alias for PreviousCandidate

    // 0xFF31 thru 0xFF3F are under Korean


    /*
     * Cursor control & motion
     */
    KeySym const KeySym_Home      = 0xFF50;
    KeySym const KeySym_Left      = 0xFF51; // Move left, left arrow
    KeySym const KeySym_Up        = 0xFF52; // Move up, up arrow
    KeySym const KeySym_Right     = 0xFF53; // Move right, right arrow
    KeySym const KeySym_Down      = 0xFF54; // Move down, down arrow
    KeySym const KeySym_Prior     = 0xFF55; // Prior, previous
    KeySym const KeySym_Page_Up   = 0xFF55; // Alias for Prior
    KeySym const KeySym_Next      = 0xFF56; // Next
    KeySym const KeySym_Page_Down = 0xFF56; // Alias for Next
    KeySym const KeySym_End       = 0xFF57; // EOL
    KeySym const KeySym_Begin     = 0xFF58; // BOL


    /*
     * Misc Functions
     */
    KeySym const KeySym_Select        = 0xFF60; // Select, mark
    KeySym const KeySym_Print         = 0xFF61;
    KeySym const KeySym_Execute       = 0xFF62; // Execute, run, do
    KeySym const KeySym_Insert        = 0xFF63; // Insert, insert here
    KeySym const KeySym_Undo          = 0xFF65; // Undo, oops
    KeySym const KeySym_Redo          = 0xFF66; // redo, again
    KeySym const KeySym_Menu          = 0xFF67;
    KeySym const KeySym_Find          = 0xFF68; // Find, search
    KeySym const KeySym_Cancel        = 0xFF69; // Cancel, stop, abort, exit
    KeySym const KeySym_Help          = 0xFF6A; // Help
    KeySym const KeySym_Break         = 0xFF6B;
    KeySym const KeySym_Mode_switch   = 0xFF7E; // Character set switch
    KeySym const KeySym_script_switch = 0xFF7E; // Alias for mode_switch
    KeySym const KeySym_Num_Lock      = 0xFF7F;


    /*
     * Keypad Functions, keypad numbers cleverly chosen to map to ascii
     */
    KeySym const KeySym_KP_Space     = 0xFF80; // space
    KeySym const KeySym_KP_Tab       = 0xFF89;
    KeySym const KeySym_KP_Enter     = 0xFF8D; // enter
    KeySym const KeySym_KP_F1        = 0xFF91; // PF1, KP_A, ...
    KeySym const KeySym_KP_F2        = 0xFF92;
    KeySym const KeySym_KP_F3        = 0xFF93;
    KeySym const KeySym_KP_F4        = 0xFF94;
    KeySym const KeySym_KP_Home      = 0xFF95;
    KeySym const KeySym_KP_Left      = 0xFF96;
    KeySym const KeySym_KP_Up        = 0xFF97;
    KeySym const KeySym_KP_Right     = 0xFF98;
    KeySym const KeySym_KP_Down      = 0xFF99;
    KeySym const KeySym_KP_Prior     = 0xFF9A;
    KeySym const KeySym_KP_Page_Up   = 0xFF9A; // Alias for KP_Prior
    KeySym const KeySym_KP_Next      = 0xFF9B;
    KeySym const KeySym_KP_Page_Down = 0xFF9B; // Alias for KP_Next
    KeySym const KeySym_KP_End       = 0xFF9C;
    KeySym const KeySym_KP_Begin     = 0xFF9D;
    KeySym const KeySym_KP_Insert    = 0xFF9E;
    KeySym const KeySym_KP_Delete    = 0xFF9F;
    KeySym const KeySym_KP_Equal     = 0xFFBD; // equals
    KeySym const KeySym_KP_Multiply  = 0xFFAA;
    KeySym const KeySym_KP_Add       = 0xFFAB;
    KeySym const KeySym_KP_Separator = 0xFFAC; // separator, often comma
    KeySym const KeySym_KP_Subtract  = 0xFFAD;
    KeySym const KeySym_KP_Decimal   = 0xFFAE;
    KeySym const KeySym_KP_Divide    = 0xFFAF;

    KeySym const KeySym_KP_0   = 0xFFB0;
    KeySym const KeySym_KP_1   = 0xFFB1;
    KeySym const KeySym_KP_2   = 0xFFB2;
    KeySym const KeySym_KP_3   = 0xFFB3;
    KeySym const KeySym_KP_4   = 0xFFB4;
    KeySym const KeySym_KP_5   = 0xFFB5;
    KeySym const KeySym_KP_6   = 0xFFB6;
    KeySym const KeySym_KP_7   = 0xFFB7;
    KeySym const KeySym_KP_8   = 0xFFB8;
    KeySym const KeySym_KP_9   = 0xFFB9;


    /*
     * Auxilliary Functions; note the duplicate definitions for left and right
     * function keys;  Sun keyboards and a few other manufactures have such
     * function key groups on the left and/or right sides of the keyboard.
     * We've not found a keyboard with more than 35 function keys total.
     */
    KeySym const KeySym_F1   = 0xFFBE;
    KeySym const KeySym_F2   = 0xFFBF;
    KeySym const KeySym_F3   = 0xFFC0;
    KeySym const KeySym_F4   = 0xFFC1;
    KeySym const KeySym_F5   = 0xFFC2;
    KeySym const KeySym_F6   = 0xFFC3;
    KeySym const KeySym_F7   = 0xFFC4;
    KeySym const KeySym_F8   = 0xFFC5;
    KeySym const KeySym_F9   = 0xFFC6;
    KeySym const KeySym_F10  = 0xFFC7;
    KeySym const KeySym_F11  = 0xFFC8;
    KeySym const KeySym_L1   = 0xFFC8; // Alias for F11
    KeySym const KeySym_F12  = 0xFFC9;
    KeySym const KeySym_L2   = 0xFFC9; // Alias for F12
    KeySym const KeySym_F13  = 0xFFCA;
    KeySym const KeySym_L3   = 0xFFCA; // Alias for F13
    KeySym const KeySym_F14  = 0xFFCB;
    KeySym const KeySym_L4   = 0xFFCB; // Alias for F14
    KeySym const KeySym_F15  = 0xFFCC;
    KeySym const KeySym_L5   = 0xFFCC; // Alias for F15
    KeySym const KeySym_F16  = 0xFFCD;
    KeySym const KeySym_L6   = 0xFFCD; // Alias for F16
    KeySym const KeySym_F17  = 0xFFCE;
    KeySym const KeySym_L7   = 0xFFCE; // Alias for F17
    KeySym const KeySym_F18  = 0xFFCF;
    KeySym const KeySym_L8   = 0xFFCF; // Alias for F18
    KeySym const KeySym_F19  = 0xFFD0;
    KeySym const KeySym_L9   = 0xFFD0; // Alias for F19
    KeySym const KeySym_F20  = 0xFFD1;
    KeySym const KeySym_L10  = 0xFFD1; // Alias for F20
    KeySym const KeySym_F21  = 0xFFD2;
    KeySym const KeySym_R1   = 0xFFD2; // Alias for F21
    KeySym const KeySym_F22  = 0xFFD3;
    KeySym const KeySym_R2   = 0xFFD3; // Alias for F22
    KeySym const KeySym_F23  = 0xFFD4;
    KeySym const KeySym_R3   = 0xFFD4; // Alias for F23
    KeySym const KeySym_F24  = 0xFFD5;
    KeySym const KeySym_R4   = 0xFFD5; // Alias for F24
    KeySym const KeySym_F25  = 0xFFD6;
    KeySym const KeySym_R5   = 0xFFD6; // Alias for F25
    KeySym const KeySym_F26  = 0xFFD7;
    KeySym const KeySym_R6   = 0xFFD7; // Alias for F26
    KeySym const KeySym_F27  = 0xFFD8;
    KeySym const KeySym_R7   = 0xFFD8; // Alias for F27
    KeySym const KeySym_F28  = 0xFFD9;
    KeySym const KeySym_R8   = 0xFFD9; // Alias for F28
    KeySym const KeySym_F29  = 0xFFDA;
    KeySym const KeySym_R9   = 0xFFDA; // Alias for F29
    KeySym const KeySym_F30  = 0xFFDB;
    KeySym const KeySym_R10  = 0xFFDB; // Alias for F30
    KeySym const KeySym_F31  = 0xFFDC;
    KeySym const KeySym_R11  = 0xFFDC; // Alias for F31
    KeySym const KeySym_F32  = 0xFFDD;
    KeySym const KeySym_R12  = 0xFFDD; // Alias for F32
    KeySym const KeySym_F33  = 0xFFDE;
    KeySym const KeySym_R13  = 0xFFDE; // Alias for F33
    KeySym const KeySym_F34  = 0xFFDF;
    KeySym const KeySym_R14  = 0xFFDF; // Alias for F34
    KeySym const KeySym_F35  = 0xFFE0;
    KeySym const KeySym_R15  = 0xFFE0; // Alias for F35


    /*
     * Modifiers
     */
    KeySym const KeySym_Shift_L    = 0xFFE1; // Left shift
    KeySym const KeySym_Shift_R    = 0xFFE2; // Right shift
    KeySym const KeySym_Control_L  = 0xFFE3; // Left control
    KeySym const KeySym_Control_R  = 0xFFE4; // Right control
    KeySym const KeySym_Caps_Lock  = 0xFFE5; // Caps lock
    KeySym const KeySym_Shift_Lock = 0xFFE6; // Shift lock

    KeySym const KeySym_Meta_L     = 0xFFE7; // Left meta
    KeySym const KeySym_Meta_R     = 0xFFE8; // Right meta
    KeySym const KeySym_Alt_L      = 0xFFE9; // Left alt
    KeySym const KeySym_Alt_R      = 0xFFEA; // Right alt
    KeySym const KeySym_Super_L    = 0xFFEB; // Left super
    KeySym const KeySym_Super_R    = 0xFFEC; // Right super
    KeySym const KeySym_Hyper_L    = 0xFFED; // Left hyper
    KeySym const KeySym_Hyper_R    = 0xFFEE; // Right hyper


    /*
     * ISO 9995 Function and Modifier Keys
     * Byte 3 = 0xFE
     */
    KeySym const KeySym_ISO_Lock             = 0xFE01;
    KeySym const KeySym_ISO_Level2_Latch     = 0xFE02;
    KeySym const KeySym_ISO_Level3_Shift     = 0xFE03;
    KeySym const KeySym_ISO_Level3_Latch     = 0xFE04;
    KeySym const KeySym_ISO_Level3_Lock      = 0xFE05;
    KeySym const KeySym_ISO_Group_Shift      = 0xFF7E; // Alias for mode_switch
    KeySym const KeySym_ISO_Group_Latch      = 0xFE06;
    KeySym const KeySym_ISO_Group_Lock       = 0xFE07;
    KeySym const KeySym_ISO_Next_Group       = 0xFE08;
    KeySym const KeySym_ISO_Next_Group_Lock  = 0xFE09;
    KeySym const KeySym_ISO_Prev_Group       = 0xFE0A;
    KeySym const KeySym_ISO_Prev_Group_Lock  = 0xFE0B;
    KeySym const KeySym_ISO_First_Group      = 0xFE0C;
    KeySym const KeySym_ISO_First_Group_Lock = 0xFE0D;
    KeySym const KeySym_ISO_Last_Group       = 0xFE0E;
    KeySym const KeySym_ISO_Last_Group_Lock  = 0xFE0F;

    KeySym const KeySym_ISO_Left_Tab                = 0xFE20;
    KeySym const KeySym_ISO_Move_Line_Up            = 0xFE21;
    KeySym const KeySym_ISO_Move_Line_Down          = 0xFE22;
    KeySym const KeySym_ISO_Partial_Line_Up         = 0xFE23;
    KeySym const KeySym_ISO_Partial_Line_Down       = 0xFE24;
    KeySym const KeySym_ISO_Partial_Space_Left      = 0xFE25;
    KeySym const KeySym_ISO_Partial_Space_Right     = 0xFE26;
    KeySym const KeySym_ISO_Set_Margin_Left         = 0xFE27;
    KeySym const KeySym_ISO_Set_Margin_Right        = 0xFE28;
    KeySym const KeySym_ISO_Release_Margin_Left     = 0xFE29;
    KeySym const KeySym_ISO_Release_Margin_Right    = 0xFE2A;
    KeySym const KeySym_ISO_Release_Both_Margins    = 0xFE2B;
    KeySym const KeySym_ISO_Fast_Cursor_Left        = 0xFE2C;
    KeySym const KeySym_ISO_Fast_Cursor_Right       = 0xFE2D;
    KeySym const KeySym_ISO_Fast_Cursor_Up          = 0xFE2E;
    KeySym const KeySym_ISO_Fast_Cursor_Down        = 0xFE2F;
    KeySym const KeySym_ISO_Continuous_Underline    = 0xFE30;
    KeySym const KeySym_ISO_Discontinuous_Underline = 0xFE31;
    KeySym const KeySym_ISO_Emphasize               = 0xFE32;
    KeySym const KeySym_ISO_Center_Object           = 0xFE33;
    KeySym const KeySym_ISO_Enter                   = 0xFE34;

    KeySym const KeySym_dead_grave            = 0xFE50;
    KeySym const KeySym_dead_acute            = 0xFE51;
    KeySym const KeySym_dead_circumflex       = 0xFE52;
    KeySym const KeySym_dead_tilde            = 0xFE53;
    KeySym const KeySym_dead_macron           = 0xFE54;
    KeySym const KeySym_dead_breve            = 0xFE55;
    KeySym const KeySym_dead_abovedot         = 0xFE56;
    KeySym const KeySym_dead_diaeresis        = 0xFE57;
    KeySym const KeySym_dead_abovering        = 0xFE58;
    KeySym const KeySym_dead_doubleacute      = 0xFE59;
    KeySym const KeySym_dead_caron            = 0xFE5A;
    KeySym const KeySym_dead_cedilla          = 0xFE5B;
    KeySym const KeySym_dead_ogonek           = 0xFE5C;
    KeySym const KeySym_dead_iota             = 0xFE5D;
    KeySym const KeySym_dead_voiced_sound     = 0xFE5E;
    KeySym const KeySym_dead_semivoiced_sound = 0xFE5F;
    KeySym const KeySym_dead_belowdot         = 0xFE60;
    KeySym const KeySym_dead_hook             = 0xFE61;
    KeySym const KeySym_dead_horn             = 0xFE62;

    KeySym const KeySym_First_Virtual_Screen  = 0xFED0;
    KeySym const KeySym_Prev_Virtual_Screen   = 0xFED1;
    KeySym const KeySym_Next_Virtual_Screen   = 0xFED2;
    KeySym const KeySym_Last_Virtual_Screen   = 0xFED4;
    KeySym const KeySym_Terminate_Server      = 0xFED5;

    KeySym const KeySym_AccessX_Enable          = 0xFE70;
    KeySym const KeySym_AccessX_Feedback_Enable = 0xFE71;
    KeySym const KeySym_RepeatKeys_Enable       = 0xFE72;
    KeySym const KeySym_SlowKeys_Enable         = 0xFE73;
    KeySym const KeySym_BounceKeys_Enable       = 0xFE74;
    KeySym const KeySym_StickyKeys_Enable       = 0xFE75;
    KeySym const KeySym_MouseKeys_Enable        = 0xFE76;
    KeySym const KeySym_MouseKeys_Accel_Enable  = 0xFE77;
    KeySym const KeySym_Overlay1_Enable         = 0xFE78;
    KeySym const KeySym_Overlay2_Enable         = 0xFE79;
    KeySym const KeySym_AudibleBell_Enable      = 0xFE7A;

    KeySym const KeySym_Pointer_Left          = 0xFEE0;
    KeySym const KeySym_Pointer_Right         = 0xFEE1;
    KeySym const KeySym_Pointer_Up            = 0xFEE2;
    KeySym const KeySym_Pointer_Down          = 0xFEE3;
    KeySym const KeySym_Pointer_UpLeft        = 0xFEE4;
    KeySym const KeySym_Pointer_UpRight       = 0xFEE5;
    KeySym const KeySym_Pointer_DownLeft      = 0xFEE6;
    KeySym const KeySym_Pointer_DownRight     = 0xFEE7;
    KeySym const KeySym_Pointer_Button_Dflt   = 0xFEE8;
    KeySym const KeySym_Pointer_Button1       = 0xFEE9;
    KeySym const KeySym_Pointer_Button2       = 0xFEEA;
    KeySym const KeySym_Pointer_Button3       = 0xFEEB;
    KeySym const KeySym_Pointer_Button4       = 0xFEEC;
    KeySym const KeySym_Pointer_Button5       = 0xFEED;
    KeySym const KeySym_Pointer_DblClick_Dflt = 0xFEEE;
    KeySym const KeySym_Pointer_DblClick1     = 0xFEEF;
    KeySym const KeySym_Pointer_DblClick2     = 0xFEF0;
    KeySym const KeySym_Pointer_DblClick3     = 0xFEF1;
    KeySym const KeySym_Pointer_DblClick4     = 0xFEF2;
    KeySym const KeySym_Pointer_DblClick5     = 0xFEF3;
    KeySym const KeySym_Pointer_Drag_Dflt     = 0xFEF4;
    KeySym const KeySym_Pointer_Drag1         = 0xFEF5;
    KeySym const KeySym_Pointer_Drag2         = 0xFEF6;
    KeySym const KeySym_Pointer_Drag3         = 0xFEF7;
    KeySym const KeySym_Pointer_Drag4         = 0xFEF8;
    KeySym const KeySym_Pointer_Drag5         = 0xFEFD;

    KeySym const KeySym_Pointer_EnableKeys    = 0xFEF9;
    KeySym const KeySym_Pointer_Accelerate    = 0xFEFA;
    KeySym const KeySym_Pointer_DfltBtnNext   = 0xFEFB;
    KeySym const KeySym_Pointer_DfltBtnPrev   = 0xFEFC;


    /*
     * 3270 Terminal Keys
     * Byte 3 = 0xFD
     */
    KeySym const KeySym_3270_Duplicate      = 0xFD01;
    KeySym const KeySym_3270_FieldMark      = 0xFD02;
    KeySym const KeySym_3270_Right2         = 0xFD03;
    KeySym const KeySym_3270_Left2          = 0xFD04;
    KeySym const KeySym_3270_BackTab        = 0xFD05;
    KeySym const KeySym_3270_EraseEOF       = 0xFD06;
    KeySym const KeySym_3270_EraseInput     = 0xFD07;
    KeySym const KeySym_3270_Reset          = 0xFD08;
    KeySym const KeySym_3270_Quit           = 0xFD09;
    KeySym const KeySym_3270_PA1            = 0xFD0A;
    KeySym const KeySym_3270_PA2            = 0xFD0B;
    KeySym const KeySym_3270_PA3            = 0xFD0C;
    KeySym const KeySym_3270_Test           = 0xFD0D;
    KeySym const KeySym_3270_Attn           = 0xFD0E;
    KeySym const KeySym_3270_CursorBlink    = 0xFD0F;
    KeySym const KeySym_3270_AltCursor      = 0xFD10;
    KeySym const KeySym_3270_KeyClick       = 0xFD11;
    KeySym const KeySym_3270_Jump           = 0xFD12;
    KeySym const KeySym_3270_Ident          = 0xFD13;
    KeySym const KeySym_3270_Rule           = 0xFD14;
    KeySym const KeySym_3270_Copy           = 0xFD15;
    KeySym const KeySym_3270_Play           = 0xFD16;
    KeySym const KeySym_3270_Setup          = 0xFD17;
    KeySym const KeySym_3270_Record         = 0xFD18;
    KeySym const KeySym_3270_ChangeScreen   = 0xFD19;
    KeySym const KeySym_3270_DeleteWord     = 0xFD1A;
    KeySym const KeySym_3270_ExSelect       = 0xFD1B;
    KeySym const KeySym_3270_CursorSelect   = 0xFD1C;
    KeySym const KeySym_3270_PrintScreen    = 0xFD1D;
    KeySym const KeySym_3270_Enter          = 0xFD1E;


    /*
     *  Latin 1
     *  Byte 3 = 0
     */
    KeySym const KeySym_space               = 0x020;
    KeySym const KeySym_exclam              = 0x021;
    KeySym const KeySym_quotedbl            = 0x022;
    KeySym const KeySym_numbersign          = 0x023;
    KeySym const KeySym_dollar              = 0x024;
    KeySym const KeySym_percent             = 0x025;
    KeySym const KeySym_ampersand           = 0x026;
    KeySym const KeySym_apostrophe          = 0x027;
    KeySym const KeySym_parenleft           = 0x028;
    KeySym const KeySym_parenright          = 0x029;
    KeySym const KeySym_asterisk            = 0x02a;
    KeySym const KeySym_plus                = 0x02b;
    KeySym const KeySym_comma               = 0x02c;
    KeySym const KeySym_minus               = 0x02d;
    KeySym const KeySym_period              = 0x02e;
    KeySym const KeySym_slash               = 0x02f;
    KeySym const KeySym_0                   = 0x030;
    KeySym const KeySym_1                   = 0x031;
    KeySym const KeySym_2                   = 0x032;
    KeySym const KeySym_3                   = 0x033;
    KeySym const KeySym_4                   = 0x034;
    KeySym const KeySym_5                   = 0x035;
    KeySym const KeySym_6                   = 0x036;
    KeySym const KeySym_7                   = 0x037;
    KeySym const KeySym_8                   = 0x038;
    KeySym const KeySym_9                   = 0x039;
    KeySym const KeySym_colon               = 0x03a;
    KeySym const KeySym_semicolon           = 0x03b;
    KeySym const KeySym_less                = 0x03c;
    KeySym const KeySym_equal               = 0x03d;
    KeySym const KeySym_greater             = 0x03e;
    KeySym const KeySym_question            = 0x03f;
    KeySym const KeySym_at                  = 0x040;
    KeySym const KeySym_A                   = 0x041;
    KeySym const KeySym_B                   = 0x042;
    KeySym const KeySym_C                   = 0x043;
    KeySym const KeySym_D                   = 0x044;
    KeySym const KeySym_E                   = 0x045;
    KeySym const KeySym_F                   = 0x046;
    KeySym const KeySym_G                   = 0x047;
    KeySym const KeySym_H                   = 0x048;
    KeySym const KeySym_I                   = 0x049;
    KeySym const KeySym_J                   = 0x04a;
    KeySym const KeySym_K                   = 0x04b;
    KeySym const KeySym_L                   = 0x04c;
    KeySym const KeySym_M                   = 0x04d;
    KeySym const KeySym_N                   = 0x04e;
    KeySym const KeySym_O                   = 0x04f;
    KeySym const KeySym_P                   = 0x050;
    KeySym const KeySym_Q                   = 0x051;
    KeySym const KeySym_R                   = 0x052;
    KeySym const KeySym_S                   = 0x053;
    KeySym const KeySym_T                   = 0x054;
    KeySym const KeySym_U                   = 0x055;
    KeySym const KeySym_V                   = 0x056;
    KeySym const KeySym_W                   = 0x057;
    KeySym const KeySym_X                   = 0x058;
    KeySym const KeySym_Y                   = 0x059;
    KeySym const KeySym_Z                   = 0x05a;
    KeySym const KeySym_bracketleft         = 0x05b;
    KeySym const KeySym_backslash           = 0x05c;
    KeySym const KeySym_bracketright        = 0x05d;
    KeySym const KeySym_asciicircum         = 0x05e;
    KeySym const KeySym_underscore          = 0x05f;
    KeySym const KeySym_grave               = 0x060;
    KeySym const KeySym_a                   = 0x061;
    KeySym const KeySym_b                   = 0x062;
    KeySym const KeySym_c                   = 0x063;
    KeySym const KeySym_d                   = 0x064;
    KeySym const KeySym_e                   = 0x065;
    KeySym const KeySym_f                   = 0x066;
    KeySym const KeySym_g                   = 0x067;
    KeySym const KeySym_h                   = 0x068;
    KeySym const KeySym_i                   = 0x069;
    KeySym const KeySym_j                   = 0x06a;
    KeySym const KeySym_k                   = 0x06b;
    KeySym const KeySym_l                   = 0x06c;
    KeySym const KeySym_m                   = 0x06d;
    KeySym const KeySym_n                   = 0x06e;
    KeySym const KeySym_o                   = 0x06f;
    KeySym const KeySym_p                   = 0x070;
    KeySym const KeySym_q                   = 0x071;
    KeySym const KeySym_r                   = 0x072;
    KeySym const KeySym_s                   = 0x073;
    KeySym const KeySym_t                   = 0x074;
    KeySym const KeySym_u                   = 0x075;
    KeySym const KeySym_v                   = 0x076;
    KeySym const KeySym_w                   = 0x077;
    KeySym const KeySym_x                   = 0x078;
    KeySym const KeySym_y                   = 0x079;
    KeySym const KeySym_z                   = 0x07a;
    KeySym const KeySym_braceleft           = 0x07b;
    KeySym const KeySym_bar                 = 0x07c;
    KeySym const KeySym_braceright          = 0x07d;
    KeySym const KeySym_asciitilde          = 0x07e;

    KeySym const KeySym_nobreakspace        = 0x0a0;
    KeySym const KeySym_exclamdown          = 0x0a1;
    KeySym const KeySym_cent                = 0x0a2;
    KeySym const KeySym_sterling            = 0x0a3;
    KeySym const KeySym_currency            = 0x0a4;
    KeySym const KeySym_yen                 = 0x0a5;
    KeySym const KeySym_brokenbar           = 0x0a6;
    KeySym const KeySym_section             = 0x0a7;
    KeySym const KeySym_diaeresis           = 0x0a8;
    KeySym const KeySym_copyright           = 0x0a9;
    KeySym const KeySym_ordfeminine         = 0x0aa;
    KeySym const KeySym_guillemotleft       = 0x0ab; // left angle quotation mark
    KeySym const KeySym_notsign             = 0x0ac;
    KeySym const KeySym_hyphen              = 0x0ad;
    KeySym const KeySym_registered          = 0x0ae;
    KeySym const KeySym_macron              = 0x0af;
    KeySym const KeySym_degree              = 0x0b0;
    KeySym const KeySym_plusminus           = 0x0b1;
    KeySym const KeySym_twosuperior         = 0x0b2;
    KeySym const KeySym_threesuperior       = 0x0b3;
    KeySym const KeySym_acute               = 0x0b4;
    KeySym const KeySym_mu                  = 0x0b5;
    KeySym const KeySym_paragraph           = 0x0b6;
    KeySym const KeySym_periodcentered      = 0x0b7;
    KeySym const KeySym_cedilla             = 0x0b8;
    KeySym const KeySym_onesuperior         = 0x0b9;
    KeySym const KeySym_masculine           = 0x0ba;
    KeySym const KeySym_guillemotright      = 0x0bb; // right angle quotation mark
    KeySym const KeySym_onequarter          = 0x0bc;
    KeySym const KeySym_onehalf             = 0x0bd;
    KeySym const KeySym_threequarters       = 0x0be;
    KeySym const KeySym_questiondown        = 0x0bf;
    KeySym const KeySym_Agrave              = 0x0c0;
    KeySym const KeySym_Aacute              = 0x0c1;
    KeySym const KeySym_Acircumflex         = 0x0c2;
    KeySym const KeySym_Atilde              = 0x0c3;
    KeySym const KeySym_Adiaeresis          = 0x0c4;
    KeySym const KeySym_Aring               = 0x0c5;
    KeySym const KeySym_AE                  = 0x0c6;
    KeySym const KeySym_Ccedilla            = 0x0c7;
    KeySym const KeySym_Egrave              = 0x0c8;
    KeySym const KeySym_Eacute              = 0x0c9;
    KeySym const KeySym_Ecircumflex         = 0x0ca;
    KeySym const KeySym_Ediaeresis          = 0x0cb;
    KeySym const KeySym_Igrave              = 0x0cc;
    KeySym const KeySym_Iacute              = 0x0cd;
    KeySym const KeySym_Icircumflex         = 0x0ce;
    KeySym const KeySym_Idiaeresis          = 0x0cf;
    KeySym const KeySym_ETH                 = 0x0d0;
    KeySym const KeySym_Ntilde              = 0x0d1;
    KeySym const KeySym_Ograve              = 0x0d2;
    KeySym const KeySym_Oacute              = 0x0d3;
    KeySym const KeySym_Ocircumflex         = 0x0d4;
    KeySym const KeySym_Otilde              = 0x0d5;
    KeySym const KeySym_Odiaeresis          = 0x0d6;
    KeySym const KeySym_multiply            = 0x0d7;
    KeySym const KeySym_Ooblique            = 0x0d8;
    KeySym const KeySym_Oslash              = KeySym_Ooblique; // symbol alias
    KeySym const KeySym_Ugrave              = 0x0d9;
    KeySym const KeySym_Uacute              = 0x0da;
    KeySym const KeySym_Ucircumflex         = 0x0db;
    KeySym const KeySym_Udiaeresis          = 0x0dc;
    KeySym const KeySym_Yacute              = 0x0dd;
    KeySym const KeySym_THORN               = 0x0de;
    KeySym const KeySym_ssharp              = 0x0df;
    KeySym const KeySym_agrave              = 0x0e0;
    KeySym const KeySym_aacute              = 0x0e1;
    KeySym const KeySym_acircumflex         = 0x0e2;
    KeySym const KeySym_atilde              = 0x0e3;
    KeySym const KeySym_adiaeresis          = 0x0e4;
    KeySym const KeySym_aring               = 0x0e5;
    KeySym const KeySym_ae                  = 0x0e6;
    KeySym const KeySym_ccedilla            = 0x0e7;
    KeySym const KeySym_egrave              = 0x0e8;
    KeySym const KeySym_eacute              = 0x0e9;
    KeySym const KeySym_ecircumflex         = 0x0ea;
    KeySym const KeySym_ediaeresis          = 0x0eb;
    KeySym const KeySym_igrave              = 0x0ec;
    KeySym const KeySym_iacute              = 0x0ed;
    KeySym const KeySym_icircumflex         = 0x0ee;
    KeySym const KeySym_idiaeresis          = 0x0ef;
    KeySym const KeySym_eth                 = 0x0f0;
    KeySym const KeySym_ntilde              = 0x0f1;
    KeySym const KeySym_ograve              = 0x0f2;
    KeySym const KeySym_oacute              = 0x0f3;
    KeySym const KeySym_ocircumflex         = 0x0f4;
    KeySym const KeySym_otilde              = 0x0f5;
    KeySym const KeySym_odiaeresis          = 0x0f6;
    KeySym const KeySym_division            = 0x0f7;
    KeySym const KeySym_ooblique            = 0x0f8;
    KeySym const KeySym_oslash              = KeySym_ooblique; // symbol alias
    KeySym const KeySym_ugrave              = 0x0f9;
    KeySym const KeySym_uacute              = 0x0fa;
    KeySym const KeySym_ucircumflex         = 0x0fb;
    KeySym const KeySym_udiaeresis          = 0x0fc;
    KeySym const KeySym_yacute              = 0x0fd;
    KeySym const KeySym_thorn               = 0x0fe;
    KeySym const KeySym_ydiaeresis          = 0x0ff;


    /*
     *   Latin 2
     *   Byte 3 = 1
     */
    KeySym const KeySym_Aogonek             = 0x1a1;
    KeySym const KeySym_breve               = 0x1a2;
    KeySym const KeySym_Lstroke             = 0x1a3;
    KeySym const KeySym_Lcaron              = 0x1a5;
    KeySym const KeySym_Sacute              = 0x1a6;
    KeySym const KeySym_Scaron              = 0x1a9;
    KeySym const KeySym_Scedilla            = 0x1aa;
    KeySym const KeySym_Tcaron              = 0x1ab;
    KeySym const KeySym_Zacute              = 0x1ac;
    KeySym const KeySym_Zcaron              = 0x1ae;
    KeySym const KeySym_Zabovedot           = 0x1af;
    KeySym const KeySym_aogonek             = 0x1b1;
    KeySym const KeySym_ogonek              = 0x1b2;
    KeySym const KeySym_lstroke             = 0x1b3;
    KeySym const KeySym_lcaron              = 0x1b5;
    KeySym const KeySym_sacute              = 0x1b6;
    KeySym const KeySym_caron               = 0x1b7;
    KeySym const KeySym_scaron              = 0x1b9;
    KeySym const KeySym_scedilla            = 0x1ba;
    KeySym const KeySym_tcaron              = 0x1bb;
    KeySym const KeySym_zacute              = 0x1bc;
    KeySym const KeySym_doubleacute         = 0x1bd;
    KeySym const KeySym_zcaron              = 0x1be;
    KeySym const KeySym_zabovedot           = 0x1bf;
    KeySym const KeySym_Racute              = 0x1c0;
    KeySym const KeySym_Abreve              = 0x1c3;
    KeySym const KeySym_Lacute              = 0x1c5;
    KeySym const KeySym_Cacute              = 0x1c6;
    KeySym const KeySym_Ccaron              = 0x1c8;
    KeySym const KeySym_Eogonek             = 0x1ca;
    KeySym const KeySym_Ecaron              = 0x1cc;
    KeySym const KeySym_Dcaron              = 0x1cf;
    KeySym const KeySym_Dstroke             = 0x1d0;
    KeySym const KeySym_Nacute              = 0x1d1;
    KeySym const KeySym_Ncaron              = 0x1d2;
    KeySym const KeySym_Odoubleacute        = 0x1d5;
    KeySym const KeySym_Rcaron              = 0x1d8;
    KeySym const KeySym_Uring               = 0x1d9;
    KeySym const KeySym_Udoubleacute        = 0x1db;
    KeySym const KeySym_Tcedilla            = 0x1de;
    KeySym const KeySym_racute              = 0x1e0;
    KeySym const KeySym_abreve              = 0x1e3;
    KeySym const KeySym_lacute              = 0x1e5;
    KeySym const KeySym_cacute              = 0x1e6;
    KeySym const KeySym_ccaron              = 0x1e8;
    KeySym const KeySym_eogonek             = 0x1ea;
    KeySym const KeySym_ecaron              = 0x1ec;
    KeySym const KeySym_dcaron              = 0x1ef;
    KeySym const KeySym_dstroke             = 0x1f0;
    KeySym const KeySym_nacute              = 0x1f1;
    KeySym const KeySym_ncaron              = 0x1f2;
    KeySym const KeySym_odoubleacute        = 0x1f5;
    KeySym const KeySym_udoubleacute        = 0x1fb;
    KeySym const KeySym_rcaron              = 0x1f8;
    KeySym const KeySym_uring               = 0x1f9;
    KeySym const KeySym_tcedilla            = 0x1fe;
    KeySym const KeySym_abovedot            = 0x1ff;


    /*
     *   Latin 3
     *   Byte 3 = 2
     */
    KeySym const KeySym_Hstroke             = 0x2a1;
    KeySym const KeySym_Hcircumflex         = 0x2a6;
    KeySym const KeySym_Iabovedot           = 0x2a9;
    KeySym const KeySym_Gbreve              = 0x2ab;
    KeySym const KeySym_Jcircumflex         = 0x2ac;
    KeySym const KeySym_hstroke             = 0x2b1;
    KeySym const KeySym_hcircumflex         = 0x2b6;
    KeySym const KeySym_idotless            = 0x2b9;
    KeySym const KeySym_gbreve              = 0x2bb;
    KeySym const KeySym_jcircumflex         = 0x2bc;
    KeySym const KeySym_Cabovedot           = 0x2c5;
    KeySym const KeySym_Ccircumflex         = 0x2c6;
    KeySym const KeySym_Gabovedot           = 0x2d5;
    KeySym const KeySym_Gcircumflex         = 0x2d8;
    KeySym const KeySym_Ubreve              = 0x2dd;
    KeySym const KeySym_Scircumflex         = 0x2de;
    KeySym const KeySym_cabovedot           = 0x2e5;
    KeySym const KeySym_ccircumflex         = 0x2e6;
    KeySym const KeySym_gabovedot           = 0x2f5;
    KeySym const KeySym_gcircumflex         = 0x2f8;
    KeySym const KeySym_ubreve              = 0x2fd;
    KeySym const KeySym_scircumflex         = 0x2fe;


    /*
     *   Latin 4
     *   Byte 3 = 3
     */
    KeySym const KeySym_kra                 = 0x3a2;
    KeySym const KeySym_Rcedilla            = 0x3a3;
    KeySym const KeySym_Itilde              = 0x3a5;
    KeySym const KeySym_Lcedilla            = 0x3a6;
    KeySym const KeySym_Emacron             = 0x3aa;
    KeySym const KeySym_Gcedilla            = 0x3ab;
    KeySym const KeySym_Tslash              = 0x3ac;
    KeySym const KeySym_rcedilla            = 0x3b3;
    KeySym const KeySym_itilde              = 0x3b5;
    KeySym const KeySym_lcedilla            = 0x3b6;
    KeySym const KeySym_emacron             = 0x3ba;
    KeySym const KeySym_gcedilla            = 0x3bb;
    KeySym const KeySym_tslash              = 0x3bc;
    KeySym const KeySym_ENG                 = 0x3bd;
    KeySym const KeySym_eng                 = 0x3bf;
    KeySym const KeySym_Amacron             = 0x3c0;
    KeySym const KeySym_Iogonek             = 0x3c7;
    KeySym const KeySym_Eabovedot           = 0x3cc;
    KeySym const KeySym_Imacron             = 0x3cf;
    KeySym const KeySym_Ncedilla            = 0x3d1;
    KeySym const KeySym_Omacron             = 0x3d2;
    KeySym const KeySym_Kcedilla            = 0x3d3;
    KeySym const KeySym_Uogonek             = 0x3d9;
    KeySym const KeySym_Utilde              = 0x3dd;
    KeySym const KeySym_Umacron             = 0x3de;
    KeySym const KeySym_amacron             = 0x3e0;
    KeySym const KeySym_iogonek             = 0x3e7;
    KeySym const KeySym_eabovedot           = 0x3ec;
    KeySym const KeySym_imacron             = 0x3ef;
    KeySym const KeySym_ncedilla            = 0x3f1;
    KeySym const KeySym_omacron             = 0x3f2;
    KeySym const KeySym_kcedilla            = 0x3f3;
    KeySym const KeySym_uogonek             = 0x3f9;
    KeySym const KeySym_utilde              = 0x3fd;
    KeySym const KeySym_umacron             = 0x3fe;


    /*
     * Latin-8
     * Byte 3 = 18
     */
    KeySym const KeySym_Babovedot           = 0x12a1;
    KeySym const KeySym_babovedot           = 0x12a2;
    KeySym const KeySym_Dabovedot           = 0x12a6;
    KeySym const KeySym_Wgrave              = 0x12a8;
    KeySym const KeySym_Wacute              = 0x12aa;
    KeySym const KeySym_dabovedot           = 0x12ab;
    KeySym const KeySym_Ygrave              = 0x12ac;
    KeySym const KeySym_Fabovedot           = 0x12b0;
    KeySym const KeySym_fabovedot           = 0x12b1;
    KeySym const KeySym_Mabovedot           = 0x12b4;
    KeySym const KeySym_mabovedot           = 0x12b5;
    KeySym const KeySym_Pabovedot           = 0x12b7;
    KeySym const KeySym_wgrave              = 0x12b8;
    KeySym const KeySym_pabovedot           = 0x12b9;
    KeySym const KeySym_wacute              = 0x12ba;
    KeySym const KeySym_Sabovedot           = 0x12bb;
    KeySym const KeySym_ygrave              = 0x12bc;
    KeySym const KeySym_Wdiaeresis          = 0x12bd;
    KeySym const KeySym_wdiaeresis          = 0x12be;
    KeySym const KeySym_sabovedot           = 0x12bf;
    KeySym const KeySym_Wcircumflex         = 0x12d0;
    KeySym const KeySym_Tabovedot           = 0x12d7;
    KeySym const KeySym_Ycircumflex         = 0x12de;
    KeySym const KeySym_wcircumflex         = 0x12f0;
    KeySym const KeySym_tabovedot           = 0x12f7;
    KeySym const KeySym_ycircumflex         = 0x12fe;


    /*
     * Latin-9 (a.k.a. Latin-0)
     * Byte 3 = 19
     */
    KeySym const KeySym_OE                  = 0x13bc;
    KeySym const KeySym_oe                  = 0x13bd;
    KeySym const KeySym_Ydiaeresis          = 0x13be;


    /*
     * Katakana
     * Byte 3 = 4
     */
    KeySym const KeySym_overline                                    = 0x47e;
    KeySym const KeySym_kana_fullstop                               = 0x4a1;
    KeySym const KeySym_kana_openingbracket                         = 0x4a2;
    KeySym const KeySym_kana_closingbracket                         = 0x4a3;
    KeySym const KeySym_kana_comma                                  = 0x4a4;
    KeySym const KeySym_kana_conjunctive                            = 0x4a5;
    KeySym const KeySym_kana_WO                                     = 0x4a6;
    KeySym const KeySym_kana_a                                      = 0x4a7;
    KeySym const KeySym_kana_i                                      = 0x4a8;
    KeySym const KeySym_kana_u                                      = 0x4a9;
    KeySym const KeySym_kana_e                                      = 0x4aa;
    KeySym const KeySym_kana_o                                      = 0x4ab;
    KeySym const KeySym_kana_ya                                     = 0x4ac;
    KeySym const KeySym_kana_yu                                     = 0x4ad;
    KeySym const KeySym_kana_yo                                     = 0x4ae;
    KeySym const KeySym_kana_tsu                                    = 0x4af;
    KeySym const KeySym_prolongedsound                              = 0x4b0;
    KeySym const KeySym_kana_A                                      = 0x4b1;
    KeySym const KeySym_kana_I                                      = 0x4b2;
    KeySym const KeySym_kana_U                                      = 0x4b3;
    KeySym const KeySym_kana_E                                      = 0x4b4;
    KeySym const KeySym_kana_O                                      = 0x4b5;
    KeySym const KeySym_kana_KA                                     = 0x4b6;
    KeySym const KeySym_kana_KI                                     = 0x4b7;
    KeySym const KeySym_kana_KU                                     = 0x4b8;
    KeySym const KeySym_kana_KE                                     = 0x4b9;
    KeySym const KeySym_kana_KO                                     = 0x4ba;
    KeySym const KeySym_kana_SA                                     = 0x4bb;
    KeySym const KeySym_kana_SHI                                    = 0x4bc;
    KeySym const KeySym_kana_SU                                     = 0x4bd;
    KeySym const KeySym_kana_SE                                     = 0x4be;
    KeySym const KeySym_kana_SO                                     = 0x4bf;
    KeySym const KeySym_kana_TA                                     = 0x4c0;
    KeySym const KeySym_kana_CHI                                    = 0x4c1;
    KeySym const KeySym_kana_TSU                                    = 0x4c2;
    KeySym const KeySym_kana_TE                                     = 0x4c3;
    KeySym const KeySym_kana_TO                                     = 0x4c4;
    KeySym const KeySym_kana_NA                                     = 0x4c5;
    KeySym const KeySym_kana_NI                                     = 0x4c6;
    KeySym const KeySym_kana_NU                                     = 0x4c7;
    KeySym const KeySym_kana_NE                                     = 0x4c8;
    KeySym const KeySym_kana_NO                                     = 0x4c9;
    KeySym const KeySym_kana_HA                                     = 0x4ca;
    KeySym const KeySym_kana_HI                                     = 0x4cb;
    KeySym const KeySym_kana_FU                                     = 0x4cc;
    KeySym const KeySym_kana_HE                                     = 0x4cd;
    KeySym const KeySym_kana_HO                                     = 0x4ce;
    KeySym const KeySym_kana_MA                                     = 0x4cf;
    KeySym const KeySym_kana_MI                                     = 0x4d0;
    KeySym const KeySym_kana_MU                                     = 0x4d1;
    KeySym const KeySym_kana_ME                                     = 0x4d2;
    KeySym const KeySym_kana_MO                                     = 0x4d3;
    KeySym const KeySym_kana_YA                                     = 0x4d4;
    KeySym const KeySym_kana_YU                                     = 0x4d5;
    KeySym const KeySym_kana_YO                                     = 0x4d6;
    KeySym const KeySym_kana_RA                                     = 0x4d7;
    KeySym const KeySym_kana_RI                                     = 0x4d8;
    KeySym const KeySym_kana_RU                                     = 0x4d9;
    KeySym const KeySym_kana_RE                                     = 0x4da;
    KeySym const KeySym_kana_RO                                     = 0x4db;
    KeySym const KeySym_kana_WA                                     = 0x4dc;
    KeySym const KeySym_kana_N                                      = 0x4dd;
    KeySym const KeySym_voicedsound                                 = 0x4de;
    KeySym const KeySym_semivoicedsound                             = 0x4df;
    KeySym const KeySym_kana_switch                                 = 0xFF7E; // Alias for mode_switch


    /*
     *  Arabic
     *  Byte 3 = 5
     */
    KeySym const KeySym_Farsi_0                                     = 0x590;
    KeySym const KeySym_Farsi_1                                     = 0x591;
    KeySym const KeySym_Farsi_2                                     = 0x592;
    KeySym const KeySym_Farsi_3                                     = 0x593;
    KeySym const KeySym_Farsi_4                                     = 0x594;
    KeySym const KeySym_Farsi_5                                     = 0x595;
    KeySym const KeySym_Farsi_6                                     = 0x596;
    KeySym const KeySym_Farsi_7                                     = 0x597;
    KeySym const KeySym_Farsi_8                                     = 0x598;
    KeySym const KeySym_Farsi_9                                     = 0x599;
    KeySym const KeySym_Arabic_percent                              = 0x5a5;
    KeySym const KeySym_Arabic_superscript_alef                     = 0x5a6;
    KeySym const KeySym_Arabic_tteh                                 = 0x5a7;
    KeySym const KeySym_Arabic_peh                                  = 0x5a8;
    KeySym const KeySym_Arabic_tcheh                                = 0x5a9;
    KeySym const KeySym_Arabic_ddal                                 = 0x5aa;
    KeySym const KeySym_Arabic_rreh                                 = 0x5ab;
    KeySym const KeySym_Arabic_comma                                = 0x5ac;
    KeySym const KeySym_Arabic_fullstop                             = 0x5ae;
    KeySym const KeySym_Arabic_0                                    = 0x5b0;
    KeySym const KeySym_Arabic_1                                    = 0x5b1;
    KeySym const KeySym_Arabic_2                                    = 0x5b2;
    KeySym const KeySym_Arabic_3                                    = 0x5b3;
    KeySym const KeySym_Arabic_4                                    = 0x5b4;
    KeySym const KeySym_Arabic_5                                    = 0x5b5;
    KeySym const KeySym_Arabic_6                                    = 0x5b6;
    KeySym const KeySym_Arabic_7                                    = 0x5b7;
    KeySym const KeySym_Arabic_8                                    = 0x5b8;
    KeySym const KeySym_Arabic_9                                    = 0x5b9;
    KeySym const KeySym_Arabic_semicolon                            = 0x5bb;
    KeySym const KeySym_Arabic_question_mark                        = 0x5bf;
    KeySym const KeySym_Arabic_hamza                                = 0x5c1;
    KeySym const KeySym_Arabic_maddaonalef                          = 0x5c2;
    KeySym const KeySym_Arabic_hamzaonalef                          = 0x5c3;
    KeySym const KeySym_Arabic_hamzaonwaw                           = 0x5c4;
    KeySym const KeySym_Arabic_hamzaunderalef                       = 0x5c5;
    KeySym const KeySym_Arabic_hamzaonyeh                           = 0x5c6;
    KeySym const KeySym_Arabic_alef                                 = 0x5c7;
    KeySym const KeySym_Arabic_beh                                  = 0x5c8;
    KeySym const KeySym_Arabic_tehmarbuta                           = 0x5c9;
    KeySym const KeySym_Arabic_teh                                  = 0x5ca;
    KeySym const KeySym_Arabic_theh                                 = 0x5cb;
    KeySym const KeySym_Arabic_jeem                                 = 0x5cc;
    KeySym const KeySym_Arabic_hah                                  = 0x5cd;
    KeySym const KeySym_Arabic_khah                                 = 0x5ce;
    KeySym const KeySym_Arabic_dal                                  = 0x5cf;
    KeySym const KeySym_Arabic_thal                                 = 0x5d0;
    KeySym const KeySym_Arabic_ra                                   = 0x5d1;
    KeySym const KeySym_Arabic_zain                                 = 0x5d2;
    KeySym const KeySym_Arabic_seen                                 = 0x5d3;
    KeySym const KeySym_Arabic_sheen                                = 0x5d4;
    KeySym const KeySym_Arabic_sad                                  = 0x5d5;
    KeySym const KeySym_Arabic_dad                                  = 0x5d6;
    KeySym const KeySym_Arabic_tah                                  = 0x5d7;
    KeySym const KeySym_Arabic_zah                                  = 0x5d8;
    KeySym const KeySym_Arabic_ain                                  = 0x5d9;
    KeySym const KeySym_Arabic_ghain                                = 0x5da;
    KeySym const KeySym_Arabic_tatweel                              = 0x5e0;
    KeySym const KeySym_Arabic_feh                                  = 0x5e1;
    KeySym const KeySym_Arabic_qaf                                  = 0x5e2;
    KeySym const KeySym_Arabic_kaf                                  = 0x5e3;
    KeySym const KeySym_Arabic_lam                                  = 0x5e4;
    KeySym const KeySym_Arabic_meem                                 = 0x5e5;
    KeySym const KeySym_Arabic_noon                                 = 0x5e6;
    KeySym const KeySym_Arabic_ha                                   = 0x5e7;
    KeySym const KeySym_Arabic_waw                                  = 0x5e8;
    KeySym const KeySym_Arabic_alefmaksura                          = 0x5e9;
    KeySym const KeySym_Arabic_yeh                                  = 0x5ea;
    KeySym const KeySym_Arabic_fathatan                             = 0x5eb;
    KeySym const KeySym_Arabic_dammatan                             = 0x5ec;
    KeySym const KeySym_Arabic_kasratan                             = 0x5ed;
    KeySym const KeySym_Arabic_fatha                                = 0x5ee;
    KeySym const KeySym_Arabic_damma                                = 0x5ef;
    KeySym const KeySym_Arabic_kasra                                = 0x5f0;
    KeySym const KeySym_Arabic_shadda                               = 0x5f1;
    KeySym const KeySym_Arabic_sukun                                = 0x5f2;
    KeySym const KeySym_Arabic_madda_above                          = 0x5f3;
    KeySym const KeySym_Arabic_hamza_above                          = 0x5f4;
    KeySym const KeySym_Arabic_hamza_below                          = 0x5f5;
    KeySym const KeySym_Arabic_jeh                                  = 0x5f6;
    KeySym const KeySym_Arabic_veh                                  = 0x5f7;
    KeySym const KeySym_Arabic_keheh                                = 0x5f8;
    KeySym const KeySym_Arabic_gaf                                  = 0x5f9;
    KeySym const KeySym_Arabic_noon_ghunna                          = 0x5fa;
    KeySym const KeySym_Arabic_heh_doachashmee                      = 0x5fb;
    KeySym const KeySym_Farsi_yeh                                   = 0x5fc;
    KeySym const KeySym_Arabic_farsi_yeh                            = KeySym_Farsi_yeh; // symbol alias
    KeySym const KeySym_Arabic_yeh_baree                            = 0x5fd;
    KeySym const KeySym_Arabic_heh_goal                             = 0x5fe;
    KeySym const KeySym_Arabic_switch                               = 0xFF7E; // Alias for mode_switch


    /*
     * Cyrillic
     * Byte 3 = 6
     */
    KeySym const KeySym_Cyrillic_GHE_bar                            = 0x680;
    KeySym const KeySym_Cyrillic_ghe_bar                            = 0x690;
    KeySym const KeySym_Cyrillic_ZHE_descender                      = 0x681;
    KeySym const KeySym_Cyrillic_zhe_descender                      = 0x691;
    KeySym const KeySym_Cyrillic_KA_descender                       = 0x682;
    KeySym const KeySym_Cyrillic_ka_descender                       = 0x692;
    KeySym const KeySym_Cyrillic_KA_vertstroke                      = 0x683;
    KeySym const KeySym_Cyrillic_ka_vertstroke                      = 0x693;
    KeySym const KeySym_Cyrillic_EN_descender                       = 0x684;
    KeySym const KeySym_Cyrillic_en_descender                       = 0x694;
    KeySym const KeySym_Cyrillic_U_straight                         = 0x685;
    KeySym const KeySym_Cyrillic_u_straight                         = 0x695;
    KeySym const KeySym_Cyrillic_U_straight_bar                     = 0x686;
    KeySym const KeySym_Cyrillic_u_straight_bar                     = 0x696;
    KeySym const KeySym_Cyrillic_HA_descender                       = 0x687;
    KeySym const KeySym_Cyrillic_ha_descender                       = 0x697;
    KeySym const KeySym_Cyrillic_CHE_descender                      = 0x688;
    KeySym const KeySym_Cyrillic_che_descender                      = 0x698;
    KeySym const KeySym_Cyrillic_CHE_vertstroke                     = 0x689;
    KeySym const KeySym_Cyrillic_che_vertstroke                     = 0x699;
    KeySym const KeySym_Cyrillic_SHHA                               = 0x68a;
    KeySym const KeySym_Cyrillic_shha                               = 0x69a;

    KeySym const KeySym_Cyrillic_SCHWA                              = 0x68c;
    KeySym const KeySym_Cyrillic_schwa                              = 0x69c;
    KeySym const KeySym_Cyrillic_I_macron                           = 0x68d;
    KeySym const KeySym_Cyrillic_i_macron                           = 0x69d;
    KeySym const KeySym_Cyrillic_O_bar                              = 0x68e;
    KeySym const KeySym_Cyrillic_o_bar                              = 0x69e;
    KeySym const KeySym_Cyrillic_U_macron                           = 0x68f;
    KeySym const KeySym_Cyrillic_u_macron                           = 0x69f;

    KeySym const KeySym_Serbian_dje                                 = 0x6a1;
    KeySym const KeySym_Macedonia_gje                               = 0x6a2;
    KeySym const KeySym_Cyrillic_io                                 = 0x6a3;
    KeySym const KeySym_Ukrainian_ie                                = 0x6a4;
    KeySym const KeySym_Macedonia_dse                               = 0x6a5;
    KeySym const KeySym_Ukrainian_i                                 = 0x6a6;
    KeySym const KeySym_Ukrainian_yi                                = 0x6a7;
    KeySym const KeySym_Cyrillic_je                                 = 0x6a8;
    KeySym const KeySym_Cyrillic_lje                                = 0x6a9;
    KeySym const KeySym_Cyrillic_nje                                = 0x6aa;
    KeySym const KeySym_Serbian_tshe                                = 0x6ab;
    KeySym const KeySym_Macedonia_kje                               = 0x6ac;
    KeySym const KeySym_Ukrainian_ghe_with_upturn                   = 0x6ad;
    KeySym const KeySym_Byelorussian_shortu                         = 0x6ae;
    KeySym const KeySym_Cyrillic_dzhe                               = 0x6af;
    KeySym const KeySym_numerosign                                  = 0x6b0;
    KeySym const KeySym_Serbian_DJE                                 = 0x6b1;
    KeySym const KeySym_Macedonia_GJE                               = 0x6b2;
    KeySym const KeySym_Cyrillic_IO                                 = 0x6b3;
    KeySym const KeySym_Ukrainian_IE                                = 0x6b4;
    KeySym const KeySym_Macedonia_DSE                               = 0x6b5;
    KeySym const KeySym_Ukrainian_I                                 = 0x6b6;
    KeySym const KeySym_Ukrainian_YI                                = 0x6b7;
    KeySym const KeySym_Cyrillic_JE                                 = 0x6b8;
    KeySym const KeySym_Cyrillic_LJE                                = 0x6b9;
    KeySym const KeySym_Cyrillic_NJE                                = 0x6ba;
    KeySym const KeySym_Serbian_TSHE                                = 0x6bb;
    KeySym const KeySym_Macedonia_KJE                               = 0x6bc;
    KeySym const KeySym_Ukrainian_GHE_WITH_UPTURN                   = 0x6bd;
    KeySym const KeySym_Byelorussian_SHORTU                         = 0x6be;
    KeySym const KeySym_Cyrillic_DZHE                               = 0x6bf;
    KeySym const KeySym_Cyrillic_yu                                 = 0x6c0;
    KeySym const KeySym_Cyrillic_a                                  = 0x6c1;
    KeySym const KeySym_Cyrillic_be                                 = 0x6c2;
    KeySym const KeySym_Cyrillic_tse                                = 0x6c3;
    KeySym const KeySym_Cyrillic_de                                 = 0x6c4;
    KeySym const KeySym_Cyrillic_ie                                 = 0x6c5;
    KeySym const KeySym_Cyrillic_ef                                 = 0x6c6;
    KeySym const KeySym_Cyrillic_ghe                                = 0x6c7;
    KeySym const KeySym_Cyrillic_ha                                 = 0x6c8;
    KeySym const KeySym_Cyrillic_i                                  = 0x6c9;
    KeySym const KeySym_Cyrillic_shorti                             = 0x6ca;
    KeySym const KeySym_Cyrillic_ka                                 = 0x6cb;
    KeySym const KeySym_Cyrillic_el                                 = 0x6cc;
    KeySym const KeySym_Cyrillic_em                                 = 0x6cd;
    KeySym const KeySym_Cyrillic_en                                 = 0x6ce;
    KeySym const KeySym_Cyrillic_o                                  = 0x6cf;
    KeySym const KeySym_Cyrillic_pe                                 = 0x6d0;
    KeySym const KeySym_Cyrillic_ya                                 = 0x6d1;
    KeySym const KeySym_Cyrillic_er                                 = 0x6d2;
    KeySym const KeySym_Cyrillic_es                                 = 0x6d3;
    KeySym const KeySym_Cyrillic_te                                 = 0x6d4;
    KeySym const KeySym_Cyrillic_u                                  = 0x6d5;
    KeySym const KeySym_Cyrillic_zhe                                = 0x6d6;
    KeySym const KeySym_Cyrillic_ve                                 = 0x6d7;
    KeySym const KeySym_Cyrillic_softsign                           = 0x6d8;
    KeySym const KeySym_Cyrillic_yeru                               = 0x6d9;
    KeySym const KeySym_Cyrillic_ze                                 = 0x6da;
    KeySym const KeySym_Cyrillic_sha                                = 0x6db;
    KeySym const KeySym_Cyrillic_e                                  = 0x6dc;
    KeySym const KeySym_Cyrillic_shcha                              = 0x6dd;
    KeySym const KeySym_Cyrillic_che                                = 0x6de;
    KeySym const KeySym_Cyrillic_hardsign                           = 0x6df;
    KeySym const KeySym_Cyrillic_YU                                 = 0x6e0;
    KeySym const KeySym_Cyrillic_A                                  = 0x6e1;
    KeySym const KeySym_Cyrillic_BE                                 = 0x6e2;
    KeySym const KeySym_Cyrillic_TSE                                = 0x6e3;
    KeySym const KeySym_Cyrillic_DE                                 = 0x6e4;
    KeySym const KeySym_Cyrillic_IE                                 = 0x6e5;
    KeySym const KeySym_Cyrillic_EF                                 = 0x6e6;
    KeySym const KeySym_Cyrillic_GHE                                = 0x6e7;
    KeySym const KeySym_Cyrillic_HA                                 = 0x6e8;
    KeySym const KeySym_Cyrillic_I                                  = 0x6e9;
    KeySym const KeySym_Cyrillic_SHORTI                             = 0x6ea;
    KeySym const KeySym_Cyrillic_KA                                 = 0x6eb;
    KeySym const KeySym_Cyrillic_EL                                 = 0x6ec;
    KeySym const KeySym_Cyrillic_EM                                 = 0x6ed;
    KeySym const KeySym_Cyrillic_EN                                 = 0x6ee;
    KeySym const KeySym_Cyrillic_O                                  = 0x6ef;
    KeySym const KeySym_Cyrillic_PE                                 = 0x6f0;
    KeySym const KeySym_Cyrillic_YA                                 = 0x6f1;
    KeySym const KeySym_Cyrillic_ER                                 = 0x6f2;
    KeySym const KeySym_Cyrillic_ES                                 = 0x6f3;
    KeySym const KeySym_Cyrillic_TE                                 = 0x6f4;
    KeySym const KeySym_Cyrillic_U                                  = 0x6f5;
    KeySym const KeySym_Cyrillic_ZHE                                = 0x6f6;
    KeySym const KeySym_Cyrillic_VE                                 = 0x6f7;
    KeySym const KeySym_Cyrillic_SOFTSIGN                           = 0x6f8;
    KeySym const KeySym_Cyrillic_YERU                               = 0x6f9;
    KeySym const KeySym_Cyrillic_ZE                                 = 0x6fa;
    KeySym const KeySym_Cyrillic_SHA                                = 0x6fb;
    KeySym const KeySym_Cyrillic_E                                  = 0x6fc;
    KeySym const KeySym_Cyrillic_SHCHA                              = 0x6fd;
    KeySym const KeySym_Cyrillic_CHE                                = 0x6fe;
    KeySym const KeySym_Cyrillic_HARDSIGN                           = 0x6ff;


    /*
     * Greek
     * Byte 3 = 7
     */
    KeySym const KeySym_Greek_ALPHAaccent                           = 0x7a1;
    KeySym const KeySym_Greek_EPSILONaccent                         = 0x7a2;
    KeySym const KeySym_Greek_ETAaccent                             = 0x7a3;
    KeySym const KeySym_Greek_IOTAaccent                            = 0x7a4;
    KeySym const KeySym_Greek_IOTAdieresis                          = 0x7a5;
    KeySym const KeySym_Greek_OMICRONaccent                         = 0x7a7;
    KeySym const KeySym_Greek_UPSILONaccent                         = 0x7a8;
    KeySym const KeySym_Greek_UPSILONdieresis                       = 0x7a9;
    KeySym const KeySym_Greek_OMEGAaccent                           = 0x7ab;
    KeySym const KeySym_Greek_accentdieresis                        = 0x7ae;
    KeySym const KeySym_Greek_horizbar                              = 0x7af;
    KeySym const KeySym_Greek_alphaaccent                           = 0x7b1;
    KeySym const KeySym_Greek_epsilonaccent                         = 0x7b2;
    KeySym const KeySym_Greek_etaaccent                             = 0x7b3;
    KeySym const KeySym_Greek_iotaaccent                            = 0x7b4;
    KeySym const KeySym_Greek_iotadieresis                          = 0x7b5;
    KeySym const KeySym_Greek_iotaaccentdieresis                    = 0x7b6;
    KeySym const KeySym_Greek_omicronaccent                         = 0x7b7;
    KeySym const KeySym_Greek_upsilonaccent                         = 0x7b8;
    KeySym const KeySym_Greek_upsilondieresis                       = 0x7b9;
    KeySym const KeySym_Greek_upsilonaccentdieresis                 = 0x7ba;
    KeySym const KeySym_Greek_omegaaccent                           = 0x7bb;
    KeySym const KeySym_Greek_ALPHA                                 = 0x7c1;
    KeySym const KeySym_Greek_BETA                                  = 0x7c2;
    KeySym const KeySym_Greek_GAMMA                                 = 0x7c3;
    KeySym const KeySym_Greek_DELTA                                 = 0x7c4;
    KeySym const KeySym_Greek_EPSILON                               = 0x7c5;
    KeySym const KeySym_Greek_ZETA                                  = 0x7c6;
    KeySym const KeySym_Greek_ETA                                   = 0x7c7;
    KeySym const KeySym_Greek_THETA                                 = 0x7c8;
    KeySym const KeySym_Greek_IOTA                                  = 0x7c9;
    KeySym const KeySym_Greek_KAPPA                                 = 0x7ca;
    KeySym const KeySym_Greek_LAMDA                                 = 0x7cb; // Alias for Greek_LAMBDA
    KeySym const KeySym_Greek_LAMBDA                                = 0x7cb;
    KeySym const KeySym_Greek_MU                                    = 0x7cc;
    KeySym const KeySym_Greek_NU                                    = 0x7cd;
    KeySym const KeySym_Greek_XI                                    = 0x7ce;
    KeySym const KeySym_Greek_OMICRON                               = 0x7cf;
    KeySym const KeySym_Greek_PI                                    = 0x7d0;
    KeySym const KeySym_Greek_RHO                                   = 0x7d1;
    KeySym const KeySym_Greek_SIGMA                                 = 0x7d2;
    KeySym const KeySym_Greek_TAU                                   = 0x7d4;
    KeySym const KeySym_Greek_UPSILON                               = 0x7d5;
    KeySym const KeySym_Greek_PHI                                   = 0x7d6;
    KeySym const KeySym_Greek_CHI                                   = 0x7d7;
    KeySym const KeySym_Greek_PSI                                   = 0x7d8;
    KeySym const KeySym_Greek_OMEGA                                 = 0x7d9;
    KeySym const KeySym_Greek_alpha                                 = 0x7e1;
    KeySym const KeySym_Greek_beta                                  = 0x7e2;
    KeySym const KeySym_Greek_gamma                                 = 0x7e3;
    KeySym const KeySym_Greek_delta                                 = 0x7e4;
    KeySym const KeySym_Greek_epsilon                               = 0x7e5;
    KeySym const KeySym_Greek_zeta                                  = 0x7e6;
    KeySym const KeySym_Greek_eta                                   = 0x7e7;
    KeySym const KeySym_Greek_theta                                 = 0x7e8;
    KeySym const KeySym_Greek_iota                                  = 0x7e9;
    KeySym const KeySym_Greek_kappa                                 = 0x7ea;
    KeySym const KeySym_Greek_lamda                                 = 0x7eb; // Alias for Greek_lambda
    KeySym const KeySym_Greek_lambda                                = 0x7eb;
    KeySym const KeySym_Greek_mu                                    = 0x7ec;
    KeySym const KeySym_Greek_nu                                    = 0x7ed;
    KeySym const KeySym_Greek_xi                                    = 0x7ee;
    KeySym const KeySym_Greek_omicron                               = 0x7ef;
    KeySym const KeySym_Greek_pi                                    = 0x7f0;
    KeySym const KeySym_Greek_rho                                   = 0x7f1;
    KeySym const KeySym_Greek_sigma                                 = 0x7f2;
    KeySym const KeySym_Greek_finalsmallsigma                       = 0x7f3;
    KeySym const KeySym_Greek_tau                                   = 0x7f4;
    KeySym const KeySym_Greek_upsilon                               = 0x7f5;
    KeySym const KeySym_Greek_phi                                   = 0x7f6;
    KeySym const KeySym_Greek_chi                                   = 0x7f7;
    KeySym const KeySym_Greek_psi                                   = 0x7f8;
    KeySym const KeySym_Greek_omega                                 = 0x7f9;
    KeySym const KeySym_Greek_switch                                = 0xFF7E; // Alias for mode_switch


    /*
     * Technical
     * Byte 3 = 8
     */
    KeySym const KeySym_leftradical                                 = 0x8a1;
    KeySym const KeySym_topleftradical                              = 0x8a2;
    KeySym const KeySym_horizconnector                              = 0x8a3;
    KeySym const KeySym_topintegral                                 = 0x8a4;
    KeySym const KeySym_botintegral                                 = 0x8a5;
    KeySym const KeySym_vertconnector                               = 0x8a6;
    KeySym const KeySym_topleftsqbracket                            = 0x8a7;
    KeySym const KeySym_botleftsqbracket                            = 0x8a8;
    KeySym const KeySym_toprightsqbracket                           = 0x8a9;
    KeySym const KeySym_botrightsqbracket                           = 0x8aa;
    KeySym const KeySym_topleftparens                               = 0x8ab;
    KeySym const KeySym_botleftparens                               = 0x8ac;
    KeySym const KeySym_toprightparens                              = 0x8ad;
    KeySym const KeySym_botrightparens                              = 0x8ae;
    KeySym const KeySym_leftmiddlecurlybrace                        = 0x8af;
    KeySym const KeySym_rightmiddlecurlybrace                       = 0x8b0;
    KeySym const KeySym_topleftsummation                            = 0x8b1;
    KeySym const KeySym_botleftsummation                            = 0x8b2;
    KeySym const KeySym_topvertsummationconnector                   = 0x8b3;
    KeySym const KeySym_botvertsummationconnector                   = 0x8b4;
    KeySym const KeySym_toprightsummation                           = 0x8b5;
    KeySym const KeySym_botrightsummation                           = 0x8b6;
    KeySym const KeySym_rightmiddlesummation                        = 0x8b7;
    KeySym const KeySym_lessthanequal                               = 0x8bc;
    KeySym const KeySym_notequal                                    = 0x8bd;
    KeySym const KeySym_greaterthanequal                            = 0x8be;
    KeySym const KeySym_integral                                    = 0x8bf;
    KeySym const KeySym_therefore                                   = 0x8c0;
    KeySym const KeySym_variation                                   = 0x8c1;
    KeySym const KeySym_infinity                                    = 0x8c2;
    KeySym const KeySym_nabla                                       = 0x8c5;
    KeySym const KeySym_approximate                                 = 0x8c8;
    KeySym const KeySym_similarequal                                = 0x8c9;
    KeySym const KeySym_ifonlyif                                    = 0x8cd;
    KeySym const KeySym_implies                                     = 0x8ce;
    KeySym const KeySym_identical                                   = 0x8cf;
    KeySym const KeySym_radical                                     = 0x8d6;
    KeySym const KeySym_includedin                                  = 0x8da;
    KeySym const KeySym_includes                                    = 0x8db;
    KeySym const KeySym_intersection                                = 0x8dc;
    KeySym const KeySym_union                                       = 0x8dd;
    KeySym const KeySym_logicaland                                  = 0x8de;
    KeySym const KeySym_logicalor                                   = 0x8df;
    KeySym const KeySym_partialderivative                           = 0x8ef;
    KeySym const KeySym_function                                    = 0x8f6;
    KeySym const KeySym_leftarrow                                   = 0x8fb;
    KeySym const KeySym_uparrow                                     = 0x8fc;
    KeySym const KeySym_rightarrow                                  = 0x8fd;
    KeySym const KeySym_downarrow                                   = 0x8fe;


    /*
     *  Special
     *  Byte 3 = 9
     */
    KeySym const KeySym_blank                                       = 0x9df;
    KeySym const KeySym_soliddiamond                                = 0x9e0;
    KeySym const KeySym_checkerboard                                = 0x9e1;
    KeySym const KeySym_ht                                          = 0x9e2;
    KeySym const KeySym_ff                                          = 0x9e3;
    KeySym const KeySym_cr                                          = 0x9e4;
    KeySym const KeySym_lf                                          = 0x9e5;
    KeySym const KeySym_nl                                          = 0x9e8;
    KeySym const KeySym_vt                                          = 0x9e9;
    KeySym const KeySym_lowrightcorner                              = 0x9ea;
    KeySym const KeySym_uprightcorner                               = 0x9eb;
    KeySym const KeySym_upleftcorner                                = 0x9ec;
    KeySym const KeySym_lowleftcorner                               = 0x9ed;
    KeySym const KeySym_crossinglines                               = 0x9ee;
    KeySym const KeySym_horizlinescan1                              = 0x9ef;
    KeySym const KeySym_horizlinescan3                              = 0x9f0;
    KeySym const KeySym_horizlinescan5                              = 0x9f1;
    KeySym const KeySym_horizlinescan7                              = 0x9f2;
    KeySym const KeySym_horizlinescan9                              = 0x9f3;
    KeySym const KeySym_leftt                                       = 0x9f4;
    KeySym const KeySym_rightt                                      = 0x9f5;
    KeySym const KeySym_bott                                        = 0x9f6;
    KeySym const KeySym_topt                                        = 0x9f7;
    KeySym const KeySym_vertbar                                     = 0x9f8;


    /*
     *  Publishing
     *  Byte 3 = a
     */
    KeySym const KeySym_emspace                                     = 0xaa1;
    KeySym const KeySym_enspace                                     = 0xaa2;
    KeySym const KeySym_em3space                                    = 0xaa3;
    KeySym const KeySym_em4space                                    = 0xaa4;
    KeySym const KeySym_digitspace                                  = 0xaa5;
    KeySym const KeySym_punctspace                                  = 0xaa6;
    KeySym const KeySym_thinspace                                   = 0xaa7;
    KeySym const KeySym_hairspace                                   = 0xaa8;
    KeySym const KeySym_emdash                                      = 0xaa9;
    KeySym const KeySym_endash                                      = 0xaaa;
    KeySym const KeySym_signifblank                                 = 0xaac;
    KeySym const KeySym_ellipsis                                    = 0xaae;
    KeySym const KeySym_doubbaselinedot                             = 0xaaf;
    KeySym const KeySym_onethird                                    = 0xab0;
    KeySym const KeySym_twothirds                                   = 0xab1;
    KeySym const KeySym_onefifth                                    = 0xab2;
    KeySym const KeySym_twofifths                                   = 0xab3;
    KeySym const KeySym_threefifths                                 = 0xab4;
    KeySym const KeySym_fourfifths                                  = 0xab5;
    KeySym const KeySym_onesixth                                    = 0xab6;
    KeySym const KeySym_fivesixths                                  = 0xab7;
    KeySym const KeySym_careof                                      = 0xab8;
    KeySym const KeySym_figdash                                     = 0xabb;
    KeySym const KeySym_leftanglebracket                            = 0xabc;
    KeySym const KeySym_decimalpoint                                = 0xabd;
    KeySym const KeySym_rightanglebracket                           = 0xabe;
    KeySym const KeySym_marker                                      = 0xabf;
    KeySym const KeySym_oneeighth                                   = 0xac3;
    KeySym const KeySym_threeeighths                                = 0xac4;
    KeySym const KeySym_fiveeighths                                 = 0xac5;
    KeySym const KeySym_seveneighths                                = 0xac6;
    KeySym const KeySym_trademark                                   = 0xac9;
    KeySym const KeySym_signaturemark                               = 0xaca;
    KeySym const KeySym_trademarkincircle                           = 0xacb;
    KeySym const KeySym_leftopentriangle                            = 0xacc;
    KeySym const KeySym_rightopentriangle                           = 0xacd;
    KeySym const KeySym_emopencircle                                = 0xace;
    KeySym const KeySym_emopenrectangle                             = 0xacf;
    KeySym const KeySym_leftsinglequotemark                         = 0xad0;
    KeySym const KeySym_rightsinglequotemark                        = 0xad1;
    KeySym const KeySym_leftdoublequotemark                         = 0xad2;
    KeySym const KeySym_rightdoublequotemark                        = 0xad3;
    KeySym const KeySym_prescription                                = 0xad4;
    KeySym const KeySym_minutes                                     = 0xad6;
    KeySym const KeySym_seconds                                     = 0xad7;
    KeySym const KeySym_latincross                                  = 0xad9;
    KeySym const KeySym_hexagram                                    = 0xada;
    KeySym const KeySym_filledrectbullet                            = 0xadb;
    KeySym const KeySym_filledlefttribullet                         = 0xadc;
    KeySym const KeySym_filledrighttribullet                        = 0xadd;
    KeySym const KeySym_emfilledcircle                              = 0xade;
    KeySym const KeySym_emfilledrect                                = 0xadf;
    KeySym const KeySym_enopencircbullet                            = 0xae0;
    KeySym const KeySym_enopensquarebullet                          = 0xae1;
    KeySym const KeySym_openrectbullet                              = 0xae2;
    KeySym const KeySym_opentribulletup                             = 0xae3;
    KeySym const KeySym_opentribulletdown                           = 0xae4;
    KeySym const KeySym_openstar                                    = 0xae5;
    KeySym const KeySym_enfilledcircbullet                          = 0xae6;
    KeySym const KeySym_enfilledsqbullet                            = 0xae7;
    KeySym const KeySym_filledtribulletup                           = 0xae8;
    KeySym const KeySym_filledtribulletdown                         = 0xae9;
    KeySym const KeySym_leftpointer                                 = 0xaea;
    KeySym const KeySym_rightpointer                                = 0xaeb;
    KeySym const KeySym_club                                        = 0xaec;
    KeySym const KeySym_diamond                                     = 0xaed;
    KeySym const KeySym_heart                                       = 0xaee;
    KeySym const KeySym_maltesecross                                = 0xaf0;
    KeySym const KeySym_dagger                                      = 0xaf1;
    KeySym const KeySym_doubledagger                                = 0xaf2;
    KeySym const KeySym_checkmark                                   = 0xaf3;
    KeySym const KeySym_ballotcross                                 = 0xaf4;
    KeySym const KeySym_musicalsharp                                = 0xaf5;
    KeySym const KeySym_musicalflat                                 = 0xaf6;
    KeySym const KeySym_malesymbol                                  = 0xaf7;
    KeySym const KeySym_femalesymbol                                = 0xaf8;
    KeySym const KeySym_telephone                                   = 0xaf9;
    KeySym const KeySym_telephonerecorder                           = 0xafa;
    KeySym const KeySym_phonographcopyright                         = 0xafb;
    KeySym const KeySym_caret                                       = 0xafc;
    KeySym const KeySym_singlelowquotemark                          = 0xafd;
    KeySym const KeySym_doublelowquotemark                          = 0xafe;
    KeySym const KeySym_cursor                                      = 0xaff;


    /*
     *  APL
     *  Byte 3 = b
     */
    KeySym const KeySym_leftcaret                                   = 0xba3;
    KeySym const KeySym_rightcaret                                  = 0xba6;
    KeySym const KeySym_downcaret                                   = 0xba8;
    KeySym const KeySym_upcaret                                     = 0xba9;
    KeySym const KeySym_overbar                                     = 0xbc0;
    KeySym const KeySym_downtack                                    = 0xbc2;
    KeySym const KeySym_upshoe                                      = 0xbc3;
    KeySym const KeySym_downstile                                   = 0xbc4;
    KeySym const KeySym_underbar                                    = 0xbc6;
    KeySym const KeySym_jot                                         = 0xbca;
    KeySym const KeySym_quad                                        = 0xbcc;
    KeySym const KeySym_uptack                                      = 0xbce;
    KeySym const KeySym_circle                                      = 0xbcf;
    KeySym const KeySym_upstile                                     = 0xbd3;
    KeySym const KeySym_downshoe                                    = 0xbd6;
    KeySym const KeySym_rightshoe                                   = 0xbd8;
    KeySym const KeySym_leftshoe                                    = 0xbda;
    KeySym const KeySym_lefttack                                    = 0xbdc;
    KeySym const KeySym_righttack                                   = 0xbfc;


    /*
     * Hebrew
     * Byte 3 = c
     */
    KeySym const KeySym_hebrew_doublelowline                        = 0xcdf;
    KeySym const KeySym_hebrew_aleph                                = 0xce0;
    KeySym const KeySym_hebrew_bet                                  = 0xce1;
    KeySym const KeySym_hebrew_gimel                                = 0xce2;
    KeySym const KeySym_hebrew_dalet                                = 0xce3;
    KeySym const KeySym_hebrew_he                                   = 0xce4;
    KeySym const KeySym_hebrew_waw                                  = 0xce5;
    KeySym const KeySym_hebrew_zain                                 = 0xce6;
    KeySym const KeySym_hebrew_chet                                 = 0xce7;
    KeySym const KeySym_hebrew_tet                                  = 0xce8;
    KeySym const KeySym_hebrew_yod                                  = 0xce9;
    KeySym const KeySym_hebrew_finalkaph                            = 0xcea;
    KeySym const KeySym_hebrew_kaph                                 = 0xceb;
    KeySym const KeySym_hebrew_lamed                                = 0xcec;
    KeySym const KeySym_hebrew_finalmem                             = 0xced;
    KeySym const KeySym_hebrew_mem                                  = 0xcee;
    KeySym const KeySym_hebrew_finalnun                             = 0xcef;
    KeySym const KeySym_hebrew_nun                                  = 0xcf0;
    KeySym const KeySym_hebrew_samech                               = 0xcf1;
    KeySym const KeySym_hebrew_ayin                                 = 0xcf2;
    KeySym const KeySym_hebrew_finalpe                              = 0xcf3;
    KeySym const KeySym_hebrew_pe                                   = 0xcf4;
    KeySym const KeySym_hebrew_finalzade                            = 0xcf5;
    KeySym const KeySym_hebrew_zade                                 = 0xcf6;
    KeySym const KeySym_hebrew_qoph                                 = 0xcf7;
    KeySym const KeySym_hebrew_resh                                 = 0xcf8;
    KeySym const KeySym_hebrew_shin                                 = 0xcf9;
    KeySym const KeySym_hebrew_taw                                  = 0xcfa;
    KeySym const KeySym_Hebrew_switch                               = 0xFF7E; // Alias for mode_switch


    /*
     * Thai
     * Byte 3 = d
     */
    KeySym const KeySym_Thai_kokai             = 0xda1;
    KeySym const KeySym_Thai_khokhai           = 0xda2;
    KeySym const KeySym_Thai_khokhuat          = 0xda3;
    KeySym const KeySym_Thai_khokhwai          = 0xda4;
    KeySym const KeySym_Thai_khokhon           = 0xda5;
    KeySym const KeySym_Thai_khorakhang        = 0xda6;  
    KeySym const KeySym_Thai_ngongu            = 0xda7;  
    KeySym const KeySym_Thai_chochan           = 0xda8;  
    KeySym const KeySym_Thai_choching          = 0xda9;   
    KeySym const KeySym_Thai_chochang          = 0xdaa;  
    KeySym const KeySym_Thai_soso              = 0xdab;
    KeySym const KeySym_Thai_chochoe           = 0xdac;
    KeySym const KeySym_Thai_yoying            = 0xdad;
    KeySym const KeySym_Thai_dochada           = 0xdae;
    KeySym const KeySym_Thai_topatak           = 0xdaf;
    KeySym const KeySym_Thai_thothan           = 0xdb0;
    KeySym const KeySym_Thai_thonangmontho     = 0xdb1;
    KeySym const KeySym_Thai_thophuthao        = 0xdb2;
    KeySym const KeySym_Thai_nonen             = 0xdb3;
    KeySym const KeySym_Thai_dodek             = 0xdb4;
    KeySym const KeySym_Thai_totao             = 0xdb5;
    KeySym const KeySym_Thai_thothung          = 0xdb6;
    KeySym const KeySym_Thai_thothahan         = 0xdb7;
    KeySym const KeySym_Thai_thothong          = 0xdb8;
    KeySym const KeySym_Thai_nonu              = 0xdb9;
    KeySym const KeySym_Thai_bobaimai          = 0xdba;
    KeySym const KeySym_Thai_popla             = 0xdbb;
    KeySym const KeySym_Thai_phophung          = 0xdbc;
    KeySym const KeySym_Thai_fofa              = 0xdbd;
    KeySym const KeySym_Thai_phophan           = 0xdbe;
    KeySym const KeySym_Thai_fofan             = 0xdbf;
    KeySym const KeySym_Thai_phosamphao        = 0xdc0;
    KeySym const KeySym_Thai_moma              = 0xdc1;
    KeySym const KeySym_Thai_yoyak             = 0xdc2;
    KeySym const KeySym_Thai_rorua             = 0xdc3;
    KeySym const KeySym_Thai_ru                = 0xdc4;
    KeySym const KeySym_Thai_loling            = 0xdc5;
    KeySym const KeySym_Thai_lu                = 0xdc6;
    KeySym const KeySym_Thai_wowaen            = 0xdc7;
    KeySym const KeySym_Thai_sosala            = 0xdc8;
    KeySym const KeySym_Thai_sorusi            = 0xdc9;
    KeySym const KeySym_Thai_sosua             = 0xdca;
    KeySym const KeySym_Thai_hohip             = 0xdcb;
    KeySym const KeySym_Thai_lochula           = 0xdcc;
    KeySym const KeySym_Thai_oang              = 0xdcd;
    KeySym const KeySym_Thai_honokhuk          = 0xdce;
    KeySym const KeySym_Thai_paiyannoi         = 0xdcf;
    KeySym const KeySym_Thai_saraa             = 0xdd0;
    KeySym const KeySym_Thai_maihanakat        = 0xdd1;
    KeySym const KeySym_Thai_saraaa            = 0xdd2;
    KeySym const KeySym_Thai_saraam            = 0xdd3;
    KeySym const KeySym_Thai_sarai             = 0xdd4;   
    KeySym const KeySym_Thai_saraii            = 0xdd5;   
    KeySym const KeySym_Thai_saraue            = 0xdd6;    
    KeySym const KeySym_Thai_sarauee           = 0xdd7;    
    KeySym const KeySym_Thai_sarau             = 0xdd8;    
    KeySym const KeySym_Thai_sarauu            = 0xdd9;   
    KeySym const KeySym_Thai_phinthu           = 0xdda;
    KeySym const KeySym_Thai_maihanakat_maitho = 0xdde;
    KeySym const KeySym_Thai_baht              = 0xddf;
    KeySym const KeySym_Thai_sarae             = 0xde0;    
    KeySym const KeySym_Thai_saraae            = 0xde1;
    KeySym const KeySym_Thai_sarao             = 0xde2;
    KeySym const KeySym_Thai_saraaimaimuan     = 0xde3;   
    KeySym const KeySym_Thai_saraaimaimalai    = 0xde4;  
    KeySym const KeySym_Thai_lakkhangyao       = 0xde5;
    KeySym const KeySym_Thai_maiyamok          = 0xde6;
    KeySym const KeySym_Thai_maitaikhu         = 0xde7;
    KeySym const KeySym_Thai_maiek             = 0xde8;   
    KeySym const KeySym_Thai_maitho            = 0xde9;
    KeySym const KeySym_Thai_maitri            = 0xdea;
    KeySym const KeySym_Thai_maichattawa       = 0xdeb;
    KeySym const KeySym_Thai_thanthakhat       = 0xdec;
    KeySym const KeySym_Thai_nikhahit          = 0xded;
    KeySym const KeySym_Thai_leksun            = 0xdf0; 
    KeySym const KeySym_Thai_leknung           = 0xdf1;  
    KeySym const KeySym_Thai_leksong           = 0xdf2; 
    KeySym const KeySym_Thai_leksam            = 0xdf3;
    KeySym const KeySym_Thai_leksi             = 0xdf4;  
    KeySym const KeySym_Thai_lekha             = 0xdf5;  
    KeySym const KeySym_Thai_lekhok            = 0xdf6;  
    KeySym const KeySym_Thai_lekchet           = 0xdf7;  
    KeySym const KeySym_Thai_lekpaet           = 0xdf8;  
    KeySym const KeySym_Thai_lekkao            = 0xdf9; 


    /*
     *   Korean
     *   Byte 3 = e
     */
    KeySym const KeySym_Hangul                     = 0xff31; // Hangul start/stop(toggle)
    KeySym const KeySym_Hangul_Start               = 0xff32; // Hangul start
    KeySym const KeySym_Hangul_End                 = 0xff33; // Hangul end, English start
    KeySym const KeySym_Hangul_Hanja               = 0xff34; // Start Hangul->Hanja Conversion
    KeySym const KeySym_Hangul_Jamo                = 0xff35; // Hangul Jamo mode
    KeySym const KeySym_Hangul_Romaja              = 0xff36; // Hangul Romaja mode
    KeySym const KeySym_Hangul_Codeinput           = 0xff37; // Hangul code input mode (Alias for Codeinput)
    KeySym const KeySym_Hangul_Jeonja              = 0xff38; // Jeonja mode
    KeySym const KeySym_Hangul_Banja               = 0xff39; // Banja mode
    KeySym const KeySym_Hangul_PreHanja            = 0xff3a; // Pre Hanja conversion
    KeySym const KeySym_Hangul_PostHanja           = 0xff3b; // Post Hanja conversion
    KeySym const KeySym_Hangul_SingleCandidate     = 0xff3c; // Alias for SingleCandidate
    KeySym const KeySym_Hangul_MultipleCandidate   = 0xff3d; // Alias for MultipleCandidate
    KeySym const KeySym_Hangul_PreviousCandidate   = 0xff3e; // Alias for PreviousCandidate
    KeySym const KeySym_Hangul_Special             = 0xff3f; // Special symbols
    KeySym const KeySym_Hangul_switch              = 0xFF7E; // Alias for mode_switch
    // Hangul Consonant Characters
    KeySym const KeySym_Hangul_Kiyeog              = 0xea1;
    KeySym const KeySym_Hangul_SsangKiyeog         = 0xea2;
    KeySym const KeySym_Hangul_KiyeogSios          = 0xea3;
    KeySym const KeySym_Hangul_Nieun               = 0xea4;
    KeySym const KeySym_Hangul_NieunJieuj          = 0xea5;
    KeySym const KeySym_Hangul_NieunHieuh          = 0xea6;
    KeySym const KeySym_Hangul_Dikeud              = 0xea7;
    KeySym const KeySym_Hangul_SsangDikeud         = 0xea8;
    KeySym const KeySym_Hangul_Rieul               = 0xea9;
    KeySym const KeySym_Hangul_RieulKiyeog         = 0xeaa;
    KeySym const KeySym_Hangul_RieulMieum          = 0xeab;
    KeySym const KeySym_Hangul_RieulPieub          = 0xeac;
    KeySym const KeySym_Hangul_RieulSios           = 0xead;
    KeySym const KeySym_Hangul_RieulTieut          = 0xeae;
    KeySym const KeySym_Hangul_RieulPhieuf         = 0xeaf;
    KeySym const KeySym_Hangul_RieulHieuh          = 0xeb0;
    KeySym const KeySym_Hangul_Mieum               = 0xeb1;
    KeySym const KeySym_Hangul_Pieub               = 0xeb2;
    KeySym const KeySym_Hangul_SsangPieub          = 0xeb3;
    KeySym const KeySym_Hangul_PieubSios           = 0xeb4;
    KeySym const KeySym_Hangul_Sios                = 0xeb5;
    KeySym const KeySym_Hangul_SsangSios           = 0xeb6;
    KeySym const KeySym_Hangul_Ieung               = 0xeb7;
    KeySym const KeySym_Hangul_Jieuj               = 0xeb8;
    KeySym const KeySym_Hangul_SsangJieuj          = 0xeb9;
    KeySym const KeySym_Hangul_Cieuc               = 0xeba;
    KeySym const KeySym_Hangul_Khieuq              = 0xebb;
    KeySym const KeySym_Hangul_Tieut               = 0xebc;
    KeySym const KeySym_Hangul_Phieuf              = 0xebd;
    KeySym const KeySym_Hangul_Hieuh               = 0xebe;
    // Hangul Vowel Characters
    KeySym const KeySym_Hangul_A                   = 0xebf;
    KeySym const KeySym_Hangul_AE                  = 0xec0;
    KeySym const KeySym_Hangul_YA                  = 0xec1;
    KeySym const KeySym_Hangul_YAE                 = 0xec2;
    KeySym const KeySym_Hangul_EO                  = 0xec3;
    KeySym const KeySym_Hangul_E                   = 0xec4;
    KeySym const KeySym_Hangul_YEO                 = 0xec5;
    KeySym const KeySym_Hangul_YE                  = 0xec6;
    KeySym const KeySym_Hangul_O                   = 0xec7;
    KeySym const KeySym_Hangul_WA                  = 0xec8;
    KeySym const KeySym_Hangul_WAE                 = 0xec9;
    KeySym const KeySym_Hangul_OE                  = 0xeca;
    KeySym const KeySym_Hangul_YO                  = 0xecb;
    KeySym const KeySym_Hangul_U                   = 0xecc;
    KeySym const KeySym_Hangul_WEO                 = 0xecd;
    KeySym const KeySym_Hangul_WE                  = 0xece;
    KeySym const KeySym_Hangul_WI                  = 0xecf;
    KeySym const KeySym_Hangul_YU                  = 0xed0;
    KeySym const KeySym_Hangul_EU                  = 0xed1;
    KeySym const KeySym_Hangul_YI                  = 0xed2;
    KeySym const KeySym_Hangul_I                   = 0xed3;
    // Hangul syllable-final (JongSeong) Characters
    KeySym const KeySym_Hangul_J_Kiyeog            = 0xed4;
    KeySym const KeySym_Hangul_J_SsangKiyeog       = 0xed5;
    KeySym const KeySym_Hangul_J_KiyeogSios        = 0xed6;
    KeySym const KeySym_Hangul_J_Nieun             = 0xed7;
    KeySym const KeySym_Hangul_J_NieunJieuj        = 0xed8;
    KeySym const KeySym_Hangul_J_NieunHieuh        = 0xed9;
    KeySym const KeySym_Hangul_J_Dikeud            = 0xeda;
    KeySym const KeySym_Hangul_J_Rieul             = 0xedb;
    KeySym const KeySym_Hangul_J_RieulKiyeog       = 0xedc;
    KeySym const KeySym_Hangul_J_RieulMieum        = 0xedd;
    KeySym const KeySym_Hangul_J_RieulPieub        = 0xede;
    KeySym const KeySym_Hangul_J_RieulSios         = 0xedf;
    KeySym const KeySym_Hangul_J_RieulTieut        = 0xee0;
    KeySym const KeySym_Hangul_J_RieulPhieuf       = 0xee1;
    KeySym const KeySym_Hangul_J_RieulHieuh        = 0xee2;
    KeySym const KeySym_Hangul_J_Mieum             = 0xee3;
    KeySym const KeySym_Hangul_J_Pieub             = 0xee4;
    KeySym const KeySym_Hangul_J_PieubSios         = 0xee5;
    KeySym const KeySym_Hangul_J_Sios              = 0xee6;
    KeySym const KeySym_Hangul_J_SsangSios         = 0xee7;
    KeySym const KeySym_Hangul_J_Ieung             = 0xee8;
    KeySym const KeySym_Hangul_J_Jieuj             = 0xee9;
    KeySym const KeySym_Hangul_J_Cieuc             = 0xeea;
    KeySym const KeySym_Hangul_J_Khieuq            = 0xeeb;
    KeySym const KeySym_Hangul_J_Tieut             = 0xeec;
    KeySym const KeySym_Hangul_J_Phieuf            = 0xeed;
    KeySym const KeySym_Hangul_J_Hieuh             = 0xeee;
    // Ancient Hangul Consonant Characters
    KeySym const KeySym_Hangul_RieulYeorinHieuh    = 0xeef;
    KeySym const KeySym_Hangul_SunkyeongeumMieum   = 0xef0;
    KeySym const KeySym_Hangul_SunkyeongeumPieub   = 0xef1;
    KeySym const KeySym_Hangul_PanSios             = 0xef2;
    KeySym const KeySym_Hangul_KkogjiDalrinIeung   = 0xef3;
    KeySym const KeySym_Hangul_SunkyeongeumPhieuf  = 0xef4;
    KeySym const KeySym_Hangul_YeorinHieuh         = 0xef5;
    // Ancient Hangul Vowel Characters
    KeySym const KeySym_Hangul_AraeA               = 0xef6;
    KeySym const KeySym_Hangul_AraeAE              = 0xef7;
    // Ancient Hangul syllable-final (JongSeong) Characters
    KeySym const KeySym_Hangul_J_PanSios           = 0xef8;
    KeySym const KeySym_Hangul_J_KkogjiDalrinIeung = 0xef9;
    KeySym const KeySym_Hangul_J_YeorinHieuh       = 0xefa;
    // Korean currency symbol
    KeySym const KeySym_Korean_Won                 = 0xeff;


    /*
     *   Armenian
     *   Byte 3 = 0x14
     */
    KeySym const KeySym_Armenian_eternity        = 0x14a1;
    KeySym const KeySym_Armenian_ligature_ew     = 0x14a2;
    KeySym const KeySym_Armenian_full_stop       = 0x14a3;
    KeySym const KeySym_Armenian_verjaket        = 0x14a3; // Alias for Armenian_full_stop
    KeySym const KeySym_Armenian_parenright      = 0x14a4;
    KeySym const KeySym_Armenian_parenleft       = 0x14a5;
    KeySym const KeySym_Armenian_guillemotright  = 0x14a6;
    KeySym const KeySym_Armenian_guillemotleft   = 0x14a7;
    KeySym const KeySym_Armenian_em_dash         = 0x14a8;
    KeySym const KeySym_Armenian_dot             = 0x14a9;
    KeySym const KeySym_Armenian_mijaket         = 0x14a9; // Alias for Armenian_dot
    KeySym const KeySym_Armenian_separation_mark = 0x14aa;
    KeySym const KeySym_Armenian_but             = 0x14aa; // Alias for Armenian_separation_mark
    KeySym const KeySym_Armenian_comma           = 0x14ab;
    KeySym const KeySym_Armenian_en_dash         = 0x14ac;
    KeySym const KeySym_Armenian_hyphen          = 0x14ad;
    KeySym const KeySym_Armenian_yentamna        = 0x14ad; // Alias for Armenian_hyphen
    KeySym const KeySym_Armenian_ellipsis        = 0x14ae;
    KeySym const KeySym_Armenian_exclam          = 0x14af;
    KeySym const KeySym_Armenian_amanak          = 0x14af; // Alias for Armenian_exclam
    KeySym const KeySym_Armenian_accent          = 0x14b0;
    KeySym const KeySym_Armenian_shesht          = 0x14b0; // Alias for Armenian_accent
    KeySym const KeySym_Armenian_question        = 0x14b1;
    KeySym const KeySym_Armenian_paruyk          = 0x14b1; // Alias for Armenian_question
    KeySym const KeySym_Armenian_AYB             = 0x14b2;
    KeySym const KeySym_Armenian_ayb             = 0x14b3;
    KeySym const KeySym_Armenian_BEN             = 0x14b4;
    KeySym const KeySym_Armenian_ben             = 0x14b5;
    KeySym const KeySym_Armenian_GIM             = 0x14b6;
    KeySym const KeySym_Armenian_gim             = 0x14b7;
    KeySym const KeySym_Armenian_DA              = 0x14b8;
    KeySym const KeySym_Armenian_da              = 0x14b9;
    KeySym const KeySym_Armenian_YECH            = 0x14ba;
    KeySym const KeySym_Armenian_yech            = 0x14bb;
    KeySym const KeySym_Armenian_ZA              = 0x14bc;
    KeySym const KeySym_Armenian_za              = 0x14bd;
    KeySym const KeySym_Armenian_E               = 0x14be;
    KeySym const KeySym_Armenian_e               = 0x14bf;
    KeySym const KeySym_Armenian_AT              = 0x14c0;
    KeySym const KeySym_Armenian_at              = 0x14c1;
    KeySym const KeySym_Armenian_TO              = 0x14c2;
    KeySym const KeySym_Armenian_to              = 0x14c3;
    KeySym const KeySym_Armenian_ZHE             = 0x14c4;
    KeySym const KeySym_Armenian_zhe             = 0x14c5;
    KeySym const KeySym_Armenian_INI             = 0x14c6;
    KeySym const KeySym_Armenian_ini             = 0x14c7;
    KeySym const KeySym_Armenian_LYUN            = 0x14c8;
    KeySym const KeySym_Armenian_lyun            = 0x14c9;
    KeySym const KeySym_Armenian_KHE             = 0x14ca;
    KeySym const KeySym_Armenian_khe             = 0x14cb;
    KeySym const KeySym_Armenian_TSA             = 0x14cc;
    KeySym const KeySym_Armenian_tsa             = 0x14cd;
    KeySym const KeySym_Armenian_KEN             = 0x14ce;
    KeySym const KeySym_Armenian_ken             = 0x14cf;
    KeySym const KeySym_Armenian_HO              = 0x14d0;
    KeySym const KeySym_Armenian_ho              = 0x14d1;
    KeySym const KeySym_Armenian_DZA             = 0x14d2;
    KeySym const KeySym_Armenian_dza             = 0x14d3;
    KeySym const KeySym_Armenian_GHAT            = 0x14d4;
    KeySym const KeySym_Armenian_ghat            = 0x14d5;
    KeySym const KeySym_Armenian_TCHE            = 0x14d6;
    KeySym const KeySym_Armenian_tche            = 0x14d7;
    KeySym const KeySym_Armenian_MEN             = 0x14d8;
    KeySym const KeySym_Armenian_men             = 0x14d9;
    KeySym const KeySym_Armenian_HI              = 0x14da;
    KeySym const KeySym_Armenian_hi              = 0x14db;
    KeySym const KeySym_Armenian_NU              = 0x14dc;
    KeySym const KeySym_Armenian_nu              = 0x14dd;
    KeySym const KeySym_Armenian_SHA             = 0x14de;
    KeySym const KeySym_Armenian_sha             = 0x14df;
    KeySym const KeySym_Armenian_VO              = 0x14e0;
    KeySym const KeySym_Armenian_vo              = 0x14e1;
    KeySym const KeySym_Armenian_CHA             = 0x14e2;
    KeySym const KeySym_Armenian_cha             = 0x14e3;
    KeySym const KeySym_Armenian_PE              = 0x14e4;
    KeySym const KeySym_Armenian_pe              = 0x14e5;
    KeySym const KeySym_Armenian_JE              = 0x14e6;
    KeySym const KeySym_Armenian_je              = 0x14e7;
    KeySym const KeySym_Armenian_RA              = 0x14e8;
    KeySym const KeySym_Armenian_ra              = 0x14e9;
    KeySym const KeySym_Armenian_SE              = 0x14ea;
    KeySym const KeySym_Armenian_se              = 0x14eb;
    KeySym const KeySym_Armenian_VEV             = 0x14ec;
    KeySym const KeySym_Armenian_vev             = 0x14ed;
    KeySym const KeySym_Armenian_TYUN            = 0x14ee;
    KeySym const KeySym_Armenian_tyun            = 0x14ef;
    KeySym const KeySym_Armenian_RE              = 0x14f0;
    KeySym const KeySym_Armenian_re              = 0x14f1;
    KeySym const KeySym_Armenian_TSO             = 0x14f2;
    KeySym const KeySym_Armenian_tso             = 0x14f3;
    KeySym const KeySym_Armenian_VYUN            = 0x14f4;
    KeySym const KeySym_Armenian_vyun            = 0x14f5;
    KeySym const KeySym_Armenian_PYUR            = 0x14f6;
    KeySym const KeySym_Armenian_pyur            = 0x14f7;
    KeySym const KeySym_Armenian_KE              = 0x14f8;
    KeySym const KeySym_Armenian_ke              = 0x14f9;
    KeySym const KeySym_Armenian_O               = 0x14fa;
    KeySym const KeySym_Armenian_o               = 0x14fb;
    KeySym const KeySym_Armenian_FE              = 0x14fc;
    KeySym const KeySym_Armenian_fe              = 0x14fd;
    KeySym const KeySym_Armenian_apostrophe      = 0x14fe;
    KeySym const KeySym_Armenian_section_sign    = 0x14ff;


    /*
     *   Georgian
     *   Byte 3 = 0x15
     */
    KeySym const KeySym_Georgian_an      = 0x15d0;
    KeySym const KeySym_Georgian_ban     = 0x15d1;
    KeySym const KeySym_Georgian_gan     = 0x15d2;
    KeySym const KeySym_Georgian_don     = 0x15d3;
    KeySym const KeySym_Georgian_en      = 0x15d4;
    KeySym const KeySym_Georgian_vin     = 0x15d5;
    KeySym const KeySym_Georgian_zen     = 0x15d6;
    KeySym const KeySym_Georgian_tan     = 0x15d7;
    KeySym const KeySym_Georgian_in      = 0x15d8;
    KeySym const KeySym_Georgian_kan     = 0x15d9;
    KeySym const KeySym_Georgian_las     = 0x15da;
    KeySym const KeySym_Georgian_man     = 0x15db;
    KeySym const KeySym_Georgian_nar     = 0x15dc;
    KeySym const KeySym_Georgian_on      = 0x15dd;
    KeySym const KeySym_Georgian_par     = 0x15de;
    KeySym const KeySym_Georgian_zhar    = 0x15df;
    KeySym const KeySym_Georgian_rae     = 0x15e0;
    KeySym const KeySym_Georgian_san     = 0x15e1;
    KeySym const KeySym_Georgian_tar     = 0x15e2;
    KeySym const KeySym_Georgian_un      = 0x15e3;
    KeySym const KeySym_Georgian_phar    = 0x15e4;
    KeySym const KeySym_Georgian_khar    = 0x15e5;
    KeySym const KeySym_Georgian_ghan    = 0x15e6;
    KeySym const KeySym_Georgian_qar     = 0x15e7;
    KeySym const KeySym_Georgian_shin    = 0x15e8;
    KeySym const KeySym_Georgian_chin    = 0x15e9;
    KeySym const KeySym_Georgian_can     = 0x15ea;
    KeySym const KeySym_Georgian_jil     = 0x15eb;
    KeySym const KeySym_Georgian_cil     = 0x15ec;
    KeySym const KeySym_Georgian_char    = 0x15ed;
    KeySym const KeySym_Georgian_xan     = 0x15ee;
    KeySym const KeySym_Georgian_jhan    = 0x15ef;
    KeySym const KeySym_Georgian_hae     = 0x15f0;
    KeySym const KeySym_Georgian_he      = 0x15f1;
    KeySym const KeySym_Georgian_hie     = 0x15f2;
    KeySym const KeySym_Georgian_we      = 0x15f3;
    KeySym const KeySym_Georgian_har     = 0x15f4;
    KeySym const KeySym_Georgian_hoe     = 0x15f5;
    KeySym const KeySym_Georgian_fi      = 0x15f6;


    /*
     * Azeri (and other Turkic or Caucasian languages of ex-USSR)
     * Byte 3 = 0x16
     */
    // latin
    KeySym const KeySym_Ccedillaabovedot = 0x16a2;
    KeySym const KeySym_Xabovedot        = 0x16a3;
    KeySym const KeySym_Qabovedot        = 0x16a5;
    KeySym const KeySym_Ibreve           = 0x16a6;
    KeySym const KeySym_IE               = 0x16a7;
    KeySym const KeySym_UO               = 0x16a8;
    KeySym const KeySym_Zstroke          = 0x16a9;
    KeySym const KeySym_Gcaron           = 0x16aa;
    KeySym const KeySym_Obarred          = 0x16af;
    KeySym const KeySym_ccedillaabovedot = 0x16b2;
    KeySym const KeySym_xabovedot        = 0x16b3;
    KeySym const KeySym_Ocaron           = 0x16b4;
    KeySym const KeySym_qabovedot        = 0x16b5;
    KeySym const KeySym_ibreve           = 0x16b6;
    KeySym const KeySym_ie               = 0x16b7;
    KeySym const KeySym_uo               = 0x16b8;
    KeySym const KeySym_zstroke          = 0x16b9;
    KeySym const KeySym_gcaron           = 0x16ba;
    KeySym const KeySym_ocaron           = 0x16bd;
    KeySym const KeySym_obarred          = 0x16bf;
    KeySym const KeySym_SCHWA            = 0x16c6;
    KeySym const KeySym_schwa            = 0x16f6;
    // those are not really Caucasus, but I put them here for now
    // For Inupiak
    KeySym const KeySym_Lbelowdot        = 0x16d1;
    KeySym const KeySym_Lstrokebelowdot  = 0x16d2;
    KeySym const KeySym_lbelowdot        = 0x16e1;
    KeySym const KeySym_lstrokebelowdot  = 0x16e2;
    // For Guarani
    KeySym const KeySym_Gtilde           = 0x16d3;
    KeySym const KeySym_gtilde           = 0x16e3;


    /*
     *   Vietnamese
     *   Byte 3 = 0x1e
     */
    KeySym const KeySym_Abelowdot           = 0x1ea0;
    KeySym const KeySym_abelowdot           = 0x1ea1;
    KeySym const KeySym_Ahook               = 0x1ea2;
    KeySym const KeySym_ahook               = 0x1ea3;
    KeySym const KeySym_Acircumflexacute    = 0x1ea4;
    KeySym const KeySym_acircumflexacute    = 0x1ea5;
    KeySym const KeySym_Acircumflexgrave    = 0x1ea6;
    KeySym const KeySym_acircumflexgrave    = 0x1ea7;
    KeySym const KeySym_Acircumflexhook     = 0x1ea8;
    KeySym const KeySym_acircumflexhook     = 0x1ea9;
    KeySym const KeySym_Acircumflextilde    = 0x1eaa;
    KeySym const KeySym_acircumflextilde    = 0x1eab;
    KeySym const KeySym_Acircumflexbelowdot = 0x1eac;
    KeySym const KeySym_acircumflexbelowdot = 0x1ead;
    KeySym const KeySym_Abreveacute         = 0x1eae;
    KeySym const KeySym_abreveacute         = 0x1eaf;
    KeySym const KeySym_Abrevegrave         = 0x1eb0;
    KeySym const KeySym_abrevegrave         = 0x1eb1;
    KeySym const KeySym_Abrevehook          = 0x1eb2;
    KeySym const KeySym_abrevehook          = 0x1eb3;
    KeySym const KeySym_Abrevetilde         = 0x1eb4;
    KeySym const KeySym_abrevetilde         = 0x1eb5;
    KeySym const KeySym_Abrevebelowdot      = 0x1eb6;
    KeySym const KeySym_abrevebelowdot      = 0x1eb7;
    KeySym const KeySym_Ebelowdot           = 0x1eb8;
    KeySym const KeySym_ebelowdot           = 0x1eb9;
    KeySym const KeySym_Ehook               = 0x1eba;
    KeySym const KeySym_ehook               = 0x1ebb;
    KeySym const KeySym_Etilde              = 0x1ebc;
    KeySym const KeySym_etilde              = 0x1ebd;
    KeySym const KeySym_Ecircumflexacute    = 0x1ebe;
    KeySym const KeySym_ecircumflexacute    = 0x1ebf;
    KeySym const KeySym_Ecircumflexgrave    = 0x1ec0;
    KeySym const KeySym_ecircumflexgrave    = 0x1ec1;
    KeySym const KeySym_Ecircumflexhook     = 0x1ec2;
    KeySym const KeySym_ecircumflexhook     = 0x1ec3;
    KeySym const KeySym_Ecircumflextilde    = 0x1ec4;
    KeySym const KeySym_ecircumflextilde    = 0x1ec5;
    KeySym const KeySym_Ecircumflexbelowdot = 0x1ec6;
    KeySym const KeySym_ecircumflexbelowdot = 0x1ec7;
    KeySym const KeySym_Ihook               = 0x1ec8;
    KeySym const KeySym_ihook               = 0x1ec9;
    KeySym const KeySym_Ibelowdot           = 0x1eca;
    KeySym const KeySym_ibelowdot           = 0x1ecb;
    KeySym const KeySym_Obelowdot           = 0x1ecc;
    KeySym const KeySym_obelowdot           = 0x1ecd;
    KeySym const KeySym_Ohook               = 0x1ece;
    KeySym const KeySym_ohook               = 0x1ecf;
    KeySym const KeySym_Ocircumflexacute    = 0x1ed0;
    KeySym const KeySym_ocircumflexacute    = 0x1ed1;
    KeySym const KeySym_Ocircumflexgrave    = 0x1ed2;
    KeySym const KeySym_ocircumflexgrave    = 0x1ed3;
    KeySym const KeySym_Ocircumflexhook     = 0x1ed4;
    KeySym const KeySym_ocircumflexhook     = 0x1ed5;
    KeySym const KeySym_Ocircumflextilde    = 0x1ed6;
    KeySym const KeySym_ocircumflextilde    = 0x1ed7;
    KeySym const KeySym_Ocircumflexbelowdot = 0x1ed8;
    KeySym const KeySym_ocircumflexbelowdot = 0x1ed9;
    KeySym const KeySym_Ohornacute          = 0x1eda;
    KeySym const KeySym_ohornacute          = 0x1edb;
    KeySym const KeySym_Ohorngrave          = 0x1edc;
    KeySym const KeySym_ohorngrave          = 0x1edd;
    KeySym const KeySym_Ohornhook           = 0x1ede;
    KeySym const KeySym_ohornhook           = 0x1edf;
    KeySym const KeySym_Ohorntilde          = 0x1ee0;
    KeySym const KeySym_ohorntilde          = 0x1ee1;
    KeySym const KeySym_Ohornbelowdot       = 0x1ee2;
    KeySym const KeySym_ohornbelowdot       = 0x1ee3;
    KeySym const KeySym_Ubelowdot           = 0x1ee4;
    KeySym const KeySym_ubelowdot           = 0x1ee5;
    KeySym const KeySym_Uhook               = 0x1ee6;
    KeySym const KeySym_uhook               = 0x1ee7;
    KeySym const KeySym_Uhornacute          = 0x1ee8;
    KeySym const KeySym_uhornacute          = 0x1ee9;
    KeySym const KeySym_Uhorngrave          = 0x1eea;
    KeySym const KeySym_uhorngrave          = 0x1eeb;
    KeySym const KeySym_Uhornhook           = 0x1eec;
    KeySym const KeySym_uhornhook           = 0x1eed;
    KeySym const KeySym_Uhorntilde          = 0x1eee;
    KeySym const KeySym_uhorntilde          = 0x1eef;
    KeySym const KeySym_Uhornbelowdot       = 0x1ef0;
    KeySym const KeySym_uhornbelowdot       = 0x1ef1;
    KeySym const KeySym_Ybelowdot           = 0x1ef4;
    KeySym const KeySym_ybelowdot           = 0x1ef5;
    KeySym const KeySym_Yhook               = 0x1ef6;
    KeySym const KeySym_yhook               = 0x1ef7;
    KeySym const KeySym_Ytilde              = 0x1ef8;
    KeySym const KeySym_ytilde              = 0x1ef9;
    KeySym const KeySym_Ohorn               = 0x1efa; // U+01a0
    KeySym const KeySym_ohorn               = 0x1efb; // U+01a1
    KeySym const KeySym_Uhorn               = 0x1efc; // U+01af
    KeySym const KeySym_uhorn               = 0x1efd; // U+01b0

    KeySym const KeySym_combining_tilde     = 0x1e9f; // U+0303
    KeySym const KeySym_combining_grave     = 0x1ef2; // U+0300
    KeySym const KeySym_combining_acute     = 0x1ef3; // U+0301
    KeySym const KeySym_combining_hook      = 0x1efe; // U+0309
    KeySym const KeySym_combining_belowdot  = 0x1eff; // U+0323


    /**
     * Currency
     */
    KeySym const KeySym_EcuSign       = 0x20a0;
    KeySym const KeySym_ColonSign     = 0x20a1;
    KeySym const KeySym_CruzeiroSign  = 0x20a2;
    KeySym const KeySym_FFrancSign    = 0x20a3;
    KeySym const KeySym_LiraSign      = 0x20a4;
    KeySym const KeySym_MillSign      = 0x20a5;
    KeySym const KeySym_NairaSign     = 0x20a6;
    KeySym const KeySym_PesetaSign    = 0x20a7;
    KeySym const KeySym_RupeeSign     = 0x20a8;
    KeySym const KeySym_WonSign       = 0x20a9;
    KeySym const KeySym_NewSheqelSign = 0x20aa;
    KeySym const KeySym_DongSign      = 0x20ab;
    KeySym const KeySym_EuroSign      = 0x20ac;
  }
}

#endif // ARCHON_DISPLAY_KEYSYMS_HPP
