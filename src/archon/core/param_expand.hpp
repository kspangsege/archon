/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef ARCHON_CORE_PARAM_SUBST_HPP
#define ARCHON_CORE_PARAM_SUBST_HPP

/// \file
///
/// \author Kristian Spangsege

#include <string>
#include <sstream>
#include <ostream>

#include <archon/features.h>
#include <archon/core/tuple.hpp>

namespace archon {
namespace Core {


#ifdef TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

/// Same as passing the parameter pack as a tuple to
/// param_expand_tuple().
template<class Char, class Traits, class... Params>
void param_expand(std::basic_ostream<Char, Traits>&, const char* pattern, Params... params);

#else // ! TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

/// Same as passing an empty tuple to param_expand_tuple().
template<class Char, class Traits>
void param_expand(std::basic_ostream<Char, Traits>&, const char* pattern);

/// Same as passing a single paramaeter to param_expand_tuple().
template<class Char, class Traits, class T1>
void param_expand(std::basic_ostream<Char, Traits>&, const char* pattern, const T1&);

/// Same as passing a pair of parameters to param_expand_tuple().
template<class Char, class Traits, class T1, class T2>
void param_expand(std::basic_ostream<Char, Traits>&, const char* pattern, const T1&, const T2&);

/// Same as passing a triple of parameters to param_expand_tuple().
template<class Char, class Traits, class T1, class T2, class T3>
void param_expand(std::basic_ostream<Char, Traits>&, const char* pattern, const T1&, const T2&,
                  const T3&);

/// Same as passing a quadruple of parameters to param_expand_tuple().
template<class Char, class Traits, class T1, class T2, class T3, class T4>
void param_expand(std::basic_ostream<Char, Traits>&, const char* pattern, const T1&, const T2&,
                  const T3&, const T4&);

#endif // ! TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

/// Write the specified pattern to the specified output stream while
/// substituting the specified parameters for the corresponding
/// parameter markers in the pattern.
///
/// The specified pattern is scanned from left to right for
/// occurrences of the character `%` that are followed immediately by
/// a digit (ASCII characters `0` -> `9`) or another `%`. When a `%`
/// is followed by a digit, it introduces a parameter marker, and when
/// it is followed by another `%`, the pair is replaced by a single
/// `%`.
///
/// A parameter marker consists of the introducing `%`, all the digits
/// that follows it immediately, and an optional closing `;` following
/// immediately after the digits. Note that the closing `;` is
/// required when the marker needs to be followed by a literal digit
/// or `;` in the pattern.
///
/// The numeric part of a parameter marker selects the parameter that
/// is to be substituted for the marker. A value of one selects the
/// first of the specified parameters, a value of two selects the
/// second one, and so on. A marker is invalid if the value zero or
/// greater than the number of specified parameters. Invalid markers
/// will cause an exception to be thrown. It is not an error to have
/// multiple markers that select the same parameter in a single
/// pattern.
///
/// The pattern must consist entirely of characters from the portable
/// character set (see CharEnc). Each parameter `p` is written to the
/// specified stream using an expression of the form `out << p`, so a
/// particular parameter type is allowed if, and only if an applicable
/// `<<` operator exists.
template<class Char, class Traits, class Types>
void param_expand_tuple(std::basic_ostream<Char, Traits>& out, const char* pattern,
                        const Tuple<Types>& params);




// Implementation:

namespace _impl {

template<class Char, class Traits> class ParamExpander {
public:
    typedef std::basic_ostream<Char, Traits> ostream;
    typedef void (*expand_func)(ostream&, const void* value_ptr);

    struct Param {
        expand_func m_expand_func;
        const void* m_value_ptr;
    };

    ParamExpander(Param* param_buffer):
        m_param_buffer(param_buffer),
        m_num_params(0)
    {
    }

    template<class T> void add_param(const T& param)
    {
        Param& p = m_param_buffer[m_num_params];
        p.m_expand_func = &expand_param<T>;
        p.m_value_ptr = &param;
        ++m_num_params;
    }

#ifdef TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

    static void add_params()
    {
    }

    template<class T, class... Params> void add_params(const T& param, Params... params)
    {
        add_param<T>(param);
        add_params(params...);
    }

#endif // TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

    template<class T> struct AddParam {
        void operator()(const T& param, ParamExpander* expander)
        {
            expander->add_param<T>(param);
        }
    };

    void expand(ostream& out, const char* pattern)
    {
        const char* p = pattern;
        for (;;) {
            // Find next parameter marker
            for (;;) {
                char c = *p;
                if (c == 0)
                    return; // End of pattern
                ++p;
                if (c == '%') {
                    if (*p >= '0' && *p <= '9')
                        break; // Got `%` followed by digit
                    if (*p == '%')
                        break; // Got `%%`
                }
                out.put(out.widen(c));
            }
            if (*p == '%') {
                // `%%` -> `%`
                out.put(out.widen('%'));
                ++p;
                continue;
            }
            std::size_t param_ord = 0;
            bool overflow = false;
            for (;;) {
                std::size_t digit = *p - '0';
                if (digit > m_num_params - param_ord)
                    overflow = true;
                param_ord += digit;
                ++p;
                if (*p < '0' || *p > '9')
                    break;
                if (param_ord > m_num_params / 10)
                    overflow = true;
                param_ord *= 10;
            }
            if (param_ord == 0 || overflow)
                throw std::runtime_error("Parameter marker out of range");
            const Param& param = m_param_buffer[param_ord-1];
            (*param.m_expand_func)(out, param.m_value_ptr);
            if (*p == ';')
                ++p;
        }
    }

private:
    Param* const m_param_buffer;
    std::size_t m_num_params;

    template<class T> static void expand_param(ostream& out, const void* value_ptr)
    {
        const T& value = *static_cast<const T*>(value_ptr);
        out << value;
    }
};

} // namespace _impl


#ifdef TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

template<class Char, class Traits, class... Params>
inline void param_expand(std::basic_ostream<Char, Traits>& out, const char* pattern, Params... params)
{
    typedef _impl::ParamExpander<Char, Traits> Expander;
    typename Expander::Param param_buffer[sizeof... Params];
    Expander expander(param_buffer);
    expander.add_params(params);
    expander.expand(out, pattern);
}

#else // ! TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

template<class Char, class Traits>
inline void param_expand(std::basic_ostream<Char, Traits>& out, const char* pattern)
{
    param_expand_tuple(out, pattern, tuple());
}

template<class Char, class Traits, class T1>
inline void param_expand(std::basic_ostream<Char, Traits>& out, const char* pattern, const T1& a)
{
    param_expand_tuple(out, pattern, tuple(a));
}

template<class Char, class Traits, class T1, class T2>
inline void param_expand(std::basic_ostream<Char, Traits>& out, const char* pattern, const T1& a,
                         const T2& b)
{
    param_expand_tuple(out, pattern, tuple(a,b));
}

template<class Char, class Traits, class T1, class T2, class T3>
inline void param_expand(std::basic_ostream<Char, Traits>& out, const char* pattern, const T1& a,
                         const T2& b, const T3& c)
{
    param_expand_tuple(out, pattern, tuple(a,b,c));
}

template<class Char, class Traits, class T1, class T2, class T3, class T4>
inline void param_expand(std::basic_ostream<Char, Traits>& out, const char* pattern, const T1& a,
                         const T2& b, const T3& c, const T4& d)
{
    param_expand_tuple(out, pattern, tuple(a,b,c,d));
}

#endif // ! TIGHTDB_HAVE_CXX11_VARIADIC_TEMPLATES

template<class Char, class Traits, class Types>
inline void param_expand_tuple(std::basic_ostream<Char, Traits>& out, const char* pattern,
                               const Tuple<Types>& params)
{
    typedef _impl::ParamExpander<Char, Traits> Expander;
    typename Expander::Param param_buffer[TypeCount<Types>::value];
    Expander expander(param_buffer);
    for_each<Expander::template AddParam>(params, &expander);
    expander.expand(out, pattern);
}

} // namespace Core
} // namespace archon

#endif // ARCHON_CORE_PARAM_SUBST_HPP
