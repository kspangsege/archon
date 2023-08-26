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
#include <utility>
#include <array>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/math.hpp>
#include <archon/core/buffer.hpp>
#include <archon/util/kdtree.hpp>
#include <archon/image/size.hpp>
#include <archon/image/pos.hpp>
#include <archon/image/box.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/bit_medium.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/blend.hpp>
#include <archon/image/image.hpp>
#include <archon/image/impl/workspace.hpp>
#include <archon/image/reader.hpp>
#include <archon/image/writer.hpp>


using namespace archon;
using image::Writer;


auto Writer::fill(const image::Box& area, ColorSlot slot) -> Writer&
{
    image::Box box = area;
    image::Box boundary = { get_image_size() };
    if (ARCHON_UNLIKELY(!boundary.clip(box)))
        return *this;

    bool is_solid = is_solid_color(slot); // Throws
    image::float_type opacity = get_opacity();
    bool no_blending = (!get_blending_enabled() || (is_solid && opacity == 1));
    if (ARCHON_LIKELY(no_blending)) {
        bool direct_color = !has_indexed_color();
        if (ARCHON_LIKELY(direct_color)) {
            // Alternative: No blending, direct color
            repr_dispatch([&](auto tag) {
                constexpr image::CompRepr repr = tag.comp_repr;
                using comp_type = image::comp_type<repr>;
                if (ARCHON_LIKELY(opacity == 1)) {
                    // NOTE: ensure_color_slot_r() and ensure_color_slot_u() clobber the
                    // primary workspace buffer.
                    const comp_type* color;
                    if (!has_alpha_channel()) {
                        color = ensure_color_slot_r<repr>(slot); // Throws
                    }
                    else {
                        color = ensure_color_slot_u<repr>(slot); // Throws
                    }
                    get_image().fill(box, color); // Throws
                    return;
                }
                // NOTE: ensure_color_slot_f() clobbers the primary workspace buffer
                const image::float_type* color = ensure_color_slot_f(slot); // Throws
                int num_channels = get_num_channels();
                std::array<image::float_type, s_default_workspace_seed_size> seed_mem_1;
                impl::Workspace<image::float_type> workspace_1(seed_mem_1, m_workspace_buffer_1,
                                                               num_channels); // Throws
                // FIXME: Explain sanity of sometimes ignoring the alpha component here                        
                for (int i = 0; i < num_channels; ++i)
                    workspace_1[i] = opacity * color[i];
                std::array<comp_type, s_default_workspace_seed_size> seed_mem_2;
                impl::Workspace<comp_type> workspace_2(seed_mem_2, m_workspace_buffer_2, num_channels); // Throws
                constexpr image::CompRepr float_repr = image::CompRepr::float_;
                image::comp_repr_convert<float_repr, repr>(workspace_1.data(), workspace_2.data(), num_channels,
                                                           has_alpha_channel()); // Throws
                get_image().fill(box, workspace_2.data()); // Throws
            }); // Throws
            return *this;
        }

        // Alternative: No blending, indirect color
        //
        // NOTE: ensure_color_slot_f() clobbers the primary workspace buffer
        const image::float_type* color = ensure_color_slot_f(slot); // Throws
        std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
        int num_channels_ext = m_num_channels_ext;
        impl::Workspace<image::float_type> workspace(seed_mem, m_workspace_buffer_1, num_channels_ext); // Throws
        for (int i = 0; i < num_channels_ext; ++i)
            workspace[i] = opacity * color[i];
        ensure_palette_kdtree(); // Throws
        std::size_t index_1 = do_reverse_palette_lookup(workspace.data()); // Throws
        // No overlow is possible here because palette size is clamped to available range of
        // index representation.
        //
        // FIXME: Index representation should be made varyable                        
        auto index_2 = image::unpacked_comp_type<image::color_index_repr>(index_1);
        image::comp_type<image::color_index_repr> index_3 = image::comp_repr_pack<image::color_index_repr>(index_2);
        get_image().fill(box, &index_3); // Throws
        return *this;
    }

    // Alternative: Blending
    //
    // NOTE: ensure_color_slot_f() clobbers the primary workspace buffer
    const image::float_type* color = ensure_color_slot_f(slot); // Throws
    int num_channels_ext = m_num_channels_ext;
    // Using secondary and tertiary workspace buffers here because read() and write()
    // blobber the primary workspace buffer.
    std::array<image::float_type, s_default_workspace_seed_size> seed_mem;
    impl::Workspace<image::float_type> workspace_1(seed_mem, m_workspace_buffer_2, num_channels_ext); // Throws
    for (int i = 0; i < num_channels_ext; ++i)
        workspace_1[i] = opacity * color[i];
    subdivide(box, [&](const image::Box& subbox) {
        impl::Workspace<image::float_type> workspace_2(m_workspace_buffer_3, num_channels_ext, subbox.size); // Throws
        image::Tray tray = workspace_2.tray(num_channels_ext, subbox.size);
        constexpr image::CompRepr float_repr = image::CompRepr::float_;
        bool ensure_alpha = true;
        read<float_repr>(subbox.pos, tray, ensure_alpha); // Throws
        for (int y = 0; y < tray.size.height; ++y) {
            for (int x = 0; x < tray.size.width; ++x) {
                const image::float_type* left = workspace_1.data();
                const image::float_type* right = tray(x, y);
                image::float_type* destin = tray(x, y);
                auto mode = image::BlendMode::over;
                image::blend(left, right, destin, num_channels_ext, mode); // Throws
            }
        }
        write(subbox.pos, tray); // Throws
    }); // Throws
    return *this;
}


auto Writer::put_image_a(image::Pos pos, image::Reader& reader, image::Box box) -> Writer&
{
    image::Box destin_box = { pos, box.size };
    image::Box boundary = { get_image_size() };
    if (ARCHON_UNLIKELY(!boundary.clip(destin_box)))
        return *this;

    image::Image::TransferInfo origin_info = reader.get_transfer_info();
    image::Image::TransferInfo destin_info = get_transfer_info();
    bool same_comp_repr = (origin_info.comp_repr == destin_info.comp_repr);
    bool same_color_space = (origin_info.color_space == destin_info.color_space);
    bool remove_alpha = (origin_info.has_alpha && !destin_info.has_alpha);
    bool is_float = (destin_info.comp_repr == image::CompRepr::float_);
    image::float_type opacity = get_opacity();
    bool blending = (get_blending_enabled() && (origin_info.has_alpha || opacity != 1));
    bool is_indexed = has_indexed_color();
    bool lossless = (same_comp_repr && same_color_space && (!remove_alpha || is_float) && opacity == 1 && !blending &&
                     !is_indexed);
    if (ARCHON_LIKELY(lossless)) {
        repr_dispatch([&](auto tag) {
            subdivide(destin_box, [&](const image::Box& destin_subbox) {
                // FIXME: Explain sanity of having the extracted alpha channel ignored when (remove_alpha && is_float)                                                                                       
                image::Size displacement = destin_subbox.pos - pos;
                image::Pos origin_subpos = box.pos + displacement;
                core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
                int num_channels = get_num_channels() + int(remove_alpha);
                bool ensure_alpha = has_alpha_channel();
                constexpr image::CompRepr repr = tag.comp_repr;
                using comp_type = image::comp_type<repr>;
                impl::Workspace<comp_type> workspace(buffer, num_channels, destin_subbox.size); // Throws
                image::Tray tray = workspace.tray(num_channels, destin_subbox.size);
                reader.read_e<repr>(origin_subpos, tray, ensure_alpha); // Throws
                get_image().write(destin_subbox.pos, tray); // Throws
            }); // Throws
        }); // Throws
        return *this;
    }

    subdivide(destin_box, [&](const image::Box& destin_subbox) {
        image::Size displacement = destin_subbox.pos - pos;
        image::Pos origin_subpos = box.pos + displacement;
        // Using tertiary workspace buffer becasue write_b() clobbers primary and secondary
        // workspace buffers.
        core::Buffer<std::byte>& buffer = m_workspace_buffer_3;
        int num_channels_ext = m_num_channels_ext;
        impl::Workspace<image::float_type> workspace(buffer, num_channels_ext, destin_subbox.size); // Throws
        image::Tray tray = workspace.tray(num_channels_ext, destin_subbox.size);
        bool has_alpha = true;
        constexpr image::CompRepr float_repr = image::CompRepr::float_;
        reader.read_g<float_repr>(origin_subpos, tray, *destin_info.color_space, has_alpha); // Throws
        if (ARCHON_UNLIKELY(opacity != 1)) {
            std::size_t n = workspace.size();
            for (std::size_t i = 0; i < n; ++i)
                workspace[i] *= opacity;
        }
        write_b(destin_subbox.pos, tray); // Throws
    }); // Throws
    return *this;
}


auto Writer::put_block_mask(image::Pos pos, const const_int8_tray_type& tray) -> Writer&
{
    constexpr image::CompRepr mask_comp_repr = image::CompRepr::int8; // FIXME: Should be made variable         

    image::Box box = { pos, tray.size };
    image::Box boundary = { get_image_size() };
    if (ARCHON_UNLIKELY(!boundary.clip(box)))
        return *this;

    // NOTE: ensure_color_slot_f() clobbers primary workspace buffer
    const image::float_type* bg = ensure_color_slot_f(ColorSlot::background); // Throws
    const image::float_type* fg = ensure_color_slot_f(ColorSlot::foreground); // Throws

    subdivide(box, [&](const image::Box& subbox) {
        // Using tertiary workspace buffer becasue write_b() clobbers primary and secondary
        // workspace buffers.
        core::Buffer<std::byte>& buffer = m_workspace_buffer_3;
        int num_channels_ext = m_num_channels_ext;
        impl::Workspace<image::float_type> workspace(buffer, num_channels_ext, subbox.size); // Throws
        image::Tray tray_1 = tray.subtray(subbox, pos);
        image::Tray tray_2 = workspace.tray(num_channels_ext, subbox.size);
        image::float_type fg_alpha = fg[num_channels_ext - 1];
        image::float_type opacity = m_opacity;
        for (int y = 0; y < subbox.size.height; ++y) {
            for (int x = 0; x < subbox.size.width; ++x) {
                using mask_comp_type = image::comp_type<mask_comp_repr>;
                const mask_comp_type* origin = tray_1(x, y);
                image::float_type* destin = tray_2(x, y);
                // destin = opacity * ((mask * fg) + bg)
                mask_comp_type val = origin[0];
                image::float_type mask = image::alpha_comp_to_float<mask_comp_repr>(val); // Throws
                image::float_type alpha = mask * fg_alpha;
                image::float_type beta = (1 - alpha);
                for (int i = 0; i < num_channels_ext; ++i) {
                    image::float_type source = mask * fg[i];
                    destin[i] = opacity * (source + beta * bg[i]);
                }
            }
        }
        write_b(subbox.pos, tray_2); // Throws
    }); // Throws

    return *this;
}


void Writer::write_b(image::Pos pos, const image::Tray<const image::float_type>& tray)
{
    if (ARCHON_LIKELY(!m_blending_enabled)) {
        // Note: write() clobbers the primary workspace buffer
        write(pos, tray); // Throws
        return;
    }

    // Using the secondary workspace buffer because read() and write() clobber the primary
    // workspace buffer.
    core::Buffer<std::byte>& buffer = m_workspace_buffer_2;
    int num_channels_ext = m_num_channels_ext;
    impl::Workspace<image::float_type> workspace(buffer, num_channels_ext, tray.size); // Throws
    image::Tray tray_2 = workspace.tray(num_channels_ext, tray.size);
    bool ensure_alpha = true;
    constexpr image::CompRepr float_repr = image::CompRepr::float_;
    read<float_repr>(pos, tray_2, ensure_alpha); // Throws
    for (int y = 0; y < tray.size.height; ++y) {
        for (int x = 0; x < tray.size.width; ++x) {
            const image::float_type* left = tray(x, y);
            const image::float_type* right = tray_2(x, y);
            image::float_type* destin = tray_2(x, y);
            auto mode = image::BlendMode::over;
            image::blend(left, right, destin, num_channels_ext, mode); // Throws
        }
    }
    // FIXME: Explain how it makes sense that alpha channel may be ignored here                                
    write(pos, tray_2); // Throws
}


void Writer::write(image::Pos pos, const image::Tray<const image::float_type>& tray)
{
    bool direct_color = !has_indexed_color();
    if (ARCHON_LIKELY(direct_color)) {
        if (ARCHON_LIKELY(get_comp_repr() != image::CompRepr::float_)) {
            repr_dispatch([&](auto tag) {
                constexpr image::CompRepr repr = tag.comp_repr;
                using comp_type = image::comp_type<repr>;
                core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
                int num_channels = get_num_channels();
                impl::Workspace<comp_type> workspace(buffer, num_channels, tray.size); // Throws
                image::Tray tray_2 = workspace.tray(num_channels, tray.size);
                constexpr image::CompRepr float_repr = image::CompRepr::float_;
                bool origin_has_alpha = true;
                bool destin_has_alpha = has_alpha_channel();
                convert_1<float_repr, repr>(tray, origin_has_alpha, tray_2.iter, destin_has_alpha); // Throws
                get_image().write(pos, tray_2); // Throws
            }); // Throws
            return;
        }

        get_image().write(pos, tray); // Throws
        return;
    }

    constexpr image::CompRepr index_repr = image::color_index_repr; // FIXME: Should be made varyable, and be provided through TransferInfo                                  
    using index_comp_type = image::comp_type<index_repr>;
    core::Buffer<std::byte>& buffer = m_workspace_buffer_1;
    int num_index_channels = 1;
    impl::Workspace<index_comp_type> workspace(buffer, num_index_channels, tray.size); // Throws
    image::Tray tray_2 = workspace.tray(num_index_channels, tray.size);
    ensure_palette_kdtree(); // Throws
    for (int y = 0; y < tray.size.height; ++y) {
        for (int x = 0; x < tray.size.width; ++x) {
            const image::float_type* color = tray(x, y);
            std::size_t index_1 = do_reverse_palette_lookup(color); // Throws
            // No overlow is possible here because palette size is clamped to available
            // range of index representation.
            auto index_2 = image::unpacked_comp_type<index_repr>(index_1);
            index_comp_type index_3 = image::comp_repr_pack<index_repr>(index_2);
            tray_2(x, y)[0] = index_3;
        }
    }
    get_image().write(pos, tray_2); // Throws
}


void Writer::instantiate_palette_kdtree()
{
    ARCHON_ASSERT(!m_palette_kdtree);

    const image::float_type* float_components = ensure_palette_cache_f(); // Throws
    std::size_t palette_size = get_palette_size();
    auto kdtree = std::make_unique<std::size_t[]>(palette_size); // Throws
    for (std::size_t i = 0; i < palette_size; ++i)
        kdtree[i] = i;

    int num_channels_ext = m_num_channels_ext;
    auto get_comp = [&](std::size_t color_index, int comp_index) noexcept {
        std::size_t i = std::size_t(color_index * num_channels_ext + comp_index);
        return float_components[i];
    };

    int k = get_num_channels();
    auto begin = kdtree.get();
    auto end = begin + palette_size;
    util::kdtree_sort(k, begin, end, std::move(get_comp)); // Throws

    // Install
    m_palette_kdtree = std::move(kdtree);
}


auto Writer::do_reverse_palette_lookup_a(const image::float_type* color) -> std::size_t
{
    if (ARCHON_LIKELY(has_indexed_color())) {
        ensure_palette_kdtree(); // Throws
        return do_reverse_palette_lookup(color); // Throws
    }
    return 0;
}


auto Writer::do_color_sqdist_a(image::float_type* a, image::float_type* b) -> image::float_type
{
    // FIXME: Must take configured comparison color space into account when that feature is added                          
    double sqdist = 0;
    int n = m_num_channels_ext;
    for (int i = 0; i < n; ++i)
        sqdist += core::square(double(a[i]) - double(b[i])); // Throws
    return image::float_type(sqdist); // Throws
}
