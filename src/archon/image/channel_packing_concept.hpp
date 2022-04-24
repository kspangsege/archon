/**

\page Concept_Archon_Image_ChannelPacking Concept: Image_ChannelPacking

This concept specifies the requirements that a type must meet in order to qualify as a
channel packing specification from the point of view of the Archon Image library.

A channel packing specification provides a sequence of \ref image::BitField objects. Each of
those objects specifies the width and relative position of the bit field that makes up the
corresponding channel within the bits of some sufficiently wide integer word type.

From the point of view of a packing specification, the first bit position in a word is
understoofd to be the one of most significance, and the last bit position is understood as
the one of least significance.

The last channel in the packing specification is the one that occupies the bit positions of
least significance. If W is the width of the last channel and G is the gap specified for it,
then the last channel occupies the last W bit positions that precede the last G bit
positions.

Consider then a channel that is not the last one, and let W be its width and G be its gap,
then that channel occupies the last W bit positions that precede the last G bit positions
that precede the bit positions occupied by the subsequent channel.


Assume the following identifications:

  - Let `P` be a type.


Then `P` conforms to the `Image_ChannelPacking` concept if, and only if all of the following
requirements are met:

  - `P::num_fields` must be a static compile-time constant expression of type `int`. It must
    specify the number of channels described by the packing specification.

  - `P::fields` must be a static compile-time constant expression of type
    `image::BitField[N]` where `N` is the number of described channels
    (`P::num_fields`). The entries in this array must specify the widths and relative
    positions of each channel (see explanation above).

*/
