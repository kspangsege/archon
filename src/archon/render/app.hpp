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

#ifndef ARCHON_RENDER_APP_HPP
#define ARCHON_RENDER_APP_HPP

#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <memory>
#include <map>
#include <locale>
#include <string>
#include <chrono>

#include <GL/glu.h>

#include <archon/core/series.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/time.hpp>
#include <archon/core/config.hpp>
#include <archon/math/rotation.hpp>
#include <archon/math/coord_system.hpp>
#include <archon/util/perspect_proj.hpp>
#include <archon/graphics/virt_trackball.hpp>
#include <archon/display/connection.hpp>
#include <archon/render/text_formatter.hpp>


namespace archon {
namespace render {

/// A convenience base class for applications that wishes to render into a
/// window using OpenGL.
///
/// This class handles such things as creation of the window, creation of an
/// OpenGL rendering context, main event loop with frame rate control, and
/// navigation using a virtual trackball.
///
/// The OpenGL viewing system is initialized such that a distance of one in
/// global modelview coordinates should be considered as corresponding to 1
/// meter. This is, of course, only going to be true if the relevant
/// configuration parameters are set correctly.
///
/// In the default navigation mode, the distance from the camera to the point of
/// interest is adjusted automatically such that the sphere of interest fits
/// perfectly inside the current viewport. This is done by sliding the camera
/// along a line that keeps the camera angle constant. When the viewport size
/// changes, for example due to the window being resized, then the camera
/// distance will be adjusted accordingly.
///
/// The window and the GL rendering context are bound to the thread that
/// constructs the application object, and it remains bound until the
/// application object is destroyed. This means that both the constructor and
/// the destroctur of your derived class will be called with a bound GL context.
///
/// This class is not strongly thread-safe. It is, however, weakly-thread-safe,
/// so each instance must be accessed by at most one thread.
///
/// \sa ThreadSafety
///
/// \todo FIXME: This class should not derive privately from
/// display::EventHandler, since the overwritten virtual methods could be
/// redefined by a sub-class.
class Application: private display::EventHandler {
public:
    struct Config {
        /// The initial frame rate. The frame rate marks the upper limit of
        /// frames per second.
        ///
        /// The frame rate can be changed at any time by calling
        /// <tt>set_frame_rate</tt>. It can also be manipulated interactively by
        /// pressing '+' and '-'-keys on the number keypad.
        ///
        /// The default is 59.95.
        double frame_rate = 59.95;

        /// The initial size (width, height) in pixels of the windows contents
        /// area.
        ///
        /// The window size can be changed at any time by calling
        /// <tt>set_window_size</tt>. If there is a window manager, then the
        /// size can also be manipulated interactively by the user.
        ///
        /// The default is (500, 500).
        core::Series<2, int> win_size = 500;

        /// The initial position (x,y) in pixels of the upper left corner of the
        /// outside window frame, relative to the upper left corner of the
        /// screen.
        ///
        /// If any of the two coordinates are negative, both coordinates are
        /// ignored, and the window manager will choose the initial position.
        ///
        /// The window position can be changed at any time by calling
        /// <tt>set_window_pos</tt>. If there is a window manager, then the
        /// position can also be manipulated interactively by the user.
        ///
        /// The defaut is (-1, -1).
        core::Series<2, int> win_pos = -1;

        /// The resolution (horizontal, vertical) of the target screen in dots
        /// per centimeter. If the value in one direction is zero or negative,
        /// then the effective value in that direction will be determinaed
        /// automatically, which may, or may not yield an accurate result.
        ///
        /// To translate from dots per inch (dpi) to dots per centimeter, divide
        /// by 2.54 cm/in.
        ///
        /// Specifying the wrong values here will produce the wrong field of
        /// view, which in turn will produce the wrong aspect ratio between the
        /// Z-axis and the X-Y-plane, which in turn leads to the depth effect
        /// appearing either stretched or squeezed. It may also produce the
        /// wrong aspect ratio between the X and Y-axes, which will lead to
        /// circles in the X-Y-plane appearing egg-shaped.
        ///
        /// The screen resolution can be changed at any time by calling
        /// <tt>set_screen_dpcm</tt>.
        ///
        /// The default is (0,0).
        core::Series<2, double> scr_dpcm = 0.0;

        /// The initial distance in meters between your eyes and the
        /// screen. Specifying the wrong distance here will produce the wrong
        /// field of view, which in turn will produce the wrong aspect ratio
        /// between the Z-axis and the X-Y plane, which in turn leads to the
        /// depth effect appearing either stretched or squeezed.
        ///
        /// The eye-to-screen distance can be changed at any time by calling
        /// <tt>set_eye_screen_dist</tt>.
        ///
        /// The default is 0.5, which corresponds to 50cm.
        double eye_scr_dist = 0.5;

        /// The initial depth of field. The depth of field is the ratio between
        /// the depth of the near and the far clipping planes. It must be
        /// greater than 1. Smaller values produce more accurate depth tests but
        /// makes it more likely that your scene will be clipped.
        ///
        /// The depth of filed can be changed at any time by calling
        /// <tt>set_depth_of_field</tt>.
        ///
        /// The default is 1000.
        double depth_of_field = 1000;

        /// The diameter of the initial sphere of interest in global modelview
        /// coordinates. By default, the viewing frustum will be made as narrow
        /// as possible while it still contains the sphere of interest
        /// completely.
        ///
        /// The interest ball diameter can be changed at any time by calling
        /// <tt>set_interest_size</tt>.
        ///
        /// The default is 2.
        double interest_size = 2;

        /// The zoom factor. When you double the zoom factor, you double the
        /// size of the on-screen projections of scene features.
        ///
        /// The zoom factor can be changed at any time by calling
        /// <tt>set_zoom_factor</tt>.
        ///
        /// The default is 1.
        double zoom = 1;

        /// The initial level of detail. The level of detail controls the
        /// general quality of the rendering, for example, by adjusting the
        /// number of faces used to render a curved surface. A value of 1
        /// corresponds to the normal level of detail, while a value of 2
        /// corresponds to twice the normal level of detail. Any value is
        /// allowed.
        ///
        /// The level of detail can be changed at any time by calling
        /// <tt>set_detail_level</tt>.
        ///
        /// The default is 1.
        double detail_level = 1;

        /// If true, attempt to create a direct rendering contexts to gain
        /// performance. This may fail, in which case, there will be a silent
        /// fallback to indirect rendering.
        ///
        /// The default is 'true'.
        bool direct_render = true;

        /// If true, open all windows in fullscreen mode.
        ///
        /// Fullscreen mode for all open windows can be enabled and disabled at
        /// any time by calling <tt>set_fullscreen_enabled</tt>. Fullscreen mode
        /// can also be toggled interactively for any window by pressing the
        /// 't'-key while the window has focus.
        ///
        /// The default is 'false'.
        bool fullscreen = false;

        /// If true, the headlight will be turned on initially.
        ///
        /// The default is 'true'.
        bool headlight = true;

        /// The global ambient intencity.
        ///
        /// For each shaded pixel, this value times the ambient color of the
        /// material is aded to the final color of the pixel.
        ///
        /// The default is 1/5.
        double ambience = 0.2;

        /// The background color specified as a RGBA quadruple.
        ///
        /// The default is (0,0,0,0).
        core::Series<4, double> bgcolor = 0.0;

        /// The glyph resolution (horiz,vert) used by the default font
        /// provider. This is actually the resulution of the EM-square, and
        /// fractional values are allowed.
        ///
        /// The default is (64,64).
        core::Series<2, float> glyph_resol = 64;

        /// If true, mipmapping is used from glyph textures generated by the
        /// default font provider.
        ///
        /// The default is 'true'.
        bool glyph_mipmap = true;

        /// Save all glyph textures, generated by the default font provider, as
        /// images.
        ///
        /// The default is 'false'.
        bool glyph_save = false;

        /// The path to the directory in which the idiosyncratic read-only
        /// architecture-independent data objects used by the Archon libraries
        /// are installed. It must be specified with a trailing slash.
        ///
        /// The default is determined at build time, and is usually
        /// `/usr/share/archon/`.
        std::string archon_datadir;

        void populate(core::ConfigBuilder&);

        Config();
    };

    // Get the rotation that, when applied to the coordinate system of the
    // scene, produces the coordinate system of the current view. It is assumed
    // here that the negative z-axis and the positive y-axis of the view
    // coordinate system is the view direction and the up direction
    // respectively.
    math::Rotation3 get_view_orientation() const noexcept;

    void set_window_size(int w, int h);
    void set_window_pos(int x, int y);
    void set_fullscreen_enabled(bool enable);
    void set_headlight_enabled(bool enable);
    void set_frame_rate(double r);
    void set_scene_orientation(math::Rotation3);
    void set_scene_spin(math::Rotation3);
    void set_detail_level(double level);
    void set_interest_size(double diameter);
    void set_zoom_factor(double zoom);
    void set_eye_screen_dist(double dist);
    void set_screen_dpcm(double horiz, double vert); // An arg that is zero or negative will be ignored.
    void set_depth_of_field(double ratio);
    void set_wireframe_enabled(bool enable);
    void set_axes_display_enabled(bool enable);
    void set_global_ambience(double intencity);
    void set_background_color(math::Vec4 rgba);


    using clock = std::chrono::steady_clock;

    void set_status(std::wstring text, clock::time_point timeout = clock::time_point());

    void set_int_status(std::wstring prefix, int value, std::wstring suffix = L"",
                        clock::time_point timeout = clock::time_point());

    void set_float_status(std::wstring prefix, double value, int precision = 2,
                          std::wstring suffix = L"", clock::time_point timeout = clock::time_point());

    void set_on_off_status(std::wstring prefix, bool value,
                           clock::time_point timeout = clock::time_point());

    void activate_status(clock::time_point timeout = clock::time_point());

    enum class BuiltinKeyHandler {
        shift_modifier,
        quit,
        reset_view,
        inc_frame_rate,
        dec_frame_rate,
        show_help,
        toggle_headlight,
        toggle_fullscreen,
        toggle_wireframe,
        toggle_show_axes,
        toggle_status_hud
    };

    int get_builtin_key_handler(BuiltinKeyHandler) const noexcept;

    /// Register an event handler for the specified key. The \c down argument of
    /// the handler will be set to \c true when the key is pressed down, and \c
    /// false when it is released.
    ///
    /// The handler must decide if the processing of the event requires a redraw
    /// of the window contents. If, yes, it must return <tt>true</tt>, otherwise
    /// it must return <tt>false</tt>.
    int register_key_handler(std::function<bool(bool down)>, std::string description);

    enum class KeyModifier { none, shift };

    struct KeyPressMultiplicity {
        int value;
    };

    static constexpr KeyPressMultiplicity single_tap   = KeyPressMultiplicity{1};
    static constexpr KeyPressMultiplicity single_click = KeyPressMultiplicity{1};
    static constexpr KeyPressMultiplicity double_tap   = KeyPressMultiplicity{2};
    static constexpr KeyPressMultiplicity double_click = KeyPressMultiplicity{2};

    int bind_key(display::KeySym, std::function<bool(bool down)>, std::string description);
    int bind_key(display::KeySym, KeyModifier, std::function<bool(bool down)>,
                 std::string description);
    int bind_key(display::KeySym, KeyModifier, KeyPressMultiplicity,
                 std::function<bool(bool down)>, std::string description);
    void bind_key(display::KeySym, int handler_index);
    void bind_key(display::KeySym, KeyModifier, int handler_index);
    void bind_key(display::KeySym, KeyModifier, KeyPressMultiplicity, int handler_index);
    int unbind_key(display::KeySym, KeyModifier = KeyModifier::none,
                   KeyPressMultiplicity = single_tap) noexcept;
    int get_key_binding(display::KeySym, KeyModifier = KeyModifier::none,
                        KeyPressMultiplicity = single_tap) noexcept;

    int bind_button(int button_number, std::function<bool(bool down)>, std::string description);
    int bind_button(int button_number, KeyModifier, std::function<bool(bool down)>,
                    std::string description);
    int bind_button(int button_number, KeyModifier, KeyPressMultiplicity,
                    std::function<bool(bool down)>, std::string description);
    void bind_button(int button_number, int handler_index);
    void bind_button(int button_number, KeyModifier, int handler_index);
    void bind_button(int button_number, KeyModifier, KeyPressMultiplicity, int handler_index);
    int unbind_button(int button_number, KeyModifier = KeyModifier::none,
                      KeyPressMultiplicity = single_click) noexcept;
    int get_button_binding(int button_number, KeyModifier = KeyModifier::none,
                           KeyPressMultiplicity = single_click) noexcept;

    void run();

    /// \param eye The current position of the eye/camera in global coordinates.
    ///
    /// \param screen A description of the position and orientation of the
    /// current viewing frame/screen in global coordinates. The actual frame is
    /// the unit sqaure within the described 2-D coordinate system. The frame is
    /// guaranteed to be rectangular in 3-space.
    void get_current_view(math::Vec3& eye, math::CoordSystem3x2& screen);

    int get_window_width();
    int get_window_height();
    bool is_headlight_enabled();
    double get_global_ambience();
    math::Vec4 get_background_color();

    TextureDecl declare_texture(std::string image_path, bool repeat = true, bool mipmap = true);
    TextureUse load_texture(std::string image_path, bool repeat = true, bool mipmap = true);
    TextureCache& get_texture_cache();
    FontProvider& get_font_provider();

    virtual ~Application();

protected:
    /// \brief Render the scene.
    ///
    /// Render the scene in its current state. Multiple sequential calls with no
    /// inbetween calls of tick() must produce the same result, i.e., the state
    /// of the scene must be unchanged.
    ///
    /// This function may be called many times per tick to redraw the scene. If
    /// may also be called less than once per tick, depending on such things as
    /// when the application return true from tick().
    virtual void render();

    /// \brief Opportunity to update the state of the scene.
    ///
    /// Called once per frame (barring lag) according to the currently selected
    /// frame rate. The application can use this opportunity to update the state
    /// of the scene.
    ///
    /// \return True if the state of the scene was changed in a way that
    /// requires it to be redrawn. Otherwise false.
    virtual bool tick(clock::time_point now);

    int adjust_detail(int val, int min);

    /// \param title The desired title for the application window.
    ///
    /// \param texture_cache If unspecified, a new texture cache will be created
    /// on demand.
    ///
    /// \todo FIXME: Is the title specified using a UTF-8 encoded string?
    Application(std::string title = "", const Config& cfg = Config(),
                const std::locale& loc = std::locale::classic(),
                display::Connection::Arg conn = display::Connection::Ptr(),
                TextureCache* texture_cache = nullptr,
                std::shared_ptr<font::FontCache> font_cache = nullptr);

private:
    int adjust(int val, int min, double f)
    {
        return std::max(int(std::ceil(f*val)), min);
    }

    void redraw();
    void internal_tick(clock::time_point);
    void emit_gl_error(GLenum error, bool last);
    void set_viewport_size(int w, int h);
    void update_gl_projection();
    void update_proj_and_trackball();
    void render_frame();
    void render_hud();
    void modify_zoom(int diff);
    void modify_dist(int diff);

    void on_resize(const display::SizeEvent&) override;
    void on_close(const display::Event&) override;
    void on_keydown(const display::KeyEvent&) override;
    void on_keyup(const display::KeyEvent&) override;
    void on_mousedown(const display::MouseButtonEvent&) override;
    void on_mouseup(const display::MouseButtonEvent&) override;
    void on_mousemove(const display::MouseEvent&) override;
    void on_show(const display::Event&) override;
    void on_hide(const display::Event&) override;
    void on_damage(const display::AreaEvent&) override;
    bool before_sleep() override;

    clock::time_point get_status_hud_timout();

    display::Connection::Ptr m_conn;
    display::Context::Ptr m_ctx;
    display::EventProcessor::Ptr m_event_proc;
    std::unique_ptr<display::Cursor> m_cursor_normal, m_cursor_trackball;
    display::Window::Ptr m_win;
    display::Bind m_gl_binding;

    util::PerspectiveProjection m_proj;
    math::Rotation3 m_orientation = math::Rotation3::zero();
    graphics::VirtualTrackball m_trackball;

    int m_max_gl_errors = 20; // Max OpenGL errors to be reported
    int m_win_x, m_win_y;
    bool m_win_pos_set = false;
    double m_frame_rate;
    clock::duration m_time_per_frame;
    double m_detail_level;
    bool m_need_misc_update = false, m_projection_needs_update = false;
    double m_interest_size;
    int m_viewport_width, m_viewport_height;
    bool m_need_redraw = false, m_need_immediate_redraw = false;
    bool m_but1_down = false, m_fullscreen_mode = false;
    bool m_headlight = true, m_headlight_prev = false, m_headlight_blocked = false;
    bool m_wireframe_mode = false, m_wireframe_mode_prev = false, m_wireframe_mode_blocked = false;
    bool m_axes_display = false, m_axes_display_first = true;
    bool m_terminate = false;
    double m_global_ambience;
    math::Vec4 m_background_color;

    math::Rotation3 m_initial_orientation;
    double m_initial_interest_size, m_initial_zoom_factor;
    bool m_first_run = true;

    GLUquadric* m_quadric = nullptr;
    GLuint m_one_axis_dpy_list = 0, m_all_axes_dpy_list;

    std::wstring m_status_hud_text;
    bool m_status_hud_enabled = true; // If false, the display of new status messages is inhibited.
    bool m_status_hud_active = false; // Whether the status HUD is currently being displayed.
    clock::time_point m_status_hud_timeout; // The point at which the the status HUD should disappear.
    GLuint m_status_hud_disp_list = 0; // Name of the OpenGL display list that renders the status HUD, or zero if no list has been created yet.
    bool m_status_hud_dirty = true; // True indicates that the OpenGL display list needs to be updated.
    bool m_status_hud_activate_cam_dist = false; // True indicates that the camera distance must be displayed as soon as the projection has been updated.
    clock::time_point m_status_hud_activate_cam_dist_timeout; // Use this timeout when displaying the camera distance.

    class PrivateState;
    const std::shared_ptr<PrivateState> m_private_state;

    struct KeyHandler {
        std::function<bool(bool down)> callback;
        std::string description;
    };

    class KeyIdent {
    public:
        static KeyIdent from_key_sym(display::KeySym key_sym) noexcept
        {
            KeyIdent key;
            key.m_is_key_sym = true;
            key.m_key_sym = key_sym;
            return key;
        }
        static KeyIdent from_button_number(int button_number) noexcept
        {
            KeyIdent key;
            key.m_is_key_sym = false;
            key.m_button_number = button_number;
            return key;
        }
        bool operator<(KeyIdent key) const noexcept
        {
            if (int(m_is_key_sym) < int(key.m_is_key_sym))
                return true;
            if (m_is_key_sym != key.m_is_key_sym)
                return false;
            if (m_is_key_sym) {
                return m_key_sym < key.m_key_sym;
            }
            return m_button_number < key.m_button_number;
        }
    private:
        bool m_is_key_sym;
        union {
            display::KeySym m_key_sym;
            int m_button_number;
        };
    };

    using key_press_count_type = std::int_fast64_t;

    struct KeyPressMultiplicitySlot {
        key_press_count_type press_count_at_last_press = 0;
        int handler_index = -1;
    };

    struct KeyModifierSlot {
        // The key of this map is key press multiplicity. A value of 1 means
        // "single press/single click", a value of 2 means "double press/double
        // click", and so fourth.
        std::map<int, KeyPressMultiplicitySlot> multiplicities;
    };

    struct KeySlot {
        using Timestamp = std::chrono::milliseconds;
        key_press_count_type press_count = 0;
        // A previous multiplicity of zero acts like a barrier preventing
        // connection between next key press and earlier presses.
        int prev_press_multiplicity = 0;
        Timestamp prev_press_time = Timestamp{};
        int down_handler_index = -1;
        std::map<KeyModifier, KeyModifierSlot> modifiers;
    };

    std::vector<KeyHandler> m_key_handlers;

    std::map<KeyIdent, KeySlot> m_key_bindings;

    std::map<BuiltinKeyHandler, int> m_builtin_key_handlers;

    KeyModifier m_key_modifier = KeyModifier::none;

    std::int_fast64_t m_num_dropped_frames = 0, m_num_dropped_frames_report = 1;

    void on_key_down_or_up(KeyIdent, bool down, KeySlot::Timestamp);

    int register_builtin_key_handler(bool (Application::*handler)(bool down),
                                     std::string description, BuiltinKeyHandler);
    void do_bind_key(KeyIdent, KeyModifier, int multiplicity, int handler_index);
    int do_unbind_key(KeyIdent, KeyModifier, int multiplicity) noexcept;
    int do_get_key_binding(KeyIdent, KeyModifier, int multiplicity) noexcept;

    bool key_func_shift_modifier(bool down);
    bool key_func_quit(bool down);
    bool key_func_reset_view(bool down);
    bool key_func_inc_frame_rate(bool down);
    bool key_func_dec_frame_rate(bool down);
    bool key_func_show_help(bool down);
    bool key_func_toggle_headlight(bool down);
    bool key_func_toggle_fullscreen(bool down);
    bool key_func_toggle_wireframe(bool down);
    bool key_func_toggle_show_axes(bool down);
    bool key_func_toggle_status_hud(bool down);
};




// Implementation

inline math::Rotation3 Application::get_view_orientation() const noexcept
{
    return -m_orientation;
}

constexpr Application::KeyPressMultiplicity Application::single_tap;
constexpr Application::KeyPressMultiplicity Application::single_click;
constexpr Application::KeyPressMultiplicity Application::double_tap;
constexpr Application::KeyPressMultiplicity Application::double_click;

inline int Application::bind_key(display::KeySym key, std::function<bool(bool down)> handler,
                                 std::string descr)
{
    return bind_key(key, KeyModifier::none, std::move(handler), std::move(descr)); // Throws
}

inline int Application::bind_key(display::KeySym key, KeyModifier mod,
                                 std::function<bool(bool down)> handler, std::string descr)
{
    return bind_key(key, mod, single_tap, std::move(handler), std::move(descr)); // Throws
}

inline int Application::bind_key(display::KeySym key, KeyModifier mod, KeyPressMultiplicity mul,
                                 std::function<bool(bool down)> handler, std::string descr)
{
    int handler_index = register_key_handler(std::move(handler), std::move(descr)); // Throws
    bind_key(key, mod, mul, handler_index); // Throws
    return handler_index;
}

inline void Application::bind_key(display::KeySym key, int handler)
{
    bind_key(key, KeyModifier::none, handler); // Throws
}

inline void Application::bind_key(display::KeySym key, KeyModifier mod, int handler)
{
    bind_key(key, mod, single_tap, handler); // Throws
}

inline void Application::bind_key(display::KeySym key, KeyModifier mod, KeyPressMultiplicity mul,
                                  int handler)
{
    do_bind_key(KeyIdent::from_key_sym(key), mod, mul.value, handler); // Throws
}

inline int Application::unbind_key(display::KeySym key, KeyModifier mod,
                                   KeyPressMultiplicity mul) noexcept
{
    return do_unbind_key(KeyIdent::from_key_sym(key), mod, mul.value);
}

inline int Application::get_key_binding(display::KeySym key, KeyModifier mod,
                                        KeyPressMultiplicity mul) noexcept
{
    return do_get_key_binding(KeyIdent::from_key_sym(key), mod, mul.value);
}

inline int Application::bind_button(int but, std::function<bool(bool down)> handler,
                                    std::string descr)
{
    return bind_button(but, KeyModifier::none, std::move(handler), std::move(descr)); // Throws
}

inline int Application::bind_button(int but, KeyModifier mod,
                                    std::function<bool(bool down)> handler, std::string descr)
{
    return bind_button(but, mod, single_click, std::move(handler), std::move(descr)); // Throws
}

inline int Application::bind_button(int but, KeyModifier mod, KeyPressMultiplicity mul,
                                    std::function<bool(bool down)> handler, std::string descr)
{
    int handler_index = register_key_handler(std::move(handler), std::move(descr)); // Throws
    bind_button(but, mod, mul, handler_index); // Throws
    return handler_index;
}

inline void Application::bind_button(int but, int handler)
{
    bind_button(but, KeyModifier::none, handler); // Throws
}

inline void Application::bind_button(int but, KeyModifier mod, int handler)
{
    bind_button(but, mod, single_click, handler); // Throws
}

inline void Application::bind_button(int but, KeyModifier mod, KeyPressMultiplicity mul,
                                     int handler)
{
    do_bind_key(KeyIdent::from_button_number(but), mod, mul.value, handler); // Throws
}

inline int Application::unbind_button(int but, KeyModifier mod,
                                      KeyPressMultiplicity mul) noexcept
{
    return do_unbind_key(KeyIdent::from_button_number(but), mod, mul.value);
}

inline int Application::get_button_binding(int but, KeyModifier mod,
                                           KeyPressMultiplicity mul) noexcept
{
    return do_get_key_binding(KeyIdent::from_button_number(but), mod, mul.value);
}

inline int Application::get_window_width()
{
    return m_viewport_width;
}

inline int Application::get_window_height()
{
    return m_viewport_height;
}

inline bool Application::is_headlight_enabled()
{
    return m_headlight;
}

inline double Application::get_global_ambience()
{
    return m_global_ambience;
}

inline math::Vec4 Application::get_background_color()
{
    return m_background_color;
}

inline TextureUse Application::load_texture(std::string image_path, bool repeat, bool mipmap)
{
    return declare_texture(image_path, repeat,  mipmap).acquire();
}

inline int Application::adjust_detail(int val, int min)
{
    return adjust(val, min, m_detail_level);
}

} // namespace render
} // namespace archon

#endif // ARCHON_RENDER_APP_HPP
