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
#include <string_view>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/integer.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>
#include <archon/image/file_format_png.hpp>


using namespace archon;


namespace {


class DefaultRegistry
    : public image::FileFormatRegistry {
public:
    DefaultRegistry()
    {
        const image::FileFormat* known_file_formats[] {
            image::get_file_format_png(),
        };

        for (const image::FileFormat* format : known_file_formats) {
            if (format)
                register_file_format(*format); // Throws
        }
    }
};

auto get_default_registry() -> const DefaultRegistry&
{
    static DefaultRegistry registry; // Throws
    return registry;
}


} // unnamed namespace


using image::FileFormatRegistry;


auto FileFormatRegistry::get_default_registry() -> const FileFormatRegistry&
{
    return ::get_default_registry(); // Throws
}


auto FileFormatRegistry::lookup(std::string_view ident) const noexcept -> const image::FileFormat*
{
    auto i = m_format_map.find(ident);
    if (ARCHON_LIKELY(i != m_format_map.end())) {
        std::size_t format_index = i->second;
        return m_formats[format_index];
    }
    return nullptr;
}


auto FileFormatRegistry::lookup_by_mime_type(std::string_view mime_type) const -> const image::FileFormat*
{
    auto p = m_formats_by_mime_type.equal_range(mime_type);
    auto begin = p.first;
    auto end   = p.second;
    if (ARCHON_LIKELY(begin != end)) {
        std::size_t format_index = begin->second;
        return m_formats[format_index];
    }
    return nullptr;
}


void FileFormatRegistry::lookup_by_mime_type(std::string_view mime_type, tray_type& tray) const
{
    auto p = m_formats_by_mime_type.equal_range(mime_type);
    auto begin = p.first;
    auto end   = p.second;
    for (auto i = begin; i != end; ++i) {
        std::size_t format_index = i->second;
        tray.push_back(m_formats[format_index]); // Throws
    }
}


auto FileFormatRegistry::lookup_by_extension(std::string_view extension) const -> const image::FileFormat*
{
    auto p = m_formats_by_extension.equal_range(extension);
    auto begin = p.first;
    auto end   = p.second;
    if (ARCHON_LIKELY(begin != end)) {
        std::size_t format_index = begin->second;
        return m_formats[format_index];
    }
    return nullptr;
}


void FileFormatRegistry::lookup_by_extension(std::string_view extension, tray_type& tray) const
{
    auto p = m_formats_by_extension.equal_range(extension);
    auto begin = p.first;
    auto end   = p.second;
    for (auto i = begin; i != end; ++i) {
        std::size_t format_index = i->second;
        tray.push_back(m_formats[format_index]); // Throws
    }
}


void FileFormatRegistry::register_file_format(const image::FileFormat& format)
{
    if (ARCHON_UNLIKELY(core::int_is_max(get_num_file_formats())))
        throw std::length_error("File format registry size");

    core::Span<const std::string_view> extensions = format.get_filename_extensions();
    core::Span<const std::string_view> mime_types = format.get_mime_types();

    m_formats.reserve_extra(1); // Throws
    m_formats_by_extension.reserve_extra(extensions.size()); // Throws
    m_formats_by_mime_type.reserve_extra(mime_types.size()); // Throws

    std::string_view ident = format.get_ident();
    std::size_t format_index = m_formats.size();
    auto p = m_format_map.emplace(ident, format_index);
    bool was_inserted = p.first;
    if (!was_inserted)
        throw std::invalid_argument("File format already registered");

    m_formats.push_back(&format);
    for (std::string_view ext : extensions)
        m_formats_by_extension.emplace(ext, format_index); // Throws
    for (std::string_view type : mime_types)
        m_formats_by_mime_type.emplace(type, format_index); // Throws
}
