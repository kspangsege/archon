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


                                            
#include <cstdlib>
#include <memory>
#include <string_view>
#include <locale>

#include <archon/core/format.hpp>
#include <archon/core/quote.hpp>
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

    auto new_connection(const std::locale&, const display::Connection::Config&) const ->
        std::unique_ptr<display::Connection> override final;
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


class ConnectionImpl
    : private display::ConnectionEventHandler
    , public display::Connection {
public:
    const ImplementationImpl& impl;

    ConnectionImpl(const ImplementationImpl&) noexcept;
    ~ConnectionImpl() noexcept override;

    void open(const std::locale&, const display::ConnectionConfigX11&);

    auto new_window(std::string_view, display::Size, display::WindowEventHandler&, display::Window::Config) ->
        std::unique_ptr<display::Window> override final;
    auto new_window(int, std::string_view, display::Size, display::WindowEventHandler&, display::Window::Config) ->
        std::unique_ptr<display::Window> override final;
    void process_events(display::ConnectionEventHandler*) override final;
    bool process_events(time_point_type, display::ConnectionEventHandler*) override final;
    int get_num_displays() const override final;
    int get_default_display() const override final;
    bool try_get_display_conf(int, core::Buffer<display::Screen>&, core::Buffer<char>&,
                              std::size_t&) const override final;
    auto get_implementation() const noexcept -> const display::Implementation& override final;

private:
    Display* m_dpy = nullptr;
};



inline ImplementationImpl::ImplementationImpl(Slot& slot) noexcept
    : m_slot(slot)
{
}


auto ImplementationImpl::new_connection(const std::locale& locale, const display::Connection::Config& config) const ->
    std::unique_ptr<display::Connection>
{
    auto conn = std::make_unique<ConnectionImpl>(*this); // Throws
    conn->open(locale, config.x11); // Throws
    return conn;
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



inline ConnectionImpl::ConnectionImpl(const ImplementationImpl& impl_2) noexcept
    : impl(impl_2)
{
}


ConnectionImpl::~ConnectionImpl() noexcept
{
    if (m_dpy)
        XCloseDisplay(m_dpy);
}


void ConnectionImpl::open(const std::locale& locale, const display::ConnectionConfigX11& config)
{
    std::string display_name;
    if (!config.display.empty()) {
        display_name = std::string(config.display); // Throws
    }
    else if (char* val = std::getenv("DISPLAY")) {
        display_name = std::string(val); // Throws
    }
    Display* dpy = XOpenDisplay(display_name.data());
    if (ARCHON_LIKELY(dpy)) {
        m_dpy = dpy;
        return;
    }
    std::string message = core::format(locale, "Failed to open X11 display connection (%s)",
                                       core::quoted(std::string_view(display_name))); // Throws
    throw std::runtime_error(message);
}


auto ConnectionImpl::new_window(std::string_view title, display::Size size,
                                display::WindowEventHandler& event_handler,
                                display::Window::Config config) -> std::unique_ptr<display::Window>
{
    static_cast<void>(title);    
    static_cast<void>(size);    
    static_cast<void>(event_handler);    
    static_cast<void>(config);    
    throw std::runtime_error("*click*");     
}


auto ConnectionImpl::new_window(int display, std::string_view title, display::Size size,
                                display::WindowEventHandler& event_handler,
                                display::Window::Config config) -> std::unique_ptr<display::Window>
{
    static_cast<void>(display);    
    static_cast<void>(title);    
    static_cast<void>(size);    
    static_cast<void>(event_handler);    
    static_cast<void>(config);    
    throw std::runtime_error("*click*");     
}


void ConnectionImpl::process_events(display::ConnectionEventHandler* connection_event_handler)
{
    static_cast<void>(connection_event_handler);    
    throw std::runtime_error("*click*");     
}


bool ConnectionImpl::process_events(time_point_type deadline,
                                    display::ConnectionEventHandler* connection_event_handler)
{
    static_cast<void>(deadline);    
    static_cast<void>(connection_event_handler);    
    throw std::runtime_error("*click*");     
}


int ConnectionImpl::get_num_displays() const
{
    throw std::runtime_error("*click*");     
}


int ConnectionImpl::get_default_display() const
{
    throw std::runtime_error("*click*");     
}


bool ConnectionImpl::try_get_display_conf(int display, core::Buffer<display::Screen>&, core::Buffer<char>&,
                                          std::size_t&) const
{
    static_cast<void>(display);    
    throw std::runtime_error("*click*");     
}


auto ConnectionImpl::get_implementation() const noexcept -> const display::Implementation&
{
    return impl;
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
