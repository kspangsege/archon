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


#include <utility>
#include <stdexcept>
#include <functional>
#include <string_view>
#include <string>
#include <chrono>

#include <archon/core/features.h>
#include <archon/core/pair.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/render/key_binding_support.hpp>
#include <archon/render/impl/key_bindings.hpp>


using namespace archon;
using render::impl::KeyBindings;


struct KeyBindings::KeyHandler {
    std::function<bool(bool down)> func;
    std::string label;
};


struct KeyBindings::KeySlot {
    bool is_pressed = false;

    // If `multiplicity` is zero, the next key press is not connected to any previous press
    // of the key. Otherwise, the next key press may be connected to the last press,
    // `timestamp` is the time of the last press, and `modifier_mode` is the key modifier
    // mode that was in effect for the last press.
    //
    int multiplicity = 0;
    Timestamp timestamp = {};
    render::KeyModifierMode modifier_mode = {};

    using Subkey = core::Pair<render::KeyModifierMode, render::KeyPressMultiplicity>;
    core::FlatMap<Subkey, int, 2> handlers;

    // The maximum multiplicity among the currently registered handlers (`handlers`), or 0
    // if no handlers are registered.
    int max_multiplicity = 0;

    // Updated on every "key down" event. If the "key down" event is associated with a
    // handler, `handler_index` is set to the index of that handler. Otherwise,
    // `handler_index` is set to -1. The handler index determines the handler that gets
    // called on the "key down" event as well as the subsequent "key up" event.
    //
    int handler_index = -1;

    void on_reconfigured() noexcept;
    void reset_key_state() noexcept;
};


KeyBindings::KeyBindings()
{
}


KeyBindings::~KeyBindings() noexcept
{
}


auto KeyBindings::register_handler(std::string_view label, std::function<bool(bool down)> func) ->
    render::KeyHandlerIdent
{
    decltype(render::KeyHandlerIdent::value) handler_index = {};
    core::int_cast(m_handlers.size(), handler_index); // Throws
    auto label_2 = std::string(label); // Throws
    m_handlers.push_back({ std::move(func), std::move(label_2) }); // Throws
    return { handler_index };
}


void KeyBindings::bind_key(const KeyIdent& ident, render::KeyModifierMode modifier_mode,
                           render::KeyPressMultiplicity multiplicity, render::KeyHandlerIdent handler)
{
    auto handler_index = handler.value;
    if (ARCHON_UNLIKELY(handler_index < 0 || core::to_unsigned(handler_index) >= m_handlers.size()))
        throw std::invalid_argument("Handler identifier");

    KeySlot& slot = m_key_slots[ident]; // Throws
    slot.handlers[{ modifier_mode, multiplicity }] = handler_index; // Throws
    slot.on_reconfigured();
}


bool KeyBindings::on_keydown(const KeyIdent& key_ident, Timestamp timestamp)
{
    constexpr bool down = true;
    return on_key<down>(key_ident, timestamp); // Throws
}


bool KeyBindings::on_keyup(const KeyIdent& key_ident, Timestamp timestamp)
{
    constexpr bool down = false;
    return on_key<down>(key_ident, timestamp); // Throws
}


template<bool down> inline bool KeyBindings::on_key(const KeyIdent& key_ident, Timestamp timestamp)
{
    auto i = m_key_slots.find(key_ident);
    if (ARCHON_UNLIKELY(i == m_key_slots.end())) {
        // Nothing registered for this key
        return true; // Allow event processing to proceed
    }

    KeySlot& slot = i->second;
    if constexpr (down) {
        if (ARCHON_UNLIKELY(slot.is_pressed)) {
            // Key already pressed (a prior "key up" event was missed)
            return true; // Allow event processing to proceed
        }
        constexpr auto max_multipress_period = std::chrono::milliseconds(250);
        bool connected_to_prev = (slot.multiplicity > 0 &&
                                  m_modifier_mode == slot.modifier_mode &&
                                  timestamp - slot.timestamp <= max_multipress_period &&
                                  slot.multiplicity < slot.max_multiplicity);
        if (ARCHON_LIKELY(!connected_to_prev))
            slot.multiplicity = 0;

        slot.is_pressed = true;
        slot.multiplicity += 1;
        slot.timestamp = timestamp;
        slot.modifier_mode = m_modifier_mode;

        render::KeyPressMultiplicity multiplicity = { slot.multiplicity };
        auto j = slot.handlers.find({ slot.modifier_mode, multiplicity });
        if (ARCHON_UNLIKELY(j == slot.handlers.end())) {
            // Key not bound for current modifier mode and multiplicity
            slot.handler_index = -1;
            return true; // Allow event processing to proceed
        }

        slot.handler_index = j->second;
    }
    else {
        if (ARCHON_UNLIKELY(!slot.is_pressed)) {
            // Key already released (a prior "key down" event was missed)
            return true; // Allow event processing to proceed
        }

        slot.is_pressed = false;

        if (ARCHON_UNLIKELY(slot.handler_index < 0)) {
            // No handler was selected when key was pressed down
            return true; // Allow event processing to proceed
        }
    }

    ARCHON_ASSERT(slot.handler_index >= 0);
    const KeyHandler& handler = m_handlers[slot.handler_index];
    return handler.func(down); // Throws
}


bool KeyBindings::resume_incomplete_on_blur()
{
    ARCHON_ASSERT(m_on_blur_in_progress);

    for (auto& entry : m_key_slots) {
        KeySlot& slot = entry.second;
        bool was_pressed = slot.is_pressed;
        int handler_index = slot.handler_index;
        slot.reset_key_state();
        if (ARCHON_LIKELY(!was_pressed || handler_index < 0))
            continue;
        const KeyHandler& handler = m_handlers[handler_index];
        bool down = false;
        bool proceed = handler.func(down); // Throws
        if (ARCHON_LIKELY(proceed))
            continue;
        return false; // Interrupt event processing
    }

    m_on_blur_in_progress = false;
    return true; // Allow event processing to proceed
}



void KeyBindings::KeySlot::on_reconfigured() noexcept
{
    reset_key_state();
    max_multiplicity = 0;
    for (const auto& entry : handlers) {
        const Subkey& subkey = entry.first;
        render::KeyPressMultiplicity multiplicity_2 = subkey.second;
        if (multiplicity_2.value <= max_multiplicity)
            continue;
        max_multiplicity = multiplicity_2.value;
    }
}


inline void KeyBindings::KeySlot::reset_key_state() noexcept
{
    is_pressed = false;
    multiplicity = 0;
}
