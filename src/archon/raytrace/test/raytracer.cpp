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

/// \file
///
/// \author Kristian Spangsege
///
/// Testing the raytracer.

#include <cstdlib>
#include <string>
#include <stdexcept>

#include <archon/platform.hpp> // Never include this one in header files
#include <archon/core/cxx.hpp>
#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/util/perspect_proj.hpp>
#include <archon/util/color.hpp>
#include <archon/display/implementation.hpp>
#ifdef ARCHON_HAVE_OPENGL
#  include <archon/render/scene_builder.hpp>
#  include <archon/render/app.hpp>
#endif
#include <archon/raytrace/scene_build.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::image;
using namespace archon::display;
#ifdef ARCHON_HAVE_OPENGL
using namespace archon::render;
#endif
using namespace archon::raytrace;


namespace {

class AppError: public std::runtime_error {
public:
    AppError(const std::string& msg):
        std::runtime_error(msg)
    {
    }
};



void build_scene(SceneBuilder& builder)
{
    std::string assets_dir = get_value_of(build_config_param_DataDir) + "raytrace/test/";

/*
    // Shadow
    builder.push();
    builder.translate(Vec3(0, 1, -2));
    builder.rotate(Vec3(0, 1, 0), M_PI/4);

    builder.push();
    builder.translate(Vec3(0, -1, 0));
    builder.set_texture(assets_dir+"alien.jpg");
    builder.tex_scale(2);
    builder.add_cone();
    builder.reset_tex_transform();
    builder.set_texture();
    builder.pop();

    builder.push();
    builder.translate(Vec3(0, 0, -2));
    builder.set_texture(assets_dir+"proc_african_jade.jpg");
    builder.add_sphere();
    builder.set_texture();
    builder.pop();

    builder.push();
    builder.translate(Vec3(0, 0, 2));

    builder.push();
    builder.scale(0.03);
    builder.set_material_diffuse_color(Vec3(1,0,0));
    builder.add_sphere();
    builder.pop();

    for (int i = 0; i < 3; ++i) {
        Vec3 color = i == 0 ? Vec3(1,0,0) : i == 1 ? Vec3(0,1,0) : Vec3(0,0,1);
        builder.push();
        builder.rotate(Vec3(0, 0, 1), i*2*M_PI/3);
        builder.translate(Vec3(0.3, 0, 0));
        builder.set_light_color(color);
        builder.add_point_light();
        builder.scale(0.03);
        builder.translate(Vec3(0, 0, 1.1));
        builder.rotate(Vec3(1, 0, 0), M_PI/2);
        builder.set_material_diffuse_color(color);
        builder.add_cylinder();
        builder.pop();
    }

    builder.pop();

    builder.pop();
*/

/*
    // Box
    builder.set_texture(assets_dir+"test.png");
    builder.add_sphere();
*/


    // Torus
    builder.push();
    builder.set_texture(assets_dir+"spotty.png");
    builder.rotate(Vec3(0, 0, 1), -M_PI/36);
    builder.rotate(Vec3(0, 1, 0), -M_PI/9);
    builder.rotate(Vec3(1, 0, 0), M_PI/9);
    builder.scale(Vec3(1, 2, 1));
    builder.add_torus(3);
    builder.pop();

/*
    builder.set_color(Vec4(color::red, 1));

    builder.push();
    builder.translate(Vec3(0, 0.5, 0));
    builder.scale(0.25);
    builder.add_sphere();
    builder.pop();

    builder.set_color(Vec4(color::green, 1));

    builder.push();
    builder.translate(Vec3(0, -0.5, 0));
    builder.scale(Vec3(5, 0.25, 0.25));
    builder.add_sphere();
    builder.pop();
*/

/*
    builder.translate(Vec3(10, 20, 30));
    builder.add_point_light();
*/
}



#ifdef ARCHON_HAVE_OPENGL
class Preview: public Application {
public:
    Preview(archon::display::Connection::Arg display,
            const Application::Config& cfg, Raytracer& raytracer):
        Application("archon::Raytracer::Preview", cfg, std::locale(""), display),
        m_raytracer(raytracer)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);

        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

        m_list_id = glGenLists(1);
        OpenGlSceneBuilder gl_builder(m_list_id, get_texture_cache(), &m_used_textures);
        std::unique_ptr<SceneBuilder> ray_builder = make_scene_builder(m_raytracer, &gl_builder);
        build_scene(*ray_builder);
    }

    virtual ~Preview()
    {
        glDeleteLists(m_list_id, 1);
    }

private:
    void render_scene()
    {
        glCallList(m_list_id);
    }

    Raytracer& m_raytracer;
    GLuint m_list_id;
    std::vector<TextureUse> m_used_textures;
};
#endif



archon::display::Connection::Ptr try_get_display(bool insist = true)
{
    using namespace archon::display;
    try {
        Implementation::Ptr impl = get_default_implementation();
        return impl->new_connection();
    }
    catch (NoImplementationException& e) {
        if (insist)
            throw AppError("Display API is unavailable: " + std::string(e.what()));
    }
    catch (NoDisplayException& e) {
        if (insist)
            throw AppError("Could not connect to display: " + std::string(e.what()));
    }
    return archon::display::Connection::Ptr(); // Null
}

} // unnamed namespace


int main(int argc, const char* argv[]) throw()
{
    std::set_terminate(&Cxx::terminate_handler);

    try_fix_preinstall_datadir(argv[0], "raytrace/test/");

#ifdef ARCHON_HAVE_OPENGL
    bool              opt_preview(true);
#else
    bool              opt_preview(false);
#endif
    Series<2, int>    opt_img_size(512);
    Series<2, double> opt_scr_dpcm(0,0);
    double            opt_eye_scr_dist(0.5);
    double            opt_depth_of_field(1000);
    double            opt_interest_size(2);
    bool              opt_headlight(true);
    double            opt_ambience(0.2);
    Series<4, double> opt_bgcolor(0.0);
    int               opt_supersample(0);

    CommandlineOptions opts;
    opts.add_help("Test application for the raytracer");
    opts.check_num_args();
    opts.add_stop_opts();
    opts.add_param("p", "preview", opt_preview,
                   "Preview the scene in an interactive viewer before starting the actual "
                   "raytracing");
    opts.add_param("s", "img-size", opt_img_size,
                   "The desired size in pixels (width, height) of the raytraced image");
    opts.add_param("r", "scr-dpcm", opt_scr_dpcm,
                   "The resolution (horizontal, vertical) of the target screen in dots per "
                   "centimeter. If the value in one direction is zero or negative, then the "
                   "effective value in that direction will be determinaed automatically, "
                   "which may, or may not yield an accurate result.\n"
                   "To translate from dots per inch (dpi) to dots per centimeter, divide by "
                   "2.54 cm/in.\n"
                   "Specifying the wrong values here will produce the wrong field of view, "
                   "which in turn will produce the wrong aspect ratio between the Z-axis and "
                   "the X-Y-plane, which in turn leads to the depth effect appearing either "
                   "stretched or squeezed. It may also produce the wrong aspect ratio between "
                   "the X and Y-axes, which will lead to circles in the X-Y-plane appearing "
                   "egg-shaped");
    opts.add_param("d", "eye-scr-dist", opt_eye_scr_dist,
                   "The initial physical distance in meters between your eyes and the screen. "
                   "Specifying the wrong distance here will produce the wrong field of view, "
                   "which in turn will produce the wrong aspect ratio between the Z-axis "
                   "and the X-Y plane, which in turn leads to the depth effect appearing "
                   "either stretched or squeezed");
    opts.add_param("i", "interest-size", opt_interest_size,
                   "The diameter of the initial sphere of interest in global modelview"
                   "coordinates. By default, the viewing frustum will be made as narrow as "
                   "possible while it still contains the sphere of interest completely.");
    opts.add_param("H", "headlight", opt_headlight, "Turn on the headlight.");
    opts.add_param("a", "ambience", opt_ambience,
                   "The global ambient intencity. For each shaded pixel, this value times the "
                   "ambient color of the material is aded to the final color of the pixel");
    opts.add_param("b", "bgcolor", opt_bgcolor,
                   "The background color specified as a RGBA quadruple");
    opts.add_param("u", "supersample", opt_supersample, "The supersampling level. The number "
                   "of rays traced per target pixel is four to the power of the specified level. "
                   "Going beyond level 4 will normally not anything extra unless your target "
                   "image uses more than 8 bits per color/alpha channel.");

    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    std::unique_ptr<Raytracer> raytracer = make_raytracer();

    archon::display::Connection::Ptr display;

    // Auto-detect screen resolution
    if (opt_scr_dpcm[0] <= 0 || opt_scr_dpcm[1] <= 0) {
        display = try_get_display(false);
        if (opt_scr_dpcm[0] <= 0)
            opt_scr_dpcm[0] = display ? 0.01 / display->get_horiz_dot_pitch() : 96/2.54;
        if (opt_scr_dpcm[1] <= 0)
            opt_scr_dpcm[1] = display ? 0.01 / display->get_vert_dot_pitch()  : 96/2.54;
    }

    Vec3 eye;
    CoordSystem3x2 screen;

    if (opt_preview) {
#ifdef ARCHON_HAVE_OPENGL
        if (!display)
            display = try_get_display();

        {
            Application::Config app_cfg;
            app_cfg.win_size       = opt_img_size;
            app_cfg.scr_dpcm       = opt_scr_dpcm;
            app_cfg.eye_scr_dist   = opt_eye_scr_dist;
            app_cfg.depth_of_field = opt_depth_of_field;
            app_cfg.interest_size  = opt_interest_size;
            app_cfg.headlight      = opt_headlight;
            app_cfg.ambience       = opt_ambience;
            app_cfg.bgcolor        = opt_bgcolor;

            Preview preview(display, app_cfg, *raytracer);
            preview.run();
            preview.get_current_view(eye, screen);
            opt_img_size[0] = preview.get_window_width();
            opt_img_size[1] = preview.get_window_height();
            opt_headlight   = preview.is_headlight_enabled();
            opt_ambience    = preview.get_global_ambience();
            opt_bgcolor     = preview.get_background_color();
        }

        display->flush_output(); // Ensure window closure
#else
        throw AppError("OpenGL is not available");
#endif
    }
    else {
        Vec3 point_of_interest(0,0,0);
        Vec3 view_direction(0,0,-1);

        {
            PerspectiveProjection proj;
            proj.set_resol_dpcm(opt_scr_dpcm[0], opt_scr_dpcm[1]);
            proj.view_dist = opt_eye_scr_dist;
            proj.set_viewport_size_pixels(opt_img_size[0], opt_img_size[1]);
            proj.far_to_near_clip_ratio = opt_depth_of_field;
            proj.auto_dist(opt_interest_size, proj.get_min_field_factor());

            Vec3 z_axis = unit(-view_direction);
            Vec3 x_axis = Vec3(0,1,0) * z_axis;
            double x_sq = sq_sum(x_axis);
            if (x_sq == 0) {
                x_axis = Vec3(1,0,0);
            }
            else {
                x_axis /= sqrt(x_sq);
            }
            Vec3 y_axis = z_axis * x_axis;

            eye = point_of_interest + proj.camera_dist * z_axis;

            // Describe the 2-D screen coordinate system relative to the 3-D
            // view coordinate system
            screen.basis.col(0).set(proj.get_near_clip_width(), 0, 0);
            screen.basis.col(1).set(0, proj.get_near_clip_height(), 0);
            screen.origin.set(0, 0, -proj.get_near_clip_dist());
            screen.translate(Vec2(-0.5));

            // Rotate the screen to match the viewing direction
            CoordSystem3x3 view;
            view.basis.col(0) = x_axis;
            view.basis.col(1) = y_axis;
            view.basis.col(2) = z_axis;
            view.origin = eye;
            screen.pre_mult(view);
        }

        std::unique_ptr<SceneBuilder> builder = make_scene_builder(*raytracer);
        build_scene(*builder);
    }

    if (opt_headlight) {
        std::unique_ptr<Light> light(new PointLight(eye));
        raytracer->add_light(std::move(light));
    }

    raytracer->set_global_ambience(opt_ambience);
    raytracer->set_background_color(opt_bgcolor);

    Image::Ref img =
        Image::new_image(opt_img_size[0], opt_img_size[1], ColorSpace::get_RGB(), true);

    {
        ProgressBar progress;
        raytracer->render(img, eye, screen, &progress, opt_supersample);
    }

    std::string path = "/tmp/ray.png";
    img->save(path);
    std::cerr << "Saved '"<<path<<"'\n";
}
