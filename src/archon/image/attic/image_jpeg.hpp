/*
 * This file is part of the "Archon" framework.
 * (http://files3d.sourceforge.net)
 *
 * Copyright © 2002 by Kristian Spangsege and Brian Kristiansen.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation under the terms of the GNU General Public License is
 * hereby granted. No representations are made about the suitability of
 * this software for any purpose. It is provided "as is" without express
 * or implied warranty. See the GNU General Public License
 * (http://www.gnu.org/copyleft/gpl.html) for more details.
 *
 * The characters in this file are ISO8859-1 encoded.
 *
 * The documentation in this file is in "Doxygen" style
 * (http://www.doxygen.org).
 */

#ifndef ARCHON_UTILITIES_IMAGE_JPEG_HPP
#define ARCHON_UTILITIES_IMAGE_JPEG_HPP

#include <archon/util/image.hpp>

namespace Archon
{
  namespace Utilities
  {
    /**
     * The JPEG image format used by Image by default.
     *
     * \return 0 if there is no support for JPEG, otherwise a pointer
     * to the default JPEG format. The ownership of the referenced
     * object remains with the callee.
     */
    Image::Format *getDefaultJpegFormat();
  }
}

#endif // ARCHON_UTILITIES_IMAGE_JPEG_HPP
