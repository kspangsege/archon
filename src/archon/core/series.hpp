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

#ifndef ARCHON_CORE_SERIES_HPP
#define ARCHON_CORE_SERIES_HPP

#include <algorithm>
#include <ios>


namespace archon
{
  namespace core
  {
    template<int N, class T> struct SeriesBase
    {
      typedef T element_type;
      static int const size = N;

      T       &operator[](int i)       { return v[i]; }
      T const &operator[](int i) const { return v[i]; }

      bool operator==(SeriesBase const &w) { return std::equal(v, v+N, w.v); }

      T       *get()       { return v; }
      T const *get() const { return v; }

      void set(T w)        { std::fill(v, v+N, w); }
      void set(T const *w) { std::copy(w, w+N, v); }

    private:
      T v[N];
    };

    template<class T> struct SeriesBase<0,T>
    {
      typedef T element_type;
      static int const size = 0;

      bool operator==(SeriesBase const &) { return true; }

      T       *get()       { return nullptr; }
      T const *get() const { return nullptr; }

      void set(T)         {}
      void set(T const *) {}
    };

    /**
     * \todo FIXME: This class should be called <tt>Array</tt>, and
     * the class that is currently called Array should be
     * <tt>UniqueArray</tt>.
     */
    template<int N, class T> struct Series: SeriesBase<N,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T const *v) { this->set(v); }
    };

    template<class T> struct Series<1,T>: SeriesBase<1,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T const *v) { this->set(v); }
    };

    template<class T> struct Series<2,T>: SeriesBase<2,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T v1, T v2) { T v[2] = { v1, v2 }; this->set(v); }
      Series(T const *v) { this->set(v); }
    };

    template<class T> struct Series<3,T>: SeriesBase<3,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T v1, T v2, T v3) { T v[3] = { v1, v2, v3 }; this->set(v); }
      Series(T const *v) { this->set(v); }
    };

    template<class T> struct Series<4,T>: SeriesBase<4,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T v1, T v2, T v3, T v4) { T v[4] = { v1, v2, v3, v4 }; this->set(v); }
      Series(T const *v) { this->set(v); }
    };

    template<class T> struct Series<5,T>: SeriesBase<5,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T v1, T v2, T v3, T v4, T v5) { T v[5] = { v1, v2, v3, v4, v5 }; this->set(v); }
      Series(T const *v) { this->set(v); }
    };

    template<class T> struct Series<6,T>: SeriesBase<6,T>
    {
      Series() {}
      Series(T v) { this->set(v); }
      Series(T v1, T v2, T v3, T v4, T v5, T v6) { T v[6] = { v1, v2, v3, v4, v5, v6 }; this->set(v); }
      Series(T const *v) { this->set(v); }
    };


    /**
     * Print the specified series onto the specified output stream.
     */
    template<class C, class S, int N, class T>
    std::basic_ostream<C,S> &operator<<(std::basic_ostream<C,S> &out, Series<N,T> const &s)
    {
      C const comma(out.widen(','));
      if(0 < N)
      {
        out << s[0];
        for(int i=1; i<N; ++i) out << comma << s[i];
      }
      return out;
    }


    /**
     * Parse characters from the specified input stream as a series of
     * the same type as the one specified. If successful, assign the
     * result to the specified series.
     */
    template<class C, class S, int N, class T>
    std::basic_istream<C,S> &operator>>(std::basic_istream<C,S> &in, Series<N,T> &s)
    {
      Series<N,T> t;
      bool bad = false;
      C const comma(in.widen(','));
      C ch;
      if(0 < N)
      {
        in >> t[0];
        if(!in) bad = true;
        else
        {
          for(int i=1; i<N; ++i)
          {
            if(in.eof())
            {
              // Fill remaining elements with value of last one given.
              T const v = t[i-1];
              do t[i] = v; while(++i < N);
              break;
            }

            in >> ch >> t[i];
            if(!in || ch != comma)
            {
              bad = true;
              break;
            }
          }
        }
      }
      if(bad) in.setstate(std::ios_base::badbit);
      else s = t;
      return in;
    }
  }
}

#endif // ARCHON_CORE_SERIES_HPP
