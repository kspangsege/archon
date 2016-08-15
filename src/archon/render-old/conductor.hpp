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

#ifndef ARCHON_RENDER_CONDUCTOR_HPP
#define ARCHON_RENDER_CONDUCTOR_HPP

#include <archon/display/visual.hpp>
#include <archon/render/context.hpp>


namespace archon
{
  namespace Render
  {
    /**
     * The purpose of a Conductor is to handle paralellized rendering
     * of a scene. It does this by managing several rendering
     * threads. Each thread has its own dedicated rendering pipe.
     *
     * The number of pipelines (threads) is crusial in terms of
     * performance when rendering on systems with multiple hardware
     * rendering pipedlines. To maximize hardware utilization on such
     * systems the number of pipes (threads) should match the number of
     * physical rendering pipelines allocated to the application.
     *
     * Rendering pipelines are generally virtualized, meaning that it
     * is possible to allocate more rendering contexts than the number
     * of available hardware pipelines (just like it is possible to
     * create more processes than the number of available CPUs).
     *
     * In fact, it may be a good thing for performance to deploy two
     * or more software pipes on systems with only one hardware
     * pipeline. Imagine that you are to render a stereo view of your
     * scene and your scene rendering fuction is CPU limited due some
     * complex processing during rendering. If you render the two
     * views sequentially through one software pipe you would have
     * your rendering hardware idle some of the time. To achive higher
     * utilization of the single hardware pipeline, you should render
     * the two views in parallel through two software pipes. If your
     * system features more than one CPU, this would reduce the idle
     * time of your rendering hardware.
     *
     * It is the responsibility of the application to add channels to
     * each rendering pipe. A channel corresponds to a single
     * invocation of your scene rendering function from a certain
     * viewpoint onto a certain viewport in a certain window.
     *
     * Every rendering context owned by this conductor shares display
     * lists and textures.
     */
    struct Conductor
    {
      static core::UniquePtr<Conductor> create();

      /**
       * Add another pipeline to this conductor.
       *
       * Elsewhere pipelines are identified by their index referring
       * to the order in which they were added.
       */
      virtual void add_pipeline(display::Visual::ConstRefArg, bool direct = true) = 0;

      /**
       * Render a single frame.
       *
       * This method is not thread-safe.
       */
      virtual void render() = 0;

      //Pipe::Ref getMasterPipe() const;

      /**
       * Send termination requests to all the rendering threads.
       *
       * This must be done when you stop using the Conductor.
       *
       * \todo FIXME: This should probably be done automatically when
       * the application looses all direct and indirect (Pipe)
       * references to it.
       */
      virtual void terminate() = 0;
    };
  }
}

#endif // ARCHON_RENDER_CONDUCTOR_HPP
