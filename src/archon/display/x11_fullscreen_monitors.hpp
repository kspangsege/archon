// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
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

#ifndef ARCHON_X_DISPLAY_X_X11_FULLSCREEN_MONITORS_HPP
#define ARCHON_X_DISPLAY_X_X11_FULLSCREEN_MONITORS_HPP

/// \file


#include <cstddef>
#include <utility>
#include <array>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/as_list.hpp>


namespace archon::display {


/// \brief Specification of multi-monitor fullscreen area.
///
/// An object of this type specifies which Xinerama screens (monitors) that a fullscreen
/// window should cover. It is a hint to the window manager, and so it may or may not be
/// honored. It is primarily intended to be used with \ref
/// display::x11_connection_config. See \ref
/// display::x11_connection_config::fullscreen_monitors.
///
/// The four components (\p top, \p bottom, \p left, and \p right) are Xinerama screen
/// (monitor) indexes (see documentation for `_NET_WM_FULLSCREEN_MONITORS`). Xinerama
/// screens correspond to active XRandR monitors. The list of active XRandR monitors can be
/// displayed using `xrandr --listactivemonitors` on the command line. The order of monitors
/// in this list is consistent with the order of Xinerama screens as seen from
/// Xinerama. While this agreement on order is not mandated by the XRandR protocol
/// specification, it is guaranteed by the X.Org Server (see `ProcRRXineramaQueryScreens()`
/// in `randr/rrxinerama.c` of X.Org Server version 21.1.13).
///
/// The four indexes specify the fullscreen area as follows: The top edge of the fullscreen
/// area is supposed to coincide with the top edge of the Xinerama screen specified by \p
/// top. Likewise for the remaining three indexes, i.e., the bottom edge of the fullscreen
/// area should coincide with the bottom edge of the Xinerama screen specified by \p bottom,
/// etc.
///
/// \note Because the configuration of an X11 screen can change at any time, the meaning of
/// a "fullscreen monitors" specification can end up having an effect that is not the
/// intended one. It is therefore also not an error to specify a Xinerama screen index that
/// is out of range when it applies.
///
/// A "fullscreen monitors" specification can be formatted, i.e., it can be written to an
/// output stream. Indexes are separated by a comma (`,`) and no space will be included
/// after the comma. If all four indexes are equal, only one is shown (`{ 0, 0, 0, 0 }` is
/// formatted as `0`). If \p top is equal to \p left and \p bottom is equal to \p right, two
/// indexes are shown (`{ 0, 1, 0, 1 }` is formatted as `0,1`). Otherwise, all four indexes
/// are shown (`{ 0, 1, 0, 2 }` is formatted as `0,1,0,2`).
///
/// A "fullscreen monitors" specification can be parsed through a value parser (\ref
/// core::BasicValueParserSource). If the parsed string contains only one value, that value
/// is used for all four indexes. If there are two values, the first one will be used for \p
/// top and \p left and the second one will be used for \p bottom and \p right. Otherwise
/// there must be four values, one for each index. The values must be separated by a comma
/// (`,`). Space is allowed after commas.
///
/// \sa \ref display::x11_connection_config::fullscreen_monitors
/// \sa https://specifications.freedesktop.org/wm-spec/latest/ (Extended Window Manager Hints)
///
struct x11_fullscreen_monitors {
    long top;
    long bottom;
    long left;
    long right;
};


/// \brief Write textual representation of "fullscreen monitors" specification to output
/// stream.
///
/// This stream output operator writes a textual representation of the "fullscreen monitors"
/// specification (\p spec) to the specified output stream (\p out). See \ref
/// display::x11_fullscreen_monitors for information on the format of the textual
/// representation.
///
template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out, const display::x11_fullscreen_monitors& spec) ->
    std::basic_ostream<C, T>&;


/// \brief Read textual representation of "fullscreen monitors" specification from source.
///
/// This function reads a textual representation of a "fullscreen monitors" specification
/// (\p spec) from the specified value parser source (\p src). See \ref
/// display::x11_fullscreen_monitors for information on the format of the textual
/// representation. This function is intended to be invoked by a value parser, see \ref
/// core::BasicValueParser for more information.
///
template<class C, class T>
bool parse_value(core::BasicValueParserSource<C, T>& src, display::x11_fullscreen_monitors& spec);








// Implementation


template<class C, class T>
auto operator<<(std::basic_ostream<C, T>& out,
                const display::x11_fullscreen_monitors& spec) -> std::basic_ostream<C, T>&
{
    if (ARCHON_LIKELY(spec.top == spec.left && spec.bottom == spec.right)) {
        if (ARCHON_LIKELY(spec.top == spec.bottom))
            return out << core::as_int(spec.top); // Throws
        return out << core::formatted("%s,%s", core::as_int(spec.top), core::as_int(spec.bottom)); // Throws
    }
    return out << core::formatted("%s,%s,%s,%s", core::as_int(spec.top), core::as_int(spec.bottom),
                                  core::as_int(spec.left), core::as_int(spec.right)); // Throws
}


template<class C, class T>
bool parse_value(core::BasicValueParserSource<C, T>& src, display::x11_fullscreen_monitors& spec)
{
    std::array<int, 4> components = {};
    auto func = [](int& val) {
        return core::as_int(val);
    };
    core::AsListConfig config;
    config.space = core::AsListSpace::allow;
    std::size_t n = {};
    bool success = src.delegate(core::as_list_v(components, std::move(func), n, std::move(config))); // Throws
    if (ARCHON_LIKELY(success)) {
        if (n == 1) {
            spec = { components[0], components[0], components[0], components[0] };
            return true;
        }
        if (n == 2) {
            spec = { components[0], components[1], components[0], components[1] };
            return true;
        }
        if (n == 4) {
            spec = { components[0], components[1], components[2], components[3] };
            return true;
        }
    }
    return false;
}


} // namespace archon::core

#endif // ARCHON_X_DISPLAY_X_X11_FULLSCREEN_MONITORS_HPP
