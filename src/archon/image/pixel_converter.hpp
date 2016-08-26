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

/// \file
///
/// \author Kristian Spangsege

#ifndef ARCHON_IMAGE_PIXEL_CONVERTER_HPP
#define ARCHON_IMAGE_PIXEL_CONVERTER_HPP

#include <stdexcept>
#include <algorithm>
#include <vector>

#include <archon/core/types.hpp>
#include <archon/core/memory.hpp>
#include <archon/image/color_space.hpp>


namespace archon {
namespace image {

class TransferFormat {
public:
    TransferFormat()
    {
    }
    TransferFormat(const ColorSpace* c, bool has_alpha, WordType t):
        color_space(c),
        has_alpha(has_alpha),
        word_type(t)
    {
    }

    const ColorSpace* color_space;
    bool has_alpha;
    WordType word_type;

    bool operator==(const TransferFormat& f) const
    {
        return color_space == f.color_space && has_alpha == f.has_alpha && word_type == f.word_type;
    }

    bool operator!=(const TransferFormat& f) const
    {
        return color_space != f.color_space || has_alpha != f.has_alpha || word_type != f.word_type;
    }
};


/// This class is used to translate pixel data from one transfer format to
/// another.
class PixelConverter {
public:
    /// This class descibes a specific pixel transfer format. A pixel transfer
    /// format is comprised of a color space, a flag for the presence of an
    /// alpha channel, and a word type specifier. The set of all possible pixel
    /// transfer formats is essentially a subset of the set of all possible
    /// pixel buffer formats.
    class Format {
    public:
        Format(const ColorSpace* color_space, bool has_alpha, WordType word_type);
        Format(const TransferFormat&);

        const TransferFormat format;
        const int num_channels, bytes_per_word, bytes_per_pixel;
    };

    /// This class provides a set of buffers that will be allocated
    /// just-in-time.
    class Buffers {
    public:
        Buffers(std::size_t size_of_buffers):
            m_size_of_buffers{size_of_buffers}
        {
        }
        std::size_t get_size_of_buffers() const
        {
            return m_size_of_buffers;
        }
        char* get_first()
        {
            if (!m_first)
                m_first = std::make_unique<char[]>(m_size_of_buffers); // Throws
            return m_first.get();
        }
        char* get_second()
        {
            if (!m_second)
                m_second = std::make_unique<char[]>(m_size_of_buffers); // Throws
            return m_second.get();
        }

    private:
        std::size_t m_size_of_buffers;
        std::unique_ptr<char[]> m_first, m_second;
    };


    /// Construct an uninitialized pixel converter. You will need to call one of
    /// the \c init methods before using it.
    PixelConverter()
    {
    }


    /// Construct an initialized pixel converter.
    PixelConverter(const Format& source, const Format& target, Buffers& buffers);


    class Manipulator {
    public:
        virtual void manip(const char* source, char* target, std::size_t n) const = 0;
        virtual ~Manipulator() {}
    };

    class ConvSpec {
    public:
        const Manipulator* cvt;
        TransferFormat src_fmt, tgt_fmt;
        ConvSpec()
        {
        }
        ConvSpec(const Manipulator* c, const TransferFormat& s, const TransferFormat& t):
            cvt(c),
            src_fmt(s),
            tgt_fmt(t)
        {
        }
    };


    void init(const Format& source, const Format& target, Buffers& buffers,
              const ConvSpec* converters = 0, int num_converters = 0);


    bool is_noop() const
    {
        return converters.empty();
    }

    int get_source_pixel_size() const
    {
        return source_pixel_size;
    }

    int get_target_pixel_size() const
    {
        return target_pixel_size;
    }


    void operator()(const char* source, char* target, std::size_t n) const
    {
        char* src = buffer2;
        char* tgt = buffer1;
        int first = int(converters.size()) - 1;
        for (int i = first; 0 <= i; --i) {
            bool not_first = i < first;
            bool not_last  = 0 < i;
            const char* s = not_first ? src : source;
            char*       t = not_last  ? tgt : target;
            const ConvUnion& c = converters[i];
            switch (c.any.type) {
                case conv_WordType:
                    (*c.word_type.cvt)(s, t, n*c.word_type.num_channels);
                    break;
                case conv_ColorSpace:
                    c.color_space.cvt->cvt(s,t,n);
                    break;
                case conv_Custom:
                    c.custom.cvt->manip(s,t,n);
                    break;
            }
            if (not_last)
                std::swap(src, tgt);
        }
    }


    /// Result is unreliable until after <tt>ensure_internal_source</tt> has
    /// been called.
    char* get_internal_source() const
    {
        return buffer2;
    }


    /// Result is unreliable until after <tt>ensure_internal_target</tt> has
    /// been called.
    char* get_internal_target() const
    {
        return (internal_target_is_buffer1 ? buffer1 : buffer2);
    }


    /// Must not be called for a noop conversion.
    ///
    /// Should be called by the application if it needs to use an internal
    /// buffer for passing input to the converter.
    void ensure_internal_source(Buffers& buffers)
    {
        if (!buffer2)
            buffer2 = buffers.get_second();
    }


    /// Must not be called for a noop conversion.
    ///
    /// Should be called by the application if it needs the output from the
    /// converter to be made available in an internal buffer.
    void ensure_internal_target(Buffers& buffers)
    {
        char*& b = (internal_target_is_buffer1 ? buffer1 : buffer2);
        if (!b)
            b = (internal_target_is_buffer1 ? buffers.get_first() : buffers.get_second());
    }


    /// Returns zero if the number of conversion steps is less than 2, because
    /// then there are no intermediate pixel formats.
    int get_max_intermediate_pixel_size() const
    {
        return max_intermediate_pixel_size;
    }


private:
    bool add_cvt_step(const Format& source, const Format& target);


    void update_max_intermediate_pixel_size(int s)
    {
        if (max_intermediate_pixel_size < s)
            max_intermediate_pixel_size = s;
    }


    using CvtMethod = void (PixelConverter::*)(const char* source, char* target, std::size_t n) const;

    enum ConvType {
        conv_WordType,
        conv_ColorSpace,
        conv_Custom
    };

    struct ConvAny {
        ConvType type;
    };

    struct ConvWordType {
        ConvType type;
        int num_channels;
        WordTypeConverter cvt;
    };

    struct ConvColorSpace {
        ConvType type;
        ColorSpace::Converter const *cvt;
    };

    struct ConvCustom {
        ConvType type;
        Manipulator const *cvt;
    };

    union ConvUnion {
        ConvAny any;
        ConvWordType word_type;
        ConvColorSpace color_space;
        ConvCustom custom;
        ConvUnion(int num_channels, WordTypeConverter cvt)
        {
            word_type.type         = conv_WordType;
            word_type.num_channels = num_channels;
            word_type.cvt          = cvt;
        }
        ConvUnion(const ColorSpace::Converter* cvt)
        {
            color_space.type = conv_ColorSpace;
            color_space.cvt  = cvt;
        }
        ConvUnion(const Manipulator* cvt)
        {
            custom.type = conv_Custom;
            custom.cvt  = cvt;
        }
    };


    std::vector<ConvUnion> converters; // In reverse order
    int source_pixel_size, target_pixel_size;
    int max_intermediate_pixel_size; // The maximum pixel size over all intermediate pixel formats stored in one of the internal buffers.
    char* buffer1;
    char* buffer2;
    bool internal_target_is_buffer1;
};




// Implementation

inline PixelConverter::Format::Format(const ColorSpace* color_space,
                                      bool has_alpha, WordType word_type):
    format(color_space, has_alpha, word_type),
    num_channels(color_space->get_num_primaries() + (has_alpha?1:0)),
    bytes_per_word(get_bytes_per_word(word_type)),
    bytes_per_pixel(num_channels * bytes_per_word)
{
}

inline PixelConverter::Format::Format(const TransferFormat& f):
    format(f),
    num_channels(f.color_space->get_num_primaries() + (f.has_alpha?1:0)),
    bytes_per_word(get_bytes_per_word(f.word_type)),
    bytes_per_pixel(num_channels * bytes_per_word)
{
}

inline PixelConverter::PixelConverter(const Format& s, const Format& t, Buffers& b)
{
    init(s,t,b);
}

inline void PixelConverter::init(const Format& src, const Format& tgt, Buffers& buffers,
                                 const ConvSpec* convs, int num_convs)
{
    source_pixel_size = src.bytes_per_pixel;
    target_pixel_size = tgt.bytes_per_pixel;
    max_intermediate_pixel_size = 0;
    converters.clear();
    if (num_convs < 1) {
        add_cvt_step(src, tgt);
    }
    else {
        const ConvSpec* c = convs + (num_convs-1);
        {
            Format s(c->tgt_fmt);
            if (add_cvt_step(s, tgt))
                update_max_intermediate_pixel_size(s.bytes_per_pixel);
        }
        for (;;) {
            converters.push_back(c->cvt); // The custom converter
            if (c == convs)
                break;
            const ConvSpec* d = c--;
            Format s(c->tgt_fmt), t(d->src_fmt);
            if (add_cvt_step(s,t))
                update_max_intermediate_pixel_size(t.bytes_per_pixel);
            update_max_intermediate_pixel_size(s.bytes_per_pixel);
        }
        {
            Format t(c->src_fmt);
            if (add_cvt_step(src, t))
                update_max_intermediate_pixel_size(t.bytes_per_pixel);
        }
    }

    int n = converters.size();
    if (0 < n) {
        internal_target_is_buffer1 = n & 1;
        if (1 < n) {
            buffer1 = buffers.get_first();
            buffer2 = (2 < n ? buffers.get_second() : 0);
        }
        else {
            buffer1 = buffer2 = 0;
        }
    }
}


// Conversion steps must be added in reverse order. Returns true if conversion
// was needed.
inline bool PixelConverter::add_cvt_step(const Format& s, const Format& t)
{
    ColorSpace::AlphaType alpha;
    if (t.format.word_type == s.format.word_type) {
        if (  (s.format.color_space == t.format.color_space &&
               s.format.has_alpha == t.format.has_alpha))
            return false;

        alpha = ColorSpace::get_alpha_type(s.format.has_alpha, t.format.has_alpha);

        // We have a short cut way if we can convert the source color space to
        // the target color space directly. The word type would have to be the
        // common word type of the source and target formats.
        const ColorSpace::Converter* c =
            s.format.color_space->to_any(t.format.color_space, s.format.word_type, alpha);
        if (c) {
            converters.push_back(c);
            return true;
        }
    }
    else { // t.word_type != s.word_type
        alpha = ColorSpace::get_alpha_type(s.format.has_alpha, t.format.has_alpha);

        if (s.format.color_space == t.format.color_space) {
            if (alpha != ColorSpace::alpha_Merge) {
                if (alpha == ColorSpace::alpha_Add) {
                    // In this case there is no internal conversion to floating
                    // point format in the color space conversion
                    converters.push_back(&s.format.color_space->to_self(t.format.word_type,
                                                                        ColorSpace::alpha_Add));
                    update_max_intermediate_pixel_size(s.num_channels * t.bytes_per_word);
                }
                WordTypeConverter cvt =
                    get_word_type_frac_converter(s.format.word_type, t.format.word_type);
                converters.push_back(ConvUnion(s.num_channels, cvt));
                return true;
            }
        }
    }

    // Find the most appropriate floating point type for color space conversion.
    bool source_is_float = is_floating_point(s.format.word_type);
    bool target_is_float = is_floating_point(t.format.word_type);

    WordType inter_float;
    if (target_is_float != source_is_float) {
        // One is a float, the other is not
        inter_float = source_is_float ? s.format.word_type : t.format.word_type;
    }
    else if(target_is_float) {
        // Both are floats
        inter_float = std::min(s.format.word_type, t.format.word_type);
    }
    else {
        // Neither is a float
        inter_float = get_smallest_float_cover(std::min(s.format.word_type, t.format.word_type));
    }
    int inter_float_size = get_bytes_per_word(inter_float);

    // We consider the conversions in reverse order such that we can know
    // whether a particular conversion is the last one. This is important in the
    // determination of the maximum intermediate pixels size.
    bool prev_can_be_last = true;
    if (inter_float != t.format.word_type) {
        WordTypeConverter cvt =
            get_word_type_frac_converter(inter_float, t.format.word_type);
        converters.push_back(ConvUnion(t.num_channels, cvt));
        prev_can_be_last = false;
    }

    // We might have the option of converting directly from the source to the
    // target color space, but we have already checked for this in the case
    // where the source and target word types are the same.
    {
        const ColorSpace::Converter* cvt;
        if (  (t.format.word_type != s.format.word_type &&
               (cvt = s.format.color_space->to_any(t.format.color_space, inter_float, alpha)))) {
            converters.push_back(cvt);
            if (prev_can_be_last) {
                prev_can_be_last = false;
            }
            else {
                update_max_intermediate_pixel_size(t.num_channels * inter_float_size);
            }
        }
        else {
            // We know at this point that neither of the two color spaces are
            // RGB, because we have tried earlier to acquire a direct converter,
            // but without success.
            converters.push_back(&t.format.color_space->
                                 from_rgb(inter_float, s.format.has_alpha ? t.format.has_alpha ?
                                          ColorSpace::alpha_Keep : ColorSpace::alpha_No : alpha));
            converters.push_back(&s.format.color_space->
                                 to_rgb(inter_float, t.format.has_alpha ? s.format.has_alpha ?
                                        ColorSpace::alpha_Keep : ColorSpace::alpha_No : alpha));
            if (prev_can_be_last) {
                prev_can_be_last = false;
            }
            else {
                int n = std::max(t.num_channels, alpha == ColorSpace::alpha_Keep ? 4 : 3);
                update_max_intermediate_pixel_size(n * inter_float_size);
            }
        }
    }

    if (inter_float != s.format.word_type) {
        WordTypeConverter cvt = get_word_type_frac_converter(s.format.word_type, inter_float);
        converters.push_back(ConvUnion(s.num_channels, cvt));
        if (!prev_can_be_last)
            update_max_intermediate_pixel_size(s.num_channels * inter_float_size);
    }

    return true;
}

} // namespace image
} // namespace archon

#endif // ARCHON_IMAGE_PIXEL_CONVERTER_HPP
