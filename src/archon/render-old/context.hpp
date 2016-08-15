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

#ifndef ARCHON_RENDER_CONTEXT_HPP
#define ARCHON_RENDER_CONTEXT_HPP

#include <archon/display/window.hpp>
#include <archon/render/view.hpp>


namespace archon
{
  namespace render
  {
    /**
     * This class represents a single virtualized rendering pipeline,
     * thus it corresponds closely to a \c display::Context. All work
     * done by a pipline is serialized. To get multiple rendering
     * threads ro run concurrently, you must create multiple
     * pipelines. All the piplines are managed by a \c Conductor
     * object.
     *
     * The pipeline is equipped with a list of rendering channels. For
     * each frame the pipeline object will render each of its channels
     * once and in succession.
     *
     * New pipelines are created through \c Conductor::add_pipeline().
     *
     * \sa Conductor
     */
    struct Pipeline
    {
      /**
       * Add a channel to this pipeline. A channel is a sub-task that
       * needs to be carried out at each frame of the overall
       * rendering process.
       */
      virtual void add_channel(display::Window *,
                               Viewport::ConstRefArg, Screen::ConstRefArg,
                               Eye::ConstRefArg, Clip::ConstRefArg) = 0;
    };
  }
}

#endif // ARCHON_RENDER_CONTEXT_HPP
