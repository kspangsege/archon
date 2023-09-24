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


#include <archon/math/quaternion.hpp>


using namespace archon;
using Quaternion = math::Quaternion;


auto Quaternion::from_proper_euler_angles(comp_type alpha, comp_type beta, comp_type gamma) noexcept -> Quaternion
{
    comp_type ca = std::cos(0.5 * alpha), sa = std::sin(0.5 * alpha);
    comp_type cb = std::cos(0.5 * beta),  sb = std::sin(0.5 * beta);
    comp_type cg = std::cos(0.5 * gamma), sg = std::sin(0.5 * gamma);

    comp_type cc = ca * cg;
    comp_type ss = sa * sg;
    comp_type sc = sa * cg;
    comp_type cs = ca * sg;

    comp_type w = (cc - ss) * cb;
    comp_type x = (cc + ss) * sb;
    comp_type y = (sc - cs) * sb;
    comp_type z = (sc + cs) * cb;

    return { w, x, y, z };
}
