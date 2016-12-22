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

#ifndef ARCHON_RAYTRACE_MATERIAL_HPP
#define ARCHON_RAYTRACE_MATERIAL_HPP

#include <memory>
#include <vector>

#include <archon/math/vector.hpp>


namespace archon {
namespace raytrace {

/*
class TransmitProps {
public:
    math::Vec3 specular; // Modulated by color of texture
    double shininess;
};
*/



/// An abstract material with the ability to map texture coordinates to surface
/// properties and compute the color seen by an incident ray due to a specific
/// light source.
class Material {
public:
    /// Get the default material. This method is thread-safe.
    static std::shared_ptr<Material> get_default();


    struct LightInfo {
        math::Vec3 direction; // Must be a unit vector
        math::Vec3 color;
        double ambience;  // ambient.red = ambience * color.red
        double intencity; // diffuse.red = intencity * color.red, same with specular
        LightInfo(math::Vec3 d, math::Vec3 c, double a, double i):
            direction{d},
            color{c},
            ambience{a},
            intencity{i}
        {
        }
    };


    /// Must be thread-safe.
    ///
    /// \param texture_point The texture coordinates at the shaded surface
    /// point.
    ///
    /// \param normal The surface normal at the shaded surface point. It must be
    /// a vector of unit length.
    ///
    /// \param view_dir The direction from the shaded surface point towards the
    /// view point. It must be a vector of unit length.
    ///
    /// \note All vectors must be expressed relative to the same coordinate
    /// system; which one is imaterial.
    virtual void shade(math::Vec2 texture_point, math::Vec3 normal, math::Vec3 view_dir,
                       const std::vector<LightInfo>& lights, double global_ambience,
                       math::Vec4& rgba) const = 0;


    virtual ~Material() {}
};



class PhongMaterialBase: public Material {
protected:
    PhongMaterialBase(math::Vec3 emis_col, math::Vec3 spec_col, double ambi, double shin):
        m_emissive_color{emis_col},
        m_specular_color{spec_col},
        m_ambience{ambi},
        m_shininess{shin}
    {
    }

    void shade(math::Vec2 texture_point, math::Vec3 normal, math::Vec3 view_dir,
               const std::vector<LightInfo>& lights, double global_ambience,
               math::Vec4& rgba) const;

    /// Must be thread-safe.
    virtual void get_diffuse_color(math::Vec2 texture_point, math::Vec4& rgba) const = 0;

private:
    const math::Vec3 m_emissive_color;
    const math::Vec3 m_specular_color;
    const double m_ambience; // Ambient modifier, ambient_red = ambience * diffuse.red
    const double m_shininess; // 1 corresponds to a specular exponent of 128 in the Phong reflection model.
};




class PhongMaterial: public PhongMaterialBase {
public:
    PhongMaterial(math::Vec3 emis_col = math::Vec3{0.0},
                  math::Vec3 diff_col = math::Vec3{0.8},
                  math::Vec3 spec_col = math::Vec3{0.0},
                  double ambi = 0.2, double shin = 0.2, double tran = 0):
        PhongMaterialBase{emis_col, spec_col, ambi, shin},
        m_diffuse_color{diff_col[0], diff_col[1], diff_col[2], 1-tran}
    {
    }

private:
    void get_diffuse_color(math::Vec2, math::Vec4& rgba) const
    {
        rgba = m_diffuse_color;
    }

    const math::Vec4 m_diffuse_color;
};

} // namespace raytrace
} // namespace archon

#endif // ARCHON_RAYTRACE_MATERIAL_HPP
