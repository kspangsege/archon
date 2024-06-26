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

#ifndef ARCHON_X_CORE_X_TEST_X_LOCALE_UTILS_HPP
#define ARCHON_X_CORE_X_TEST_X_LOCALE_UTILS_HPP


#include <cwchar>
#include <array>
#include <vector>
#include <locale>

#include <archon/core/assert.hpp>
#include <archon/core/impl/codecvt_quirks.hpp>


namespace archon::core::test {


class CandidateLocales {
public:
    auto begin() const noexcept;
    auto end() const noexcept;

    CandidateLocales();

private:
    std::vector<std::locale> m_locales;
};


// This function must only be called after the invocation of `main()`.
//
auto get_candidate_locales() -> const CandidateLocales&;


// Try to find a byte that causes a decode error / a character that cuases an encode error.
//
template<class C> bool find_decode_error(const std::locale&, char& ch, bool followed_by_star = false);
template<class C> bool find_encode_error(const std::locale&, C& ch);








// Implementation


inline auto CandidateLocales::begin() const noexcept
{
    return m_locales.begin();
}


inline auto CandidateLocales::end() const noexcept
{
    return m_locales.end();
}


namespace impl {

// Due to the non-static initialization, this global variable must only be accessed after
// the invocation of `main()`.
//
// The initialization needs to be done early because of race-conditions / data races in
// `core::has_locale()` and the construction of locale objects from locale names (see
// `core::has_locale()`).
//
inline CandidateLocales g_candidate_locales;

} // namespace impl


inline auto get_candidate_locales() -> const CandidateLocales&
{
    return impl::g_candidate_locales;
}


template<class C> inline bool find_decode_error(const std::locale& locale, char& ch, bool followed_by_star)
{
    using char_type = C;
    using codecvt_type = std::codecvt<char_type, char, std::mbstate_t>;
    const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);

    // If both the "partial result instead of ok result" and "partial result instead of
    // error result" quirks were present, we would not know whether "partial" means "ok" or
    // "error".
    static_assert(!(core::impl::codecvt_quirk_partial_result_on_partial_char &&
                    core::impl::codecvt_quirk_partial_result_on_invalid_byte_seq));

    std::array candidates = {
        char(-1),
    };
    for (char bad_char : candidates) {
        std::array data { bad_char, '*' };
        std::array<char_type, 2> buffer;
        std::mbstate_t state = {};
        const char* from = data.data();
        const char* from_end = from + (followed_by_star ? 2 : 1);
        const char* from_next = {};
        char_type* to = buffer.data();
        char_type* to_end = to + buffer.size();
        char_type* to_next = {};
        std::codecvt_base::result result = codecvt.in(state, from, from_end, from_next, to, to_end, to_next);
        switch (result) {
            case std::codecvt_base::ok:
            case std::codecvt_base::noconv:
                break;
            case std::codecvt_base::partial:
                if constexpr (core::impl::codecvt_quirk_partial_result_on_partial_char) {
                    ARCHON_ASSERT(to_next != to_end);
                    break;
                }
                else if constexpr (core::impl::codecvt_quirk_partial_result_on_invalid_byte_seq) {
                    ARCHON_ASSERT(to_next != to_end);
                }
                else {
                    ARCHON_ASSERT(false);
                    break;
                }
                [[fallthrough]];
            case std::codecvt_base::error:
                if (from_next == from) {
                    ch = bad_char;
                    return true;
                }
                break;
        }
    }
    return false;
}


template<class C> inline bool find_encode_error(const std::locale& locale, C& ch)
{
    using char_type = C;
    using traits_type = std::char_traits<C>;
    using codecvt_type = std::codecvt<char_type, char, std::mbstate_t>;
    const codecvt_type& codecvt = std::use_facet<codecvt_type>(locale);

    std::array candidates = {
        char_type(-1),
        traits_type::to_char_type(0xD800),
        traits_type::to_char_type(0xDC00),
    };
    for (char_type bad_char : candidates) {
        std::array data { bad_char };
        std::array<char, 1> buffer;

        std::mbstate_t state = {};
        const char_type* from = data.data();
        const char_type* from_end = from + data.size();
        const char_type* from_next = nullptr;
        char* to = buffer.data();
        char* to_end = to + buffer.size();
        char* to_next = nullptr;
        std::codecvt_base::result result = codecvt.out(state, from, from_end, from_next, to, to_end, to_next);
        switch (result) {
            case std::codecvt_base::ok:
            case std::codecvt_base::partial:
            case std::codecvt_base::noconv:
                break;
            case std::codecvt_base::error:
                ch = bad_char;
                return true;
        }
    }
    return false;
}


} // namespace archon::core::test

#endif // ARCHON_X_CORE_X_TEST_X_LOCALE_UTILS_HPP
