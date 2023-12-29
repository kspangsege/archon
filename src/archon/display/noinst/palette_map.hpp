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

#ifndef ARCHON_X_DISPLAY_X_NOINST_X_PALETTE_MAP_HPP
#define ARCHON_X_DISPLAY_X_NOINST_X_PALETTE_MAP_HPP


#include <cstddef>
#include <utility>
#include <algorithm>
#include <memory>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/color_space.hpp>
#include <archon/image/image.hpp>
#include <archon/image/reader.hpp>


namespace archon::display::impl {


// FIXME: Move to image library and allow for palette map to be passed to image::Reader
// constructor so that the reader does not have to read from the palette image and the
// writer does not have to build the kd-tree.                    
//
class PaletteMap {
public:
    PaletteMap() noexcept = default;
    ~PaletteMap() noexcept;

    // Movability
    PaletteMap(PaletteMap&&) noexcept;
    auto operator=(PaletteMap&&) noexcept -> PaletteMap&;

    PaletteMap(const image::Image&);

    template<image::CompRepr R> PaletteMap(const image::ColorSpace& color_space, int num_channels_ext,
                                           bool trivial_alpha, int size,
                                           std::unique_ptr<image::comp_type<R>[]> components,
                                           std::size_t num_components);

    auto color_space() const noexcept -> const image::ColorSpace&;
    int num_channels() const noexcept;
    int num_channels_ext() const noexcept;
    int size() const noexcept;

    bool reverse_lookup(const image::float_type* color, int& index) const;

    template<image::CompRepr R> static void read_palette(image::Reader&, int& size,
                                                         std::unique_ptr<image::comp_type<R>[]>& components,
                                                         std::size_t& num_components, int max_size = -1);

private:
    const image::ColorSpace* m_color_space = nullptr;
    int m_num_channels = 0;
    int m_num_channels_ext = 1;
    int m_size = 0;
    image::CompRepr m_native_comp_repr = image::CompRepr::int8;
    std::size_t m_num_components = 0;
    void* m_native_components = nullptr;
    std::unique_ptr<image::float_type[]> m_components;
    std::unique_ptr<int[]> m_kdtree;

    void destroy() noexcept;
    void move_from(PaletteMap&&) noexcept;

    template<image::CompRepr R> void init(const image::ColorSpace&, int num_channels, int num_channels_ext, int size,
                                          std::unique_ptr<image::comp_type<R>[]> components,
                                          std::size_t num_components);

    void convert();
    void setup_kdtree();
};








// Implementation


inline PaletteMap::~PaletteMap() noexcept
{
    destroy();
}


inline PaletteMap::PaletteMap(PaletteMap&& other) noexcept
{
    move_from(std::move(other));
}


inline auto PaletteMap::operator=(PaletteMap&& other) noexcept -> PaletteMap&
{
    destroy();
    move_from(std::move(other));
    return *this;
}


template<image::CompRepr R> inline PaletteMap::PaletteMap(const image::ColorSpace& color_space, int num_channels_ext,
                                                          bool trivial_alpha, int size,
                                                          std::unique_ptr<image::comp_type<R>[]> components,
                                                          std::size_t num_components)
{
    constexpr image::CompRepr comp_repr = R;
    int num_channels = num_channels_ext - int(trivial_alpha);
    init<comp_repr>(color_space, num_channels, num_channels_ext, size, std::move(components),
                    num_components); // Throws
}


inline auto PaletteMap::color_space() const noexcept -> const image::ColorSpace&
{
    if (ARCHON_LIKELY(m_color_space))
        return *m_color_space;
    return image::ColorSpace::get_degen();
}


inline int PaletteMap::num_channels() const noexcept
{
    return m_num_channels;
}


inline int PaletteMap::num_channels_ext() const noexcept
{
    return m_num_channels_ext;
}


inline int PaletteMap::size() const noexcept
{
    return m_size;
}


template<image::CompRepr R> void PaletteMap::read_palette(image::Reader& reader, int& size,
                                                          std::unique_ptr<image::comp_type<R>[]>& components,
                                                          std::size_t& num_components, int max_size)
{
    constexpr image::CompRepr comp_repr = R;

    // Determine palette size, i.e., number of colors
    image::Size image_size = reader.get_image_size();
    int width  = std::max(image_size.width, 0);
    int height = std::max(image_size.height, 0);
    int size_2 = 1;
    bool success = (core::try_int_mul(size_2, width) && core::try_int_mul(size_2, height));
    if (ARCHON_UNLIKELY(!success))
        size_2 = core::int_max<decltype(size_2)>();
    if (ARCHON_UNLIKELY(max_size >= 0 && size_2 > max_size))
        size_2 = max_size;

    // Determine number of components
    int num_channels_ext = reader.get_num_channels_ext();
    std::size_t num_components_2 = 1;
    core::int_mul(num_components_2, num_channels_ext); // Throws
    core::int_mul(num_components_2, size_2); // Throws

    // Allocate memory for components
    using comp_type = image::comp_type<comp_repr>;
    std::unique_ptr<comp_type[]> components_2 = std::make_unique<comp_type[]>(num_components_2); // Throws

    // Read components
    int num_full_rows = size_2 / width;
    std::ptrdiff_t horz_stride = num_channels_ext;
    std::ptrdiff_t vert_stride = width * horz_stride;
    image::Iter iter = { components_2.get(), horz_stride, vert_stride };
    image::Tray tray_1 = { iter, image::Size(width, num_full_rows) };
    const image::ColorSpace& color_space = reader.get_color_space();
    bool has_alpha = true;
    reader.get_block_a<comp_repr>(image::Pos(0, 0), tray_1, color_space, has_alpha); // Throws
    int remain = size_2 - num_full_rows * width;
    if (remain > 0) {
        image::Tray tray_2 = { iter + image::Size(0, num_full_rows), image::Size(remain, 1) };
        reader.get_block_a<comp_repr>(image::Pos(0, num_full_rows), tray_2, color_space, has_alpha); // Throws
    }

    size = size_2;
    components = std::move(components_2);
    num_components = num_components_2;
}


template<image::CompRepr R> inline void PaletteMap::init(const image::ColorSpace& color_space, int num_channels,
                                                         int num_channels_ext, int size,
                                                         std::unique_ptr<image::comp_type<R>[]> components,
                                                         std::size_t num_components)
{
    constexpr image::CompRepr comp_repr = R;

    std::size_t expected_num_components = 1;
    core::int_mul(expected_num_components, num_channels_ext); // Throws
    core::int_mul(expected_num_components, size); // Throws
    if (ARCHON_UNLIKELY(num_components != expected_num_components))
        throw std::invalid_argument("Number of components");

    m_color_space = &color_space;
    m_num_channels = num_channels;
    m_num_channels_ext = num_channels_ext;
    m_size = size;
    m_native_comp_repr = comp_repr;
    m_num_components = num_components;

    if constexpr (comp_repr == image::CompRepr::float_) {
        m_components = std::move(components);
    }
    else {
        m_native_components = components.release();
        convert(); // Throws
    }
    setup_kdtree(); // Throws
}


} // namespace archon::display::impl

#endif // ARCHON_X_DISPLAY_X_NOINST_X_PALETTE_MAP_HPP
