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
#include <utility>
#include <memory>
#include <optional>

#include <archon/util/kdtree.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/image.hpp>
#include <archon/image/reader.hpp>
#include <archon/display/noinst/palette_map.hpp>


using namespace archon;
namespace impl = display::impl;
using impl::PaletteMap;


PaletteMap::PaletteMap(const image::Image& palette)
{
    image::Reader reader(palette);
    image::comp_repr_dispatch(reader.get_comp_repr(), [&](auto tag) {
        constexpr image::CompRepr comp_repr = decltype(tag)::comp_repr;
        using comp_type = image::comp_type<comp_repr>;
        int size = {};
        std::unique_ptr<comp_type[]> components;
        std::size_t num_components = {};
        read_palette<comp_repr>(reader, size, components, num_components); // Throws
        const image::ColorSpace& color_space = reader.get_color_space();
        int num_channels = reader.get_num_channels();
        int num_channels_ext = reader.get_num_channels_ext();
        init<comp_repr>(color_space, num_channels, num_channels_ext, size, std::move(components),
                        num_components); // Throws
    }); // Throws
}


bool PaletteMap::reverse_lookup(const image::float_type* color, int& index) const
{
    int num_channels_ext = m_num_channels_ext;
    const image::float_type* components = m_components.get();
    auto get_comp = [&](int color_index, int comp_index) noexcept {
        auto i = color_index * std::size_t(num_channels_ext) + comp_index;
        return components[i];
    };
    int k = m_num_channels;
    auto begin = m_kdtree.get();
    auto end = begin + m_size;
    std::optional<image::float_type> max_dist = {}; // No max dist
    image::float_type dist = 0;
    return util::kdtree_find(k, begin, end, get_comp, color, max_dist, index, dist); // Throws
}


void PaletteMap::destroy() noexcept
{
    if (m_native_components) {
        image::comp_repr_dispatch_nothrow(m_native_comp_repr, [&](auto tag) noexcept {
            constexpr image::CompRepr comp_repr = decltype(tag)::comp_repr;
            using comp_type = image::comp_type<comp_repr>;
            comp_type* components = static_cast<comp_type*>(m_native_components);
            delete[] components;
        });
    }
}


void PaletteMap::move_from(PaletteMap&& other) noexcept
{
    m_color_space       = other.m_color_space;
    m_num_channels      = other.m_num_channels;
    m_size              = other.m_size;
    m_native_comp_repr  = other.m_native_comp_repr;
    m_num_components    = other.m_num_components;
    m_native_components = other.m_native_components;
    m_components        = std::move(other.m_components);
    m_kdtree            = std::move(other.m_kdtree);

    other.m_native_components = nullptr;
}


void PaletteMap::convert()
{
    auto components = std::make_unique<image::float_type[]>(m_num_components); // Throws
    image::comp_repr_dispatch_nothrow(m_native_comp_repr, [&](auto tag) noexcept {
        int num_channels_ext = m_num_channels_ext;
        constexpr image::CompRepr comp_repr = decltype(tag)::comp_repr;
        using comp_type = image::comp_type<comp_repr>;
        const comp_type* origin = static_cast<const comp_type*>(m_native_components);
        image::float_type* destin = components.get();
        int n = m_size;
        for (int i = 0; i < n; ++i) {
            constexpr image::CompRepr repr_1 = comp_repr;
            constexpr image::CompRepr repr_2 = image::CompRepr::float_;
            bool has_alpha = true; // Alpha channel always present on both sides
            image::comp_repr_convert<repr_1, repr_2>(origin, destin, num_channels_ext, has_alpha);
            origin += num_channels_ext;
            destin += num_channels_ext;
        }
    });
    m_components = std::move(components);
}


void PaletteMap::setup_kdtree()
{
    auto kdtree = std::make_unique<int[]>(std::size_t(m_size)); // Throws
    for (int i = 0; i < m_size; ++i)
        kdtree[i] = i;
    int num_channels_ext = m_num_channels_ext;
    const image::float_type* components = m_components.get();
    auto get_comp = [&](int color_index, int comp_index) noexcept {
        auto i = color_index * std::size_t(num_channels_ext) + comp_index;
        return components[i];
    };
    int k = m_num_channels;
    auto begin = kdtree.get();
    auto end = begin + m_size;
    util::kdtree_sort(k, begin, end, get_comp); // Throws
    m_kdtree = std::move(kdtree);
}
