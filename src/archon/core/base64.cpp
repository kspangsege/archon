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


#include <archon/core/base64.hpp>


using namespace archon;
using core::base64::IncrementalEncoder;
using core::base64::IncrementalDecoder;


namespace {


constexpr char g_base64_chars[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};


constexpr char g_base64url_chars[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
};


constexpr int get_max_char() noexcept
{
    int max = std::numeric_limits<int>::min();
    char chars[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
        '-', '_', '=', ' ', '\t', '\n'
    };
    for (int i = 0; i < int(sizeof chars); ++i) {
        using uchar = unsigned char;
        int val = uchar(chars[i]);
        if (val > max)
            max = val;
    }
    return max;
}


constexpr auto get_lookup_table() noexcept
{
    constexpr int max = get_max_char();
    static_assert(max < 256);
    constexpr int size = max + 1;
    std::array<std::uint_least8_t, size> table = {};
    for (int i = 0; i < size; ++i)
        table[i] = 66;
    char chars[62] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
    };
    using uchar = unsigned char;
    for (int i = 0; i < 62; ++i) {
        int val = uchar(chars[i]);
        table[val] = std::uint_least8_t(i);
    }
    table[uchar('+')]  = 0b10000000 + 62; // Special
    table[uchar('-')]  = 0b11000000 + 62; // Special + URL safe
    table[uchar('/')]  = 0b10000000 + 63; // Special
    table[uchar('_')]  = 0b11000000 + 63; // Special + URL safe
    table[uchar('=')]  =              64; // Padding
    table[uchar(' ')]  =              65; // Whitespace
    table[uchar('\t')] =              65; // Whitespace
    table[uchar('\n')] =              65; // Whitespace
    return table;
}


auto decode_char(char ch) noexcept -> std::uint_least8_t
{
    constexpr int max = get_max_char();
    if constexpr (max < 256) {
        static constexpr auto table = get_lookup_table();
        using uchar = unsigned char;
        int val = uchar(ch);
        if (val <= max)
            return table[val];
    }
    else {
        switch (ch) {
            case 'A':
                return std::uint_least8_t(0);
            case 'B':
                return std::uint_least8_t(1);
            case 'C':
                return std::uint_least8_t(2);
            case 'D':
                return std::uint_least8_t(3);
            case 'E':
                return std::uint_least8_t(4);
            case 'F':
                return std::uint_least8_t(5);
            case 'G':
                return std::uint_least8_t(6);
            case 'H':
                return std::uint_least8_t(7);
            case 'I':
                return std::uint_least8_t(8);
            case 'J':
                return std::uint_least8_t(9);
            case 'K':
                return std::uint_least8_t(10);
            case 'L':
                return std::uint_least8_t(11);
            case 'M':
                return std::uint_least8_t(12);
            case 'N':
                return std::uint_least8_t(13);
            case 'O':
                return std::uint_least8_t(14);
            case 'P':
                return std::uint_least8_t(15);
            case 'Q':
                return std::uint_least8_t(16);
            case 'R':
                return std::uint_least8_t(17);
            case 'S':
                return std::uint_least8_t(18);
            case 'T':
                return std::uint_least8_t(19);
            case 'U':
                return std::uint_least8_t(20);
            case 'V':
                return std::uint_least8_t(21);
            case 'W':
                return std::uint_least8_t(22);
            case 'X':
                return std::uint_least8_t(23);
            case 'Y':
                return std::uint_least8_t(24);
            case 'Z':
                return std::uint_least8_t(25);
            case 'a':
                return std::uint_least8_t(26);
            case 'b':
                return std::uint_least8_t(27);
            case 'c':
                return std::uint_least8_t(28);
            case 'd':
                return std::uint_least8_t(29);
            case 'e':
                return std::uint_least8_t(30);
            case 'f':
                return std::uint_least8_t(31);
            case 'g':
                return std::uint_least8_t(32);
            case 'h':
                return std::uint_least8_t(33);
            case 'i':
                return std::uint_least8_t(34);
            case 'j':
                return std::uint_least8_t(35);
            case 'k':
                return std::uint_least8_t(36);
            case 'l':
                return std::uint_least8_t(37);
            case 'm':
                return std::uint_least8_t(38);
            case 'n':
                return std::uint_least8_t(39);
            case 'o':
                return std::uint_least8_t(40);
            case 'p':
                return std::uint_least8_t(41);
            case 'q':
                return std::uint_least8_t(42);
            case 'r':
                return std::uint_least8_t(43);
            case 's':
                return std::uint_least8_t(44);
            case 't':
                return std::uint_least8_t(45);
            case 'u':
                return std::uint_least8_t(46);
            case 'v':
                return std::uint_least8_t(47);
            case 'w':
                return std::uint_least8_t(48);
            case 'x':
                return std::uint_least8_t(49);
            case 'y':
                return std::uint_least8_t(50);
            case 'z':
                return std::uint_least8_t(51);
            case '0':
                return std::uint_least8_t(52);
            case '1':
                return std::uint_least8_t(53);
            case '2':
                return std::uint_least8_t(54);
            case '3':
                return std::uint_least8_t(55);
            case '4':
                return std::uint_least8_t(56);
            case '5':
                return std::uint_least8_t(57);
            case '6':
                return std::uint_least8_t(58);
            case '7':
                return std::uint_least8_t(59);
            case '8':
                return std::uint_least8_t(60);
            case '9':
                return std::uint_least8_t(61);
            case '+':
                return std::uint_least8_t(0b10000000 + 62);
            case '-':
                return std::uint_least8_t(0b11000000 + 62);
            case '/':
                return std::uint_least8_t(0b10000000 + 63);
            case '_':
                return std::uint_least8_t(0b11000000 + 63);
            case '=':
                return std::uint_least8_t(64);
            case ' ':
            case '\t':
            case '\n':
                return std::uint_least8_t(65);
        }
    }
    return std::uint_least8_t(66);
}


} // unnamed namespace


bool IncrementalEncoder::encode(const char*& data_begin, const char* data_end, bool end_of_input,
                                char*& buffer_begin, char* buffer_end) noexcept
{
    ARCHON_ASSERT(m_hold_buffer.size() >= 4);
    ARCHON_ASSERT(m_holding_output || m_holding_size < 3);

    using uchar = unsigned char;
    using int_type = std::int_fast32_t;
    const char* chars = (m_config.url_safe_variant ? g_base64url_chars : g_base64_chars);

    if (m_holding_output)
        goto output;

  input:
    ARCHON_ASSERT(!m_holding_output);
    if (ARCHON_LIKELY(data_begin != data_end)) {
        char ch = *data_begin;
        if (ARCHON_LIKELY(core::int_less_equal(uchar(ch), 255))) {
            ++data_begin;
            int i = m_holding_size++;
            m_hold_buffer[i] = ch;
            if (ARCHON_LIKELY(m_holding_size < 3))
                goto input;
            goto batch;
        }
        return false;
    }
    if (ARCHON_LIKELY(!end_of_input))
        return false;
    if (ARCHON_LIKELY(m_holding_size > 0))
        goto partial;
    goto finalize;

  batch:
    ARCHON_ASSERT(!m_holding_output);
    ARCHON_ASSERT(m_holding_size == 3);
    {
        int_type batch = 0;
        for (int i = 0; i < 3; ++i)
            batch |= int_type(uchar(m_hold_buffer[i])) << ((2 - i) * 8);
        for (int i = 0; i < 4; ++i) {
            std::size_t j = m_hold_buffer.size() - 4 + i;
            m_hold_buffer[j] = chars[(batch >> ((3 - i) * 6)) & 0b111111];
        }
        m_holding_size = 4;
        m_holding_output = true;
    }

  output:
    ARCHON_ASSERT(m_holding_output);
    if (ARCHON_LIKELY(buffer_begin != buffer_end)) {
        if (ARCHON_LIKELY(m_config.line_size == 0 || m_line_size < m_config.line_size)) {
            ARCHON_ASSERT(m_holding_size > 0);
            int i = int(m_hold_buffer.size()) - m_holding_size--;
            *buffer_begin++ = m_hold_buffer[i];
            if (ARCHON_LIKELY(m_config.line_size == 0)) {
                if (ARCHON_LIKELY(m_holding_size > 0))
                    goto output;
            }
            else {
                ++m_line_size;
                if (ARCHON_LIKELY(m_holding_size > 0 || m_line_size == m_config.line_size))
                    goto output;
            }
            m_holding_output = false;
            goto input;
        }
        *buffer_begin++ = '\n';
        m_line_size = 0;
        if (ARCHON_LIKELY(m_holding_size > 0))
            goto output;
        m_holding_output = false;
        goto input;
    }
    return false;

  partial:
    ARCHON_ASSERT(!m_holding_output);
    ARCHON_ASSERT(m_holding_size == 1 || m_holding_size == 2);
    {
        int_type batch = int_type(uchar(m_hold_buffer[0])) << (2 * 8);
        int n = 2;
        if (m_holding_size > 1) {
            batch |= int_type(uchar(m_hold_buffer[1])) << (1 * 8);
            n = 3;
        }
        if (!m_config.use_padding) {
            for (int i = 0; i < n; ++i) {
                std::size_t j = m_hold_buffer.size() - n + i;
                m_hold_buffer[j] = chars[(batch >> ((3 - i) * 6)) & 0b111111];
            }
            m_holding_size = std::int_least8_t(n);
        }
        else {
            for (int i = 0; i < n; ++i) {
                std::size_t j = m_hold_buffer.size() - 4 + i;
                m_hold_buffer[j] = chars[(batch >> ((3 - i) * 6)) & 0b111111];
            }
            for (int i = n; i < 4; ++i) {
                std::size_t j = m_hold_buffer.size() - 4 + i;
                m_hold_buffer[j] = '='; // Padding
            }
            m_holding_size = 4;
        }
        m_holding_output = true;
    }
    goto output;

  finalize:
    if (m_config.line_size == 0 || m_line_size == 0)
        return true;
    if (ARCHON_LIKELY(buffer_begin != buffer_end)) {
        *buffer_begin++ = '\n';
        return true;
    }
    return false;
}


bool IncrementalDecoder::decode(const char*& data_begin, const char* data_end, bool end_of_input,
                                char*& buffer_begin, char* buffer_end) noexcept
{
    using uchar = unsigned char;
    using int_type = std::int_fast32_t;

    auto valid_partial = [&]() noexcept {
        ARCHON_ASSERT(m_holding_size <= 3);
        if (m_holding_size > 2)
            return ((m_hold_buffer[2] & 0b000011) == 0);
        if (m_holding_size > 1)
            return ((m_hold_buffer[1] & 0b001111) == 0);
        return false;
    };

    if (m_holding_output)
        goto output;

  input:
    ARCHON_ASSERT(!m_holding_output);
    if (ARCHON_LIKELY(data_begin != data_end)) {
        char ch = *data_begin;
        int val = int(decode_char(ch));
        bool special = ((val & 0b10000000) != 0);
        bool discr   = ((val & 0b01000000) != 0);
        bool is_regular = ((special && m_config.url_safe_variant) == discr);
        if (ARCHON_LIKELY(is_regular)) {
            if (ARCHON_LIKELY(m_padding_size == 0)) {
                ++data_begin;
                int i = m_holding_size++;
                m_hold_buffer[i] = char(val & 0b111111);
                if (ARCHON_LIKELY(m_holding_size < 4))
                    goto input;
                goto batch;
            }
            return false; // Bad input
        }
        bool is_whitespace = (val == 65);
        if (ARCHON_LIKELY(is_whitespace)) {
            if (m_config.allow_whitespace) {
                ++data_begin;
                goto input;
            }
            return false; // Bad input
        }
        bool is_padding = (val == 64);
        if (ARCHON_LIKELY(is_padding)) {
            bool allow_padding = (m_config.padding != DecodeConfig::Padding::reject);
            if (ARCHON_LIKELY(allow_padding && valid_partial())) {
                ++m_padding_size;
                if (ARCHON_LIKELY(m_holding_size + m_padding_size < 4))
                    goto input;
                goto partial;
            }
            return false; // Bad input
        }
        return false; // Bad input
    }
    if (ARCHON_LIKELY(!end_of_input))
        return false;
    if (ARCHON_LIKELY(m_padding_size == 0)) {
        bool require_padding = (m_config.padding == DecodeConfig::Padding::require);
        if (ARCHON_LIKELY(!require_padding && valid_partial()))
            goto partial;
        if (ARCHON_LIKELY(m_holding_size == 0))
            return true;
    }
    return false; // Prematute end of input

  batch:
    ARCHON_ASSERT(!m_holding_output);
    ARCHON_ASSERT(m_holding_size == 4);
    {
        int_type batch = 0;
        for (int i = 0; i < 4; ++i)
            batch |= int_type(m_hold_buffer[i]) << ((3 - i) * 6);
        for (int i = 0; i < 3; ++i) {
            std::size_t j = m_hold_buffer.size() - 3 + i;
            int_type value = (batch >> ((2 - i) * 8)) & 0b11111111;
            m_hold_buffer[j] = core::twos_compl_cast_a<char>(uchar(value));
        }
        m_holding_size = 3;
        m_holding_output = true;
    }
    goto output;

  output:
    ARCHON_ASSERT(m_holding_output);
    ARCHON_ASSERT(m_holding_size > 0);
    ARCHON_ASSERT(m_holding_size <= 3);
    if (ARCHON_LIKELY(buffer_begin != buffer_end)) {
        int i = int(m_hold_buffer.size()) - m_holding_size--;
        *buffer_begin++ = m_hold_buffer[i];
        if (ARCHON_LIKELY(m_holding_size > 0))
            goto output;
        m_holding_output = false;
        goto input;
    }
    return false;

  partial:
    ARCHON_ASSERT(!m_holding_output);
    ARCHON_ASSERT(m_holding_size == 2 || m_holding_size == 3);
    ARCHON_ASSERT(valid_partial());
    {
        int_type batch = 0;
        for (int i = 0; i < m_holding_size; ++i)
            batch |= int_type(m_hold_buffer[i]) << ((3 - i) * 6);
        int n = m_holding_size - 1;
        for (int i = 0; i < n; ++i) {
            std::size_t j = m_hold_buffer.size() - n + i;
            int_type value = (batch >> ((2 - i) * 8)) & 0b11111111;
            m_hold_buffer[j] = core::cast_from_twos_compl_a<char>(uchar(value));
        }
        m_holding_size = std::int_least8_t(n);
        m_holding_output = true;
    }
    goto output;
}
