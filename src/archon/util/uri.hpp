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
 * This file contains functions for working with URIs.
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_UTIL_URI_HPP
#define ARCHON_UTIL_URI_HPP

#include <algorithm>
#include <string>
#include <vector>
#include <map>


namespace archon
{
  namespace util
  {
    namespace Uri
    {
      class Params;


      /**
       * Encode a URI component using percent encoding.
       *
       * \todo FIXME: This function assumes UTF-8.
       */
      std::string encode_comp(std::string const &);


      /**
       * Decode a URI component using percent encoding.
       *
       * \throw std::invalid_argument If the specified string is not a
       * valid percent encoded string. It is valid if, and only if
       * every occurance of '%' is followed by two hexadecimal digits.
       *
       * \todo FIXME: This function assumes UTF-8.
       */
      std::string decode_comp(std::string const &);



      /**
       * Contains a decomposed URI reference. The URI is decomposed
       * into its 5 main components, scheme, authority, path, query,
       * and fragment identifier.
       *
       * This decomposition allows for efficient resolution of a
       * relative URI agains a base URI.
       *
       * For a URI
       * "http://www.ietf.org/rfc/rfc2396.txt?foo=bar#chp3", the
       * result is the following set of components:
       *
       * <pre>
       *
       *   'scheme' -> 'http'
       *   'auth'   -> 'www.ietf.org'
       *   'path'   -> '/rfc/rfc2396.txt'
       *   'query'  -> 'foo=bar'
       *   'frag'   -> 'chp3'
       *
       * </pre>
       *
       * Optionally, the autority component contains a username, a
       * password, and a port number.
       *
       * \sa http://tools.ietf.org/html/rfc3986
       */
      struct Decomposed
      {
        /**
         * Decompose the specified URI reference into its five main
         * parts according to the rules in RFC 3986.
         */
        Decomposed(std::string const &uri);


        /**
         * Reconstruct a URI reference from its 5 components.
         */
        std::string recompose() const { return scheme + auth + path + query + frag; }


        /**
         * Resolve this URI reference against the specified base URI
         * reference according to the rules described in section 5.2
         * of RFC 3986.
         *
         * Be aware that a fragment identifier on the base URI
         * reference is never carried over to the result. This is in
         * accordance with the RFC.
         */
        void resolve(Decomposed const &base, bool strict = true);


        /**
         * Remove empty URI components. Also, for URI references
         * having either a scheme part or an authority part, replace
         * an absent path with '/'.
         */
        void canonicalize();


        /**
         * Get the scheme part of this URI reference including the
         * trailing ':'.
         */
        std::string get_scheme() const { return scheme; }


        /**
         * Get the authority part of this URI reference including the
         * leading '//'.
         */
        std::string get_auth()   const { return auth;   }


        /**
         * Get the path part of this URI reference.
         */
        std::string get_path()   const { return path;   }


        /**
         * Get the query part of this URI reference including the
         * leading '?'.
         */
        std::string get_query()  const { return query;  }


        /**
         * Get the fragment identifier of this URI reference including
         * the leading '#'.
         */
        std::string get_frag()   const { return frag;   }


        /**
         * The specified string must either be empty or have a final
         * ':'. Also, it must not contain '/', '?', or '#', nor may it
         * contain more than one ':'.
         *
         * \throw std::invalid_argument If the specified string is not
         * valid according to the specified rules.
         */
        void set_scheme(std::string const &);


        /**
         * The specified string must either be empty or have '//' as a
         * prefix. Also, it must not contain '?' or '#', nor may it
         * contain '/' beyond the first two.
         *
         * \throw std::invalid_argument If the specified string is not
         * valid according to the specified rules.
         */
        void set_auth(std::string const &);


        /**
         * The specified string must not contain '?' or '#'.
         *
         * \throw std::invalid_argument If the specified string is not
         * valid according to the specified rules.
         */
        void set_path(std::string const &);


        /**
         * The specified string must either be empty or have a leading
         * '?'. Also, it must not contain '#'.
         *
         * \throw std::invalid_argument If the specified string is not
         * valid according to the specified rules.
         */
        void set_query(std::string const &);


        /**
         * Set the query string to the serialized form of the
         * specified set of query parameters. This is slightly faster
         * than set_query(q.encode()) because it avoids the validity
         * check on the string.
         */
        void set_query(Params const &);


        /**
         * The specified string must either be empty or have a leading
         * '#'.
         *
         * \throw std::invalid_argument If the specified string is not
         * valid according to the specified rules.
         */
        void set_frag(std::string const &);


        bool is_absolute() const { return !scheme.empty(); }


      private:
        std::string scheme, auth, path, query, frag;
      };



      /**
       * \todo FIXME: This class assumes UTF-8.
       */
      struct Params
      {
        /**
         * Parse a URI query string into an ordered list of key, value
         * pairs.
         *
         * throw std::invalid_argument if the specified string fails
         * to parse.
         */
        Params(std::string const &query);


        /**
         * Serialize this list of key, value pairs.
         */
        std::string serialize() const;


        /**
         * Get the value of the first entry with the specified key. If
         * the entry has no value, the mepty string will be returned.
         */
        std::string get(std::string const &key) const;


        /**
         * Returns true if, and only if there is at least one entry
         * with the specified key, and the first one has a value.
         */
        bool has_value(std::string const &key) const;


        /**
         * Add the specified key, value pair.
         *
         * If one or more entries already exists with the same key,
         * the value of the first one is modified. Otherwise the
         * effect is the same as add(key, value).
         */
        void set(std::string const &key, std::string const &value);


        /**
         * Same as set(key, "") except that the affected entry will
         * have its '=' removed too.
         */
        void set_no_value(std::string const &key);


        /**
         * Add the specified key, value pair.
         *
         * The new key, value pair is added to the end of the
         * list. Other entries with the same key will be retained.
         */
        void add(std::string const &key, std::string const &value);


        /**
         * Add the specified key without a value. The effect is the
         * same as add(key, "") except that there will be no '='
         * sign. For example, if add("foo", "") would produce
         * 'http://alpha/beta?foo=' then add("foo") would produce
         * 'http://alpha/beta?foo'
         */
        void add(std::string const &key);


        /**
         * Remove all entries with the specified key.
         */
        void remove(std::string const &key);


        /**
         * Remove all entries with the specified key except the first
         * one.
         */
        void remove_recurring(std::string const &key);


      private:
        struct Value;
        typedef std::multimap<std::string, Value> Map;
        typedef std::vector<Map::value_type *> Order;
        Map map;
        Order order;

        void add(std::string const &key, Value const &value);
      };








      // Implementation:

      inline void Decomposed::set_query(Params const &p)
      {
        query = p.serialize();
      }


      struct Params::Value
      {
        Value(): has(false) {}
        Value(std::string const &s): has(true), str(s) {}

        bool has_value() const { return has; }
        std::string get() const { return str; }
        void clear() { has=false; str.clear(); }
        void set(std::string const &s) { has=true; str=s; }

      private:
        bool has;
        std::string str;
      };


      inline std::string Params::get(std::string const &key) const
      {
        Map::const_iterator i = map.find(key);
        return i == map.end() ? std::string() : i->second.get();
      }


      inline bool Params::has_value(std::string const &key) const
      {
        Map::const_iterator i = map.find(key);
        return i != map.end() && i->second.has_value();
      }


      inline void Params::set(std::string const &key, std::string const &value)
      {
        Map::iterator i = map.find(key);
        if (i == map.end()) {
          add(key, value);
          return;
        }
        i->second.set(value);
      }


      inline void Params::set_no_value(std::string const &key)
      {
        Map::iterator i = map.find(key);
        if (i == map.end()) {
          add(key);
          return;
        }
        i->second.clear();
      }


      inline void Params::add(std::string const &key, std::string const &value)
      {
        add(key, Value(value));
      }


      inline void Params::add(std::string const &key)
      {
        add(key, Value());
      }


      inline void Params::remove(std::string const &key)
      {
        std::pair<Map::iterator, Map::iterator> const p = map.equal_range(key);
        for (Map::iterator i = p.first; i != p.second; ++i) {
          order.erase(std::find(order.begin(), order.end(), &*i));
        }
        map.erase(p.first, p.second);
      }


      inline void Params::remove_recurring(std::string const &key)
      {
        std::pair<Map::iterator, Map::iterator> p = map.equal_range(key);
        if (p.first == p.second) return;
        ++p.first;
        for (Map::iterator i = p.first; i != p.second; ++i) {
          order.erase(std::find(order.begin(), order.end(), &*i));
        }
        map.erase(p.first, p.second);
      }


      inline void Params::add(std::string const &key, Value const &value)
      {
        order.push_back(&*map.insert(make_pair(key, value)));
      }
    }
  }
}

#endif // ARCHON_UTIL_URI_HPP
