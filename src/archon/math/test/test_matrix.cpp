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
#include <algorithm>
#include <array>
#include <random>

#include <archon/core/char_mapper.hpp>
#include <archon/core/value_formatter.hpp>
#include <archon/check.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/test/util.hpp>


using namespace archon;
namespace test = math::test;


namespace {


ARCHON_TEST_VARIANTS(char_variants,
                     ARCHON_TEST_TYPE(char,    Regular),
                     ARCHON_TEST_TYPE(wchar_t, Wide));


} // unnamed namespace


ARCHON_TEST(Math_Matrix_Basics)
{
    math::Matrix2 mat;

    mat[0] = math::Vector2(1, 2);
    mat[1] = std::array { 3, 4 };
    ARCHON_CHECK_EQUAL(mat, math::Matrix2({ 1, 2 },
                                          { 3, 4 }));

    mat.set_col(0, math::Vector2(5, 6));
    mat.set_col(1, std::array { 7, 8 });
    ARCHON_CHECK_EQUAL(mat, math::Matrix2({ 5, 7 },
                                          { 6, 8 }));

    mat.set_diag(math::Vector2(1, 2));
    ARCHON_CHECK_EQUAL(mat, math::Matrix2({ 1, 7 },
                                          { 6, 2 }));
    mat.set_diag(std::array { 3, 4 });
    ARCHON_CHECK_EQUAL(mat, math::Matrix2({ 3, 7 },
                                          { 6, 4 }));
}


ARCHON_TEST(Math_Matrix_Subscr)
{
    math::Matrix2 x = {{ 1, 2 },
                       { 3, 4 }};
    ARCHON_CHECK_EQUAL(x[0], math::Vector2(1, 2));
    ARCHON_CHECK_EQUAL(x[1], math::Vector2(3, 4));
    x[0] = math::Vector2(5, 6);
    ARCHON_CHECK_EQUAL(x[0], math::Vector2(5, 6));
}


ARCHON_TEST(Math_Matrix_Col)
{
    math::Matrix2 x = {{ 1, 2 },
                       { 3, 4 }};
    ARCHON_CHECK_EQUAL(x.get_col(0), math::Vector2(1, 3));
    ARCHON_CHECK_EQUAL(x.get_col(1), math::Vector2(2, 4));
    x.set_col(0, math::Vector2(5, 6));
    ARCHON_CHECK_EQUAL(x.get_col(0), math::Vector2(5, 6));
}


ARCHON_TEST(Math_Matrix_Diag)
{
    math::Matrix2 x = {{ 1, 2 },
                       { 3, 4 }};
    ARCHON_CHECK_EQUAL(x.get_diag(), math::Vector2(1, 4));
    x.set_diag(math::Vector2(5, 6));
    ARCHON_CHECK_EQUAL(x.get_diag(), math::Vector2(5, 6));
}


ARCHON_TEST(Math_Matrix_Submatrix)
{
    math::Matrix3x4 x = {{ 1, 2, 3, 4 },
                         { 3, 4, 5, 6 },
                         { 5, 6, 7, 8 }};
    ARCHON_CHECK_EQUAL((x.get_submatrix<2, 3>(0, 0)),
                       math::Matrix2x3({ 1, 2, 3 },
                                       { 3, 4, 5 }));

    x.set_submatrix<2, 3>(0, 0, math::Matrix2x3({ 2, 3, 4 },
                                    { 5, 6, 7 }));
    ARCHON_CHECK_EQUAL((x.get_submatrix<2, 3>(0, 0)),
                       math::Matrix2x3({ 2, 3, 4 },
                                       { 5, 6, 7 }));
}


ARCHON_TEST(Math_Matrix_Compare)
{
    math::Matrix2 mat_1 = {{ 1, 3 }, { 4, 6 }};
    math::Matrix2 mat_2 = {{ 1, 3 }, { 4, 6 }};
    math::Matrix2 mat_3 = {{ 1, 3 }, { 4, 5 }};
    math::Matrix2 mat_4 = {{ 2, 3 }, { 4, 6 }};

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


ARCHON_TEST(Math_Matrix_ComponentTypeConversion)
{
    // Lossless copy construct and copy assign from different component type
    {
        math::Matrix<2, 2, float> mat_1 = {{ 1, 2 },
                                           { 3, 4 }};
        math::Matrix<2, 2, float> mat_2 = {{ 5, 6 },
                                           { 7, 8 }};
        math::Matrix<2, 2, double> mat_3 = mat_1;
        ARCHON_CHECK_EQUAL(mat_3, mat_1);
        mat_3 = mat_2;
        ARCHON_CHECK_EQUAL(mat_3, mat_2);
    }

    // Lossy copy construct and copy assign from different component type
    {
        math::Matrix<2, 2, double> mat_1 = {{ 1, 2 },
                                            { 3, 4 }};
        math::Matrix<2, 2, double> mat_2 = {{ 5, 6 },
                                            { 7, 8 }};
        math::Matrix<2, 2, float> mat_3 = math::Matrix<2, 2, float>(mat_1);
        ARCHON_CHECK_EQUAL(mat_3, mat_1);
        mat_3 = math::Matrix<2, 2, float>(mat_2);
        ARCHON_CHECK_EQUAL(mat_3, mat_2);
    }
}


ARCHON_TEST(Math_Matrix_Generate)
{
    math::Matrix x  = math::Matrix<3, 4>::generate([](int i, int j) noexcept {
        return double(i * 10 + j);
    });
    static_assert(x.num_rows == 3);
    static_assert(x.num_cols == 4);

    for (int i = 0; i < x.num_rows; ++i) {
        for (int j = 0; j < x.num_cols; ++j)
            ARCHON_CHECK_EQUAL(x[i][j], i * 10 + j);
    }
}


ARCHON_TEST_BATCH(Math_Matrix_Format, char_variants)
{
    using char_type = test_type;
    std::array<char_type, 32> seed_memory_1;
    core::BasicValueFormatter formatter(seed_memory_1, test_context.locale);
    std::array<char_type, 32> seed_memory_2;
    core::BasicStringWidener widener(test_context.locale, seed_memory_2);
    ARCHON_CHECK_EQUAL(formatter.format(math::Matrix2({ 1.5, 2.5 },
                                                      { 3.5, 4.5 })), widener.widen("[[1.5, 2.5], [3.5, 4.5]]"));
}


ARCHON_TEST(Math_Matrix_AdditionSubtractionNegation)
{
    math::Matrix2 a = {{ 1, 2 },
                       { 3, 5 }};
    math::Matrix2F b = {{ 3, 5 },
                        { 7, 9 }};
    ARCHON_CHECK_EQUAL(a + b, math::Matrix2({  4,  7 },
                                            { 10, 14 }));
    ARCHON_CHECK_EQUAL(a - b, math::Matrix2({ -2, -3 },
                                            { -4, -4 }));
    ARCHON_CHECK_EQUAL(-a, math::Matrix2({ -1, -2 },
                                         { -3, -5 }));
    a += b;
    ARCHON_CHECK_EQUAL(a, math::Matrix2({  4,  7 },
                                        { 10, 14 }));
    a -= b;
    ARCHON_CHECK_EQUAL(a, math::Matrix2({ 1, 2 },
                                        { 3, 5 }));
    static_assert(std::is_same_v<decltype(a + b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a - b)::comp_type, double>);
}


ARCHON_TEST(Math_Matrix_Scaling)
{
    math::Matrix2F a = {{ 1, 2 },
                        { 3, 5 }};
    ARCHON_CHECK_EQUAL(a * 2, math::Matrix2({ 2,  4 },
                                            { 6, 10 }));
    ARCHON_CHECK_EQUAL(3 * a, math::Matrix2({ 3,  6 },
                                            { 9, 15 }));
    ARCHON_CHECK_EQUAL(a / 2, math::Matrix2({ 0.5, 1.0 },
                                            { 1.5, 2.5 }));
    a *= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Matrix2({ 2,  4 },
                                        { 6, 10 }));
    a /= 2.0F;
    ARCHON_CHECK_EQUAL(a, math::Matrix2({ 1, 2 },
                                        { 3, 5 }));
    static_assert(std::is_same_v<decltype(2.0 * a)::comp_type, double>);
    static_assert(std::is_same_v<decltype(2.0F * a)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a * 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * 2.0F)::comp_type, float>);
    static_assert(std::is_same_v<decltype(a / 2.0)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a / 2.0F)::comp_type, float>);
}


ARCHON_TEST(Math_Matrix_MatrixVectorMultiplication)
{
    math::Matrix2F a = {{ 1, 2 },
                        { 3, 4 }};
    math::Matrix3x2F b = {{ 1, 3 },
                          { 2, 5 },
                          { 4, 6 }};
    math::Vector2 c = { 3, 5 };
    ARCHON_CHECK_EQUAL(a * c, math::Vector2(13, 29));
    ARCHON_CHECK_EQUAL(b * c, math::Vector3(18, 31, 42));
    static_assert(std::is_same_v<decltype(a * c)::comp_type, double>);
    static_assert(std::is_same_v<decltype(b * c)::comp_type, double>);
}


ARCHON_TEST(Math_Matrix_VectorMatrixMultiplication)
{
    math::Vector2 a = { 3, 5 };
    math::Matrix2F b = {{ 1, 2 },
                        { 3, 4 }};
    math::Matrix2x3F c = {{ 1, 2, 3 },
                          { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(a * b, math::Vector2(18, 26));
    ARCHON_CHECK_EQUAL(a * c, math::Vector3(23, 31, 39));
    static_assert(std::is_same_v<decltype(a * b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * c)::comp_type, double>);
}


ARCHON_TEST(Math_Matrix_MatrixMatrixMultiplication)
{
    math::Matrix2 a = {{ 2, 4 },
                       { 3, 5 }};
    math::Matrix2F b = {{ 1, 2 },
                        { 3, 4 }};
    math::Matrix2x3F c = {{ 1, 2, 3 },
                          { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(a * b, math::Matrix2({ 14, 20 },
                                            { 18, 26 }));
    ARCHON_CHECK_EQUAL(a * c, math::Matrix2x3({ 18, 24, 30 },
                                              { 23, 31, 39 }));
    static_assert(std::is_same_v<decltype(a * b)::comp_type, double>);
    static_assert(std::is_same_v<decltype(a * c)::comp_type, double>);
}


ARCHON_TEST(Math_Matrix_ScalarMatrixDivision)
{
    math::Matrix2 x = {{ 1, 2 },
                       { 3, 4 }};
    double eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(2 / x, math::Matrix2({ -4,  2 },
                                              {  3, -1 }),
                         test::matrix_compare(10 * eps));
}


ARCHON_TEST(Math_Matrix_MatrixMatrixDivision)
{
    math::Matrix3x2 x = {{ 5, 6 },
                         { 6, 7 },
                         { 7, 8 }};
    math::Matrix2 y = {{ 1, 2 },
                       { 3, 4 }};
    double eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(x / y, math::Matrix3x2({ -1.0, 2.0 },
                                                { -1.5, 2.5 },
                                                { -2.0, 3.0 }),
                         test::matrix_compare(10 * eps));
}


ARCHON_TEST(Math_Matrix_Transpose)
{
    math::Matrix2x3 x = {{ 1, 2, 3 },
                         { 4, 5, 6 }};
    ARCHON_CHECK_EQUAL(math::transpose(x), math::Matrix3x2({ 1, 4 },
                                                           { 2, 5 },
                                                           { 3, 6 }));
}


ARCHON_TEST(Math_Matrix_Trace)
{
    math::Matrix2 x = {{ 1, 2 },
                       { 3, 4 }};
    ARCHON_CHECK_EQUAL(math::tr(x), 5);
    math::Matrix3 y = {{ 1, 2, 3 },
                       { 4, 5, 6 },
                       { 7, 8, 9 }};
    ARCHON_CHECK_EQUAL(math::tr(y), 15);
}


ARCHON_TEST(Math_Matrix_Determinant)
{
    double eps = std::numeric_limits<double>::epsilon();
    math::Matrix2 x = {{  1, 3 },
                       { -3, 2 }};
    ARCHON_CHECK_COMPARE(math::det(x), 11, test::scalar_compare(10 * eps));
    math::Matrix3 y = {{  1, 2, -1 },
                       {  0, 3, -4 },
                       { -1, 2,  1 }};
    ARCHON_CHECK_COMPARE(math::det(y), 16, test::scalar_compare(10 * eps));
}


ARCHON_TEST(Math_Matrix_Inverse)
{
    math::Matrix2F x = {{ 1, 2 },
                        { 3, 4 }};
    double eps = std::numeric_limits<float>::epsilon();
    ARCHON_CHECK_COMPARE(math::inv(x), math::Matrix2({ -2.0,  1.0 },
                                                     {  1.5, -0.5 }),
                         test::matrix_compare(10 * eps));
    static_assert(std::is_same_v<decltype(math::inv(x))::comp_type, float>);
}


ARCHON_TEST(Math_Matrix_OuterProductOfVectors)
{
    math::Vector2 a = { 1, 2 };
    math::Vector3F b = { 3, 4, 5 };
    ARCHON_CHECK_EQUAL(math::outer(a, b), math::Matrix2x3({ 3, 4,  5 },
                                                          { 6, 8, 10 }));
    static_assert(std::is_same_v<decltype(math::outer(a, b))::comp_type, double>);
}


namespace {


template<int N> void test_try_invert(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr(0.5); // 0.5 -> 1.0
    math::Matrix x = math::Matrix<N>::generate([&](int i, int j) {
        return 0.25 + (i <= j ? distr(random) : 0); // Throws
    }); // Throws
    math::Matrix y = x;
    if (ARCHON_CHECK(math::try_inv(y))) {
        double eps = std::numeric_limits<double>::epsilon();
        ARCHON_CHECK_COMPARE(x * y, math::Matrix<N>::identity(), test::matrix_compare(100 * eps));
    }
}


template<int N> void test_try_invert_lower_triangular(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr(0.5); // 0.5 -> 1.0

    // Do not assume matrix is unitriangular
    {
        math::Matrix x = math::Matrix<N>::generate([&](int, int) {
            return distr(random); // Throws
        }); // Throws
        math::Matrix y = x;
        constexpr bool assume_unitri = false;
        if (ARCHON_CHECK(math::try_lower_tri_inv<assume_unitri>(y))) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    if (i < j) {
                        // Verify that elements above the diagonal are unmodified
                        ARCHON_CHECK_EQUAL(x[i][j], y[i][j]);
                        x[i][j] = 0;
                        y[i][j] = 0;
                    }
                }
            }
            double eps = std::numeric_limits<double>::epsilon();
            ARCHON_CHECK_COMPARE(x * y, math::Matrix<N>::identity(), test::matrix_compare(10 * eps));
        }
    }

    // Assume matrix is unitriangular
    {
        math::Matrix x = math::Matrix<N>::generate([&](int, int) {
            return distr(random); // Throws
        }); // Throws
        math::Matrix y = x;
        constexpr bool assume_unitri = true;
        if (ARCHON_CHECK(math::try_lower_tri_inv<assume_unitri>(y))) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
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
            ARCHON_CHECK_COMPARE(x * y, math::Matrix<N>::identity(), test::matrix_compare(10 * eps));
        }
    }

    // Verify detection of singular matrix
    {
        math::Matrix x = math::Matrix<N>::generate([&](int, int) {
            return distr(random); // Throws
        }); // Throws
        int i = std::uniform_int_distribution<int>(0, N - 1)(random);
        x[i][i] = 0;
        constexpr bool assume_unitri = false;
        ARCHON_CHECK_NOT(math::try_lower_tri_inv<assume_unitri>(x));
    }
}


template<int N> void test_try_invert_upper_triangular(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr(0.5); // 0.5 -> 1.0

    // Do not assume matrix is unitriangular
    {
        math::Matrix x = math::Matrix<N>::generate([&](int, int) {
            return distr(random); // Throws
        }); // Throws
        math::Matrix y = x;
        constexpr bool assume_unitri = false;
        if (ARCHON_CHECK(math::try_upper_tri_inv<assume_unitri>(y))) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    if (i > j) {
                        // Verify that elements below the diagonal are unmodified
                        ARCHON_CHECK_EQUAL(x[i][j], y[i][j]);
                        x[i][j] = 0;
                        y[i][j] = 0;
                    }
                }
            }
            double eps = std::numeric_limits<double>::epsilon();
            ARCHON_CHECK_COMPARE(x * y, math::Matrix<N>::identity(), test::matrix_compare(10 * eps));
        }
    }

    // Assume matrix is unitriangular
    {
        math::Matrix x = math::Matrix<N>::generate([&](int, int) {
            return distr(random); // Throws
        }); // Throws
        math::Matrix y = x;
        constexpr bool assume_unitri = true;
        if (ARCHON_CHECK(math::try_upper_tri_inv<assume_unitri>(y))) {
            for (int i = 0; i < N; ++i) {
                for (int j = 0; j < N; ++j) {
                    if (i >= j) {
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
            ARCHON_CHECK_COMPARE(x * y, math::Matrix<N>::identity(), test::matrix_compare(10 * eps));
        }
    }

    // Verify detection of singular matrix
    {
        math::Matrix x = math::Matrix<N>::generate([&](int, int) {
            return distr(random); // Throws
        }); // Throws
        int i = std::uniform_int_distribution<int>(0, N - 1)(random);
        x[i][i] = 0;
        constexpr bool assume_unitri = false;
        ARCHON_CHECK_NOT(math::try_upper_tri_inv<assume_unitri>(x));
    }
}


template<int M, int N> void test_decompose(check::TestContext& test_context, std::mt19937_64& random)
{
    std::uniform_real_distribution<> distr; // 0.0 -> 1.0
    math::Matrix<M, N> x = math::Matrix<M, N>::generate([&](int, int) {
        return distr(random); // Throws
    }); // Throws
    constexpr int n = std::min(M, N);
    math::Matrix y = x;
    std::array<int, n> pivots = math::decompose(y);
    math::Matrix p = math::Matrix<M>::identity();
    math::Matrix l = math::extend<M, n>(math::Matrix<n>::identity(), 0, 0);
    math::Matrix u = math::Matrix<n, N>();
    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i <= j) {
                u[i][j] = y[i][j];
            }
            else {
                l[i][j] = y[i][j];
            }
        }
    }
    for (int k = 0; k < n; ++k) {
        auto perm = [&](int i) noexcept {
            if (i == k)
                return pivots[k];
            if (i == pivots[k])
                return k;
            return i;
        };
        p *= math::Matrix<M>::generate([&](int i, int j) noexcept {
            return (perm(j) == i ? 1 : 0);
        });
    }
    double eps = std::numeric_limits<double>::epsilon();
    ARCHON_CHECK_COMPARE(p * l * u, x, test::matrix_compare(10 * eps));
}


} // unnamed namespace


ARCHON_TEST(Math_Matrix_TryInvert)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (int i = 0; i < 16; ++i) {
        test_try_invert<1>(test_context, random);
        test_try_invert<2>(test_context, random);
        test_try_invert<3>(test_context, random);
        test_try_invert<4>(test_context, random);
        test_try_invert<5>(test_context, random);
    }
}


ARCHON_TEST(Math_Matrix_TryInvertLowerTriangular)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (int i = 0; i < 16; ++i) {
        test_try_invert_lower_triangular<1>(test_context, random);
        test_try_invert_lower_triangular<2>(test_context, random);
        test_try_invert_lower_triangular<3>(test_context, random);
        test_try_invert_lower_triangular<4>(test_context, random);
        test_try_invert_lower_triangular<5>(test_context, random);
    }
}


ARCHON_TEST(Math_Matrix_TryInvertUpperTriangular)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (int i = 0; i < 16; ++i) {
        test_try_invert_upper_triangular<1>(test_context, random);
        test_try_invert_upper_triangular<2>(test_context, random);
        test_try_invert_upper_triangular<3>(test_context, random);
        test_try_invert_upper_triangular<4>(test_context, random);
        test_try_invert_upper_triangular<5>(test_context, random);
    }
}


ARCHON_TEST(Math_Matrix_Decompose)
{
    std::mt19937_64 random(test_context.seed_seq());

    for (int i = 0; i < 16; ++i) {
        test_decompose<4, 4>(test_context, random);
        test_decompose<4, 5>(test_context, random);
        test_decompose<5, 4>(test_context, random);
        test_decompose<4, 6>(test_context, random);
        test_decompose<6, 4>(test_context, random);
    }
}
