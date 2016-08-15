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

#ifndef ARCHON_UTIL_HASH_MAP_HPP
#define ARCHON_UTIL_HASH_MAP_HPP

#include <cstddef>
#include <algorithm>
#include <utility>
#include <vector>

#include <archon/core/iterator.hpp>
#include <archon/util/hashing.hpp>
#include <archon/util/prime.hpp>


namespace archon
{
  namespace Util
  {
    template<class Int> struct HashFuncInt
    {
      static int hash(Int i, int n);
    };

    template<class Float> struct HashFuncFloat
    {
      static int hash(Float f, int n);
    };

    template<class T> struct HashFunc {};

    template<> struct HashFunc<bool>:           HashFuncInt<bool>           {};
    template<> struct HashFunc<char>:           HashFuncInt<char>           {};
    template<> struct HashFunc<signed char>:    HashFuncInt<signed char>    {};
    template<> struct HashFunc<unsigned char>:  HashFuncInt<unsigned char>  {};
    template<> struct HashFunc<wchar_t>:        HashFuncInt<wchar_t>        {};
    template<> struct HashFunc<short>:          HashFuncInt<short>          {};
    template<> struct HashFunc<unsigned short>: HashFuncInt<unsigned short> {};
    template<> struct HashFunc<int>:            HashFuncInt<int>            {};
    template<> struct HashFunc<unsigned>:       HashFuncInt<unsigned>       {};
    template<> struct HashFunc<long>:           HashFuncInt<long>           {};
    template<> struct HashFunc<unsigned long>:  HashFuncInt<unsigned long>  {};
    template<> struct HashFunc<float>:          HashFuncFloat<float>        {};
    template<> struct HashFunc<double>:         HashFuncFloat<double>       {};
    template<> struct HashFunc<long double>:    HashFuncFloat<long double>  {};

    template<class T> struct HashFunc<T *>
    {
      static int hash(T *, int n);
    };

    template<class C, class T, class A> struct HashFunc<std::basic_string<C,T,A> >
    {
      static int hash(std::basic_string<C,T,A> const &s, int n);
    };




    /**
     * Information about a hashing function for use in a HashMap.
     *
     * \tparam K The 'key' type of this map.
     */
    struct RehashPolicy
    {
      static int const init_buckets = 11; // Prime

      static int buckets(int current_num_buckets)
      {
        return std::max<int>(Util::get_prime_not_under(min_growth_factor() * current_num_buckets),
                             current_num_buckets+1);
      }

      static std::size_t limit(int current_num_buckets)
      {
        return std::ceil(max_load_factor() * current_num_buckets);
      }

    private:
      static double min_growth_factor() { return 2; }
      static double max_load_factor()   { return 1; }
    };




    /**
     * A generic hash map implementation.
     *
     * \tparam K The 'key' type of this map.
     *
     * \tparam V The 'value' type of this map.
     *
     * \tparam H Plug-in configuration of the hash map.
     *
     * \note The default hashing function does not work for std::string.
     *
     * \todo FIXME: Must reduce the number of buckets when the number
     * of entries go down, otherwise the entry link updateing
     * procudure will suffer on efficiency.
     *
     * \todo FIXME: Also provide a HashSet variant.
     */
    template<class K, class V, class H = HashFunc<K>, class P = RehashPolicy> struct HashMap
    {
      typedef std::pair<K,V> value_type;

      V &operator[](K const &key);

      bool empty() const throw() { return num_entries == 0; }

      std::size_t size() const throw() { return num_entries; }

      std::size_t bucket_count() const { return buckets.size(); }

      std::size_t bucket_size(std::size_t i) const;

      float load_factor () const { return double(num_entries)/buckets.size(); }

      std::size_t erase(K const &key);

      void clear();

      HashMap(): first_entry(0), num_entries(0), num_entries_rehash_limit(0) {}

      ~HashMap() { clear(); }

    private:
      struct Entry
      {
        value_type v;
        Entry *next;
        Entry(K k): v(k, V()) {}
      };
      struct Bucket
      {
        Entry *first; // Undefined if !last
        Entry *last;
      };
      typedef std::vector<Bucket> Buckets;
      Buckets buckets;
      Entry *first_entry;
      std::size_t num_entries, num_entries_rehash_limit;

      void rehash();

      void append(Entry *, typename Buckets::iterator);

      struct Iter
      {
        typedef std::pair<K,V> value_type;
        static value_type &deref(Entry *e) { return e->v; }
        static Entry *next(Entry *e) { return e->next; }
      };

      struct ConstIter
      {
        typedef std::pair<K,V> const value_type;
        static value_type &deref(Entry const *e) { return e->v; }
        static Entry const *next(Entry const *e) { return e->next; }
      };

    public:
      typedef core::IncIter<Entry       *, Iter>      iterator;
      typedef core::IncIter<Entry const *, ConstIter> const_iterator;

      iterator begin() { return iterator(first_entry); }
      iterator end()   { return iterator(0); }

      const_iterator begin() const { return const_iterator(first_entry); }
      const_iterator end()   const { return const_iterator(0); }
    };






    // Implementation:

    template<class Int> inline int HashFuncInt<Int>::hash(Int i, int n)
    {
      int const w = sizeof(Int) * std::numeric_limits<unsigned char>::digits;
      typedef typename core::FastestUnsignedWithBits<w>::type UInt;
      return UInt(i) % n;
    }


    template<class Float> inline int HashFuncFloat<Float>::hash(Float f, int n)
    {
      Hash_FNV_1a_32 h;
      h.add_float(f);
      return h.get_hash(n);
    }


    template<class T> inline int HashFunc<T *>::hash(T *p, int n)
    {
      Hash_FNV_1a_32 h;
      h.add_int(p);
      return h.get_hash(n);
    }


    template<class C, class T, class A>
    inline int HashFunc<std::basic_string<C,T,A> >::hash(std::basic_string<C,T,A> const &s, int n)
    {
      Hash_FNV_1a_32 h;
      h.add_string(s);
      return h.get_hash(n);
    }


    template<class K, class V, class H, class P>
    inline V &HashMap<K,V,H,P>::operator[](K const &key)
    {
      std::size_t n = buckets.size();
      if(0 < n)
      {
        typename Buckets::iterator const bucket = buckets.begin() + H::hash(key, n);
        Entry *const last = bucket->last;
        if (last) {
          Entry *e = bucket->first;
          for (;;) {
            if (e->v.first == key) return e->v.second;
            if (e == last) break;
            e = e->next;
          }
        }
        if(num_entries < num_entries_rehash_limit)
        {
          Entry *const e = new Entry(key);
          append(e, bucket);
          ++num_entries;
          return e->v.second;
        }
      }
      rehash();
      return operator[](key);
    }


    template<class K, class V, class H, class P>
    inline std::size_t HashMap<K,V,H,P>::bucket_size(std::size_t i) const
    {
      std::size_t n = 0;
      Bucket &b = buckets[i];
      if (b.last) {
        Entry *e = b.first;
        for (;;) {
          ++n;
          if (e == b.last) break;
          e = e->next;
        }
      }
      return n;
    }


    template<class K, class V, class H, class P>
    inline std::size_t HashMap<K,V,H,P>::erase(K const &key)
    {
      std::size_t n = buckets.size();
      if(0 < n)
      {
        typename Buckets::iterator const first_bucket = buckets.begin();
        typename Buckets::iterator const bucket = first_bucket + H::hash(key, n);
        Entry *const last = bucket->last;
        if (last) {
          Entry *e = bucket->first;
          Entry *prev = 0;
          for (;;) {
            if (e->v.first == key) {
              typename Buckets::iterator b;
              if (prev) {
                prev->next = e->next;
                if (e == last) bucket->last = prev;
                goto done;
              }
              if (e == last) bucket->last = 0;
              else bucket->first = e->next;
              b = bucket;
              while (b != first_bucket) {
                --b;
                if (Entry *l = b->last) {
                  l->next = e->next;
                  goto done;
                }
              }
              first_entry = e->next;
            done:
              --num_entries;
              delete e;
              return 1;
            }
            if (e == last) break;
            prev = e;
            e = e->next;
          }
        }
      }
      return 0;
    }


    template<class K, class V, class H, class P> inline void HashMap<K,V,H,P>::clear()
    {
      buckets.clear();
      num_entries_rehash_limit = num_entries = 0;
      Entry *e = first_entry;
      first_entry = 0;
      while(e) {
        Entry *const f = e->next;
        delete e;
        e = f;
      }
    }


    template<class K, class V, class H, class P> inline void HashMap<K,V,H,P>::rehash()
    {
      if(num_entries == 0)
      {
        buckets.resize(P::init_buckets);
        num_entries_rehash_limit = P::limit(P::init_buckets);
        return;
      }
      int const num_new_buckets = P::buckets(buckets.size());
      buckets.resize(num_new_buckets);
      typename Buckets::iterator const buckets_begin = buckets.begin();
      typename Buckets::iterator const buckets_end   = buckets.end();
      for (typename Buckets::iterator i=buckets_begin; i!=buckets_end; ++i) i->last = 0;
      Entry *e = first_entry;
      first_entry = 0;
      while(e) {
        Entry *const f = e->next;
        append(e, buckets_begin + H::hash(e->v.first, num_new_buckets));
        e = f;
      }
      num_entries_rehash_limit = P::limit(num_new_buckets);
    }


    template<class K, class V, class H, class P>
    inline void HashMap<K,V,H,P>::append(Entry *e, typename Buckets::iterator b)
    {
      Entry *const last = b->last;
      b->last = e;
      if (last) {
        e->next = last->next;
        last->next = e;
        return;
      }
      b->first = e;
      typename Buckets::iterator const first_bucket = buckets.begin();
      while (b != first_bucket) {
        --b;
        if (Entry *l = b->last) {
          e->next = l->next;
          l->next = e;
          return;
        }
      }
      e->next = first_entry;
      first_entry = e;
    }
  }
}

#endif // ARCHON_UTIL_HASH_MAP_HPP
