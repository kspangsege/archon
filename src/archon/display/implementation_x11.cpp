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


                                            
#include <memory>
#include <string_view>
#include <locale>

#include <archon/display/impl/config.h>
#include <archon/display/implementation.hpp>
#include <archon/display/implementation_x11.hpp>

#if ARCHON_DISPLAY_HAVE_X11
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#  include <X11/XKBlib.h>
#  if ARCHON_DISPLAY_HAVE_XRANDR
#    include <X11/extensions/Xrandr.h>
#  endif
#  if ARCHON_DISPLAY_HAVE_XRENDER
#    include <X11/extensions/Xrender.h>
#  endif
#endif


using namespace archon;


namespace {


constexpr std::string_view g_implementation_ident = "x11";


#if ARCHON_DISPLAY_HAVE_X11


class ImplementationImpl
    : public display::Implementation {
public:
    ImplementationImpl(Slot&) noexcept;

    auto new_connection(const std::locale&) const -> std::unique_ptr<display::Connection> override final;
    bool try_map_key_to_key_code(display::Key, display::KeyCode&) const override final;
    bool try_map_key_code_to_key(display::KeyCode, display::Key&) const override final;
    bool try_get_key_name(display::KeyCode, std::string_view&) const override final;
    auto get_slot() const noexcept -> const Slot& override final;

private:
    const Slot& m_slot;
};


class SlotImpl
    : public display::Implementation::Slot {
public:
    SlotImpl() noexcept;

    auto ident() const noexcept -> std::string_view override final;
    auto get_implementation_a(const display::Guarantees&) const noexcept ->
        const display::Implementation* override final;

private:
    ImplementationImpl m_impl;
};



inline ImplementationImpl::ImplementationImpl(Slot& slot) noexcept
    : m_slot(slot)
{
}


auto ImplementationImpl::new_connection(const std::locale& locale) const -> std::unique_ptr<display::Connection>
{
    static_cast<void>(locale);    
    throw std::runtime_error("*click*");    
}


bool ImplementationImpl::try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const
{
    static_cast<void>(key);    
    static_cast<void>(key_code);    
    return false;                      
}


bool ImplementationImpl::try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const
{
    static_cast<void>(key_code);    
    static_cast<void>(key);    
    return false;                      
}


bool ImplementationImpl::try_get_key_name(display::KeyCode key_code, std::string_view& name) const
{
    static_cast<void>(key_code);    
    static_cast<void>(name);    
    return false;                      
}


auto ImplementationImpl::get_slot() const noexcept -> const Slot&
{
    return m_slot;
}



inline SlotImpl::SlotImpl() noexcept
    : m_impl(*this)
{
}


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees& guarantees) const noexcept ->
    const display::Implementation*
{
    bool is_available = (guarantees.no_other_use_of_x11 &&
                         guarantees.main_thread_exclusive);
    if (ARCHON_LIKELY(is_available))
        return &m_impl;
    return nullptr;
}


#else // !ARCHON_DISPLAY_HAVE_X11


class SlotImpl
    : public display::Implementation::Slot {
public:
    auto ident() const noexcept -> std::string_view override final;
    auto get_implementation_a(const display::Guarantees&) const noexcept ->
        const display::Implementation* override final;
};


auto SlotImpl::ident() const noexcept -> std::string_view
{
    return g_implementation_ident;
}


auto SlotImpl::get_implementation_a(const display::Guarantees&) const noexcept -> const display::Implementation*
{
    return nullptr;
}


#endif // !ARCHON_DISPLAY_HAVE_X11


} // unnamed namespace


auto display::get_x11_implementation_slot() noexcept -> const display::Implementation::Slot&
{
    static SlotImpl slot;
    return slot;
}
