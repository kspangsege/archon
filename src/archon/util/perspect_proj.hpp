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

#ifndef ARCHON_UTIL_PERSPECT_PROJ_HPP
#define ARCHON_UTIL_PERSPECT_PROJ_HPP

#include<cmath>
#include<algorithm>

#include <archon/core/functions.hpp>
#include <archon/math/functions.hpp>


namespace archon
{
  namespace util
  {
    /**
     * How to acheive the correct perspective in the rendered image
     * such that spheres actually look spheric and a cubes are
     * perceived as cubic.
     *
     * Often viewing systems are set up in a way that produces weird
     * artifacts in the depth effect. Sometimes depth seems to be
     * compressed like on on a photo taken with a high zoom factor. In
     * other cases it appears to be stretched almost like a fish-eye
     * perspective. The problem can generally be described as an
     * incorrect ratio of distanses along the depth axis to distances
     * in the view plane.
     *
     * The problem always seems to be related to a discrepancy between
     * the field of view of the configured viewing frustum and the
     * actual field of view, which is a function of the on-screen size
     * of the rendered image and of the distance between you and the
     * screen.
     *
     *
     *
     *
     *
     * Under monoscopic perspective projection it is impossible to
     * distinguish size from distance. This is because we really only
     * see the angle of the individual points. If you increase the
     * size of an object and its distance to the camera in equal
     * proportions the projected image will remain exactly the same.
     *
     * The size / distance ratio determines the size of the
     * projection.
     *
     * We can also say that the projected image of an object is
     * constant under object scaling as long as the distance to the
     * camera is varied in equal proportions.
     *
     * More precisely, take any object O and any point P in object
     * space, then let E be the position of the camera in object space
     * and let D be the direction from E to P, then scale the object
     * uniformly around P by a factor F, then translate the scaled
     * object in the direction of D such that the distance from E to P
     * is increased by the factor F, then this resulting object will
     * produces exactly the same projected image as the original as
     * long as the view transformation is kept constant.
     *
     * Note that in stereoscopic perspective projection this duality
     * no longer holds. If two objects at different distance look
     * idenical to one eye/camera they will looke different to the
     * other.
     *
     *
     * It is surprising to many that any 2-D projection or picture of
     * a 3-D scene carries a "build in" viewing position (station
     * point). Viewing it from any point other than that will cause a
     * distorted perspective. Spheres become ellipsoids and cubes are
     * no longer cubic.
     *
     * Conversely, under fixed viewing position a 2-D projection
     * cannot be scaled (2-D scaling) without destroying the
     * perspective, assuming of course it was correct in the first
     * place. This also follows from the observation that the third
     * axis is left unscaled, inevitably introducing the artifacts of
     * zooming (non-uniform spatial scaling).
     *
     *
     *
     *
     * The following should be moved to the description of
     * <tt>set_viewport_size_meters</tt>:
     *
     * Let us examine the case of you sitting before your monitor
     * looking at a monoscopic projection. If the perspecive looks
     * correct to you now, it will look wrong if you move your eyes to
     * any other position. This is a problem that can only be solved
     * with eye-tracking equipment.
     *
     * To make things work we need to make two assumptions: First we
     * assume that the midpoint between your eyes when projected
     * perpedicularly onto the viewport is always coincident with the
     * center of the viewport. Next we assume that the distance
     * between your eyes and the screen is constant. In reality the
     * perception of the perspective is resilient enough that the
     * deviations from these assumptions generally will not hurt.
     *
     * Since your eyes are assumed to be located centrally in front of
     * the viewport the field of view is determined simply by the
     * angle subtended at your eye by the viewport.
     *
     * So to be able to calculate the field of view the system needs
     * to know the distance between your eyes and the screen and the
     * physical dimensions of the vewport.
     *
     *
     *
     *
     * 
     *
     * We assume that the midpoint between your eyes when projected perpedicularly onto the viewport is coincident with the center of the viewport.
     * We further assume that the distance between the midpoint between your eyes and the center of the viewport.
     *
     *
     *
     * Determining the correct field of view.
     *
     * The field of view is a function of the on-screen size of the
     * rendering viewport and it is a function of your distance from
     * the screen. There are several different definitions:
     *
     * The maximum field of view is defined as the angle subtended at
     * your eye by the diagonal of the smallest circle that encloses
     * the viewport.
     *
     * The minimum field of view is defined as the angle subtended at
     * your eye by the diagonal of the largest circle that fits inside
     * the viewport.
     *
     * For rectangular viewports the following definitions are
     * usefull:
     *
     * The diagonal field of view is defined as the angle subtended at
     * your eye by one of the diagonals of the viewport. This
     * corresponds to the maximum field of view.
     *
     * The horizontal field of view is defined as the angle at your
     * eye between the mid-points of the left and right sides of the
     * viewport.
     * 
     * The vertical field of view is defined as the angle at your eye
     * between the mid-points of the top and bottom sides of the
     * viewport.
     *
     * The minimum field of view is equal to either the horizontal or
     * the vertical field of view whichever is smallest.
     *
     * <PRE>
     *
     *   maximum field of view = 2 ang(d, len(w, h)/2)
     *   minimum field of view = 2 ang(d, min(w, h)/2)
     *
     * where
     *
     *   ang(x, y) is the angle described by vector (x, y)
     *   len(x, y) is the length of vector (x, y)
     *   min(x, y) is smaller of x and y
     *   d is the distance from your eye to the screen
     *   w is the width of the viewport in same same coordinates as d
     *   h is the height of the viewport in same same coordinates as d
     *
     * </PRE>
     *
     * Head tracking.
     *
     * Zoom factor
     *
     *
     * A view system that does not distort the perspective must
     * operate with a field of view that is coincident with the
     * physical field of view determined by the physical configuration
     * of your eyes and the viewport..
     *
     * Physical: viewpoint - the position of the eye in physical coordinates.
     *           viewport  - position and dimensions of the on-screen rendering area in physical coordinates.
     *           viewbox
     *
     * Virtual:  camerapoint - the position of the focal point of the virtual camera in object coordinates.
     *           cameraport  - position and dimensions of the picture on the picture plane of the virtual camera in object coordinates.
     *           camerabox
     *
     * The stationpoint and the pictureplane collectively make up the
     * virtual camera and the position of the camera is understood as
     * the position of the station point.
     *
     * In the projection the stationpoint is mapped to the eye and the
     * pictureplane to the viewport.
     *
     * To preserve the perspective the 
     */

    /**
     *
     * Auto distance perspective
     *
     * The idea here is that an object of certain size
     * <tt>interest_ball_radius</tt> in object coordinates will be scaled
     * such that it just fits within the field of view but without
     * distorting the perspective.
     *
     * If the field of view is changed (eg. by resizing the window)
     * the size of the projection on the screen will change.Assuming
     * that we reduce it, the visual effect can then either be
     * interpreted as a uniform down-scaling of the object around its
     * center while keeping the distance to the camera constant, or it
     * can be understood as a translation of the camera (or the
     * object) to greater distance. Due to the nature of perspective
     * projection one cannot tell the difference.
     *
     * I belive it is best, though, to think of the size of objects as
     * constant. This makes the physical interpretation easier. In
     * this case we should think of it as the camera being
     * automatically moved along a straight rail such as to fit the
     * focus sphere ball of interest...
     */

    /**
     * This class describes a simple monoscopic perspective projection
     * comprised of a camera, its relation to the volume of interest
     * and its relation to the condition under which the result is
     * presented.
     *
     * The projection is modeled as a set of independant parameters,
     * each one carefully chosen to ease its interpretation in terms
     * of well known physical quantities.
     *
     * To understand the camera model one must think of a rectangular
     * box. At the center of the front face there is a small hole
     * (center of projection) where light rays enter the box. These
     * rays project an image onto the opposite face of the box (view
     * plane). The width and height of the box relative to the depth
     * of the box defines the field of view of the camera.
     *
     * The viewport is the flat rectangular area on the screen in
     * which the projected image is displayed.
     *
     * The view distance is the distance from your eyes to the
     * viewport along an axis that is perpendicular to the viewport.
     *
     * The view plane is a plane perpenndicular to the optical axis
     * onto which the...
     *
     * 
     */
    struct PerspectiveProjection
    {
      /**
       * Horizontal distance between pixels on the viewport
       * (eg. between pixel centers) measured in meters.
       */
      double horiz_dot_pitch;

      /**
       * Vertical distance between pixels on the viewport (eg. between
       * pixel centers) measured in meters.
       */
      double vert_dot_pitch;

      /**
       * Physical distance along the optical axis from your eyes to
       * the viewport measured in meters.
       *
       * The optical axis is the line that runs through the midpoint
       * between your eyes and is perpendicular to the viewport
       * plane. the point of intersection with the viewport plane is
       * called the pricipal point, and in the case of this simplified
       * projection the pricipal point is always conincident with the
       * center of the viewport.
       */
      double view_dist;

      /**
       * Ratio of image width to image height on view plane.
       */
      double aspect_ratio;

      /**
       * The neutral field of view of the camera. That is, the field
       * of view when the zoom factor is set to 1. The actual field of
       * view of the camera is affected by the neutral field of view
       * of the camera as well as the zoom factor. See
       * <tt>zoom_factor</tt> for further details on the actual field
       * of view of the camera.
       *
       * To get a predictable perception of perspective while viewing
       * the projected image on your screen, the neutral field of view
       * of the camera should at all times be kept equal to the field
       * of view inherent to the physical viewing condition. Thus,
       * both the distance from your eyes to the screen and the size
       * of the viewport affects the correct value for the neutral
       * field of view of the camera.
       *
       * The most intuitive way of specifying the neutral field of
       * view of the camera is to set <tt>view_dist</tt> to the the
       * actual viewing distance and then call
       * <tt>set_viewport_size_meters</tt> to specify the actual
       * dimensions of the viewport. That said, this class offers
       * numerous other ways of specifying the same thing.
       *
       * Keeping the neutral field of view of the camera equal to the
       * field of view of the physical viewing condition guarantees a
       * correct (undistorted) perception of perspective as long as
       * the zoom factor is 1 (just as if the viewport had been a
       * window into a space behind the screen.)
       *
       * The neutral field of view of the camera is defined as the
       * geometric mean between the width and the height of the image
       * as it would appear without zoom on a view plane at distance 1
       * from the center of projection.
       */
      double neutral_fov;

      /**
       * Ratio of far clipping distance to near clipping distance
       */
      double far_to_near_clip_ratio;

      /**
       * Together with the neutral field of view the zoom factor
       * determines the actual field of view of the camera.
       *
       * With a definition similar to that of the neutral field of
       * view we get the following simple relation:
       *
       * <pre>
       *
       *   actual_fov * zoom_factor = neutral_fov
       *
       * </pre>
       *
       * Thus setting <tt>zoom_factor</tt> to 1 means that the camera
       * will produce an image with an undistorted perspective, just
       * as if the viewport had been a window into a space behind the
       * screen, but only as long as the neutral field of view of the
       * camera is correctly specified to match the field of view
       * experienced by you when you look "through" the viewport on
       * your screen and the aspect ratio of the camera is correctly
       * specified to match the aspect ratio of the viewport.
       *
       * Setting the <tt>zoom_factor</tt> to 2 will produce an image
       * where all features will appear twice as wide and twice as
       * heigh as they would with <tt>zoom_factor = 1</tt>. At the
       * same time there will be no apparant change to the depth of
       * these objects, thus they will appear squeezed in that
       * direction. This is the usual and well known (from
       * photography) distortion of perspective caused by zooming.
       *
       * There is an interesting duality between zooming and 2-D
       * scaling. If you capture an image with zoom factor set to 2,
       * then scale it to half size (half width and half height) the
       * the result will be indistinguishable from the image you get
       * with zoom factor set to 1 and then cropping it uniformly to
       * half size. In a sense, zooming and 2-D scaling is the same
       * thing. This is also a proof of the (maybe surprising) fact
       * that 2-D scaling distorts the perspective of the scaled
       * image.
       */
      double zoom_factor;

      /**
       * Distance from center of projection to center of interest.
       */
      double camera_dist;



      /**
       * Construct a perspective projection.
       *
       * \param horiz_dot_pitch See the description of member of
       * same name.
       *
       * \param vert_dot_pitch See the description of member of same
       * name.
       *
       * \param view_dist See the description of member of same
       * name.
       *
       * \param aspect_ratio See the description of member of same
       * name.
       *
       * \param neutral_fov See the description of member of
       * same name.
       *
       * \param far_to_near_clip_ratio See the description of member of
       * same name.
       *
       * \param zoom_factor See the description of member of same name.
       *
       * \param camera_dist See the description of member of same
       * name.
       */
      PerspectiveProjection(double horiz_dot_pitch        = 0.0254/96,
                            double vert_dot_pitch         = 0.0254/96,
                            double view_dist              = 0.5,
                            double aspect_ratio           = 1,
                            double neutral_fov            = 0.828427124747,
                            double far_to_near_clip_ratio = 100,
                            double zoom_factor            = 1,
                            double camera_dist            = 10):
        horiz_dot_pitch(horiz_dot_pitch), vert_dot_pitch(vert_dot_pitch),
        view_dist(view_dist), aspect_ratio(aspect_ratio), neutral_fov(neutral_fov),
        far_to_near_clip_ratio(far_to_near_clip_ratio), zoom_factor(zoom_factor),
        camera_dist(camera_dist) {}



      /**
       * Get the horizontal resolution of the screen measured in dots
       * per centimeter.
       *
       * \return The horizontal resolution of the screen in dpcm.
       */
      double get_horiz_resol_dpcm()
      {
        return 0.01 / horiz_dot_pitch;
      }

      /**
       * Get the vertical resolution of the screen measured in dots
       * per centimeter.
       *
       * \return The vertical resolution of the screen in dpcm.
       */
      double get_vert_resol_dpcm()
      {
        return 0.01 / vert_dot_pitch;
      }

      /**
       * Set the resolution of your screen as dots per centimeter.
       *
       * \param horiz The horizontal resolution of your screen in
       * dpcm.
       *
       * \param vert The vertical resolution of your screen in dpcm.
       */
      void set_resol_dpcm(double horiz, double vert)
      {
        horiz_dot_pitch = 0.01 / horiz;
        vert_dot_pitch  = 0.01 / vert;
      }



      /**
       * Get the horizontal resolution of the screen measured in dots
       * per inch.
       *
       * \return The horizontal resolution of the screen in dpi.
       */
      double get_horiz_resol_dpi()
      {
        return 0.0254 / horiz_dot_pitch;
      }

      /**
       * Get the vertical resolution of the screen measured in dots
       * per inch.
       *
       * \return The vertical resolution of the screen in dpi.
       */
      double get_vert_resol_dpi()
      {
        return 0.0254 / vert_dot_pitch;
      }

      /**
       * Set the resolution of your screen as dots per inch.
       *
       * \param h The horizontal resolution of your screen in dpi.
       *
       * \param v The vertical resolution of your screen in dpi.
       */
      void set_resol_dpi(double h, double v)
      {
        horiz_dot_pitch = 0.0254 / h;
        vert_dot_pitch  = 0.0254 / v;
      }



      double get_viewport_width_meters()
      {
        return  view_dist * zoom_factor * get_actual_horiz_fov();
      }

      double get_viewport_height_meters()
      {
        return  view_dist * zoom_factor * get_actual_vert_fov();
      }

      void set_viewport_size_meters(double width, double height)
      {
        aspect_ratio = width / height;
        neutral_fov  = std::sqrt(width * height) / view_dist;
      }

      int get_viewport_width_pixels()
      {
        return core::archon_round(get_viewport_width_meters() / horiz_dot_pitch);
      }

      int get_viewport_height_pixels()
      {
        return core::archon_round(get_viewport_height_meters() / vert_dot_pitch);
      }

      void set_viewport_size_pixels(int width, int height)
      {
        set_viewport_size_meters(width * horiz_dot_pitch, height * vert_dot_pitch);
      }



      /**
       * Get the distance from the center of projection to the near
       * clipping plane.
       *
       * Use this quantity in the specification of the view frustum of
       * your rendering API (eg. glFrustum.)
       *
       * The near and far clipping distances are both function of the
       * camera distance and the requested far to near distance
       * ratio. They are determined such that the camera distance is
       * equal to the geometric mean between the near and the far
       * clipping distances and such that the requested ratio between
       * the near and the far clipping distances is maintained.
       *
       * This particular way of determining the clipping distances is
       * simple and seems to fit most needs, but of course there are
       * many other ways of doing it.
       *
       * \return The near clipping distance.
       */
      double get_near_clip_dist()
      {
        return camera_dist / std::sqrt(far_to_near_clip_ratio);
      }

      /**
       * Get the distance from the center of projection to the far
       * clipping plane.
       *
       * Use this quantity in the specification of the view frustum of
       * your rendering API (eg. glFrustum.)
       *
       * See <tt>get_near_clip_dist</tt> for further details.
       *
       * \return The far clipping distance.
       */
      double get_far_clip_dist()
      {
        return get_near_clip_dist() * far_to_near_clip_ratio;
      }

      /**
       * Get the width of the image as it is when projected onto a
       * view plane at the near clipping distance. The width is
       * determined by the actual field of view as well as the aspect
       * ratio of the camera.
       *
       * Use this quantity in the specification of the view frustum of
       * your rendering API (eg. glFrustum.)
       *
       * \sa get_near_clip_dist
       *
       * \return The image width on the near clipping plane.
       */
      double get_near_clip_width()
      {
        return get_near_clip_dist() * get_actual_horiz_fov();
      }

      /**
       * Get the height of the image as it is when projected onto a
       * view plane at the near clipping distance. The height is
       * determined by the actual field of view as well as the aspect
       * ratio of the camera.
       *
       * Use this quantity in the specification of the view frustum of
       * your rendering API (eg. glFrustum.)
       *
       * \sa get_near_clip_dist
       *
       * \return The image height on the near clipping plane.
       */
      double get_near_clip_height()
      {
        return get_near_clip_dist() * get_actual_vert_fov();
      }



      /**
       * Move the camera to a distance where the sphere of interest
       * fits perfectly within the field of view, or equivalently,
       * where the projection of the sphere of interest fits perfectly
       * inside the viewport. The precise meaning of a 'perfect fit'
       * is determined by the field factor (see below.)
       *
       * This operation basically allows you to control how big your
       * scene will look on your screen while at the same time giving
       * you full control over the perspective in the projected image
       * as experienced by you. In particular if the zoom factor is
       * set to 1 and the neutral field of view of the camera is
       * correctly specified to match the field of view experienced by
       * you when you look "through" the viewport on your screen and
       * the aspect ratio of the camera is correctly specified to
       * match the aspect ratio of the viewport, then the perspective
       * in the image you see will be perfect. That is, sheres will
       * appear spheric (not elliptic) and cubes will look cubic (not
       * rectangular).
       *
       * By default (f=1) the sphere of interest will be fitted to the
       * mean field of view meaning that the diameter of the
       * projection will be equal to the geometric mean between the
       * with and the height of the vewport.
       *
       * One can obtain other fits by specifying other values for the
       * field factor f. For example setting f =
       * <tt>get_horiz_field_factor()</tt> will result in a diameter
       * equal to the width of the viewport while f =
       * <tt>get_diag_field_factor()</tt> will result in a diameter
       * equal to the diameter of the viewport.
       *
       * \param interest The length of the diagonal of the sphere of
       * interest.
       *
       * \param f The field factor.
       *
       * \sa auto_zoom
       */
      void auto_dist(double interest, double f = 1)
      {
        camera_dist = interest * std::sqrt(0.25 + Math::square(/*zoom_factor*/1 / neutral_fov / f));
      }

      /**
       * Adjust the zoom factor such that the sphere of interest fits
       * perfectly within the field of view, or equivalently, such
       * that the projection of the sphere of interest fits perfectly
       * inside the viewport. The precise meaning of a perfect fit is
       * determined by the field factor (see below.)
       *
       * This operation basically allows you to control how big your
       * scene will look on your screen while at the same time keeping
       * the camera distance fixed.
       *
       * Unlike in the case of <tt>auto_dist</tt> the resulting
       * perspective in the image you see will be distorted by the
       * zooming and sometimes even heavily distorted. Spheres will
       * appear elliptical and cubes will look rectangular. On the
       * other hand, if the position of your camera is controlled in
       * ways that cannot be interfeared with, this is the only way to
       * acheive the fitting effect.
       *
       * By default (f=1) the sphere of interest will be fitted to the
       * mean field of view meaning that the diameter of the
       * projection will be equal to the geometric mean between the
       * with and the height of the vewport.
       *
       * One can obtain other fits by specifying other values for the
       * field factor f. For example setting f =
       * <tt>get_horiz_field_factor()</tt> will result in a diameter
       * equal to the width of the viewport while f =
       * <tt>get_diag_field_factor()</tt> will result in a diameter
       * equal to the diameter of the viewport.
       *
       * \param interest The length of the diagonal of the sphere of
       * interest.
       *
       * \param f The field factor.
       *
       * \sa auto_dist
       */
      void auto_zoom(double interest, double f = 1)
      {
        zoom_factor = neutral_fov * f * std::sqrt(Math::square(camera_dist/interest) - 0.25);
      }



      /**
       * Get the width of the image when projected onto a view plane
       * at distance 1 from the center of projection.
       *
       * \return The horizontal field of view of the camera.
       */
      double get_actual_horiz_fov()
      {
        return neutral_fov / zoom_factor * get_horiz_field_factor();
      }

      /**
       * Get the height of the image when projected onto a view plane
       * at distance 1 from the center of projection.
       *
       * \return The vertical field of view of the camera.
       */
      double get_actual_vert_fov()
      {
        return neutral_fov / zoom_factor * get_vert_field_factor();
      }



      /**
       * Get the solid angle corresponding with the neutral field of
       * view of the camera.
       *
       * The solid angle of view is defined as the area of the
       * projection of the image onto the unit sphere.
       *
       * \return The neutral solid angle of view of the camera
       * measured in steradians.
       */
      double get_neutral_solid_angle_of_view()
      {
        return 4 * std::atan(1 / std::sqrt(4/Math::square(neutral_fov) +
                                           aspect_ratio + 1/aspect_ratio));
      }

      /**
       * Modify the neutral field of view of the camera in terms of
       * solid angle of view. The aspect ratio is maintained.
       *
       * The solid angle of view is defined as the area of the
       * projection of the image onto the unit sphere.
       *
       * You should generally specify a value here that is equal to
       * the solid angle subtended at your eyes by the viewport on
       * your screen. This guarantees an undistorted perception of
       * perspective when the zoom factor of the camera is 1.
       *
       * \param v The neutral solid angle of view measured in
       * steradians.
       */
      void set_neutral_solid_angle_of_view(double v)
      {
        neutral_fov = std::sqrt(4/(Math::square(1/std::tan(v/4)) -
                                   aspect_ratio - 1/aspect_ratio));
      }


      /**
       * Get the solid angle corresponding with the actual field of
       * view of the camera. That is, the angle of view with the zoom
       * factor applied.
       *
       * The solid angle of view is defined as the area of the
       * projection of the image onto the unit sphere.
       *
       * \return The actual solid angle of view of the camera measured
       * in steradians.
       */
      double get_actual_solid_angle_of_view()
      {
        return 4 * std::atan(1 / std::sqrt(4/Math::square(neutral_fov/zoom_factor) +
                                           aspect_ratio + 1/aspect_ratio));
      }

      /**
       * Modify the actual field of view of the camera in terms of
       * solid angle of view. This modifies only the zoom factor. The
       * neutral field of view is left unchanged. The aspect ratio is
       * maintained.
       *
       * The solid angle of view is defined as the area of the
       * projection of the image onto the unit sphere.
       *
       * \param v The actual solid angle of view measured in
       * steradians.
       */
      void set_actual_solid_angle_of_view(double v)
      {
        neutral_fov = zoom_factor * std::sqrt(4/(Math::square(1/std::tan(v/4)) -
                                                 aspect_ratio - 1/aspect_ratio));
      }



      /**
       * Get the angle of view corresponding with the neutral field of
       * view of the camera.
       *
       * By default (<tt>f=1</tt>) the returned angle is the mean
       * angle of view. See <tt>get_mean_field_factor</tt> for the
       * definition of mean angle of view.
       *
       * One can obtain other flavors of the angle of view by
       * specifying other values for the field factor <tt>f</tt>. For
       * example setting <tt>f = get_horiz_field_factor()</tt> will
       * produce the horizontal angle of view while <tt>f =
       * get_diag_field_factor()</tt> will produce the diagonal angle
       * of view.
       *
       * \param f The field factor.
       *
       * \return The neutral angle of view measured in radians.
       */
      double get_neutral_angle_of_view(double f = 1)
      {
        return 2 * std::atan(f*neutral_fov/2);
      }

      /**
       * Set the neutral field of view of the camera to correspond
       * with the specified angle of view. The aspect ratio is
       * maintained.
       *
       * By default (<tt>f=1</tt>) the specified angle is the mean
       * angle of view. See <tt>get_mean_field_factor</tt> for the
       * definition of mean angle of view.
       *
       * One can specify other flavors of the angle of view by
       * specifying other values for the field factor <tt>f</tt>. For
       * example one can specyfy the horizontal angle of view by
       * setting <tt>f = get_horiz_field_factor()</tt> or the diagonal
       * angle of view with <tt>f = get_diag_field_factor()</tt>.
       *
       * You should generally specify a value here that is equal to
       * the angle subtended at your eyes by the viewport on your
       * screen. This guarantees an undistorted perception of
       * perspective when the zoom factor of the camera is 1.
       *
       * \param v The neutral angle of view measured in radians.
       *
       * \param f The field factor.
       */
      void set_neutral_angle_of_view(double v, double f = 1)
      {
        neutral_fov = 2 * std::sqrt(std::tan(v/2)) / f;
      }


      /**
       * Get the angle of view corresponding with the actual field of
       * view of the camera. That is, the angle of view with the zoom
       * factor applied.
       *
       * By default (<tt>f=1</tt>) the returned angle is the mean
       * angle of view. See <tt>get_mean_field_factor</tt> for the
       * definition of mean angle of view.
       *
       * One can obtain other flavors of the angle of view by
       * specifying other values for the field factor <tt>f</tt>. For
       * example setting <tt>f = get_horiz_field_factor()</tt> will
       * produce the horizontal angle of view while <tt>f =
       * get_diag_field_factor()</tt> will produce the diagonal angle
       * of view.
       *
       * \param f The field factor.
       *
       * \return The actual angle of view measured in radians.
       */
      double get_actual_angle_of_view(double f = 1)
      {
        return 2 * std::atan(f*neutral_fov/zoom_factor/2);
      }

      /**
       * Set the actual field of view of the camera to correspond with
       * the specified angle of view. This modifies only the zoom
       * factor. The neutral field of view is left unchanged. The
       * aspect ratio is maintained.
       *
       * By default (<tt>f=1</tt>) the specified angle is the mean
       * angle of view. See <tt>get_mean_field_factor</tt> for the
       * definition of mean angle of view.
       *
       * One can specify other flavors of the angle of view by
       * specifying other values for the field factor <tt>f</tt>. For
       * example one can specyfy the horizontal angle of view by
       * setting <tt>f = get_horiz_field_factor()</tt> or the diagonal
       * angle of view with <tt>f = get_diag_field_factor()</tt>.
       *
       * \param v The actual angle of view measured in radians.
       *
       * \param f The field factor.
       */
      void set_actual_angle_of_view(double v, double f = 1)
      {
        neutral_fov = 2 * zoom_factor * std::sqrt(std::tan(v/2)) / f;
      }



      /**
       * Use with <tt>get_neutral_angle_of_view</tt> /
       * <tt>set_neutral_angle_of_view</tt> /
       * <tt>get_actual_angle_of_view</tt> /
       * <tt>set_actual_angle_of_view</tt> to get/set the mean angle
       * of view.
       *
       * The mean angle of view is defined as the angle subtended at
       * the center of projection by a line segment lying centered on
       * the view plane and whose length is equal to the geometric
       * mean between the with and the height of the image when
       * projected onto that view plane.
       *
       * \return The mean field factor.
       */
      double get_mean_field_factor()
      {
        return 1;
      }

      /**
       * Use with <tt>get_neutral_angle_of_view</tt> /
       * <tt>set_neutral_angle_of_view</tt> /
       * <tt>get_actual_angle_of_view</tt> /
       * <tt>set_actual_angle_of_view</tt> to get/set the horizontal
       * angle of view.
       *
       * The horizontal angle of view is defined as the angle
       * subtended at the center of projection by a line segment
       * running from the midpoint of the left edge to the midpoint of
       * the right edge of the image on the view plane.
       *
       * \return The horizontal field factor.
       */
      double get_horiz_field_factor()
      {
        return std::sqrt(aspect_ratio);
      }

      /**
       * Use with <tt>get_neutral_angle_of_view</tt> /
       * <tt>set_neutral_angle_of_view</tt> /
       * <tt>get_actual_angle_of_view</tt> /
       * <tt>set_actual_angle_of_view</tt> to get/set the vertical
       * angle of view.
       *
       * The vertical angle of view is defined as the angle subtended
       * at the center of projection by a line segment running from
       * the midpoint of the bootom edge to the midpoint of the top
       * edge of the image on the view plane.
       *
       * \return The vertical field factor.
       */
      double get_vert_field_factor()
      {
        return 1/std::sqrt(aspect_ratio);
      }

      /**
       * Use with <tt>get_neutral_angle_of_view</tt> /
       * <tt>set_neutral_angle_of_view</tt> /
       * <tt>get_actual_angle_of_view</tt> /
       * <tt>set_actual_angle_of_view</tt> to get/set the diagonal
       * angle of view.
       *
       * The diagonal angle of view is defined as the angle subtended
       * at the center of projection by the diagonal of the image on
       * the view plane.
       *
       * \return The diagonal field factor.
       */
      double get_diag_field_factor()
      {
        return std::sqrt(aspect_ratio + 1/aspect_ratio);
      }

      /**
       * Use with <tt>get_neutral_angle_of_view</tt> /
       * <tt>set_neutral_angle_of_view</tt> /
       * <tt>get_actual_angle_of_view</tt> /
       * <tt>set_actual_angle_of_view</tt> to get/set the minimum
       * angle of view.
       *
       * The minimum angle of view is defined as being either the
       * horizontal or the vertical angle of view whichever is
       * smallest.
       *
       * \return The minimum field factor.
       */
      double get_min_field_factor()
      {
        return std::sqrt(std::min(aspect_ratio, 1/aspect_ratio));
      }

      /**
       * Use with <tt>get_neutral_angle_of_view</tt> /
       * <tt>set_neutral_angle_of_view</tt> /
       * <tt>get_actual_angle_of_view</tt> /
       * <tt>set_actual_angle_of_view</tt> to get/set the maximum
       * angle of view.
       *
       * The maximum angle of view is defined as being either the
       * horizontal or the vertical angle of view whichever is
       * largest.
       *
       * \return The maximum field factor.
       */
      double get_max_field_factor()
      {
        return std::sqrt(std::max(aspect_ratio, 1/aspect_ratio));
      }
    };
  }
}

#endif // ARCHON_UTIL_PERSPECT_PROJ_HPP
