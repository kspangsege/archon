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

#include <archon/log/logger.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>
#include <archon/display/geometry.hpp>
#include <archon/render/key_binding_support.hpp>
#include <archon/render/engine.hpp>
#include <archon/render/noinst/engine_impl.hpp>


using namespace archon;
namespace impl = render::impl;
using render::Engine;


class Engine::Impl
    : public impl::EngineImpl {
public:
    using impl::EngineImpl::EngineImpl;
};


Engine::Engine(std::string_view window_title, display::Size window_size, const std::locale& locale,
               const Config& config)
    : m_impl(std::make_unique<Impl>(window_title, window_size, locale, config)) // Throws
{
}


void Engine::set_scene(Scene&scene) noexcept
{
    m_impl->set_scene(scene);
}


void Engine::run()
{
    m_impl->run(); // Throws
}


void Engine::set_frame_rate(double rate)
{
    m_impl->set_frame_rate(rate); // Throws
}


void Engine::set_background_color(util::Color color)
{
    m_impl->set_background_color(color); // Throws
}


void Engine::set_base_orientation(const math::Rotation& orientation)
{
    m_impl->set_base_orientation(orientation); // Throws
}


void Engine::set_base_spin(const math::Rotation& spin)
{
    m_impl->set_base_spin(spin); // Throws
}


void Engine::set_base_zoom_factor(double factor)
{
    m_impl->set_base_zoom_factor(factor); // Throws
}


void Engine::set_base_interest_size(double size)
{
    m_impl->set_base_interest_size(size); // Throws
}


auto Engine::get_logger() noexcept -> log::Logger&
{
    return m_impl->get_logger();
}


void Engine::bind_key_a(render::KeyIdent key, render::KeyModifierMode modifier,
                        render::KeyPressMultiplicity multiplicity, render::KeyHandlerIdent handler)
{
    m_impl->bind_key(key, modifier, multiplicity, handler); // Throws
}


auto Engine::get_builtin_key_handler(BuiltinKeyHandler ident) const noexcept -> render::KeyHandlerIdent
{
    return m_impl->get_builtin_key_handler(ident);
}


void Engine::set_window_title(std::string_view title)
{
    m_impl->set_window_title(title); // Throws
}


void Engine::set_window_size(display::Size size)
{
    m_impl->set_window_size(size); // Throws
}


void Engine::set_fullscreen_mode(bool on)
{
    m_impl->set_fullscreen_mode(on); // Throws
}


void Engine::set_orientation(const math::Rotation& orientation)
{
    m_impl->set_orientation(orientation); // Throws
}


void Engine::set_spin(const math::Rotation& spin)
{
    m_impl->set_spin(spin); // Throws
}


void Engine::set_zoom_factor(double factor)
{
    m_impl->set_zoom_factor(factor); // Throws
}


void Engine::set_interest_size(double diameter)
{
    m_impl->set_interest_size(diameter); // Throws
}


void Engine::reset_view()
{
    m_impl->reset_view(); // Throws
}


void Engine::set_headlight_mode(bool on)
{
    m_impl->set_headlight_mode(on); // Throws
}


void Engine::set_wireframe_mode(bool on)
{
    m_impl->set_wireframe_mode(on); // Throws
}


Engine::~Engine() noexcept
{
}


auto Engine::get_key_bindings() noexcept -> impl::KeyBindings&
{
    return m_impl->get_key_bindings();
}


void Engine::Scene::init()
{
}


void Engine::Scene::render()
{
}


bool Engine::Scene::tick(Clock::time_point)
{
    return false;
}
