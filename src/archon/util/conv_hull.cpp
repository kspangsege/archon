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

#include <limits>
#include <algorithm>
#include <stack>
#include <string>
#include <iostream>

#include <archon/core/assert.hpp>
#include <archon/core/generate.hpp>
#include <archon/core/text_join.hpp>
#include <archon/math/geometry.hpp>
#include <archon/util/permutation.hpp>
#include <archon/util/conv_hull.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Math;
using namespace archon::Util;

/*

  Latest error considerations:

  Due to imprecision in numerical calculations the hull construction is
  affected when the order of the vertices are rotated for the
  determination of visibility through a cross product and then a dot
  product. This is due to vary small values of the dot product shifting
  between being negative and being non-negative. This in turn is
  probably due to the presence of coplanar (or nearly coplanar)
  vertices.

  One way to deal with this would be to truncate very small values of
  the dot product to zero (less than n*epsilon where n is as small as
  possible but with a safety margin.) The expected effect would be that
  some vertex may end up lying outside the constructed hull and/or a hull
  that is very slightly concave.

  If one wants infinite precision, the solution might be to switch to
  infinite precision calculations when the ordinary precision
  calculation yields a dot product whose absolute value is less than
  n*epsilon. Infinite calculations can be achieved using the GNU MPFR
  library, or as follows: Represent scalars as implied quotients of
  explicit pairs (numerator, denominator) of infinite precision
  integers. The integer type need only support addition, subtraction and
  multiplication. Addition and subtraction is then done by fraction
  extension, and multiplication and division is done by multiplying
  numerators and denominators.

  When a facet degenerates - two vertices collapse (or very nearly
  collapse), it can product an improper normal, this could in turn be
  misinterpreted as a hole/cavity in the surface, and this violates the
  assumptions and make the "horizon search" algorithm fail
  miserably. The "horizon search" algorithm requires that the looked
  upon object is convex. The effect is that the horizon in such cases
  effectively cosist of multiple distinct boundaries. This could
  probably also happen as a result of co-hyperplanar vertices when they
  are viewed at a very steep angle (think of a ball with a slight dent
  in it, then turn the ball such that the dent remains visible, but is
  close the the edge of the visible part of the ball, now there is a
  section of the dented surface whose normal has an angle less than 90
  degrees with the viewing direction, and thus, the boundary of that
  region is a second horizon). The problem can probably be handled as
  part of the n*epsilon comparison solution since if the facet is
  degenerate, the length of the normal should be zero or very nealy
  zero, leading to a dot product with is zero or very nearly zero.

  Consider whe harmfulness of the following conditions:

  1> The presence of two vertices that are exactly coincident.
  2> The presence of two vertices that are very nearly coincident.
  3> The presence of N vertices that are exactly co-hyperlinear.
  4> The presence of N vertices that are very nearly co-hyperlinear.
  5> The presence of N+1 vertices that are exactly co-hyperplanar.
  6> The presence of N+1 vertices that are very nearly co-hyperplanar.

  Any set of N+2 vertices are necessarily co-hyperspacial, and that is
  of course not a problem.


  The multiple horizons isse is now relibaly detected, but not solved.

  There appears to still be a serious problem. The image gamut
  computation reveals it clearly. The surface is perfectly connection,
  but it is clearly heighly concave and self overlapping. The problem
  likely has to do with coplanar and colinear vertices.


  I tried to set a minimum height for points over a facet when
  repartitioning the accumulator point set. This appeared to have very
  positive consequences. It is almost equivalent to setting the maximum
  allowed distance between the real hull surface and the computed hull
  surface, and also the minimum distance between points of the computed
  hull. One problem, though, is that at vertexes where the inner angle
  of the hull surface is heighly acute, the effective limiting surface
  has a vertex which has an equally acute inner angle and lies much
  farther away from the vertex of the real surface that the specified
  maximum deviation due to the acuteness of the angle.

  Even with this fix, the image gamut computation still fails, either
  with "multiple horizons" or a surface that is both heighly concave and
  heighly self overlapping, but has a perfectly consistent structure
  (neighborship/front/back)

  The reason for this must be found.

*/


/*

  Ideas for N-D horizon search:

  1> Construct the boundary of the initial facet as N fully connected
  ridges.

  2> Visit neighboring facets recursively (depth-first) while expanding
  the boundary along the way.

  3> Visiting retreats from a branch when the neighbor is beyond the
  horizon. When the recursion terminates, the constructed boundary is
  the horizon.

  4> Delete the facets that were within the horizon.

  5> Pick a staring ridge, and visit all its neighbors recursively, and
  construct a new facet for each visited ridge. Each new facet is
  constructed from the vertices of the corresponding ridge plus the
  top vertex from which the horizon was determined. Connect the new
  facets together along the way, and connect each of them with their
  respective original facet next to the boundary.

  6> The hardest part will be to determine a proper order for the
  vertices of new ridges and of new facets such that neighbors always
  are in front/back side agreement.

*/


/*

  Ideas for validation of N-D convex hull:

  1> Generate a set of random vertices.

  2> Validate the integrity of the resulting hypersurface.

  3> Validate the convexity of the resulting hypersurface. For each
  facet use a hyperplane to check that all the vertices of
  the neighbor facets, that are not part of the current origin facet
  (one vertex per neighbor), are behind the current origin facet.

  4> For each facet of the resulting hypersurface, use a hyperplane
  to check that all of the original input vertices except
  those of the current origin facet are strictly behind the current
  origin facet.

  5> There may be a problem with co-hyperplanar facets.

*/

namespace {

struct VertexSet {
    // The index of the associated facet.
    size_t facet;

    // The index of the last vertex of the set of vertices associated
    // with the facet, or numeric_limits<size_t>::max() if the set is
    // empty. The first vertex in the set, next_vertex[last_vertex],
    // is also the highest vertex above the hyperplane of the facet.
    size_t last_vertex;

    VertexSet(size_t f, size_t l): facet(f), last_vertex(l) {}
};


// N-D description of an (N-1)-Simplex (for now, a 3-D description of a triangle)
struct Facet {
    void reset(size_t v0, size_t v1, size_t v2, size_t n0, size_t n1, size_t n2)
    {
        vertices[0]  = v0;
        vertices[1]  = v1;
        vertices[2]  = v2;
        neighbors[0] = n0;
        neighbors[1] = n1;
        neighbors[2] = n2;
    }

    // In counterclockwise order when viewed from the front
    size_t vertices[3];

    // neighbors[0] is the neighboring facet across the edge from vertex 1 to vertex 2.
    // neighbors[1] is the neighboring facet across the edge from vertex 2 to vertex 0.
    size_t neighbors[3];
};

typedef Facet Triangle;

/*
  sturct BoundaryRidge {
  // neighbors[0] is the neighboring ridge across the second vertex
  size_t neighbors[2];

  size_t facet;

  int ridge_pos;
  };
*/


template<class C, class T>
basic_ostream<C,T> &operator<<(std::basic_ostream<C,T> &out, Facet const &f)
{
    return out << out.widen('[') << text_join(f.vertices, f.vertices+3, out.widen(',')) <<
        out.widen(';') << text_join(f.neighbors, f.neighbors+3, out.widen(',')) << out.widen(']');
}


// FIXME: Make this a method of a class called DynamicIndexPartition (efficient: all operations have constant time complexity) (no dynamic memory allocation)
inline size_t remove_vertex(size_t prev, size_t &last, vector<size_t> &next)
{
    size_t const i = next[prev];
    if(i == prev) last = numeric_limits<size_t>::max();
    else
    {
        if(last == i) last = prev;
        next[prev] = next[i];
    }
    return i;
}


inline size_t remove_front_vertex(size_t &last, vector<size_t> &next)
{
    size_t const first = next[last];
    if(first == last) last = numeric_limits<size_t>::max();
    else next[last] = next[first];
    return first;
}


int const tri_inc[3] = { 1, 2, 0 };
int const tri_dec[3] = { 2, 0, 1 };


void generate_trifans_from_surface(size_t num_vertices, vector<Triangle> const &triangles,
                                   size_t entry_triangle, ConvHull::TrifanHandler &handler)
{
    vector<bool> seen_vertices(num_vertices);
    vector<bool> seen_triangles(triangles.size());
    vector<pair<size_t, int> > rim_stack;
    size_t i = entry_triangle; // Index of next triangle to be processed
    int d = 2; // Index in triangle of crossed edge, also index in triangle of next vertex to be added
    Triangle const *t = &triangles[i];
    handler.add_vertex(t->vertices[0]);
    handler.add_vertex(t->vertices[1]);
    seen_vertices[t->vertices[0]] = true;
    seen_vertices[t->vertices[1]] = true;
    for(;;)
    {
        seen_triangles[i] = true;
        size_t const v = t->vertices[d];
        handler.add_vertex(v);
        int c = tri_dec[d]; // Edge to be crossed next
        if(seen_vertices[v])
        {
            int const e = tri_inc[d];
            bool const stop_fan =  seen_triangles[t->neighbors[c]];
            bool const new_fan  = !seen_triangles[t->neighbors[e]];
            if(stop_fan)
            {
                handler.close_trifan();
                if(new_fan) c = e;
                else
                {
                    handler.close_trifan_set();
                    if(rim_stack.empty()) break;
                    i = rim_stack.back().first;
                    c = rim_stack.back().second;
                    rim_stack.pop_back();
                    t = &triangles[i];
                    handler.add_vertex(t->vertices[tri_dec[c]]);
                    handler.add_vertex(t->vertices[tri_inc[c]]);
                }
            }
            else if(new_fan) rim_stack.push_back(make_pair(i,e));
        }
        seen_vertices[v] = true;
        size_t const j = t->neighbors[c];
        t = &triangles[j];
        d = find(t->neighbors, t->neighbors+3, i) - t->neighbors;
        i = j;
    }
}


template<class T> struct TempValOverride {
    TempValOverride(T &var, T const &new_val): var(var), orig_val(var) { var = new_val; }
    ~TempValOverride() { var = orig_val; }
private:
    T &var;
    T const orig_val;
};


template<int N> struct Validator {
    Validator(vector<Facet> const &f, wostream *logger = 0):
        logger(logger), valid(true), facets(f), seen_facets(f.size()), num_facets_seen(0) {}

    void validate(size_t start_facet, size_t num_facets_used)
    {
        current_facet = current_neighbor = numeric_limits<size_t>::max();
        if(!assert(start_facet < facets.size(),
                   L"Index of start facet is out of range")) return;
        validate_facet(start_facet);
        assert(num_facets_seen == num_facets_used, L"Mismatch in number of facets");
    }

    bool is_valid() const { return valid; }

private:
    void validate_facet(size_t facet)
    {
        TempValOverride<size_t> current_facet_override(current_facet, facet);
        current_facet = facet;
        seen_facets[facet] = true;
        ++num_facets_seen;
        Facet const &f = facets[facet];
        for(int i=1; i<N; ++i)
            if(!assert(f.vertices[i] != f.vertices[0], L"Non-distinct facet vertices")) break;
        for(int i=1; i<N; ++i)
            if(!assert(f.neighbors[i] != f.neighbors[0], L"Non-distinct facet neighbors")) break;
        for(int i=0; i<N; ++i)
            if(!assert(f.neighbors[i] != facet, L"Facet is its own neighbor")) break;
        for(int i=0; i<N; ++i)
        {
            size_t const neighbor = f.neighbors[i];
            if(!assert(neighbor < facets.size(),
                       L"Index of facet neighbor is out of range")) continue;
            if(!seen_facets[neighbor]) validate_facet(neighbor);
            if(facet < neighbor) continue;
            size_t ridge[N];
            Parity parity;
            get_ridge(i, f, ridge, parity);
            validate_neigborship(facet, neighbor, ridge, parity);
        }
    }

    void validate_neigborship(size_t origin, size_t neighbor, size_t const *ridge1, Parity parity1)
    {
        TempValOverride<size_t> current_neighbor_override(current_neighbor, neighbor);
        Facet const &f = facets[neighbor];
        int const i = find(f.neighbors, f.neighbors+N, origin) - f.neighbors;
        if(!assert(i < N, L"Nonmutual facet neighborship")) return;
        size_t ridge2[N];
        Parity parity2;
        get_ridge(i, f, ridge2, parity2);
        // Determine the parity of the permutation that maps ridge1[0:N-1] to ridge2[0:N-1]
        pair<Parity, size_t const *> const r =
            get_parity_of_permutation(ridge2, ridge2+(N-1), ridge1);
        if(!assert(r.second == ridge1+(N-1), L"Facet neighbors disagree on shared ridge")) return;
        Parity const total_parity = parity1 + r.first + parity2;
        assert(total_parity == Parity::odd, L"Front/back disagreement between facet neighbors");
    }

    bool assert(bool cond, wstring message)
    {
        if(cond) return true;
        if(logger)
        {
            *logger << L"Surface validation error: " << message << "\n";
            *logger << L"FACET "<<current_facet<<": "<<facets[current_facet]<<"\n";
            if(current_neighbor != numeric_limits<size_t>::max())
                *logger << L"NEIGHBOR "<<current_neighbor<<": "<<facets[current_neighbor]<<"\n";
            logger->flush();
        }
        valid = false;
        return false;
    }

    void get_ridge(int i, Facet const &f, size_t *ridge, Parity &parity)
    {
        int n = (i+1) % N;
        rotate_copy(f.vertices, f.vertices+n, f.vertices+N, ridge);
        parity = Parity(n * (N-1));
    }

    wostream *const logger;
    size_t current_facet;
    size_t current_neighbor;
    bool valid;
    vector<Facet> const &facets;
    vector<bool> seen_facets;
    size_t num_facets_seen;
};


// Verification of soundness of surface structure
void validate_surface(vector<Facet> const &facets, size_t entry_facet,
                      size_t num_used_facets)
{
    Validator<3> v(facets, &wcerr);
    v.validate(entry_facet, num_used_facets);
    ARCHON_ASSERT_1(v.is_valid(), "Invalid surface");
}

} // unnamed namespace


namespace archon {
namespace Util {
namespace ConvHull {

/**
 * Quick hull
 */
void compute(vector<Vec3> const &vertices, TrifanHandler &handler, int max_depth)
{
    size_t const num_vertices = vertices.size();
    cerr << "########################################### num_vertices = " << num_vertices << endl;
    if(num_vertices < 1)
    {
        cerr << "Degenerate -1-D case" << endl;
        return; // FIXME: Degenerate -1-D case
    }

    vector<Facet> facets;
    vector<size_t> unused_facets;
    size_t entry_facet = 0;
    {
        vector<size_t> next_vertex(num_vertices);
        generate(next_vertex.begin(), next_vertex.end()-1, make_inc_generator<size_t>(1));
        next_vertex.back() = 0;
        size_t last_vertex = num_vertices - 1;

        size_t idx0 = 0, idx1 = 0;
        {
            Vec3 min(std::numeric_limits<double>::infinity()),
                max(-std::numeric_limits<double>::infinity());
            size_t min_x_j = 0, min_y_j = 0, min_z_j = 0, max_x_j = 0, max_y_j = 0, max_z_j = 0;
            size_t j = last_vertex;
            do
            {
                size_t const i = next_vertex[j];
                Vec3 const &v = vertices[i];
                if(v[0] < min[0]) { min[0] = v[0]; min_x_j = j; }
                if(max[0] < v[0]) { max[0] = v[0]; max_x_j = j; }
                if(v[1] < min[1]) { min[1] = v[1]; min_y_j = j; }
                if(max[1] < v[1]) { max[1] = v[1]; max_y_j = j; }
                if(v[2] < min[2]) { min[2] = v[2]; min_z_j = j; }
                if(max[2] < v[2]) { max[2] = v[2]; max_z_j = j; }
                j = i;
            }
            while(j != last_vertex);
            size_t j0 = 0, j1 = 0;
            // FIXME: Choose the axis where the span is largest for stability reasons
            if(min[0] < max[0])      { j0 = min_x_j; j1 = max_x_j; }
            else if(min[1] < max[1]) { j0 = min_y_j; j1 = max_y_j; }
            else if(min[2] < max[2]) { j0 = min_z_j; j1 = max_z_j; }
            else
            {
                cerr << "Degenerate 0-D case" << endl;
                return; // FIXME: Degenerate 0-D case
            }
            idx0 = remove_vertex(j0,                   last_vertex, next_vertex);
            idx1 = remove_vertex(j1 == idx0 ? j0 : j1, last_vertex, next_vertex);
        }

        size_t idx2 = 0;
        {
            Line3 const line(vertices[idx0], vertices[idx1] - vertices[idx0]);
            double max = 0;
            size_t max_j = 0;
            if(last_vertex != numeric_limits<size_t>::max())
            {
                size_t j = last_vertex;
                do
                {
                    size_t const i = next_vertex[j];
                    double const d = sq_sum(line.direction * (line.origin-vertices[i]));
                    if(max < d)
                    {
                        max = d;
                        max_j = j;
                    }
                    j = i;
                }
                while(j != last_vertex);
            }
            if(max == 0)
            {
                cerr << "Degenerate 1-D case" << endl;
                return; // FIXME: Degenerate 1-D case
            }
            idx2 = remove_vertex(max_j, last_vertex, next_vertex);
        }

        size_t idx3 = 0;
        {
            Hyperplane3 const plane((vertices[idx1]-vertices[idx0]) *
                                    (vertices[idx2]-vertices[idx0]), vertices[idx0]);
            double max = 0;
            size_t max_j = 0;
            bool max_pos = false;
            if(last_vertex != numeric_limits<size_t>::max())
            {
                size_t j = last_vertex;
                do
                {
                    size_t const i = next_vertex[j];
                    double const h = plane.height(vertices[i]);
                    double const d = abs(h);
                    if(max < d)
                    {
                        max = d;
                        max_j = j;
                        max_pos = 0 < h;
                    }
                    j = i;
                }
                while(j != last_vertex);
            }
            if(max == 0)
            {
                cerr << "Degenerate 2-D case" << endl;
                return; // FIXME: Degenerate 2-D case
            }
            if(max_pos) swap(idx0, idx1);
            idx3 = remove_vertex(max_j, last_vertex, next_vertex);
        }

        facets.resize(4);
        facets[0].reset(idx0, idx1, idx2, 3, 1, 2);
        facets[1].reset(idx0, idx2, idx3, 3, 2, 0);
        facets[2].reset(idx0, idx3, idx1, 3, 0, 1);
        facets[3].reset(idx1, idx3, idx2, 1, 0, 2);

        vector<size_t> new_facets;
        new_facets.push_back(0);
        new_facets.push_back(1);
        new_facets.push_back(2);
        new_facets.push_back(3);

        vector<bool> seen_facets;
        vector<size_t> removed_facets;
        vector<VertexSet> vertex_sets;
        // If 'i' is the index of a facet, then facet_vertex_sets[i]
        // is the index the set of vertices associated with the
        // facet, or it is numeric_limits<size_t>::max() if the
        // facet has no associated vertex set.
        vector<size_t> facet_vertex_sets(facets.size(), numeric_limits<size_t>::max());
        // Pairs (origin, neighbor) of facet indices for each ridge
        // that remains to be crossed.
        // FIXME: Would it not be better to store (origin, ridge_pos)?
        vector<pair<size_t, size_t> > ridge_stack;

        int depth = 0;
        for(;;)
        {
            // Partition the current vertex set: For each of the new
            // facets, there will be a part of the partition with all
            // the vertices that lie in front of the facet. Finally,
            // there will be a part of the partition with all the
            // remaining vertices that do not lie in front of any of
            // the new facets. Of course there is no unique way to
            // construct this partition, since vertices may lie in
            // front of multiple facets, but this is not a problem for
            // the algorithm; we just put those vertices arbitrarily
            // into one of the applicable partition parts.
            {
                entry_facet = new_facets.front();
                for(vector<size_t>::const_iterator k=new_facets.begin(); k!=new_facets.end(); ++k) cerr << "New facet ["<<*k<<"]: "<<facets[*k]<< endl;
                if(0 < max_depth && ++depth == max_depth) goto output;
                typedef vector<size_t>::const_iterator iter;
                iter const e = new_facets.end();
                for(iter k=new_facets.begin(); k!=e; ++k)
                {
                    if(last_vertex == numeric_limits<size_t>::max()) break;
                    size_t const new_facet = *k;
//cerr << "new_facet = " << new_facet << endl;
                    Facet &f = facets[new_facet];
                    size_t const i0 = f.vertices[0], i1 = f.vertices[1], i2 = f.vertices[2];
// cerr << "i0 = " << i0 << ", i1 = " << i1 << ", i2 = " << i2 << endl;
                    Vec3 const &v0 = vertices[i0], &v1 = vertices[i1], &v2 = vertices[i2];
                    Hyperplane3 const plane(unit((v1-v0)*(v2-v0)), v0);
                    double max = 0;
                    size_t first_front = numeric_limits<size_t>::max(),
                        prev_front = numeric_limits<size_t>::max(), max_prev;
                    size_t j = last_vertex;
                    for(;;)
                    {
                        size_t const i = next_vertex[j];
                        bool const is_last = i == last_vertex;
                        double const h = plane.height(vertices[i]);
                        // FIXME: Make similar tests when constructing the initial simplex.
                        if(h <= 0.5/256) // FIXME: Parameterize this
                        {
                            if(is_last) break;
                            j = i;
                            continue;
                        }
                        if(max < h)
                        {
                            max = h;
                            max_prev = prev_front;
                        }
                        remove_vertex(j, last_vertex, next_vertex);
                        if(first_front == numeric_limits<size_t>::max()) first_front = i;
                        else next_vertex[prev_front] = i;
                        prev_front = i;
                        if(is_last) break;
                    }
                    if(first_front == numeric_limits<size_t>::max()) continue;
                    next_vertex[prev_front] = first_front;
                    if(max_prev == numeric_limits<size_t>::max()) max_prev = prev_front;
                    facet_vertex_sets[new_facet] = vertex_sets.size();
                    vertex_sets.push_back(VertexSet(new_facet, max_prev));
                }
                new_facets.clear();
            }


            // Find a non-empty vertex set and its associated facet,
            // look back at the hull from the vertex in the set that
            // is heighest above the facet, remove all the hull facets
            // that are visible from this vertex (find the horizon),
            // and then add new facets connecting the horizon with the
            // heighest vertex.
            {
                size_t i0;
                for(;;)
                {
                    if(vertex_sets.empty()) goto output;
                    VertexSet &s = vertex_sets.back();
                    if(s.last_vertex != numeric_limits<size_t>::max())
                    {
                        i0 = s.facet;
                        last_vertex = s.last_vertex;
                        vertex_sets.pop_back();
                        break;
                    }
                    vertex_sets.pop_back();
                }
                size_t prev_new_facet = numeric_limits<size_t>::max(), first_new_facet;
                size_t prev_new_facet_third_vertex = 0, first_new_facet_second_vertex;
                size_t const top_vertex = remove_front_vertex(last_vertex, next_vertex);
                cerr << "=========== Decomposing facet "<<i0<<" "<<facets[i0]<<" using top_vertex = "<<top_vertex<<endl;
                // When seen from the top point, we will discover the
                // ridges of the horizon in a counter clockwise order
                seen_facets.clear();
                seen_facets.resize(facets.size());
                seen_facets[i0] = true;
                for(int e=0; e<3; ++e)
                {
                    size_t i = facets[i0].neighbors[e];
                    if(seen_facets[i]) continue;
                    size_t j = i0;
                    for(;;)
                    {
                        Facet *f = &facets[i];
                        int const e0 = find(f->neighbors, f->neighbors+3, j) - f->neighbors;
                        ARCHON_ASSERT(e0 < 3); // FEXME: Remove me!
                        int const e1 = tri_inc[e0];
                        int const e2 = tri_dec[e0];
//                  size_t const j0 = f->vertices[0], j1 = f->vertices[1], j2 = f->vertices[2];
                        size_t const j0 = f->vertices[e0], j1 = f->vertices[e1], j2 = f->vertices[e2];
                        Vec3 const &v0 = vertices[j0], &v1 = vertices[j1], &v2 = vertices[j2];
                        cerr << "NORMAL ["<<i<<"] = "<<(v1-v0)*(v2-v0)<<"   DOT = "<<dot((v1-v0)*(v2-v0), vertices[top_vertex]-v0)<<endl;
                        if(dot((v1-v0)*(v2-v0), vertices[top_vertex]-v0) <= 0)
                        {
                            // Boundary ridge detected
                            cerr << "Boundary ["<<f->vertices[e2]<<","<<f->vertices[e1]<<"] due to ("<<j<<") "<<facets[j]<<" -> ("<<i<<") "<<facets[i]<<endl;
                            // Allocate a new facet
                            size_t new_facet;
                            if(unused_facets.empty())
                            {
                                new_facet = facets.size();
                                facets.resize(new_facet+1);
                                facet_vertex_sets.resize(new_facet+1, numeric_limits<size_t>::max());
                                ARCHON_ASSERT(i < facets.size()); // FEXME: Remove me!
                                f = &facets[i]; // Since the vector may reallocate memory
                            }
                            else
                            {
                                new_facet = unused_facets.back();
                                unused_facets.pop_back();
                                facet_vertex_sets[new_facet] = numeric_limits<size_t>::max();
                            }
                            if(prev_new_facet == numeric_limits<size_t>::max())
                            {
                                first_new_facet = new_facet;
                                first_new_facet_second_vertex = f->vertices[e2];
//                      first_new_facet_second_vertex = j2;
                            }
                            else
                            {
                                ARCHON_ASSERT_1(f->vertices[e2] == prev_new_facet_third_vertex, "Multiple horizons detected");
//                      ARCHON_ASSERT_1(j2 == prev_new_facet_third_vertex, "Multiple horizons detected");
                                facets[prev_new_facet].neighbors[1] = new_facet;
                            }
                            f->neighbors[e0] = new_facet; // Make the new facet a neighbor of the detected boundary facet
                            facets[new_facet].reset(top_vertex, f->vertices[e2], f->vertices[e1],
                                                    i, numeric_limits<size_t>::max(), prev_new_facet);
//                    facets[new_facet].reset(top_vertex, j2, j1, i, numeric_limits<size_t>::max(), prev_new_facet);
                            new_facets.push_back(new_facet);
                            prev_new_facet = new_facet;
                            prev_new_facet_third_vertex = f->vertices[e1];
//                    prev_new_facet_third_vertex = j1;
                        }
                        else
                        {
                            ARCHON_ASSERT(i < seen_facets.size()); // FEXME: Remove me!
                            seen_facets[i] = true;
                            // Transfer all the vertices from the obsolete
                            // facet to the accumulator point set
                            size_t const vertex_set = facet_vertex_sets[i];
                            if(vertex_set != numeric_limits<size_t>::max())
                            {
                                ARCHON_ASSERT(vertex_set < vertex_sets.size()); // FEXME: Remove me!
                                VertexSet &s = vertex_sets[vertex_set];
                                if(last_vertex == numeric_limits<size_t>::max()) last_vertex = s.last_vertex; // FIXME: Rename last_vertex to accum_last_vertex
                                else
                                {
                                    ARCHON_ASSERT(s.last_vertex < next_vertex.size() && last_vertex < next_vertex.size()); // FEXME: Remove me!
                                    swap(next_vertex[s.last_vertex], next_vertex[last_vertex]);
                                }
                                s.last_vertex = numeric_limits<size_t>::max();
                            }
                            // Facet indices need to be unique over a
                            // horizion search, so we cannot release facets
                            // just yet
                            removed_facets.push_back(i);
                            if(!seen_facets[f->neighbors[e1]])
                            {
                                ridge_stack.push_back(make_pair(f->neighbors[e2], i));
                                j = i;
                                i = f->neighbors[e1];
                                continue;
                            }
                            else if(!seen_facets[f->neighbors[e2]])
                            {
                                j = i;
                                i = f->neighbors[e2];
                                continue;
                            }
                        }
                      pop:
                        if(ridge_stack.empty()) break;
                        i = ridge_stack.back().first;
                        j = ridge_stack.back().second;
                        ridge_stack.pop_back();
                        if(seen_facets[i]) goto pop;
                    }
                }
                ARCHON_ASSERT(prev_new_facet_third_vertex == first_new_facet_second_vertex);
                // Connect first and last facet
                ARCHON_ASSERT(first_new_facet < facets.size() && prev_new_facet < facets.size()); // FEXME: Remove me!
                facets[first_new_facet].neighbors[2] = prev_new_facet;
                facets[prev_new_facet].neighbors[1]  = first_new_facet;
                unused_facets.push_back(i0);
                // Delete stale facets
                cerr << "Delete facet ["<<i0<<"]: "<<facets[i0]<< endl;
                {
                    typedef vector<size_t>::const_iterator iter;
                    iter const e = removed_facets.end();
                    for(iter i=removed_facets.begin(); i!=e; ++i) unused_facets.push_back(*i);
                    for(iter i=removed_facets.begin(); i!=e; ++i) cerr << "Delete facet ["<<*i<<"]: "<<facets[*i]<< endl;
                }
                removed_facets.clear();
            }
        }
    }

  output:
    {
        size_t const num_used_facets = facets.size() - unused_facets.size();
        validate_surface(facets, entry_facet, num_used_facets);
    }
    generate_trifans_from_surface(num_vertices, facets, entry_facet, handler);
}




void TriangleHandler::add_vertex(size_t i)
{
    switch(state)
    {
        case 0: vertex0 = i; state = 1; break;

        case 1: vertex1 = i; state = 2; break;

        case 2:
            vertex2 = i;
            state = 3;
            add_triangle(vertex0, vertex1, vertex2);
            break;

        default:
            vertex1 = vertex2;
            vertex2 = i;
            add_triangle(vertex0, vertex1, vertex2);
            break;
    }
}


void TriangleHandler::close_trifan()
{
    vertex0 = vertex2;
    state = 2;
}


void TriangleHandler::close_trifan_set()
{
    state = 0;
}

} // namespace ConvHull
} // namespace Util
} // namespace archon





/*

Gift wrapping

Is a much simpler algorithm and lends itself to a much
simpler and leaner implementation, but suffers from sensitivity to
lack of numerical precision, and from slow speed due to higher time
complexity.


namespace
{
  struct RimFrag
  {
    size_t first, last;
    Vec3 proj_z;
    RimFrag(size_t first, size_t last, Vec3 const &proj_z):
      first(first), last(last), proj_z(proj_z) {}
  };
}

namespace archon
{
  namespace Util
  {
    namespace ConvHull
    {
      void compute(vector<Vec3> const &points, TrifanHandler &handler)
      {
        size_t const num_points = points.size();
        if(num_points == 0) return;

        // Create a list of all vertex indices. This will be used to
        // keep track of an order without moving the actual points
        // around.
        vector<size_t> index_list;
        index_list.resize(num_points);
        generate(index_list.begin(), index_list.end(), make_inc_generator<size_t>());

        // Find the first vertex P0: Any vertex with minimum
        // x-coordinate, and bring it to the first position in the
        // index list.
        {
          size_t min_i;
          double min_v = numeric_limits<double>::infinity();
          for(size_t i=0; i<num_points; ++i)
          {
            Vec3 const &p = points[i];
            if(p[0] < min_v)
            {
              min_v = p[0];
              min_i = i;
            }
          }
          if(min_i != 0) swap(index_list[0], index_list[min_i]);
          handler.add_vertex(index_list[0]);
        }

        if(num_points < 2) return;

        // The second vertex, P1, is the one where the line through it
        // and P0 has the minimum slope in the x-y-plane (y/x). Find
        // it, and bring it to the second position in the index list.
        {
          size_t min_i;
          double min_v = numeric_limits<double>::infinity();
          Vec3 const &p0 = points[index_list[0]];
          for(size_t i=1; i<num_points; ++i)
          {
            Vec3 const &p = points[index_list[i]];
            double x = p[0] - p0[0];
            double y = p[1] - p0[1];
            ARCHON_ASSERT(0 <= x);
            double const slope = y/x;
            if(slope < min_v)
            {
              min_v = slope;
              min_i = i;
            }
          }
          if(min_i != 1) swap(index_list[1], index_list[min_i]);
          handler.add_vertex(index_list[1]);
        }

        if(num_points < 3) return;

        size_t first_undecided = 2; // Index within index_list of first
        // undecided vertex, where an
        // undecided vertext is one that has
        // not yet been identified as beeing
        // part of the convex hull.

        size_t first_in_rim = 0; // Index within index_list of first vertex in current surface rim
        size_t last_in_rim  = 1; // Index within index_list of last vertex in current surface rim
        Vec3 last_proj_z = Vec3(0,0,1); // z = x * y of previous projection plane

        stack<RimFrag> rim_stack;

        bool error = false;
        size_t error_1, error_2, error_3;

        bool fan_faces = false, fans = false;
        for(;;)
        {
          Vec3 const pivot_vertex = points[index_list[first_in_rim]];

          // Construct a projection plane normal to the current pivoting edge
          Vec3 const proj_z = unit(pivot_vertex - points[index_list[last_in_rim]]);
          Vec3 const proj_x = unit(proj_z * last_proj_z);
          Vec3 const proj_y = proj_z * proj_x;
          // Search for the vertex with the minimum slope in the
          // projection plane. Only undecided vertices, and vertices in
          // the current rim (except the first and last ones) are
          // searched.
          size_t min_i = numeric_limits<size_t>::max();
          double min_v = numeric_limits<double>::infinity();
          for(size_t i=first_in_rim+1; i<last_in_rim; ++i)
          {
            Vec3 const p = points[index_list[i]] - pivot_vertex;
            double x = dot(proj_x, p);
            double const y = dot(proj_y, p);
            if(x < 0)
            {
              if(x < -10*numeric_limits<double>::epsilon())
              {
                cerr << "ERROR: (1) Very negative x = " << x << endl;
                error_1 = index_list[first_in_rim];
                error_2 = index_list[last_in_rim];
                error_3 = index_list[i];
                if(fan_faces)
                {
                  handler.close_trifan();
                  fans = true;
                }
                if(fans) handler.close_trifan_set();
                error = true;
                goto stop;
              }
//            cerr << "WARNING: (1) Negative x = " << x << endl;
              x = 0;
            }
            double const slope = y/x;
            if(slope < min_v)
            {
              min_v = slope;
              min_i = i;
            }
          }
          for(size_t i=first_undecided; i<num_points; ++i)
          {
            Vec3 const p = points[index_list[i]] - pivot_vertex;
            double x = dot(proj_x, p);
            double const y = dot(proj_y, p);
            if(x < 0)
            {
              if(x < -10*numeric_limits<double>::epsilon())
              {
                cerr << "ERROR: (2) Very negative x = " << x << endl;
                error_1 = index_list[first_in_rim];
                error_2 = index_list[last_in_rim];
                error_3 = index_list[i];
                if(fan_faces)
                {
                  handler.close_trifan();
                  fans = true;
                }
                if(fans) handler.close_trifan_set();
                error = true;
                goto stop;
              }
//            cerr << "WARNING: (2) Negative x = " << x << endl;
              x = 0;
            }
            double const slope = y/x;
            if(slope < min_v)
            {
              min_v = slope;
              min_i = i;
            }
          }
          if(min_i == numeric_limits<size_t>::max())
          {
//          cerr << "WARNING: Nothing found!" << endl;
            break;
          }

          handler.add_vertex(index_list[min_i]);
          fan_faces = true;

          if(last_in_rim < min_i)
          {
            ++last_in_rim;
            bool const first = min_i == first_undecided;
            if(last_in_rim == first_undecided)
            {
              if(!first) swap(index_list[last_in_rim], index_list[min_i]);
            }
            else
            {
              index_list[last_in_rim] = index_list[min_i];
              if(!first) index_list[min_i] = index_list[first_undecided];
            }
            ++first_undecided;
            last_proj_z = proj_z;
            continue;
          }

          size_t rim_len = last_in_rim - first_in_rim; // Number of edges in rim
          ARCHON_ASSERT_1(1 < rim_len, "Short rim");

          if(rim_len == 2)
          {
            handler.close_trifan();
            fans = true;
            handler.close_trifan_set();
            if(rim_stack.size() == 0) break; // All done
            fan_faces = fans = false;
            RimFrag const &rim = rim_stack.top();
            first_in_rim = rim.first;
            last_in_rim = rim.last;
            last_proj_z = rim.proj_z;
            rim_stack.pop();
            handler.add_vertex(index_list[first_in_rim]);
            handler.add_vertex(index_list[last_in_rim]);
            continue;
          }

          if(min_i == last_in_rim - 1)
          {
            --last_in_rim;
            last_proj_z = proj_z;
            continue;
          }

          if(min_i == first_in_rim + 1)
          {
            ++first_in_rim;
            handler.close_trifan();
            fans = true;
            fan_faces = false;
            last_proj_z = -proj_z;
            continue;
          }

          rim_stack.push(RimFrag(first_in_rim, min_i, proj_z));
          first_in_rim = min_i;
          handler.close_trifan();
          fans = true;
          fan_faces = false;
          last_proj_z = -proj_z;
        }

      stop:

        if(error)
        {
          cerr << "ERROR: "<<error_1<<","<<error_2<<","<<error_3<<" = "
            ""<<points[error_1]<<","<<points[error_2]<<","<<points[error_3] << endl;
        }
      }
    }
  }
}

*/



/*

        cerr << "Status:\n";
        {
          cerr << "Sets:";
          for(size_t i=0; i<trifan_sets.size(); ++i) cerr << " " << trifan_sets[i];
          cerr << endl;
          cerr << "Fans:";
          for(size_t i=0; i<trifans.size(); ++i) cerr << " " << trifans[i];
          cerr << endl;

          size_t fan_idx = 0, tri_idx = 0, vtx_idx = 0;
          for(size_t i=0; i<trifan_sets.size(); ++i)
          {
            size_t n = trifan_sets[i], m = trifans[fan_idx++];
            size_t i0 = vertices[vtx_idx++];
            size_t i1 = vertices[vtx_idx++];
            Vec3 p0 = points[i0];
            Vec3 p1 = points[i1];
            for(;;)
            {
              size_t const i2 = vertices[vtx_idx++];
              Vec3 const p2 = points[i2];

              cerr << "set="<<(i+1)<<"/"<<trifan_sets.size()<<", "
                "fan="<<fan_idx<<"/"<<trifan_sets[i]<<", "
                "tri="<<tri_idx<<" = " << i0 << "," << i1 << "," << i2 << " = "
                "" << p0 << ", " << p1 << ", " << p2 << endl;
              ++tri_idx;

              if(--m == 0)
              {
                if(--n == 0) break;
                m = trifans[fan_idx++];
                i0 = i2;
                p0 = p2;
              }
              else
              {
                i1 = i2;
                p1 = p2;
              }
            }
          }
        }

*/
