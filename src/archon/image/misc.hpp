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

#ifndef ARCHON_IMAGE_MISC_HPP
#define ARCHON_IMAGE_MISC_HPP

#include <archon/core/enum.hpp>


namespace archon
{
  namespace image
  {
    /**
     * One of three ways to handle access beyond the clipping region
     * of an image reader or writer.
     *
     * If the clipping region is empty, read operations always
     * return the background color.
     *
     * \sa ReaderOps::setFalloff
     */
    enum Falloff
    {
      falloff_Background, ///< Use background color
      falloff_Edge,       ///< Use nearest edge pixel
      falloff_Repeat      ///< Repeat image indefinitely
    };

    namespace _Impl
    {
      struct FalloffSpec { static core::EnumAssoc map[]; };
    }

    typedef core::Enum<Falloff, _Impl::FalloffSpec> FalloffEnum;
  }
}

#endif // ARCHON_IMAGE_MISC_HPP
