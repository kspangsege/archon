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


#include <cstddef>
#include <limits>
#include <algorithm>
#include <array>
#include <complex>
#include <random>

#include <archon/core/assert.hpp>
#include <archon/core/inexact_compare.hpp>
#include <archon/check.hpp>
#include <archon/math/vec.hpp>
#include <archon/math/mat.hpp>
#include <archon/math/test/util.hpp>


using namespace archon;


ARCHON_TEST(Math_Mat_Compare)
{
    math::Mat2 mat_1 = {{ 1, 3 }, { 4, 6 }};
    math::Mat2 mat_2 = {{ 1, 3 }, { 4, 6 }};
    math::Mat2 mat_3 = {{ 1, 3 }, { 4, 5 }};
    math::Mat2 mat_4 = {{ 2, 3 }, { 4, 6 }};

    ARCHON_CHECK(mat_1 == mat_2);
    ARCHON_CHECK_NOT(mat_1 == mat_3);
    ARCHON_CHECK_NOT(mat_1 == mat_4);

    ARCHON_CHECK_NOT(mat_1 != mat_2);
    ARCHON_CHECK(mat_1 != mat_3);
    ARCHON_CHECK(mat_1 != mat_4);

    ARCHON_CHECK_NOT(mat_1 < mat_2);
    ARCHON_CHECK_NOT(mat_1 < mat_3);
    ARCHON_CHECK(mat_1 < mat_4);

    ARCHON_CHECK(mat_1 <= mat_2);
    ARCHON_CHECK_NOT(mat_1 <= mat_3);
    ARCHON_CHECK(mat_1 <= mat_4);

    ARCHON_CHECK_NOT(mat_1 > mat_2);
    ARCHON_CHECK(mat_1 > mat_3);
    ARCHON_CHECK_NOT(mat_1 > mat_4);

    ARCHON_CHECK(mat_1 >= mat_2);
    ARCHON_CHECK(mat_1 >= mat_3);
    ARCHON_CHECK_NOT(mat_1 >= mat_4);
}


ARCHON_TEST(Math_Mat_Basics)
{
    math::Mat2 mat;

    mat.row(0) = math::Vec2(1, 2);
    mat.row(1) = std::array { 3, 4 };
    ARCHON_CHECK_EQUAL(mat, math::Mat2({ 1, 2 },
                                       { 3, 4 }));

    mat.col(0) = math::Vec2(5, 6);
    mat.col(1) = std::array { 7, 8 };
    ARCHON_CHECK_EQUAL(mat, math::Mat2({ 5, 7 },
                                       { 6, 8 }));

    mat.diag() = math::Vec2(1, 2);
    ARCHON_CHECK_EQUAL(mat, math::Mat2({ 1, 7 },
                                       { 6, 2 }));
    mat.diag() = std::array { 3, 4 };
    ARCHON_CHECK_EQUAL(mat, math::Mat2({ 3, 7 },
                                       { 6, 4 }));
}


ARCHON_TEST(Math_Mat_Complex)
{
    using namespace std::complex_literals;
    math::Mat<2, 2, std::complex<double>> vec = {{ 1, 2 },
                                                 { 3, 4 }};
    ARCHON_CHECK_EQUAL(vec, math::Mat2({ 1, 2 },
                                       { 3, 4 }));
    vec *= 1i;
    ARCHON_CHECK_EQUAL(vec, (math::Mat<2, 2, std::complex<double>>({ 1i, 2i },
                                                                   { 3i, 4i })));
}


ARCHON_TEST(Math_Mat_Subscr)
{
    math::Mat2 x = {{ 1, 2 },
                    { 3, 4 }};
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x)[0], math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x)[1], math::Vec2(3, 4));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x)[0], math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x)[1], math::Vec2(3, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x)[0], math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x)[1], math::Vec2(3, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x)[0], math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x)[1], math::Vec2(3, 4));
    static_cast<math::Mat2&>(x)[0] = math::Vec2(5, 6);
    ARCHON_CHECK_EQUAL(x[0], math::Vec2(5, 6));
    // Verify that object returned by y[0] does not refer to y when y is an r-value.
    static_cast<math::Mat2&&>(x)[0] = math::Vec2(7, 8);
    ARCHON_CHECK_EQUAL(x[0], math::Vec2(5, 6));
    ARCHON_CHECK_EQUAL(x[1], math::Vec2(3, 4));
}


ARCHON_TEST(Math_Mat_Row)
{
    math::Mat2 x = {{ 1, 2 },
                    { 3, 4 }};
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x).row(0), math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x).row(1), math::Vec2(3, 4));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x).row(0), math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x).row(1), math::Vec2(3, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x).row(0), math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x).row(1), math::Vec2(3, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x).row(0), math::Vec2(1, 2));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x).row(1), math::Vec2(3, 4));
    static_cast<math::Mat2&>(x).row(0) = math::Vec2(5, 6);
    ARCHON_CHECK_EQUAL(x.row(0), math::Vec2(5, 6));
    // Verify that object returned by y.row(0) does not refer to y when y is an r-value.
    static_cast<math::Mat2&&>(x).row(0) = math::Vec2(7, 8);
    ARCHON_CHECK_EQUAL(x.row(0), math::Vec2(5, 6));
    ARCHON_CHECK_EQUAL(x.row(1), math::Vec2(3, 4));
}


ARCHON_TEST(Math_Mat_Col)
{
    math::Mat2 x = {{ 1, 2 },
                    { 3, 4 }};
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x).col(0), math::Vec2(1, 3));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x).col(1), math::Vec2(2, 4));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x).col(0), math::Vec2(1, 3));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x).col(1), math::Vec2(2, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x).col(0), math::Vec2(1, 3));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x).col(1), math::Vec2(2, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x).col(0), math::Vec2(1, 3));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x).col(1), math::Vec2(2, 4));
    static_cast<math::Mat2&>(x).col(0) = math::Vec2(5, 6);
    ARCHON_CHECK_EQUAL(x.col(0), math::Vec2(5, 6));
    // Verify that object returned by y.col(0) does not refer to y when y is an r-value.
    static_cast<math::Mat2&&>(x).col(0) = math::Vec2(7, 8);
    ARCHON_CHECK_EQUAL(x.col(0), math::Vec2(5, 6));
    ARCHON_CHECK_EQUAL(x.col(1), math::Vec2(2, 4));
}


ARCHON_TEST(Math_Mat_Diag)
{
    math::Mat2 x = {{ 1, 2 },
                    { 3, 4 }};
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&>(x).diag(), math::Vec2(1, 4));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2&&>(x).diag(), math::Vec2(1, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&>(x).diag(), math::Vec2(1, 4));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2&&>(x).diag(), math::Vec2(1, 4));
    static_cast<math::Mat2&>(x).diag() = math::Vec2(5, 6);
    ARCHON_CHECK_EQUAL(x.diag(), math::Vec2(5, 6));
    // Verify that object returned by y.diag() does not refer to y when y is an r-value.
    static_cast<math::Mat2&&>(x).diag() = math::Vec2(7, 8);
    ARCHON_CHECK_EQUAL(x, math::Mat2({ 5, 2 },
                                     { 3, 6 }));
}


ARCHON_TEST(Math_Mat_Sub)
{
    math::Mat3x4 x = {{ 1, 2, 3, 4 },
                      { 3, 4, 5, 6 },
                      { 5, 6, 7, 8 }};
    ARCHON_CHECK_EQUAL((static_cast<math::Mat3x4&>(x).sub<2, 3>()),
                       math::Mat2x3({ 1, 2, 3 },
                                    { 3, 4, 5 }));
    ARCHON_CHECK_EQUAL((static_cast<math::Mat3x4&&>(x).sub<2, 3>()),
                       math::Mat2x3({ 1, 2, 3 },
                                    { 3, 4, 5 }));
    ARCHON_CHECK_EQUAL((static_cast<const math::Mat3x4&>(x).sub<2, 3>()),
                       math::Mat2x3({ 1, 2, 3 },
                                    { 3, 4, 5 }));
    ARCHON_CHECK_EQUAL((static_cast<const math::Mat3x4&&>(x).sub<2, 3>()),
                       math::Mat2x3({ 1, 2, 3 },
                                    { 3, 4, 5 }));

    static_cast<math::Mat3x4&>(x).sub<2, 3>() = math::Mat2x3({ 2, 3, 4 },
                                                             { 5, 6, 7 });
    ARCHON_CHECK_EQUAL((x.sub<2, 3>()), math::Mat2x3({ 2, 3, 4 },
                                                     { 5, 6, 7 }));

    // Verify that object returned by y.sub<...>() does not refer to y when y is an r-value.
    static_cast<math::Mat3x4&&>(x).sub<2, 3>() = math::Mat2x3({ 3, 4, 5 },
                                                              { 6, 7, 8 });
    ARCHON_CHECK_EQUAL((x.sub<2, 3>()), math::Mat2x3({ 2, 3, 4 },
                                                     { 5, 6, 7 }));

    ARCHON_CHECK_EQUAL((x.sub<2, 4>().sub<2, 3>()), math::Mat2x3({ 2, 3, 4 },
                                                                 { 5, 6, 7 }));
}


ARCHON_TEST(Math_Mat_Transposed)
{
    math::Mat2x3 x = {{ 1, 2, 3 },
                      { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2x3&>(x).transposed(),
                       math::Mat3x2({ 1, 4 },
                                    { 2, 5 },
                                    { 3, 6 }));
    ARCHON_CHECK_EQUAL(static_cast<math::Mat2x3&&>(x).transposed(),
                       math::Mat3x2({ 1, 4 },
                                    { 2, 5 },
                                    { 3, 6 }));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2x3&>(x).transposed(),
                       math::Mat3x2({ 1, 4 },
                                    { 2, 5 },
                                    { 3, 6 }));
    ARCHON_CHECK_EQUAL(static_cast<const math::Mat2x3&&>(x).transposed(),
                       math::Mat3x2({ 1, 4 },
                                    { 2, 5 },
                                    { 3, 6 }));

    static_cast<math::Mat2x3&>(x).transposed() = math::Mat3x2({ 4, 5 },
                                                              { 6, 7 },
                                                              { 8, 9 });
    ARCHON_CHECK_EQUAL(x, math::Mat2x3({ 4, 6, 8 },
                                       { 5, 7, 9 }));

    // Verify that object returned by y.transposed() does not refer to y when y is an
    // r-value.
    static_cast<math::Mat2x3&&>(x).transposed() = math::Mat3x2({ 2, 3 },
                                                               { 4, 5 },
                                                               { 6, 7 });
    ARCHON_CHECK_EQUAL(x, math::Mat2x3({ 4, 6, 8 },
                                       { 5, 7, 9 }));

    ARCHON_CHECK_EQUAL(x.transposed().transposed(), x);
}


ARCHON_TEST(Math_Mat_Transpose)
{
    math::Mat3 x = {{ 1, 2, 3 },
                    { 4, 5, 6 },
                    { 7, 8, 9 }};
    x.transpose();
    ARCHON_CHECK_EQUAL(x, math::Mat3({ 1, 4, 7 },
                                     { 2, 5, 8 },
                                     { 3, 6, 9 }));
}


namespace {


template<std::size_t N>
void test_invert(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr(0.5); // 0.5 -> 1.0
    math::Mat x = math::gen_mat<N>([&](std::size_t i, std::size_t j) {
        return 0.25 + (i <= j ? distr(random) : 0);
    });
    math::Mat y = x;
    if (ARCHON_CHECK(y.try_inv())) {
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(x * y, math::ident<N>(), math::test::matrix_compare(100 * eps));
    }
}


} // unnamed namespace


ARCHON_TEST(Math_Mat_Invert)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (std::size_t i = 0; i < 16; ++i) {
        test_invert<1>(test_context, random);
        test_invert<2>(test_context, random);
        test_invert<3>(test_context, random);
        test_invert<4>(test_context, random);
        test_invert<5>(test_context, random);
    }
}


namespace {


template<std::size_t M, std::size_t N>
void test_decompose(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr; // 0.0 -> 1.0
    math::Mat x = math::gen_mat<M, N>([&](std::size_t, std::size_t) {
        return distr(random);
    });
    constexpr std::size_t n = std::min(M, N);
    math::Mat y = x;
    std::array<std::size_t, n> pivots = y.decompose();
    math::Mat p = math::ident<M>();
    math::Mat l = math::extend<M, n>(math::ident<n>());
    math::Mat u = math::Mat<n, N>();
    for (std::size_t i = 0; i < M; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            if (i <= j) {
                u[i][j] = y[i][j];
            }
            else {
                l[i][j] = y[i][j];
            }
        }
    }
    for (std::size_t k = 0; k < n; ++k) {
        auto perm = [&](std::size_t i) {
            if (i == k)
                return pivots[k];
            if (i == pivots[k])
                return k;
            return i;
        };
        p *= math::gen_mat<M>([&](std::size_t i, std::size_t j) {
            return (perm(j) == i ? 1 : 0);
        });
    }
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(p * l * u, x, math::test::matrix_compare(10 * eps));
}


} // unnamed namespace


ARCHON_TEST(Math_Mat_Decompose)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (std::size_t i = 0; i < 16; ++i) {
        test_decompose<4, 4>(test_context, random);
        test_decompose<4, 5>(test_context, random);
        test_decompose<5, 4>(test_context, random);
        test_decompose<4, 6>(test_context, random);
        test_decompose<6, 4>(test_context, random);
    }
}


namespace {


template<std::size_t N>
void test_invert_triangular(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr(0.5); // 0.5 -> 1.0

    // Do not assume matrix is unitriangular
    {
        math::Mat x = math::gen_mat<N>([&](std::size_t, std::size_t) {
            return distr(random);
        });
        math::Mat y = x;
        constexpr bool assume_unitri = false;
        if (ARCHON_CHECK(y.template try_lower_tri_inv<assume_unitri>())) {
            for (std::size_t i = 0; i < N; ++i) {
                for (std::size_t j = 0; j < N; ++j) {
                    if (i < j) {
                        // Verify that elements above the diagonal are unmodified
                        ARCHON_CHECK_EQUAL(x[i][j], y[i][j]);
                        x[i][j] = 0;
                        y[i][j] = 0;
                    }
                }
            }
            double eps = std::numeric_limits<double>::epsilon();
            ARCHON_CHECK_COMPARE(x * y, math::ident<N>(), math::test::matrix_compare(10 * eps));
        }
    }

    // Assume matrix is unitriangular
    {
        math::Mat x = math::gen_mat<N>([&](std::size_t, std::size_t) {
            return distr(random);
        });
        math::Mat y = x;
        constexpr bool assume_unitri = true;
        if (ARCHON_CHECK(y.template try_lower_tri_inv<assume_unitri>())) {
            for (std::size_t i = 0; i < N; ++i) {
                for (std::size_t j = 0; j < N; ++j) {
                    if (i <= j) {
                        // Verify that elements on, and above the diagonal are unmodified
                        ARCHON_CHECK_EQUAL(x[i][j], y[i][j]);
                        if (i == j) {
                            x[i][j] = 1;
                            y[i][j] = 1;
                        }
                        else {
                            x[i][j] = 0;
                            y[i][j] = 0;
                        }
                    }
                }
            }
            double eps = std::numeric_limits<double>::epsilon();
            ARCHON_CHECK_COMPARE(x * y, math::ident<N>(), math::test::matrix_compare(10 * eps));
        }
    }

    // Verify detection of singular matrix
    {
        math::Mat x = math::gen_mat<N>([&](std::size_t, std::size_t) {
            return distr(random);
        });
        std::size_t i = std::uniform_int_distribution<std::size_t>(0, N - 1)(random);
        x[i][i] = 0;
        constexpr bool assume_unitri = false;
        ARCHON_CHECK_NOT(x.template try_lower_tri_inv<assume_unitri>());
    }
}


} // unnamed namespace


ARCHON_TEST(Math_Mat_InvertTriangular)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (std::size_t i = 0; i < 16; ++i) {
        test_invert_triangular<1>(test_context, random);
        test_invert_triangular<2>(test_context, random);
        test_invert_triangular<3>(test_context, random);
        test_invert_triangular<4>(test_context, random);
        test_invert_triangular<5>(test_context, random);
    }
}
