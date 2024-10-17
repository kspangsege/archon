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

#ifndef ARCHON_X_CORE_X_ENUM_HPP
#define ARCHON_X_CORE_X_ENUM_HPP

/// \file


#include <type_traits>
#include <algorithm>
#include <string_view>
#include <map>
#include <stdexcept>
#include <locale>
#include <istream>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/stream_input.hpp>
#include <archon/core/as_int.hpp>


namespace archon::core {


/// \brief Extend enum type with format and parse capabilities.
///
/// This template class allows you to endow a fundamental `enum` type with information about
/// how to print out the individual values, and how to parse them.
///
/// Here is an example:
///
/// \code{.cpp}
///
///   enum class Color { orange, purple, brown };
///
///   struct ColorSpec {
///       static constexpr archon::core::EnumAssoc map[] = {
///           { int(Color::orange), "orange" },
///           { int(Color::purple), "purple" },
///           { int(Color::brown),  "brown"  }
///       };
///   };
///   using ColorEnum = archon::core::Enum<Color, ColorSpec>;
///
///   // Application
///
///   ColorEnum color = Color::purple;
///
///   std::cin  >> color;  // Read a color
///   std::cout << color;  // Write a color
///
/// \endcode
///
/// See also \ref EnumTraits, for a way to avoid having to explicitly wrap enumeration
/// values in \ref Enum objects.
///
/// The implementation assumes that all characters used in item names can be individually
/// widened and narrowed using `std::ctype<C>::widen()` and `std::ctype<C>::narrow()` for
/// any character type `C`. In particular, it assumes that no multibyte characters are
/// used. For maximum portability, item names should consist only of characters from the
/// basic characterset, as defined by the C standard.
///
/// The current implementation is restricted to enumeration types whose values can all be
/// represented in a regular integer, i.e., in an object of type `int`.
///
/// \tparam E An enumeration type.
///
/// \tparam S A class type with a public static data member named `map` that can be
/// implicitly converted to `core::Span<const core::EnumAssoc>`, such as `const EnumAssoc
/// [N]` for some integer `N`. It is assumed that the entries in `map` remain constant so
/// that they can be accessed at any time without leading to data races.
///
/// \tparam ignore_case If `true`, letter case is ignored while reading values from input
/// streams.
///
template<class E, class S, bool ignore_case = false> class Enum {
public:
    static_assert(std::is_enum_v<E>);

    using base_enum_type = E;

    Enum(E = {}) noexcept;

    operator E() const noexcept;

    auto name() const -> std::string_view;

    bool name(std::string_view&) const;

    /// \return True if, and only if successful.
    ///
    static bool parse(std::string_view string, E& value);

private:
    E m_value = {};
};




template<class C, class T, class E, class S, bool ignore_case>
auto operator<<(std::basic_ostream<C, T>&, const core::Enum<E, S, ignore_case>&) -> std::basic_ostream<C, T>&;

template<class C, class T, class E, class S, bool ignore_case>
auto operator>>(std::basic_istream<C, T>&, core::Enum<E, S, ignore_case>&) -> std::basic_istream<C, T>&;




/// \brief Enumeration value/name association entry.
///
/// A value/name association entry for specifying how to read and write enumeration values
/// from input streams and to output streams. See \ref Enum.
///
struct EnumAssoc {
    int value;
    const char* name;
};




/// \brief Enable formatting and parsing of values of fundamental enum types.
///
/// An application that uses \ref core::Enum with an enumeration type, can choose to also
/// specialize \ref EnumTraits for that enumeration type. Doing so, makes it possible to
/// read and write enumeration values directly without needing to explicitly wrap them in
/// \ref Enum objects.
///
/// A specialization must define at least these members:
///
/// - A static constant boolean data member named `is_specialized` with value `true`.
///
/// - A type member named `Spec` which will be used as the second template argument (`S`) to
///   \ref Enum.
///
/// - A static constant boolean data member named `ignore_case` which will be used as the
///   third template argument (`S`) to \ref Enum.
///
/// Here is an example:
///
/// \code{.cpp}
///
///   enum class Color { orange, purple, brown };
///
///   namespace archon::core {
///
///   template<> struct EnumTraits<Color> {
///       static constexpr bool is_specialized = true;
///       struct Spec {
///           static constexpr EnumAssoc map[] = {
///               { int(Color::orange), "orange" },
///               { int(Color::purple), "purple" },
///               { int(Color::brown),  "brown"  }
///           };
///       };
///       static constexpr bool ignore_case = false;
///   };
///
///   } // namespace archon::core
///
///   // Application
///
///   Color color = Color::purple;
///
///   std::cin  >> color;  // Read a color
///   std::cout << color;  // Write a color
///
/// \endcode
///
/// Note that the specialization needs to happen in namespace `archon::core`.
///
template<class E> struct EnumTraits {
    static constexpr bool is_specialized = false;
};








// Implementation


namespace impl {


template<bool ignore_case> class EnumMapper {
public:
    EnumMapper(core::Span<const core::EnumAssoc> map)
    {
        for (const core::EnumAssoc& entry : map) {
            std::string_view name = entry.name;
            auto p_1 = m_value_to_name.emplace(entry.value, name); // Throws
            bool was_inserted_1 = p_1.second;
            if (!was_inserted_1)
                throw std::invalid_argument("Duplicate enum item value");
            auto p_2 = m_name_to_value.emplace(name, entry.value); // Throws
            bool was_inserted_2 = p_2.second;
            if (!was_inserted_2)
                throw std::invalid_argument("Duplicate enum item name");
        }
    }

    bool name(int value, std::string_view& string) const noexcept
    {
        auto i = m_value_to_name.find(value);
        if (i == m_value_to_name.end())
            return false;
        string = i->second;
        return true;
    }

    bool parse(std::string_view string, int& value) const noexcept
    {
        auto i = m_name_to_value.find(string);
        if (i == m_name_to_value.end())
            return false;
        value = i->second;
        return true;
    }

private:
    struct Compare {
        using ctype_type = std::ctype<char>;
        const ctype_type& m_ctype = std::use_facet<ctype_type>(std::locale::classic());
        bool operator()(std::string_view a, std::string_view b) const noexcept
        {
            auto cmp = [&](char lhs, char rhs) {
                return (m_ctype.toupper(lhs) < m_ctype.toupper(rhs));
            };
            if (ignore_case)
                return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), cmp);
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
        }
    };

    std::map<int, std::string_view> m_value_to_name;
    std::map<std::string_view, int, Compare> m_name_to_value;
};


template<class S, bool ignore_case> auto get_enum_mapper() -> const impl::EnumMapper<ignore_case>&
{
    static impl::EnumMapper<ignore_case> mapper(S::map); // Throws
    return mapper;
}


template<class E> using NeedEnum = std::enable_if_t<EnumTraits<E>::is_specialized>;


} // namespace impl


template<class E, class S, bool ignore_case>
inline Enum<E, S, ignore_case>::Enum(E value) noexcept
    : m_value(value)
{
}


template<class E, class S, bool ignore_case>
inline Enum<E, S, ignore_case>::operator E() const noexcept
{
    return m_value;
}


template<class E, class S, bool ignore_case>
inline auto Enum<E, S, ignore_case>::name() const -> std::string_view
{
    std::string_view string;
    if (name(string))
        return string;
    throw std::out_of_range("Enum value");
}


template<class E, class S, bool ignore_case>
inline bool Enum<E, S, ignore_case>::name(std::string_view& string) const
{
    const auto& mapper = impl::get_enum_mapper<S, ignore_case>(); // Throws
    return mapper.name(int(m_value), string);
}


template<class E, class S, bool ignore_case>
inline bool Enum<E, S, ignore_case>::parse(std::string_view name, E& value)
{
    const auto& mapper = impl::get_enum_mapper<S, ignore_case>(); // Throws
    int value_2 = 0;
    if (ARCHON_LIKELY(mapper.parse(name, value_2))) {
        value = E(value_2);
        return true;
    }
    return false;
}


template<class C, class T, class E, class S, bool ignore_case>
inline auto operator<<(std::basic_ostream<C, T>& out, const core::Enum<E, S, ignore_case>& e) ->
    std::basic_ostream<C, T>&
{
    std::string_view string;
    if (ARCHON_LIKELY(e.name(string))) {
        // Was initialized from null-terminated string
        const char* c_str = string.data();
        out << c_str;
    }
    else {
        out << core::as_int(int(E(e)));
    }
    return out;
}


template<class C, class T, class E, class S, bool ignore_case>
auto operator>>(std::basic_istream<C, T>& in, core::Enum<E, S, ignore_case>& e) -> std::basic_istream<C, T>&
{
    return core::istream_sentry(in, [&](core::BasicStreamInputHelper<C, T>& helper) {
        std::locale loc = in.getloc();
        const std::ctype<C>& ctype = std::use_facet<std::ctype<C>>(loc);
        using char_mapper_type = core::BasicCharMapper<C, T>;
        char_mapper_type char_mapper(ctype);
        C underscore(char_mapper.widen('_')); // Throws
        core::ArraySeededBuffer<C, 128> buffer;
        std::size_t size = 0;
        C ch;
        if (ARCHON_UNLIKELY(!helper.peek(ch)))
            return false;
        for (;;) {
            if (ARCHON_UNLIKELY(!ctype.is(std::ctype_base::alnum, ch) && ch != underscore)) {
                if (ARCHON_LIKELY(size > 0))
                    break;
                return false;
            }
            buffer.append_a(ch, size); // Throws
            if (ARCHON_UNLIKELY(!helper.next(ch)))
                break;
        }
        std::basic_string_view<C, T> string = { buffer.data(), size };
        typename char_mapper_type::template ArraySeededNarrowBuffer<128> narrow_buffer;
        std::string_view string_2 = char_mapper.narrow(string, '\0', narrow_buffer); // Throws
        E value = {};
        bool success = core::Enum<E, S, ignore_case>::parse(string_2, value); // Throws
        if (ARCHON_UNLIKELY(!success))
            return false;
        e = value;
        return true;
    });
}


} // namespace archon::core


namespace std {


template<class C, class T, class E, class = archon::core::impl::NeedEnum<E>>
inline auto operator<<(std::basic_ostream<C, T>& out, const E& e) -> std::basic_ostream<C, T>&
{
    using U = typename archon::core::EnumTraits<E>;
    archon::core::Enum<E, typename U::Spec, U::ignore_case> f(e);
    out << f; // Throws
    return out;
}


template<class C, class T, class E, class = archon::core::impl::NeedEnum<E>>
inline auto operator>>(std::basic_istream<C, T>& in, E& e) -> std::basic_istream<C, T>&
{
    using U = typename archon::core::EnumTraits<E>;
    archon::core::Enum<E, typename U::Spec, U::ignore_case> f;
    in >> f; // Throws
    if (in)
        e = f;
    return in;
}


} // namespace std

#endif // ARCHON_X_CORE_X_ENUM_HPP
