/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <archon/util/unicode.hpp>

using namespace std;

namespace archon
{
  namespace util
  {
    namespace Unicode
    {
      wstring tolower(wstring s)
      {
        wchar_t b[512];
        wstring::size_type n = s.size(), i = 0;
        wstring t;
        t.reserve(n);
        while(i < n)
        {
          wstring::size_type m = s.copy(b, sizeof(b)/sizeof(*b), i);
          getUnicodeFacet()->tolower(b, b+m);
          t.append(b, m);
          i += sizeof(b)/sizeof(*b);
        }
        return t;
      }

      wstring toupper(wstring s)
      {
        wchar_t b[512];
        wstring::size_type n = s.size(), i = 0;
        wstring t;
        t.reserve(n);
        while(i < n)
        {
          wstring::size_type m = s.copy(b, sizeof(b)/sizeof(*b), i);
          getUnicodeFacet()->toupper(b, b+m);
          t.append(b, m);
          i += sizeof(b)/sizeof(*b);
        }
        return t;
      }
    }
  }
}
