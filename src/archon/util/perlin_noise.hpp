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

#ifndef ARCHON_X_UTIL_X_PERLIN_NOISE_HPP
#define ARCHON_X_UTIL_X_PERLIN_NOISE_HPP

/// \file


#include <cstddef>
#include <cmath>
#include <memory>
#include <utility>
#include <array>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/random.hpp>
#include <archon/math/vec.hpp>


namespace archon::util {


/// \brief Base class for Perlin noise configurations.
///
/// This class serves as a placeholder for utilities that shared by all template
/// instantiations of \ref PerlinNoise.
///
struct PerlinNoiseBase {
    /// \brief Interpolation schemes.
    ///
    /// These are the interpolation schemes that are available for use with \ref
    /// PerlinNoise.
    ///
    enum class Interp {
        linear,   //< Lerp
        smooth,   //< Smoothstep
        smoother, //< Smootherstep
    };
};


/// \brief Particular configuration of Perlin noise.
///
/// An instance of this class represents a particular configuration of Perlin noise. It uses
/// a grid of randomized gradients as a basis for generating the noise contours. The
/// allocation of memory for holding gradients must be handled by the application (see \ref
/// alloc_gradients() and \ref num_gradients()).
///
/// Given an instance of this class, one computes the noise value at a given location using
/// \ref operator().
///
template<int N = 2, class T = double, PerlinNoiseBase::Interp I = PerlinNoiseBase::Interp::linear>
class PerlinNoise
    : public PerlinNoiseBase {
public:
    using value_type = T;

    static constexpr int num_dims = N;
    static constexpr PerlinNoiseBase::Interp interp = I;

    static_assert(num_dims > 0);

    using size_type = std::array<int, num_dims>;
    using vec_type = math::Vec<num_dims, value_type>;

    /// \brief Total number of gradients for particular grid size.
    ///
    /// This function determines the number of gradients needed for the specified grid
    /// size. This can be used when manually allocating space for gradients.
    ///
    /// There is one gradient at each vertex of the grid. The grid size is the number of
    /// grid modules along each axis. Since the number of vertices in the grid along a
    /// particular axis is one more than the grid size (number of grid modules), the total
    /// number of gradients is determined by adding one to each component of the specified
    /// grid size, and then multiplying those results together.
    ///
    /// \sa \ref alloc_gradients()
    /// \sa \ref init_gradients()
    ///
    static constexpr auto num_gradients(size_type grid_size) -> std::size_t;

    /// \brief Allocate memory for gradients.
    ///
    /// This function allocates dynamic memory for gradients corresponding to the specified
    /// grid size. The number of allocated gradients is as determined by \ref
    /// num_gradients().
    ///
    /// Before passing the returned memory to the constructor, it must be initialized using
    /// \ref init_gradients().
    ///
    static auto alloc_gradients(size_type grid_size) -> std::unique_ptr<vec_type[]>;

    /// \brief Initialize gradients.
    ///
    /// This function initializes the specified array of gradients (\p gradients). This
    /// function assumes that the size of the array is as determined by \ref
    /// num_gradients().
    ///
    /// Regardless of whether memory is allocated manually or using \ref alloc_gradients(),
    /// it must be initialized using this function before being passed to the constructor.
    ///
    template<class E> static void init_gradients(size_type grid_size, vec_type* gradients, E& random_engine);

    /// \brief Amplitude of noise.
    ///
    /// This function returns the standard amplitude of Perlin noise, which is `sqrt(n/2)`,
    /// where `n` is the number of dimensions (\ref num_dims).
    ///
    /// This is the maximum magnitude of values returned by the single-argument overload of
    /// \ref operator(). If `a` is the standard amplitude, `noise` is an instance of
    /// `PerlinNoise`, and `pos` is a position, then the result of `noise(pos)` is between
    /// `-a` and `a`, both inclusive.
    ///
    static value_type amplitude();

    /// \brief Construct Perlin noise configuration.
    ///
    /// This function constructs a Perlin noise configuration with a grid of gradients of
    /// the specified size (\p grid_size), and with the specified set of initialized
    /// gradients (\p gradients).
    ///
    /// The gradients must have been initialized using \ref init_gradients().  The ownership
    /// of the memory, that contains the gradients, remains with the caller. The caller must
    /// ensure that the gradients remain "alive" for as long as the Perlin noise object
    /// remains in use (destruction of the Perlin noise object is allowed to occur after the
    /// destruction of the gradients).
    ///
    /// The grid gauge (\p grid_gauge) is the size of a grid module, which is also the
    /// distance from one grid vertex to the next along a particular axis.
    ///
    /// Along a particular axis, `i`, the grid extends from `grid_pos[i]` to `grid_pos[i] +
    /// grid_size[i] * grid_gauge[i]`.
    ///
    PerlinNoise(size_type grid_size, vec_type grid_gauge, vec_type grid_pos, const vec_type* gradients) noexcept;

    /// \{
    ///
    /// \brief Calculate noise value at particular position.
    ///
    /// These operators calculate the noise value at the specified position. The
    /// single-argument overload returns the noise value in the range of the standard
    /// amplitude (see \ref amplitude()). The three-argument overload returns the noise
    /// value linearly transformed to the specified range (\p from -> \p to).
    ///
    /// \note The returned value may slightly overflow the standard amplitude, or specified
    /// range, whichever applies, due to numerical imprecision. Manual clamping is necessary
    /// when no overlow can be tolerated.
    ///
    /// When the specified position (\p pos) is outside the grid (see \ref PerlinNoise() for
    /// information about the extent of the grid), the returned value is the value at the
    /// closes point on the boundary of the grid.
    ///
    auto operator()(const vec_type pos) -> value_type;
    auto operator()(const vec_type pos, value_type from, value_type to) -> value_type;
    /// \}

private:
    size_type m_grid_size;
    vec_type m_inv_grid_gauge;
    vec_type m_grid_pos;
    const vec_type* m_gradients;

    static auto interpolate(value_type a, value_type b, value_type t) -> value_type;
};








// Implementation


template<int N, class T, PerlinNoiseBase::Interp I>
constexpr auto PerlinNoise<N, T, I>::num_gradients(size_type grid_size) -> std::size_t
{
    std::size_t n = 1;
    for (int i = 0; i < num_dims; ++i) {
        if (ARCHON_UNLIKELY(grid_size[i] < 1))
            throw std::out_of_range("Grid size component");
        std::size_t m = 0;
        core::int_cast(grid_size[i], m); // Throws
        core::int_add(m, 1); // Throws
        core::int_mul(n, m); // Throws
    }
    return n;
}


template<int N, class T, PerlinNoiseBase::Interp I>
inline auto PerlinNoise<N, T, I>::alloc_gradients(size_type grid_size) -> std::unique_ptr<vec_type[]>
{
    std::size_t n = num_gradients(grid_size); // Throws
    return std::make_unique<vec_type[]>(n); // Throws
}


template<int N, class T, PerlinNoiseBase::Interp I>
template<class E> void PerlinNoise<N, T, I>::init_gradients(size_type grid_size, vec_type* gradients, E& random_engine)
{
    std::size_t n = num_gradients(grid_size); // Throws
    for (std::size_t i = 0; i < n; ++i)
        core::rand_unit_vec(random_engine, gradients[i].components()); // Throws
}


template<int N, class T, PerlinNoiseBase::Interp I>
inline auto PerlinNoise<N, T, I>::amplitude() -> value_type
{
    return std::sqrt(value_type(num_dims) / 4); // Throws
}


template<int N, class T, PerlinNoiseBase::Interp I>
inline PerlinNoise<N, T, I>::PerlinNoise(size_type grid_size, vec_type grid_gauge, vec_type grid_pos,
                                      const vec_type* gradients) noexcept
    : m_grid_size(grid_size)
    , m_grid_pos(grid_pos)
    , m_gradients(gradients)
{
    for (int i = 0; i < num_dims; ++i)
        m_inv_grid_gauge[i] = value_type(1) / grid_gauge[i];
}


template<int N, class T, PerlinNoiseBase::Interp I>
auto PerlinNoise<N, T, I>::operator()(const vec_type pos) -> value_type
{
    vec_type vec = pos - m_grid_pos;
    std::size_t index = 0;
    std::array<std::size_t, num_dims> index_shifts;
    {
        std::size_t multiplier = 1;
        for (int i = 0; i < num_dims; ++i) {
            value_type val_1 = vec[i] * m_inv_grid_gauge[i];
            value_type val_2 = 0; // Fractional part
            int val_3 = 0; // Integer part
            if (ARCHON_LIKELY(val_1 >= 0)) {
                if (ARCHON_LIKELY(core::float_less_int(val_1, m_grid_size[i]))) { // Throws
                    value_type val = std::trunc(val_1); // Throws
                    val_2 = val_1 - val;
                    val_3 = int(val);
                }
                else {
                    val_2 = 1;
                    val_3 = m_grid_size[i] - 1;
                }
            }
            vec[i] = val_2;
            ARCHON_ASSERT(val_3 >= 0 && val_3 < m_grid_size[i]);
            index += multiplier * val_3;
            index_shifts[i] = multiplier;
            multiplier *= std::size_t(m_grid_size[i]) + 1;
        }
    }

    vec_type vec_shift = {};

    int dim;
    value_type val;
    std::array<value_type, num_dims> stack;

  enter:
    dim = 0;
    val = math::dot(m_gradients[index], vec - vec_shift); // Throws

  leave:
    if (ARCHON_LIKELY(dim < num_dims)) {
        if (vec_shift[dim] == 0) {
            stack[dim] = val;
            index += index_shifts[dim];
            vec_shift[dim] = 1;
            goto enter;
        }
        index -= index_shifts[dim];
        vec_shift[dim] = 0;
        val = interpolate(stack[dim], val, vec[dim]); // Throws
        dim += 1;
        goto leave;
    }
    return val;
}


template<int N, class T, PerlinNoiseBase::Interp I>
inline auto PerlinNoise<N, T, I>::operator()(const vec_type pos, value_type from, value_type to) -> value_type
{
    // FIXME: Document that returned value may overflow slightly due to numerical imprecision                        
    value_type val = (1 + operator()(pos) / amplitude()) / 2; // Throws
    return from + val * (to - from);
}


template<int N, class T, PerlinNoiseBase::Interp I>
inline auto PerlinNoise<N, T, I>::interpolate(value_type a, value_type b, value_type t) -> value_type
{
    using promoted_value_type = decltype(std::declval<value_type>() + std::declval<double>());
    promoted_value_type u = t;
    switch (interp) {
        case Interp::linear:
            break;
        case Interp::smooth:
            u = (3.0 - t * 2.0) * t * t;
            break;
        case Interp::smoother:
            u = (t * (t * 6.0 - 15.0) + 10.0) * t * t * t;
            break;
    }
    return value_type(core::lerp<promoted_value_type>(a, b, u)); // Throws
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_PERLIN_NOISE_HPP
