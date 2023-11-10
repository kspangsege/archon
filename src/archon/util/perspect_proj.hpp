// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2023 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_UTIL_X_PERSPECT_PROJ_HPP
#define ARCHON_X_UTIL_X_PERSPECT_PROJ_HPP

/// \file


#include <cmath>
#include <algorithm>

#include <archon/core/float.hpp>
#include <archon/core/math.hpp>


namespace archon::util {


/// \brief Simple monoscopic perspective projection.
///
/// This class describes a simple monoscopic perspective projection. The projection is
/// specified as a set of independent parameters, and functions are provided to compute
/// various useful properties from those parameters.
///
/// The parameterization of the projection is done with the intent of reflecting well known
/// physical quantities.
///
/// To understand the camera model one can think of a rectangular box. At the center of the
/// front face of this box there is a small hole (oculus) where light rays enter the
/// box. These rays project an image onto the image plane, which is the opposite face of the
/// box.
///
/// The relation between the width and height of the captured part of the projected image on
/// the image plane defines the aspect ratio of the camera.
///
/// The relation between the size of the captured image and the distance between the image
/// place and the oculus defines the field of view of the camera.
///
/// The image captured by the camera is assumed to be displayed on a screen. The *view
/// distance* is the distance from your eyes to that screen along an axis that is
/// perpendicular to the screen.
///
/// The part of the screen that displays the image is called the *viewport*. Typically, this
/// corresponds to the area inside a window, or, in fullscreen mode, it corresponds to the
/// entire screen.
///
class PerspectiveProjection {
public:
    /// \brief Horizontal distance between pixels.
    ///
    /// This is the horizontal distance between pixels on the screen (eg. between pixel
    /// centers) measured in meters.
    ///
    double horz_dot_pitch = 0.0254 / 96;

    /// \brief Vertical distance between pixels.
    ///
    /// Vertical distance between pixels on the screen (eg. between pixel centers) measured
    /// in meters.
    ///
    double vert_dot_pitch = 0.0254 / 96;

    /// \brief Distance from your eyes to the screen.
    ///
    /// Physical distance in meters from your eyes to the screen along an axis that is
    /// perpendicular to the screen. The default value is 60 centimeters (cm).
    ///
    double view_dist = 0.6;

    /// \brief Aspect ratio of captured image.
    ///
    /// This is the ratio of width to height (in meters) of the viewport on the screen. At
    /// the same time, it is the ratio of width to height of the captured part of the image
    /// on the image plane of the camera.
    ///
    double aspect_ratio = 1;

    /// \brief Mean neutral field of view of camera.
    ///
    /// This is the mean neutral field of view of the camera. That is, the mean effective
    /// field of view of the camera when the zoom factor is set to 1 (see \ref
    /// zoom_factor). The effective field of view is determined both by the neutral field of
    /// view and the zoom factor.
    ///
    /// The mean neutral field of view of the camera is defined as the geometric mean
    /// between the width and height of the captured part of the image on the image plane of
    /// the camera divided by the distance between the image plane and the oculus.
    ///
    /// To get the intended perception of perspective while viewing the projected image on
    /// your screen, the neutral field of view of the camera must be set to match the actual
    /// field of view of the physical viewing condition. The actual field of view of the
    /// physical viewing condition is the geometric mean between the width and height (in
    /// meters) of the viewport on the screen divided by the viewing distance (\ref
    /// view_dist). Thus, both the distance from your eyes to the screen and the size of the
    /// viewport on the screen matter when determining the correct value for the neutral
    /// field of view.
    ///
    /// The easiest way of determining the right value for the neutral field of view is
    /// probably to first set the viewing distance (\ref view_dist) to the actual viewing
    /// distance, and then call \ref set_viewport_size_meters() passing the actual size of
    /// the viewport.
    ///
    /// The default value (0.5) corresponds to a viewport with an area of 900 square
    /// centimeters (30cm by 30cm) viewed at a distance of 60 centimeters (cm).
    ///
    double mean_neutral_fov = 0.5;

    /// \brief Ratio of far to the near clipping distance.
    ///
    /// This is the ratio of far clipping distance to the near clipping distance. This is
    /// used by \ref get_near_clip_dist() and \ref get_far_clip_dist() to determine good
    /// values for the absolute distances to the near and far clipping planes
    /// respectively. These are important to OpenGL.
    ///
    double far_to_near_clip_ratio = 100;

    /// \brief Zoom factor of camera.
    ///
    /// Together with the mean neutral field of view (\ref mean_neutral_fov), the zoom
    /// factor determines the effective field of view of the camera.
    ///
    /// With a definition similar to that of the neutral field of view we get the following
    /// simple relation:
    ///
    ///     zoom_factor * mean_effective_fov = mean_neutral_fov
    ///
    /// Thus, setting \p zoom_factor to 1 means that the camera will produce an image with
    /// an undistorted perspective, just as if the viewport had been a window into a space
    /// behind the screen, but only as long as the neutral field of view of the camera (\ref
    /// mean_neutral_fov) is correctly specified to match the actual field of view of the
    /// viewing condition, and the aspect ratio (\ref aspect_ratio) is correctly specified
    /// to match the aspect ratio of the viewport.
    ///
    /// Setting \p zoom_factor to 2 will produce an image where all features will appear
    /// twice as wide and twice as high as they would with `zoom_factor = 1`. At the same
    /// time there will be no apparent change to the depth of these objects, thus they will
    /// appear squeezed in the depth-wise direction. This is the usual and well known (from
    /// photography) distortion of perspective caused by zooming.
    ///
    /// There is an interesting duality between zooming and 2-D scaling. If you capture an
    /// image with zoom factor set to 2, then scale it to half size (half width and half
    /// height) the result will be indistinguishable from the image you get with zoom factor
    /// set to 1 and then cropping it uniformly to half size. In a sense, zooming and 2-D
    /// scaling is the same thing. This can be taken as proof of the (maybe surprising) fact
    /// that 2-D scaling distorts the perspective of the scaled image.
    ///
    double zoom_factor = 1;

    /// \brief Distance from camera to center of interest.
    ///
    /// Distance in the virtual space of the camera from the oculus of the camera to the
    /// center of interest.
    ///
    double camera_dist = 10;


    /// \brief Default constructor.
    ///
    /// This constructor default constructs a perspective projection.
    ///
    PerspectiveProjection() noexcept = default;

    /// \{
    ///
    /// \brief Horizontal and vertical resolution of screen in dots per centimeter.
    ///
    /// These functions respectively return the horizontal and vertical resolutions of the
    /// screen measured in dots per centimeter (dpcm). This is a simple conversion of \ref
    /// horz_dot_pitch and \ref vert_dot_pitch from meters per dot to dots per centimeter.
    ///
    double get_horz_resol_dpcm() noexcept;
    double get_vert_resol_dpcm() noexcept;
    /// \}

    /// \brief Set horizontal and vertical resolution of screen in dots per centimeter.
    ///
    /// This function sets the horizontal and vertical resolution of the screen (\ref
    /// horz_dot_pitch, \ref vert_dot_pitch). This is a simple conversion from dots per
    /// centimeter to meters per dot.
    ///
    /// \param horz, vert The horizontal and vertical resolution of the screen in dots per
    /// centimeter (dpcm).
    ///
    void set_resol_dpcm(double horz, double vert) noexcept;

    /// \{
    ///
    /// \brief Horizontal and vertical resolution of screen in dots per inch.
    ///
    /// These functions respectively return the horizontal and vertical resolutions of the
    /// screen measured in dots per inch (dpi). This is a simple conversion of \ref
    /// horz_dot_pitch and \ref vert_dot_pitch from meters per dot to dots per inch.
    ///
    double get_horz_resol_dpi() noexcept;
    double get_vert_resol_dpi() noexcept;
    /// \}

    /// \brief Set horizontal and vertical resolution of screen in dots per inch.
    ///
    /// This function sets the horizontal and vertical resolution of the screen (\ref
    /// horz_dot_pitch, \ref vert_dot_pitch). This is a simple conversion from dots per inch
    /// to meters per dot.
    ///
    /// \param horz, vert The horizontal and vertical resolution of the screen in dots per
    /// inch (dpi).
    ///
    void set_resol_dpi(double horz, double vert) noexcept;

    /// \{
    ///
    /// \brief Size of viewport in meters.
    ///
    /// These functions respectively return the width and height of the viewport in
    /// meters. The viewport size is calculated as the viewing distance (\ref view_dist)
    /// times the neutral field of view (\ref get_neutral_fov()) using the horizontal and
    /// vertical field factors (\ref get_horz_field_factor(), \ref get_vert_field_factor).
    ///
    double get_viewport_width_meters() noexcept;
    double get_viewport_height_meters() noexcept;
    /// \}

    /// \brief Set width and height of viewport in meters.
    ///
    /// This function uses the specified width and height of the viewport on the screen to
    /// compute a new aspect ratio (\ref aspect_ratio). It then uses the new aspect ratio
    /// and the configured viewing distance (\ref view_dist) to compute a new neutral field
    /// of view (\ref mean_neutral_fov).
    ///
    /// \param width, height The width and height of the viewport in meters.
    ///
    void set_viewport_size_meters(double width, double height) noexcept;

    /// \{
    ///
    /// \brief Size of viewport in pixels.
    ///
    /// These functions respectively return the width and height of the viewport in
    /// pixels. The viewport size in pixels is calculated as the viewport size in meters
    /// (\ref get_viewport_width_meters, \ref get_viewport_height_meters) times the screen
    /// resolution (\ref horz_dot_pitch, \ref vert_dot_pitch).
    ///
    int get_viewport_width_pixels() noexcept;
    int get_viewport_height_pixels() noexcept;
    /// \}

    /// \brief Set width and height of viewport in pixels.
    ///
    /// This function uses the currently configured screen resolution (\ref horz_dot_pitch,
    /// \ref vert_dot_pitch) to convert the specified size into a size in meters, and than
    /// calls \ref set_viewport_size_meters() passing the calculated size in meters.
    ///
    /// \param width, height The width and height of the viewport in pixels.
    ///
    void set_viewport_size_pixels(int width, int height) noexcept;

    /// \{
    ///
    /// \brief Distance to near and far clipping planes (OpenGL).
    ///
    /// These functions return the distances in the virtual space of the camera from the
    /// oculus of the camera to the near and far clipping planes respectively. These
    /// distances are important in the context of OpenGL rendering (see `glFrustum()`).
    ///
    /// The near and far clipping distances are determined such that the ratio between them
    /// is as specified by \ref far_to_near_clip_ratio, and such that the geometric mean
    /// between the two distances coincides with the configured camera distance (\ref
    /// camera_dist).
    ///
    double get_near_clip_dist() noexcept;
    double get_far_clip_dist() noexcept;
    /// \}

    /// \{
    ///
    /// \brief Size of captured image on near clipping plane.
    ///
    /// These functions respectively compute the width and height in the virtual space of
    /// the camera of the captured image as it would appear if projected onto the near
    /// clipping plane. These sizes are important in the context of OpenGL rendering (see
    /// `glFrustum()`).
    ///
    /// The size of the image as projected on the near clipping plane is calculated as the
    /// distance to the near clipping plane (\ref get_near_clip_dist()) times the effective
    /// field of view (\ref get_effective_fov()) for the horizontal and vertical field
    /// factors (\ref get_horz_field_factor(), \ref get_vert_field_factor()).
    ///
    double get_near_clip_width() noexcept;
    double get_near_clip_height() noexcept;
    /// \}

    /// \brief Fit sphere of interest to viewport by adjusting camera distance.
    ///
    /// This function moves the camera to a distance (\ref camera_dist) where the projection
    /// of the specified sphere of interest (\p interest_size) fits perfectly inside the
    /// captured part of the projected image. The meaning of a "perfect fit" is determined
    /// by the specified field factor (\p field_factor). See below for more on this.
    ///
    /// This operation allows for the size of the projection of the object of interest to be
    /// matched to the size of the viewport in a way that does not distort the perception of
    /// perspective, so long as the zoom factor remains at 1, and the aspect ratio (\ref
    /// aspect_ratio) and neutral field of view (\ref mean_neutral_fov) are properly
    /// configured.
    ///
    /// Both this function and \ref auto_zoom() can be used to control the projected size of
    /// the object of interest. This function does it by moving the object of interest to an
    /// appropriate distance from the camera, and \ref auto_zoom() does it be changing the
    /// zoom factor. The changing of the zoom factor does generally distort the perception
    /// of perspective.
    ///
    /// By default, when the field factor (\p field_factor) is 1, the sphere of interest
    /// will be fitted to the mean field of view meaning that the diameter of the projection
    /// of the sphere will be equal to the geometric mean between the with and the height of
    /// the viewport.
    ///
    /// One can obtain other fits by specifying other values for the field factor. For
    /// example, setting it to the value returned by \ref get_min_field_factor(), will
    /// result in a diameter of projection equal to the smaller of the width and the height
    /// of the viewport.
    ///
    /// \param interest_size The length of the diameter of the sphere of interest.
    ///
    /// \param field_factor Determines how the projection of the sphere of interest is
    /// fitted to the viewport (see general description above).
    ///
    /// \sa \ref auto_zoom()
    ///
    void auto_dist(double interest_size, double field_factor = 1) noexcept;

    /// \brief Fit sphere of interest to viewport by adjusting zoom factor.
    ///
    /// This function adjusts the zoom factor (\ref zoom_factor) such that the projection of
    /// the specified sphere of interest (\p interest_size) fits perfectly inside the
    /// captured part of the projected image. The meaning of a "perfect fit" is determined
    /// by the specified field factor (\p field_factor). See \ref auto_dist() for more on
    /// the meaning of a perfect fit and the field factor.
    ///
    /// This operation allows for the size of the projection of the object of interest to be
    /// matched to the size of the viewport in a way that does not change the camera
    /// distance (\ref camera_dist).
    ///
    /// Because the zoom factor is changed, using this function generally leads to a
    /// distortion of the perception of perspective (the well known depth-wise compression
    /// due to zoom). See \ref auto_dist() for another way of controlling the projected size
    /// of the object of interest, that avoids this distortion.
    ///
    /// \param interest_size The length of the diameter of the sphere of interest.
    ///
    /// \param field_factor Determines how the projection of the sphere of interest is
    /// fitted to the viewport. See \ref auto_dist() for more.
    ///
    /// \sa \ref auto_dist()
    ///
    void auto_zoom(double interest_size, double field_factor = 1) noexcept;

    /// \brief Neutral field of view for specific field factor.
    ///
    /// This function returns the neutral field of view of the camera for the specified
    /// field factor. The neutral field of view is calculated as the mean neutral field of
    /// view (\ref mean_neutral_fov) times the specified field factor (\p field_factor). See
    /// \ref get_mean_field_factor() for an explanation of the notion of a field factor.
    ///
    double get_neutral_fov(double field_factor = 1) noexcept;

    /// \brief Effective field of view for specific field factor.
    ///
    /// This function returns the effective field of view of the camera for the specified
    /// field factor. The effective field of view is calculated as the neutral field of view
    /// (\ref get_neutral_fov()) divided by the zoom factor (\ref zoom_factor). See \ref
    /// get_mean_field_factor() for an explanation of the notion of a field factor.
    ///
    double get_effective_fov(double field_factor = 1) noexcept;

    /// \brief Get neutral solid angle of view of camera.
    ///
    /// This function returns the neutral solid angle of view of the camera in
    /// steradians. This is the solid angle that corresponds to the currently configured
    /// mean neutral field of view of the camera (\ref mean_neutral_fov).
    ///
    /// The solid angle of view is defined as the area covered by the projection of the
    /// captured image onto a unit sphere centered on the oculus of the camera.
    ///
    /// \return The neutral solid angle of view of the camera in steradians.
    ///
    double get_neutral_solid_angle_of_view() noexcept;

    /// \brief Set neutral solid angle of view of camera.
    ///
    /// This function updates the mean neutral field of view of the camera (\ref
    /// mean_neutral_fov) such that it corresponds to the specified neutral solid angle of
    /// view.
    ///
    /// See \ref get_neutral_solid_angle_of_view() for more on solid angles of view.
    ///
    /// \param solid_angle The neutral solid angle of view measured in steradians.
    ///
    void set_neutral_solid_angle_of_view(double solid_angle) noexcept;

    /// \brief Get effective solid angle of view of camera.
    ///
    /// This function returns the effective solid angle of view of the camera in
    /// steradians. This is the solid angle that corresponds to the current effective field
    /// of view of the camera as specified by \ref mean_neutral_fov and \ref zoom_factor.
    ///
    /// See \ref get_neutral_solid_angle_of_view() for more on solid angles of view.
    ///
    /// \return The effective solid angle of view of the camera in steradians.
    ///
    double get_effective_solid_angle_of_view() noexcept;

    /// \brief Set effective solid angle of view of camera.
    ///
    /// This function updates the mean neutral field of view of the camera (\ref
    /// mean_neutral_fov) such that it corresponds to the specified effective solid angle of
    /// view. This takes the currently configured zoom factor (\ref zoom_factor) into
    /// account.
    ///
    /// See \ref get_neutral_solid_angle_of_view() for more on solid angles of view.
    ///
    /// \param solid_angle The effective solid angle of view measured in steradians.
    ///
    void set_effective_solid_angle_of_view(double solid_angle) noexcept;

    /// \brief Get neutral angle of view of camera.
    ///
    /// This function returns the neutral angle of view of the camera for the specified
    /// field factor (\p field_factor). See \ref get_mean_field_factor() for an explanation
    /// of the notion of a field factor.
    ///
    /// The returned neutral angle of view is determined by the currently configured mean
    /// neutral angle of view (\ref mean_neutral_fov) and the specified field factor.
    ///
    /// See \ref set_neutral_angle_of_view() for the correspondence between the neutral
    /// angle of view and the mean neutral field of view.
    ///
    /// \return The neutral angle of view measured in radians.
    ///
    double get_neutral_angle_of_view(double field_factor = 1) noexcept;

    /// \brief Set neutral angle of view of camera.
    ///
    /// This function sets the neutral field of view of the camera for the specified field
    /// factor (\p field_factor) to correspond to the specified angle of view (\p
    /// angle). See \ref get_mean_field_factor() for an explanation of the notion of a field
    /// factor.
    ///
    /// The correspondence between the mean neutral field of view of the camera (\ref
    /// mean_neutral_fov) and the specified neutral angle of view is as follows:
    ///
    ///     field_factor * mean_neutral_fov = fov
    ///     fov = 2 * tan(angle / 2)
    ///
    /// \param angle The neutral angle of view measured in radians.
    ///
    void set_neutral_angle_of_view(double angle, double field_factor = 1) noexcept;

    /// \brief Get effective angle of view of camera.
    ///
    /// This function returns the effective angle of view of the camera for the specified
    /// field factor (\p field_factor). See \ref get_mean_field_factor() for an explanation
    /// of the notion of a field factor.
    ///
    /// The returned effective angle of view is determined by the currently configured mean
    /// neutral angle of view (\ref mean_neutral_fov), the currently configured zoom factor
    /// (\ref zoom_factor) and the specified field factor.
    ///
    /// See \ref set_effective_angle_of_view() for the correspondence between the effective
    /// angle of view and the mean neutral field of view.
    ///
    /// \return The effective angle of view measured in radians.
    ///
    double get_effective_angle_of_view(double field_factor = 1) noexcept;

    /// \brief Set effective angle of view of camera.
    ///
    /// This function sets the neutral field of view of the camera for the specified field
    /// factor (\p field_factor) to correspond to the specified effective angle of view (\p
    /// angle). See \ref get_mean_field_factor() for an explanation of the notion of a
    /// field factor.
    ///
    /// The correspondence between the mean neutral field of view of the camera (\ref
    /// mean_neutral_fov) and the specified effective angle of view is as follows:
    ///
    ///     field_factor * mean_neutral_fov = zoom_factor * fov
    ///     fov = 2 * tan(angle / 2)
    ///
    /// \param angle The effective angle of view measured in radians.
    ///
    void set_effective_angle_of_view(double angle, double field_factor = 1) noexcept;

    /// \brief Field factor for mean field of view.
    ///
    /// This function returns 1, which is the field factor that corresponds to the mean
    /// field of view. The mean field of view is the geometric mean between the fields of
    /// view in the horizontal and vertical directions.
    ///
    /// Field factors can be passed to various functions of this class in order to adjust
    /// their behavior. Examples are \ref auto_dist(), \ref get_neutral_angle_of_view(), and
    /// \ref set_neutral_angle_of_view().
    ///
    /// The field factor for the mean field of view is the only field factor that is
    /// independent of aspect ratio (\ref aspect_ratio). All other field factors, such as
    /// the horizontal field factor (\ref get_horz_field_factor()), depend on the currently
    /// configured aspect ratio.
    ///
    /// \sa \ref get_horz_field_factor()
    /// \sa \ref get_vert_field_factor()
    /// \sa \ref get_diag_field_factor()
    /// \sa \ref get_min_field_factor()
    /// \sa \ref get_max_field_factor()
    ///
    double get_mean_field_factor() noexcept;

    /// \{
    ///
    /// \brief Field factor for horizontal and vertical directions.
    ///
    /// These functions return the field factors for the horizontal and vertical directions
    /// respectively.
    ///
    /// The field factor for the horizontal direction is the square root of the aspect ratio
    /// (\ref aspect_ratio), and the field factor for the vertical direction is the inverse
    /// of the square root of the aspect ratio.
    ///
    /// The horizontal field factor is the square root of the aspect ratio because when the
    /// mean field of view is multiplied by it, the result is the field of view in the
    /// horizontal direction. Likewise for the vertical direction. This is summarized below
    /// where \p horz_fov and \p vert_fov refer to the fields of view in the horizontal and
    /// vertical directions (neutral or effective):
    ///
    ///     mean_fov = sqrt(horz_fov * vert_fov)
    ///     aspect_ratio = horz_fov / vert_fov
    ///     horz_field_factor = sqrt(aspect_ratio)
    ///     vert_field_factor = sqrt(1 / aspect_ratio)
    ///
    ///     horz_field_factor * mean_fov = sqrt(aspect_ratio) * sqrt(horz_fov * vert_fov)
    ///                                  = sqrt(horz_fov / vert_fov) * sqrt(horz_fov * vert_fov)
    ///                                  = sqrt(horz_fov / vert_fov * horz_fov * vert_fov)
    ///                                  = sqrt(horz_fov * horz_fov)
    ///                                  = horz_fov
    ///
    ///     vert_field_factor * mean_fov = sqrt(1 / aspect_ratio) * sqrt(horz_fov * vert_fov)
    ///                                  = sqrt(vert_fov / horz_fov) * sqrt(horz_fov * vert_fov)
    ///                                  = sqrt(vert_fov / horz_fov * horz_fov * vert_fov)
    ///                                  = sqrt(vert_fov * vert_fov)
    ///                                  = vert_fov
    ///
    /// These field factors can be used with various functions of the perspective projection
    /// class, such as \ref auto_dist(), \ref get_neutral_angle_of_view(), and \ref
    /// set_neutral_angle_of_view().
    ///
    /// \sa \ref get_mean_field_factor()
    ///
    double get_horz_field_factor() noexcept;
    double get_vert_field_factor() noexcept;
    /// \}

    /// \brief Field factor for diagonal direction.
    ///
    /// This function returns the field factor for the diagonal direction, which is the
    /// field factor that, when multiplied with the mean field of view of the camera (\ref
    /// mean_neutral_fov), produces the field of view in the diagonal direction. The neutral
    /// diagonal field of view of the camera is the length of the diagonal of the captured
    /// part of the projected image on the image plane divided by the distance between the
    /// image plane and the oculus.
    ///
    /// The field factor for the diagonal direction is the square root of the sum of the
    /// aspect ratio and the inverse of the aspect ratio (\ref aspect_ratio). The reasons
    /// for why this works are summarized below where \p horz_fov, \p vert_fov, and \p
    /// diag_fov refer to the fields of view in the horizontal, vertical, and diagonal
    /// directions (neutral or effective):
    ///
    ///     mean_fov = sqrt(horz_fov * vert_fov)
    ///     aspect_ratio = horz_fov / vert_fov
    ///     diag_field_factor = sqrt(aspect_ratio + 1 / aspect_ratio)
    ///     diag_fov = sqrt(horz_fov * horz_fov + vert_fov * vert_fov)
    ///
    ///     diag_field_factor * mean_fov = sqrt(aspect_ratio + 1 / aspect_ratio) * sqrt(horz_fov * vert_fov)
    ///                                  = sqrt(horz_fov / vert_fov + vert_fov / horz_fov) * sqrt(horz_fov * vert_fov)
    ///                                  = sqrt((horz_fov / vert_fov + vert_fov / horz_fov) * horz_fov * vert_fov)
    ///                                  = sqrt(horz_fov * horz_fov + vert_fov * vert_fov)
    ///                                  = diag_fov
    ///
    /// This field factor can be used with various functions of the perspective projection
    /// class, such as \ref auto_dist(), \ref get_neutral_angle_of_view(), and \ref
    /// set_neutral_angle_of_view().
    ///
    /// \sa \ref get_mean_field_factor()
    ///
    double get_diag_field_factor() noexcept;

    /// \{
    ///
    /// \brief Minimum and maximum of horizontal and vertical field factors.
    ///
    /// These functions respectively return the minimum and maximum of the field factors for
    /// the horizontal and vertical directions. Thus, if the viewport height is less than
    /// the viewport width, then `get_min_field_factor()` is the same as
    /// `get_vert_field_factor()` and `get_max_field_factor()` is the same as
    /// `get_horz_field_factor()`. Likewise, if if the viewport height is greater than the
    /// viewport width, then `get_min_field_factor()` is the same as
    /// `get_horz_field_factor()` and `get_max_field_factor()` is the same as
    /// `get_vert_field_factor()`.
    ///
    /// These field factors can be used with various functions of the perspective projection
    /// class, such as \ref auto_dist(), \ref get_neutral_angle_of_view(), and \ref
    /// set_neutral_angle_of_view().
    ///
    /// \sa \ref get_mean_field_factor()
    /// \sa \ref get_horz_field_factor()
    /// \sa \ref get_vert_field_factor()
    ///
    double get_min_field_factor() noexcept;
    double get_max_field_factor() noexcept;
    /// \}
};








// Implementation


inline double PerspectiveProjection::get_horz_resol_dpcm() noexcept
{
    return 0.01 / horz_dot_pitch;
}


inline double PerspectiveProjection::get_vert_resol_dpcm() noexcept
{
    return 0.01 / vert_dot_pitch;
}


inline void PerspectiveProjection::set_resol_dpcm(double horz, double vert) noexcept
{
    horz_dot_pitch = 0.01 / horz;
    vert_dot_pitch = 0.01 / vert;
}


inline double PerspectiveProjection::get_horz_resol_dpi() noexcept
{
    return 0.0254 / horz_dot_pitch;
}


inline double PerspectiveProjection::get_vert_resol_dpi() noexcept
{
    return 0.0254 / vert_dot_pitch;
}


inline void PerspectiveProjection::set_resol_dpi(double h, double v) noexcept
{
    horz_dot_pitch = 0.0254 / h;
    vert_dot_pitch = 0.0254 / v;
}


inline double PerspectiveProjection::get_viewport_width_meters() noexcept
{
    return  view_dist * get_neutral_fov(get_horz_field_factor());
}


inline double PerspectiveProjection::get_viewport_height_meters() noexcept
{
    return  view_dist * get_neutral_fov(get_vert_field_factor());
}


inline void PerspectiveProjection::set_viewport_size_meters(double width, double height) noexcept
{
    aspect_ratio = width / height;
    mean_neutral_fov  = std::sqrt(width * height) / view_dist;
}


inline int PerspectiveProjection::get_viewport_width_pixels() noexcept
{
    return core::clamped_float_to_int<int>(std::round(get_viewport_width_meters() / horz_dot_pitch));
}


inline int PerspectiveProjection::get_viewport_height_pixels() noexcept
{
    return core::clamped_float_to_int<int>(std::round(get_viewport_height_meters() / vert_dot_pitch));
}


inline void PerspectiveProjection::set_viewport_size_pixels(int width, int height) noexcept
{
    set_viewport_size_meters(double(width) * horz_dot_pitch, double(height) * vert_dot_pitch);
}


inline double PerspectiveProjection::get_near_clip_dist() noexcept
{
    return camera_dist / std::sqrt(far_to_near_clip_ratio);
}


inline double PerspectiveProjection::get_far_clip_dist() noexcept
{
    return get_near_clip_dist() * far_to_near_clip_ratio;
}


inline double PerspectiveProjection::get_near_clip_width() noexcept
{
    return get_near_clip_dist() * get_effective_fov(get_horz_field_factor());
}


inline double PerspectiveProjection::get_near_clip_height() noexcept
{
    return get_near_clip_dist() * get_effective_fov(get_vert_field_factor());
}


inline void PerspectiveProjection::auto_dist(double interest_size, double field_factor) noexcept
{
    double zoom_factor = 1;
    camera_dist = interest_size * std::sqrt(0.25 + core::square(zoom_factor / mean_neutral_fov / field_factor));
}


inline void PerspectiveProjection::auto_zoom(double interest_size, double field_factor) noexcept
{
    zoom_factor = mean_neutral_fov * field_factor * std::sqrt(core::square(camera_dist / interest_size) - 0.25);
}


inline double PerspectiveProjection::get_neutral_fov(double field_factor) noexcept
{
    return mean_neutral_fov * field_factor;
}


inline double PerspectiveProjection::get_effective_fov(double field_factor) noexcept
{
    return get_neutral_fov(field_factor) / zoom_factor;
}


inline double PerspectiveProjection::get_neutral_solid_angle_of_view() noexcept
{
    return 4 * std::atan(1 / std::sqrt(4 / core::square(mean_neutral_fov) + aspect_ratio + 1 / aspect_ratio));
}


inline void PerspectiveProjection::set_neutral_solid_angle_of_view(double solid_angle) noexcept
{
    mean_neutral_fov = std::sqrt(4 / (core::square(1 / std::tan(solid_angle / 4)) - aspect_ratio - 1 / aspect_ratio));
}


inline double PerspectiveProjection::get_effective_solid_angle_of_view() noexcept
{
    return 4 * std::atan(1 / std::sqrt(4 / core::square(mean_neutral_fov / zoom_factor) + aspect_ratio +
                                       1 / aspect_ratio));
}


inline void PerspectiveProjection::set_effective_solid_angle_of_view(double solid_angle) noexcept
{
    mean_neutral_fov = zoom_factor * std::sqrt(4 / (core::square(1 / std::tan(solid_angle / 4)) - aspect_ratio -
                                                    1 / aspect_ratio));
}


inline double PerspectiveProjection::get_neutral_angle_of_view(double field_factor) noexcept
{
    return 2 * std::atan(field_factor * mean_neutral_fov / 2);
}


inline void PerspectiveProjection::set_neutral_angle_of_view(double angle, double field_factor) noexcept
{
    mean_neutral_fov = 2 * std::tan(angle / 2) / field_factor;
}


inline double PerspectiveProjection::get_effective_angle_of_view(double field_factor) noexcept
{
    return 2 * std::atan(field_factor * mean_neutral_fov / zoom_factor / 2);
}


inline void PerspectiveProjection::set_effective_angle_of_view(double angle, double field_factor) noexcept
{
    mean_neutral_fov = 2 * zoom_factor * std::tan(angle / 2) / field_factor;
}


inline double PerspectiveProjection::get_mean_field_factor() noexcept
{
    return 1;
}


inline double PerspectiveProjection::get_horz_field_factor() noexcept
{
    return std::sqrt(aspect_ratio);
}


inline double PerspectiveProjection::get_vert_field_factor() noexcept
{
    return 1 / std::sqrt(aspect_ratio);
}


inline double PerspectiveProjection::get_diag_field_factor() noexcept
{
    return std::sqrt(aspect_ratio + 1 / aspect_ratio);
}


inline double PerspectiveProjection::get_min_field_factor() noexcept
{
    return std::sqrt(std::min(aspect_ratio, 1 / aspect_ratio));
}


inline double PerspectiveProjection::get_max_field_factor() noexcept
{
    return std::sqrt(std::max(aspect_ratio, 1 / aspect_ratio));
}


} // namespace archon::util

#endif // ARCHON_X_UTIL_X_PERSPECT_PROJ_HPP
