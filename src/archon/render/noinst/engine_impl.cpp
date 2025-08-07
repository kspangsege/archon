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


#include <cmath>
#include <algorithm>
#include <utility>
#include <memory>
#include <functional>
#include <string_view>
#include <string>
#include <locale>
#include <chrono>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/math.hpp>
#include <archon/core/format.hpp>
#include <archon/core/format_as.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/display.hpp>
#include <archon/render/opengl.hpp>
#include <archon/render/key_binding_support.hpp>
#include <archon/render/impl/key_bindings.hpp>
#include <archon/render/noinst/engine_impl.hpp>


using namespace archon;
namespace impl = render::impl;
using impl::EngineImpl;


namespace {


constexpr double g_zoom_factor_min  = 1.0 / 8;
constexpr double g_zoom_factor_max  = 32;


} // unnamed namespace


EngineImpl::EngineImpl(Scene& scene, display::Connection& conn, const std::locale& locale, const Config& config)
    : m_locale(locale)
    , m_scene(scene)
    , m_conn(conn)
    , m_screen(config.screen >= 0 ? config.screen : conn.get_default_screen()) // Throws
    , m_logger(config.logger ? *config.logger : instantiate_fallback_logger(m_fallback_logger, locale)) // Throws
    , m_headlight_feature_enabled(!config.disable_headlight_feature)
    , m_wireframe_feature_enabled(!config.disable_wireframe_feature)
    , m_resolution_tracking_enabled(!config.disable_resolution_tracking)
    , m_frame_rate_tracking_enabled(!config.disable_frame_rate_tracking)
    , m_default_resolution(config.resolution)
    , m_default_frame_rate(config.frame_rate)
    , m_key_bindings() // Throws
    , m_base_orientation(config.orientation)
    , m_base_spin(config.spin)
    , m_base_zoom_factor(config.zoom_factor)
    , m_base_interest_size(config.interest_size)
    , m_trackball() // Throws
{
    m_conn.set_event_handler(m_event_handler); // Throws
}


EngineImpl::~EngineImpl() noexcept
{
    m_conn.unset_event_handler();
}


bool EngineImpl::try_init(std::string_view window_title, display::Size window_size, const Config& config,
                          std::string& error)
{
    ARCHON_ASSERT(!m_initialized);

#if !ARCHON_RENDER_HAVE_OPENGL
    error = "OpenGL not available"; // Throws
    return false;
#endif

    update_window_size(window_size);
    update_resolution(m_default_resolution); // Throws
    update_frame_rate(m_default_frame_rate); // Throws
    set_background_color(util::colors::black); // Throws
    reset_view(); // Throws
    set_headlight_mode(config.headlight_mode); // Throws
    set_wireframe_mode(config.wireframe_mode); // Throws

    render::KeyHandlerIdent handler;
    handler = register_builtin_key_handler(BuiltinKeyHandler::shift_modifier,
                                           "Shift modifier mode",
                                           &EngineImpl::key_func_shift_modifier); // Throws
    bind_key(display::Key::shift_left, handler); // Throws
    bind_key(display::Key::shift_right, handler); // Throws

    handler = register_builtin_key_handler(BuiltinKeyHandler::control_modifier,
                                           "Control modifier mode",
                                           &EngineImpl::key_func_control_modifier); // Throws
    bind_key(display::Key::ctrl_left, handler); // Throws
    bind_key(display::Key::ctrl_right, handler); // Throws

    handler = register_builtin_key_handler(BuiltinKeyHandler::alt_modifier,
                                           "Alt modifier mode",
                                           &EngineImpl::key_func_alt_modifier); // Throws
    bind_key(display::Key::alt_left, handler); // Throws
    bind_key(display::Key::alt_right, handler); // Throws

    handler = register_builtin_key_handler(BuiltinKeyHandler::meta_modifier,
                                           "Meta modifier mode",
                                           &EngineImpl::key_func_meta_modifier); // Throws
    bind_key(display::Key::meta_left, handler); // Throws
    bind_key(display::Key::meta_right, handler); // Throws

    handler = register_builtin_key_handler(BuiltinKeyHandler::quit,
                                           "Quit application",
                                           &EngineImpl::key_func_quit); // Throws
    bind_key(display::Key::escape, handler); // Throws
    bind_key(display::Key::small_q, handler); // Throws

    if (!config.disable_frame_rate_control) {
        handler = register_builtin_key_handler(BuiltinKeyHandler::inc_frame_rate,
                                               "Increase frame rate",
                                               &EngineImpl::key_func_inc_frame_rate); // Throws
        bind_key(display::Key::keypad_add, handler); // Throws

        handler = register_builtin_key_handler(BuiltinKeyHandler::dec_frame_rate,
                                               "Decrease frame rate",
                                               &EngineImpl::key_func_dec_frame_rate); // Throws
        bind_key(display::Key::keypad_subtract, handler); // Throws
    }

    if (config.allow_window_resize) {
        handler = register_builtin_key_handler(BuiltinKeyHandler::toggle_fullscreen,
                                               "Toggle fullscreen mode",
                                               &EngineImpl::key_func_toggle_fullscreen); // Throws
        bind_key(display::Key::small_f, handler); // Throws
    }

    handler = register_builtin_key_handler(BuiltinKeyHandler::reset_view,
                                           "Reset view",
                                           &EngineImpl::key_func_reset_view); // Throws
    bind_key(display::Key::space, handler); // Throws

    if (!config.disable_headlight_feature) {
        handler = register_builtin_key_handler(BuiltinKeyHandler::toggle_headlight,
                                               "Toggle headlight",
                                               &EngineImpl::key_func_toggle_headlight); // Throws
        bind_key(display::Key::small_l, handler); // Throws
    }

    if (!config.disable_wireframe_feature) {
        handler = register_builtin_key_handler(BuiltinKeyHandler::toggle_wireframe,
                                               "Toggle wireframe mode",
                                               &EngineImpl::key_func_toggle_wireframe); // Throws
        bind_key(display::Key::small_w, handler); // Throws
    }

    display::Window::Config window_config;
    window_config.screen = m_screen;
    window_config.resizable = config.allow_window_resize;
    window_config.fullscreen = config.fullscreen_mode;
    window_config.enable_opengl_rendering = true;
    window_config.require_opengl_depth_buffer = config.require_depth_buffer;
    std::unique_ptr<display::Window> window;
    std::string error_2;
    if (ARCHON_UNLIKELY(!m_conn.try_new_window(window_title, window_size, window_config,
                                               window, error_2))) { // Throws
            error = core::format(m_locale, "Failed to create window: %s", error_2); // Throws
            return false;
    }

    window->set_event_handler(m_event_handler); // Throws
    window->opengl_make_current(); // Throws

#if ARCHON_RENDER_HAVE_OPENGL
    m_logger.detail("OpenGL Vendor: %s", glGetString(GL_VENDOR)); // Throws
    m_logger.detail("OpenGL Renderer: %s", glGetString(GL_RENDERER)); // Throws
    m_logger.detail("OpenGL Version: %s", glGetString(GL_VERSION)); // Throws
#endif // ARCHON_RENDER_HAVE_OPENGL

    if (ARCHON_UNLIKELY(!m_scene.try_prepare(error))) // Throws
        return false;

    m_window = std::move(window);
    m_fullscreen_mode = config.fullscreen_mode;
    m_initialized = true;

    return true;
}


void EngineImpl::run()
{
    ARCHON_ASSERT(m_initialized);
    ARCHON_ASSERT(!m_started);

    fetch_screen_conf(); // Throws
    track_screen_conf(); // Throws

    if (m_headlight_feature_enabled) {
#if ARCHON_RENDER_HAVE_OPENGL
        GLfloat params[4]  = { 0, 0, 0, 1 };
        glLightfv(GL_LIGHT0, GL_POSITION, params);
#endif // ARCHON_RENDER_HAVE_OPENGL
    }

    m_scene.render_init(); // Throws

    m_window->show(); // Throws
    m_started = true;
    update_resolution(m_resolution); // Throws
    update_frame_rate(m_frame_rate); // Throws
    set_spin(m_spin); // Throws

    // Loop once per frame tick
    Clock::time_point deadline = Clock::now();
    for (;;) {
        deadline += m_time_per_frame;
        Clock::time_point now = Clock::now();
        if (ARCHON_UNLIKELY(deadline < now))
            deadline = now;

      redraw:
        if (ARCHON_UNLIKELY(m_need_redraw)) {
            redraw(); // Throws
            m_need_redraw = false;
        }

        m_interrupt_before_sleep = false;
        m_refresh_rate_changed = false;
        bool expired = process_events(deadline); // Throws
        if (!expired) {
            if (ARCHON_UNLIKELY(m_quit))
                break;
            if (ARCHON_LIKELY(!m_refresh_rate_changed))
                goto redraw;
            deadline = Clock::now();
        }

        tick(deadline); // Throws
    }
}


void EngineImpl::set_background_color(util::Color color)
{
    color.to_vec(m_background_color);
    m_need_misc_update = true;
    m_need_redraw = true;
}


void EngineImpl::set_base_orientation(const math::Rotation& orientation)
{
    m_base_orientation = orientation;
    set_orientation(orientation); // Throws
}


void EngineImpl::set_base_spin(const math::Rotation& spin)
{
    m_base_spin = spin;
    set_spin(spin); // Throws
}


void EngineImpl::set_base_zoom_factor(double factor)
{
    m_base_zoom_factor = factor;
    set_zoom_factor(factor); // Throws
}


void EngineImpl::set_base_interest_size(double size)
{
    m_base_interest_size = size;
    set_interest_size(size); // Throws
}


void EngineImpl::bind_key(render::KeyIdent key, render::KeyModifierMode modifier,
                          render::KeyPressMultiplicity multiplicity, render::KeyHandlerIdent handler)
{
    impl::KeyBindings::KeyIdent ident = {};
    if (ARCHON_LIKELY(map_key_ident(key, ident))) // Throws
        m_key_bindings.bind_key(ident, modifier, multiplicity, handler); // Throws
}


void EngineImpl::set_window_title(std::string_view title)
{
    m_window->set_title(title); // Throws
}


void EngineImpl::set_window_size(display::Size size)
{
    m_window->set_size(size); // Throws
}


void EngineImpl::set_fullscreen_mode(bool on)
{
    m_fullscreen_mode = on;
    m_window->set_fullscreen_mode(on); // Throws
}


void EngineImpl::set_orientation(const math::Rotation& orientation)
{
    m_orientation = orientation;
    m_trackball.set_orientation(orientation);
    m_need_redraw = true;
}


void EngineImpl::set_spin(const math::Rotation& spin)
{
    m_spin = spin;
    if (ARCHON_LIKELY(m_started))
        m_trackball.set_spin(m_spin, Clock::now());
}


void EngineImpl::set_zoom_factor(double factor)
{
    m_perspect_proj.zoom_factor = std::clamp(factor, g_zoom_factor_min, g_zoom_factor_max);
    m_projection_and_viewport_need_update = true;
    m_need_redraw = true;
}


void EngineImpl::set_interest_size(double diameter)
{
    m_interest_size = diameter;
    m_projection_and_viewport_need_update = true;
    m_need_redraw = true;
}


void EngineImpl::reset_view()
{
    set_orientation(m_base_orientation); // Throws
    set_spin(m_base_spin);  // Throws
    set_zoom_factor(m_base_zoom_factor); // Throws
    set_interest_size(m_base_interest_size); // Throws
}


void EngineImpl::set_headlight_mode(bool on)
{
    m_headlight_mode = on;
    m_need_redraw = true;
}


void EngineImpl::set_wireframe_mode(bool on)
{
    m_wireframe_mode = on;
    m_need_redraw = true;
}


bool EngineImpl::EventHandler::on_keydown(const display::KeyEvent& ev)
{
    return m_engine.m_key_bindings.on_keydown(impl::KeyBindings::KeyIdent(ev.key_code), ev.timestamp); // Throws
}


bool EngineImpl::EventHandler::on_keyup(const display::KeyEvent& ev)
{
    return m_engine.m_key_bindings.on_keyup(impl::KeyBindings::KeyIdent(ev.key_code), ev.timestamp); // Throws
}


bool EngineImpl::EventHandler::on_mousedown(const display::MouseButtonEvent& ev)
{
    if (ev.button == display::MouseButton::left) {
        // m_engine.m_window->set_cursor(*m_engine.m_cursor_trackball);        
        m_engine.m_trackball.acquire(Clock::now());
        m_engine.m_trackball.track(ev.pos, ev.timestamp);
        m_engine.m_need_redraw = true;
        return true;
    }
    return m_engine.m_key_bindings.on_keydown(impl::KeyBindings::KeyIdent(ev.button), ev.timestamp); // Throws
}


bool EngineImpl::EventHandler::on_mouseup(const display::MouseButtonEvent& ev)
{
    if (ev.button == display::MouseButton::left) {
        m_engine.m_trackball.track(ev.pos, ev.timestamp);
        m_engine.m_trackball.release(Clock::now());
        // m_engine.m_window->set_cursor(*m_engine.m_cursor_normal);        
        m_engine.m_need_redraw = true;
        return true;
    }
    return m_engine.m_key_bindings.on_keyup(impl::KeyBindings::KeyIdent(ev.button), ev.timestamp); // Throws
}


bool EngineImpl::EventHandler::on_mousemove(const display::MouseEvent& ev)
{
    m_engine.m_trackball.track(ev.pos, ev.timestamp);
    m_engine.m_need_redraw = true;
    return true;
}


bool EngineImpl::EventHandler::on_scroll(const display::ScrollEvent& ev)
{
    render::KeyModifierMode mode = m_engine.m_key_bindings.get_modifier_mode();
    if (ARCHON_LIKELY(mode == render::modif_none)) {
        m_engine.modify_dist(-ev.amount[1]); // Throws
    }
    else if (ARCHON_LIKELY(mode == render::modif_shift)) {
        m_engine.modify_zoom(ev.amount[1]); // Throws
    }
    return true;
}


bool EngineImpl::EventHandler::on_blur(const display::WindowEvent&)
{
    // Note: Because we invoke impl::KeyBindings::on_blur(), we are obligated to ensure that
    // impl::KeyBindings::resume_incomplete_on_blur() gets invoked before any subsequent
    // invocation of impl::KeyBindings::on_keydown(), impl::KeyBindings::on_keyup(), or
    // impl::KeyBindings::on_blur(). This happens in process_events().

    return m_engine.m_key_bindings.on_blur(); // Throws
}


bool EngineImpl::EventHandler::on_expose(const display::WindowEvent&)
{
    m_engine.m_need_redraw = true;
    m_engine.m_interrupt_before_sleep = true;
    return true;
}


bool EngineImpl::EventHandler::on_resize(const display::WindowSizeEvent& ev)
{
    m_engine.update_window_size(ev.size);
    m_engine.track_screen_conf(); // Throws
    return true;
}


bool EngineImpl::EventHandler::on_reposition(const display::WindowPosEvent& ev)
{
    m_engine.update_window_pos(ev.pos);
    m_engine.track_screen_conf(); // Throws
    return true;
}


bool EngineImpl::EventHandler::on_close(const display::WindowEvent&)
{
    m_engine.m_quit = true;
    return false; // Interrupt event processing
}


bool EngineImpl::EventHandler::on_screen_change(int screen)
{
    if (screen == m_engine.m_screen) {
        m_engine.fetch_screen_conf(); // Throws
        m_engine.track_screen_conf(); // Throws
    }
    return true;
}


bool EngineImpl::EventHandler::before_sleep()
{
    return !m_engine.m_interrupt_before_sleep;
}


bool EngineImpl::EventHandler::on_quit()
{
    m_engine.m_quit = true;
    return false; // Interrupt event processing
}


auto EngineImpl::instantiate_fallback_logger(std::unique_ptr<log::FileLogger>& logger,
                                             const std::locale& locale) -> log::Logger&
{
    logger = std::make_unique<log::FileLogger>(core::File::get_stdout(), locale); // Throws
    return *logger;
}


bool EngineImpl::map_key_ident(const render::KeyIdent& key, impl::KeyBindings::KeyIdent& ident) const
{
    display::Key key_2 = {};
    display::KeyCode key_code = {};
    display::MouseButton mouse_button = {};
    switch (key.get(key_2, key_code, mouse_button)) {
        case render::KeyIdent::Type::key: {
            bool found = m_conn.try_map_key_to_key_code(key_2, key_code); // Throws
            if (ARCHON_LIKELY(found)) {
                ident = impl::KeyBindings::KeyIdent(key_code);
                return true;
            }
            // FIXME: Consider warning when a key handler is bound to a key that is unknown to the selected display implementation                
            return false;
        }
        case render::KeyIdent::Type::key_code:
            ident = impl::KeyBindings::KeyIdent(key_code);
            return true;
        case render::KeyIdent::Type::mouse_button:
            ident = impl::KeyBindings::KeyIdent(mouse_button);
            return true;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return {};
}


auto EngineImpl::register_builtin_key_handler(BuiltinKeyHandler ident, std::string_view label,
                                              bool (EngineImpl::* func)(bool down)) -> render::KeyHandlerIdent
{
    std::function<bool(bool)> func_2 = [this, func](bool down) {
        return (this->*func)(down); // Throws
    }; // Throws
    render::KeyHandlerIdent handler = m_key_bindings.register_handler(label, std::move(func_2)); // Throws
    m_builtin_key_handlers[ident] = handler; // Throws
    return handler;
}


bool EngineImpl::key_func_shift_modifier(bool down)
{
    auto mode = m_key_bindings.get_modifier_mode();
    if (down) {
        mode = mode | render::modif_shift;
    }
    else {
        mode = mode & ~render::modif_shift;
    }
    m_key_bindings.set_modifier_mode(mode);
    return true;
}


bool EngineImpl::key_func_control_modifier(bool down)
{
    auto mode = m_key_bindings.get_modifier_mode();
    if (down) {
        mode = mode | render::modif_ctrl;
    }
    else {
        mode = mode & ~render::modif_ctrl;
    }
    m_key_bindings.set_modifier_mode(mode);
    return true;
}


bool EngineImpl::key_func_alt_modifier(bool down)
{
    auto mode = m_key_bindings.get_modifier_mode();
    if (down) {
        mode = mode | render::modif_alt;
    }
    else {
        mode = mode & ~render::modif_alt;
    }
    m_key_bindings.set_modifier_mode(mode);
    return true;
}


bool EngineImpl::key_func_meta_modifier(bool down)
{
    auto mode = m_key_bindings.get_modifier_mode();
    if (down) {
        mode = mode | render::modif_meta;
    }
    else {
        mode = mode & ~render::modif_meta;
    }
    m_key_bindings.set_modifier_mode(mode);
    return true;
}


bool EngineImpl::key_func_quit(bool down)
{
    if (down)
        m_quit = true;
    return false; // Interrupt event processing
}


bool EngineImpl::key_func_inc_frame_rate(bool down)
{
    if (down) {
        set_frame_rate(m_frame_rate * 2); // Throws
        // set_float_status(L"FRAME RATE = ", m_frame_rate);                                             
    }
    return true;
}


bool EngineImpl::key_func_dec_frame_rate(bool down)
{
    if (down) {
        set_frame_rate(m_frame_rate / 2); // Throws
        // set_float_status(L"FRAME RATE = ", m_frame_rate);                                             
    }
    return true;
}


bool EngineImpl::key_func_toggle_fullscreen(bool down)
{
    if (down)
        set_fullscreen_mode(!m_fullscreen_mode); // Throws
    return true;
}


bool EngineImpl::key_func_reset_view(bool down)
{
    if (down) {
        reset_view(); // Throws
        // FIXME: Show "RESET VIEW" is status HUD         
    }
    return true;
}


bool EngineImpl::key_func_toggle_headlight(bool down)
{
    if (down) {
        set_headlight_mode(!m_headlight_mode); // Throws
        // set_on_off_status(L"HEADLIGHT", m_headlight_mode);                       
    }
    return true;
}


bool EngineImpl::key_func_toggle_wireframe(bool down)
{
    if (down) {
        set_wireframe_mode(!m_wireframe_mode); // Throws
        // set_on_off_status(L"WIREFRAME", m_wireframe_mode);                       
    }
    return true;
}


void EngineImpl::redraw()
{
    if (ARCHON_UNLIKELY(m_need_misc_update)) {
#if ARCHON_RENDER_HAVE_OPENGL
/*
        GLfloat params[] = { GLfloat(m_global_ambience),
                             GLfloat(m_global_ambience),
                             GLfloat(m_global_ambience), 1 };
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, params);
*/
        glClearColor(m_background_color[0], m_background_color[1],
                     m_background_color[2], m_background_color[3]);
#endif // ARCHON_RENDER_HAVE_OPENGL
        m_need_misc_update = false;
    }

    if (ARCHON_UNLIKELY(m_projection_and_viewport_need_update)) {
        update_projection_and_viewport();
        m_projection_and_viewport_need_update = false;
    }

    render_frame(); // Throws

    m_window->opengl_swap_buffers(); // Throws

#if ARCHON_RENDER_HAVE_OPENGL
    if (m_max_opengl_errors > 0) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            m_logger.error("OpenGL error: %s", render::get_opengl_error_message(error)); // Throws
            m_max_opengl_errors -= 1;
            if (m_max_opengl_errors == 0)
                m_logger.error("No more OpenGL error will be reported"); // Throws
        }
    }
#endif // ARCHON_RENDER_HAVE_OPENGL
}


inline bool EngineImpl::process_events(Clock::time_point deadline)
{
    // See also EventHandler::on_blur()
    bool proceed = m_key_bindings.resume_incomplete_on_blur_if_any(); // Throws

    if (ARCHON_LIKELY(proceed))
        return m_conn.process_events_a(deadline); // Throws

    return false; // Interrupt (no expiration yet)
}


void EngineImpl::tick(Clock::time_point time_of_tick)
{
    math::Rotation orientation = m_trackball.get_orientation(time_of_tick);
    if (ARCHON_UNLIKELY(orientation != m_orientation)) {
        m_orientation = orientation;
        m_need_redraw = true;
    }

    if (ARCHON_UNLIKELY(m_scene.tick(time_of_tick))) // Throws
        m_need_redraw = true;
}


void EngineImpl::render_frame()
{
    // Handle headlight feature
    if (ARCHON_UNLIKELY(m_headlight_feature_enabled && m_headlight_mode != m_headlight_mode_prev)) {
#if ARCHON_RENDER_HAVE_OPENGL
        if (m_headlight_mode) {
            glEnable(GL_LIGHT0);
        }
        else {
            glDisable(GL_LIGHT0);
        }
#endif // ARCHON_RENDER_HAVE_OPENGL
        m_headlight_mode_prev = m_headlight_mode;
    }

    // Handle wireframe feature
    if (ARCHON_UNLIKELY(m_wireframe_feature_enabled && m_wireframe_mode != m_wireframe_mode_prev)) {
#if ARCHON_RENDER_HAVE_OPENGL
        glPolygonMode(GL_FRONT_AND_BACK, m_wireframe_mode ? GL_LINE : GL_FILL);
#endif // ARCHON_RENDER_HAVE_OPENGL
        m_wireframe_mode_prev = m_wireframe_mode;
    }

#if ARCHON_RENDER_HAVE_OPENGL
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslated(0, 0, -m_perspect_proj.camera_dist);
    glRotated(core::rad_to_deg(m_orientation.angle), m_orientation.axis[0], m_orientation.axis[1],
              m_orientation.axis[2]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif // ARCHON_RENDER_HAVE_OPENGL

    m_scene.render(); // Throws
}


void EngineImpl::update_projection_and_viewport()
{
    update_perpect_proj_and_trackball(); // Throws

#if ARCHON_RENDER_HAVE_OPENGL

    double view_plane_right = m_perspect_proj.get_near_clip_width()  / 2;
    double view_plane_top   = m_perspect_proj.get_near_clip_height() / 2;
    double view_plane_dist  = m_perspect_proj.get_near_clip_dist();
    double far_clip_dist    = m_perspect_proj.get_far_clip_dist();

/*
    m_logger.trace("Camera distance:    %s", m_perspect_proj.camera_dist); // Throws
    m_logger.trace("Near clip distance: %s", view_plane_dist); // Throws
    m_logger.trace("Far clip distance:  %s", far_clip_dist); // Throws
    m_logger.trace("View plane width:   %s", m_perspect_proj.get_near_clip_width()); // Throws
    m_logger.trace("View plane height:  %s", m_perspect_proj.get_near_clip_height()); // Throws
    m_logger.trace("Window size:        %s", m_window_size); // Throws
*/

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-view_plane_right, view_plane_right, -view_plane_top, view_plane_top, view_plane_dist, far_clip_dist);
    glViewport(0, 0, m_window_size.width, m_window_size.height);
#endif // ARCHON_RENDER_HAVE_OPENGL
}


void EngineImpl::update_perpect_proj_and_trackball() noexcept
{
    m_perspect_proj.set_viewport_size_pixels(m_window_size.width, m_window_size.height);
    m_perspect_proj.auto_dist(m_interest_size, m_perspect_proj.get_min_field_factor());
    m_trackball.set_viewport_size(m_window_size);
}


void EngineImpl::update_window_size(display::Size size) noexcept
{
    m_window_size = size;
    m_projection_and_viewport_need_update = true;
    m_need_redraw = true;
    // FIXME: Mark status HUD dirty     
}


void EngineImpl::update_window_pos(display::Pos pos) noexcept
{
    m_window_pos = pos;
}


void EngineImpl::fetch_screen_conf()
{
    if (ARCHON_LIKELY(m_resolution_tracking_enabled || m_frame_rate_tracking_enabled)) {
        m_num_viewports = 0;
        m_conn.try_get_screen_conf(m_screen, m_viewports, m_viewport_strings, m_num_viewports); // Throws
    }
}


void EngineImpl::track_screen_conf()
{
    if (ARCHON_LIKELY(m_resolution_tracking_enabled || m_frame_rate_tracking_enabled)) {
        display::Resolution resolution = m_default_resolution;
        double frame_rate = m_default_frame_rate;
        core::Span viewports = { m_viewports.data(), m_num_viewports };
        std::size_t i = display::find_viewport(viewports, m_window_pos, m_window_size);
        if (ARCHON_LIKELY(i != std::size_t(-1))) {
            const display::Viewport& viewport = m_viewports[i];
            if (ARCHON_LIKELY(viewport.resolution.has_value()))
                resolution = viewport.resolution.value();
            if (ARCHON_LIKELY(viewport.refresh_rate.has_value()))
                frame_rate = viewport.refresh_rate.value();
        }

        if (ARCHON_UNLIKELY(m_resolution_tracking_enabled && resolution != m_resolution))
            update_resolution(resolution); // Throws
        if (ARCHON_UNLIKELY(m_frame_rate_tracking_enabled && frame_rate != m_frame_rate))
            update_frame_rate(frame_rate); // Throws
    }
}


void EngineImpl::update_resolution(const display::Resolution& resol)
{
    m_resolution = resol;
    if (m_started) {
        m_perspect_proj.set_resol_dpcm(resol.horz_ppcm, resol.vert_ppcm);
        double pixel_aspect_ratio = m_perspect_proj.horz_dot_pitch / m_perspect_proj.vert_dot_pitch;
        m_trackball.set_pixel_aspect_ratio(pixel_aspect_ratio);
        m_projection_and_viewport_need_update = true;
        m_need_redraw = true;
        m_logger.detail("Resolution (ppcm): %s", m_resolution); // Throws
    }
}


void EngineImpl::update_frame_rate(double rate)
{
    m_frame_rate = rate;
    if (m_started) {
        auto nanos_per_frame = std::chrono::nanoseconds::rep(std::floor(1E9 / m_frame_rate));
        auto time_per_frame = std::chrono::nanoseconds(nanos_per_frame);
        m_time_per_frame = std::chrono::duration_cast<Clock::duration>(time_per_frame);
        m_interrupt_before_sleep = true;
        m_refresh_rate_changed = true;
        m_logger.detail("Frame rate: %sf/s (%s per frame)", m_frame_rate, core::as_time(m_time_per_frame)); // Throws
    }
}


void EngineImpl::modify_dist(double diff)
{
    // The distance modification comes about indirectly. We modify the size of the sphere of
    // interest, and the auto-distance feature then makes the corresponding change in
    // distance.
    double step = std::pow(2, 1.0 / 8); // 8 steps to double
    set_interest_size(std::pow(step, diff) * m_interest_size); // Throws
    // FIXME: Should be shown in a status HUD (DIST = ...) -> HUD must be activated and updated where the new distance is calculated            
}


void EngineImpl::modify_zoom(double diff)
{
    double step = std::pow(2, 1.0 / 8); // 8 steps to double
    set_zoom_factor(std::pow(step, diff) * m_perspect_proj.zoom_factor); // Throws
    // FIXME: Should be shown in a status HUD (ZOOM = ...)            
}
