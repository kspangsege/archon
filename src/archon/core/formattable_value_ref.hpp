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

#ifndef ARCHON_X_CORE_X_FORMATTABLE_VALUE_REF_HPP
#define ARCHON_X_CORE_X_FORMATTABLE_VALUE_REF_HPP

/// \file


#include <ostream>


namespace archon::core {


/// \brief Type-erased reference to formattable value.
///
/// An object of this type stores a type-erased reference to a object of any formattable
/// value type, and it offerd the ability to subsequently format the referenced value (\ref
/// format()).
///
template<class C, class T = std::char_traits<C>> class BasicFormattableValueRef {
public:
    using ostream_type = std::basic_ostream<C, T>;

    /// \brief Construct a null reference.
    ///
    /// Construct a formattable value reference object that does not refer to anything. It
    /// is an error to invoke \ref format() on such an object.
    ///
    BasicFormattableValueRef() noexcept = default;

    /// \brief Construct a reference to a formattable value.
    ///
    /// Construct a formattable value reference object that refers to the specified
    /// formattable value.
    ///
    template<class V> explicit BasicFormattableValueRef(const V& value) noexcept;

    /// \brief Format the referenced value.
    ///
    /// Write a formatted version of the referenced value to the specified output stream.
    ///
    void format(ostream_type&) const;

    /// \brief Record references to a series of values.
    ///
    /// Form type-erased references to a series of value arguments of varying formattable
    /// types. The number of entries in the specied buffer must be greater than, or equal to
    /// the number of specified values (`sizeof... (V)`).
    ///
    template<class... V> static void record(BasicFormattableValueRef* buffer, const V&... values) noexcept;

private:
    using FormatFunc = void(ostream_type&, const void*);

    const void* m_value_ptr = 0;
    FormatFunc* m_format_func = 0;

    template<class V> static void do_format(ostream_type&, const void*);

    static void do_record(BasicFormattableValueRef* buffer) noexcept;

    template<class V, class... W> static void do_record(BasicFormattableValueRef* buffer, const V& value,
                                                        const W&... values) noexcept;
};


using FormattableValueRef     = BasicFormattableValueRef<char>;
using WideFormattableValueRef = BasicFormattableValueRef<wchar_t>;




/// \brief Format a formattable value.
///
/// Write a formatted version of the specified formattable value to the specified output
/// stream.
///
template<class C, class T> auto operator<<(std::basic_ostream<C, T>&, const BasicFormattableValueRef<C, T>&) ->
    std::basic_ostream<C, T>&;








// Implementation


template<class C, class T>
template<class V> inline BasicFormattableValueRef<C, T>::BasicFormattableValueRef(const V& value) noexcept
    : m_value_ptr(&value)
    , m_format_func(&BasicFormattableValueRef::do_format<V>)
{
}


template<class C, class T>
inline void BasicFormattableValueRef<C, T>::format(ostream_type& out) const
{
    (*m_format_func)(out, m_value_ptr); // Throws
}


template<class C, class T>
template<class... V>
inline void BasicFormattableValueRef<C, T>::record(BasicFormattableValueRef* buffer, const V&... values) noexcept
{
    do_record(buffer, values...);
}


template<class C, class T>
template<class V> void BasicFormattableValueRef<C, T>::do_format(ostream_type& out, const void* value_ptr)
{
    out << *static_cast<const V*>(value_ptr); // Throws
}


template<class C, class T>
inline void BasicFormattableValueRef<C, T>::do_record(BasicFormattableValueRef*) noexcept
{
}


template<class C, class T>
template<class V, class... W>
inline void BasicFormattableValueRef<C, T>::do_record(BasicFormattableValueRef* buffer, const V& value,
                                                      const W&... values) noexcept
{
    BasicFormattableValueRef& ref = *buffer;
    ref = BasicFormattableValueRef(value);
    do_record(buffer + 1, values...);
}


template<class C, class T>
inline auto operator<<(std::basic_ostream<C, T>& out, const BasicFormattableValueRef<C, T>& ref) ->
    std::basic_ostream<C, T>&
{
    ref.format(out); // Throws
    return out;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FORMATTABLE_VALUE_REF_HPP
