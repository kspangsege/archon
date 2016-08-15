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


#include <stdexcept>
#include <locale>
#include <sstream>
#include <iomanip>

#include <archon/core/file.hpp>
#include <archon/core/text.hpp>
#include <archon/util/uri.hpp>


using namespace std;
using namespace archon::Core;


/*
reserved    = gen-delims sub-delims
gen-delims  = : / ? # [ ] @
sub-delims  = ! $ & ' ( ) * + , ; =
unreserved  = alpha digit - . _ ~
scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
host        = IP-literal / IPv4address / reg-name
reg-name    = *( unreserved / pct-encoded / sub-delims )
*/


namespace
{
  struct CompCodec
  {
    void encode(string &s) const
    {
      char const *b = s.data();
      char const *const e = b + s.size();
      char const *p = b;
      for (;;) {
        p = ctype_facet.scan_not(ctype_base::alnum, p, e);
        if (p == e) return;
        if (!is_extra(*p)) break;
        ++p;
      }

      ostringstream o;
      o << hex << uppercase << setfill('0'); // RFC 3986 mandates upper case
      for (;;) {
        o.write(b, p-b);
        typedef char_traits<char> traits;
        o << "%" << setw(2) << traits::to_int_type(*p);
        ++p;
        b = p;
        for (;;) {
          p = ctype_facet.scan_not(ctype_base::alnum, p, e);
          if (p == e) {
            o.write(b, p-b);
            s = o.str();
            return;
          }
          if (!is_extra(*p)) break;
          ++p;
        }
      }
    }


    void decode(string &s) const
    {
      char const *b = s.data();
      char const *const e = b + s.size();
      char const *p = find(b, e, '%');
      if (p == e) return;

      string s2;
      s2.reserve(s.size());
      for (;;) {
        s2.append(b, p);
        if (e-p < 3) throw invalid_argument("Unterminate escape sequence in percent encoding");
        s2.append(1, decode_hex(string(p+1, p+3)));
        b = p + 3;
        p = find(b, e, '%');
        if (p == e) {
          s2.append(b, p);
          s = s2;
          return;
        }
      }
    }


    CompCodec():
      loc(locale::classic()), ctype_facet(use_facet<ctype<char> >(loc)), hex_decoder(loc)  {}


  private:
    locale const loc;
    ctype<char> const &ctype_facet;
    Text::HexDecoder<char> const hex_decoder;

    bool is_extra(char c) const
    {
      // According to RFC 3986 section 2.3 Unreserved Characters (January 2005)
      return c == '-' || c == '_' || c == '.' || c == '~';
    }

    char decode_hex(string const &hh) const
    {
      try {
        return hex_decoder.decode(hh);
      }
      catch (Text::ParseException const &e) {
        throw invalid_argument("Bad escape sequence inpercent encoding: " + string(e.what()));
      }
    }
  };
}



namespace archon
{
  namespace Util
  {
    namespace Uri
    {
      string encode_comp(string const &s)
      {
        string s2 = s;
        CompCodec().encode(s2);
        return s2;
      }


      string decode_comp(string const &s)
      {
        string s2 = s;
        CompCodec().decode(s2);
        return s2;
      }



      Decomposed::Decomposed(string const &uri)
      {
        char const *b = uri.data();
        char const *const e = b + uri.size();

        // Scheme
        {
          char const *c = ":/?#";
          char const *p = find_first_of(b, e, c, c+4);
          if (p != e && *p == ':') {
            scheme.assign(b, ++p);
            b = p;
          }
        }

        // Authority
        if (2 <= e-b && b[0] == '/' && b[1] == '/') {
          char const *c = "/?#";
          char const *const p = find_first_of(b+2, e, c, c+3);
          auth.assign(b, p);
          b = p;
        }

        // Path
        {
          char const *c = "?#";
          char const *const p = find_first_of(b, e, c, c+2);
          path.assign(b, p);
          b = p;
        }

        // Query
        {
          char const *const p = find(b, e, '#');
          query.assign(b, p);
          b = p;
        }

        // Fragment
        frag.assign(b, e);
      }


      void Decomposed::set_scheme(string const &s)
      {
        if (!s.empty()) {
          if (s[s.size()-1] != ':')
            throw invalid_argument("URI scheme part must have a trailing ':'");
          if (s.substr(0, s.size()-1).find_first_of(":/?#") != string::npos)
            throw invalid_argument("URI scheme part must not contain '/', '?' or '#', "
                                   "nor may it contain more than one ':'");
        }
        scheme = s;
      }


      void Decomposed::set_auth(string const &s)
      {
        if (!s.empty()) {
          if (s.size() < 2 || s[0] != '/' || s[1] != '/')
            throw invalid_argument("URI authority part must have '//' as a prefix");
          if (s.find_first_of("/?#", 2) != string::npos)
            throw invalid_argument("URI authority part must not contain '?' or '#', "
                                   "nor may it contain '/' beyond the two in the prefix");
        }
        auth = s;
      }


      void Decomposed::set_path(string const &s)
      {
        if (s.find_first_of("?#") != string::npos)
          throw invalid_argument("URI path part must not contain '?' or '#'");
        path = s;
      }


      void Decomposed::set_query(string const &s)
      {
        if (!s.empty()) {
          if (s[0] != '?')
            throw invalid_argument("URI query string must have a leading '?'");
          if (s.find('#', 1) != string::npos)
            throw invalid_argument("URI query string must not contain '#'");
        }
        query = s;
      }


      void Decomposed::set_frag(string const &s)
      {
        if (!s.empty() && s[0] != '#')
          throw invalid_argument("Fragment identifier must have a leading '#'");
        frag = s;
      }


      void Decomposed::canonicalize()
      {
        if (scheme.size() == 1) scheme.clear();
        if (auth.size() == 2)   auth.clear();
        if (path.empty() && (!scheme.empty() || !auth.empty())) path = '/';
        if (query.size() == 1)  query.clear();
        if (frag.size() == 1)   frag.clear();
      }


      void Decomposed::resolve(Decomposed const &base, bool strict)
      {
        if (!strict && scheme == base.scheme) scheme.clear();

        bool normalize_path = true;
        if (scheme.empty()) {
          scheme = base.scheme;
          if (auth.empty()) {
            auth = base.auth;
            if (path.empty()) {
              normalize_path = false;
              path = base.path;
              if (query.empty()) query = base.query;
            }
            else if (path[0] != '/') {
              if (!base.auth.empty() &&  base.path.empty()) path = "/" + path;
              else {
                string::size_type const i = base.path.rfind('/');
                if (i != string::npos) path = base.path.substr(0, i+1) + path;
              }
            }
          }
        }
        if (normalize_path) path = File::canonicalize_path(path, false);
      }



      Params::Params(string const &query)
      {
        if (query.empty()) return;
        if (query[0] != '?') throw invalid_argument("Query string must have a leading '?'");
        CompCodec comp_codec;
        typedef string::const_iterator iter;
        iter i = query.begin();
        iter const e = query.end();
        for (;;) {
          iter const j = find(i, e, '&');
          iter const k = find(i, j, '=');
          string name(i, k);
          comp_codec.decode(name);
          if (k != j) {
            string value(k+1, j);
            comp_codec.decode(value);
            add(name, Value(value));
          }
          else add(name, Value());
          if (j == e) break;
          i = j + 1;
        }
      }


      string Params::serialize() const
      {
        string str, s;
        CompCodec comp_codec;
        typedef Order::const_iterator iter;
        iter const b = order.begin(), e = order.end();
        for (iter i=b; i!=e; ++i) {
          str += i == b ? '?' : '&';
          Map::value_type *const p = *i;
          s = p->first;
          comp_codec.encode(s);
          str += s;
          if(p->second.has_value()) {
            str += '=';
            s = p->second.get();
            comp_codec.encode(s);
            str += s;
          }
        }
        return str;
      }
    }
  }
}
