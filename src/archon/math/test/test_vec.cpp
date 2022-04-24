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


#include <complex>
#include <vector>

#include <archon/core/span.hpp>
#include <archon/check.hpp>
#include <archon/math/vec_adapt.hpp>
#include <archon/math/vec.hpp>


using namespace archon;


ARCHON_TEST(Math_Vec_Compare)
{
    math::Vec2 vec_1 = { 1, 3 };
    math::Vec2 vec_2 = { 1, 3 };
    math::Vec2 vec_3 = { 1, 2 };
    math::Vec2 vec_4 = { 2, 1 };

    ARCHON_CHECK(vec_1 == vec_2);
    ARCHON_CHECK_NOT(vec_1 == vec_3);
    ARCHON_CHECK_NOT(vec_1 == vec_4);

    ARCHON_CHECK_NOT(vec_1 != vec_2);
    ARCHON_CHECK(vec_1 != vec_3);
    ARCHON_CHECK(vec_1 != vec_4);

    ARCHON_CHECK_NOT(vec_1 < vec_2);
    ARCHON_CHECK_NOT(vec_1 < vec_3);
    ARCHON_CHECK(vec_1 < vec_4);

    ARCHON_CHECK(vec_1 <= vec_2);
    ARCHON_CHECK_NOT(vec_1 <= vec_3);
    ARCHON_CHECK(vec_1 <= vec_4);

    ARCHON_CHECK_NOT(vec_1 > vec_2);
    ARCHON_CHECK(vec_1 > vec_3);
    ARCHON_CHECK_NOT(vec_1 > vec_4);

    ARCHON_CHECK(vec_1 >= vec_2);
    ARCHON_CHECK(vec_1 >= vec_3);
    ARCHON_CHECK_NOT(vec_1 >= vec_4);
}


ARCHON_TEST(Math_Vec_ConstructFromComponents)
{
    // This form is only supported for 2, 3, and 4-vectors, and works by way of template
    // specialization for those sizes.
    math::Vec2 v_1 = { 1, 2 };
    math::Vec3 v_2 = { 1, 2, 3 };
    math::Vec4 v_3 = { 1, 2, 3, 4 };
    ARCHON_CHECK_EQUAL(v_1, math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(v_2, math::Vec3(1, 2, 3));
    ARCHON_CHECK_EQUAL(v_3, math::Vec4(1, 2, 3, 4));
}


ARCHON_TEST(Math_Vec_ConstructFromArray)
{
    math::Vec v_1 = std::array { 1.0 };
    math::Vec v_2 = std::array { 1.0, 2.0 };
    math::Vec v_3 = std::array { 1.0, 2.0, 3.0 };
    math::Vec v_4 = std::array { 1.0, 2.0, 3.0, 4.0 };
    math::Vec v_5 = std::array { 1.0, 2.0, 3.0, 4.0, 5.0 };
    ARCHON_CHECK_EQUAL(v_1, math::Vec(std::array { 1.0 }));
    ARCHON_CHECK_EQUAL(v_2, math::Vec(std::array { 1.0, 2.0 }));
    ARCHON_CHECK_EQUAL(v_3, math::Vec(std::array { 1.0, 2.0, 3.0 }));
    ARCHON_CHECK_EQUAL(v_4, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0 }));
    ARCHON_CHECK_EQUAL(v_5, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vec_AssignFromArray1)
{
    math::Vec<1> v_1;
    double a_1[] = { 1 };
    v_1 = a_1;
    ARCHON_CHECK_EQUAL(v_1, math::Vec(std::array { 1.0 }));
    math::Vec<2> v_2;
    double a_2[] = { 1, 2 };
    v_2 = a_2;
    ARCHON_CHECK_EQUAL(v_2, math::Vec(std::array { 1.0, 2.0 }));
    math::Vec<3> v_3;
    double a_3[] = { 1, 2, 3 };
    v_3 = a_3;
    ARCHON_CHECK_EQUAL(v_3, math::Vec(std::array { 1.0, 2.0, 3.0 }));
    math::Vec<4> v_4;
    double a_4[] = { 1, 2, 3, 4 };
    v_4 = a_4;
    ARCHON_CHECK_EQUAL(v_4, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vec<5> v_5;
    double a_5[] = { 1, 2, 3, 4, 5 };
    v_5 = a_5;
    ARCHON_CHECK_EQUAL(v_5, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vec_AssignFromArray2)
{
    math::Vec<1> v_1;
    v_1 = std::array { 1.0 };
    ARCHON_CHECK_EQUAL(v_1, math::Vec(std::array { 1.0 }));
    math::Vec<2> v_2;
    v_2 = std::array { 1.0, 2.0 };
    ARCHON_CHECK_EQUAL(v_2, math::Vec(std::array { 1.0, 2.0 }));
    math::Vec<3> v_3;
    v_3 = std::array { 1.0, 2.0, 3.0 };
    ARCHON_CHECK_EQUAL(v_3, math::Vec(std::array { 1.0, 2.0, 3.0 }));
    math::Vec<4> v_4;
    v_4 = std::array { 1.0, 2.0, 3.0, 4.0 };
    ARCHON_CHECK_EQUAL(v_4, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vec<5> v_5;
    v_5 = std::array { 1.0, 2.0, 3.0, 4.0, 5.0 };
    ARCHON_CHECK_EQUAL(v_5, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vec_AssignFromVecVal)
{
    math::Vec<1> v_1;
    v_1 = math::vec_adapt(std::array { 1.0 });
    ARCHON_CHECK_EQUAL(v_1, math::Vec(std::array { 1.0 }));
    math::Vec<2> v_2;
    v_2 = math::vec_adapt(std::array { 1.0, 2.0 });
    ARCHON_CHECK_EQUAL(v_2, math::Vec(std::array { 1.0, 2.0 }));
    math::Vec<3> v_3;
    v_3 = math::vec_adapt(std::array { 1.0, 2.0, 3.0 });
    ARCHON_CHECK_EQUAL(v_3, math::Vec(std::array { 1.0, 2.0, 3.0 }));
    math::Vec<4> v_4;
    v_4 = math::vec_adapt(std::array { 1.0, 2.0, 3.0, 4.0 });
    ARCHON_CHECK_EQUAL(v_4, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vec<5> v_5;
    v_5 = math::vec_adapt(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 });
    ARCHON_CHECK_EQUAL(v_5, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vec_CopyAssign)
{
    math::Vec<1> v_1;
    v_1 = math::Vec(std::array { 1.0 });
    ARCHON_CHECK_EQUAL(v_1, math::Vec(std::array { 1.0 }));
    math::Vec<2> v_2;
    v_2 = math::Vec(std::array { 1.0, 2.0 });
    ARCHON_CHECK_EQUAL(v_2, math::Vec(std::array { 1.0, 2.0 }));
    math::Vec<3> v_3;
    v_3 = math::Vec(std::array { 1.0, 2.0, 3.0 });
    ARCHON_CHECK_EQUAL(v_3, math::Vec(std::array { 1.0, 2.0, 3.0 }));
    math::Vec<4> v_4;
    v_4 = math::Vec(std::array { 1.0, 2.0, 3.0, 4.0 });
    ARCHON_CHECK_EQUAL(v_4, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0 }));
    math::Vec<5> v_5;
    v_5 = math::Vec(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 });
    ARCHON_CHECK_EQUAL(v_5, math::Vec(std::array { 1.0, 2.0, 3.0, 4.0, 5.0 }));
}


ARCHON_TEST(Math_Vec_ComponentsAsSpan)
{
    math::Vec3 vec = { 1, 2, 3 };
    ARCHON_CHECK_EQUAL_SEQ(vec.components(), (std::array { 1, 2, 3 }));
}


ARCHON_TEST(Math_Vec_Complex)
{
    using namespace std::complex_literals;
    math::Vec<2, std::complex<double>> vec = { 1, 2 };
    ARCHON_CHECK_EQUAL(vec, math::Vec2(1, 2));
    vec *= 1i;
    ARCHON_CHECK_EQUAL(vec, (math::Vec<2, std::complex<double>>(1i, 2i)));
}
