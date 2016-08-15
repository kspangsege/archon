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

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_RENDER_VIEW_HPP
#define ARCHON_RENDER_VIEW_HPP

#include <archon/core/refcnt.hpp>
#include <archon/math/vector.hpp>


namespace archon
{
  namespace render
  {
    struct Viewport: core::CntRefObjectBase, core::CntRefDefs<Viewport>
    {
      /**
       * Change the properties of the viewport.
       *
       * \param left, bottom Defines the lower left corner of the
       * viewport relative to the associated window. 0 corresponds
       * with the left edge or the bottom of the window. 1 corresponds
       * with the right edge or the top of the window.
       *
       * \param width, height Defines the width and height of the
       * viewport relative to the associated window. 1 designates a
       * width or height equal to that of the associated window.
       *
       * This method is thread-safe.
       */
      virtual void set(double left, double bottom,
                       double width, double height) = 0;
    };



    struct Screen: core::CntRefObjectBase, core::CntRefDefs<Screen>
    {
      /**
       * Change the geometric properties of the screen.
       *
       * \param center Defines the center of the screen measured in
       * the view coordinate system.
       *
       * \param x, y Defines the directions of the x and y axes of the
       * screen measured in the view coordinate system. They must be
       * mutually perpendicular and of unit length.
       *
       * \param halfWidth, halfHeight Defines the width and height of
       * the screen measured in the view coordinate system. Unless you
       * accept circles appearing elliptical, make sure that the
       * aspect ratio (halfWidth/halfHeight) matches the real aspect
       * ratio of the physical screen/viewport/window.
       *
       * This method thread-safe.
       */
      virtual void set(math::Vec3 const &center,
                       math::Vec3 const &x, math::Vec3 const &y,
                       double halfWidth, double halfHeight) = 0;

      /**
       * A less general alternative to set(Vec3, Vec3, Vec3,
       * double, double).
       *
       * In this case the screen is assumed to be parallel to the
       * x-y-plane, have sides that are parallel to the x and y axes,
       * intersecting the z-axis for a negative value of z and with
       * the center on the z-axis.
       *
       * \param aspectRatio Is width devided by height.
       *
       * \param fieldOfView The angle between the top and bottom of
       * the screen when seen from the origin of the view coordinate
       * system or the angle between its sides if that angle is
       * smaller.
       *
       * \param distance The positive distance from the origin of the
       * view coordinate system to the screen measured in the view
       * coordinate system.
       *
       * \note The distance is arbitrary if the eye is located at the
       * origin of the view coordinate system (non-headtracked
       * monoscopic viewing) and should be left at 1 in this case.
       *
       * This method is thread-safe.
       */
      virtual void set(double aspectRatio,
                       double fieldOfView = M_PI/4,
                       double distance = 1) = 0;
    };



    struct Eye: core::CntRefObjectBase, core::CntRefDefs<Eye>
    {
      /**
       * Change the properties of the eye.
       *
       * \param position Position of the eye measured in the view
       * coordinate system.
       *
       * This method is thread-safe.
       */
      virtual void set(math::Vec3 position) = 0;
    };



    struct Clip: core::CntRefObjectBase, core::CntRefDefs<Clip>
    {
      /**
       * Change the clipping planes.
       *
       * \param near, far Specifies the positive distances to the near
       * and far clipping planes measured in the view coordinate
       * system.
       *
       * This method is thread-safe.
       */
      virtual void set(double near, double far) = 0;
    };



    /**
     * To prevent cyclic references (and thus memery leak) you must
     * prevent any object derived from this class from containing
     * references (directly or indirectly) to any of the other objects
     * defined in the Render namespace. This includes: Conductor,
     * Pipe, Viewport, Eye, Screen, Clip and View.
     */
    struct Renderer: core::CntRefObjectBase, core::CntRefDefs<Renderer>
    {
      /**
       * Called to initialize each OpenGL rending context. This is
       * done before any calls are made to the render method of this
       * class for the same OpenGL rending context.
       *
       * Exactly one call is made per OpenGL rending context.
       *
       * This method must be thread-safe.
       */
      virtual void initOpenGlContext() = 0;

      /**
       * Called to render your scene by each rendering context once
       * each time you call Conductor::render.
       *
       * This method must be thread-safe.
       */
      virtual void render() = 0;
    };



    struct View: core::CntRefObjectBase, core::CntRefDefs<View>
    {
      static Ref newView(Renderer::RefArg r);

      /**
       * Get a default viewport which corresponds to the entire
       * screen.
       */
      virtual Viewport::Ref newViewport() const = 0;

      /**
       * Get a default screen. A default screen corresponds with a
       * screen for which set(1, M_PI/4, 1) is called.
       *
       * \see set(double, double, double)
       */
      virtual Screen::Ref newScreen() const = 0;

      /**
       * Get a default eye. A default eye is located at the origin of
       * the view coordinate system.
       */
      virtual Eye::Ref newEye() const = 0;

      /**
       * Get the default clipping planes. The default clipping plane
       * distances are 0.2 and 200 respectively.
       */
      virtual Clip::Ref newClip() const = 0;
    };
  }
}

#endif // ARCHON_RENDER_VIEW_HPP
