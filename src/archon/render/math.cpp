// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <archon/math/matrix.hpp>
#include <archon/render/math.hpp>


using namespace archon;


auto render::make_perspective(double left, double right, double bottom, double top,
                              double near_, double far_) noexcept -> math::Matrix4F
{
    double width  = right - left;
    double height = top - bottom;
    double depth  = far_ - near_;
    math::Matrix4F mat;
    mat[0][0] = 2 * near_         / width;
    mat[0][2] = (right + left)    / width;
    mat[1][1] = 2 * near_         / height;
    mat[1][2] = (top + bottom)    / height;
    mat[2][2] = -(far_ + near_)   / depth;
    mat[2][3] = -2 * far_ * near_ / depth;
    mat[3][2] = -1;
    return mat;
}
