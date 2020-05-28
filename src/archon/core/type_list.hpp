// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_CORE_X_TYPE_LIST_HPP
#define ARCHON_X_CORE_X_TYPE_LIST_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <tuple>

#include <archon/core/features.h>


namespace archon::core {


namespace impl {
template<class... T> struct TypeList;
template<class T, std::size_t N> struct TypeListFromRep;
template<class T> struct TypeListFromTuple;
template<class L> struct TypeCount;
template<class L, std::size_t I> struct TypeAt;
template<class L, class T> struct TypeAppend;
template<class L, class P, class F> struct FindType;
template<class L, class M> struct TypeListConcat;
template<class L, class M> struct TypeListProduct;
template<class L, class T> struct HasType;
template<class L, class P, std::size_t I> struct HasTypeA;
template<class L, class F, std::size_t I> struct ForEachType;
} // namespace impl



/// \brief Construct type list from head and tail.
///
/// This is the 'cons' operator for building lists of types.
///
/// \tparam H The head of the list, that is, the first type in the list.
///
/// \tparam T The tail of the list, that is, the list of types following the head. It is
/// `void` if nothing follows the head, otherwise it matches `TypeCons<H2, T2>`.
///
/// Note that `void` is used to represent the empty list.
///
template<class H, class T> struct TypeCons {
    using head = H;
    using tail = T;
};


/// \brief Build type list from specified types.
///
/// Build a type list (see \ref core::TypeCons) from the specified types.
///
template<class... T> using TypeList = typename impl::TypeList<T...>::type;


/// \brief Build type list by repeating specified type.
///
/// Build a type list (see \ref core::TypeCons) by repeating the specified type (\p T) the
/// specified number of times (\p N).
///
template<class T, std::size_t N> using TypeListFromRep = typename impl::TypeListFromRep<T, N>::type;


/// \brief Build type list from elements of tuple type.
///
/// Build a type list (see \ref core::TypeCons) from the elemenmts of the specified tuple
/// type.
///
/// The tuple type can be `std::tuple', `std::pair`, or `std::array`.
///
template<class T> using TypeListFromTuple = typename impl::TypeListFromTuple<T>::type;


/// \brief Number of types in type list.
///
/// The number of elements in the specified type list.
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
template<class L> constexpr std::size_t type_count = impl::TypeCount<L>::value;


/// \brief Get type at index from type list.
///
/// Get the type at the specified index from the specified type list. Compilation fails if
/// \p I is out of range.
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam I The index within the type list of the type to get.
///
template<class L, std::size_t I> using TypeAt = typename impl::TypeAt<L, I>::type;


/// \brief Append a type to a type list.
///
/// Add a type to the the end of a type list.
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam T The type to append.
///
template<class L, class T> using TypeAppend = typename impl::TypeAppend<L, T>::type;


/// \brief Find type in type list.
///
/// Find the first type in the specified type list that satisfies the specified predicate.
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam P The predicate type. It must be such that `P::template value<T>` is valid,
/// refers to a compile-time constant, and is `true` if, and only if the predicate is
/// satisfied for `T`.
///
/// \tparam F The fallback type that will be used if the specified predicate is not
/// satisfied for any type in the specified list.
///
template<class L, class P, class F = void> using FindType = typename impl::FindType<L, P, F>::type;


/// \brief Concatenation of type lists.
///
/// This type is the concatenation of the two specified type lists.
///
/// \tparam L, M Two type lists (see \ref core::TypeCons). Note that `void` is used to
/// represent the empty list.
///
template<class L, class M> using TypeListConcat = typename impl::TypeListConcat<L, M>::type;


/// \brief Cartesian product of type lists.
///
/// This type is the cartesian product of the two specified type lists. The cartesian
/// product is itself a type list whose elements are instantiatios of `std::pair` such that
/// the first type is one of the types in \p L and the second type is one of the types in \p
/// M. The cartesian product contains all combinations, and in lexicograpical order.
///
/// \tparam L, M Two type lists (see \ref core::TypeCons). Note that `void` is used to
/// represent the empty list.
///
template<class L, class M> using TypeListProduct = typename impl::TypeListProduct<L, M>::type;


/// \brief Cartesian square of type list.
///
/// This type is the cartesian product of the specified type list and itself. See \ref
/// core::TypeListProduct.
///
template<class L> using TypeListSquare = core::TypeListProduct<L, L>;


/// \brief Whether type list contains particular type.
///
/// This value is `true` if, and only if the specified type (\p T) is contained in the
/// specified type list (\p L).
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam T The type whose presence in the list is enquired about.
///
template<class L, class T> constexpr bool has_type = impl::HasType<L, T>::value;


/// \brief Determine "no throw" guarantee for "has type" functions.
///
/// Determine whether \ref core::has_type_a() is guaranteed to not throw exceptions if it is
/// instantiated with the same template arguments.
///
/// This function returns true if, and only if `noexcept(P::template exec<T,
/// I>(std::declval<A>...))` is true for all pairs (`T`, `I`) where `T` is a type in `L` and
/// `I` is the index of `T` in `L`.
///
template<class L, class P, class... A> constexpr bool has_type_a_noexcept() noexcept;


/// \brief Check list for presence of type.
///
/// Execute the specified "type predicate" for each type in the specified type list, and
/// return true if, and only if the predicate returns true for at least one of the
/// types. Iteration over the type list terminates as soon as a predicate returns true.
///
/// This function throws only if the specified predicate throws (see \ref
/// core::has_type_a_noexcept()).
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam P A class that specifies the predicate function to execute for each type in the
/// specified type list. It must be such that the expression `P::template exec<T,
/// I>(args...)` is valid and is convertible to a boolean value. Here, `I` is the index of
/// `T` within the list. The type of `I` is `std::size_t`.
///
template<class L, class P, class... A> constexpr bool has_type_a(A&&... args)
    noexcept(core::has_type_a_noexcept<L, P, A...>());


/// \brief Determine "no throw" guarantee for "for each type" functions.
///
/// Determine whether \ref core::for_each_type() and \ref core::for_each_type_a() are
/// guaranteed to not throw exceptions if they are instantiated with the same template
/// arguments.
///
/// This function returns true if, and only if `noexcept(F::template exec<T,
/// I>(std::declval<A>...))` is true for all pairs (`T`, `I`) where `T` is a type in `L` and
/// `I` is the index of `T` in `L`.
///
template<class L, class F, class... A> constexpr bool for_each_type_noexcept() noexcept;


/// \brief Execute function for each type in list.
///
/// Execute the specified "type function" for each type in the specified type list.
///
/// This function throws only if the specified "type function" throws (see \ref
/// core::for_each_type_noexcept()).
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam F A class that specifies the function to execute for each type in the specified
/// type list. It must be such that the expression `F::template exec<T, I>(args...)` is
/// valid. Here, `I` is the index of `T` within the list.
///
/// \sa \ref core::for_each_type_a()
///
template<class L, class F, class... A> void for_each_type(A&&... args)
    noexcept(core::for_each_type_noexcept<L, F, A...>());


/// \brief Execute function for each type in list until failure.
///
/// Execute the specified "type function" for each type in the specified type list until an
/// execution of the function returns `false`. If execution returns `false` for a particular
/// type, no execution of the function will take place for subsequent types in the list.
///
/// If execution of the "type function" returns `true` for every type in the list, this
/// function returns `true`, otherwise it returns `false`.
///
/// This function throws only if the specified "type function" throws (see \ref
/// core::for_each_type_noexcept()).
///
/// \tparam L A type list (see \ref core::TypeCons). Note that `void` is used to represent
/// the empty list.
///
/// \tparam F A class that specifies the function to execute for each type in the specified
/// type list. It must be such that the expression `bool(F::template exec<T, I>(args...))`
/// is valid. Here, `I` is the index of `T` within the list.
///
/// \sa \ref core::for_each_type()
///
template<class L, class F, class... A> bool for_each_type_a(A&&... args)
    noexcept(core::for_each_type_noexcept<L, F, A...>());








// Implementation


namespace impl {


template<class H, class... T> struct TypeList<H, T...> {
    using type = TypeCons<H, typename TypeList<T...>::type>;
};
template<> struct TypeList<> {
    using type = void;
};


template<class T, std::size_t N> struct TypeListFromRep {
    using type = TypeCons<T, typename TypeListFromRep<T, N - 1>::type>;
};
template<class T> struct TypeListFromRep<T, 0> {
    using type = void;
};


template<class... T> struct TypeListFromTuple<std::tuple<T...>> {
    using type = typename impl::TypeList<T...>::type;
};
template<class T, class U> struct TypeListFromTuple<std::pair<T, U>> {
    using type = typename impl::TypeList<T, U>::type;
};
template<class T, std::size_t N> struct TypeListFromTuple<std::array<T, N>> {
    using type = typename impl::TypeListFromRep<T, N>::type;
};


template<class L> struct TypeCount {
    static constexpr std::size_t value = 1 + TypeCount<typename L::tail>::value;
};
template<> struct TypeCount<void> {
    static constexpr std::size_t value = 0;
};


template<class L, std::size_t I> struct TypeAt {
    using type = typename TypeAt<typename L::tail, I - 1>::type;
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


template<class L, class P, class F> struct FindType {
private:
    using type_1 = typename L::head;
    using type_2 = typename FindType<typename L::tail, P, F>::type;

public:
    using type = typename std::conditional<P::template value<type_1>, type_1, type_2>::type;
};
template<class P, class F> struct FindType<void, P, F> {
    using type = F;
};


template<class L, class M> struct TypeListConcat {
    using type = core::TypeCons<typename L::head, typename TypeListConcat<typename L::tail, M>::type>;
};
template<class M> struct TypeListConcat<void, M> {
    using type = M;
};


template<class T, class M> struct TypeListProductHelper {
    using type = core::TypeCons<std::pair<T, typename M::head>,
                                typename TypeListProductHelper<T, typename M::tail>::type>;
};
template<class T> struct TypeListProductHelper<T, void> {
    using type = void;
};

template<class L, class M> struct TypeListProduct {
    using type = core::TypeListConcat<typename TypeListProductHelper<typename L::head, M>::type,
                                      typename TypeListProduct<typename L::tail, M>::type>;
};
template<class M> struct TypeListProduct<void, M> {
    using type = void;
};


template<class L, class F, std::size_t I, class... A> struct ForEachTypeNoexcept {
    static constexpr bool exec() noexcept
    {
        return (noexcept(F::template exec<typename L::head, I>(std::declval<A>()...)) &&
                ForEachTypeNoexcept<typename L::tail, F, I + 1, A...>::exec());
    }
};
template<class F, std::size_t I, class... A> struct ForEachTypeNoexcept<void, F, I, A...> {
    static constexpr bool exec() noexcept
    {
        return true;
    }
};


template<class L, class T> struct HasType {
    static constexpr bool value = (std::is_same_v<typename L::head, T> || impl::HasType<typename L::tail, T>::value);
};
template<class T> struct HasType<void, T> {
    static constexpr bool value = false;
};


template<class L, class P, std::size_t I> struct HasTypeA {
    template<class... A> static constexpr bool exec(A&&... args)
    {
        return (P::template exec<typename L::head, I>(args...) ||
                HasTypeA<typename L::tail, P, I + 1>::exec(std::forward<A>(args)...)); // Throws
    }
};
template<class P, std::size_t I> struct HasTypeA<void, P, I> {
    template<class... A> static constexpr bool exec(A&&...) noexcept
    {
        return false;
    }
};


template<class L, class F, std::size_t I> struct ForEachType {
    template<class... A> static void exec(A&&... args)
    {
        F::template exec<typename L::head, I>(args...); // Throws
        ForEachType<typename L::tail, F, I + 1>::exec(std::forward<A>(args)...); // Throws
    }
    template<class... A> static bool exec_a(A&&... args)
    {
        if (ARCHON_LIKELY((F::template exec<typename L::head, I>(args...)))) { // Throws
            return ForEachType<typename L::tail, F, I + 1>::exec_a(std::forward<A>(args)...); // Throws
        }
        return false;
    }
};
template<class F, std::size_t I> struct ForEachType<void, F, I> {
    template<class... A> static void exec(A&&...) noexcept
    {
    }
    template<class... A> static bool exec_a(A&&...) noexcept
    {
        return true;
    }
};


} // namespace impl


template<class L, class P, class... A> constexpr bool has_type_a_noexcept() noexcept
{
    return impl::ForEachTypeNoexcept<L, P, 0, A...>::exec();
}


template<class L, class P, class... A> constexpr bool has_type_a(A&&... args)
    noexcept(core::has_type_a_noexcept<L, P, A...>())
{
    return impl::HasTypeA<L, P, 0>::exec(std::forward<A>(args)...); // Throws
}


template<class L, class F, class... A> constexpr bool for_each_type_noexcept() noexcept
{
    return impl::ForEachTypeNoexcept<L, F, 0, A...>::exec();
}


template<class L, class F, class... A> inline void for_each_type(A&&... args)
    noexcept(core::for_each_type_noexcept<L, F, A...>())
{
    impl::ForEachType<L, F, 0>::exec(std::forward<A>(args)...); // Throws
}


template<class L, class F, class... A> inline bool for_each_type_a(A&&... args)
    noexcept(core::for_each_type_noexcept<L, F, A...>())
{
    return impl::ForEachType<L, F, 0>::exec_a(std::forward<A>(args)...); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TYPE_LIST_HPP
