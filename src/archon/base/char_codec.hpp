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

#ifndef ARCHON_X_BASE_X_CHAR_CODEC_HPP
#define ARCHON_X_BASE_X_CHAR_CODEC_HPP

/// \file


#include <cwchar>
#include <locale>


namespace archon::base {


template<class C, class T = std::char_traits<C>> class BasicCharCodec {
public:
    BasicCharCodec(std::locale, bool lenient = false);

    auto inc_decode(std::mbstate_t&, const char*& ext_begin, const char* ext_end,
                    C*& int_begin, C* int_end, bool flush) -> std::codecvt_base::result;

    bool inc_decode_valid_end(const std::mbstate_t&, const char* ext_begin,
                              const char* ext_end) noexcept;

private:
    using codecvt_type = std::codecvt<C, char, std::mbstate_t>;

    const std::locale m_locale;
    const codecvt_type& m_codecvt;
    const bool m_stateful = (m_codecvt.encoding() == -1);
    const bool m_lenient;
};








// Implementation


template<class C, class T>
inline BasicCharCodec<C, T>::BasicCharCodec(std::locale locale, bool lenient) :
    m_locale(locale),
    m_codecvt(std::use_facet<codecvt_type>(m_locale)), // Throws
    m_lenient(lenient)
{
}


template<class C, class T>
auto BasicCharCodec<C, T>::inc_decode(std::mbstate_t& state, const char*& ext_begin,
                                      const char* ext_end, C*& int_begin, C* int_end,
                                      bool flush) -> std::codecvt_base::result
{
    std::codecvt_base::result result = m_codecvt.in(state, ext_begin, ext_end, ext_begin,
                                                    int_begin, int_end, int_begin); // Throws
    bool nontrivial = (result == std::codecvt_base::ok ||
                       (result == std::codecvt_base::partial && int_begin != int_end));
    if (ARCHON_LIKELY(!nontrivial))
        return result;
    if (ARCHON_LIKELY(!flush || this->inc_decode_valid_end(state, ext_begin, ext_end)))
        return std::codecvt_base::ok;
    return std::codecvt_base::error;
}


template<class C, class T>
inline bool BasicCharCodec<C, T>::inc_decode_valid_end(const std::mbstate_t& state,
                                                       const char* ext_begin,
                                                       const char* ext_end) noexcept
{
    return (ext_begin == ext_end && (m_stateful || std::mbsinit(&state)));
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_CHAR_CODEC_HPP
