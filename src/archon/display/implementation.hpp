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
#include <string>
#include <locale>

#include <archon/display/guarantees.hpp>
#include <archon/display/connection.hpp>


namespace archon::display {


/// \brief Representation of an underlying display implementation.
///
/// This class specifies the public interface of a display implementation. A display
/// implementation is a representation of a particular underlying way of interacting with
/// the graphical user interface of the platform. An example is X11 (\ref
/// display::get_x11_implementation_slot()).
///
/// The primary role of a display implementation object is to facilitate the creation of a
/// display connection. See \ref new_connection().
///
/// An implementation object can be obtained by calling \ref
/// display::get_default_implementation() or \ref Slot::get_implementation().
///
/// \sa \ref display::get_default_implementation()
/// \sa \ref Slot::get_implementation()
/// \sa \ref display::get_x11_implementation_slot()
/// \sa \ref display::get_sdl_implementation_slot()
///
class Implementation {
public:
    class Slot;

    /// \brief Connect to display.
    ///
    /// This function is shorthand for calling \ref try_new_connection() and then returning
    /// the connection object on success or throwing an exception on failure.
    ///
    /// \sa \ref try_new_connection()
    ///
    auto new_connection(const std::locale&, const display::Connection::Config& = {}) const ->
        std::unique_ptr<display::Connection>;

    /// \brief Try to connect to display.
    ///
    /// This function attempts to creates a new display connection (\ref display::Connection
    /// using the specified locale and configuration (\p locale, \p config). On success,
    /// this function returns `true` after setting \p conn to the created connection
    /// object. On failure, it returns `false` after setting \p error to a message that
    /// describes the reason for the failure. On success, \p error is left untouched. On
    /// failure, \p conn is left untouched.
    ///
    /// Note that if the implementation was obtained by providing the display guarantee,
    /// \ref display::Guarantees::only_one_connection, then at most one connection may exist
    /// per operating system process at any given time.
    ///
    /// Note that if the implementation was obtained by providing the display guarantee,
    /// \ref display::Guarantees::main_thread_exclusive, then the creation of new
    /// connections must be done only by the main thread. Further more, the returned
    /// connection object must be used only by the main thread. This includes the
    /// destruction of that connection object.
    ///
    /// \sa \ref new_connection()
    ///
    virtual bool try_new_connection(const std::locale& locale, const display::Connection::Config& config,
                                    std::unique_ptr<display::Connection>& conn, std::string& error) const = 0;

    /// \brief Get slot for this implementation.
    ///
    /// This function returns the implementation slot for this implementation.
    ///
    virtual auto get_slot() const noexcept -> const Slot& = 0;

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
/// implementation slots as exposed by \ref display::get_num_implementation_slots() and \ref
/// display::get_implementation_slot().
///
/// An implementation is available if \ref display::Implementation::is_available() would return
/// `true` for the specified guarantees (\p guarantees).
///
/// If there are no available implementations, this function returns null.
///
/// \sa \ref get_default_implementation()
///
auto get_default_implementation_a(const display::Guarantees& guarantees) noexcept -> const display::Implementation*;



/// \brief Slots for individual display implementations.
///
/// Every display implementation is associated with an implementation slot. While a
/// particular implementation may or may not be available on a particular platform, and
/// given a particular set of display guarantees, the corresponding slot is always
/// available. Slots can therefore be used to inquire about an implementation even when it
/// is not available. An application can also iterator over all implementation slots using
/// \ref display::get_num_implementation_slots() and \ref
/// display::get_implementatio_slot(). A particular slot can be looked up by implementation
/// name using \ref display::lookup_implementation().
///
/// A particular display implementation is unavailable for a particular set of display
/// guarantees (\ref display::Guarantees) if \ref get_implementation_a() returns null for
/// that set of guarantees. Otherwise, that display implementation is available for that set
/// of guarantees.
///
class Implementation::Slot {
public:
    /// \brief Unique identifier for implementation.
    ///
    /// This function returns the unique identifier for the implementation in this slot
    /// regardless of whether the implementation is available. This is a short name composed
    /// of lower case letters, digits, and hyphens.
    ///
    virtual auto ident() const noexcept -> std::string_view = 0;

    /// \brief Whether implementation is available for given guarantees.
    ///
    /// This function returns `true` when, and only when the implementation in this slot is
    /// available.
    ///
    /// If `slot` is a display implementation slot, then `slot.is_available(guarantees)` is
    /// shorthand for `bool(slot.get_implementation_a(guarantees))`.
    ///
    bool is_available(const display::Guarantees& guarantees) const noexcept;

    /// \brief Get implementation.
    ///
    /// This function returns the implementation in this slot if it is available for the
    /// specified guarantees. Otherwise, this function throws.
    ///
    /// \sa \ref get_implementation_a()
    ///
    auto get_implementation(const display::Guarantees& guarantees) const -> const Implementation&;

    /// \brief Get implementation if available for given guarantees.
    ///
    /// If the implementation in this slot is available for the specified guarantees, this
    /// function returns a pointer to the implementation. Otherwise this function returns
    /// null.
    ///
    /// \sa \ref get_implementation_a()
    ///
    virtual auto get_implementation_a(const display::Guarantees& guarantees) const noexcept ->
        const Implementation* = 0;

protected:
    ~Slot() noexcept = default;
};


/// \brief Number of display implementation slots.
///
/// This function returns the number of display implementation slots (see \ref
/// display::Implementation::Slot). Each one can be accessed using \ref
/// display::get_implementation_slot().
///
/// \sa \ref display::Implementation::Slot
/// \sa \ref display::get_implementation_slot()
///
int get_num_implementation_slots() noexcept;


/// \brief Get display implementation slot by index.
///
/// This function returns the specified display implementation slot (\ref
/// display::Implementation::Slot). The slot is specified in terms of its index within the
/// built-in list of implementation slots. The number of slots in this list can be obtained
/// by calling \ref display::get_num_implementation_slots().
///
/// \sa \ref display::Implementation::Slot
/// \sa \ref display::get_num_implementation_slots()
///
auto get_implementation_slot(int index) -> const display::Implementation::Slot&;


/// \brief Lookup display implementation by identifier.
///
/// If the specified identifier matches one of the known display implementations (\ref
/// display::Implementation), then this function returns the implementation slot of that
/// implementation regardless of whether that implementation is available. Otherwise, this
/// function returns null.
///
auto lookup_implementation(std::string_view ident) noexcept -> const display::Implementation::Slot*;








// Implementation


inline bool Implementation::Slot::is_available(const display::Guarantees& guarantees) const noexcept
{
    return bool(get_implementation_a(guarantees));
}


} // namespace archon::display

#endif // ARCHON_X_DISPLAY_X_IMPLEMENTATION_HPP
