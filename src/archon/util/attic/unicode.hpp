/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_UTIL_UNICODE_HPP
#define ARCHON_UTIL_UNICODE_HPP

#include <cwchar>
#include <string>
#include <locale>

namespace archon
{
  namespace util
  {
    /**
     * \sa http://www.research.att.com/~bs/3rd_loc.pdf
     */
    namespace Unicode
    {
      /**
       * Get a classic C locale but upgraded so that wchar_t holds a
       * Unicode code point and UTF-8 is the character encoding used
       * in wide stream I/O.
       *
       * Do the following to set the wcin/wcout/wcerr streams to use
       * the UTF-8 character codec:
       *
       * <PRE>
       *
       *   ios_base::sync_with_stdio(false);
       *   wcin.imbue(Unicode::getUnicodeLocale());
       *
       * </PRE>
       *
       * This should be one of the first things you do in 'main'. No
       * reference must be made to wcin/wcout/wcerr before this
       * code. If instead you want _all_ wide stream I/O to use the
       * UTF-8 codec (including wcin/wcout/wcerr), do the following
       * instead of the above:
       *
       * <PRE>
       *
       *   locale::global(Unicode::getUnicodeLocale());
       *   ios_base::sync_with_stdio(false);
       *
       * </PRE>
       */
      inline std::locale getUnicodeLocale()
      {
	static std::locale l(std::locale::classic(), std::locale("en_US.UTF-8"), std::locale::ctype);
	return l;
      }

      inline std::ctype<wchar_t> const *getUnicodeFacet()
      {
	static std::ctype<wchar_t> const *f = &std::use_facet<std::ctype<wchar_t> >(getUnicodeLocale());
	return f;
      }

      inline bool isspace(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::space, c);
      }

      inline bool isprint(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::print, c);
      }

      inline bool iscntrl(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::cntrl, c);
      }

      inline bool isupper(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::upper, c);
      }

      inline bool islower(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::lower, c);
      }

      inline bool isalpha(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::alpha, c);
      }

      inline bool isdigit(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::digit, c);
      }

      inline bool ispunct(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::punct, c);
      }

      inline bool isxdigit(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::xdigit, c);
      }

      inline bool isalnum(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::alnum, c);
      }

      inline bool isgraph(wchar_t c)
      {
	return getUnicodeFacet()->is(std::ctype_base::graph, c);
      }


      std::wstring tolower(std::wstring);
      std::wstring toupper(std::wstring);
    }
  }
}

#endif // ARCHON_UTIL_UNICODE_HPP
