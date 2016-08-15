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

#include <cmath>
#include <utility>
#include <vector>
#include <map>

#include <archon/math/matrix_adapt.hpp>
#include <archon/raytrace/texture.hpp>
#include <archon/raytrace/scene_build.hpp>

using namespace archon::core;
using namespace archon::math;
using namespace archon::Graphics;
using namespace archon::Imaging;
using namespace archon::Raytrace;


namespace {

class SceneBuilderImpl: public archon::Raytrace::SceneBuilder {
public:
    SceneBuilderImpl(Raytracer& raytracer, SpatialSceneBuilder* aux):
        m_raytracer(raytracer),
        m_aux_builder(aux),
        m_tex_transform(CoordSystem2::identity())
    {
        m_transform.coord_system = CoordSystem3::identity();
        m_transform.index = -1;
    }

    void translate(Vec3 v) override
    {
        if (v.is_zero())
            return;
        if (m_aux_builder)
            m_aux_builder->translate(v);
        m_transform.coord_system.translate(v);
        transform_modified();
    }

    void scale(Vec3 s) override
    {
        if (s == Vec3(1))
            return;
        if (m_aux_builder)
            m_aux_builder->scale(s);
        m_transform.coord_system.basis.scale(s);
        transform_modified();
    }

    void rotate(Rotation3 r) override
    {
        if (r.angle == 0)
            return;
        if (m_aux_builder)
            m_aux_builder->rotate(r);
        Mat3 rot;
        r.get_matrix(rot);
        m_transform.coord_system.basis *= rot;
        transform_modified();
    }

    void push() override
    {
        if (m_aux_builder)
            m_aux_builder->push_matrix();
        m_transform_stack.push_back(m_transform);
    }

    void pop() override
    {
        if (m_aux_builder)
            m_aux_builder->pop_matrix();
        m_transform = m_transform_stack.back();
        m_transform_stack.pop_back();
    }

    void add_box() override
    {
        if (m_aux_builder)
            build_box(*m_aux_builder, !m_texture_path.empty());
        ensure_material();
        add(Object::make_box(m_material));
    }

    void add_cone() override
    {
        if (m_aux_builder)
            build_cone(*m_aux_builder, !m_texture_path.empty());
        ensure_material();
        add(Object::make_cone(m_material));
    }

    void add_cylinder() override
    {
        if (m_aux_builder)
            build_cylinder(*m_aux_builder, !m_texture_path.empty());
        ensure_material();
        add(Object::make_cylinder(m_material));
    }

    void add_sphere() override
    {
        if (m_aux_builder)
            build_sphere(*m_aux_builder, !m_texture_path.empty());
        ensure_material();
        add(Object::make_sphere(m_material));
    }

    void add_torus(double major_radius) override
    {
        if (m_aux_builder)
            build_torus(*m_aux_builder, !m_texture_path.empty(), major_radius);
        ensure_material();
        add(Object::make_torus(m_material, major_radius));
    }

    void add_directional_light() override
    {
        Vec3 dir = unit(m_transform.coord_system.basis.col(2));
        std::unique_ptr<Light> light;
        light.reset(new DirectionalLight(dir, m_light_color, m_light_ambience, m_light_intencity));
        m_raytracer.add_light(std::move(light));
    }

    void add_point_light() override
    {
        Vec3 pos = m_transform.coord_system.origin;
        std::unique_ptr<Light> light;
        light.reset(new PointLight(pos, m_light_color, m_light_ambience, m_light_intencity,
                                   m_light_attenuation));
        m_raytracer.add_light(std::move(light));
    }

    void add_spot_light(double cutoff, double hotspot) override
    {
        Vec3 pos = m_transform.coord_system.origin;
        Vec3 dir = unit(m_transform.coord_system.basis.col(2));
        std::unique_ptr<Light> light;
        light.reset(new SpotLight(pos, dir, cutoff, hotspot, m_light_color,
                                  m_light_ambience, m_light_intencity, m_light_attenuation));
        m_raytracer.add_light(std::move(light));
    }

    void set_material_diffuse_color(Vec3 color) override
    {
        if (color == m_mat_diffuse)
            return;
        m_mat_diffuse = color;
        if (m_texture_path.empty())
            m_material.reset();
    }

    void set_material_transparency(double tran) override
    {
        if (tran == m_mat_transparency)
            return;
        m_mat_transparency = tran;
        if (m_texture_path.empty())
            m_material.reset();
    }

    void set_texture(std::string image_path, bool rep_s, bool rep_t) override
    {
        if (image_path == m_texture_path && rep_s == m_tex_repeat_s && rep_t == m_tex_repeat_t)
            return;
        m_texture_path = image_path;
        m_tex_repeat_s = rep_s;
        m_tex_repeat_t = rep_t;
        m_material.reset();
    }

    void tex_translate(Vec2 v) override
    {
        if (v.is_zero())
            return;
        m_tex_transform.translate(v);
        if (!m_texture_path.empty())
            m_material.reset();
        if (m_aux_builder)
            m_aux_builder->tex_translate(v);
    }

    void tex_scale(Vec2 s) override
    {
        if (s == Vec2(1))
            return;
        m_tex_transform.basis.scale(s);
        if (!m_texture_path.empty())
            m_material.reset();
        if (m_aux_builder)
            m_aux_builder->tex_scale(s);
    }

    void tex_rotate(double angle) override
    {
        if (angle == 0)
            return;
        double mat[4] = { std::cos(angle), -std::sin(angle),
                          std::sin(angle),  std::cos(angle) };
        m_tex_transform.basis *= mat2x2_adapt(mat);
        if (!m_texture_path.empty())
            m_material.reset();
        if (m_aux_builder)
            m_aux_builder->tex_rotate(angle);
    }

    void reset_tex_transform() override
    {
        m_tex_transform = CoordSystem2::identity();
        if (m_aux_builder)
            m_aux_builder->reset_tex_transform();
    }

    void set_light_color(Vec3 color) override
    {
        m_light_color = color;
    }

    void set_light_ambience(double ambience) override
    {
        m_light_ambience = ambience;
    }

    void set_light_intencity(double intencity) override
    {
        m_light_intencity = intencity;
    }

    void set_light_attenuation(double cons, double lin, double quad) override
    {
        m_light_attenuation.set(cons, lin, quad);
    }

private:
    void add(std::unique_ptr<Object> obj)
    {
        if (m_transform.index < 0)
            m_transform.index = m_raytracer.make_transform(m_transform.coord_system);
        m_raytracer.add_object(std::move(obj), m_transform.index);
    }

    void transform_modified()
    {
        m_transform.index = -1;
    }

    void ensure_material()
    {
        if (m_material)
            return;

        if (m_texture_path.empty()) {
            m_material.reset(new PhongMaterial(m_mat_emissive, m_mat_diffuse, m_mat_specular,
                                               m_mat_ambience, m_mat_shininess,
                                               m_mat_transparency));
            return;
        }

        TexEntry& tex_entry = m_textures[m_texture_path];
        if (!tex_entry.texture) {
            Image::ConstRef img = Image::load(m_texture_path);
            SharedPtr<Texture> texture =
                Texture::get_image_texture(img, m_tex_repeat_s, m_tex_repeat_t);
            if (m_aux_builder)
                tex_entry.aux_id =
                    m_aux_builder->make_texture(img, m_texture_path, m_tex_repeat_s, m_tex_repeat_t);
            tex_entry.texture = texture;
        }

        m_material.reset(new TexturedPhongMaterial(tex_entry.texture, m_tex_transform, m_mat_emissive,
                                                   m_mat_specular, m_mat_ambience, m_mat_shininess));

        if (m_aux_builder)
            m_aux_builder->bind_texture(tex_entry.aux_id);
    }

    struct Transform {
        CoordSystem3 coord_system;
        int index;
    };

    Raytracer& m_raytracer;
    SpatialSceneBuilder* const m_aux_builder;

    Transform m_transform;
    std::vector<Transform> m_transform_stack;

    Vec3 m_mat_emissive{0}, m_mat_diffuse{0.8}, m_mat_specular{0};
    double m_mat_ambience = 0.2, m_mat_shininess = 0.2, m_mat_transparency = 0;

    Vec3 m_light_color{1};
    double m_light_ambience{0}, m_light_intencity{1};
    Vec3 m_light_attenuation{1,0,0};

    std::string m_texture_path; // Path of current texture image
    bool m_tex_repeat_s = true, m_tex_repeat_t = true;

    struct TexEntry {
        SharedPtr<Texture> texture;
        int aux_id;
    };

    std::map<std::string,  TexEntry> m_textures;
    CoordSystem2 m_tex_transform;

    SharedPtr<Material> m_material;
};

} // unnamed namespace


namespace archon {
namespace Raytrace {

std::unique_ptr<SceneBuilder> make_scene_builder(Raytracer& raytracer, SpatialSceneBuilder* aux_builder)
{
    return std::make_unique<SceneBuilderImpl>(raytracer, aux_builder);
}

} // namespace Raytrace
} // namespace archon
