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
#include <map>
#include <locale>
#include <string>

#include <GL/glu.h>

#include <archon/core/series.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/core/shared_ptr.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/time.hpp>
#include <archon/core/config.hpp>
#include <archon/math/rotation.hpp>
#include <archon/math/coord_system.hpp>
#include <archon/util/perspect_proj.hpp>
#include <archon/graphics/virt_trackball.hpp>
#include <archon/display/connection.hpp>
#include <archon/render/text_formatter.hpp>


namespace Archon {
namespace Render {

/// Thrown if an attempt is made to register a key handler with a key that is
/// already bound to a handler.
///
/// \sa Application::register_key_handler
class KeyHandlerConflictException: public std::runtime_error {
public:
    KeyHandlerConflictException(const std::string& message):
        std::runtime_error(message)
    {
    }
};


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
/// Display::EventHandler, since the overwritten virtual methods could be
/// redefined by a sub-class.
class Application: private Display::EventHandler {
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
        Core::Series<2, int> win_size = 500;

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
        Core::Series<2, int> win_pos = -1;

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
        Core::Series<2, double> scr_dpcm = 0.0;

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
        Core::Series<4, double> bgcolor = 0.0;

        /// The glyph resolution (horiz,vert) used by the default font
        /// provider. This is actually the resulution of the EM-square, and
        /// fractional values are allowed.
        ///
        /// The default is (64,64).
        Core::Series<2, float> glyph_resol = 64;

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

        void populate(Core::ConfigBuilder&);

        Config();
    };


    void set_window_size(int w, int h);
    void set_window_pos(int x, int y);
    void set_fullscreen_enabled(bool enable);
    void set_headlight_enabled(bool enable);
    void set_frame_rate(double r);
    void set_scene_orientation(Math::Rotation3 rot);
    void set_scene_spin(Math::Rotation3 rot);
    void set_detail_level(double level);
    void set_interest_size(double diameter);
    void set_zoom_factor(double zoom);
    void set_eye_screen_dist(double dist);
    void set_screen_dpcm(double horiz, double vert); // An arg that is zero or negative will be ignored.
    void set_depth_of_field(double ratio);
    void set_wireframe_enabled(bool enable);
    void set_axes_display_enabled(bool enable);
    void set_global_ambience(double intencity);
    void set_background_color(Math::Vec4 rgba);


    void set_status(std::wstring text, Core::Time timeout = Core::Time());

    void set_int_status(std::wstring prefix, int value, std::wstring suffix = L"",
                        Core::Time timeout = Core::Time());

    void set_float_status(std::wstring prefix, double value, int precision = 2,
                          std::wstring suffix = L"", Core::Time timeout = Core::Time());

    void set_on_off_status(std::wstring prefix, bool value, Core::Time timeout = Core::Time());

    void activate_status(Core::Time timeout = Core::Time());


    /// Register an event handler for the specified key. The \c down argument of
    /// the handler will be set to \c true when the key is pressed down, and \c
    /// false when it is released.
    ///
    /// The handler must decide if the processing of the event requires a redraw
    /// of the window contents. If, yes, it must return <tt>true</tt>, otherwise
    /// it must return <tt>false</tt>.
    ///
    /// \throw KeyHandlerConflictException If the specified key is already bound
    /// to a handler.
    template<class C> void register_key_handler(Display::KeySym, bool (C::*handler)(bool down),
                                                std::string description);

    void run();


    /// \param eye The current position of the eye/camera in global coordinates.
    ///
    /// \param screen A description of the position and orientation of the
    /// current viewing frame/screen in global coordinates. The actual frame is
    /// the unit sqaure within the described 2-D coordinate system. The frame is
    /// guaranteed to be rectangular in 3-space.
    void get_current_view(Math::Vec3& eye, Math::CoordSystem3x2& screen);

    int get_window_width()
    {
        return viewport_width;
    }
    int get_window_height()
    {
        return viewport_height;
    }
    bool is_headlight_enabled()
    {
        return headlight;
    }
    double get_global_ambience()
    {
        return global_ambience;
    }
    Math::Vec4 get_background_color()
    {
        return background_color;
    }

    TextureDecl declare_texture(std::string image_path, bool repeat = true, bool mipmap = true);
    TextureUse load_texture(std::string image_path, bool repeat = true, bool mipmap = true);
    TextureCache& get_texture_cache();
    FontProvider* get_font_provider();

    virtual ~Application();

protected:
    virtual void render_scene() = 0;

    int adjust_detail(int val, int min)
    {
        return adjust(val, min, detail_level);
    }

    /// \param title The desired title for the application window.
    ///
    /// \param texture_cache If unspecified, a new texture cache will be created
    /// on demand.
    ///
    /// \todo FIXME: Is the title specified using a UTF-8 encoded string?
    Application(std::string title = "", const Config& cfg = Config(),
                const std::locale& loc = std::locale::classic(),
                Display::Connection::Arg conn = Display::Connection::Ptr(),
                TextureCache* texture_cache = nullptr,
                Font::FontCache::Arg font_cache = Font::FontCache::Ptr());

private:
    class InterruptException: public std::exception
    {
    };

    int adjust(int val, int min, double f)
    {
        return std::max(int(std::ceil(f*val)), min);
    }

    void emit_gl_error(GLenum error, bool last);
    void set_viewport_size(int w, int h);
    void update_gl_projection();
    void update_proj_and_trackball();
    void render_frame(Core::Time now);
    void render_hud();
    void update_observer(Core::Time now);
    void modify_zoom(int diff);
    void modify_dist(int diff);

    void on_resize(const Display::SizeEvent&);
    void on_close(const Display::Event&);
    void on_keydown(const Display::KeyEvent&);
    void on_keyup(const Display::KeyEvent&);
    void on_mousedown(const Display::MouseButtonEvent&);
    void on_mouseup(const Display::MouseButtonEvent&);
    void on_mousemove(const Display::MouseEvent&);
    void on_show(const Display::Event&);
    void on_hide(const Display::Event&);
    void on_damage(const Display::AreaEvent&);
    void before_sleep();

    class KeyHandlerBase;
    template<class> class KeyHandler;

    /// The OpenGL context is guaranteed to be bound while the handler executes.
    void register_key_handler(Display::KeySym key, Core::UniquePtr<KeyHandlerBase> handler,
                              std::string descr);

    Core::Time get_status_hud_timout();

    Display::Connection::Ptr conn;
    Display::Context::Ptr ctx;
    Display::EventProcessor::Ptr event_proc;
    Display::Cursor::Ptr cursor_normal, cursor_trackball;
    Display::Window::Ptr win;
    Display::Bind gl_binding;

    Util::PerspectiveProjection proj;
    Graphics::VirtualTrackball trackball;

    int max_gl_errors = 20; // Max OpenGL errors to be reported
    int win_x, win_y;
    bool win_pos_set = false;
    double frame_rate;
    Core::Time time_per_frame;
    double detail_level;
    bool need_misc_update = false, projection_needs_update = false;
    double interest_size;
    int viewport_width, viewport_height;
    bool need_refresh = false, but1_down = false, fullscreen_mode = false;
    bool headlight = true, headlight_prev = false, headlight_blocked = false;
    bool wireframe_mode = false, wireframe_mode_prev = false, wireframe_mode_blocked = false;
    bool axes_display = false, axes_display_first = true, shift_left_down = false, terminate = false;
    double global_ambience;
    Math::Vec4 background_color;

    Math::Rotation3 initial_rotation;
    double initial_interest_size, initial_zoom_factor;
    bool first_run = true;

    GLUquadric* quadric = nullptr;
    GLuint one_axis_dpy_list = 0, all_axes_dpy_list;

    Core::DeletingVector<KeyHandlerBase> key_handler_owner;
    typedef std::map<Display::KeySym, std::pair<KeyHandlerBase*, std::string> > KeyHandlers;
    KeyHandlers key_handlers;

    std::wstring status_hud_text;
    bool status_hud_enabled = true; // If false, the display of new status messages is inhibited.
    bool status_hud_active = false; // Whether the status HUD is currently being displayed.
    Core::Time status_hud_timeout; // The point at which the the status HUD should disappear.
    GLuint status_hud_disp_list = 0; // Name of the OpenGL display list that renders the status HUD, or zero if no list has been created yet.
    bool status_hud_dirty = true; // True indicates that the OpenGL display list needs to be updated.
    bool status_hud_activate_cam_dist = false; // True indicates that the camera distance must be displayed as soon as the projection has been updated.
    Core::Time status_hud_activate_cam_dist_timeout; // Use this timeout when displaying the camera distance.

    class PrivateState;
    const Core::SharedPtr<PrivateState> private_state;
};




// Implementation

inline TextureUse Application::load_texture(std::string image_path, bool repeat, bool mipmap)
{
    return declare_texture(image_path, repeat,  mipmap).acquire();
}

class Application::KeyHandlerBase {
public:
    virtual bool handle(Application* app, bool down) = 0;
    virtual ~KeyHandlerBase() {}
};

template<class C> class Application::KeyHandler: public Application::KeyHandlerBase {
public:
    KeyHandler(bool (C::*h)(bool)):
        handler(h)
    {
    }
    bool handle(Application* app, bool down)
    {
        return (static_cast<C*>(app)->*handler)(down);
    }
private:
    bool (C::*handler)(bool);
};

template<class C> void Application::register_key_handler(Display::KeySym key, bool (C::*handler)(bool),
                                                         std::string description)
{
    Core::UniquePtr<KeyHandlerBase> h(new KeyHandler<C>(handler));
    register_key_handler(key, h, description);
}

} // namespace Render
} // namespace Archon

#endif // ARCHON_RENDER_APP_HPP
