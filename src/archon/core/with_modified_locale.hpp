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

#ifndef ARCHON_X_CORE_X_WITH_MODIFIED_LOCALE_HPP
#define ARCHON_X_CORE_X_WITH_MODIFIED_LOCALE_HPP

/// \file


#include <utility>
#include <locale>
#include <ostream>

#include <archon/core/scope_exit.hpp>
#include <archon/core/value_parser.hpp>


namespace archon::core {


template<class R> auto with_modified_locale(R&& ref, const std::locale&, std::locale::category);
template<class R> auto with_reverted_locale(R&& ref, std::locale::category);
template<class R> auto with_reverted_numerics(R&& ref);








// Implementation


namespace impl {


template<class R> struct WithModifiedLocale {
    R ref;
    const std::locale& loc;
    std::locale::category cat;
};


template<class C, class T, class R>
inline auto with_modified_locale(std::basic_ostream<C, T>& out, const std::locale& loc, std::locale::category cat,
                                 const R& ref) -> std::basic_ostream<C, T>&
{
    std::locale orig_loc = out.getloc(); // Throws
    if constexpr (noexcept(out.imbue(orig_loc))) {
        ARCHON_SCOPE_EXIT {
            out.imbue(orig_loc);
        };
        out.imbue(std::locale(orig_loc, loc, cat)); // Throws
        out << ref; // Throws
    }
    else {
        std::basic_ostream<C, T> out_2(out.rdbuf()); // Throws
        out_2.copyfmt(out); // Throws
        out_2.imbue(std::locale(orig_loc, loc, cat)); // Throws
        out_2.clear(out.rdstate()); // Throws
        out_2 << ref; // Throws
        out.clear(out_2.rdstate()); // Throws
    }
    return out;
}


template<class C, class T, class R>
auto operator<<(std::basic_ostream<C, T>& out, const impl::WithModifiedLocale<R>& pod) -> std::basic_ostream<C, T>&
{
    return impl::with_modified_locale(out, pod.loc, pod.cat, pod.ref); // Throws
}


template<class C, class T, class R>
bool parse_value(core::BasicValueParserSource<C, T>& src, const impl::WithModifiedLocale<R>& pod)
{
    return src.with_modified_locale(pod.loc, pod.cat, pod.ref); // Throws
}


} // namespace impl


template<class R> inline auto with_modified_locale(R&& ref, const std::locale& loc, std::locale::category cat)
{
    return impl::WithModifiedLocale<R> { std::forward<R>(ref), loc, cat }; // Throws
}


template<class R> inline auto with_reverted_locale(R&& ref, std::locale::category cat)
{
    return with_modified_locale(std::forward<R>(ref), std::locale::classic(), cat); // Throws
}


template<class R> inline auto with_reverted_numerics(R&& ref)
{
    return with_reverted_locale(std::forward<R>(ref), std::locale::numeric); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_WITH_MODIFIED_LOCALE_HPP
