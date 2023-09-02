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


#include <cstddef>
#include <memory>
#include <algorithm>
#include <utility>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/image.hpp>
#include <archon/image/reader.hpp>


using namespace archon;
using image::Reader;


auto Reader::determine_palette_size(const image::Image* palette) noexcept -> std::size_t
{
    if (ARCHON_LIKELY(!palette))
        return 0;

    image::Size palette_image_size = palette->get_size();
    std::size_t palette_size = 1;
    bool overflow = (!core::try_int_mul(palette_size, std::max(palette_image_size.width, 0)) ||
                     !core::try_int_mul(palette_size, std::max(palette_image_size.height, 0)));
    if (ARCHON_UNLIKELY(overflow))
        palette_size = core::int_max<std::size_t>();

    // Clamp palette size to available index range
    image::CompRepr index_repr = image::color_index_repr; // FIXME: Should be made varyable, and be provided through TransferInfo                            
    int index_bit_width = image::comp_repr_int_bit_width(index_repr);
    std::size_t max_index = core::int_mask<std::size_t>(std::min(index_bit_width, core::int_width<std::size_t>()));
    if (ARCHON_LIKELY(palette_size == 0 || max_index >= std::size_t(palette_size - 1)))
        return palette_size;
    return std::size_t(max_index + 1);
}


bool Reader::adjust(FalloffMode mode, int image_size, int& read_pos, int read_size, int& progen_pos,
                    int& progen_size) noexcept
{
    ARCHON_ASSERT(image_size > 0);
    ARCHON_ASSERT(read_size > 0);
    switch (mode) {
        case FalloffMode::background:
            if (ARCHON_LIKELY(read_pos >= 0)) {
                if (ARCHON_LIKELY(read_pos < image_size)) {
                    progen_pos = read_pos;
                    if (ARCHON_LIKELY(read_size <= image_size - read_pos)) {
                        // Already confined
                        progen_size = read_size;
                    }
                    else {
                        // Clip to right side / bottom of image
                        progen_size = image_size - read_pos;
                    }
                    return true;
                }
            }
            else {
                if (ARCHON_LIKELY(read_pos > -read_size)) {
                    progen_pos = 0;
                    if (ARCHON_LIKELY(read_pos + read_size <= image_size)) {
                        // Clip to left side / top of image
                        progen_size = read_pos + read_size;
                    }
                    else {
                        // Clip to (left and right side) / (top and bottom) of image
                        progen_size = image_size;
                    }
                    return true;
                }
            }
            // No overlap in one direction means no overlap at all, so nothing to read
            return false;
        case FalloffMode::edge:
            if (ARCHON_LIKELY(read_pos >= 0)) {
                if (ARCHON_LIKELY(read_pos < image_size)) {
                    progen_pos = read_pos;
                    if (ARCHON_LIKELY(read_size <= image_size - read_pos)) {
                        // Already confined
                        progen_size = read_size;
                    }
                    else {
                        // Clip to right side / bottom of image
                        progen_size = image_size - read_pos;
                    }
                }
                else {
                    // Slide left to obtain a 1-pixel overlap
                    read_pos = image_size - 1;
                    progen_pos = read_pos;
                    progen_size = 1;
                }
            }
            else {
                if (ARCHON_LIKELY(read_pos > -read_size)) {
                    progen_pos = 0;
                    if (ARCHON_LIKELY(read_pos + read_size <= image_size)) {
                        // Clip to left side / top of image
                        progen_size = read_pos + read_size;
                    }
                    else {
                        // Clip to (left and right side) / (top and bottom) of image
                        progen_size = image_size;
                    }
                }
                else {
                    // Slide right to obtain a 1-pixel overlap
                    read_pos = 1 - read_size;
                    progen_pos = 0;
                    progen_size = 1;
                }
            }
            return true;
        case FalloffMode::repeat:
            int rem = core::int_periodic_mod(read_pos, image_size);
            if (read_size <= image_size - rem) {
                // The read box is confined to a single repetition module in this direction,
                // so shift the read box by an integer number of repetition modules to bring
                // it into the principal repetition module, i.e., the image.
                read_pos = rem;
                // The progenitor sub-box must coincide with the read box in this case, and
                // all of the progenitor sub-box goes into a single quadrant in this
                // direction.
                progen_pos = read_pos;
                progen_size = read_size;
            }
            else if (read_size - (image_size - rem) >= image_size) {
                // The read box covers an entire repetition module in this direction, so
                // shift the read box by an integer number of repetition modules such that
                // one of the covered modules is the principal module, i.e., the image.
                read_pos = rem - image_size;
                // Now, set the progenitor sub-box to be the read box clipped the image, and
                // make all of the progenitor sub-box go into a single quadrant in this
                // direction.
                progen_pos = 0;
                progen_size = image_size;
            }
            else {
                // The read box is not confined to a single repetition module in this
                // direction, and it also does also entirely cover any single repetition
                // module. This means that the progenitor sub-box needs to be split in this
                // direction (into quadrants). Shift the left side / top of the read box
                // into the principal repetition module, i.e., the image, so as to ensure
                // that the left side / top of the progenitor sub-box falls inside the
                // principal repetition module as required.
                read_pos = rem;
                progen_pos = read_pos;
                progen_size = std::min(read_size, image_size);
            }
            return true;
    }
    ARCHON_ASSERT_UNREACHABLE();
    return false;
}


void Reader::delete_color_slots() noexcept
{
    repr_dispatch_nothrow([&](auto tag) noexcept {
        constexpr image::CompRepr repr = tag.comp_repr;
        using comp_type = image::comp_type<repr>;
        delete[] static_cast<comp_type*>(m_color_slots_r);
        delete[] static_cast<comp_type*>(m_color_slots_u);
    });
}


void Reader::init_color_slot_f(ColorSlot slot)
{
    ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    ARCHON_ASSERT(!ctrl.have_neutral);
    if (ARCHON_LIKELY(ctrl.have_unrestricted_native))
        goto convert;
    // Note: set_default_color() clobbers the primary workspace buffer
    set_default_color(slot); // Throws
    if (ARCHON_LIKELY(ctrl.have_unrestricted_native))
        goto convert;
    ARCHON_ASSERT(ctrl.have_neutral);
    return;

  convert:
    ensure_color_slots_f(); // Throws
    repr_dispatch([&](auto tag) noexcept {
        constexpr image::CompRepr repr = tag.comp_repr;
        constexpr image::CompRepr float_repr = image::CompRepr::float_;
        using comp_type = image::comp_type<repr>;
        comp_type* origin = get_color_slot_u<repr>(slot);
        image::float_type* destin = get_color_slot_f(slot);
        bool has_alpha = true;
        image::comp_repr_convert<repr, float_repr>(origin, destin, m_num_channels_ext, has_alpha);
    });
    ctrl.have_neutral = true;
}


void Reader::alloc_color_slots_f()
{
    ARCHON_ASSERT(!m_color_slots_f);
    std::size_t size = 1;
    core::int_mul(size, m_num_channels_ext); // Throws
    core::int_mul(size, s_num_color_slots); // Throws
    m_color_slots_f = std::make_unique<image::float_type[]>(size); // Throws
}


void Reader::set_default_color(ColorSlot slot)
{
    const ColorSlotCtrl& ctrl = get_color_slot_ctrl(slot);
    ARCHON_ASSERT( !ctrl.have_neutral && !ctrl.have_restricted_native && !ctrl.have_unrestricted_native);

    // The default background color, transparent, is chosen such that all channels are at
    // zero intensity assuming RGBA. This choice was made because it causes the operation of
    // filling with the default background to be equivalent to clearing an RGBA memory
    // buffer to all zeros.
    //
    // For the sake of symmetry, the default foreground color, white, is chosen such that
    // all channels are at maximum intensity assuming RGBA.
    //
    util::Color color = util::colors::transparent;
    switch (slot) {
        case ColorSlot::background:
            break;
        case ColorSlot::foreground:
            color = util::colors::white;
            break;
    }
    // Note: do_set_color() clobbers the primary workspace buffer.
    auto color_2 = image::Pixel(color);
    constexpr image::CompRepr comp_repr = color_2.comp_repr;
    image::float_type opacity = 1;
    do_set_color<comp_repr>(slot, color_2.data(), color_2.get_color_space(), color_2.has_alpha, opacity); // Throws
}


void Reader::instantiate_palette_cache_f()
{
    ARCHON_ASSERT(m_palette);
    ARCHON_ASSERT(!m_palette_cache_f);

    std::unique_ptr<image::float_type[]> float_components;

    // Dispatch to representation used in palette
    repr_dispatch([&](auto tag) {
        int k = m_num_channels_ext;
        constexpr image::CompRepr repr = tag.comp_repr;
        using comp_type = image::comp_type<repr>;
        const comp_type* source = ensure_palette_cache<repr>(); // Throws
        std::size_t n = m_palette_size;
        std::size_t m = std::size_t(n * k);
        float_components = std::make_unique<image::float_type[]>(m); // Throws
        image::float_type* destin = float_components.get();
        for (std::size_t i = 0; i < n; ++i) {
            if constexpr (repr == image::CompRepr::float_) {
                std::copy_n(source, std::size_t(k), destin);
            }
            else {
                // Premultiply alpha
                constexpr int bit_width = image::comp_repr_bit_width<repr>();
                int last = k - 1;
                image::float_type alpha = image::int_to_float<bit_width, image::float_type>(source[last]);
                for (int i = 0; i < last; ++i)
                    destin[i] = alpha * image::compressed_int_to_float<bit_width>(source[i]);
                destin[last] = alpha;
            }
            source += k;
            destin += k;
        }
    }); // Throws

    m_palette_cache_f = std::move(float_components);
}


void Reader::instantiate_palette_cache()
{
    ARCHON_ASSERT(m_palette);
    ARCHON_ASSERT(!m_palette_cache);

    image::Reader palette_reader(*m_palette);
    image::Size palette_image_size = palette_reader.get_image_size();

    std::size_t buffer_size = 1;
    core::int_mul(buffer_size, m_num_channels_ext); // Throws
    core::int_mul(buffer_size, m_palette_size); // Throws

    // Dispatch to representation used in palette
    repr_dispatch([&](auto tag) {
        constexpr image::CompRepr repr = tag.comp_repr;
        using comp_type = image::comp_type<repr>;
        std::unique_ptr<comp_type[]> palette_entries = std::make_unique<comp_type[]>(buffer_size); // Throws
        int num_full_rows = int(m_palette_size / palette_image_size.width);
        std::ptrdiff_t horz_stride = m_num_channels_ext;
        std::ptrdiff_t vert_stride = palette_image_size.width * m_num_channels_ext;
        image::Iter iter = { palette_entries.get(), horz_stride, vert_stride };
        image::Tray tray_1 = { iter, image::Size(palette_image_size.width, num_full_rows) };
        bool has_alpha = true;
        palette_reader.get_block_a<repr>(image::Pos(0, 0), tray_1, *m_transfer_info.color_space, has_alpha); // Throws
        int remain = int(m_palette_size - num_full_rows * std::size_t(palette_image_size.width));
        if (remain > 0) {
            image::Tray tray_2 = { iter + image::Size(0, num_full_rows), image::Size(remain, 1) };
            palette_reader.get_block_a<repr>(image::Pos(0, num_full_rows), tray_2, *m_transfer_info.color_space,
                                             has_alpha); // Throws
        }
        m_palette_cache = palette_entries.release();
    }); // Throws
}


void Reader::delete_palette_cache() noexcept
{
    repr_dispatch_nothrow([&](auto tag) noexcept {
        constexpr image::CompRepr repr = tag.comp_repr;
        using comp_type = image::comp_type<repr>;
        delete[] static_cast<comp_type*>(m_palette_cache);
    });
}
