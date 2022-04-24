/**

\page Concept_Archon_Image_ChannelSpec Concept: Image_ChannelSpec

This concept specifies the requirements that a type must meet in order to qualify as a
channel specification from the point of view of the Archon Image library. Examples of types
that conform to this concept are \ref archon::image::StandardChannelSpec and \ref
archon::image::CustomChannelSpec.

A channel specification specifies the color space in use (see \ref image::ColorSpace) and
whether an alpha channel is present. The number of channels and the presence of an alpha
channel is specified as compile-time constants. A channel specification is used to
parameterize various pixel format specifications such as \ref image::IntegerPixelFormat.

A channel specification determines a canonical channel order, which is the color channels in
the canonical order specified by the color space followed by the alpha channel. The actual
order may be different for a particular pixel format, but the difference will be specified
in terms of the canonical order, for example, it could be that the actual order is the
revers of the canonical order.


Assume the following identifications:

  - Let `C` be a type.

  - Let `c` be a `const` object of type `C`.


Then `C` conforms to the `Image_ChannelSpec` concept if, and only if all of the following
requirements are met:

  - `C::num_channels` must be a static compile-time constant expression of type `int`. Its
    value must be the total number of channels, i.e., the number of channels in the color
    space plus one if an alpha channel is present.

  - `C::has_alpha_channel` must be a static compile-time constant expression of type
    `bool`. It must be `true` if, and only if an alpha channel is present.

  - `c.get_color_space()` must be a valid function invocation. The invocation must be
    `noexcept` operation. The result must be of type `const image::ColorSpace&` (see \ref
    image::ColorSpace). The invocation must be `noexcept` operation.

*/
