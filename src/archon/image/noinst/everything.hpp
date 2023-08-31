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


// Do not include this header file. It exists only to specify the canonical header order,
// which is a topological dependency ordering of all the header files of the Archon Core
// Library, including any that must never be included by applications.
#error "Do not include this header file"


// Foundation
#include <archon/image/image_namespace.hpp>
#include <archon/image/impl/config.h>
#include <archon/image/geom.hpp>
#include <archon/image/iter.hpp>
#include <archon/image/tray.hpp>
#include <archon/image/impl/bit_medium.hpp>
#include <archon/image/bit_medium.hpp>
#include <archon/image/impl/comp_types.hpp>
#include <archon/image/comp_types.hpp>
#include <archon/image/gamma.hpp>
#include <archon/image/comp_repr.hpp>
#include <archon/image/blend.hpp>
#include <archon/image/color_space.hpp>

// Support
#include <archon/image/pixel_convert.hpp>
#include <archon/image/pixel_repr.hpp>
#include <archon/image/pixel.hpp>
#include <archon/image/block.hpp>
#include <archon/image/channel_spec_concept.hpp>
#include <archon/image/standard_channel_spec.hpp>
#include <archon/image/custom_channel_spec.hpp>
#include <archon/image/bit_field.hpp>
#include <archon/image/channel_packing_concept.hpp>
#include <archon/image/channel_packing.hpp>
#include <archon/image/buffer_format.hpp>

// Images and palettes
#include <archon/image/image.hpp>
#include <archon/image/computed_image.hpp>
#include <archon/image/palette_image.hpp>
#include <archon/image/writable_image.hpp>
#include <archon/image/tray_image.hpp>
#include <archon/image/indexed_tray_image.hpp>
#include <archon/image/pixel_format_concept.hpp>
#include <archon/image/integer_pixel_format.hpp>
#include <archon/image/packed_pixel_format.hpp>
#include <archon/image/subword_pixel_format.hpp>
#include <archon/image/indexed_pixel_format.hpp>
#include <archon/image/buffered_image.hpp>
#include <archon/image/palettes.hpp>

// Image access
#include <archon/image/impl/workspace.hpp>
#include <archon/image/reader.hpp>
#include <archon/image/writer.hpp>

// Load / save
#include <archon/image/progress_tracker.hpp>
#include <archon/image/provider.hpp>
#include <archon/image/file_format.hpp>
#include <archon/image/file_format_registry.hpp>
#include <archon/image/file_format_png.hpp>
#include <archon/image/load_config.hpp>
#include <archon/image/save_config.hpp>
#include <archon/image/input.hpp>
#include <archon/image/output.hpp>
#include <archon/image/error.hpp>
#include <archon/image/load.hpp>
#include <archon/image/save.hpp>

// Testing
#include <archon/image/test/box_utils.hpp>
#include <archon/image/test/comp_repr_utils.hpp>
#include <archon/image/test/pixel_utils.hpp>
