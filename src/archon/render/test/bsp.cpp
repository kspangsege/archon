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

#include <GL/gl.h>

#include <archon/core/unique_ptr.hpp>
#include <archon/core/build_config.hpp>
#include <archon/math/geometry.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::math;
using namespace archon::render;

namespace
{
  /**
   * A \c Scene defines the following types:
   *
   * Polygon - A copyable object representing or identifying a polygon.
   */
/*
  template<class Scene> struct BSP_Tree
  {
    template<class Iter> BSP_Tree(size_t num_components, Iter polygons_begin, Iter polygons_end):
      num_components(num_components), root(build(polygons_begin, polygons_end)) {}

    template<class Op> void traverse(T const *origin, T const *dir, Op const &op, T cutoff_angle, T cutoff_dist)
    {
      Traverse<Op>(origin, dir, op, cutoff_angle, cutoff_dist).traverse(root);
    }


  private:

    struct Node
    {
      typename Scene::Polygon polygon;
      Node *front, *back;
    };


    Node const *build()
    {
      for(Iter i = polygons_begin; i != polygons_end; ++i) add_poly(*i);
  polygon = extractPolygonFromList(lastInList, polygon);

  tree->splitPlane = polygon->surface->plane;
  
  insertPolygonAtEndOfList(polygon, &tree->lastFrontPolygon);

#ifdef PROFILING
  ++bsp_numberOfPolygons;
#endif

  while(*lastInList)
  {
    polygon = extractPolygonFromList(lastInList, *lastInList);

    switch(clasifyPolygon(&tree->splitPlane, polygon))
    {
    case polygon_ON_PLANE:
      if(dotProduct(&polygon->surface->plane.normal, &tree->splitPlane.normal)>0) // We might be able to get this info from clasifyPolygon
	insertPolygonAtEndOfList(polygon, &tree->lastFrontPolygon);
      else
	insertPolygonAtEndOfList(polygon, &tree->lastBackPolygon);

#ifdef PROFILING
      ++bsp_numberOfPolygons;
#endif
      break;

    case polygon_BEFORE_PLANE:
      insertPolygonAtEndOfList(polygon, &lastInFrontList);
      break;

    case polygon_BEHIND_PLANE:
      insertPolygonAtEndOfList(polygon, &lastInBackList);
      break;

    case polygon_CROSSING_PLANE:
      splitPolygon(polygon, &tree->splitPlane, &frontPiece, &backPiece);
      deletePolygon(polygon);
      insertPolygonAtEndOfList(frontPiece, &lastInFrontList);
      insertPolygonAtEndOfList(backPiece,  &lastInBackList);

#ifdef PROFILING
      ++bsp_numberOfPolygonSplits;
#endif
      break;
    }
  }

  if(lastInFrontList)
  {
    tree->front = createBSP_Tree();
    bsp_build(tree->front, &lastInFrontList);
  }

  if(lastInBackList)
  {
    tree->back = createBSP_Tree();
    bsp_build(tree->back, &lastInBackList);
  }
    }


    template<class Op> struct Traverse
    {
      void traverse(Node const *node)
      {
        T const dist = math::vec_dot(node->normal, node->normal + num_components, origin) + node->dist;
        T const angle = std::acos(math::vec_dot(node->normal, , dir));
        if(dist > 0)
        {
          // Origin in front of the split plane
          if(node->back && angle < M_PI/2+cutoff_angle &&
             dist < std::cos(angle-cutoff_angle)*cutoff_dist) traverse(node->back);
          op(node->polygon);
          if(node->front) traverse(node->front);
        }
        else
        {
          // Origin behind the split plane
          if(node->front && M_PI/2-cutoff_angle < angle &&
             0 < dist+cos(M_PI-angle-cutoff_angle*M_PI/180)*cutoff_dist) traverse(node->front);
          if(node->back) traverse(node->back);
        }
      }

      Traverse(T const *origin, T const *dir, Op op, T cutoff_angle, T cutoff_dist):
        origin(origin), dir(dir), op(op), cutoff_angle(cutoff_angle), cutoff_dist(cutoff_dist) {}

    private:
          void apply();

      T const *const origin, *const dir;
      Op const op;
      T const cutoff_angle; // Half the outer field of view
      T const cutoff_dist;  // Distance from eye to far clipping plane along cone edge
    };


    size_t const num_components;
    Node const *const root;
  };


  struct Vertex
  {
    Vec3 point;
    vec2 tex_point;
  };

  struct Material
  {
    int texture_id;
  };

  struct Polygon
  {
    vector<Vertex> vertices;
    Material const *material;
  };
*/





/*
  struct PolygonSet
  {
    vector<Vec3> points;
    vector<size_t> vertices; // Indices into 'points'.
    vector<size_t> polygons; // One plus index of last vertex of polygon.
  };
*/



//Consider using the representation from conv_hull.cpp of solid objects using triangles specifying their neighboring trinagles.


/*


An ordered pair of distinct points on a line establishes a basis for a unique description of any point on that line

1-D: A point on a line divides that line into two parts.
2-D: Two non-coincident points on a plane divides that plane into two parts.
3-D: Three non-colinear points in a space divides that space into two parts.
4-D: Four non-coplanar points in a 4-space divides that 4-space into two parts.

1-D: Two non-coincident points on a line encloses a finite region of that line.
2-D: Three non-colinear points on a plane encloses a finite region of that plane.
3-D: Four non-coplanar points in a space encloses a finite region of that space.
4-D: Five non-cospacial points in a 4-space encloses a finite region of that 4-space.

There is a problem here - it requires a specific basis:
2-D: Two ordered non-coincident points on a plane creates a 1-D feature that has a front and a back side.
3-D: Three ordered non-colinear points in a space creates a 2-D feature that has a front and a back side.
4-D: Four ordered non-coplanar points in a 4-space creates a 3-D feature that has a front and a back side.

 */

// See http://fluxionsdividebyzero.com/p1/math/calculus/geometry/g037.html

  /**
   * A N-dimensional description of an (N-1)-dimensional hypersurface
   * constructed from a set of (N-1)-simplices. This could for example
   * be the boundary of an N-dimensional polytope. The surface has
   * orientation, so the notions of front and back are well defined.
   *
   * N=2 A line constructed from straight segments (e.g. a polygon)
   * N=3 A surface constructed from triangles (e.g. a polyhedron)
   * N=4 A solid constructed from tetrahedrons (e.g. a polychoron)
   *
   * N=1 - The facets are points
   * N=2 - The facets are line segments
   * N=3 - The facets are triangles
   * N=4 - The facets are tetrahedrons
   * N=5 - The facets are pentachorons
   */
  template<int N> struct FacetedSurface
  {
    /**
     * An (N-1)-simplex that knows its neighbors across its own
     * (N-2)-subsimplices.
     *
     * An (N-1)-simplex is composed of N subsimplices. The
     * subsimplices are (N-2)-simplices. For example, a triangle,
     * which is a 2-simplex, is composed of 3 line segments, which are
     * 1-simplices.
     *
     * The i'th subsimplex arise from the vertices as follows:
     *
     * 2-D: Line segment consisting of two points: The i'th point is the i'th vertex.
     *
     *         point
     *         -----
     * vertex | 0 1
     *
     * 3-D: Triangle consisting of 3 line segments: The i'th oriented line segment runs from i'th vertex to the (i+1)'th vertex.
     *
     *      line segment
     *         -------
     * vertex | 0 1 2
     *        | 1 2 0
     *
     * 4-D: Tetrahedron consisting of 4 triangles: The i'th triangle
     *
     *         triangle
     *         ---------
     *        | 0 1 2 3
     * vertex | 1 2 3 0
     *        | 2 3 0 1
     *
     *
     * Neighbors:
     *
     * Line segment in 2-D:
     *
     * vertex  neighbor
     * ------------------
     * 0           1
     * 1           0
     *
     * Trinagle in 3-D:
     *
     * vertex  neighbor
     * ------------------
     * 0          1-2
     * 1          2-0
     * 2          0-1
     *
     * Tetrahedron in 4-D:
     *
     * vertex  neighbor
     * ------------------
     * 0         1-2-3
     * 1         2-3-0
     * 2         3-0-1
     * 3         0-1-2
     *
     * Orientation:
     *
     * The front side of a facet is the side that its normal points
     * to. Its normal is defined as the (N-1)-ary generalized cross
     * product of the ordered set of vectors (v[i] - v[0]) where v[i]
     * = vertices[vertex_indices[i]], with 'i' running from 1 to
     * N-1. It can also be defined as the Hodge dual of the wedge
     * product of those vectors.
     *
     * See http://en.wikipedia.org/wiki/Cross_product#Multilinear_algebra
     *
     * The generalized cross product should probably be implemented as
     * a function that takes an (N-1)-by-N matrix as argument M with
     * the (v[i] - v[0]) as row i+1. The function should then compute
     * the vector V where V[i] = (-1)^(N-1+i) * det(del_col(M,i))
     * where i is in [0;N-1] and del_col(M,i) is the result of
     * deleting the (i+1)'th column from M.
     */
    struct Facet
    {
      size_t vertex_indices[N];

      /**
       * The i'th neighbor is the index of the neighboring facet
       * across the specific unique (N-2)-subfacet of this facet that
       * does not involve the i'th vertex of this facet.
       */
      size_t neighbors[N];
    };

    typedef BasicVec<N> Vertex;
    vector<Vertex> vertices;
    vector<Facet> facets;
  };


  typedef FacetedSurface<3> TriangleSurface;







/*
  struct CSG
  {
*/
    /**
     * The normal of the hyperplane need not be a unit vector.
     */
/*
    void clip(Hyperplane3 plane)
    {
      // First compute the polygon that arises as the intersection
      // between the specified plane and the axis aligned bounding box
      // of the CSG.

      // Visit vertices in this order while searching for an edge that
      // intersects the plane:
      int const order[] = {
        0, // (l,l,l)
        4, // (l,l,h)
        6, // (l,h,h)
        2, // (l,h,l)
        3, // (h,h,l)
        1, // (h,l,l)
        5, // (h,l,h)
        7  // (h,h,h)
      };
      double h1 = plane.height(corner(order[0])), h2;
      bool front = 0 < h1;
      bool back  = h1 < 0;
      int i = 1;
      for(;;)
      {
        h2 = plane.height(corner(order[i]));
        if(h2 != 0) if(0 < h2 ? back : front) goto intersection;
        if(++i == 8) break;
        h1 = h2;
      }

      // No intersection. If everything is in front of the clipping
      // plane, everything must be discarded, otherwise, everything
      // must be kept.
      if(front) root.reset();
      return;

    intersection:
      Vec3 const p1 = corner(order[i-1]);
      Vec3 const p2 = corner(order[i-0]);
      double const f = 1 / (h2 - h1);
      Vec3 const p = f*h2*p1 - f*h1*p2; // Weighted mean



      // Then send this polygon through the tree.
      clip(polygon, root);
    }

    CSG(): root(new SolidNode()) {}

  private:
    struct Node {};

    typedef UniquePtr<Node> NodePtr;

    struct DivisionNode: Node
    {
      Hyperplane3 const plane;
      NodePtr front, back;
      DivisionNode(Hyperplane3 const &p): plane(p) {}
    };

    struct SolidNode: Node {};

    Box3 boundary;
    NodePtr root;

    void clip(Hyperplane3 const &plane, NodePtr &ptr)
    {
      // Recurse
      if(DivisionNode *div = dynamic_cast<DivisionNode *>(ptr.get()))
      {
        if(div->front) clip(plane, div->front);
        if(div->back)  clip(plane, div->back);
        return;
      }

      // Divide solid
      NodePtr div(new DivisionNode(plane));
      div->back = ptr;
      ptr = div;
    }
  };
*/




  struct BSP: Application
  {
    BSP(): Application("archon::render::BSP")
    {
/*
      CSG csg;
      csg.clip(Hyperplane3(Vec3(1,0,0), 1));
      csg.clip(Hyperplane3(Vec3(0,1,0), 1));
      csg.clip(Hyperplane3(Vec3(0,0,1), 1));
      csg.extract_polygons();
*/

      glEnable(GL_LIGHTING);
      glEnable(GL_DEPTH_TEST);
      glEnable(GL_NORMALIZE);
      glEnable(GL_CULL_FACE);

      glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
      glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    }

  private:
    void render_scene()
    {
    }

//    BspTree bsp_tree;
  };
}

int main(int, const char *argv[]) throw()
{
  set_terminate(&Cxx::terminate_handler);
    
  try_fix_preinstall_datadir(argv[0], "render/test/");
  BSP().run();
  return 0;
}
