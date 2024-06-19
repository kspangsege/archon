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

#ifndef ARCHON_X_RENDER_X_NOINST_X_ENGINE_IMPL_HPP
#define ARCHON_X_RENDER_X_NOINST_X_ENGINE_IMPL_HPP


#include <memory>
#include <string_view>
#include <locale>

#include <archon/core/assert.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/log.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>
#include <archon/util/perspect_proj.hpp>
#include <archon/display.hpp>
#include <archon/render/virt_trackball.hpp>
#include <archon/render/key_binding_support.hpp>
#include <archon/render/impl/key_bindings.hpp>
#include <archon/render/engine.hpp>


namespace archon::render::impl {


class EngineImpl {
public:
    using Config            = render::Engine::Config;
    using Scene             = render::Engine::Scene;
    using BuiltinKeyHandler = render::Engine::BuiltinKeyHandler;
    using Clock             = render::Engine::Clock;

    EngineImpl(std::string_view window_title, display::Size window_size, const std::locale&, const Config&);
    void set_scene(Scene&) noexcept;

    void run();

    void set_frame_rate(double rate);
    void set_background_color(util::Color color);

    void set_base_orientation(const math::Rotation& orientation);
    void set_base_spin(const math::Rotation& spin);
    void set_base_zoom_factor(double factor);
    void set_base_interest_size(double size);

    auto get_logger() noexcept -> log::Logger&;

    auto get_key_bindings() noexcept -> impl::KeyBindings&;
    auto get_builtin_key_handler(BuiltinKeyHandler) const noexcept -> render::KeyHandlerIdent;
    void bind_key(render::KeyIdent, render::KeyHandlerIdent);
    void bind_key(render::KeyIdent, render::KeyModifierMode, render::KeyPressMultiplicity, render::KeyHandlerIdent);

    void set_window_title(std::string_view title);
    void set_window_size(display::Size size);
    void set_fullscreen_mode(bool on);

    void set_orientation(const math::Rotation& orientation);
    void set_spin(const math::Rotation& spin);
    void set_zoom_factor(double factor);
    void set_interest_size(double diameter);
    void reset_view();

    void set_headlight_mode(bool on);
    void set_wireframe_mode(bool on);

private:
    class EventHandler final
        : public display::ConnectionEventHandler
        , public display::WindowEventHandler {
    public:
        explicit EventHandler(EngineImpl&) noexcept;

        bool on_keydown(const display::KeyEvent&) override;
        bool on_keyup(const display::KeyEvent&) override;
        bool on_mousedown(const display::MouseButtonEvent&) override;
        bool on_mouseup(const display::MouseButtonEvent&) override;
        bool on_mousemove(const display::MouseEvent&) override;
        bool on_scroll(const display::ScrollEvent&) override;
        bool on_blur(const display::WindowEvent&) override;
        bool on_expose(const display::WindowEvent&) override;
        bool on_resize(const display::WindowSizeEvent&) override;
        bool on_reposition(const display::WindowPosEvent&) override;
        bool on_close(const display::WindowEvent&) override;
        bool before_sleep() override;
        bool on_quit() override;

    private:
        EngineImpl& m_engine;
    };

    using EventTimestamp = display::TimedWindowEvent::Timestamp;

    const std::locale& m_locale;
    std::unique_ptr<log::FileLogger> m_fallback_logger;
    log::Logger& m_logger;
    log::PrefixLogger m_display_logger;
    const display::Implementation& m_display_implementation;
    bool m_headlight_feature_enabled;
    bool m_wireframe_feature_enable;

    EventHandler m_event_handler { *this };
    std::unique_ptr<display::Connection> m_display_connection;
    std::unique_ptr<display::Window> m_window;

    Scene* m_scene = nullptr;

    impl::KeyBindings m_key_bindings;
    core::FlatMap<BuiltinKeyHandler, render::KeyHandlerIdent> m_builtin_key_handlers;

    double m_frame_rate;
    Clock::duration m_time_per_frame;
    display::Size m_viewport_size;
    math::Vector4F m_background_color;
    math::Rotation m_base_orientation;
    math::Rotation m_orientation;
    math::Rotation m_base_spin;
    math::Rotation m_spin;
    double m_base_zoom_factor;
    double m_base_interest_size;
    double m_interest_size;
    util::PerspectiveProjection m_perspect_proj;
    render::VirtualTrackball m_trackball;

    bool m_started = false;
    bool m_quit = false;
    bool m_interrupt_before_sleep = false;
    bool m_refresh_rate_changed = false;
    bool m_need_misc_update = true;
    bool m_projection_and_viewport_need_update = true;
    bool m_need_redraw = true;
    bool m_fullscreen_mode = false;
    bool m_headlight_mode = false;
    bool m_headlight_mode_prev = false;
    bool m_wireframe_mode = false;
    bool m_wireframe_mode_prev = false;

    int m_max_opengl_errors = 8;

    static auto instantiate_fallback_logger(std::unique_ptr<log::FileLogger>&, const std::locale&) -> log::Logger&;
    static auto determine_display_implementation(const display::Implementation::Slot*, const display::Guarantees&) ->
        const display::Implementation&;

    bool map_key_ident(const render::KeyIdent&, impl::KeyBindings::KeyIdent&) const;

    auto register_builtin_key_handler(BuiltinKeyHandler ident, std::string_view label,
                                      bool (EngineImpl::* func)(bool down)) -> render::KeyHandlerIdent;

    bool key_func_shift_modifier(bool down);
    bool key_func_control_modifier(bool down);
    bool key_func_alt_modifier(bool down);
    bool key_func_meta_modifier(bool down);
    bool key_func_quit(bool down);
    bool key_func_inc_frame_rate(bool down);
    bool key_func_dec_frame_rate(bool down);
    bool key_func_toggle_fullscreen(bool down);
    bool key_func_reset_view(bool down);
    bool key_func_toggle_headlight(bool down);
    bool key_func_toggle_wireframe(bool down);

    void set_viewport_size(display::Size) noexcept;
    void init();
    void redraw();
    bool process_events(Clock::time_point deadline);
    void tick(Clock::time_point time_of_tick);
    void update_projection_and_viewport();
    void update_perpect_proj_and_trackball() noexcept;
    void render_frame();

    void modify_dist(double diff);
    void modify_zoom(double dist);
};








// Implementation


inline void EngineImpl::set_scene(Scene& scene) noexcept
{
    m_scene = &scene;
}


inline auto EngineImpl::get_logger() noexcept -> log::Logger&
{
    return m_logger;
}


inline auto EngineImpl::get_key_bindings() noexcept -> impl::KeyBindings&
{
    return m_key_bindings;
}


inline auto EngineImpl::get_builtin_key_handler(BuiltinKeyHandler ident) const noexcept -> render::KeyHandlerIdent
{
    auto i = m_builtin_key_handlers.find(ident);
    ARCHON_ASSERT(i != m_builtin_key_handlers.end());
    return i->second;
}


inline void EngineImpl::bind_key(render::KeyIdent key, render::KeyHandlerIdent handler)
{
    bind_key(key, render::modif_none, render::single_tap, handler); // Throws
}


inline EngineImpl::EventHandler::EventHandler(EngineImpl& engine) noexcept
    : m_engine(engine)
{
}


} // namespace archon::render::impl

#endif // ARCHON_X_RENDER_X_NOINST_X_ENGINE_IMPL_HPP
