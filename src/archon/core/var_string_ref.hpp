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

#ifndef ARCHON_X_CORE_X_VAR_STRING_REF_HPP
#define ARCHON_X_CORE_X_VAR_STRING_REF_HPP

/// \file


#include <type_traits>
#include <utility>
#include <array>
#include <variant>
#include <string_view>
#include <string>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/char_mapper.hpp>


namespace archon::core {


/// \brief 
///
/// 
///
/// Variant string reference.    
///
template<class C, class T = std::char_traits<C>> class BasicVarStringRef {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;

    BasicVarStringRef() noexcept;
    template<class S> BasicVarStringRef(S&& string);

    /// \{
    ///
    /// \brief Non-throwing moce operation.
    ///
    /// Objects of this type are moveable, and the move operation does not throw.
    ///
    BasicVarStringRef(BasicVarStringRef&&) noexcept = default;
    auto operator=(BasicVarStringRef&&) noexcept -> BasicVarStringRef& = default;
    /// \}

    /// \{
    ///
    /// \brief Potentially throwning copy operation.
    ///
    /// Objects of this type are copyable.
    ///
    /// If an object of this type holds a string object, as opposed to a string reference,
    /// then the string object will be copied as part of copying a BasicVarStringRef
    /// object. The copy operation can therefore be costly, and it can involve dynamic
    /// allocation.
    ///
    BasicVarStringRef(const BasicVarStringRef&) = default;
    auto operator=(const BasicVarStringRef&) -> BasicVarStringRef& = default;
    /// \}

    bool get(std::string_view&, string_view_type&) const noexcept;

    /// \brief Copy "by reference".
    ///
    /// This function makes a copy of the variant string reference object, however, if the
    /// original object holds a string by value, the copy will hold a reference to that
    /// string value, rather than a copy of it.
    ///
    /// IMPORTANT: The copy may, or may not refer to memory owned by the original. It is
    /// therefore necessary that the original is kept alive for as long as the copy remains
    /// in use. The copy can be destroyed after destruction of the original, however.
    ///
    auto copy_by_ref() const noexcept -> BasicVarStringRef;

private:
    using string_type = std::basic_string<C, T>;

    template<class S> using mapped_type =
        std::conditional_t<std::is_lvalue_reference_v<S>,
                           std::conditional_t<std::is_convertible_v<S, std::string_view>,
                                              std::string_view,
                                              string_view_type>,
                           string_type>;

    static constexpr bool is_degenerate = std::is_same_v<string_view_type, std::string_view>;

    using variant_type = std::conditional_t<is_degenerate, std::variant<std::string_view, string_type>,
                                            std::variant<std::string_view, string_view_type, string_type>>;

    variant_type m_variant;
};


using VarStringRef     = BasicVarStringRef<char>;
using WideVarStringRef = BasicVarStringRef<wchar_t>;


template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, const BasicVarStringRef<C, T>&) ->
    std::basic_ostream<C, T>&;








// Implementation


template<class C, class T>
inline BasicVarStringRef<C, T>::BasicVarStringRef() noexcept
{
}


template<class C, class T>
template<class S> inline BasicVarStringRef<C, T>::BasicVarStringRef(S&& string)
    : m_variant(std::in_place_type_t<mapped_type<S>>(), std::forward<S>(string)) // Throws
{
}


template<class C, class T>
inline bool BasicVarStringRef<C, T>::get(std::string_view& a, string_view_type& b) const noexcept
{
    const std::string_view* ptr_1 = std::get_if<std::string_view>(&m_variant);
    if (ARCHON_LIKELY(ptr_1)) {
        a = *ptr_1;
        return true;
    }
    if constexpr (!is_degenerate) {
        const string_view_type* ptr_2 = std::get_if<string_view_type>(&m_variant);
        if (ARCHON_LIKELY(ptr_2)) {
            b = *ptr_2;
            return false;
        }
    }
    const string_type* ptr_3 = std::get_if<string_type>(&m_variant);
    ARCHON_ASSERT(ptr_3);
    b = *ptr_3;
    return false;
}


template<class C, class T>
inline auto BasicVarStringRef<C, T>::copy_by_ref() const noexcept -> BasicVarStringRef
{
    const std::string_view* ptr_1 = std::get_if<std::string_view>(&m_variant);
    if (ARCHON_LIKELY(ptr_1))
        return *ptr_1;
    if constexpr (!is_degenerate) {
        const string_view_type* ptr_2 = std::get_if<string_view_type>(&m_variant);
        if (ARCHON_LIKELY(ptr_2))
            return *ptr_2;
    }
    const string_type* ptr_3 = std::get_if<string_type>(&m_variant);
    ARCHON_ASSERT(ptr_3);
    return *ptr_3;
}


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const BasicVarStringRef<C, T>& str) -> std::basic_ostream<C, T>&
{
    using char_type = C;
    using string_view_type = std::basic_string_view<C, T>;
    using char_mapper_type = core::BasicCharMapper<C, T>;
    constexpr bool is_degenerate = std::is_same_v<string_view_type, std::string_view>;
    std::string_view str_1;
    string_view_type str_2;
    bool not_wide = str.get(str_1, str_2);
    if constexpr (is_degenerate) {
        ARCHON_ASSERT(not_wide);
        return out << str_1; // Throws
    }
    constexpr std::size_t buffer_size = 256;
    std::array<char_type, buffer_size> buffer;
    if (ARCHON_LIKELY(not_wide)) {
        std::locale locale = out.getloc(); // Throws
        char_mapper_type char_mapper(locale); // Throws
        while (str_1.size() > buffer_size) {
            char_mapper.widen(str_1.substr(0, buffer_size), buffer.data()); // Throws
            out << string_view_type(buffer.data(), buffer_size); // Throws
            str_1 = str_1.substr(buffer_size);
        }
        char_mapper.widen(str_1, buffer.data()); // Throws
        str_2 = { buffer.data(), str_1.size() };
    }
    return out << str_2; // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_VAR_STRING_REF_HPP
