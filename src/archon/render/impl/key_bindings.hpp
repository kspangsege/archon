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

#ifndef ARCHON_X_RENDER_X_IMPL_X_KEY_BINDINGS_HPP
#define ARCHON_X_RENDER_X_IMPL_X_KEY_BINDINGS_HPP


#include <functional>
#include <string_view>
#include <vector>

#include <archon/core/assert.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/mouse_button.hpp>
#include <archon/display/event.hpp>
#include <archon/render/key_binding_support.hpp>


namespace archon::render::impl {


class KeyBindings {
public:
    class KeyIdent;

    using Timestamp = display::TimedWindowEvent::Timestamp;

    KeyBindings();
    ~KeyBindings() noexcept;

    // If the specified function returns `false`, it means that event processing must be
    // interrupted. If it returns `true`, it means that event processing can proceed.
    auto register_handler(std::string_view label, std::function<bool(bool down)> func) -> render::KeyHandlerIdent;

    void bind_key(const KeyIdent&, render::KeyModifierMode, render::KeyPressMultiplicity, render::KeyHandlerIdent);

    // These return `false` if event processing must be interrupted. Otherwise they return `true`.
    bool on_keydown(const KeyIdent& key_ident, Timestamp timestamp);
    bool on_keyup(const KeyIdent& key_ident, Timestamp timestamp);

    // `on_blur()` returns `false` if event processing must be interrupted. Otherwise it
    // returns `true`. When it returns `false`, the "on blur" operation is left in an
    // incomplete state. In this state, it is an error to invoke `on_keydown()`,
    // `on_keyup()`, or `on_blur()`. The application can invoke
    // `resume_incomplete_on_blur_if_any()` at any time to resume an incomplete "on blur"
    // operation if there is one.
    //
    // `resume_incomplete_on_blur_if_any()` returns `false` if event processing must be
    // interrupted. Otherwise it returns `true`. When it returns `false`, the "on blur"
    // operation remains incomplete, and the application must follow up with another
    // invocation of `resume_incomplete_on_blur_if_any()` before it calls `on_keydown()`,
    // `on_keyup()`, or `on_blur()`. This pattern may repeat itself any number of times.
    //
    bool on_blur();
    bool resume_incomplete_on_blur_if_any();

    auto get_modifier_mode() const noexcept -> render::KeyModifierMode;
    void set_modifier_mode(render::KeyModifierMode) noexcept;

private:
    struct KeyHandler;
    struct KeySlot;

    std::vector<KeyHandler> m_handlers;

    core::FlatMap<KeyIdent, KeySlot> m_key_slots;

    render::KeyModifierMode m_modifier_mode = {};

    bool m_on_blur_in_progress = false;

    template<bool down> bool on_key(const KeyIdent& key_ident, Timestamp timestamp);
    bool resume_incomplete_on_blur();
};


class KeyBindings::KeyIdent {
public:
    KeyIdent() noexcept = default;

    explicit KeyIdent(display::KeyCode key_code) noexcept;
    explicit KeyIdent(display::MouseButton mouse_button) noexcept;

    bool operator<(const KeyIdent& other) const noexcept;

private:
    enum class Type {
        key_code,
        mouse_button,
    };
    Type m_type;
    union {
        display::KeyCode m_key_code;
        display::MouseButton m_mouse_button;
    };
};








// Implementation


inline bool KeyBindings::on_blur()
{
    m_on_blur_in_progress = true;
    return resume_incomplete_on_blur(); // Throws
}


inline bool KeyBindings::resume_incomplete_on_blur_if_any()
{
    if (ARCHON_LIKELY(!m_on_blur_in_progress))
        return true; // Proceed
    return resume_incomplete_on_blur(); // Throws
}


inline auto KeyBindings::get_modifier_mode() const noexcept -> render::KeyModifierMode
{
    return m_modifier_mode;
}


inline void KeyBindings::set_modifier_mode(render::KeyModifierMode mode) noexcept
{
    m_modifier_mode = mode;
}


inline KeyBindings::KeyIdent::KeyIdent(display::KeyCode key_code) noexcept
{
    m_type = Type::key_code;
    m_key_code = key_code;
}


inline KeyBindings::KeyIdent::KeyIdent(display::MouseButton mouse_button) noexcept
{
    m_type = Type::mouse_button;
    m_mouse_button = mouse_button;
}


inline bool KeyBindings::KeyIdent::operator<(const KeyIdent& other) const noexcept
{
    if (int(m_type) < int(other.m_type))
        return true;
    if (int(m_type) > int(other.m_type))
        return false;
    switch (m_type) {
        case Type::key_code:
            return m_key_code < other.m_key_code;
        case Type::mouse_button:
            return m_mouse_button < other.m_mouse_button;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


} // namespace archon::render::impl

#endif // ARCHON_X_RENDER_X_IMPL_X_KEY_BINDINGS_HPP
