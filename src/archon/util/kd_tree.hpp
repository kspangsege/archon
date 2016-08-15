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

#ifndef ARCHON_UTIL_KD_TREE_HPP
#define ARCHON_UTIL_KD_TREE_HPP

#include <limits>
#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <queue>
#include <ostream>

#include <archon/core/functions.hpp>
#include <archon/math/functions.hpp>
#include <archon/math/vec_ops.hpp>


namespace archon
{
  namespace Util
  {
    /**
     * An abstract kd-tree (or k-dimensional tree).
     *
     * %Thread safety: Instances are not thread safe, but the class
     * is. See \ref ThreadSafety.
     */
    template<typename T, typename Entry, class GetVec> struct BasicKdTree
    {
      typedef Entry entry_type;

      BasicKdTree(int num_components): num_components(num_components), get_vec(), dirty(false) {}

      template<class Iter> void add(Iter begin, Iter end);

      entry_type const &find_nearest(T const *v) const;

      bool empty() const { return entries.empty(); }

      size_t size() const { return entries.size(); }

      void clear() { entries.clear(); dirty = false; }

      template<typename Ch, class Tr>
      std::basic_ostream<Ch, Tr> &print(std::basic_ostream<Ch, Tr> &out) const;

    protected:
      int const num_components;

      void add_quick(Entry const &e) { entries.push_back(e); }
      void mark_dirty() { dirty = true; }

    private:
      struct SortFunc;
      struct FindContext;

      typedef std::pair<T, int> DistVec;
      typedef typename std::vector<Entry>::iterator EntryIter;

      void check_balance() const;
      void balance(EntryIter begin, EntryIter end, int dim) const;

      void find_n_nearest(int begin, int end, int dim, FindContext &) const;

      GetVec const get_vec;
      std::vector<Entry> mutable entries;
      bool mutable dirty;
    };



    /**
     * A kd-tree specialized to work roughly as a map.
     */
    template<typename T, typename H>
    struct KdTreeMap:
      BasicKdTree<T, std::pair<T const *, H>, core::SelectFirst<std::pair<T const *, H> > >
    {
    private:
      typedef std::pair<T const *, H> P;
      typedef BasicKdTree<T, P, core::SelectFirst<P> > B;

    public:
      KdTreeMap(int num_components): B(num_components) {}

      void add(T const *point, H handle) { add_contig(point, handle, 1); }

      void add_contig(T const *, H, size_t n);

      template<class Iter> void add(Iter begin, Iter end) { B::add(begin,end); }

      H const &operator[](T const *v) const { return this->find_nearest(v).second; }
    };



    /**
     * A kd-tree specialized to work roughly as a set.
     */
    template<typename T>
    struct KdTreeSet: BasicKdTree<T, T const *, core::Identity<T const *> >
    {
    private:
      typedef BasicKdTree<T, T const *, core::Identity<T const *> > B;

    public:
      KdTreeSet(int num_components): B(num_components) {}

      void add(T const *point) { add_contig(point, 1); }

      void add_contig(T const *, size_t n);

      template<class Iter> void add(Iter begin, Iter end) { B::add(begin,end); }
    };






    // Implementation

    template<typename T, typename E, class G>
    struct BasicKdTree<T,E,G>::SortFunc: std::binary_function<entry_type, entry_type, bool>
    {
      bool operator()(entry_type const &a, entry_type const &b) const
      {
        return get_vec(a)[dim] < get_vec(b)[dim];
      }
      SortFunc(int dim): dim(dim), get_vec() {}
      int const dim;
      G const get_vec;
    };


    template<typename T, typename E, class G> struct BasicKdTree<T,E,G>::FindContext
    {
      struct MaxDist: std::binary_function<DistVec, DistVec, bool>
      {
        bool operator()(DistVec const &a, DistVec const &b) const { return a.first < b.first; }
      };

      T const *const vec;
      unsigned const num; // Number of points to find

      T sq_radius;
      std::priority_queue<DistVec, std::vector<DistVec>, MaxDist> nearest;

      FindContext(T const *vec, unsigned num, T max_sq_radius):
        vec(vec), num(num), sq_radius(max_sq_radius) {}
    };


    template<typename T, typename E, class G> template<class I>
    inline void BasicKdTree<T,E,G>::add(I begin, I end)
    {
      while(begin != end) entries.push_back(*begin++);
      mark_dirty();
    }


    template<typename T, typename E, class G>
    inline E const &BasicKdTree<T,E,G>::find_nearest(T const *v) const
    {
      check_balance();
      FindContext ctx(v, 1, std::numeric_limits<T>::max());
      find_n_nearest(0, entries.size(), 0, ctx);
      return entries[ctx.nearest.top().second];
    }


    template<typename T, typename E, class G> template<typename C, class U>
    inline std::basic_ostream<C,U> &BasicKdTree<T,E,G>::print(std::basic_ostream<C,U> &o) const
    {
      check_balance();
      G get_vec;
      for(size_t i=0; i<entries.size(); ++i)
      {
        T const *const v = get_vec(entries[i]);
        Math::vec_print(o, v, v + num_components) << std::endl;
      }
      return o;
    }


    template<typename T, typename E, class G>
    inline void BasicKdTree<T,E,G>::check_balance() const
    {
      if(!dirty) return;
      balance(this->entries.begin(), this->entries.end(), 0);
      dirty = false;
    }


    template<typename T, typename E, class G>
    inline void BasicKdTree<T,E,G>::balance(EntryIter begin, EntryIter end, int dim) const
    {
      int const len = end - begin;
      EntryIter const mid = begin + len/2;
      std::nth_element(begin, mid, end, SortFunc(dim));

      int const discr = len - 4;
      if(0 <= discr)
      {
        dim = (dim+1) % num_components;
        balance(begin, mid, dim); // Balance left sub-tree
        if(0 < discr) balance(mid+1, end, dim); // Balance right sub-tree
      }
    }


    template<typename T, typename E, class G>
    inline void BasicKdTree<T,E,G>::find_n_nearest(int begin, int end, int dim,
                                                   FindContext &c) const
    {
    again:
      int const len = end - begin;
      int const mid = begin + len/2;
      entry_type const &entry = entries[mid];

      T const sq_dist = Math::vec_sq_dist(c.vec, c.vec + num_components, get_vec(entry));
      if(sq_dist < c.sq_radius)
      {
        if(c.nearest.size() < c.num)
        {
          c.nearest.push(std::make_pair(sq_dist, mid));
          // If we now have as many as we want, we will only be
          // interested in points that are nearer than the one that is
          // currently furthest away.
          if(c.nearest.size() == c.num) c.sq_radius = c.nearest.top().first;
        }
        else
        {
          // Now we know that the current point is nearer than the one
          // that is furthest away among those currently in the queue,
          // so we need to update it.
          c.nearest.pop();
          c.nearest.push(std::make_pair(sq_dist, mid));
          c.sq_radius = c.nearest.top().first;
        }
      }

      int const discr = len - 2;
      if(0 <= discr)
      {
        T const point = c.vec[dim], split = get_vec(entry)[dim];
        dim = (dim+1) % num_components;
        if(point < split)
        {
          // Look in left subtree first to maximize the chance of skipping
          // the other.
          find_n_nearest(begin, mid, dim, c);

          // Look in right subtree if we have to
          if(0 < discr && Math::square(split - point) <= c.sq_radius)
          {
            begin = mid+1;
            goto again;
          }
        }
        else
        {
          // Look in right subtree first to maximize the chance of
          // skipping the other.
          if(0 < discr) find_n_nearest(mid+1, end, dim, c);

          // Look in left subtree if we have to
          if(Math::square(point - split) <= c.sq_radius)
          {
            end = mid;
            goto again;
          }
        }
      }
    }


    template<typename T, typename H>
    inline void KdTreeMap<T,H>::add_contig(T const *begin, H h, size_t n)
    {
      for(size_t i=0; i<n; ++i)
        this->add_quick(std::make_pair(begin + i*this->num_components, h + i));
      this->mark_dirty();
    }


    template<typename T>
    inline void KdTreeSet<T>::add_contig(T const *begin, size_t n)
    {
      for(size_t i=0; i<n; ++i) this->add_quick(begin + i*this->num_components);
      this->mark_dirty();
    }
  }
}

#endif // ARCHON_UTIL_KD_TREE_HPP
