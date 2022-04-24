/**

\page Concept_Archon_Image_PixelFormat Concept: Image_PixelFormat

This concept specifies the requirements that a type must meet in order to qualify as a pixel
format specification from the point of view of the Archon Image library. Such a pixel format
can be used with \ref archon::image::BufferedImage. Examples of implementations are \ref
archon::image::IntegerPixelFormat, \ref archon::image::PackedPixelFormat, \ref
archon::image::SubwordPixelFormat, and \ref archon::image::IndexedPixelFormat.

A pixel format defines how a two dimmensional block of pixels is represented within an array
of memory words of integer or floating-point type. The "word type" is specified by the pixel
format (see `F::word_type`). Memory used to store pixels according to this format will be
accessed in terms of words of this type. How pixels are layed out within a sequence of words
is mostly up to the pixel format. One format could choose to use multiple words per channel
component while another could choose to pack multiple channels, or even multiple pixels
inside each word.

A pixel format must choose a *pixel transfer representation* to be used when pixels are
passed to it (see `write()` and `fill()`) or from it (see `read()`). This transfer
representation must be based on one of the component representation schemes offered through
\ref image::CompRepr (see `F::get_transfer_repr()`).

As part of choosing a pixel transfer representation, a pixel format must choose between
using direct or indexed color when transferring pixels (see `F::is_indexed_color`). If it
chooses direct color, each transferred pixel consists of a sequence of channel components as
described in the documentation of \ref image::CompRepr. If it chooses indexed color, each
pixel consists of a single index into the palette, which can be thought of as a single
"index channel".

FIXME: Talk about pre and post conditions involving component value overflows in tray (integer based representations: when writing, caller must ensure that no component values overflow, when reading, caller can assume that no component values overflow).                              


Assume the following identifications:

  - Let `F` be a type.

  - Let `f` be a `const` object of type `F`.

  - Let 'image_size' be an object of type \ref archon::image::Size.

  - Let `buffer_format` be an object of type \ref archon::image::BufferFormat.

  - Let `pos` be an object of type \ref archon::image::Pos.

  - Let `area` be an object of type \ref archon::image::Box.


Then `F` conforms to the `Image_PixelFormat` concept if, and only if all of the following
requirements are met:

  - `F::is_indexed_color` must be a static compile-time constant expression of type
    `bool`. It must be `true` if, and only if the pixel format uses indexed color (see
    description above).

  - `F::word_type` must be the type of the words that this pixel format is defined in terms
    of (see description above).

  - `F::get_transfer_repr()` must be a valid function invocation that can be evaluated at
    compile time. The invocation must be `noexcept` operation. The result must be of type
    \ref archon::image::CompRepr. It must specify the component representation scheme used
    in the pixel transfer representation (see descrition above).

  - `f.get_buffer_size(image_size)` must be a valid function invocation. The result must be
    of type `std::size_t`, and it must specify the size, in number of words of type
    `F::word_type`, of the buffer that would be needed to hold an image of the specified
    size. The function must throw if the image size is too large.

  - `f.get_palette()` must be a valid function invocation if the pixel format uses indexed
    color (`F::is_indexed_color`). In that case, the result must be convertible to `const
    image::Image*`, and the invocation must be `noexcept` operation. The returned image must
    be the image that functions as a palette for this pixel format. See \ref
    archon::image::Image::get_palette().

  - `f.try_describe(buffer_format)` must be a valid function invocation. The result must be
    of type `bool`. If this pixel format can be described using \ref
    archon::image::BufferFormat, the result must be `true`, and `buffer_format` must have
    been set to describe this pixel format. Otherwise the result must be `false` and
    `buffer_format` must have been left unchanged.

  - `f.get_transfer_info()` must be a valid function invocation. The invocation must be
    `noexcept` operation. The result must be of type \ref image::Image::TransferInfo.

  - `f.read(buffer, image_size, pos, tray)` must be a valid function invocations if `buffer`
    is a pointer to an array of `const` words of type `F::word_type`, and `tray` is an
    object of type `image::tray_type<R>` where `R` is `F::get_transfer_repr()`. If \p buffer
    holds an image stored according to this pixel format, and \p image_size is the size of
    that image, then this operation must read a rectangular block of pixels from the image
    and place those pixels on the specified tray with components represented as specified by
    `R` (see \ref image::CompRepr for details). The size of the block is determined by the
    size of the specified tray (\ref archon::image::Tray::size). The position within the
    image of the top-left corner of the block is determined by the specified position (\p
    pos). The caller must ensure that the block falls fully inside the image area.

  - `f.write(buffer, image_size, pos, tray)` must be a valid function invocations if
    `buffer` is a pointer to an array of words of type `F::word_type`, and `tray` is an
    object of type `image::const_tray_type<R>` where `R` is `F::get_transfer_repr()`. If \p
    buffer holds an image stored according to this pixel format, and \p image_size is the
    size of that image, then this operation must write a rectangular block of pixels to the
    image taking those pixels from the specified tray whose components must be represented
    as specified by `R` (see \ref image::CompRepr for details). The size of the block is
    determined by the size of the specified tray (\ref archon::image::Tray::size). The
    position within the image of the top-left corner of the block is determined by the
    specified position (\p pos). The caller must ensure that the block falls fully inside
    the image area.

  - `f.fill(buffer, image_size, area, color)` must be a valid function invocations if
    `buffer` is a pointer to an array of words of type `F::word_type`, and `color` is a
    pointer to an array of words of type `const image::comp_type<R>` where `R` is
    `F::get_transfer_repr()`. If \p buffer holds an image stored according to this pixel
    format, and \p image_size is the size of that image, then this operation must fill the
    specified rectangular area of the image (\p area) with pixels of the specified color (\p
    color). The caller must ensure that the area to be filled falls fully inside the image
    area. The representation of the fill color (\p color) is as specified by `R` (see \ref
    image::CompRepr for details).

*/
