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


bool BufferFormat::try_cast_to(IntegerFormat& format, IntegerType word_type) const
{
    switch (type) {
        case Type::integer: {
            if (ARCHON_UNLIKELY(!integer.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            if (word_type != integer.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // Cast to byte-based format
                int bytes_per_word = get_bytes_per_word(integer.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1)) {
                    // Cannot have unused bits
                    int bits_per_byte = core::int_width<char>();
                    int n = bits_per_byte;
                    bool all_bits_used = (core::try_int_mul(n, bytes_per_word) && integer.bits_per_word == n);
                    if (ARCHON_UNLIKELY(!all_bits_used))
                        return false;
                    // Native byte order must be determinable
                    core::Endianness byte_order = {};
                    if (ARCHON_UNLIKELY(!try_get_byte_order(integer.word_type, byte_order)))
                        return false;
                    // When word order of original format matters, it must match the native
                    // byte order
                    if (ARCHON_UNLIKELY(integer.words_per_channel > 1 && integer.word_order != byte_order))
                        return false;
                    format = integer;
                    format.word_type = IntegerType::byte;
                    format.bits_per_word = bits_per_byte;
                    format.words_per_channel *= bytes_per_word;
                    format.word_order = byte_order;
                    return true;
                }
            }
            format = integer;
            format.word_type = word_type;
            return true;
        }
        case Type::packed: {
            if (ARCHON_UNLIKELY(!packed.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            int num_channels = packed.channel_conf.get_num_channels();
            ARCHON_ASSERT(num_channels > 0);
            // Number of words per bit compound must be divisible by number of channels
            int words_per_channel = packed.words_per_pixel / num_channels;
            int remainder = packed.words_per_pixel % num_channels;
            if (ARCHON_UNLIKELY(remainder != 0))
                return false;
            // All channel shifts must be sequential multiples of the number of bits per
            // part resulting from splitting the compound into as many parts as there are
            // channels.
            int bits_per_word = packed.bits_per_word;
            int bits_per_part = words_per_channel * bits_per_word;
            {
                int i = 0;
                int gap = packed.bit_fields[i].gap;
                while (ARCHON_LIKELY(++i < num_channels)) {
                    image::BitField field = packed.bit_fields[i];
                    if (ARCHON_UNLIKELY(gap + field.width != bits_per_part))
                        return false;
                    gap = field.gap;
                }
                if (ARCHON_UNLIKELY(gap != 0))
                    return false;
            }
            core::Endianness word_order = packed.word_order;
            if (word_type != packed.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // Cast to byte-based format
                int bytes_per_word = get_bytes_per_word(packed.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1)) {
                    // Words cannot have unused bits, as that would cause the layout of the
                    // meaning of bits to differ between bytes. Since bytes cannot have
                    // padding bits, the word type can also not have padding bits. If it
                    // did, it would effectively count as unused bits.
                    int n = core::int_width<char>();
                    bool all_bits_used = (core::try_int_mul(n, bytes_per_word) && packed.bits_per_word == n);
                    if (ARCHON_UNLIKELY(!all_bits_used))
                        return false;
                    // Native byte order must be determinable
                    core::Endianness byte_order = {};
                    if (ARCHON_UNLIKELY(!try_get_byte_order(packed.word_type, byte_order)))
                        return false;
                    // The order in which words are assembled into channels must agree with
                    // the order that bytes are assembling into words (native byte order) so
                    // far as there is more than one word per channel.
                    if (ARCHON_UNLIKELY(words_per_channel > 1 && word_order != byte_order))
                        return false;
                    words_per_channel *= bytes_per_word;
                    bits_per_word = core::int_width<char>();
                    word_order = byte_order;
                }
            }
            else if (ARCHON_LIKELY(words_per_channel == 1)) {
                bits_per_word = packed.bit_fields[0].width;
                word_order = {}; // Immaterial
            }
            int depth = words_per_channel * bits_per_word;
            // All channel widths must be equal, and be equal to `depth`
            for (int i = 0; i < num_channels; ++i) {
                image::BitField field = packed.bit_fields[i];
                if (ARCHON_UNLIKELY(field.width != depth))
                    return false;
            }
            format = {
                word_type,
                bits_per_word,
                words_per_channel,
                word_order,
                packed.channel_conf,
            };
            if (word_order == core::Endianness::big)
                format.channel_conf.reverse();
            return true;
        }
        case Type::subword: {
            if (ARCHON_UNLIKELY(!subword.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            if (word_type != subword.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // Cast to byte-based format
                int bytes_per_word = get_bytes_per_word(subword.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1)) {
                    // Cannot have unused bits
                    int bits_per_word = (subword.bits_per_channel *
                                         subword.channel_conf.get_num_channels() *
                                         subword.pixels_per_word);
                    int bits_per_byte = core::int_width<char>();
                    int n = bits_per_byte;
                    bool all_bits_used = (core::try_int_mul(n, bytes_per_word) && bits_per_word == n);
                    if (ARCHON_UNLIKELY(!all_bits_used))
                        return false;
                    // There must be a whole number of bytes per channel
                    int remainder = subword.bits_per_channel % bits_per_byte;
                    if (ARCHON_UNLIKELY(remainder != 0))
                        return false;
                    // Native byte order must be determinable
                    core::Endianness byte_order = {};
                    if (ARCHON_UNLIKELY(!try_get_byte_order(subword.word_type, byte_order)))
                        return false;
                    if (ARCHON_LIKELY(subword.pixels_per_word > 1)) {
                        // Native byte order must match bit order
                        if (ARCHON_UNLIKELY(byte_order != subword.bit_order))
                            return false;
                        // Cannot have word aligned rows
                        if (ARCHON_UNLIKELY(subword.word_aligned_rows))
                            return false;
                    }
                    int bits_per_word_2 = bits_per_byte;
                    int words_per_channel = bytes_per_word;
                    core::Endianness word_order = byte_order;
                    ChannelConf channel_conf = subword.channel_conf;
                    if (byte_order == core::Endianness::little)
                        channel_conf.reverse();
                    format = {
                        word_type,
                        bits_per_word_2,
                        words_per_channel,
                        word_order,
                        channel_conf,
                    };
                    return true;
                }
            }
            // If the subword format has more than one channel in a word, it is impossible
            // to express it as an integer format based on the same word type.
            int num_channels = subword.channel_conf.get_num_channels();
            if (ARCHON_UNLIKELY(num_channels > 1 || subword.pixels_per_word > 1))
                return false;
            int bits_per_word = subword.bits_per_channel;
            int words_per_channel = 1;
            core::Endianness word_order = {}; // Immaterial
            format = {
                word_type,
                bits_per_word,
                words_per_channel,
                word_order,
                subword.channel_conf,
            };
            return true;
        }
        case Type::float_: {
            break;
        }
        case Type::indexed: {
            break;
        }
    }

    return false;
}


// FIXME: Factor out casts to functions on respective format types                                        
bool BufferFormat::try_cast_to(PackedFormat& format, IntegerType word_type) const
{
    static_cast<void>(format);            
    static_cast<void>(word_type);            

    switch (type) {
        case Type::integer: {
            if (ARCHON_UNLIKELY(!integer.is_valid()))
                throw std::runtime_error("Invalid buffer format");
/*            if (word_type != integer.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // Cast to byte-based format
                int bytes_per_word = get_bytes_per_word(integer.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1)) {
                    
                }
            }

            

            return true;

            // Observations:
            // - An integer format can be expressed as a packed format based on the same word type and same number of bits per word by setting the number of words per bit compound to the number of words per channel times the number of channels, and setting the bit fields appropriately:      ALSO CONSIDER word_order vs reversal of channel order    
            // - Is this also possible if casting from non-byte integer format to byte-based packed format?                 
*/

            // FIXME: Implement this    
            ARCHON_STEADY_ASSERT_UNREACHABLE();                                       
        }
        case Type::packed: {
            if (ARCHON_UNLIKELY(!packed.is_valid()))
                throw std::runtime_error("Invalid buffer format");
/*
            if (word_type != packed.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // Cast to byte-based format
                int bytes_per_word = get_bytes_per_word(packed.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1)) {
                    // Cannot have unused bits in each word, and word type cannot have                                                                                                                                                
                    // padding bits if there is more than one word per bit compound.
                    int bits_per_byte = core::int_width<char>();
                    if (packed.words_per_pixel > 1) {
                        int n = bits_per_byte;
                        bool all_bits_used = (core::try_int_mul(n, bytes_per_word) && packed.bits_per_word == n);
                        if (ARCHON_UNLIKELY(!all_bits_used))
                            return false;
                    }
                    // Native byte order must be determinable
                    core::Endianness byte_order = {};
                    if (ARCHON_UNLIKELY(!try_get_byte_order(packed.word_type, byte_order)))
                        return false;
                    // When word order of original format matters, it must match the native
                    // byte order
                    if (ARCHON_UNLIKELY(packed.words_per_pixel > 1 && packed.word_order != byte_order))
                        return false;
                    format = packed;
                    format.word_type = IntegerType::byte;
                    format.bits_per_word = bits_per_byte;
                    format.words_per_pixel *= bytes_per_word;
                    format.word_order = byte_order;
                    return true;
                }
            }
            format = packed;
            format.word_type = word_type;
            return true;
*/
            // FIXME: Implement this    
            ARCHON_STEADY_ASSERT_UNREACHABLE();                                       
        }
        case Type::subword: {
            if (ARCHON_UNLIKELY(!subword.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            // FIXME: Implement this    
            ARCHON_STEADY_ASSERT_UNREACHABLE();                                       
        }
        case Type::float_: {
            break;
        }
        case Type::indexed: {
            break;
        }
    }

    return false;
}


bool BufferFormat::try_cast_to(SubwordFormat& format, IntegerType word_type) const
{
    switch (type) {
        case Type::integer: {
            if (ARCHON_UNLIKELY(!integer.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            if (word_type != integer.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // If there is more than one byte per word, it is impossible to cast to a
                // byte-base format since in a byte-based subword format, a channel cannot
                // span multiple bytes.
                int bytes_per_word = get_bytes_per_word(integer.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1))
                    return false;
            }
            // If the integer format has more than one word per pixel, it is impossible to
            // express it as a subword format based on the same word type.
            int num_channels = integer.channel_conf.get_num_channels();
            if (ARCHON_UNLIKELY(num_channels > 1 || integer.words_per_channel > 1))
                return false;
            int bits_per_channel = integer.bits_per_word;
            int pixels_per_word = 1;
            core::Endianness bit_order = {}; // Immaterial
            bool word_aligned_rows = {}; // Immaterial
            format = {
                word_type,
                bits_per_channel,
                pixels_per_word,
                bit_order,
                word_aligned_rows,
                integer.channel_conf,
            };
            return true;
        }
        case Type::packed: {
            if (ARCHON_UNLIKELY(!packed.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            // A packed format, that uses one word per compound, can be expressed as a
            // subword format based on the same (or a compatible) word type with one pixel
            // per word if the layout of the bit fields satisfy certain criteria: All gaps
            // must be zero. All widths must be equal.
            if (word_type != packed.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // A packed format cannot be expressed as a byte-based subword format unless
                // there is just one byte per word. This is because, in a byte-based subword
                // format, all bytes must have the meaning of their bits equally layed out.
                int bytes_per_word = get_bytes_per_word(packed.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1))
                    return false;
            }
            // If the packed format has more than one word per bit compound, it is
            // impossible to express it as a subword format based on the same word
            // type. This is because, in a subword format, all words must have the meaning
            // of their bits equally layed out.
            if (ARCHON_UNLIKELY(packed.words_per_pixel > 1))
                return false;
            // All bit fields must have the same width, and no gap after them.
            int num_channels = packed.channel_conf.get_num_channels();
            ARCHON_ASSERT(num_channels > 0);
            int bits_per_channel = packed.bit_fields[0].width;
            for (int i = 0; i < num_channels; ++i) {
                image::BitField field = packed.bit_fields[i];
                if (ARCHON_UNLIKELY(field.width != bits_per_channel || field.gap != 0))
                    return false;
            }
            int pixels_per_word = 1;
            core::Endianness bit_order = {}; // Immaterial
            bool word_aligned_rows = {}; // Immaterial
            format = {
                word_type,
                bits_per_channel,
                pixels_per_word,
                bit_order,
                word_aligned_rows,
                packed.channel_conf,
            };
            return true;
        }
        case Type::subword: {
            if (ARCHON_UNLIKELY(!subword.is_valid()))
                throw std::runtime_error("Invalid buffer format");
            if (word_type != subword.word_type) {
                // A word of any type can be accessed in terms of the bytes that make it
                // up. Any other type punning would cause undefined behavior.
                if (ARCHON_UNLIKELY(word_type != IntegerType::byte))
                    return false;
                // Cast to byte-based format
                int bytes_per_word = get_bytes_per_word(subword.word_type);
                if (ARCHON_UNLIKELY(bytes_per_word > 1)) {
                    // Cannot have unused bits
                    int bits_per_word = (subword.bits_per_channel *
                                         subword.channel_conf.get_num_channels() *
                                         subword.pixels_per_word);
                    int bits_per_byte = core::int_width<char>();
                    int n = bits_per_byte;
                    bool all_bits_used = (core::try_int_mul(n, bytes_per_word) && bits_per_word == n);
                    if (ARCHON_UNLIKELY(!all_bits_used))
                        return false;
                    // There must be a whole number of pixels per byte
                    int pixles_per_byte = subword.pixels_per_word / bytes_per_word;
                    int remainder       = subword.pixels_per_word % bytes_per_word;
                    if (ARCHON_UNLIKELY(remainder != 0))
                        return false;
                    // Native endianness must be determinable
                    core::Endianness byte_order = {};
                    if (ARCHON_UNLIKELY(!try_get_byte_order(subword.word_type, byte_order)))
                        return false;
                    // Native endianness must match bit order
                    if (ARCHON_UNLIKELY(byte_order != subword.bit_order))
                        return false;
                    // Cannot have word aligned rows
                    if (ARCHON_UNLIKELY(subword.word_aligned_rows))
                        return false;
                    format = subword;
                    format.word_type = IntegerType::byte;
                    format.pixels_per_word = pixles_per_byte;
                    return true;
                }
            }
            format = subword;
            format.word_type = word_type;
            return true;
        }
        case Type::float_: {
            break;
        }
        case Type::indexed: {
            break;
        }
    }

    return false;
}


bool BufferFormat::try_get_byte_order(IntegerType word_type, core::Endianness& byte_order) noexcept
{
    switch (word_type) {
        case IntegerType::byte:
            byte_order = core::Endianness::big;
            return true;
        case IntegerType::char_:
            return core::try_get_byte_order<char>(byte_order);
        case IntegerType::short_:
            return core::try_get_byte_order<short>(byte_order);
        case IntegerType::int_:
            return core::try_get_byte_order<int>(byte_order);
        case IntegerType::long_:
            return core::try_get_byte_order<long>(byte_order);
        case IntegerType::llong:
            return core::try_get_byte_order<long long>(byte_order);
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}
