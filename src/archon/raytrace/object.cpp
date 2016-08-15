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

#include <archon/core/functions.hpp>
#include <archon/math/intersect.hpp>
#include <archon/raytrace/object.hpp>

using namespace std;
using namespace archon::Core;
using namespace archon::Math;
using namespace archon::Raytrace;


namespace {

class StdSurface: public Surface {
public:
    StdSurface(SharedPtr<Material> m):
        m_material(std::move(m))
    {
    }

    Material* get_material() const override
    {
        return m_material.get();
    }

private:
    const SharedPtr<Material> m_material;
};



template<int which> class StdFace: public StdSurface {
public:
    StdFace(SharedPtr<Material> m):
        StdSurface(std::move(m))
    {
    }

    void map(const Vec3& point, Vec3& normal, Vec2* tex_point) const override
    {
        double edge_len = 2;
        switch (which) {
            case 1: // left
                normal.set(-1, 0, 0);
                if (tex_point)
                    *tex_point = Vec2(1+point[2], 1+point[1]) / edge_len;
                break;
            case 2: // right
                normal.set(1, 0, 0);
                if (tex_point)
                    *tex_point = Vec2(1-point[2], 1+point[1]) / edge_len;
                break;
            case 3: // bottom
                normal.set(0, -1, 0);
                if (tex_point)
                    *tex_point = Vec2(1+point[0], 1+point[2]) / edge_len;
                break;
            case 4: // top
                normal.set(0, 1, 0);
                if (tex_point)
                    *tex_point = Vec2(1+point[0], 1-point[2]) / edge_len;
                break;
            case 5: // back
                normal.set(0, 0, -1);
                if (tex_point)
                    *tex_point = Vec2(1-point[0], 1+point[1]) / edge_len;
                break;
            case 6: // front
                normal.set(0, 0, 1);
                if (tex_point)
                    *tex_point = Vec2(1+point[0], 1+point[1]) / edge_len;
                break;
        }
    }
};


class Box: public Object {
public:
    Box(SharedPtr<Material> m):
        left(m),
        right(m),
        bottom(m),
        top(m),
        back(m),
        front(m)
    {
    }

    bool intersect(const Line3& ray, double& dist,
                   const Object* origin_obj, const Surface** surface) const override
    {
        if (origin_obj == this)
            return false; // Since we are convex
        int i = intersect_box<false>(ray, dist);
        if (i == 0)
            return false;
        if (surface) {
            switch (i) {
                case 1:
                    *surface = &left;
                    break;
                case 2:
                    *surface = &right;
                    break;
                case 3:
                    *surface = &bottom;
                    break;
                case 4:
                    *surface = &top;
                    break;
                case 5:
                    *surface = &back;
                    break;
                case 6:
                    *surface = &front;
                    break;
            }
        }
        return true;
    }

/*
    void map(const Vec3& point, Vec3& normal, Vec2* tex_point) const
    {
        double height = 2;
        double bottom_radius = 1;
        double h = bottom_radius / height;
        double f = bottom_radius * (0.5 - point[1]/height);
        if (0<f) {
            normal.set(point[0]/f, h, point[2]/f);
        }
        else {
            normal.set(0, h, 1);
        }
        if (tex_point)
            tex_point->set(pol_ang(point[2], point[0]) / (M_PI*2) + 0.5, point[1]/height + 0.5);
    }
*/

    StdFace<1> left;
    StdFace<2> right;
    StdFace<3> bottom;
    StdFace<4> top;
    StdFace<5> back;
    StdFace<6> front;
};


class Cone: public Object, public StdSurface {
public:
    Cone(SharedPtr<Material> m):
        StdSurface(m),
        bottom(m)
    {
    }

    bool intersect(const Line3& ray, double& dist,
                   const Object* origin_obj, const Surface** surface) const override
    {
        if (origin_obj == this)
            return false; // Since we are convex
        int i = intersect_cone(ray, dist);
        if (i == 0)
            return false;
        if (surface)
            *surface = i == 1 ? static_cast<const Surface*>(this) : &bottom;
        return true;
    }

    void map(const Vec3& point, Vec3& normal, Vec2* tex_point) const override
    {
        double height = 2;
        double bottom_radius = 1;
        double h = bottom_radius / height;
        double f = bottom_radius * (0.5 - point[1]/height);
        if (0 < f) {
            normal.set(point[0]/f, h, point[2]/f);
        }
        else {
            normal.set(0, h, 1);
        }
        if (tex_point)
            tex_point->set(pol_ang(point[2], point[0]) / (M_PI*2) + 0.5, point[1]/height + 0.5);
    }

    StdFace<3> bottom;
};



class Cylinder: public Object, public StdSurface {
public:
    Cylinder(SharedPtr<Material> m):
        StdSurface(m),
        bottom(m),
        top(m)
    {
    }

    bool intersect(const Line3& ray, double& dist,
                   const Object* origin_obj, const Surface** surface) const override
    {
        if (origin_obj == this)
            return false; // Since we are convex
        int i = intersect_cylinder(ray, dist);
        if (i == 0)
            return false;
        if (surface) {
            *surface = i == 1 ? static_cast<const Surface*>(this) :
                i == 2 ? static_cast<const Surface*>(&bottom) : &top;
        }
        return true;
    }

    void map(const Vec3& point, Vec3& normal, Vec2* tex_point) const override
    {
        double height = 2;
        normal.set(point[0], 0, point[2]);
        if (tex_point)
            tex_point->set(pol_ang(point[2], point[0]) / (M_PI*2) + 0.5, point[1]/height + 0.5);
    }

    StdFace<3> bottom;
    StdFace<4> top;
};


class Sphere: public Object, public StdSurface {
public:
    Sphere(SharedPtr<Material> m):
        StdSurface(m)
    {
    }

    bool intersect(const Line3& ray, double& dist,
                   const Object* origin_obj, const Surface** surface) const override
    {
        if (origin_obj == this)
            return false;
        bool i = intersect_sphere<false>(ray, dist);
        if (i && surface)
            *surface = this;
        return i;
    }

    void map(const Vec3& point, Vec3& normal, Vec2* tex_point) const override
    {
        double radius = 1;

        normal = point;
        if (tex_point) {
            tex_point->set(pol_ang(point[2], point[0]) / (2*M_PI) + 0.5,
                           acos(clamp(-point[1]/radius, -1.0, 1.0)) / M_PI);
        }
    }
};


class Torus: public Object, public StdSurface {
public:
    Torus(SharedPtr<Material> m, double r):
        StdSurface(std::move(m)),
        m_major_radius(r)
    {
    }

    bool intersect(const Line3& ray, double& dist,
                   const Object* origin_obj, const Surface** surface) const override
    {
        bool i = intersect_torus(ray, dist, m_major_radius, 1, origin_obj == this);
        if (i && surface)
            *surface = this;
        return i;
    }

    void map(const Vec3& point, Vec3& normal, Vec2* tex_point) const override
    {
        // Length of projection onto Z-X-plane
        double l = pol_len(point[0], point[2]);
        double p = l - m_major_radius;
        double f = p/l;

        normal.set(f * point[0], point[1], f * point[2]);

        if (tex_point) {
            tex_point->set(pol_ang(point[2], point[0]) / (2*M_PI) + 0.5,
                           pol_ang(p, point[1]) / (2*M_PI) + 0.5);
        }
    }

private:
    const double m_major_radius;
};

} // unnamed namespace



namespace archon {
namespace Raytrace {

std::unique_ptr<Object> Object::make_box(SharedPtr<Material>mat)
{
    return std::make_unique<Box>(std::move(mat));
}

std::unique_ptr<Object> Object::make_cone(SharedPtr<Material> mat)
{
    return std::make_unique<Cone>(std::move(mat));
}

std::unique_ptr<Object> Object::make_cylinder(SharedPtr<Material> mat)
{
    return std::make_unique<Cylinder>(std::move(mat));
}

std::unique_ptr<Object> Object::make_sphere(SharedPtr<Material> mat)
{
    return std::make_unique<Sphere>(std::move(mat));
}

std::unique_ptr<Object> Object::make_torus(SharedPtr<Material> mat, double major_radius)
{
    return std::make_unique<Torus>(std::move(mat), major_radius);
}

} // namespace Raytrace
} // namespace archon
