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

#ifndef ARCHON_X_RENDER_X_ENGINE_HPP
#define ARCHON_X_RENDER_X_ENGINE_HPP

/// \file


#include <utility>
#include <memory>
#include <string_view>
#include <string>
#include <locale>
#include <chrono>

#include <archon/core/concepts.hpp>
#include <archon/log/logger.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>
#include <archon/display.hpp>
#include <archon/render/key_binding_support.hpp>
#include <archon/render/impl/key_bindings.hpp>


namespace archon::render {


/// \brief Render application specified scene in window.
///
/// A render engine renders an application-specified scene (\ref Scene) in a window of the
/// platform's graphical user interface.
///
/// The outline of a very simple application could looks like this:
///
/// \code{.cpp}
///
///   class FooScene : public archon::render::Engine::Scene {
///   public:
///       FooScene(archon::render::Engine& engine)
///           : m_scene(scene)
///       {
///       }
///
///       void render() override
///       {
///           // ...
///       }
///
///   private:
///       archon::render::Engine& m_engine;
///   };
///
///   archon::render::Engine engine("Foo", 256, locale);                            
///   FooScene scene(engine);
///   engine.set_scene(scene);
///   engine.bind_key(archon::display::Key::lower_case_x, "X", [&](bool down) {
///       // ...
///   });
///   engine.run();
///
/// \endcode
///
/// Note that the engine is informed about the scene to be rendered using \ref
/// set_scene(). This allows for the scene to be constructed with a reference to the engine
/// object, as shown. Such an arrangement would have been harder for the application if the
/// scene to be rendered was specified at engine construction time.
///
/// Engine objects are not thread safe, meaning that if one thread is executing a member
/// function of an engine at a particular point in time, then no other thread is allowed to
/// execute a member function on the same engine object at the same point in time.
///
/// Key handlers (as shown above) are executed by the thread than executes \ref run(). With
/// the exception of \ref set_scene() and \ref run(), all the member functions of an engine
/// may be called from a key handler. These functions may also be called before \ref run()
/// is called.
///
/// Both \ref set_scene() and \ref run() may be called at most once, and \ref set_scene()
/// must be called before \ref run().
///
/// Each engine object establishes a single display connection (\ref
/// display::Connection). Using \ref Config::display_implementation, the application can
/// specify a particular display implementation to be used for establishing this display
/// connection. If a particular implementation is not specified, one will be selected using
/// \ref display::get_default_implementation() passing the specified display guarantees
/// (\ref Config::display_guarantees). If a particular implementation is specified, it must
/// be one that is available given the specified guarantees (\ref
/// display::Implementation::is_available()).
///
/// \sa \ref set_scene()
/// \sa \ref Scene::render()
///
class Engine {
public:
    struct Config;
    class Scene;
    enum class BuiltinKeyHandler;

    using Clock = std::chrono::steady_clock;

    /// \{
    ///
    /// \brief Create engine with specifically configured window.      
    ///
    /// These constructors create a render engine whose window has the specified title and          
    /// size (\p window_title, \p window_size). They are shorthands for creating a
    /// degenerate engine object and then calling \ref create() on it.
    ///
    /// \sa \ref create()
    ///
    Engine(display::Connection& conn, std::string_view window_title, display::Size window_size,
           const std::locale& locale);
    Engine(display::Connection& conn, std::string_view window_title, display::Size window_size,
           const std::locale& locale, const Config& config);
    /// \}

    /// \brief Create degenerate engine object.
    ///
    /// This constructor creates a degenerate engine object (as if moved from). Such an
    /// engine object can be made to hold an actual engine by calling \ref create() or \ref
    /// try_create().
    ///
    /// \sa \ref create()
    ///
    Engine() noexcept;

    /// \brief Create engine with specifically configured window.               
    ///
    /// This function creates a render engine whose window has the specified title and size               
    /// (\p window_title, \p window_size). It is shorthand for calling \ref try_create() and
    /// throwing an exception on failure.
    ///
    /// \sa \ref try_create()
    ///
    void create(display::Connection& conn, std::string_view window_title, display::Size window_size,
                const std::locale& locale, const Config& config);

    /// \brief Try to create engine with specifically configured window.           
    ///
    /// This function attempts to create a render engine whose window has the specified                 
    /// title and size (\p window_title, \p window_size). On success, this function returns
    /// `true`. On failure, it returns `false` after setting \p error to a message that
    /// describes the cause of the failure.
    ///
    /// When this function succeeds, the engine object becomes non-degenerate.
    ///
    /// If the engine object was not degenerate prior to the invocation of this function, it                                                              
    /// will be made degenerate as if by `*this = Engine()`, and this happens before the
    /// attempt to create a new engine. This avoids creation of a display connection while
    /// another one is already owned by the engine object (to avoid unintended violations of
    /// \ref display::Guarantees::only_one_connection).
    ///
    bool try_create(display::Connection& conn, std::string_view window_title, display::Size window_size,
                    const std::locale& locale, const Config& config, std::string& error);

    /// \brief Inform engine of scene to be rendered.
    ///
    /// This function informs the render engine of the scene to be rendered. This must be
    /// done before the engine starts to execute (\ref run()).
    ///
    /// This function must not be called while \ref run() is executing.
    ///
    /// If \ref run() is called, the specified scene object must not be destroyed until
    /// after `run()` returns.
    ///
    void set_scene(Scene&) noexcept;                             

    /// \brief Execute render engine.
    ///
    /// This function executes the render engine. It must be called at most once per engine
    /// object.
    ///
    /// The thread that executes this function is the thread that will be executing any
    /// registered key handler (\ref bind_key(), \ref register_key_handler()).
    ///
    void run();

    /// \brief Set target frame rate.
    ///
    /// This function sets the target frame rate. This will be the effective frame rate if
    /// the rendering of each frame is fast enough. If the rendering is not fast enough, the
    /// effective frame rate will be lower than what is specified.
    ///
    void set_frame_rate(double rate);

    /// \brief Set background color for rendered scene.
    ///
    /// This function sets the background color for the rendered scene.
    ///
    void set_background_color(util::Color color);

    /// \brief Set base orientation of virtual trackball.
    ///
    /// This function sets the base orientation, which is the orientation that is reset to
    /// when the view is reset. This function also sets the current orientation as if by
    /// `set_orientation(orientation)`.
    ///
    /// \sa \ref reset_view()
    /// \sa \ref set_orientation()
    ///<
    void set_base_orientation(const math::Rotation& orientation);

    /// \brief Set base spin of virtual trackball.
    ///
    /// This function sets the base spin, which is the spin that is reset to when the view
    /// is reset. This function also sets the current spin as if by `set_spin(spin)`.
    ///
    /// \sa \ref reset_view()
    /// \sa \ref set_spin()
    ///
    void set_base_spin(const math::Rotation& spin);

    /// \brief Set base zoom factor.
    ///
    /// This function sets the base zoom factor, which is the zoom factor that is reset to
    /// when the view is reset. This function also sets the current zoom factor as if by
    /// `set_zoom_factor(factor)`.
    ///
    /// \sa \ref reset_view()
    /// \sa \ref set_zoom_factor()
    ///
    void set_base_zoom_factor(double factor);

    /// \brief Set base interest size.
    ///
    /// This function sets the base interest size, which is the interest size that is reset
    /// to when the view is reset. This function also sets the current interest size as if
    /// by `set_interest_size(size)`.
    ///
    /// \sa \ref reset_view()
    /// \sa \ref set_interest_size()
    ///
    void set_base_interest_size(double size);

    /// \brief Get reference to engine's logger.
    ///
    /// This function returns a reference to the logger that is used by the render
    /// engine. If the application specifies a custom logger (\ref Config::logger), that
    /// logger is returned by this function. Otherwise, this function returns a reference to
    /// the logger that the engine falls back to. The fallback logger will be one that logs
    /// to STDOUT.
    ///
    auto get_logger() noexcept -> log::Logger&;

    /// \{
    ///
    /// \brief Register and bind a key handler.
    ///
    /// These functions are shorthands for calling \ref register_key_handler() with the
    /// specified label and function, then calling the corresponding overload of \ref
    /// bind_key_a() with the handler identifier returned by `register_key_handler()`.
    ///
    /// \sa \ref register_key_handler()
    /// \sa \ref bind_key_a()
    ///
    template<core::func_type<void(bool)> F> void bind_key(render::KeyIdent key, std::string_view label, F func);
    template<core::func_type<void(bool)> F> void bind_key(render::KeyIdent key, render::KeyModifierMode modifier,
                                                          render::KeyPressMultiplicity multiplicity,
                                                          std::string_view label, F func);
    /// \}

    /// \brief Register key handler with render engine.
    ///
    /// This function registers the specified handler function with the render engine,
    /// making it available as a target for biding keys using \ref bind_key_a(). The handler
    /// function is registered under the specified label. The returned handler identifier
    /// remains valid for the life of the engine object, and can be passed to \ref
    /// bind_key_a().
    ///
    /// When a handler is bound to a key (\ref bind_key_a()), the handler function may get
    /// executed by the render engine. When it does, it will always be executed by the
    /// thread that executes \ref run(). This also means, that no handler can execute on
    /// behalf of the render engine while \ref run is not executing.
    ///
    /// With the exception of \ref set_scene() and \ref run(), all functions of the render
    /// engine may be called from a handler function while it is executed by the render
    /// engine. I.e., render engine reentrancy is sanctioned in precisely this way.
    ///
    /// \sa \ref bind_key()
    /// \sa \ref bind_key_a()
    ///
    template<core::func_type<void(bool)> F> auto register_key_handler(std::string_view label, F func) ->
        render::KeyHandlerIdent;

    /// \{
    ///
    /// \brief Bind previously registered key handler to key.
    ///
    /// This function binds the specified handler (\p handler) to the specified key (\p
    /// key). A particular handler can be bound to multiple keys at the same time.
    ///
    /// When the modifier mode (\p modifier) and press multiplicity (\p multiplicity) are
    /// not specified, they default to \ref render::modif_none and \ref render::single_tap
    /// respectively.
    ///
    /// If another handler is currently bound to the specified key with the same modifier
    /// mode and press multiplicity specification, that other handler is unbound as a result
    /// if being bound to the new handler. A particular key can be bound to multiple
    /// handlers but only if the modifier mode and / or press multiplicity differs.
    ///
    /// \sa \ref bind_key()
    /// \sa \ref register_key_handler()
    /// \sa \ref get_builtin_key_handler()
    ///
    void bind_key_a(render::KeyIdent key, render::KeyHandlerIdent handler);
    void bind_key_a(render::KeyIdent key, render::KeyModifierMode modifier, render::KeyPressMultiplicity multiplicity,
                    render::KeyHandlerIdent handler);
    /// \}

    /// \brief Get one of the built-in key handlers.
    ///
    /// This function returns the key handler identifier for one of the built-in key
    /// handlers of the render engine.
    ///
    /// \sa \ref bind_key_a()
    ///
    auto get_builtin_key_handler(BuiltinKeyHandler) const noexcept -> render::KeyHandlerIdent;

    /// \brief Set new window title.
    ///
    /// This function sets a new title for the window opened by this render engine.
    ///
    void set_window_title(std::string_view title);

    /// \brief Change size of window.
    ///
    /// This function generates a request to change the size of the window of the render
    /// engine to the specified size. Such a request may or may not be honored by the
    /// platform. For more on this, see \ref display::Window::set_size().
    ///
    void set_window_size(display::Size size);

    /// \brief Switch to or from fullscreen mode.
    ///
    /// This function switches to or from fullscreen mode for the window of the render
    /// engine. For more on this, see \ref display::Window::set_fullscreen_mode().
    ///
    void set_fullscreen_mode(bool on);

    /// \brief Set orientation of virtual trackball.
    ///
    /// This function changes the current orientation of the virtual trackball. The
    /// orientation is specified as a rotation applied to the default orientation of the
    /// scene. The default orientation of the scene is the default orientation of OpenGL,
    /// which is to have the X-axis point to the right, the Y-axis point upwards, and the
    /// Z-axis point towards the viewer.
    ///
    /// Setting the orientation has an arresting effect on the trackball, which means that
    /// the trackball will have no spin after the new orientation is set (\ref set_spin()).
    ///
    /// \sa \ref set_base_orientation()
    ///
    void set_orientation(const math::Rotation& orientation);

    /// \brief Set spin of virtual trackball.
    ///
    /// This function changes the current spin of the virtual trackball. The spin is
    /// specified as a rotation where the angle is understood as angular velocity, i.e.,
    /// radians per second.
    ///
    /// Mechanically, the effect of calling this function is to first stop the trackball if
    /// it is already spinning, and then apply the specified spin to it.
    ///
    /// \sa \ref set_base_spin()
    ///
    void set_spin(const math::Rotation& spin);

    /// \brief Set current zoom factor.
    ///
    /// This function changes the current zoom factor. For an explanation of the function of
    /// the zoom factor, see \ref util::PerspectiveProjection::zoom_factor.
    ///
    /// \sa \ref set_base_zoom_factor()
    /// \sa \ref util::PerspectiveProjection::zoom_factor
    ///
    void set_zoom_factor(double factor);

    /// \brief Set current interest size.
    ///
    /// This function changes the diameter of the sphere of interest. For an explanation of
    /// the function of the sphere of interest, see \ref
    /// util::PerspectiveProjection::auto_dist().
    ///
    /// \sa \ref set_base_interest_size()
    /// \sa \ref util::PerspectiveProjection::auto_dist()
    ///
    void set_interest_size(double diameter);

    /// \brief Reset view of scene.
    ///
    /// This function resets the view of the scene. More specifically, it sets the
    /// orientation, spin, zoom factor, and interest size to the base orientation (\ref
    /// set_base_orientation()), base spin (\ref set_base_spin()), base zoom factor (\ref
    /// set_base_zoom_factor()), and base interest size (\ref set_base_interest_size())
    /// respectively.
    ///
    void reset_view();

    /// \brief Turn headlight on or off.
    ///
    /// If the headlight feature is not disabled (\ref Config::disable_headlight_feature),
    /// this function turns the headlight on or off. If the headlight feature is disabled,
    /// this function has no effect.
    ///
    void set_headlight_mode(bool on);

    /// \brief Turn wireframe mode on or off.
    ///
    /// If the wireframe feature is not disabled (\ref Config::disable_wireframe_feature),
    /// this function turns the wireframe mode on or off. If the wireframe feature is
    /// disabled, this function has no effect.
    ///
    void set_wireframe_mode(bool on);

    ~Engine() noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    auto get_key_bindings() noexcept -> impl::KeyBindings&;
};



/// \brief Configuration parameters of render engine.
///
/// These are the parameters that are available for configuring a render engine. A
/// configuration object of this type can be passed to the render engine constructor.
///
struct Engine::Config {
    /// \brief Screen on which window must appear.
    ///
    /// If specified, that is, if the specified value is non-negative, this is the index of
    /// the screen on which the window of the rennder engine must appear. See \ref
    /// display::Window::Config::screen. When a screen is not specified, i.e., when the
    /// specified value is negative, the window will be opened on the default screen.
    ///
    /// \sa \ref display::Window::Config::screen
    ///
    int screen = -1;

    /// \brief Log through specified logger.
    ///
    /// If a logger is not specified, messages will be routed to STDOUT.
    ///
    /// If a logger is specified, it must use a locale that is compatible with the locale
    /// that was passed to the engine constructor (\ref Engine::Engine()). The important
    /// thing is that the character encodings agree (`std::codecvt` facet).
    ///
    log::Logger* logger = nullptr;

    /// \brief Make window resizable.
    ///
    /// If set to `true`, the opened window will be made resizable and interactive toggling
    /// of fullscreen mode will be enabled.
    ///
    bool allow_window_resize = false;

    /// \brief Switch to fullscreen mode immediately.
    ///
    /// If set to `true`, fullscreen mode will be switched on immediately. In any case, if
    /// \ref allow_window_resize is `true`, fullscreen mode can be switched on and off
    /// interactively.
    ///
    bool fullscreen_mode = false;

    /// \brief Whether frame control is disabled.
    ///
    /// If set to `true`, interactive frame rate control will be disabled.
    ///
    bool disable_frame_rate_control = false;

    /// \brief Whether headlight feature is disabled.
    ///
    /// If set to `true`, the headlight feature is disabled.
    ///
    /// \sa \ref headlight_mode
    ///
    bool disable_headlight_feature = false;

    /// \brief Whether wireframe feature is disabled.
    ///
    /// If set to `true`, the wireframe feature is disabled.
    ///
    /// \sa \ref wireframe_mode
    ///
    bool disable_wireframe_feature = false;

    /// \brief The initial frame rate.
    ///
    /// This is the initial frame rate of the engine. The frame rate marks the upper limit
    /// of frames per second.
    ///
    /// The default frame rate is 60.
    ///
    double frame_rate = 60;

    /// \brief Base orientation of scene.
    ///
    /// This is the initial value for the base orientation of the virtual trackball.
    ///
    /// \sa \ref Engine::set_base_orientation()
    /// \sa \ref Engine::set_orientation()
    ///
    math::Rotation orientation;

    /// \brief Base spin of scene.
    ///
    /// This is the initial value for the base spin of the virtual trackball.
    ///
    /// \sa \ref Engine::set_base_spin()
    /// \sa \ref Engine::set_spin()
    ///
    math::Rotation spin;

    /// \brief Base zoom factor.
    ///
    /// This is the initial value for the base zoom factor.
    ///
    /// \sa \ref Engine::set_base_zoom_factor()
    /// \sa \ref Engine::set_zoom_factor()
    ///
    double zoom_factor = 1;

    /// \brief Base zoom factor.
    ///
    /// This is the initial value for the interest size.
    ///
    /// \sa \ref Engine::set_base_interest_size()
    /// \sa \ref Engine::set_interest_size()
    ///
    double interest_size = 2;

    /// \brief Whether headlight should be turned on initially.
    ///
    /// If set to `true` (the default), and the headlight feature is not disabled (\ref
    /// disable_headlight_feature) the headlight will be turned on initially.
    ///
    bool headlight_mode = true;

    /// \brief Whether wireframe mode should be turned on initially.
    ///
    /// If set to `true`, and the wireframe feature is not disabled (\ref
    /// disable_wireframe_feature) the wireframe mode will be turned on initially.
    ///
    bool wireframe_mode = false;
};



/// \brief Base class for render engine scenes.
///
/// This is the base class for application specified scenes to be rendered by a render
/// engine. An application can choose to override any or all of the virtual member functions
/// of this class. See the documentation of \ref Engine for an outline of how to implement a
/// concrete scene.
///
/// \sa \ref Engine
///
class Engine::Scene {
public:
    /// \brief Initialize OpenGL context.
    ///
    /// This function is called once before the first invocation of \ref render() with the
    /// calling thread bound to the same OpenGL rendering context as will be bound when \ref render() is
    /// called. The scene implementation can set up OpenGL rendering parameters here.
    ///
    virtual void init();

    /// \brief Render the scene.
    ///
    /// This function is called by the engine (\ref render::Engine) whenever the scene needs
    /// to be redrawn. This function must render the scene in its current state using
    /// OpenGL. Multiple sequential calls with no in-between calls of \ref tick() must
    /// produce the same result, i.e., the state of the scene must be unchanged.
    ///
    /// This function may be called many times per tick to fully redraw the scene. It may
    /// also be called less than once per tick, depending on such things as what \ref tick()
    /// returns.
    ///
    virtual void render();

    /// \brief Opportunity to update the state of the scene.
    ///
    /// This function is called by the engine (\ref render::Engine) once per frame tick
    /// (barring lag) according to the currently selected frame rate. The scene
    /// implementation can use this opportunity to update the state of the scene.
    ///
    /// If this function returns `true`, the scene will be re-rendered. This function should
    /// return `true` when it makes changes that affect the rendering of the scene.
    ///
    virtual bool tick(Clock::time_point time_of_tick);
};



/// \brief Identifiers for built-in key handler functions.
///
/// This is an enumeration of the available built-in key handlers. Use \ref
/// get_builtin_key_handler() to retrieve the corresponding handler.
///
enum class Engine::BuiltinKeyHandler {
    shift_modifier,    ///< Shift modifier mode
    control_modifier,  ///< Control modifier mode
    alt_modifier,      ///< Alt modifier mode
    meta_modifier,     ///< Meta modifier mode
    quit,              ///< Quit engine
    inc_frame_rate,    ///< Increase frame rate
    dec_frame_rate,    ///< Decrease frame rate
    toggle_fullscreen, ///< Toggle fullscreen mode
    reset_view,        ///< Reset view
    toggle_headlight,  ///< Toggle headlight
    toggle_wireframe,  ///< Toggle wireframe mode
};








// Implementation


inline Engine::Engine(display::Connection& conn, std::string_view window_title, display::Size window_size,
                      const std::locale& locale)
    : Engine(conn, window_title, window_size, locale, Config()) // Throws
{
}


template<core::func_type<void(bool)> F>
inline void Engine::bind_key(render::KeyIdent key, std::string_view label, F func)
{
    bind_key(key, render::modif_none, render::single_tap, label, std::move(func)); // Throws
}


template<core::func_type<void(bool)> F>
inline void Engine::bind_key(render::KeyIdent key, render::KeyModifierMode modifier,
                             render::KeyPressMultiplicity multiplicity, std::string_view label, F func)
{
    render::KeyHandlerIdent handler = register_key_handler(label, std::move(func)); // Throws
    bind_key_a(key, modifier, multiplicity, handler); // Throws
}


template<core::func_type<void(bool)> F>
auto Engine::register_key_handler(std::string_view label, F func) -> render::KeyHandlerIdent
{
    impl::KeyBindings& bindings = get_key_bindings();
    return bindings.register_handler(label, [func = std::move(func)](bool down) {
        func(down); // Throws
        return true;
    }); // Throws
}


inline void Engine::bind_key_a(render::KeyIdent key, render::KeyHandlerIdent handler)
{
    bind_key_a(key, render::modif_none, render::single_tap, handler); // Throws
}


} // namespace archon::render

#endif // ARCHON_X_RENDER_X_ENGINE_HPP
