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


/// \file

#ifndef ARCHON__UNIT_TEST__RANDOM_SEED_HPP
#define ARCHON__UNIT_TEST__RANDOM_SEED_HPP

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <array>
#include <locale>
#include <istream>
#include <ostream>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/integer.hpp>
#include <archon/base/span.hpp>
#include <archon/base/buffer.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/base/stream_input.hpp>
#include <archon/unit_test/seed_seq.hpp>


namespace archon::unit_test {


/// \brief Random number generator seed with serialized form.
///
/// An instance of this class is a sequence of 32-bit integer values that is
/// meant to be used for seeding pseudo random number generators. This class
/// also offers a way to produce a nondeterministic random seed (\ref random()).
///
/// Additionally, stream input and output operators are provided in order to
/// facilitate the use of such seeds as part of a command line interface.
///
/// The number of 32-bit values in a seed of this kind is always an intgeral
/// multiple of 6. This means that a seed can be thought of as a sequence of 192
/// bit blocks.
///
/// In the serialized form (using the stream output operator), a seed is a
/// sequence of blocks of alphanumeric characters with a leading and a trailing
/// dash (`-`), as well as a dash between each block. Each blocks of
/// alphanumeric characters represents a 192-bit block of the seed. For example,
/// a seed containing two blocks might look like this:
///
///     -DHCTKSZ3ezH6eqQDUU78Xz7Dq34LsJj8j-3ZaCeKvxGu0FWTTBkyC8TG7f18BaKQ0tZ-
///
/// The first 11 characters of the seqialized form of a block is produced by
/// taking the first two 32-bit words in that block, and joining them into a
/// 64-bit word with the first 32-bit word contributing the least significant
/// bits. The resulting 64-bit value is then formatted as in integer in
/// base/radix 62, where the 26 upper and lower case latin letters of ASCII are
/// used as digit values 10 -> 35 and 36 -> 61 respectively. Likewise, the 3rd
/// and 4th 32-bit words of that block make up the next 11 characters, and so
/// forth.
///
/// If `seed` is an instance of this class, then seeding can occur as follows:
///
/// \code{.cpp}
///
///   auto seed_seq = base::SeedSeq::no_copy(seed.span());
///   std::mt19937_64 random(seed_seq);
///
/// \endcode
///
/// Note that a Mersenne Twister engine such as `std::mt19937_64` has a state of
/// 19968 bits. This means that you will need a seed with 104 blocks (624
/// integer values each providing 32 bits of entropy) to fully saturate its
/// state. This would be an optimal seeding. In serialized form, such a seed
/// would be 3431 characters long.
///
class RandomSeed {
public:
    using value_type = unit_test::SeedSeq::result_type;
    using iterator = const value_type*;

    /// \brief Create empty seed.
    ///
    /// This constructor creates an empty seed (zero blocks).
    ///
    RandomSeed() noexcept = default;

    /// \brief Create seed from specified values.
    ///
    /// This constructor creates a seed containing a copy of some or all of the
    /// specified values. The numeber of copied values is the number of
    /// specified values rounded down to the nearest integral multiple of 6.
    ///
    explicit RandomSeed(base::Span<const value_type> values);

    /// \brief Create nondeterministic random seed.
    ///
    /// This function uses \ref base::seed_nondeterministically_a() to construct
    /// a nondeterministic random seed with the specified number of 192-bit
    /// blocks.
    ///
    static RandomSeed random(std::size_t num_blocks);

    base::Span<const value_type> span() const noexcept;

    iterator begin() const noexcept;
    iterator end() const noexcept;

    const value_type* data() const noexcept;
    std::size_t size() const noexcept;

    RandomSeed(const RandomSeed&);
    RandomSeed& operator=(const RandomSeed&);

    RandomSeed(RandomSeed&&) noexcept = default;
    RandomSeed& operator=(RandomSeed&&) noexcept = default;

private:
    base::Buffer<value_type> m_buffer;
};




template<class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>&, const RandomSeed&);

template<class C, class T>
std::basic_istream<C, T>& operator>>(std::basic_istream<C, T>&, RandomSeed&);








// Implementation


inline auto RandomSeed::span() const noexcept -> base::Span<const value_type>
{
    return { data(), size() };
}


inline auto RandomSeed::begin() const noexcept -> iterator
{
    return data();
}


inline auto RandomSeed::end() const noexcept -> iterator
{
    return data() + size();
}


inline auto RandomSeed::data() const noexcept -> const value_type*
{
    return m_buffer.data();
}


inline std::size_t RandomSeed::size() const noexcept
{
    return m_buffer.size();
}


inline RandomSeed::RandomSeed(const RandomSeed& other)
{
    m_buffer.set_size(other.size()); // Throws
    std::copy_n(other.data(), other.size(), m_buffer.data());
}


inline RandomSeed& RandomSeed::operator=(const RandomSeed& other)
{
    m_buffer.set_size(other.size()); // Throws
    std::copy_n(other.data(), other.size(), m_buffer.data());
    return *this;
}


namespace detail {


inline const char g_random_seed_base62_chars[62] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};


constexpr int random_seed_map_digit(char ch) noexcept
{
    switch (ch) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'A':
            return 10;
        case 'B':
            return 11;
        case 'C':
            return 12;
        case 'D':
            return 13;
        case 'E':
            return 14;
        case 'F':
            return 15;
        case 'G':
            return 16;
        case 'H':
            return 17;
        case 'I':
            return 18;
        case 'J':
            return 19;
        case 'K':
            return 20;
        case 'L':
            return 21;
        case 'M':
            return 22;
        case 'N':
            return 23;
        case 'O':
            return 24;
        case 'P':
            return 25;
        case 'Q':
            return 26;
        case 'R':
            return 27;
        case 'S':
            return 28;
        case 'T':
            return 29;
        case 'U':
            return 30;
        case 'V':
            return 31;
        case 'W':
            return 32;
        case 'X':
            return 33;
        case 'Y':
            return 34;
        case 'Z':
            return 35;
        case 'a':
            return 36;
        case 'b':
            return 37;
        case 'c':
            return 38;
        case 'd':
            return 39;
        case 'e':
            return 40;
        case 'f':
            return 41;
        case 'g':
            return 42;
        case 'h':
            return 43;
        case 'i':
            return 44;
        case 'j':
            return 45;
        case 'k':
            return 46;
        case 'l':
            return 47;
        case 'm':
            return 48;
        case 'n':
            return 49;
        case 'o':
            return 50;
        case 'p':
            return 51;
        case 'q':
            return 52;
        case 'r':
            return 53;
        case 's':
            return 54;
        case 't':
            return 55;
        case 'u':
            return 56;
        case 'v':
            return 57;
        case 'w':
            return 58;
        case 'x':
            return 59;
        case 'y':
            return 60;
        case 'z':
            return 61;
    }
    return -1;
}


constexpr bool random_seed_map_digit(char ch, int& digit) noexcept
{
    int value = random_seed_map_digit(ch);
    if (ARCHON_LIKELY(value != -1)) {
        digit = value;
        return true;
    }
    return false;
}


} // namespace detail


template<class C, class T>
std::basic_ostream<C, T>& operator<<(std::basic_ostream<C, T>& out, const RandomSeed& seed)
{
    typename std::basic_ostream<C, T>::sentry sentry(out); // Throws
    if (ARCHON_LIKELY(sentry)) {
        base::BasicCharMapper<C, T> char_mapper(out); // Throws
        C sep = char_mapper.widen('-');
        const auto* seed_data = seed.data();
        std::array<char, 33> encode_buffer;
        std::array<C, 33> widen_buffer;
        auto format_block = [&](std::size_t offset) {
            for (int i = 0; i < 3; ++i) {
                std::uint_fast64_t value = 0;
                for (int j = 0; j < 2; ++j)
                    value |= std::uint_fast64_t(seed_data[offset + i * 2 + j]) << (j * 32);
                for (int j = 0; j < 11; ++j) {
                    encode_buffer[i * 11 + (10 - j)] =
                        detail::g_random_seed_base62_chars[value % 62];
                    value /= 62;
                }
            }
            const C* widened;
            if constexpr (char_mapper.is_trivial) {
                static_cast<void>(widen_buffer);
                widened = encode_buffer.data();
            }
            else {
                char_mapper.widen({ encode_buffer.data(), encode_buffer.size() },
                                  widen_buffer.data()); // Throws
                widened = widen_buffer.data();
            }
            out.write(widened, 33); // Throws
        };
        std::size_t num_blocks = seed.size() / 6;
        ARCHON_ASSERT(seed.size() == num_blocks * 6);
        out.put(sep); // Throws
        for (std::size_t i = 0; i < num_blocks; ++i) {
            std::size_t offset = i * 6;
            format_block(offset); // Throws
            out.put(sep); // Throws
        }
    }

    return out;
}


template<class C, class T>
std::basic_istream<C, T>& operator>>(std::basic_istream<C, T>& in, RandomSeed& seed)
{
    return base::istream_sentry(in, [&](base::BasicStreamInputHelper<C, T>& helper) {
        const std::ctype<C>& ctype = std::use_facet<std::ctype<C>>(in.getloc());
        base::BasicCharMapper<C, T> mapper(ctype);
        C sep(mapper.widen('-')); // Throws
        std::array<RandomSeed::value_type, 192> seed_memory;
        base::SeedMemoryBuffer buffer(seed_memory);
        base::SeedMemoryBufferContents values(buffer);
        C ch;
        if (ARCHON_UNLIKELY(!helper.peek(ch)))
            return false;
        for (;;) {
            if (ARCHON_UNLIKELY(ch != sep))
                return false;
            if (ARCHON_UNLIKELY(!helper.next(ch) || !ctype.is(std::ctype_base::alnum, ch)))
                break;
            for (int i = 0; i< 3; ++i) {
                std::uint_fast64_t value = 0;
                for (int j = 0; j < 11; ++j) {
                    char ch_2 = mapper.narrow(ch, '\0'); // Throws
                    int digit = 0;
                    if (ARCHON_UNLIKELY(!detail::random_seed_map_digit(ch_2, digit)))
                        return false;
                    if (ARCHON_UNLIKELY(!base::try_int_mul(value, 62)))
                        return false;
                    if (ARCHON_UNLIKELY(!base::try_int_add(value, digit)))
                        return false;
                    if (ARCHON_UNLIKELY(!helper.next(ch)))
                        return false;
                }
                if (ARCHON_UNLIKELY(value >= base::int_mask<std::uint_fast64_t>(64)))
                    return false;
                auto mask = base::int_mask<RandomSeed::value_type>(32);
                auto value_1 = RandomSeed::value_type(value         & mask);
                auto value_2 = RandomSeed::value_type((value >> 32) & mask);
                values.push_back(value_1); // Throws
                values.push_back(value_2); // Throws
            }
        }
        seed = RandomSeed(values); // Throws
        return true;
    }); // Throws
}


} // namespace archon::unit_test

#endif // ARCHON__UNIT_TEST__RANDOM_SEED_HPP
