// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_RENDER_X_KEY_BINDING_SUPPORT_HPP
#define ARCHON_X_RENDER_X_KEY_BINDING_SUPPORT_HPP

/// \file


#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/mouse_button.hpp>


namespace archon::render {


/// \brief Unified key identifier.
///
/// The purpose of this type is to unify the specification of keys of different type, i.e.,
/// keyboard keys and mouse buttons, and to allow for specification of keys in different
/// ways, i.e., via implementation independent or dependent key codes (\ref display::Key or
/// \ref display::KeyCode).
///
class KeyIdent {
public:
    /// \brief Construct unified key identifier from well-known key.
    ///
    /// This constructor constructs a unified key identifier from the specified well-known
    /// key (\ref display::Key).
    ///
    constexpr KeyIdent(display::Key key) noexcept;

    /// \brief Construct unified key identifier from key code.
    ///
    /// This constructor constructs a unified key identifier from the specified key code
    /// (\ref display::KeyCode). The key code value for a particular key will generally
    /// depend on the display implementation (\ref display::Implementation).
    ///
    constexpr KeyIdent(display::KeyCode key_code) noexcept;

    /// \brief Construct unified key identifier from mouse button identifier.
    ///
    /// This constructor constructs a unified key identifier from a mouse button identifier
    /// (\ref display::MouseButton).
    ///
    constexpr KeyIdent(display::MouseButton mouse_button) noexcept;

    /// \brief Type of stored key identifier.
    ///
    /// These are the possible key identifier types that can be stored in a unified key
    /// identifier.
    ///
    enum class Type {
        key,
        key_code,
        mouse_button,
    };

    /// \brief Get type and value of stored key identifier.
    ///
    /// This function returns the type of the stored key identifier, as well as the stored
    /// identifier itself. If the type is `key` (`Type::key`), \p key is set to the stored
    /// identifier of that type. Similarly with types `key_code` and `mouse_button`. The
    /// identifier variables, that do not match the type of the stored identifier, are left
    /// unchanged by this function.
    ///
    constexpr auto get(display::Key&, display::KeyCode&, display::MouseButton&) const noexcept -> Type;

private:
    Type m_type;
    union {
        display::Key m_key;
        display::KeyCode m_key_code;
        display::MouseButton m_mouse_button;
    };

    friend class Engine;
};



/// \brief Identifier for previously registered key handler function.
///
/// Objects of this type are used by the render engine to identify a previously registered
/// key handler function when binding a key handler to a particular key.
///
/// \sa \ref render::Engine::register_key_handler()
/// \sa \ref render::Engine::get_builtin_key_handler()
/// \sa \ref render::Engine::bind_key_a()
///
struct KeyHandlerIdent {
    int value;

    constexpr auto operator<=>(const KeyHandlerIdent&) const noexcept = default;
};



/// \brief Key modifier mode.
///
/// An object of this type specifies a particular key modifier mode. A key modifier mode is
/// specified when binding a key to a handler in a render engine (\ref
/// render::Engine::bind_key()).
///
/// Key modifier modes can be composed through use of the OR-operator (see \ref
/// operator|(KeyModifierMode)). Modifier modes corresponding to single modifier keys are
/// available as named constants (see table below).
///
/// A modifier mode is in effect when the modifier keys, that are currently pressed down, is
/// exactly the set for which the corresponding bit position are 1 in the value of the
/// modifier mode.
///
/// Here is a list of simple named modfier modes:
///
/// | Constant         | Value | Meaning
/// |------------------|-------|------------------------------------------------------------
/// | \ref modif_none  | 0     | No modifier keys are pressed down
/// | \ref modif_shift | 1     | A shift key is pressed down
/// | \ref modif_ctrl  | 2     | A control key is pressed down
/// | \ref modif_alt   | 4     | An alt key is pressed down
/// | \ref modif_meta  | 8     | A meta key is pressed down (Windows / Apple / command key)
///
/// \sa \ref render::Engine::bind_key()
/// \sa \ref render::Engine::bind_key_a()
///
struct KeyModifierMode {
    unsigned value;

    constexpr auto operator<=>(const KeyModifierMode&) const noexcept = default;

    /// \{
    ///
    /// \brief Bitwise operations on modifier modes.
    ///
    /// If `a`, `b`, and `c` are key modifier modes, then `c = ~a`, `c = a & b`, and `c = a | b`
    /// are shorthands for `c = { ~a.value }`, `c = { a.value & b.value }`, and `c = { a.value |
    /// b.value }` respectively.
    ///
    constexpr auto operator~() const noexcept -> KeyModifierMode;
    constexpr auto operator&(KeyModifierMode) const noexcept -> KeyModifierMode;
    constexpr auto operator|(KeyModifierMode) const noexcept -> KeyModifierMode;
};


/// \{
///
/// These are named constants for simple uncomposed key modifier modes.
///
/// \sa \ref render::KeyModifierMode
///
constexpr render::KeyModifierMode modif_none  = { 0 };
constexpr render::KeyModifierMode modif_shift = { 1 };
constexpr render::KeyModifierMode modif_ctrl  = { 2 };
constexpr render::KeyModifierMode modif_alt   = { 4 };
constexpr render::KeyModifierMode modif_meta  = { 8 };
/// \}



/// \brief Specification of key press multiplicity.
///
/// Objects of this type are used to specify press multiplicities when binding keys, i.e.,
/// the number of fast consecutive presses or clicks needed to activate a particular
/// function.
///
/// \sa \ref render::Engine::bind_key()
/// \sa \ref render::Engine::bind_key_a()
/// \sa \ref render::double_click
///
struct KeyPressMultiplicity {
    /// \brief Multiplicity value.
    ///
    /// This is the multiplicity value. It must be greater than or equal to 1. 1 means
    /// single click / single tap and 2 means double click / double tap. Higher values are
    /// allowed.
    ///
    int value;

    constexpr auto operator<=>(const KeyPressMultiplicity&) const noexcept = default;
};


/// \{
///
/// \brief Named key press multiplicities.
///
/// These are a selection of named key press multiplicities provided for convenience and to
/// aid readability.
///
constexpr KeyPressMultiplicity single_click = { 1 };
constexpr KeyPressMultiplicity double_click = { 2 };
constexpr KeyPressMultiplicity single_tap   = single_click;
constexpr KeyPressMultiplicity double_tap   = double_click;
/// \}








// Implementation


constexpr KeyIdent::KeyIdent(display::Key key) noexcept
{
    m_type = Type::key;
    m_key = key;
}


constexpr KeyIdent::KeyIdent(display::KeyCode key_code) noexcept
{
    m_type = Type::key_code;
    m_key_code = key_code;
}


constexpr KeyIdent::KeyIdent(display::MouseButton mouse_button) noexcept
{
    m_type = Type::mouse_button;
    m_mouse_button = mouse_button;
}


constexpr auto KeyIdent::get(display::Key& key, display::KeyCode& key_code,
                             display::MouseButton& mouse_button) const noexcept -> Type
{
    switch (m_type) {
        case Type::key:
            key = m_key;
            break;
        case Type::key_code:
            key_code = m_key_code;
            break;
        case Type::mouse_button:
            mouse_button = m_mouse_button;
            break;
    }
    return m_type;
}


constexpr auto KeyModifierMode::operator~() const noexcept -> KeyModifierMode
{
    return { ~value };
}


constexpr auto KeyModifierMode::operator&(KeyModifierMode other) const noexcept -> KeyModifierMode
{
    return { value & other.value };
}


constexpr auto KeyModifierMode::operator|(KeyModifierMode other) const noexcept -> KeyModifierMode
{
    return { value | other.value };
}


} // namespace archon::render

#endif // ARCHON_X_RENDER_X_KEY_BINDING_SUPPORT_HPP
