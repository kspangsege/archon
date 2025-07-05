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

#ifndef ARCHON_X_IMAGE_X_FILE_FORMAT_REGISTRY_HPP
#define ARCHON_X_IMAGE_X_FILE_FORMAT_REGISTRY_HPP

/// \file


#include <cstddef>
#include <string_view>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/vector.hpp>
#include <archon/core/flat_map.hpp>
#include <archon/core/flat_multimap.hpp>
#include <archon/image/file_format.hpp>


namespace archon::image {


/// \brief Registry of candidate file formats.
///
/// An instance of this class is a collection of file formats. Its primary purpose is to
/// present the possible candidate file formats to functions such as \ref image::load() and
/// \ref image::save(), which attempt to transparently detect the file format. There is a
/// default registry, which is available as \ref get_default_registry().
///
/// \sa \ref image::list_file_formats()
///
class FileFormatRegistry {
public:
    /// \brief Default file format registry.
    ///
    /// This function returns the default image file format registry.
    ///
    static auto get_default_registry() -> const FileFormatRegistry&;

    /// \brief Get file format with specified identifier.
    ///
    /// If the registry contains a file format with the specified identifier, this function
    /// returns that file format. Otherwise, this function returns null.
    ///
    auto lookup(std::string_view ident) const noexcept -> const image::FileFormat*;

    using tray_type = core::BufferContents<const image::FileFormat*>;

    /// \brief Get file formats associated with MIME type.
    ///
    /// This function returns all the registered file formats that are associated with the
    /// specified MIME type. The file formats are returned in the order that they were
    /// registered. File formats are returned regardless of whether they are available or
    /// unavailable (\ref image::FileFormat::is_available()).
    ///
    void lookup_by_mime_type(std::string_view mime_type, tray_type&) const;

    /// \brief Get file formats associated with filename extension.
    ///
    /// This function returns all the registered file formats that are associated with the
    /// specified filename extension. The file formats are returned in the order that they
    /// were registered. File formats are returned regardless of whether they are available
    /// or unavailable (\ref image::FileFormat::is_available()).
    ///
    void lookup_by_extension(std::string_view extension, tray_type&) const;

    /// \brief Number of registered file formats.
    ///
    /// This function returns the number of file formats currently registered in the
    /// registry.
    ///
    int get_num_file_formats() const noexcept;

    /// \brief Get file format at index.
    ///
    /// This function returns the file format at the specified index within the list of
    /// registered file formats. The file formats occur in this list in the order that they
    /// were added to the registry.
    ///
    /// \sa \ref get_num_file_formats()
    ///
    auto get_file_format(int index) const -> const image::FileFormat&;

    /// \brief Register new file format.
    ///
    /// This function adds the specified file format to the registry. The caller must ensure
    /// that the referenced file format object stays alive for as long as the registry is in
    /// use. The registry can be safely destroyed after the destruction of the file format
    /// object. Any other use of the registry after the dstruction of the file format object
    /// causes undefined behavior.
    ///
    /// If the registry already contains the specified file format, or another file format
    /// using the same identifier, this function throws.
    ///
    void register_file_format(const image::FileFormat&);

private:
    core::Vector<const image::FileFormat*> m_formats;
    core::FlatMap<std::string_view, std::size_t> m_format_map;
    core::FlatMultimap<std::string_view, std::size_t> m_formats_by_extension;
    core::FlatMultimap<std::string_view, std::size_t> m_formats_by_mime_type;
};








// Implementation


inline int FileFormatRegistry::get_num_file_formats() const noexcept
{
    ARCHON_ASSERT(m_formats.size() <= core::to_unsigned(core::int_max<int>()));
    return int(m_formats.size());
}


inline auto FileFormatRegistry::get_file_format(int index) const -> const image::FileFormat&
{
    if (ARCHON_LIKELY(index >= 0 && index < get_num_file_formats()))
        return *m_formats[std::size_t(index)];
    throw std::out_of_range("File format index");
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_FILE_FORMAT_REGISTRY_HPP
