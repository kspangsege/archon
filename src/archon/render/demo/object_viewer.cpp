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


#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <utility>
#include <algorithm>
#include <optional>
#include <tuple>
#include <string_view>
#include <vector>
#include <string>
#include <locale>
#include <system_error>
#include <filesystem>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/rotation.hpp>
#include <archon/util/color.hpp>
#include <archon/util/colors.hpp>
#include <archon/util/as_css_color.hpp>
#include <archon/image.hpp>
#include <archon/image/file_format_png.hpp>
#include <archon/gfx/build_object.hpp>
#include <archon/display.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/opengl.hpp>
#include <archon/render/object_builder.hpp>
#include <archon/render/load_texture.hpp>
#include <archon/render/engine.hpp>


using namespace archon;
using vertex_attrib = render::object_builder::vertex_attrib;


namespace {


constexpr GLuint g_attrib_location_coord     = 0;
constexpr GLuint g_attrib_location_normal    = 1;
constexpr GLuint g_attrib_location_color     = 2;
constexpr GLuint g_attrib_location_tex_coord = 3;


constexpr vertex_attrib g_attrib_layout[] = {
    vertex_attrib::coord_3,
    vertex_attrib::normal_3,
    vertex_attrib::color_3,
    vertex_attrib::tex_coord_2,
};


constexpr std::pair<vertex_attrib, GLuint> g_attrib_map[] = {
    { vertex_attrib::coord_3,     g_attrib_location_coord     },
    { vertex_attrib::normal_3,    g_attrib_location_normal    },
    { vertex_attrib::color_3,     g_attrib_location_color     },
    { vertex_attrib::tex_coord_2, g_attrib_location_tex_coord },
};


const char* g_vertex_shader_source = R"(
    #version 410 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec3 aColor;
    layout (location = 3) in vec2 aTexCoord;

    out vec3 vFragPos_viewSpace;
    out vec3 vNormal_viewSpace;
    out vec3 vColor;
    out vec2 vTexCoord;

    uniform mat4 uProjectionMatrix;
    uniform mat4 uModelViewMatrix;

    void main()
    {
        vFragPos_viewSpace = vec3(uModelViewMatrix * vec4(aPos, 1.0));
        vNormal_viewSpace = normalize(mat3(transpose(inverse(uModelViewMatrix))) * aNormal);
        vColor = aColor;
        vTexCoord = aTexCoord;

        gl_Position = uProjectionMatrix * uModelViewMatrix * vec4(aPos, 1.0);
    }
)";


const char* g_fragment_shader_source = R"(
    #version 410 core

    in vec3 vFragPos_viewSpace;
    in vec3 vNormal_viewSpace;
    in vec3 vColor;
    in vec2 vTexCoord;

    out vec4 FragColor;

    uniform sampler2D uTexture;
    uniform bool uHeadlightOn = true;

    void main()
    {
        float specularStrength = 0.1;
        float shininess = 32;
        vec3 headlightColor = vec3(0.9, 0.9, 0.9);
        vec3 ambientColor = vec3(0.02, 0.02, 0.02);

        vec4 texSample = texture(uTexture, vTexCoord);
        vec3 color = mix(vColor, texSample.rgb, texSample.a);
        vec3 result = ambientColor * color;

        if (uHeadlightOn) {
            vec3 lightPos_viewSpace = vec3(0, 0, 0);

            vec3 norm = normalize(vNormal_viewSpace);
            vec3 lightDir = normalize(lightPos_viewSpace - vFragPos_viewSpace);
            vec3 viewDir = normalize(lightPos_viewSpace - vFragPos_viewSpace);

            float diff = max(dot(norm, lightDir), 0);
            vec3 diffuse = diff * headlightColor * color;

            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0), shininess);
            vec3 specular = spec * specularStrength * headlightColor;

            result += diffuse + specular;
        }

        FragColor = vec4(result, 1);
    }
)";


struct object_proto {
    GLuint vbo;
    GLuint ebo;
    GLsizei num_indices;
};


struct object {
    GLuint vao;
    GLsizei num_indices;
};



class Scene final
    : public render::Engine::Scene {
public:
    struct config {
        util::Color color = util::colors::ivory;
        util::Color background_color = util::colors::black;
        double subdivision_level = 4;
        bool disable_texture_smoothing = false;
    };

    Scene(const std::locale&, log::Logger&, render::Engine&, const std::filesystem::path& texture_path, const config&);

    bool try_prepare(std::string&) override final;
    void render_init() override final;
    void set_projection(const math::Matrix4F&) override final;
    void render(const math::Matrix4F&) override final;

private:
    const std::locale m_locale;
    log::Logger& m_logger;
    render::Engine& m_engine;
    const std::filesystem::path m_texture_path;
    const config m_config;

    object_proto m_box = {};
    object_proto m_cylinder = {};
    object_proto m_cone = {};
    object_proto m_sphere = {};
    object_proto m_torus = {};

    std::vector<object> m_objects;
    std::size_t m_object_index = 0;

    GLuint m_shader_program = {};
    GLuint m_texture = {};
    GLsizei m_num_indices; // For currently selected object

    GLint m_model_view_loc = {};
    GLint m_proj_loc = {};
    GLint m_texture_loc = {};
    GLint m_headlight_on_loc = {};

    bool m_headlight_mode_1 = true;
    bool m_headlight_mode_2 = false;
    bool m_wireframe_mode_1 = false;
    bool m_wireframe_mode_2 = false;

    void select_object(std::size_t i);

    int adjust_subdivision(int val, int min) noexcept;

};



inline Scene::Scene(const std::locale& locale, log::Logger& logger, render::Engine& engine,
                    const std::filesystem::path& texture_path, const config& cfg)
    : m_locale(locale)
    , m_logger(logger)
    , m_engine(engine)
    , m_texture_path(texture_path) // Throws
    , m_config(cfg)
{
}


bool Scene::try_prepare(std::string& error)
{
    math::Rotation yaw   = { { 0, 1, 0 }, core::deg_to_rad(-31) };
    math::Rotation pitch = { { 1, 0, 0 }, core::deg_to_rad(+17) };
    m_engine.set_base_orientation(yaw + pitch);

    m_engine.bind_key(display::Key::small_s, "Spin", [&](bool down) {
        if (down) {
            m_engine.set_spin(math::Rotation({ 0, 1, 0 }, core::deg_to_rad(90))); // Throws
        }
        else {
            m_engine.set_spin(math::Rotation({ 0, 1, 0 }, core::deg_to_rad(0))); // Throws
        }
    }); // Throws

    m_engine.bind_key(display::Key::prior, "Previous object", [&](bool down) {
        if (down) {
            m_object_index = std::size_t(m_object_index == 0 ? m_objects.size() - 1 : m_object_index - 1);
            select_object(m_object_index); // Throws
        }
    }); // Throws

    m_engine.bind_key(display::Key::next, "Next object", [&](bool down) {
        if (down) {
            m_object_index = std::size_t(m_object_index == std::size_t(m_objects.size() - 1) ? 0 : m_object_index + 1);
            select_object(m_object_index); // Throws
        }
    }); // Throws

    m_engine.bind_key(display::Key::small_l, "Toggle headlight", [&](bool down) {
        if (down) {
            m_headlight_mode_1 = !m_headlight_mode_1;
            m_engine.need_redraw();
            // m_engine.set_on_off_status(L"HEADLIGHT", m_headlight_mode_1); // Throws                       
        }
    }); // Throws

    m_engine.bind_key(display::Key::small_w, "Toggle wireframe mode", [&](bool down) {
        if (down) {
            m_wireframe_mode_1 = !m_wireframe_mode_1;
            m_engine.need_redraw();
            // m_engine.set_on_off_status(L"WIREFRAME", m_wireframe_mode_1); // Throws                       
        }
    }); // Throws

    GLuint vertex_shader = {};
    if (ARCHON_UNLIKELY(!render::compile_shader(GL_VERTEX_SHADER, "vertex shader", g_vertex_shader_source,
                                                m_logger, vertex_shader))) { // Throws
        error = "Faield to compile vertex shader"; // Throws
        return false;
    }

    GLuint fragment_shader = {};
    if (ARCHON_UNLIKELY(!render::compile_shader(GL_FRAGMENT_SHADER, "fragment shader", g_fragment_shader_source,
                                                m_logger, fragment_shader))) { // Throws
        error = "Faield to compile fragment shader"; // Throws
        return false;
    }

    if (ARCHON_UNLIKELY(!render::link_shader("shader program", { vertex_shader, fragment_shader },
                                             m_logger, m_shader_program))) { // Throws
        error = "Faield to link shader program"; // Throws
        return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    m_proj_loc = glGetUniformLocation(m_shader_program, "uProjectionMatrix");
    m_model_view_loc = glGetUniformLocation(m_shader_program, "uModelViewMatrix");
    m_texture_loc = glGetUniformLocation(m_shader_program, "uTexture");
    m_headlight_on_loc = glGetUniformLocation(m_shader_program, "uHeadlightOn");
    if (m_model_view_loc < 0 || m_proj_loc < 0 || m_texture_loc < 0 || m_headlight_on_loc < 0) {
        error = "Failed to get uniform locations in shader program"; // Throws
        return false;
    }

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    {
        std::unique_ptr<image::WritableImage> image;
        log::PrefixLogger sublogger(m_logger, "Load texture: "); // Throws
        image::PNGLoadConfig png_load_config;
        png_load_config.expand_indirect_color = true;
        png_load_config.expand_lum_to_rgb = true;
        png_load_config.ensure_alpha_channel = true;
        image::FileFormat::SpecialLoadConfigRegistry special_load_config_registry;
        special_load_config_registry.register_(png_load_config); // Throws
        image::LoadConfig load_config;
        load_config.vertical_flip = true;
        load_config.logger = &sublogger;
        load_config.special = &special_load_config_registry;
        std::error_code ec;
        if (ARCHON_UNLIKELY(!image::try_load(m_texture_path, image, m_locale, load_config, ec))) { // Throws
            error = core::format("Failed to load texture image %s: %s", m_texture_path, ec.message()); // Throws
            return false;
        }
        bool require_format_match = false;
        bool preserve_precision = false;
        bool no_interp = m_config.disable_texture_smoothing;
        bool no_mipmap = false;
        render::load_and_configure_texture(*image, require_format_match, preserve_precision,
                                           no_interp, no_mipmap); // Throws
    }

    render::object_builder builder;
    builder.set_attrib_layout(g_attrib_layout); // Throws

    auto create_object_proto = [&](auto&& func) -> object_proto {
        builder.reset();
        builder.set_color(m_config.color);
        func(); // Throws

        GLuint vbo = {}, ebo = {};
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        GLsizei num_indices = builder.create(); // Throws

        return {
            vbo,
            ebo,
            num_indices,
        };
    };

    m_box = create_object_proto([&]() {
        builder.translate(-0.5f * math::Vector3F(1, 1, 1));
        int steps = adjust_subdivision(12, 1);
        gfx::build_box(builder, steps); // Throws
    }); // Throws

    m_cylinder = create_object_proto([&]() {
        builder.scale(0.5);
        bool has_side = true;
        bool has_top = false;
        bool has_bottom = false;
        int azimuth_steps = adjust_subdivision(36, 6);
        int height_steps = adjust_subdivision(12, 1);
        int radial_steps = adjust_subdivision(6, 1);
        builder.matrix_mode_tex_coord();
        builder.push(); // Throws
        builder.scale(3, 1, 1);
        gfx::build_cylinder(builder, has_side, has_top, has_bottom,
                            azimuth_steps, height_steps, radial_steps); // Throws
        builder.pop();
        has_side = false;
        has_top = true;
        has_bottom = true;
        gfx::build_cylinder(builder, has_side, has_top, has_bottom,
                            azimuth_steps, height_steps, radial_steps); // Throws
    }); // Throws

    m_cone = create_object_proto([&]() {
        builder.scale(0.5);
        bool has_side = true;
        bool has_bottom = false;
        int azimuth_steps = adjust_subdivision(36, 6);
        int height_steps = adjust_subdivision(12, 1);
        int radial_steps = adjust_subdivision(6, 1);
        builder.matrix_mode_tex_coord();
        builder.push(); // Throws
        builder.scale(3, 1, 1);
        gfx::build_cone(builder, has_side, has_bottom, azimuth_steps, height_steps, radial_steps); // Throws
        builder.pop();
        has_side = false;
        has_bottom = true;
        gfx::build_cone(builder, has_side, has_bottom, azimuth_steps, height_steps, radial_steps); // Throws
    }); // Throws

    m_sphere = create_object_proto([&]() {
        builder.scale(0.5);
        builder.matrix_mode_tex_coord();
        builder.scale(3, 1, 1);
        int azimuth_steps = adjust_subdivision(36, 6);
        int elevation_steps = adjust_subdivision(18, 3);
        gfx::build_sphere(builder, azimuth_steps, elevation_steps); // Throws
    }); // Throws

    m_torus = create_object_proto([&]() {
        builder.scale(0.5);
        builder.matrix_mode_tex_coord();
        builder.scale(3, 1, 1);
        float minor_radius = 0.5;
        int major_azimuth_steps = adjust_subdivision(36, 6);
        int minor_azimuth_steps = adjust_subdivision(18, 6);
        gfx::build_torus(builder, minor_radius, major_azimuth_steps, minor_azimuth_steps); // Throws
    }); // Throws

    return true;
}


void Scene::render_init()
{
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    math::Vector4F background_color;
    m_config.background_color.to_lin_vec(background_color);
    glClearColor(background_color[0], background_color[1], background_color[2], background_color[3]);

    glUseProgram(m_shader_program);
    glUniform1i(m_texture_loc, 0); // Use texture unit 0
    glBindTexture(GL_TEXTURE_2D, m_texture);

    auto add_object = [&](const object_proto& proto) {
        GLuint vao = {};
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, proto.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, proto.ebo);

        render::object_builder::configure_and_enable_attribs(g_attrib_layout, g_attrib_map); // Throws

        object obj = {
            vao,
            proto.num_indices,
        };
        m_objects.push_back(obj); // Throws
    };

    add_object(m_box);      // Throws
    add_object(m_cylinder); // Throws
    add_object(m_cone);     // Throws
    add_object(m_sphere);   // Throws
    add_object(m_torus);    // Throws

    select_object(0); // Throws
}


void Scene::set_projection(const math::Matrix4F& proj)
{
    render::set_uniform_matrix(m_proj_loc, proj); // Throws
}


void Scene::render(const math::Matrix4F& view)
{
    if (ARCHON_UNLIKELY(m_headlight_mode_1 != m_headlight_mode_2)) {
        render::set_uniform_bool(m_headlight_on_loc, m_headlight_mode_1); // Throws
        m_headlight_mode_2 = m_headlight_mode_1;
    }

    if (ARCHON_UNLIKELY(m_wireframe_mode_1 != m_wireframe_mode_2)) {
        // FIXME: Consider adding proper wireframe mode by using the barycentric coordinates scheme    
        glPolygonMode(GL_FRONT_AND_BACK, m_wireframe_mode_1 ? GL_LINE : GL_FILL);
        m_wireframe_mode_2 = m_wireframe_mode_1;
    }

    math::Matrix4F model_view = view;
    render::set_uniform_matrix(m_model_view_loc, model_view); // Throws

    render::object_builder::draw(m_num_indices); // Throws
}


void Scene::select_object(std::size_t i)
{
    ARCHON_ASSERT(i < m_objects.size());
    const object& obj = m_objects[i];
    glBindVertexArray(obj.vao);
    m_num_indices = obj.num_indices;
    m_engine.need_redraw();
    // m_engine.set_status("%s", obj.label); // Throws                       
}



int Scene::adjust_subdivision(int val, int min) noexcept
{
    return std::max(int(std::ceil(m_config.subdivision_level * val)), min);
}


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    namespace fs = std::filesystem;
    bool list_display_implementations = false;
    render::Engine::Config engine_config;
    Scene::config scene_config;
    std::optional<fs::path> optional_texture;
    display::Size window_size = 512;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    std::optional<std::string> optional_display_implementation;
    std::optional<int> optional_screen;
    std::optional<std::string> optional_x11_display;
    std::optional<display::x11_fullscreen_monitors> optional_x11_fullscreen_monitors;
    std::optional<int> optional_x11_visual_depth;
    std::optional<display::x11_connection_config::VisualClass> optional_x11_visual_class;
    std::optional<std::uint_fast32_t> optional_x11_visual_type;
    bool x11_prefer_default_nondecomposed_colormap = false;
    bool x11_disable_double_buffering = false;
    bool x11_disable_glx_direct_rendering = false;
    bool x11_disable_detectable_autorepeat = false;
    bool x11_synchronous_mode = false;
    bool x11_install_colormaps = false;
    bool x11_colormap_weirdness = false;

    cli::Spec spec;
    pat("", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie()); // Throws

    pat("--list-display-implementations", cli::no_attributes, spec,
        "List known display implementations.",
        [&] {
            list_display_implementations = true;
        }); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-o, --color", "<color>", cli::no_attributes, spec,
        "Set the foreground color. \"@A\" can be any valid CSS3 color value with, or without an alpha component, as "
        "well as the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The default "
        "color is @Q.",
        cli::assign(util::as_css_color(scene_config.color))); // Throws

    opt("-b, --background-color", "<color>", cli::no_attributes, spec,
        "Set the background color. \"@A\" can be any valid CSS3 color value with, or without an alpha component, as "
        "well as the extended hex-forms, \"#RGBA\" and \"#RRGGBBAA\", accommodating the alpha component. The default "
        "color is @Q.",
        cli::assign(util::as_css_color(scene_config.background_color))); // Throws

    opt("-L, --subdivision-level", "<val>", cli::no_attributes, spec,
        "Change amount of geometry subdivision. Double the value means roughly twice as much subdivision. The default "
        "is @V.",
        cli::assign(scene_config.subdivision_level)); // Throws

    opt("-t, --texture", "<path>", cli::no_attributes, spec,
        "Use this texture instead of the default one.",
        cli::assign(optional_texture)); // Throws

    opt("-m, --disable-texture-smoothing", "", cli::no_attributes, spec,
        "Turn off texture smoothing.",
        cli::raise_flag(scene_config.disable_texture_smoothing)); // Throws

    opt("-S, --window-size", "<size>", cli::no_attributes, spec,
        "Set the window size in number of pixels. \"@A\" can be specified either as a pair \"<width>,<height>\", or "
        "as a single value, which is then used as both width and height. The default size is @V.",
        cli::assign(window_size)); // Throws

    opt("-E, --resolution", "<resolution>", cli::no_attributes, spec,
        "The initial physical resolution in pixels per centimeter. \"@A\" can be specified either as a pair "
        "\"<horz>,<vert>\", or as a single value, which is then used as both horizontal and vertical resolution. "
        "Values can be fractional using `.` as decimal point. The default resolution is @V.",
        cli::assign(engine_config.resolution)); // Throws

    opt("-r, --frame-rate", "<rate>", cli::no_attributes, spec,
        "The initial frame rate limit. The frame rate limit marks the upper limit on the number of frames per second. "
        "The value can be fractional using `.` as decimal point. The default initial rate limit is @V.",
        cli::assign(engine_config.frame_rate)); // Throws

    opt("-e, --disable-resolution-tracking", "", cli::no_attributes, spec,
        "Turn off resolution tracking mode.",
        cli::raise_flag(engine_config.disable_resolution_tracking)); // Throws

    opt("-g, --disable-frame-rate-tracking", "", cli::no_attributes, spec,
        "Turn off frame rate tracking mode.",
        cli::raise_flag(engine_config.disable_frame_rate_tracking)); // Throws

    opt("-f, --fullscreen", "", cli::no_attributes, spec,
        "Open window in fullscreen mode.",
        cli::raise_flag(engine_config.fullscreen_mode)); // Throws

    opt("-l, --log-level", "<level>", cli::no_attributes, spec,
        "Set the log level limit. The possible levels are @G. The default limit is @Q.",
        cli::assign(log_level_limit)); // Throws

    opt("-i, --display-implementation", "<ident>", cli::no_attributes, spec,
        "Use the specified display implementation. Use `--list-display-implementations` to see which implementations "
        "are available. It is possible that no implementations are available. By default, if any implementations are "
        "available, the one, that is listed first by `--list-display-implementations`, is used.",
        cli::assign(optional_display_implementation)); // Throws

    opt("-s, --screen", "<number>", cli::no_attributes, spec,
        "Target the specified screen (@A). This is an index between zero and the number of screens minus one. If this "
        "option is not specified, the default screen of the display will be targeted.",
        cli::assign(optional_screen)); // Throws

    opt("-D, --x11-display", "<string>", cli::no_attributes, spec,
        "When using the X11-based display implementation, target the specified X11 display (@A). If this option is "
        "not specified, the value of the DISPLAY environment variable will be used.",
        cli::assign(optional_x11_display)); // Throws

    opt("-F, --x11-fullscreen-monitors", "<monitors>", cli::no_attributes, spec,
        "When using the X11-based display implementation, use the specified Xinerama screens (monitors) to define the "
        "fullscreen area. \"@A\" can be specified as one, two, or four comma-separated Xinerama screen indexes "
        "(`xrandr --listactivemonitors`). When four values are specified they will be interpreted as the Xinerama "
        "screens that determine the top, bottom, left, and right edges of the fullscreen area. When two values are "
        "specified, the first one determines both top and left edges and the second one determines bottom and right "
        "edges. When one value is specified, it determines all edges.",
        cli::assign(optional_x11_fullscreen_monitors)); // Throws

    opt("-d, --x11-visual-depth", "<num>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified depth (@A).",
        cli::assign(optional_x11_visual_depth)); // Throws

    opt("-c, --x11-visual-class", "<name>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified class (@A). The class can be "
        "@F.",
        cli::assign(optional_x11_visual_class)); // Throws

    opt("-V, --x11-visual-type", "<num>", cli::no_attributes, spec,
        "When using the X11-based display implementation, pick a visual of the specified type (@A). The type, also "
        "known as the visual ID, is a 32-bit unsigned integer that can be expressed in decimal, hexadecimal (with "
        "prefix '0x'), or octal (with prefix '0') form.",
        cli::exec([&](std::string_view str) {
            core::ValueParser parser(locale);
            std::uint_fast32_t type = {};
            if (ARCHON_LIKELY(parser.parse(str, core::as_flex_int(type)))) {
                if (ARCHON_LIKELY(type <= core::int_mask<std::uint_fast32_t>(32))) {
                    optional_x11_visual_type.emplace(type);
                    return true;
                }
            }
            return false;
        })); // Throws

    opt("-C, --x11-prefer-default-nondecomposed-colormap", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, prefer the use of the default colormap when the default "
        "visual is used and is a PseudoColor or GrayScale visual. This succeeds if enough colors can be allocated. "
        "Otherwise a new colormap is created.",
        cli::raise_flag(x11_prefer_default_nondecomposed_colormap)); // Throws

    opt("-B, --x11-disable-double-buffering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of double buffering, even when the selected "
        "visual supports double buffering.",
        cli::raise_flag(x11_disable_double_buffering)); // Throws

    opt("-R, --x11-disable-glx-direct-rendering", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, disable use of GLX direct rendering, even in cases where "
        "GLX direct rendering is possible.",
        cli::raise_flag(x11_disable_glx_direct_rendering)); // Throws

    opt("-A, --x11-disable-detectable-autorepeat", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, do not turn on \"detectable auto-repeat\" mode, as it is "
        "offered by the X Keyboard Extension, even when it can be turned on. Instead, rely on the fall-back detection "
        "mechanism.",
        cli::raise_flag(x11_disable_detectable_autorepeat)); // Throws

    opt("-y, --x11-synchronous-mode", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, turn on X11's synchronous mode. In this mode, buffering of "
        "X protocol requests is turned off, and the Xlib functions, that generate X requests, wait for a response "
        "from the server before they return. This is sometimes useful when debugging.",
        cli::raise_flag(x11_synchronous_mode)); // Throws

    opt("-I, --x11-install-colormaps", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, install a window's colormap right after the creation of the "
        "window. This mode should only be enabled for debugging purposes, or when running against a server where "
        "there is no window manager.",
        cli::raise_flag(x11_install_colormaps)); // Throws

    opt("-W, --x11-colormap-weirdness", "", cli::no_attributes, spec,
        "When using the X11-based display implementation, introduce detectable weirdness into newly created "
        "colormaps.",
        cli::raise_flag(x11_colormap_weirdness)); // Throws

    int exit_status = 0;
    if (ARCHON_UNLIKELY(cli::process(argc, argv, spec, exit_status, locale))) // Throws
        return exit_status;

    display::Guarantees guarantees;

    // Promise to not open more than one display connection at a time.
    guarantees.only_one_connection = true;

    // Promise that all use of the display API happens on behalf of the main thread.
    guarantees.main_thread_exclusive = true;

    // Promise that there is no direct or indirect use of the Xlib library (X Window System
    // client library) other than through the Archon display library.
    guarantees.no_other_use_of_x11 = true;

    // Promise that there is no direct or indirect use of SDL (Simple DirectMedia Layer)
    // other than through the Archon Display Library, and that there is also no direct or
    // indirect use of anything that would conflict with use of SDL.
    guarantees.no_other_use_of_sdl = true;

    if (list_display_implementations) {
        display::list_implementations(core::File::get_stdout(), locale, guarantees); // Throws
        return EXIT_SUCCESS;
    }

    log::FileLogger root_logger(core::File::get_stderr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

    // `src_root` is the relative path to the root of the source tree from the root of the
    // project.
    //
    // `src_path` is the relative path to this source file from the root of source tree.
    //
    // `bin_path` is the relative path to the executable from the root of the source root as
    // it is reflected into the build directory.
    //
    core::BuildEnvironment::Params build_env_params;
    build_env_params.file_path = __FILE__;
    build_env_params.bin_path  = "archon/render/demo/archon-object-viewer";
    build_env_params.src_path  = "archon/render/demo/object_viewer.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    namespace fs = std::filesystem;
    fs::path resource_path = (build_env.get_relative_source_root() /
                              core::make_fs_path_generic("archon/render/demo", locale)); // Throws
    fs::path texture_path;
    if (optional_texture.has_value()) {
        texture_path = optional_texture.value(); // Throws
    }
    else {
        texture_path = (resource_path / core::make_fs_path_generic("archon_text.png", locale)); // Throws
    }

    const display::Implementation* impl = {};
    std::string error;
    if (ARCHON_UNLIKELY(!display::try_pick_implementation(optional_display_implementation, guarantees,
                                                          impl, error))) { // Throws
        logger.error("Failed to pick display implementation: %s", error); // Throws
        return EXIT_FAILURE;
    }
    logger.detail("Display implementation: %s", impl->get_slot().get_ident()); // Throws

    log::PrefixLogger display_logger(logger, "Display: "); // Throws
    display::Connection::Config connection_config;
    connection_config.logger = &display_logger;
    connection_config.x11.display = optional_x11_display;
    connection_config.x11.visual_depth = optional_x11_visual_depth;
    connection_config.x11.visual_class = optional_x11_visual_class;
    connection_config.x11.visual_type = optional_x11_visual_type;
    connection_config.x11.fullscreen_monitors = optional_x11_fullscreen_monitors;
    connection_config.x11.prefer_default_nondecomposed_colormap = x11_prefer_default_nondecomposed_colormap;
    connection_config.x11.disable_double_buffering = x11_disable_double_buffering;
    connection_config.x11.disable_glx_direct_rendering = x11_disable_glx_direct_rendering;
    connection_config.x11.disable_detectable_autorepeat = x11_disable_detectable_autorepeat;
    connection_config.x11.synchronous_mode = x11_synchronous_mode;
    connection_config.x11.install_colormaps = x11_install_colormaps;
    connection_config.x11.colormap_weirdness = x11_colormap_weirdness;
    std::unique_ptr<display::Connection> conn;
    if (ARCHON_UNLIKELY(!impl->try_new_connection(locale, connection_config, conn, error))) { // Throws
        logger.error("Failed to open display connection: %s", error); // Throws
        return EXIT_FAILURE;
    }

    int screen;
    if (!optional_screen.has_value()) {
        screen = conn->get_default_screen();
    }
    else {
        int val = optional_screen.value();
        int num_screens = conn->get_num_screens();
        if (ARCHON_UNLIKELY(val < 0 || val >= num_screens)) {
            logger.error("Specified screen index (%s) is out of range", core::as_int(val)); // Throws
            return EXIT_FAILURE;
        }
        screen = val;
    }

    engine_config.screen = screen;
    engine_config.logger = &logger;
    engine_config.allow_window_resize = true;
    engine_config.minimum_window_size = 128;

    render::Engine engine;
    Scene scene(locale, logger, engine, texture_path, scene_config); // Throws
    if (ARCHON_UNLIKELY(!engine.try_create(scene, *conn, "Archon Object Viewer", window_size, locale,
                                           engine_config, error))) { // Throws
        logger.error("Failed to create render engine: %s", error); // Throws
        return EXIT_FAILURE;
    }
    engine.run(); // Throws
}
