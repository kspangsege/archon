// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2024 Kristian Spangsege <kristian.spangsege@gmail.com>
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


       
#include <cstddef>
#include <memory>
#include <algorithm>

#include <archon/core/integer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/check.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/buffer_format.hpp>


using namespace archon;


struct is_double_int {
    template<class T> static constexpr bool value =
        (sizeof (T) == 2 && core::int_width<T>() == int(sizeof (T)) * core::int_width<char>());
};

struct is_quadruple_int {
    template<class T> static constexpr bool value =
        (sizeof (T) == 4 && core::int_width<T>() == int(sizeof (T)) * core::int_width<char>());
};

using double_int_type = core::pick_type<is_double_int, short, int, long, long long>;
using quadruple_int_type = core::pick_type<is_quadruple_int, short, int, long, long long>;


struct endianness_spec {
    static constexpr archon::core::EnumAssoc map[] = {
        { int(core::Endianness::big),    "big"    },
        { int(core::Endianness::little), "little" },
    };
};

using endianness_enum = archon::core::Enum<core::Endianness, endianness_spec>;


struct integer_type_spec {
    static constexpr archon::core::EnumAssoc map[] = {
        { int(image::BufferFormat::IntegerType::byte),   "byte"   },
        { int(image::BufferFormat::IntegerType::schar),  "schar"  },
        { int(image::BufferFormat::IntegerType::short_), "short"  },
        { int(image::BufferFormat::IntegerType::ushort), "uchar"  },
        { int(image::BufferFormat::IntegerType::int_),   "int"    },
        { int(image::BufferFormat::IntegerType::uint),   "uint"   },
        { int(image::BufferFormat::IntegerType::long_),  "long"   },
        { int(image::BufferFormat::IntegerType::ulong),  "ulong"  },
        { int(image::BufferFormat::IntegerType::llong),  "llong"  },
        { int(image::BufferFormat::IntegerType::ullong), "ullong" },
    };
};

using integer_type_enum = archon::core::Enum<image::BufferFormat::IntegerType, integer_type_spec>;



/*    
ARCHON_TEST(Image_BufferFormat_TryCastTo_PackedToInteger_Form1)
{
    auto test = [](auto tag) {
        using word_type = decltype(tag)::type;
        core::Endianness byte_order = {};
        if (ARCHON_LIKELY(core::try_get_byte_order<word_type>(byte_order))) {
            
        }
    };

    test(core::Type<char>());
    test(core::Type<short>());
    test(core::Type<int>());
    test(core::Type<long>());
    test(core::Type<long long>());
}
*/


ARCHON_TEST(Image_BufferFormat_TryCastTo_PackedToInteger)
{
    using IntegerType = image::BufferFormat::IntegerType;

    // A suitably arranged packed format with N times M words per bit compound and with N
    // channels must be representable as an integer format using the same word type and
    // using M word per channel
    {
        // Case 1 of 4: One channel (N = 1) and one word per component in integer format (M = 1)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 1;
        core::Endianness word_order = core::Endianness::big; // Immaterial
        int bits_per_field = 10;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = false;
        bool alpha_channel_first = false; // Immaterial
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer_type_enum(integer.word_type), integer_type_enum(word_type_2));
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_field);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, false);
        }
    }
    {
        // Case 2 of 4: Two channels (N = 2) and one word per component in integer format (M = 1)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 2;
        core::Endianness word_order = core::Endianness::big;
        int bits_per_field = 10;
        image::BitField bit_fields[] = {
            { bits_per_field, 2 }, // Luminance
            { bits_per_field, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer_type_enum(integer.word_type), integer_type_enum(word_type_2));
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_field);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_channel_first, false);
        }
    }
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 3 of 4: One channel (N = 1) and two words per component in integer format (M = 2)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 2;
        int bits_per_field = 2 * bits_per_word;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = false;
        bool alpha_channel_first = false; // Immaterial
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer_type_enum(integer.word_type), integer_type_enum(word_type_2));
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_word);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 2);
            ARCHON_CHECK_EQUAL(endianness_enum(integer.word_order), endianness_enum(word_order));
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, false);
        }
    }
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 4 of 4: Two channels (N = 2) and two words per component in integer format (M = 2)
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 4;
        int bits_per_field = 2 * bits_per_word;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
            { bits_per_field, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, word_type_2)))) {
            ARCHON_CHECK_EQUAL(integer_type_enum(integer.word_type), integer_type_enum(word_type_2));
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_word);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 2);
            ARCHON_CHECK_EQUAL(endianness_enum(integer.word_order), endianness_enum(word_order));
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_channel_first, false);
        }
    }

    // A suitably arranged packed format with N channels, one word per bit compound, and a
    // word type that is made up of N bytes must be representable as an integer format using
    // "byte" as word type and one byte per channel
    if constexpr (!std::is_same_v<double_int_type, void>) {
        IntegerType word_type = {};
        bool success = image::BufferFormat::try_map_integer_type<double_int_type>(word_type);
        ARCHON_ASSERT(success);
        int bits_per_word = core::int_width<double_int_type>();
        int words_per_pixel = 1;
        core::Endianness word_order = core::Endianness::big; // Immaterial
        int bits_per_byte = core::int_width<char>();
        image::BitField bit_fields[] = {
            { bits_per_byte, 0 }, // Luminance
            { bits_per_byte, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        // FIXME: This check is failing                                             
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, IntegerType::byte)))) {
            ARCHON_CHECK_EQUAL(integer_type_enum(integer.word_type), integer_type_enum(IntegerType::byte));
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_byte);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_channel_first, false);
        }
    }
    if constexpr (!std::is_same_v<quadruple_int_type, void>) {
        IntegerType word_type = {};
        bool success = image::BufferFormat::try_map_integer_type<quadruple_int_type>(word_type);
        ARCHON_ASSERT(success);
        int bits_per_word = core::int_width<quadruple_int_type>();
        int words_per_pixel = 1;
        core::Endianness word_order = core::Endianness::big; // Immaterial
        int bits_per_byte = core::int_width<char>();
        image::BitField bit_fields[] = {
            { bits_per_byte, 0 }, // Red
            { bits_per_byte, 0 }, // Greeen
            { bits_per_byte, 0 }, // Blue
            { bits_per_byte, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_rgb();
        bool has_alpha = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false;
        image::BufferFormat format = {};
        format.set_packed_format(word_type, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        // FIXME: This check is failing                                             
        if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, IntegerType::byte)))) {
            ARCHON_CHECK_EQUAL(integer_type_enum(integer.word_type), integer_type_enum(IntegerType::byte));
            ARCHON_CHECK_EQUAL(integer.bits_per_word, bits_per_byte);
            ARCHON_CHECK_EQUAL(integer.words_per_channel, 1);
            ARCHON_CHECK_EQUAL(integer.channel_conf.color_space, &color_space);
            ARCHON_CHECK_EQUAL(integer.channel_conf.has_alpha, true);
            ARCHON_CHECK_EQUAL(integer.channel_conf.alpha_channel_first, false);
            ARCHON_CHECK_EQUAL(integer.channel_conf.reverse_order, false);
        }
    }

    // A suitably arranged packed format with N channels, P words per bit compound, and Q
    // bytes per word must be representable as an integer format using "byte" as word type
    // and M bytes per channel so long as N times M is equal to P times Q and so long as the
    // word order in both formats is equal to the byte order of the word type in the packed
    // format

    // N = 2, P = 2, Q = 2, M = 2
    {
        using word_type = int;
        core::Endianness byte_order = {};
        if (ARCHON_LIKELY(core::try_get_byte_order<word_type>(byte_order))) {
            IntegerType word_type_2 = {};
            bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
            ARCHON_ASSERT(success);
            int bits_per_word = 12;
            int words_per_pixel = 2;
            int bits_per_field = 2 * bits_per_word - 1;
            image::BitField bit_fields[] = {
                { bits_per_field, 0 }, // Luminance
            };
            const image::ColorSpace& color_space = image::ColorSpace::get_lum();
            bool has_alpha = false;
            bool alpha_channel_first = false; // Immaterial
            bool reverse_channel_order = false; // Immaterial
            image::BufferFormat format = {};
            format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                     has_alpha, alpha_channel_first, reverse_channel_order);
            image::BufferFormat::IntegerFormat integer = {};
            if (ARCHON_LIKELY(ARCHON_CHECK(format.try_cast_to(integer, IntegerType::byte)))) {
                
            }
        }
    }

    // Negative cases
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 1: Badly arranged fields in packed format: `bits_per_field` too low
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 2;
        int bits_per_field = 2 * bits_per_word - 1;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = false;
        bool alpha_channel_first = false; // Immaterial
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        ARCHON_CHECK_NOT(format.try_cast_to(integer, word_type_2));
    }
    for (core::Endianness word_order : { core::Endianness::big, core::Endianness::little }) {
        // Case 2: Badly arranged fields in packed format: `bits_per_field` too low
        using word_type = int;
        IntegerType word_type_2 = {};
        bool success = image::BufferFormat::try_map_integer_type<word_type>(word_type_2);
        ARCHON_ASSERT(success);
        int bits_per_word = 12;
        int words_per_pixel = 4;
        int bits_per_field = 2 * bits_per_word - 1;
        image::BitField bit_fields[] = {
            { bits_per_field, 0 }, // Luminance
            { bits_per_field, 0 }, // Alpha
        };
        const image::ColorSpace& color_space = image::ColorSpace::get_lum();
        bool has_alpha = true;
        bool alpha_channel_first = false;
        bool reverse_channel_order = false; // Immaterial
        image::BufferFormat format = {};
        format.set_packed_format(word_type_2, bits_per_word, words_per_pixel, word_order, bit_fields, color_space,
                                 has_alpha, alpha_channel_first, reverse_channel_order);
        image::BufferFormat::IntegerFormat integer = {};
        ARCHON_CHECK_NOT(format.try_cast_to(integer, word_type_2));
    }
}
