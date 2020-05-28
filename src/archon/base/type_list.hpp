// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_TYPE_LIST_HPP
#define ARCHON_X_BASE_X_TYPE_LIST_HPP

/// \file


#include <type_traits>


namespace archon::base {


namespace detail {
template<class... T> struct TypeList;
template<class L> struct TypeCount;
template<class L, int i> struct TypeAt;
template<class L, class T> struct TypeAppend;
template<class L, template<class> class P, class F> struct FindType;
template<class L, template<class T, int i> class F, int i> struct HasType;
template<class L, template<class T, int i> class F, int i> struct ForEachType;
} // namespace detail



/// \brief Construct type list from head and tail.
///
/// This is the 'cons' operator for building lists of types.
///
/// \tparam H The head of the list, that is, the first type in the list.
///
/// \tparam T The tail of the list, that is, the list of types following the
/// head. It is `void` if nothing follows the head, otherwise it matches
/// `TypeCons<H2, T2>`.
///
/// Note that `void` is used to represent the empty list.
///
template<class H, class T> struct TypeCons {
    using head = H;
    using tail = T;
};


/// \brief Build type list from specified types.
///
/// Build a type list (see \ref TypeCons) from the specified types.
///
template<class... T> using TypeList = typename detail::TypeList<T...>::type;


/// \brief Number of types in type list.
///
/// The number of elements in the specified type list.
///
/// \tparam L A type list (see \ref TypeCons). Note that `void` is used to
/// represent the empty list.
///
template<class L> constexpr int type_count = detail::TypeCount<L>::value;


/// \brief Get type at index from type list.
///
/// Get the type at the specified index from the specified type list.
///
/// \tparam L A type list (see \ref TypeCons). Note that `void` is used to
/// represent the empty list.
///
/// \tparam i The index within the type list of the type to get.
///
template<class L, int i> using TypeAt = typename detail::TypeAt<L, i>::type;


/// \brief Append a type to a type list.
///
/// Add a type to the the end of a type list.
///
/// \tparam L A type list (see \ref TypeCons). Note that `void` is used to
/// represent the empty list.
///
/// \tparam T The type to append.
///
template<class L, class T> using TypeAppend = typename detail::TypeAppend<L, T>::type;


/// \brief Find type in type list.
///
/// Find the first type in the specified type list that satisfies the specified
/// predicate.
///
/// \tparam L A type list (see \ref TypeCons). Note that `void` is used to
/// represent the empty list.
///
/// \tparam P The predicate. It must be such that `P<T>::value` is true if, and
/// only if the predicate is satisfied for `T`.
///
/// \tparam F The fallback type that will be used if the specified predicate is
/// not satisfied for any type in the specified list.
///
template<class L, template<class> class P, class F = void> using FindType =
    typename detail::FindType<L, P, F>::type;


/// \brief Check list for presence of type.
///
/// Execute a predicate function for each type in the specified type list, and
/// return true if, and only if the predicate returns true for at least one of
/// the types. Iteration over the type list terminates as soon as a predicate
/// returns true.
///
/// \tparam L A type list (see \ref TypeCons). Note that `void` is used to
/// represent the empty list.
///
/// \tparam P A class template that specifies the predicate function to execute
/// for each type in the specified type list. It must be such that the
/// expression `P<T, i>::exec(args...)` is valid and is convertible to a boolean
/// value. Here, `i` is the index of `T` within the list.
///
template<class L, template<class T, int i> class P, class... A> bool has_type(const A&... args);


/// \brief Execute function for each type in list.
///
/// Execute the specified function for each type in the specified type list.
///
/// \tparam L A type list (see \ref TypeCons). Note that `void` is used to
/// represent the empty list.
///
/// \tparam F A class template that specifies the function to execute for each
/// type in the specified type list. It must be such that the expression `F<T,
/// i>::exec(args...)` is valid. Here, `i` is the index of `T` within the list.
///
template<class L, template<class T, int i> class F, class... A>
void for_each_type(const A&... args);








// Implementation


namespace detail {


template<class H, class... T> struct TypeList<H, T...> {
    using type = TypeCons<H, typename TypeList<T...>::type>;
};
template<> struct TypeList<> {
    using type = void;
};


template<class L> struct TypeCount {
    static constexpr int value = 1 + TypeCount<typename L::tail>::value;
};
template<> struct TypeCount<void> {
    static constexpr int value = 0;
};


template<class L, int i> struct TypeAt {
    using type = typename TypeAt<typename L::tail, i - 1>::type;
};
template<class L> struct TypeAt<L, 0> {
    using type = typename L::head;
};


template<class L, class T> struct TypeAppend {
    using type = TypeCons<typename L::head, typename TypeAppend<typename L::tail, T>::type>;
};
template<class T> struct TypeAppend<void, T> {
    using type = TypeCons<T, void>;
};


template<class L, template<class> class P, class F> struct FindType {
private:
    using type_1 = typename L::head;
    using type_2 = typename FindType<typename L::tail, P, F>::type;

public:
    using type = typename std::conditional<P<type_1>::value, type_1, type_2>::type;
};
template<template <class> class P, class F> struct FindType<void, P, F> {
    using type = F;
};


template<class L, template<class T, int i> class P, int i> struct HasType {
    template<class... A> static bool exec(const A&... args)
    {
        return (P<typename L::head, i>::exec(args...) ||
                HasType<typename L::tail, P, i + 1>::exec(args...)); // Throws
    }
};
template<template <class T, int i> class P, int i> struct HasType<void, P, i> {
    template<class... A> static bool exec(const A&...) noexcept
    {
        return false;
    }
};


template<class L, template<class T, int i> class F, int i> struct ForEachType {
    template<class... A> static void exec(const A&... args)
    {
        F<typename L::head, i>::exec(args...); // Throws
        ForEachType<typename L::tail, F, i + 1>::exec(args...); // Throws
    }
};
template<template<class T, int i> class F, int i> struct ForEachType<void, F, i> {
    template<class... A> static void exec(const A&...) noexcept
    {
    }
};


} // namespace detail


template<class L, template<class T, int i> class P, class... A>
inline bool has_type(const A&... args)
{
    return detail::HasType<L, P, 0>::exec(args...); // Throws
}


template<class L, template<class T, int i> class F, class... A>
inline void for_each_type(const A&... args)
{
    detail::ForEachType<L, F, 0>::exec(args...); // Throws
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_TYPE_LIST_HPP
