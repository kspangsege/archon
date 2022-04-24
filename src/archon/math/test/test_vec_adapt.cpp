// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <vector>

#include <archon/check.hpp>
#include <archon/math/vec_adapt.hpp>
#include <archon/math/vec.hpp>


using namespace archon;


ARCHON_TEST(Math_VecAdapt_FromArray)
{
    double components[2] = { 1, 2 };
    ARCHON_CHECK_EQUAL(math::vec_adapt(components), math::Vec2(1, 2));
    math::vec_adapt(components) = math::Vec2(3, 4);
    ARCHON_CHECK_EQUAL_SEQ(components, math::Vec2(3, 4).components());
}


ARCHON_TEST(Math_VecAdapt_FromArray2)
{
    std::array components = { 1.0, 2.0 };
    ARCHON_CHECK_EQUAL(math::vec_adapt(components), math::Vec2(1, 2));
    math::vec_adapt(components) = math::Vec2(3, 4);
    ARCHON_CHECK_EQUAL_SEQ(components, math::Vec2(3, 4).components());
}


ARCHON_TEST(Math_VecAdapt_Assign)
{
    double components_1[2] = { 1, 2 };
    double components_2[2] = { 3, 4 };
    math::vec_adapt(components_1) = math::vec_adapt(components_2);
    ARCHON_CHECK_EQUAL_SEQ(components_1, math::Vec2(3, 4).components());
    ARCHON_CHECK_EQUAL_SEQ(components_2, math::Vec2(3, 4).components());
    (math::vec_adapt(components_1) = math::vec_adapt(components_2)) = math::Vec2(5, 6);
    ARCHON_CHECK_EQUAL_SEQ(components_1, math::Vec2(5, 6).components());
    ARCHON_CHECK_EQUAL_SEQ(components_2, math::Vec2(3, 4).components());
}
