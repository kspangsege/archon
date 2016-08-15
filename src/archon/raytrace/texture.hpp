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

#ifndef ARCHON_RAYTRACE_TEXTURE_HPP
#define ARCHON_RAYTRACE_TEXTURE_HPP

#include <archon/math/coord_system.hpp>
#include <archon/image/image.hpp>
#include <archon/raytrace/material.hpp>


namespace archon
{
  namespace Raytrace
  {
    struct Texture
    {
      static core::SharedPtr<Texture> get_image_texture(Imaging::Image::ConstRefArg img,
                                                        bool repeat_s = true, bool repeat_t = true);


      /**
       * Must be thread-safe.
       */
      virtual void map(math::Vec2 point, math::Vec4 &rgba) const = 0;


      virtual ~Texture() {}
    };




    struct TexturedPhongMaterial: PhongMaterialBase
    {
      TexturedPhongMaterial(core::SharedPtr<Texture> const &tex,
                            math::CoordSystem2 xform = math::CoordSystem2::identity(),
                            math::Vec3 emis_col = math::Vec3(0),
                            math::Vec3 spec_col = math::Vec3(0),
                            double ambi = 0.2, double shin = 0.2):
        PhongMaterialBase(emis_col, spec_col, ambi, shin), texture(tex), transform(xform) {}

    private:
      void get_diffuse_color(math::Vec2 tex_point, math::Vec4 &rgba) const;

      core::SharedPtr<Texture> const texture;
      math::CoordSystem2 const transform;
    };
  }
}

#endif // ARCHON_RAYTRACE_TEXTURE_HPP
