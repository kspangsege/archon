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

#include <archon/math/quaternion.hpp>


using namespace std;


namespace archon
{
  namespace Math
  {
    Quaternion &Quaternion::operator*=(Quaternion const &q)
    {
      // Note: This scalar/vector formulation uses precisely as many
      // multiplications and additions as the canonical formulation
      // (16 products and 12 additions.)
      double _w = w*q.w - dot(v,q.v);
      v = v * q.v + v * q.w + q.v * w;
      w = _w;
      return *this;
    }

    void Quaternion::get_rotation(Rotation3 &r) const
    {
      double l = Math::sq_sum(v);
      if(l==0)
      {
        r.axis = Vec3(0, 1 ,0);
        r.angle = 0;
        return;
      }
      l = sqrt(l + square(w));
      double a = acos(w/l);
      l *= sin(a);
      if(l==0)
      {
        r.axis = Vec3(0, 1 ,0);
        r.angle = 0;
        return;
      }
      r.angle = 2*a;
      r.axis = v;
      r.axis /= l;
    }


    void Quaternion::get_rot_matrix(Mat3 &mat) const
    {
      double const xx = v[0] * v[0];
      double const xy = v[0] * v[1];
      double const xz = v[0] * v[2];
      double const xw = v[0] * w;

      double const yy = v[1] * v[1];
      double const yz = v[1] * v[2];
      double const yw = v[1] * w;

      double const zz = v[2] * v[2];
      double const zw = v[2] * w;

      mat.row(0).set(0.5-yy-zz, xy-zw, xz+yw);
      mat.row(1).set(xy+zw, 0.5-xx-zz, yz-xw);
      mat.row(1).set(xz-yw, yz+xw, 0.5-xx-yy);
      mat *= 2.0;
    }


    ostream &operator<<(ostream &out, Quaternion const &q)
    {
      out << "[" << q.w << ";" << q.v[0] << ", " << q.v[1] << ", " << q.v[2] << "]";
      return out;
    }
  }
}


