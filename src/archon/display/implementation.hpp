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

#include <archon/display/connection.hpp>


namespace archon::display {


/// \brief    
///
///    
///
class Implementation {
public:
    struct Mandates;

    /// \brief    
    ///
    ///    
    ///
    virtual auto ident() const noexcept -> std::string_view = 0;

    /// \bief    
    ///
    ///    
    ///
    virtual bool is_available(const Mandates&) const noexcept = 0;

    /// \brief    
    ///
    ///    
    ///
    virtual auto new_connection(const std::locale&, const Mandates&) const -> std::unique_ptr<display::Connection> = 0;

protected:
    ~Implementation() noexcept = default;
};


/// \brief    
///
///    
///
struct ExclusiveSDLMandate {};


/// \brief    
///
///    
///
struct Implementation::Mandates {
    /// \brief    
    ///
    ///    
    ///
    const display::ExclusiveSDLMandate* exclusive_sdl_mandate = nullptr;
};


/// \brief Get default display implementation.
///
/// This function returns the default display implementation given the specified mandates
/// (\p mandates) if one exists.
///
/// The default implementation is the first available implementation in the list of
/// implementations as exposed by \ref display::get_num_implementations() and \ref
/// display::get_implementation().
///
/// An implementation is available if \ref display::Implementation::is_available() returns
/// `true` for the specified mandates (\p mandates).
///
/// If there are no available implementations, this function returns null.
///
auto get_default_implementation(const display::Implementation::Mandates& mandates) noexcept ->
    const display::Implementation*;


/// \brief Number of display implementations.
///
/// This function returns the number of known display implementations (see \ref
/// display::Implementation). Each one can be accessed using \ref
/// display::get_implementation().
///
int get_num_implementations() noexcept;


/// \brief Get display implementation by index.
///
/// This functio returns the specified display implementation (\ref
/// display::Implementation). The implementation is specified in terms of its index within
/// the list of known implementations. The number of implementations in this list can be
/// obtained by calling \ref display::get_num_implementations().
///
/// An implementation is not necessarily available. Call \ref
/// display::Implementation::is_available() to see whether it is available.
///
auto get_implementation(int index) -> const display::Implementation&;


/// \brief Lookup display implmentation by identifier.
///
/// If the specified identifier matches one of the known display implementations (\ref
/// display::Implementation), this function returns that implementation. Otherwise, this
/// function returns null.
///
auto lookup_implementation(std::string_view ident) noexcept -> const display::Implementation*;


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_IMPLEMENTATION_HPP
