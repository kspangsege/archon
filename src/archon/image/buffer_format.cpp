// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2022 Kristian Spangsege <kristian.spangsege@gmail.com>
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


#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/endianness.hpp>
#include <archon/image/buffer_format.hpp>


using namespace archon;
using image::BufferFormat;


bool BufferFormat::IntegerFormat::try_cast_to(IntegerFormat& format, IntegerType target_word_type) const
{
    // CASE: integer --> integer

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid integer format");

    int bits_per_word_2 = bits_per_word;
    int words_per_channel_2 = words_per_channel;
    core::Endianness word_order_2 = word_order;

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // Cannot have unused bits
            int bits_per_byte = core::int_width<char>();
            if (ARCHON_UNLIKELY(bits_per_word != bytes_per_word * bits_per_byte))
                return false;

            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!try_get_byte_order(word_type, byte_order)))
                return false;

            // If there is more than one word per channel, native byte order must match word
            // order
            if (ARCHON_UNLIKELY(words_per_channel > 1 && word_order != byte_order))
                return false;

            bits_per_word_2 = bits_per_byte;
            words_per_channel_2 *= bytes_per_word;
            word_order_2 = byte_order;
        }
    }

    format = {
        target_word_type,
        bits_per_word_2,
        words_per_channel_2,
        word_order_2,
        channel_conf,
    };

    return true;
}


bool BufferFormat::IntegerFormat::try_cast_to(PackedFormat& format, IntegerType target_word_type) const
{
    // CASE: integer --> packed

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid integer format");

    int bits_per_word_2 = bits_per_word;
    int words_per_channel_2 = words_per_channel;
    core::Endianness word_order_2 = word_order;

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // Cannot have unused bits
            int bits_per_byte = core::int_width<char>();
            if (ARCHON_UNLIKELY(bits_per_word != bytes_per_word * bits_per_byte))
                return false;

            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!try_get_byte_order(word_type, byte_order)))
                return false;

            // If there is more than one word per channel, native byte order must match word
            // order
            if (ARCHON_UNLIKELY(words_per_channel > 1 && word_order != byte_order))
                return false;

            bits_per_word_2 = bits_per_byte;
            words_per_channel_2 *= bytes_per_word;
            word_order_2 = byte_order;
        }
    }

    int num_channels = channel_conf.get_num_channels();
    if (ARCHON_UNLIKELY(num_channels > max_bit_fields))
        return false;

    int words_per_pixel = words_per_channel_2;
    if (ARCHON_UNLIKELY(!core::try_int_mul(words_per_pixel, num_channels)))
        return false;

    format = {
        target_word_type,
        bits_per_word_2,
        words_per_pixel,
        word_order_2,
        {}, // bit fields
        channel_conf,
    };

    int bits_per_channel = words_per_channel * bits_per_word;
    for (int i = 0; i < num_channels; ++i) {
        int width = bits_per_channel;
        int gap = 0;
        format.bit_fields[i] = { width, gap };
    }

    if (word_order_2 == core::Endianness::little)
        format.channel_conf.reverse();

    return true;
}


bool BufferFormat::IntegerFormat::try_cast_to(SubwordFormat& format, IntegerType target_word_type) const
{
    // CASE: integer --> subword

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid integer format");

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        // If there is more than one byte per word, it is impossible to cast to a byte-base
        // format, since in a byte-based subword format a channel cannot cross word
        // boundaries.
        int bytes_per_word = get_bytes_per_word(word_type);
        if (ARCHON_UNLIKELY(bytes_per_word > 1))
            return false;
    }

    // If the integer format has more than one word per pixel, it is impossible to express
    // it as a subword format, because in a subword format a pixel cannot cross word
    // boundaries.
    int num_channels = channel_conf.get_num_channels();
    if (ARCHON_UNLIKELY(num_channels > 1 || words_per_channel > 1))
        return false;

    int bits_per_channel = bits_per_word;
    int pixels_per_word = 1;
    core::Endianness bit_order = core::Endianness::big;
    bool word_aligned_rows = false;
    format = {
        target_word_type,
        bits_per_channel,
        pixels_per_word,
        bit_order,
        word_aligned_rows,
        channel_conf,
    };

    return true;
}


bool BufferFormat::PackedFormat::try_cast_to(IntegerFormat& format, IntegerType target_word_type) const
{
    // CASE: packed --> integer

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid packed format");

    int num_channels = channel_conf.get_num_channels();
    int bits_per_word_2 = bits_per_word;
    int words_per_channel = {};
    core::Endianness word_order_2 = word_order;
    int field_module = {};
    int depth = {};

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // Cannot have unused bits
            int bits_per_byte = core::int_width<char>();
            if (ARCHON_UNLIKELY(bits_per_word != bytes_per_word * bits_per_byte))
                return false;

            // There must be a whole number of bytes per channel
            int bytes_per_pixel = words_per_pixel * bytes_per_word;
            int bytes_per_channel = bytes_per_pixel / num_channels;
            int remainder = bytes_per_pixel % num_channels;
            if (ARCHON_UNLIKELY(remainder != 0))
                return false;

            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!BufferFormat::try_get_byte_order(word_type, byte_order)))
                return false;

            // If there is only one word per bit compound, the origin word order has no
            // effect. If there is exactly one origin word per channel, a mismatch between
            // origin word order and native byte order for the origin word type can be
            // compensated for by channel order reversal (ChannelConf::reverse()). In all
            // other cases, compensation is impossible.
            if (words_per_pixel != 1 && words_per_pixel != num_channels && word_order != byte_order)
                return false;

            bits_per_word_2 = bits_per_byte;
            words_per_channel = bytes_per_channel;
            word_order_2 = byte_order;
            field_module = bytes_per_channel * bits_per_byte;
            depth = field_module;
            goto proceed;
        }
    }

    {
        // There must be a whole number of words per channel
        words_per_channel = words_per_pixel / num_channels;
        int remainder = words_per_pixel % num_channels;
        if (ARCHON_UNLIKELY(remainder != 0))
            return false;

        field_module = words_per_channel * bits_per_word;
        if (ARCHON_LIKELY(words_per_channel == 1)) {
            bits_per_word_2 = bit_fields[0].width;
            depth = bits_per_word_2;
        }
        else {
            depth = field_module;
        }
    }

  proceed:
    int gap = field_module - depth;
    for (int i = 0; i < num_channels; ++i) {
        const image::BitField& field = bit_fields[i];
        if (ARCHON_UNLIKELY(field.width != depth || (field.gap != (i == num_channels - 1 ? 0 : gap))))
            return false;
    }

    format = {
        target_word_type,
        bits_per_word_2,
        words_per_channel,
        word_order_2,
        channel_conf,
    };

    if ((words_per_pixel == num_channels ? word_order : format.word_order) == core::Endianness::little)
        format.channel_conf.reverse();

    return true;
}


bool BufferFormat::PackedFormat::try_cast_to(PackedFormat& format, IntegerType target_word_type) const
{
    // CASE: packed --> packed

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid packed format");

    int num_channels = channel_conf.get_num_channels();
    int bits_per_word_2 = bits_per_word;
    int words_per_pixel_2 = words_per_pixel;
    core::Endianness word_order_2 = word_order;
    std::array<image::BitField, max_bit_fields> bit_fields_2 = bit_fields;
    BufferFormat::ChannelConf channel_conf_2 = channel_conf;

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!try_get_byte_order(word_type, byte_order)))
                return false;

            int bits_per_byte = core::int_width<char>();
            bits_per_word_2 = bits_per_byte;
            words_per_pixel_2 *= bytes_per_word;
            word_order_2 = byte_order;

            if (words_per_pixel == 1)
                goto proceed;

            if (word_order == byte_order && bits_per_word == bytes_per_word * bits_per_byte)
                goto proceed;

            // Adjust bit fields and channel order to compensate for word order inversion
            // and/or insertion of bits between origin words.
            struct FieldWord {
                int word_index;
                int bit_pos;
                int num_fields;
            };
            // First entry covers the fields at the least significant bit positions in the
            // origin format.
            FieldWord field_words[max_bit_fields] {};
            int num_field_words = 0;
            {
                int offset = 0;
                int prev_word_index = -1;
                for (int i = num_channels; i > 0; --i) {
                    const image::BitField& field = bit_fields[i - 1];
                    int bit_pos = field.gap + offset;
                    int word_index = bit_pos / bits_per_word;
                    int bit_pos_2  = bit_pos % bits_per_word;
                    // The effect of word order inversion and/or insertion of bits between
                    // words cannot be compensated for in any way if a bit field crosses an
                    // word boundary.
                    bool word_confined = (field.width <= bits_per_word - bit_pos_2);
                    if (!word_confined)
                        return false;
                    ARCHON_ASSERT(word_index >= 0);
                    if (word_index == prev_word_index) {
                        ARCHON_ASSERT(num_field_words > 0);
                        FieldWord& field_word = field_words[num_field_words - 1];
                        ARCHON_ASSERT(field_word.word_index == word_index);
                        field_word.num_fields += 1;
                    }
                    else {
                        ARCHON_ASSERT(num_field_words < max_bit_fields);
                        field_words[num_field_words] = {
                            word_index,
                            bit_pos_2,
                            1
                        };
                        num_field_words += 1;
                        prev_word_index = word_index;
                    }
                    offset = field.width + bit_pos;
                }
            }

            ARCHON_ASSERT(num_field_words > 0);
            if (word_order == byte_order) {
                // When there is no inversion of origin word order, the channel order is
                // unchanged but bit compound inflation still needs to be compensated for.
                ARCHON_ASSERT(bits_per_word < bytes_per_word * bits_per_byte);
                int infl = bytes_per_word * bits_per_byte - bits_per_word;
                int i = num_channels;
                int prev_word_index = 0;
                for (int j = 0; j < num_field_words; ++j) {
                    const FieldWord& field_word = field_words[j];
                    int n = field_word.word_index - prev_word_index;
                    bit_fields_2[i - 1].gap += n * infl;
                    i -= field_word.num_fields;
                    prev_word_index = field_word.word_index;
                }
                goto proceed;
            }

            if (num_field_words == 1) {
                // When all bit fields are in the same origin word, origin word order
                // inversion does not affect channel order.
            }
            else if (num_field_words == num_channels) {
                // When there is never more than one bit field in an origin word, origin
                // word order inversion can be compensated for by channel order reversal.
                channel_conf_2.reverse();
            }
            else if (num_field_words == 2 && channel_conf.has_alpha &&
                     field_words[channel_conf.alpha_first ==
                                 channel_conf.reverse_order ? 0 : 1].num_fields == 1) {
                // When all the bit fields, that correspond to color channels, are in the
                // same origin word and the alpha channel is in a different origin word,
                // origin word order inversion can be compensated for by swicthing the alpha
                // channel side.
                channel_conf_2.alpha_first = !channel_conf_2.alpha_first;
            }
            else {
                // Channel order changed in a way that cannot be compensated for
                return false;
            }

            // Adjust bit field specifications in accordance with word order inversion
            // and/or insertion of bits between origin words.
            {
                int i = 0;
                int offset = 0;
                for (int j = num_field_words; j > 0; --j) {
                    const FieldWord& field_word = field_words[j - 1];
                    ARCHON_ASSERT(field_word.word_index < words_per_pixel);
                    int word_index = field_word.word_index;
                    word_index = words_per_pixel - 1 - word_index;
                    int bit_pos = word_index * bytes_per_word * bits_per_byte + field_word.bit_pos;
                    int n = field_word.num_fields;
                    ARCHON_ASSERT(n > 0);
                    ARCHON_ASSERT(num_channels > 0);
                    int i_1 = i + n - 1;
                    int i_2 = num_channels - i - 1;
                    int width = bit_fields[i_1].width;
                    int gap = bit_pos - offset;
                    offset = width + bit_pos;
                    int k = 0;
                    for (;;) {
                        bit_fields_2[i_2 - k] = { width, gap };
                        k += 1;
                        if (k == n)
                            break;
                        const image::BitField& field = bit_fields[i_1 - k];
                        width = field.width;
                        gap = field.gap;
                        offset = width + gap + offset;
                    }
                    i += n;
                }
                ARCHON_ASSERT(i == num_channels);
            }
        }
    }

  proceed:
    for (int i = num_channels; i < max_bit_fields; ++i)
        bit_fields_2[i] = {};

    format = {
        target_word_type,
        bits_per_word_2,
        words_per_pixel_2,
        word_order_2,
        bit_fields_2,
        channel_conf_2,
    };

    return true;
}


bool BufferFormat::PackedFormat::try_cast_to(SubwordFormat& format, IntegerType target_word_type) const
{
    // CASE: packed --> subword

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid packed format");

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        // If there is more than one byte per word, it is impossible to cast to a byte-base
        // format, since in a byte-based subword format a channel cannot cross word
        // boundaries.
        int bytes_per_word = get_bytes_per_word(word_type);
        if (ARCHON_UNLIKELY(bytes_per_word > 1))
            return false;
    }

    // If the packed format has more than one word per bit compound, it is impossible to
    // express it as a subword format, because in a subword format a pixel cannot cross word
    // boundaries.
    if (ARCHON_UNLIKELY(words_per_pixel > 1))
        return false;

    // Bit fields must all have the same width and there must be no gaps between them and
    // the least significan bit position must be part of a field.
    int depth = bit_fields[0].width;
    int num_channels = channel_conf.get_num_channels();
    for (int i = 0; i < num_channels; ++i) {
        const image::BitField& field = bit_fields[i];
        if (ARCHON_UNLIKELY(field.width != depth || field.gap != 0))
            return false;
    }

    int bits_per_channel = depth;
    int pixels_per_word = 1;
    core::Endianness bit_order = core::Endianness::big;
    bool word_aligned_rows = false;
    format = {
        target_word_type,
        bits_per_channel,
        pixels_per_word,
        bit_order,
        word_aligned_rows,
        channel_conf,
    };

    return true;
}


bool BufferFormat::SubwordFormat::try_cast_to(IntegerFormat& format, IntegerType target_word_type) const
{
    // CASE: subword --> integer

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid subword format");

    int num_channels = channel_conf.get_num_channels();
    int bits_per_word = bits_per_channel;
    int words_per_channel = 1;
    core::Endianness word_order = core::Endianness::big;

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // There must be a whole number of bytes per channel
            int bits_per_byte = core::int_width<char>();
            int bytes_per_channel = bits_per_channel / bits_per_byte;
            int remainder = bits_per_channel % bits_per_byte;
            if (ARCHON_UNLIKELY(remainder != 0))
                return false;

            // Cannot have unused bits
            int bits_per_word_2 = (bits_per_channel * num_channels * pixels_per_word);
            if (ARCHON_UNLIKELY(bits_per_word_2 != bytes_per_word * bits_per_byte))
                return false;

            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!BufferFormat::try_get_byte_order(word_type, byte_order)))
                return false;

            if (ARCHON_LIKELY(pixels_per_word > 1)) {
                // Bit order must match native byte order
                if (ARCHON_UNLIKELY(byte_order != bit_order))
                    return false;

                // Cannot have word aligned rows
                if (ARCHON_UNLIKELY(word_aligned_rows))
                    return false;
            }

            bits_per_word = bits_per_byte;
            words_per_channel = bytes_per_channel;
            word_order = byte_order;
            goto proceed;
        }
    }

    // If the subword format has more than one channel in a word, it is impossible to
    // express it as an integer format based on the same word type.
    if (ARCHON_UNLIKELY(num_channels > 1 || pixels_per_word > 1))
        return false;

  proceed:
    format = {
        target_word_type,
        bits_per_word,
        words_per_channel,
        word_order,
        channel_conf,
    };

    if (word_order == core::Endianness::little)
        format.channel_conf.reverse();

    return true;
}


bool BufferFormat::SubwordFormat::try_cast_to(PackedFormat& format, IntegerType target_word_type) const
{
    // CASE: subword --> packed

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid subword format");

    int num_channels = channel_conf.get_num_channels();
    int bits_per_pixel = num_channels * bits_per_channel;
    int bits_per_word = bits_per_pixel;
    int words_per_pixel = 1;
    core::Endianness word_order = core::Endianness::big;

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bits_per_byte = core::int_width<char>();
        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!try_get_byte_order(word_type, byte_order)))
                return false;

            if (pixels_per_word > 1) {
                // There must be a whole number of bytes per pixel
                int remainder = bits_per_pixel % bits_per_byte;
                if (ARCHON_UNLIKELY(remainder != 0))
                    return false;

                // Cannot have unused bits
                int bits_per_word_2 = (pixels_per_word * bits_per_pixel);
                if (ARCHON_UNLIKELY(bits_per_word_2 != bytes_per_word * bits_per_byte))
                    return false;

                // Bit order must match native byte order
                if (ARCHON_UNLIKELY(byte_order != bit_order))
                    return false;

                // Cannot have word aligned rows
                if (ARCHON_UNLIKELY(word_aligned_rows))
                    return false;
            }

            bits_per_word = bits_per_byte;
            words_per_pixel = bytes_per_word / pixels_per_word;
            word_order = byte_order;
            goto proceed;
        }
    }

    // If the subword format has more than one pixel per word, it is impossible to express
    // it as a packed format based on the same word type.
    if (ARCHON_UNLIKELY(pixels_per_word > 1))
        return false;

  proceed:
    if (ARCHON_UNLIKELY(num_channels > max_bit_fields))
        return false;

    format = {
        target_word_type,
        bits_per_word,
        words_per_pixel,
        word_order,
        {}, // bit fields
        channel_conf,
    };

    for (int i = 0; i < num_channels; ++i) {
        int width = bits_per_channel;
        int gap = 0;
        format.bit_fields[i] = { width, gap };
    }

    return true;
}


bool BufferFormat::SubwordFormat::try_cast_to(SubwordFormat& format, IntegerType target_word_type) const
{
    // CASE: subword --> subword

    if (ARCHON_UNLIKELY(!is_valid()))
        throw std::runtime_error("Invalid subword format");

    int pixels_per_word_2 = pixels_per_word;

    if (target_word_type != word_type) {
        // A word of any type can be accessed in terms of the bytes that make it up
        // (`std::byte`, `char`, or `unsigned char`, but not `signed char`). Any other type
        // punning would cause undefined behavior.
        if (ARCHON_UNLIKELY(target_word_type != IntegerType::byte))
            return false;

        int bytes_per_word = get_bytes_per_word(word_type);
        if (bytes_per_word > 1) {
            // Cannot have unused bits
            int num_channels = channel_conf.get_num_channels();
            int bits_per_word = bits_per_channel * num_channels * pixels_per_word;
            int bits_per_byte = core::int_width<char>();
            if (ARCHON_UNLIKELY(bits_per_word != bytes_per_word * bits_per_byte))
                return false;

            // There must be a whole number of pixels per byte
            int pixels_per_byte = pixels_per_word / bytes_per_word;
            int remainder       = pixels_per_word % bytes_per_word;
            if (ARCHON_UNLIKELY(remainder != 0))
                return false;

            // Native byte order must be determinable
            core::Endianness byte_order = {};
            if (ARCHON_UNLIKELY(!try_get_byte_order(word_type, byte_order)))
                return false;

            // Bit order must match native byte order
            if (ARCHON_UNLIKELY(byte_order != bit_order))
                return false;

            // Cannot have requirement of word aligned rows
            if (ARCHON_UNLIKELY(word_aligned_rows))
                return false;

            pixels_per_word_2 = pixels_per_byte;
        }
    }

    format = {
        target_word_type,
        bits_per_channel,
        pixels_per_word_2,
        bit_order,
        word_aligned_rows,
        channel_conf,
    };

    return true;
}


bool BufferFormat::is_valid() const noexcept
{
    switch (type) {
        case Type::integer:
            return integer.is_valid();
        case Type::packed:
            return packed.is_valid();
        case Type::subword:
            return subword.is_valid();
        case Type::float_:
            return float_.is_valid();
        case Type::indexed:
            return indexed.is_valid();
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


bool BufferFormat::try_cast_to(IntegerFormat& format, IntegerType target_word_type) const
{
    switch (type) {
        case Type::integer:
            return integer.try_cast_to(format, target_word_type); // Throws
        case Type::packed:
            return packed.try_cast_to(format, target_word_type); // Throws
        case Type::subword:
            return subword.try_cast_to(format, target_word_type); // Throws
        case Type::float_:
        case Type::indexed:
            break;
    }

    return false;
}


bool BufferFormat::try_cast_to(PackedFormat& format, IntegerType target_word_type) const
{
    switch (type) {
        case Type::integer:
            return integer.try_cast_to(format, target_word_type); // Throws
        case Type::packed:
            return packed.try_cast_to(format, target_word_type); // Throws
        case Type::subword:
            return subword.try_cast_to(format, target_word_type); // Throws
        case Type::float_:
        case Type::indexed:
            break;
    }

    return false;
}


bool BufferFormat::try_cast_to(SubwordFormat& format, IntegerType target_word_type) const
{
    switch (type) {
        case Type::integer:
            return integer.try_cast_to(format, target_word_type); // Throws
        case Type::packed:
            return packed.try_cast_to(format, target_word_type); // Throws
        case Type::subword:
            return subword.try_cast_to(format, target_word_type); // Throws
        case Type::float_:
        case Type::indexed:
            break;
    }

    return false;
}


bool BufferFormat::try_get_byte_order(IntegerType word_type, core::Endianness& byte_order) noexcept
{
    // Unsigned integer types are required to have the same object representation as their
    // corresponding signed types (C++20 section 6.8.1 [basic.fundamental]), which implies
    // thay they use the same byte order.
    switch (word_type) {
        case IntegerType::byte:
        case IntegerType::schar:
            byte_order = core::Endianness::big;
            return true;
        case IntegerType::short_:
        case IntegerType::ushort:
            return core::try_get_byte_order<short>(byte_order);
        case IntegerType::int_:
        case IntegerType::uint:
            return core::try_get_byte_order<int>(byte_order);
        case IntegerType::long_:
        case IntegerType::ulong:
            return core::try_get_byte_order<long>(byte_order);
        case IntegerType::llong:
        case IntegerType::ullong:
            return core::try_get_byte_order<long long>(byte_order);
        case IntegerType::fict_1:
            byte_order = core::Endianness::big;
            return true;
        case IntegerType::fict_2:
            byte_order = core::Endianness::little;
            return true;
        case IntegerType::fict_3:
            return false;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}
