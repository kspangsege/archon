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

#ifndef ARCHON_CORE_STREAM_UTILS_HPP
#define ARCHON_CORE_STREAM_UTILS_HPP

#include<ios>
#include<ostream>


namespace Archon
{
  namespace Core
  {
    /**
     * This class acts as a sentry, and can be used to safely rollback
     * the format of a standard I/O stream, even in the face of
     * exceptions.
     *
     * When the sentry object is destroyed, the format is
     * automatically reset to the state it had at the time the sentry
     * object was created.
     *
     * Included in the saved state is precisely the format flags, the
     * fill character, and the floating point precision.
     */
    template<class C, class T = std::char_traits<C> > struct BasicIosFormatResetter
    {
      typedef C                   char_type;
      typedef std::basic_ios<C,T> stream_type;

      BasicIosFormatResetter(stream_type &s);
      ~BasicIosFormatResetter();

    private:
      stream_type &stream;
      std::ios_base::fmtflags const orig_flags;
      char_type orig_fill;
      std::streamsize const orig_prec;
    };

    typedef BasicIosFormatResetter<char>    IosFormatResetter;
    typedef BasicIosFormatResetter<wchar_t> WideIosFormatResetter;







    // Implementation:

    template<class C, class T> BasicIosFormatResetter<C,T>::
    BasicIosFormatResetter(stream_type &s):
      stream(s), orig_flags(s.flags()), orig_fill(s.fill()), orig_prec(s.precision()) {}

    template<class C, class T>
    BasicIosFormatResetter<C,T>::~BasicIosFormatResetter()
    {
      stream.fill(orig_fill);
      stream.flags(orig_flags);
      stream.precision(orig_prec);
    }
  }
}

#endif // ARCHON_CORE_STREAM_UTILS_HPP
