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


#include <iterator>
#include <memory>
#include <stdexcept>
#include <optional>
#include <string_view>
#include <string>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/string.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_x11.hpp>
#include <archon/display/implementation_sdl.hpp>


using namespace archon;
using display::Implementation;


auto Implementation::new_connection(const std::locale& locale, const display::Connection::Config& config) const ->
    std::unique_ptr<display::Connection>
{
    std::unique_ptr<display::Connection> conn;
    std::string error;
    if (ARCHON_LIKELY(try_new_connection(locale, config, conn, error))) // Throws
        return conn;
    using namespace std::literals;
    std::string message = core::concat("Failed to open display connection: "sv, std::string_view(error)); // Throws
    throw std::runtime_error(message);
}


auto Implementation::Slot::get_implementation(const display::Guarantees& guarantees) const -> const Implementation&
{
    const Implementation* impl = get_implementation_a(guarantees);
    if (ARCHON_LIKELY(impl))
        return *impl;
    throw std::runtime_error("Unavailable display implementation");
}


namespace {


using slot_getter_type = auto (*)() noexcept -> const display::Implementation::Slot&;


constexpr slot_getter_type g_implementation_slots[] {
#if !(ARCHON_APPLE || ARCHON_WINDOWS)
    &display::get_x11_implementation_slot,
#endif
    &display::get_sdl_implementation_slot,
#if ARCHON_APPLE || ARCHON_WINDOWS
    &display::get_x11_implementation_slot,
#endif
};


constexpr int g_num_implementation_slots = int(std::size(g_implementation_slots));


} // unnamed namespace


auto display::get_default_implementation(const display::Guarantees& guarantees) -> const display::Implementation&
{
    const display::Implementation* impl = get_default_implementation_a(guarantees);
    if (ARCHON_LIKELY(impl))
        return *impl;
    throw std::runtime_error("No available display implementations");
}


auto display::get_default_implementation_a(const display::Guarantees& guarantees) noexcept ->
    const display::Implementation*
{
    int n = g_num_implementation_slots;
    for (int i = 0; i < n; ++i) {
        const display::Implementation::Slot& slot = (*g_implementation_slots[i])();
        const display::Implementation* impl = slot.get_implementation_a(guarantees);
        if (ARCHON_LIKELY(!impl))
            continue;
        return impl;
    }
    return nullptr;
}


int display::get_num_implementation_slots() noexcept
{
    return g_num_implementation_slots;
}


auto display::get_implementation_slot(int index) -> const display::Implementation::Slot&
{
    if (ARCHON_LIKELY(index >= 0 && index < g_num_implementation_slots))
        return (*g_implementation_slots[index])();
    throw std::out_of_range("Implementation slot index");
}


auto display::lookup_implementation(std::string_view ident) noexcept -> const display::Implementation::Slot*
{
    int n = g_num_implementation_slots;
    for (int i = 0; i < n; ++i) {
        const display::Implementation::Slot& slot = (*g_implementation_slots[i])();
        if (ARCHON_LIKELY(slot.ident() != ident))
            continue;
        return &slot;
    }
    return nullptr;
}


auto display::pick_implementation(const std::optional<std::string_view>& ident,
                                  const display::Guarantees& guarantees) -> const display::Implementation&
{
    const display::Implementation* impl = {};
    std::string error;
    if (ARCHON_LIKELY(display::try_pick_implementation(ident, guarantees, impl, error))) // Throws
        return *impl;
    throw std::runtime_error(error);
}


bool display::try_pick_implementation(const std::optional<std::string_view>& ident,
                                      const display::Guarantees& guarantees,
                                      const display::Implementation*& impl, std::string& error)
{
    const display::Implementation* impl_2;
    if (ident.has_value()) {
        using namespace std::literals;
        std::string_view ident_2 = ident.value();
        const display::Implementation::Slot* slot = display::lookup_implementation(ident_2);
        if (ARCHON_UNLIKELY(!slot)) {
            error = core::concat("Unknown display implementation (\""sv, ident_2, "\")"sv); // Throws
            return false;
        }
        impl_2 = slot->get_implementation_a(guarantees);
        if (ARCHON_UNLIKELY(!impl_2)) {
            error = core::concat("Unavailable display implementation (\""sv, ident_2, "\")"sv); // Throws
            return false;
        }
    }
    else {
        impl_2 = display::get_default_implementation_a(guarantees);
        if (ARCHON_UNLIKELY(!impl_2)) {
            error = "No display implementations are available"; // Throws
            return false;
        }
    }
    impl = impl_2;
    return true;
}
