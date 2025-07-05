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

#ifndef ARCHON_X_IMAGE_X_FILE_FORMAT_HPP
#define ARCHON_X_IMAGE_X_FILE_FORMAT_HPP

/// \file


#include <memory>
#include <string_view>
#include <locale>
#include <system_error>

#include <archon/core/span.hpp>
#include <archon/core/typed_object_registry.hpp>
#include <archon/core/source.hpp>
#include <archon/core/sink.hpp>
#include <archon/log/logger.hpp>
#include <archon/image/image.hpp>
#include <archon/image/writable_image.hpp>
#include <archon/image/progress_tracker.hpp>
#include <archon/image/image_provider.hpp>
#include <archon/image/comment_handler.hpp>


namespace archon::image {


/// \brief Image file format.
///
/// An instance of this class represents a particular image file format, and allows for the
/// loading of, and saving of images using that file format. For example, it could represent
/// the PNG (Portable Network Graphics) image file format.
///
/// In some cases, an instance of this class can represent one of several alternative ways
/// to work with a particular file format.
///
class FileFormat {
public:
    struct LoadConfig;
    struct SaveConfig;

    /// \{
    ///
    /// \brief Base classes for special configuration parameters.
    ///
    /// See \ref SpecialLoadConfigRegistry and \ref SpecialSaveConfigRegistry for details.
    ///
    struct SpecialLoadConfig {};
    struct SpecialSaveConfig {};
    /// \}

    /// \{
    ///
    /// \brief Facilities for passing special parameters to load and save processes.
    ///
    /// *Special configuration parameters* are configuration parameters that are specific to
    /// a particular image file format. If a file format offers special configuration
    /// parameters for the loading or saving processes, it defines classes that inherit from
    /// \ref SpecialLoadConfig and \ref SpecialSaveConfig respectively. These classes will
    /// be defined in the header (include file) that pertains to the file format. For
    /// example, see \ref image::PNGSaveConfig.
    ///
    /// An application can pass special configuration parameters to the loading and saving
    /// processes through use of objects of type `SpecialLoadConfigRegistry` and
    /// `SpecialSaveConfigRegistry` respectively.
    ///
    /// Here is an example of how to turn interlacing on for a saved PNG image:
    ///
    /// \code{.cpp}
    ///
    ///   archon::image::PNGSaveConfig png_config;
    ///   png_config.use_adam7_interlacing = true;
    ///   archon::image::FileFormat::SpecialSaveConfigRegistry special_config;
    ///   special_config.register_(png_config);
    ///   archon::image::SaveConfig config;
    ///   config.special = &special_config;
    ///   archon::image::save(image, "foo.png", locale, config);
    ///
    /// \endcode
    ///
    /// PNG-specific configuration parameters will matter only when the saved image uses the
    /// PNG file format. Likewise for other file formats. An application can specify special
    /// configuration parameters for multiple file formats at once by registering more than
    /// one special parameters object with the `special_config` object.
    ///
    /// The scheme for passing special configuration parameters to the loading process is
    /// exactly analogous the one shown above. Instead of a `SpecialSaveConfigRegistry`, use
    /// a `SpecialLoadConfigRegistry`.
    ///
    /// \sa \ref image::PNGSaveConfig
    /// \sa \ref SpecialLoadConfig, \ref SpecialSaveConfig
    /// \sa \ref LoadConfig::special, \ref SaveConfig::special
    /// \sa \ref core::TypedObjectRegistry
    ///
    using SpecialLoadConfigRegistry = core::TypedObjectRegistry<const SpecialLoadConfig, 8>;
    using SpecialSaveConfigRegistry = core::TypedObjectRegistry<const SpecialSaveConfig, 8>;
    /// \}

    /// \brief File format identifier.
    ///
    /// This function returns the identifier for the image file format. Identifiers, such as
    /// this one, are used to uniquely identify file formats in a file format registry (\ref
    /// image::FileFormatRegistry).
    ///
    /// The returned string view must remain valid until the file format object is destroyed.
    ///
    virtual auto get_ident() const noexcept -> std::string_view = 0;

    /// \brief File format description.
    ///
    /// This function returns the description of the image file format. The description is
    /// supposed to be a short text that serves to identify the file format in a broader
    /// context.
    ///
    /// The returned string view must remain valid until the file format object is destroyed.
    ///
    virtual auto get_descr() const -> std::string_view = 0;

    /// \brief Associated MIME types.
    ///
    /// This function returns the list of MIME types for which the file format should be
    /// considered a likely candidate.
    ///
    /// The returned string views must remain valid until the file format object is destroyed.
    ///
    virtual auto get_mime_types() const noexcept -> core::Span<const std::string_view> = 0;

    /// \brief Associated filename extensions.
    ///
    /// This function returns the list of filename extensions for which the file format
    /// should be considered a likely candidate.
    ///
    /// The returned string views must remain valid until the file format object is destroyed.
    ///
    virtual auto get_filename_extensions() const noexcept -> core::Span<const std::string_view> = 0;

    /// \brief Whether file format is available.
    ///
    /// This function returns `true` when, and only when this file format is
    /// available. Ordinarily, it will be available if support for this file format was
    /// enabled when the Archon image library was built.
    ///
    /// When the file format is unavailable, certain functions (\ref try_recognize(), \ref
    /// try_load(), \ref try_save()) fail with \ref image::Error::file_format_unavailable.
    ///
    virtual bool is_available() const noexcept = 0;

    /// \brief Try to determine whether leading bytes match this file format.
    ///
    /// By looking only at a prefix of the specified byte sequence (\p source), this
    /// function attempts to determine whether the byte sequence appears to be an image file
    /// that is using this file format.
    ///
    /// The caller should expect that this function only reads as much of the byte sequence
    /// as it needs in order to decide this question.
    ///
    /// If the determination succeeds, this function returns `true` after setting \p
    /// recognized to `true` when the answer is "yes", and to `false` when it is "no".
    ///
    /// If the determination fails, this function returns `false` after setting \p ec to
    /// reflect the cause of the failure.
    ///
    /// If the file format is unavailable, this function fails with \ref
    /// image::Error::file_format_unavailable.
    ///
    /// \sa image::try_load().
    /// \sa try_load().
    ///
    virtual bool try_recognize(core::Source& source, bool& recognized, const std::locale& locale, log::Logger& logger,
                               std::error_code& ec) const = 0;

    /// \{
    ///
    /// \brief Try to load image using this file format.
    ///
    /// These functions attempt to read an image from the file or stream represented by the
    /// specified source (\p source) using the file format that this `FileFormat` object
    /// represents.
    ///
    /// On success, these functions return `true` after setting \p image to refer to an
    /// image object that contains the loaded image. In this case, \p ec is left
    /// unchanged.
    ///
    /// On failure, these functions return `false` after setting \p ec to an error code that
    /// reflects the cause of the failure. In this case, \p image is left unchanged.
    ///
    /// If the file format is unavailable, these functions fail with \ref
    /// image::Error::file_format_unavailable.
    ///
    /// \sa image::try_load().
    /// \sa try_recognize().
    ///
    bool try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image, const std::locale& locale,
                  log::Logger& logger, std::error_code& ec) const;
    bool try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image, const std::locale& locale,
                  log::Logger& logger, const LoadConfig& config, std::error_code& ec) const;
    /// \}

    /// \{
    ///
    /// \brief Try to save image using this file format.
    ///
    /// These functions attempt to write the specified image (\p image) to the file or
    /// stream represented by the specified sink (\p sink) using the file format that this
    /// `FileFormat` object represents.
    ///
    /// On success, these functions return `true` and leave \p ec unchanged.
    ///
    /// On failure, they return `false` after setting \p ec to an error code that reflects
    /// the cause of the failure.
    ///
    /// If the file format is unavailable, these functions fail with \ref
    /// image::Error::file_format_unavailable.
    ///
    /// \sa image::try_save().
    ///
    bool try_save(const image::Image& image, core::Sink& sink, const std::locale& locale, log::Logger& logger,
                  std::error_code& ec) const;
    bool try_save(const image::Image& image, core::Sink& sink, const std::locale& locale, log::Logger& logger,
                  const SaveConfig& config, std::error_code& ec) const;
    /// \}

    virtual ~FileFormat() noexcept = default;

protected:
    /// \brief Abstract load function.
    ///
    /// This function is called by \ref try_load().
    ///
    virtual bool do_try_load(core::Source&, std::unique_ptr<image::WritableImage>&, const std::locale&, log::Logger&,
                             const LoadConfig&, std::error_code&) const = 0;

    /// \brief Abstract save function.
    ///
    /// This function is called by \ref try_save().
    ///
    virtual bool do_try_save(const image::Image&, core::Sink&, const std::locale&, log::Logger&, const SaveConfig&,
                             std::error_code&) const = 0;
};


/// \brief Configuration of image loading process.
///
/// An object of this type is used to specify parameters that control the image loading
/// process as it is invoked through \ref try_load().
///
struct FileFormat::LoadConfig {
    /// \brief Track progress of loading process.
    ///
    /// An application that wishes to be notified about progress of the loading process can
    /// instantiate a progress tracker (\ref image::ProgressTracker) and then reference it
    /// here.
    ///
    /// \sa \ref image::ProgressTracker.
    ///
    image::ProgressTracker* progress_tracker = nullptr;

    /// \brief   
    ///
    ///   
    ///
    image::ImageProvider* image_provider = nullptr;

    /// \brief Discover text comments in loaded images.
    ///
    /// An application that wishes to be notified about text comments in loaded image files
    /// can instantiate a comment handler (\ref image::CommentHandler) and then reference it
    /// here.
    ///
    image::CommentHandler* comment_handler = nullptr;

    /// \brief Opportunity to pass special configuration parameters to loading process.
    ///
    /// If special configuration parameters need to be passed to the loading process, the
    /// application must create an instance of \ref SpecialLoadConfigRegistry and then
    /// reference it here. See \ref SpecialLoadConfigRegistry for details.
    ///
    /// \sa \ref SpecialLoadConfigRegistry
    ///
    const SpecialLoadConfigRegistry* special = nullptr;
};


/// \brief Configuration of image saving process.
///
/// An object of this type is used to specify parameters that control the image saving
/// process as it is invoked through \ref try_save().
///
struct FileFormat::SaveConfig {
    /// \brief Opportunity to track progress of saving process.
    ///
    /// An application that wishes to be notified about progress of the saving process can
    /// instantiate a progress tracker (\ref image::ProgressTracker) and then reference it
    /// here.
    ///
    /// \sa \ref image::ProgressTracker.
    ///
    image::ProgressTracker* progress_tracker = nullptr;

    /// \brief Opportunity to pass special configuration parameters to saving process.
    ///
    /// If special configuration parameters need to be passed to the saving process, the
    /// application must create an instance of \ref SpecialSaveConfigRegistry and then
    /// reference it here. See \ref SpecialSaveConfigRegistry for details.
    ///
    /// \sa \ref SpecialSaveConfigRegistry
    ///
    const SpecialSaveConfigRegistry* special = nullptr;
};








// Implementation


inline bool FileFormat::try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image,
                                 const std::locale& loc, log::Logger& logger, std::error_code& ec) const
{
    return try_load(source, image, loc, logger, {}, ec); // Throws
}


inline bool FileFormat::try_load(core::Source& source, std::unique_ptr<image::WritableImage>& image,
                                 const std::locale& loc, log::Logger& logger, const LoadConfig& config,
                                 std::error_code& ec) const
{
    return do_try_load(source, image, loc, logger, config, ec); // Throws
}


inline bool FileFormat::try_save(const image::Image& image, core::Sink& sink, const std::locale& loc,
                                 log::Logger& logger, std::error_code& ec) const
{
    return try_save(image, sink, loc, logger, {}, ec); // Throws
}


inline bool FileFormat::try_save(const image::Image& image, core::Sink& sink, const std::locale& loc,
                                 log::Logger& logger, const SaveConfig& config, std::error_code& ec) const
{
    return do_try_save(image, sink, loc, logger, config, ec); // Throws
}


} // namespace archon::image

#endif // ARCHON_X_IMAGE_X_FILE_FORMAT_HPP
