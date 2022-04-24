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


#include <type_traits>
#include <limits>

#include <archon/check.hpp>
#include <archon/math/vec.hpp>
#include <archon/math/mat.hpp>
#include <archon/math/test/util.hpp>


using namespace archon;


ARCHON_TEST(Math_MatOps_AdditionSubtractionNegation)
{
    math::Mat2 a = {{ 1, 2 },
                    { 3, 5 }};
    math::Mat2F b = {{ 3, 5 },
                     { 7, 9 }};
    ARCHON_CHECK_EQUAL(a + b, math::Mat2({  4,  7 },
                                         { 10, 14 }));
    ARCHON_CHECK_EQUAL(a - b, math::Mat2({ -2, -3 },
                                         { -4, -4 }));
    ARCHON_CHECK_EQUAL(-a, math::Mat2({ -1, -2 },
                                      { -3, -5 }));
    a += b;
    ARCHON_CHECK_EQUAL(a, math::Mat2({  4,  7 },
                                     { 10, 14 }));
    a -= b;
    ARCHON_CHECK_EQUAL(a, math::Mat2({ 1, 2 },
                                     { 3, 5 }));
    static_assert(std::is_same_v<decltype(a + b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a - b)::comp_type, double>);
}


ARCHON_TEST(Math_MatOps_Scaling)
{
    math::Mat2F a = {{ 1, 2 },
                     { 3, 5 }};
    ARCHON_CHECK_EQUAL(a * 2, math::Mat2({ 2,  4 },
                                         { 6, 10 }));
    ARCHON_CHECK_EQUAL(3 * a, math::Mat2({ 3,  6 },
                                         { 9, 15 }));
    ARCHON_CHECK_EQUAL(a / 2, math::Mat2({ 0.5, 1.0 },
                                         { 1.5, 2.5 }));
    a *= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Mat2({ 2,  4 },
                                     { 6, 10 }));
    a /= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Mat2({ 1, 2 },
                                     { 3, 5 }));
    static_assert(std::is_same_v<decltype(2.0 * a)::comp_type, double>);
    static_assert(std::is_same_v<decltype(2.0F * a)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a * 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * 2.0F)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a / 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a / 2.0F)::comp_type, float>);
}


ARCHON_TEST(Math_MatOps_MatrixVectorMultiplication)
{
    math::Mat2F a = {{ 1, 2 },
                     { 3, 4 }};
    math::Mat3x2F b = {{ 1, 3 },
                       { 2, 5 },
                       { 4, 6 }};
    math::Vec2 c = { 3, 5 };
    ARCHON_CHECK_EQUAL(a * c, math::Vec2(13, 29));
    ARCHON_CHECK_EQUAL(b * c, math::Vec3(18, 31, 42));
    static_assert(std::is_same_v<decltype(a * c)::comp_type, double>);
    static_assert(std::is_same_v<decltype(b * c)::comp_type, double>);
}


ARCHON_TEST(Math_MatOps_VectorMatrixMultiplication)
{
    math::Vec2 a = { 3, 5 };
    math::Mat2F b = {{ 1, 2 },
                     { 3, 4 }};
    math::Mat2x3F c = {{ 1, 2, 3 },
                       { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(a * b, math::Vec2(18, 26));
    ARCHON_CHECK_EQUAL(a * c, math::Vec3(23, 31, 39));
    static_assert(std::is_same_v<decltype(a * b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * c)::comp_type, double>);
}


ARCHON_TEST(Math_MatOps_MatrixMatrixMultiplication)
{
    math::Mat2 a = {{ 2, 4 },
                    { 3, 5 }};
    math::Mat2F b = {{ 1, 2 },
                     { 3, 4 }};
    math::Mat2x3F c = {{ 1, 2, 3 },
                       { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(a * b, math::Mat2({ 14, 20 },
                                         { 18, 26 }));
    ARCHON_CHECK_EQUAL(a * c, math::Mat2x3({ 18, 24, 30 },
                                           { 23, 31, 39 }));
    static_assert(std::is_same_v<decltype(a * b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * c)::comp_type, double>);
}


ARCHON_TEST(Math_MatOps_ScalarMatrixDivision)
{
    math::Mat2 x = {{ 1, 2 },
                    { 3, 4 }};
    double eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(2 / x, math::Mat2({ -4,  2 },
                                           {  3, -1 }),
                         math::test::matrix_compare(10 * eps));
}


ARCHON_TEST(Math_MatOps_MatrixMatrixDivision)
{
    math::Mat3x2 x = {{ 5, 6 },
                      { 6, 7 },
                      { 7, 8 }};
    math::Mat2 y = {{ 1, 2 },
                    { 3, 4 }};
    double eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(x / y, math::Mat3x2({ -1.0, 2.0 },
                                             { -1.5, 2.5 },
                                             { -2.0, 3.0 }),
                         math::test::matrix_compare(10 * eps));
}


ARCHON_TEST(Math_MatOps_Transpose)
{
    math::Mat2x3 x = {{ 1, 2, 3 },
                      { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(math::transpose(x), math::Mat3x2({ 1, 4 },
                                                        { 2, 5 },
                                                        { 3, 6 }));
}


ARCHON_TEST(Math_MatOps_Trace)
{
    math::Mat2 x = {{ 1, 2 },
                    { 3, 4 }};
    ARCHON_CHECK_EQUAL(math::tr(x), 5);
    math::Mat3 y = {{ 1, 2, 3 },
                    { 4, 5, 6 },
                    { 7, 8, 9 }};
    ARCHON_CHECK_EQUAL(math::tr(y), 15);
}


ARCHON_TEST(Math_MatOps_Determinant)
{
    double eps = std::numeric_limits<double>::epsilon();
    math::Mat2 x = {{  1, 3 },
                    { -3, 2 }};
    ARCHON_CHECK_APPROXIMATELY_EQUAL(math::det(x), 11, 20 * eps);
    math::Mat3 y = {{  1, 2, -1 },
                    {  0, 3, -4 },
                    { -1, 2,  1 }};
    ARCHON_CHECK_APPROXIMATELY_EQUAL(math::det(y), 16, 100 * eps);
}


ARCHON_TEST(Math_MatOps_Inverse)
{
    math::Mat2F x = {{ 1, 2 },
                     { 3, 4 }};
    double eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(math::inv(x), math::Mat2({ -2.0,  1.0 },
                                                  {  1.5, -0.5 }),
                         math::test::matrix_compare(10 * eps));
    static_assert(std::is_same_v<decltype(inv(x))::comp_type, float>);
}


ARCHON_TEST(Math_MatOps_OuterProductOfVectors)
{
    math::Vec2 a = { 1, 2 };
    math::Vec3F b = { 3, 4, 5 };
    ARCHON_CHECK_EQUAL(math::outer(a, b), math::Mat2x3({ 3, 4,  5 },
                                                       { 6, 8, 10 }));
    static_assert(std::is_same_v<decltype(math::outer(a, b))::comp_type, double>);
}


ARCHON_TEST(Math_MatOps_GenMat)
{
    math::Mat x  = math::gen_mat<3, 4>([](std::size_t i, std::size_t j) {
        return double(i * 10 + j);
    });
    static_assert(x.num_rows == 3);
    static_assert(x.num_cols == 4);

    for (std::size_t i = 0; i < x.num_rows; ++i) {
        for (std::size_t j = 0; j < x.num_cols; ++j)
            ARCHON_CHECK_EQUAL(x[i][j], i * 10 + j);
    }
}
