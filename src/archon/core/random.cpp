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

#include <unistd.h>
#include <cmath>
#include <stdexcept>
#include <limits>
#include <utility>

#include <archon/core/atomic.hpp>
#include <archon/core/time.hpp>
#include <archon/core/random.hpp>


using namespace std;
using namespace archon::Core;

namespace
{
  archon::Core::Atomic counter;


  /**
   * Algorithm 'lifted' from "The Art of Computer Programming", volume
   * 2, by Donnald E. Knuth.
   */
  struct NormalDistribution: Random::Distribution
  {
    double get()
    {
      double x;
      // Do we have a value from last time?
      if(stock)
      {
        x = y;
        stock = false;
      }
      else for(;;)
      {
         // Will loop 1.27 times on average, with a standard deviation
         // of 0.587.
        double v = 2*r.get_uniform()-1, w = 2*r.get_uniform()-1;
        double s = v*v + w*w;
        if(s < 1)
        {
          if(s) s = sqrt(-2*log(s)/s);
          x = v * s;
          y = w * s;
          stock = true;
          break;
        }
      }
      return deviation * x + mean;
    }

    NormalDistribution(double mean, double deviation):
      mean(mean), deviation(deviation), stock(false) {}

    Random r;
    double const mean, deviation;
    double y;
    bool stock;
  };


  /**
   * Code 'lifted' from "Newran02C" which is a comprehensive, general
   * purpose library for generating random numbers, by R B Davies.
   *
   * \sa http://www.robertnz.net/nr02doc.htm
   */
  struct FiniteDistribution: Random::Distribution
  {
    double get()
    {
      int i = v.size() * r.get_uniform();
      pair<double, int> const &p = v[i];
      return r.get_uniform() < p.first ? p.second : i;
    }

    FiniteDistribution(vector<double> const &probs)
    {
      size_t const n = probs.size();
      v.reserve(n);
      double sum = 0;
      for(size_t i = 0; i<n; ++i)
      {
        double p = probs[i];
        if(p < 0) throw invalid_argument("Negtive probability");
        sum += p;
        v.push_back(make_pair(0.0, -1));
      }
      for(size_t i=0; i<n; ++i)
      {
        double pmin = 1, pmax = -1;
        int jmin = -1, jmax = -1;
        for(size_t j=0; j<n; ++j)
        {
          pair<double, int> const &e = v[j];
          if(e.second < 0)
          {
            double p = probs[j]/sum - e.first;
            if(pmax <= p) { pmax = p; jmax = j; }
            if(p <= pmin) { pmin = p; jmin = j; }
          }
        }
        v[jmin].second  = jmax;
        double p = 1.0/n - pmin;
        v[jmax].first += p;
        p *= n;
        v[jmin].first = p;
      }
    }

    Random r;
    vector<pair<double, int> > v;
  };


  /**
   * Code 'lifted' from "Newran02C" which is a comprehensive, general
   * purpose library for generating random numbers, by R B Davies.
   *
   * \sa http://www.robertnz.net/nr02doc.htm
   */
  struct AsymmetricDistribution: Random::Distribution
  {
    virtual double density(double) const = 0;

    virtual double get()
    {
      double ak, y;
      int ir1;
      if(not_ready) init();
      do
      {
        double r1 = r.get_uniform();
        int ir = r1 * xi;
        double sxi = sx[ir];
        ir1 = ir == ic ? 120 : ir+1;
        ak = sxi + (sx[ir1]-sxi) * r.get_uniform();
        y = sfx[ir] * r.get_uniform();
      }
      while(sfx[ir1] <= y && density(ak) <= y);
      return ak;
    }

    void init()
    {
      sx.resize(121);
      sfx.resize(121);
      double sxi = mode;
      int i;
      for(i=0; i<120; ++i)
      {
        sx[i] = sxi;
        double f1 = density(sxi);
        sfx[i] = f1;
        if(f1 <= 0) goto L20;
        sxi += 0.01 / f1;
      }
      throw runtime_error("AsymmetricDistribution: Area too large (a)");

    L20:
      ic = i-1;
      sx[120] = sxi;
      sfx[120] = 0;
      sxi = mode;
      for(; i<120; ++i)
      {
        sx[i] = sxi;
        double f1 = density(sxi);
        sfx[i] = f1;
        if(f1 <= 0) goto L30;
        sxi -= 0.01/f1;
      }
      throw runtime_error("AsymmetricDistribution: Area too large (b)");

    L30:
      if(i < 100) throw runtime_error("AsymmetricDistribution: area too small");
      xi = i;

      not_ready = false;
    }

    AsymmetricDistribution(double mode): mode(mode), not_ready(true) {}

  private:
    Random r;
    double const mode;
    bool not_ready;
    vector<double> sx, sfx;
    double xi;
    int ic;
  };


  /**
   * Code 'lifted' from "Newran02C" which is a comprehensive, general
   * purpose library for generating random numbers, by R B Davies.
   *
   * \sa http://www.robertnz.net/nr02doc.htm
   */
  double ln_gamma(double xx)
  {
    // log gamma function adapted from numerical recipes in C

    if(xx < 1) // Use reflection formula
    {
      double piz = M_PI * (1-xx);
      return log(piz/sin(piz)) - ln_gamma(2-xx);
    }

    static double cof[6] =
      {
        76.18009173,  -86.50532033,     24.01409822,
        -1.231739516,   0.120858003e-2, -0.536382e-5
      };

    double x = xx - 1;
    double tmp = x + 5.5, ser = 1;
    tmp -= (x+0.5) * log(tmp);
    for(int j=0; j<=5; ++j)
    {
      x   += 1;
      ser += cof[j] / x;
    }
    return -tmp + log(2.50662827465*ser);
  }


  /**
   * Code 'lifted' from "Newran02C" which is a comprehensive, general
   * purpose library for generating random numbers, by R B Davies.
   *
   * \sa http://www.robertnz.net/nr02doc.htm
   */
  struct PoissonDistributionHighMean: AsymmetricDistribution
  {
    double get()
    {
      return floor(AsymmetricDistribution::get());
    }

    // Poisson density function
    double density(double x) const
    {
      if(x < 0) return 0;
      double ix = floor(x);
      double l = ln_lambda * ix - lambda - ln_gamma(1 + ix);
      return  l < -40 ? 0 : exp(l);
    }

    PoissonDistributionHighMean(double lambda):
      AsymmetricDistribution(lambda), lambda(lambda), ln_lambda(log(lambda)) {}

    double lambda, ln_lambda;
  };
}

namespace archon
{
  namespace Core
  {
    Random::Random()
    {
      Time t = Time::now();
      unsigned long s = t.get_as_seconds();
      s += t.get_nanos_part();
      s += getpid();
      // Make sure that two instances made in quick succession still
      // get different seeds
      s += ++counter;
      seed(s);
    }


    Random::Random(unsigned long v)
    {
      seed(v);
    }


    void Random::seed(unsigned long v)
    {
      xsubi[0] = xsubi[1] = xsubi[2] = 0;
      int n = numeric_limits<unsigned long>::digits / 16;
      for(int i=0; i<n; ++i) xsubi[(i+1)%3] += v >> i * 16;
    }


    UniquePtr<Random::Distribution>
    Random::get_normal_distrib(double mean, double deviation)
    {
      UniquePtr<Distribution> d(new NormalDistribution(mean, deviation));
      return d;
    }


    UniquePtr<Random::Distribution>
    Random::get_poisson_distrib(double lambda)
    {
      if(lambda <= 8)
      {
        vector<double> probs;
        probs.push_back(exp(-lambda));
        for(int i=1; i<40; ++i) probs.push_back(probs[i-1]*lambda/i);
        UniquePtr<Random::Distribution> d(get_finite_distrib(probs).release());
        return d;
      }
      UniquePtr<Distribution> d(new PoissonDistributionHighMean(lambda));
      return d;
    }


    UniquePtr<Random::Distribution>
    Random::get_finite_distrib(vector<double> const &probs)
    {
      UniquePtr<Distribution> d(new FiniteDistribution(probs));
      return d;
    }
  }
}
