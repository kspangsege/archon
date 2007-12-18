/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_DISPLAY_IMPLEMENTATION_HPP
#define ARCHON_DISPLAY_IMPLEMENTATION_HPP

#include <archon/display/connection.hpp>


namespace Archon
{
  namespace Display
  {
    /**
     * Thrown if no display implementations are available.
     */
    struct NoImplementationException: std::runtime_error
    {
      NoImplementationException(): std::runtime_error("") {}
    };

    /**
     * Thrown if a display connection could not be established.
     *
     * \sa Implementation::new_connection
     * \sa Connection
     */
    struct NoDisplayException: std::runtime_error
    {
      NoDisplayException(std::string m): std::runtime_error(m) {}
    };

    /**
     * Thrown if an implementation parameter name is not recognized.
     */
    struct BadParamException: std::runtime_error
    {
      BadParamException(std::string m): std::runtime_error(m) {}
    };

    /**
     * Thrown by most of the methods in the display library if the
     * connection to the display is lost or otherwise corrupted.
     */
    struct BadConnectionException: std::runtime_error
    {
      BadConnectionException(std::string m): std::runtime_error(m) {}
    };



    struct Implementation
    {
      typedef Core::SharedPtr<Implementation> Ptr;
      typedef Ptr const &Arg;

      /**
       * Get the mnemonic that identifies this display implementation.
       *
       * As of Jun 19 2009 there is only an X11 Library based
       * implementation, and its mnemonic is "xlib".
       *
       * \return A short mnemonic that identifies this implementation.
       *
       * \note This method is thread-safe.
       *
       * \sa Screen
       */
      virtual std::string get_mnemonic() const = 0;

      /**
       * Create a new connection to the default display for this
       * implementation.
       *
       * On the X Window System this is a connection to the display
       * mentioned in the DISPLAY environment variable (often the
       * display of the local host.)
       *
       * \return A display connection.
       *
       * \throw NoDisplayException If no display was found.
       *
       * \note This method is thread-safe.
       *
       * \sa Connection
       */
      virtual Connection::Ptr new_connection() = 0;

      /**
       * Set an implementation specific paramter.
       *
       * As of Jun 19 2009 there is only an X11 Library based
       * implementation, and it has only one defined parameter that
       * can be set. The name of this parameter is <tt>display</tt>,
       * and it determines which X server that should be connected
       * to. It has the same format as the \c DISPLAY environment
       * variable. If this parameter is not set, the value of the
       * environment variable will be used. The format is <tt><host
       * name>:<display number>.<default screen number></tt>.
       *
       * \throw BadParamException If the specified parameter name is
       * not recognized by this implementation.
       *
       * \note This method is thread-safe.
       */
      virtual void set_param(std::string name, std::string value) = 0;

      virtual ~Implementation() {}
    };


    /**
     * Get an instance of the default implementation.
     *
     * \return The default display implementation.
     *
     * \throw NoImplementationException If no display implementation
     * was available.
     *
     * \note This function is thread-safe.
     *
     * \sa Implementation
     */
    Implementation::Ptr get_default_implementation() throw(NoImplementationException);

    /**
     * Get the number of display implementations.
     *
     * \return The number of available implementations.
     *
     * \note This function is thread-safe.
     *
     * \sa get_implementation
     */
    int get_num_implementations();

    /**
     * Get the specified display implementation.
     *
     * \param index The implementation index.
     *
     * \return A display implementation.
     *
     * \note This function is thread-safe.
     *
     * \sa Implementation
     */
    Implementation::Ptr get_implementation(int index) throw(std::out_of_range);
  }
}

#endif // ARCHON_DISPLAY_IMPLEMENTATION_HPP
