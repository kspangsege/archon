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

#ifndef ARCHON_X_DISPLAY_X_IMPLEMENTATION_HPP
#define ARCHON_X_DISPLAY_X_IMPLEMENTATION_HPP

/// \file


#include <memory>
#include <string_view>

#include <archon/display/key.hpp>
#include <archon/display/key_code.hpp>
#include <archon/display/guarantees.hpp>
#include <archon/display/connection.hpp>


namespace archon::display {


/// \brief Representation of an underlying display implementation.
///
/// This class specifies the public interface of a display implementation. A display
/// implementation is a bridge to a particular underlying way of interacting with the
/// graphical user interface of the platform. An example is X11 (\ref
/// display::get_x11_implementation()).
///
/// The primary role of a display implementation object is to facilitate the creation of a
/// display connection. See \ref new_connection().
///
/// An implementation object can be obtained by calling \ref
/// display::get_default_implementation(), \ref display::get_implementation(), or \ref
/// display::lookup_implementation().
///
class Implementation {
public:
    /// \brief Unique identifier for implementation.
    ///
    /// This function returns the unique identifier for this display implementation. It is a
    /// short name composed of lower case letters, digits, and hyphens.
    ///
    virtual auto ident() const noexcept -> std::string_view = 0;

    /// \brief Whether implementation is available given particular set of guarantees.
    ///
    /// This function returns `false` if the implementation was disabled at compile time, or
    /// if an insufficient set of guarantees were given (\p guarantees). Otherwise it
    /// returns `true`.
    ///
    virtual bool is_available(const display::Guarantees& guarantees) const noexcept = 0;

    /// \brief Connect to display.
    ///
    /// This function creates a new connection object, which represents a connection to a
    /// particular display, or in some cases, to a particular set of displays (a number of
    /// "screens" using the terminology of the X Window System).
    ///
    /// FIXME: Make note about the requirements that at most one connection is allowed to exists at any time if the display implementation requires \ref display::Guarantees::only_one_connection               
    ///
    /// FIXME: Make note about the requirement that the destruction of the returned unique pointer must happen on the main thread if the display implementation requires \ref display::Guarantees::main_thread_exclusive               
    ///
    virtual auto new_connection(const std::locale&, const display::Guarantees&) const ->
        std::unique_ptr<display::Connection> = 0;

    /// \brief Map well-known key to key code.
    ///
    /// This function maps the specified well-known key (\p key) to the corresponding key
    /// code (\p key_code). If a corresponding key code exists, this function returns `true`
    /// after setting \p key_code to that key code. If a corresponding key code does not
    /// exist, this function returns `false` and leaves \p key_code unchanged.
    ///
    virtual bool try_map_key_to_key_code(display::Key key, display::KeyCode& key_code) const = 0;

    /// \brief Map key code to well-known key.
    ///
    /// This function maps the specified key code (\p key_code) to the corresponding
    /// well-known key (\p key). If a corresponding well-known key exists, this function
    /// returns `true` after setting \p key to that key. If a corresponding well-known key
    /// does not exist, this function returns `false` and leaves \p key unchanged.
    ///
    virtual bool try_map_key_code_to_key(display::KeyCode key_code, display::Key& key) const = 0;

    /// \brief Get key name.
    ///
    /// This function returns the name of the specified key as known to this
    /// implementation. Key names are not guaranteed to be invariant across implementations.
    ///
    /// If the implementation knows the name of the specified key (\p key_code), this
    /// function returns `true` after setting \p name to that name. Otherwise this function
    /// returns `false` and leaves \p name unchanged.
    ///
    virtual bool try_get_key_name(display::KeyCode key_code, std::string_view& name) const = 0;

protected:
    ~Implementation() noexcept = default;
};


/// \brief Get default display implementation.
///
/// This function is like \ref get_default_implementation_a() except that is throws an
/// exception instead of returning null if no display implementations are available.
///
/// \sa \ref get_default_implementation_a()
///
auto get_default_implementation(const display::Guarantees& guarantees) -> const display::Implementation&;


/// \brief Get default display implementation if available.
///
/// This function returns the default display implementation given the specified guarantees
/// (\p guarantees) if one exists.
///
/// The default implementation is the first available implementation in the list of
/// implementations as exposed by \ref display::get_num_implementations() and \ref
/// display::get_implementation().
///
/// An implementation is available if \ref display::Implementation::is_available() would return
/// `true` for the specified guarantees (\p guarantees).
///
/// If there are no available implementations, this function returns null.
///
/// \sa \ref get_default_implementation()
///
auto get_default_implementation_a(const display::Guarantees& guarantees) noexcept -> const display::Implementation*;


/// \brief Number of display implementations.
///
/// This function returns the number of known display implementations (see \ref
/// display::Implementation). Each one can be accessed using \ref
/// display::get_implementation().
///
int get_num_implementations() noexcept;


/// \brief Get display implementation by index.
///
/// This function returns the specified display implementation (\ref
/// display::Implementation). The implementation is specified in terms of its index within
/// the list of known implementations. The number of implementations in this list can be
/// obtained by calling \ref display::get_num_implementations().
///
/// An implementation is not necessarily available. Call \ref
/// display::Implementation::is_available() to see whether it is available.
///
auto get_implementation(int index) -> const display::Implementation&;


/// \brief Lookup display implementation by identifier.
///
/// If the specified identifier matches one of the known display implementations (\ref
/// display::Implementation), this function returns that implementation. Otherwise, this
/// function returns null.
///
auto lookup_implementation(std::string_view ident) noexcept -> const display::Implementation*;


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_IMPLEMENTATION_HPP
