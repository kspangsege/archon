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
#include <optional>
#include <string_view>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/index_range.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/display/noinst/edid.hpp>


using namespace archon;
namespace impl = display::impl;
using impl::EdidParser;


bool EdidParser::parse(std::string_view str, impl::EdidInfo& info, core::StringBufferContents& string_data)
{
    if (ARCHON_LIKELY(str.size() >= 128)) {
        using uchar = unsigned char;
        unsigned used_bits = 0;
        unsigned checksum = 0;
        for (int i = 0; i < 128; ++i) {
            unsigned val = uchar(str[i]);
            used_bits |= val;
            checksum += val;
        }
        if (ARCHON_LIKELY((used_bits | 255) == 255 && (checksum & 255) == 0)) {
            int major = int(uchar(str[18]));
            int minor = int(uchar(str[19]));
            if (ARCHON_LIKELY(major > 1 || (major == 1 && minor >= 4))) {
                std::optional<core::IndexRange> monitor_name;
                std::size_t string_data_size = string_data.size();
                for (int j = 0; j < 4; ++j) {
                    const char* base = str.data() + (54 + j * 18);
                    bool is_monitor_descriptor = (base[0] == 0 && base[1] == 0);
                    if (is_monitor_descriptor) {
                        int type = int(uchar(base[3]));
                        switch (type) {
                            case 0xFC:
                                // Monitor name
                                if (m_is_unicode_locale) {
                                    constexpr int slot_offset = 5;
                                    constexpr int slot_size = 13;
                                    wchar_t data[slot_size];
                                    int n = 0;
                                    while (n < slot_size && base[slot_offset + n] != '\x0A') {
                                        data[n] = uchar(base[slot_offset + n]);
                                        ++n;
                                    }
                                    core::Span data_2 = { data, std::size_t(n) };
                                    core::Buffer<char>& buffer = string_data.buffer();
                                    std::size_t offset = string_data_size;
                                    bool success = m_string_codec.try_encode(data_2, buffer,
                                                                             string_data_size); // Throws
                                    if (ARCHON_LIKELY(success)) {
                                        monitor_name = core::IndexRange {
                                            offset,
                                            std::size_t(string_data_size - offset),
                                        };
                                    }
                                }
                        }
                    }
                }
                info.major = major;
                info.minor = minor;
                info.monitor_name = monitor_name;
                string_data.set_size(string_data_size);
                return true;
            }
        }
    }
    return false;
}
