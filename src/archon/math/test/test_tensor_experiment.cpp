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


#include <archon/check.hpp>


using namespace archon;


namespace {


/*
template<std::size_t... N> struct Index;

template<std::size_t M, std::size_t... N> struct Index {
    std::size_t head;
    Index<N...> tail;
};

template<> struct Index<> {};
*/


template<std::size_t... N> struct Size {
    static constexpr std::size_t order = sizeof... (N);
};

/*
template<std::size_t M, std::size_t... N> static constexpr auto num_components(Size<M, N...>) -> std::size_t
{
    std::size_t n = num_components(Size<N...>()); // Throws
    core::int_mul(n, M); // Throws
    return n;
}

static constexpr auto num_components(Size<>) -> std::size_t
{
    return 1;
}
*/

//template<class R, class S> using Concat = ...;


template<class S, class T> class Tensor {
public:
    static constexpr std::size_t order = S::order;

/*
    auto operator[](std::size_t index) const
    {
        static_assert(order > 0);
        return SliceRef<SliceSize<S>>(m_components.data() + index * num_components(SliceSize<S>));
    }

    template<class L> auto slice(std::array<bool, N>)
*/

private:
//    std::array<T, num_components(S())> m_components;
};


template<std::size_t N, class T = double> using Vector = Tensor<Size<N>, T>;

template<std::size_t M, std::size_t N, class T = double> using Matrix = Tensor<Size<M, N>, T>;


/*
auto outer(Tensor<R, T> a, Tensor<S, U> b)
{
    using type = decltype(T() * U());
    Tensor<Concat<R, S>, type> result;
    for_each(a, result, b);
    return result;
}
*/


} // unnamed namespace


ARCHON_TEST(Math_Vec_TensorExperiment)
{
}
