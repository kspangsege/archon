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

#ifndef ARCHON_X_MATH_X_VECTOR_BASE_HPP
#define ARCHON_X_MATH_X_VECTOR_BASE_HPP

/// \file


#include <type_traits>
#include <array>


namespace archon::math {


/// \brief Indirect base of vector class.
///
/// This is an indirect base class of \ref math::Vector and provides the platform on top of
/// which the specializations of \ref math::VectorBase2 are built.
///
template<int N, class T> class VectorBase1 {
public:
    using comp_type = T;

    static_assert(std::is_floating_point_v<T>);
    static_assert(N > 0);

    using array_type = std::array<T, N>;

    /// \brief Construct vector with all components set to zero.
    ///
    /// This is the default constructor. A default constructed vector has all its components set to zero.
    ///
    constexpr VectorBase1() noexcept = default;

    /// \brief Construct vector with all components set to specific value.
    ///
    /// This constructor sets all the components of the vector equal to the specified
    /// component value.
    ///
    constexpr VectorBase1(comp_type) noexcept;

    /// \{
    ///
    /// \brief Construct vector from array of component values.
    ///
    /// These constructors set the components of the vector equal to the components of the
    /// specified array.
    ///
    template<class U> constexpr VectorBase1(U (& components)[N]) noexcept;
    template<class U> constexpr VectorBase1(const std::array<U, N>& components) noexcept;
    /// \}

    /// \{
    ///
    /// \brief Access vector components as array.
    ///
    /// These functions expose the components of the vector as an array.
    ///
    constexpr auto components() noexcept       -> array_type&;
    constexpr auto components() const noexcept -> const array_type&;
    /// \}

private:
    array_type m_components = {};
};



/// \brief Direct base of vector class.
///
/// This is a direct base class of \ref math::Vector and exists to allow for specializations
/// to provide additional convenience constructors.
///
/// \sa \ref VectorBase2<2, T>
/// \sa \ref VectorBase2<3, T>
/// \sa \ref VectorBase2<4, T>
///
template<int N, class T> class VectorBase2
    : public math::VectorBase1<N, T> {
public:
    using math::VectorBase1<N, T>::VectorBase1;
};



/// \brief Direct base of vector class for 2-vectors.
///
/// This is a specialization of the direct base class of \ref math::Vector for 2-vectors.
///
template<class T> class VectorBase2<2, T>
    : public math::VectorBase1<2, T> {
public:
    using comp_type = T;

    using math::VectorBase1<2, T>::VectorBase1;

    /// \brief Construct 2-vector from two component values.
    ///
    /// This constructor sets the two components of the vector equal to the two specified
    /// component values.
    ///
    constexpr VectorBase2(comp_type, comp_type) noexcept;
};



/// \brief Direct base of vector class for 3-vectors.
///
/// This is a specialization of the direct base class of \ref math::Vector for 3-vectors.
///
template<class T> class VectorBase2<3, T>
    : public math::VectorBase1<3, T> {
public:
    using comp_type = T;

    using math::VectorBase1<3, T>::VectorBase1;

    /// \brief Construct 3-vector from three component values.
    ///
    /// This constructor sets the three components of the vector equal to the three
    /// specified component values.
    ///
    constexpr VectorBase2(comp_type, comp_type, comp_type) noexcept;
};



/// \brief Direct base of vector class for 4-vectors.
///
/// This is a specialization of the direct base class of \ref math::Vector for 4-vectors.
///
template<class T> class VectorBase2<4, T>
    : public math::VectorBase1<4, T> {
public:
    using comp_type = T;

    using math::VectorBase1<4, T>::VectorBase1;

    /// \brief Construct 4-vector from four component values.
    ///
    /// This constructor sets the four components of the vector equal to the four specified
    /// component values.
    ///
    constexpr VectorBase2(comp_type, comp_type, comp_type, comp_type) noexcept;
};








// Implementation


// ============================ VectorBase1 ============================


template<int N, class T>
constexpr VectorBase1<N, T>::VectorBase1(comp_type comp) noexcept
{
    for (int i = 0; i < N; ++i)
        m_components[i] = comp;
}


template<int N, class T>
template<class U> constexpr VectorBase1<N, T>::VectorBase1(U (& components)[N]) noexcept
{
    static_assert(noexcept(T(components[int()])));
    for (int i = 0; i < N; ++i)
        m_components[i] = T(components[i]);
}


template<int N, class T>
template<class U> constexpr VectorBase1<N, T>::VectorBase1(const std::array<U, N>& components) noexcept
{
    static_assert(noexcept(T(components[int()])));
    for (int i = 0; i < N; ++i)
        m_components[i] = T(components[i]);
}


template<int N, class T>
constexpr auto VectorBase1<N, T>::components() noexcept -> array_type&
{
    return m_components;
}


template<int N, class T>
constexpr auto VectorBase1<N, T>::components() const noexcept -> const array_type&
{
    return m_components;
}



// ============================ VectorBase2 ============================


template<class T>
constexpr VectorBase2<2, T>::VectorBase2(comp_type a, comp_type b) noexcept
    : VectorBase2(std::array { a, b })
{
}


template<class T>
constexpr VectorBase2<3, T>::VectorBase2(comp_type a, comp_type b, comp_type c) noexcept
    : VectorBase2(std::array { a, b, c })
{
}


template<class T>
constexpr VectorBase2<4, T>::VectorBase2(comp_type a, comp_type b, comp_type c, comp_type d) noexcept
    : VectorBase2(std::array { a, b, c, d })
{
}


} // namespace archon::math

#endif // ARCHON_X_MATH_X_VECTOR_BASE_HPP
