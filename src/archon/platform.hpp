/**
 * \file
 *
 * \author Kristian Spangsege
 */


///////////////////////////////////////////////////////////////////////////
// THIS FILE MUST NOT BE DIRECTLY OR INDIRECTLY INCLUDED BY APPLICATIONS //
///////////////////////////////////////////////////////////////////////////


#include <archon/config.h>


#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif


namespace Archon
{
#ifdef HAVE_STDINT_H

  typedef uintmax_t UIntMax;        // Maximum width integer

#else // not HAVE_STDINT_H

  typedef unsigned long UIntMax;

#endif // HAVE_STDINT_H
}
