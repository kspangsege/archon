// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2025 Kristian Spangsege <kristian.spangsege@gmail.com>
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


                        
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <string_view>
#include <string>
#include <unordered_map>
#include <locale>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/hash_fnv.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/rotation.hpp>
#include <archon/display.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/render/opengl.hpp>
#include <archon/render/engine.hpp>


using namespace archon;


namespace {


constexpr int g_chunk_size_x = 16;
constexpr int g_chunk_size_y = 16;
constexpr int g_chunk_size_z = 16;



using chunk_array_coord_type = std::int_fast8_t;
using block_coord_type = std::int_fast64_t;


struct chunk_array_pos {
    chunk_array_coord_type x, y, z;
};


struct block_pos {
    block_coord_type x, y, z;
};


struct chunk_pos {
    block_coord_type x, y, z;

    constexpr auto operator<=>(const chunk_pos& other) const noexcept = default;
};


struct chunk_pos_hash {
    auto operator()(const chunk_pos& pos) const noexcept -> std::size_t
    {
        core::Hash_FNV_1a_32 hash;
        hash.add_int(pos.x);
        hash.add_int(pos.y);
        hash.add_int(pos.z);
        return hash.get();
    }
};


struct chunk {
    std::uint_least16_t blocks[g_chunk_size_x][g_chunk_size_y][g_chunk_size_z];

    chunk* next;
    chunk* prev;
};



class world final
    : public render::Engine::Scene {
public:
    world();

    void render_init() override final;
    void render() override final;

private:
    std::unordered_map<chunk_pos, std::unique_ptr<chunk>, chunk_pos_hash> m_chunks;
    chunk* m_unlinked_chunks; // First one is the one that became unlinked first

    bool m_thrust_forwards = false, m_thrust_backwards = false;
    bool m_thrust_leftwards = false, m_thrust_rightwards = false;
    bool m_thrust_upwards = false, m_thrust_downwards = false;
    bool m_sprint_mode = false;

    chunk_pos m_current_chunk = { 0, 0, 0 };

    // Position of camera relative to origin of current chunk. Measured in texel units. One
    // texel unit is one 16th of the side length of a block.
    math::Vector3 m_position;

    // Velocity of camera. Measured in texel units.
    math::Vector3 m_velocity;

    // Consider the chunk at `m_chunk_array[z][y][x]` to be at X, Y, and Z-coordinates, `x`,
    // `y`, and `z`.
    int m_chunk_array_size_x;
    int m_chunk_array_size_y;
    int m_chunk_array_size_z;
    std::unique_ptr<chunk*[]> m_chunk_array;

    // List of chunks that are within the currently selected render distance. Chunks occur
    // in an order that ensures back-to-front rendering. Chunk array positions are relative
    // to the center chunk (the one containing the player).
    std::vector<chunk_array_pos> m_chunk_order;

    // In number of blocks
    void change_render_distance(double horz_dist, double vert_dist);

    // In number of blocks
    void set_position(const math::Vector3& pos, const block_pos& ref = {});
    bool try_set_position(const math::Vector3& pos, const block_pos& ref = {}) noexcept;

    void render_avatar();
    void render_opaque(const chunk&);

    // In meters above feet
    double get_eye_height() const noexcept;

    auto get_chunk(const chunk_array_pos& pos) const noexcept -> chunk*&;
};



world::world()
{
    change_render_distance(64, 64); // Throws
    set_position({ 0, 0, 0 }); // Throws
}



void world::render_init()
{
#if ARCHON_RENDER_HAVE_OPENGL

    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);

#ifdef GL_LIGHT_MODEL_COLOR_CONTROL
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
#endif
#ifdef GL_LIGHT_MODEL_LOCAL_VIEWER
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
#endif

#endif // ARCHON_RENDER_HAVE_OPENGL
}



void world::render()
{
    glEnable(GL_DEPTH_TEST);
    // FIXME: Guard against missing OpenGL header      
    int n = 16;          
    glScaled(1.0 / n, 1.0 / n, 1.0 / n);    
    math::Vector3 eye_displacement = { 0, 0, 0 };
    eye_displacement[1] += get_eye_height() * n;
    glTranslated(-eye_displacement[0], -eye_displacement[1], -eye_displacement[2]);
    glDisable(GL_TEXTURE_2D);
    render_avatar(); // Throws
    glTranslated(-m_position[0], -m_position[1], -m_position[2]);
    glEnable(GL_TEXTURE_2D);
    glColor3d(1, 1, 1);


    // Render entities
    

    // Render opaque blocks
    for (chunk_array_pos pos : m_chunk_order) {
        // FIXME: Find a way to efficiently skip some of the chunnks that are definitely not overlapping with the view frustum    

        chunk_pos pos_2 = {
            block_coord_type(m_current_chunk.x + pos.x),
            block_coord_type(m_current_chunk.y + pos.y),
            block_coord_type(m_current_chunk.z + pos.z),
        };
        int x = core::int_periodic_mod(pos_2.x,  m_chunk_array_size_x);
        int y = core::int_periodic_mod(pos_2.y,  m_chunk_array_size_y);
        int z = core::int_periodic_mod(pos_2.z,  m_chunk_array_size_z);
        std::size_t i = std::size_t((std::size_t(z) * m_chunk_array_size_y + y) * m_chunk_array_size_x + x);
        chunk*& chunk_1 = m_chunk_array[i];
        if (ARCHON_UNLIKELY(!chunk_1)) {
            std::unique_ptr<chunk>& chunk_2 = m_chunks[pos_2]; // Throws
            if (ARCHON_UNLIKELY(!chunk_2)) {
                // FIXME: Consider setting max cache size to number of chunks in largest possible chunk array    
                // FIXME: If chunk cache is now too large, and there are unlinked chunks, discard one of those but reuse the chunk object    
                chunk_2 = std::make_unique<chunk>(); // Throws
                // FIXME: Request initialization of this chunk    
            }
            chunk_1 = chunk_2.get();
        }

        // FIXME: Skip this chunk if it has no contents (all air, or all same block type and fully surrounded by that block type)

        render_opaque(*chunk_1); // Throws

        // FIXME: If this chunk has semi-transparent texels, add it to m_transparent_chunks
    }

    // Render transparent blocks
    

    // Render HUD
    glDisable(GL_DEPTH_TEST);
    

    // Rendering proceeds in three stages:
    //
    //   1. Render entities. These cannot make use of transparent or semi-transaprent textures. This is done with depth test and writing to depth buffer.
    //
    //   2. Render the opaque parts of chunks. This can be done in any chunk order, and make use of a precomputed call list for each chunk. This is done with depth test and writing to depth buffer.
    //
    //   3. Rendering of the surfaces with transparent textures. This is done chunk by chunk in order of closeness to the player (farthest first). For each chunk that has textured surfaces with transparent features, render those in order of closenes to the player. Achieve this by way of a tree (bsp, octree, ...)
    //
    // In order to facilitate the handling of chunks in the right order (farthest to closest), a list of chunk grid positions must be precomputed for a particular rendering distance. Chunks are then visited in this order in step 2, where each chunk with transparency is appended to a list of chunks to be processed in step 3.

    // Both transparent and non-transparent quads are rendered with backface culling enabled.

    // FIXME: Consider blending parameters to allow for textures with holes as opposed to textures with semitransparent texels      

    // How to arrange the transparent quads in each chunk to allow for fast ordered traversal? --> kd-tree-ish over block positions

    // FIXME: When position is changed: Clear rolled-over part of chunk grid

    // Loop over all chunk positions in grid:
    //   If chunk intersects with viewing frustum out to render distance:
    //     If chunk is missing:
    //       Look it up in cache by hash key from chunk coordinates:
    //         If not found:
    //           Request it (generate it)
    //     If chunk is non-empty, render it

/*
    for (int i_z = 0; i_z < 12; ++i_z) {
        for (int i_y = 0; i_y < 12; ++i_y) {
            for (int i_x = 0; i_x < 12; ++i_x) {
                int x_2 = x + i_x;
                int y_2 = y + i_y;
                int z_2 = z + i_z;
                chunk& chunk = get_chunk(x_2, y_2, z_2);
                if (chunk.call_list == 0)
                    continue;
                if (chunk.dirty)
                    update_chunk_call_list(chunk, x_2, y_2, z_2);
                glPushMatrix();
                int x_3 = x_2 * (s_num_x*n);
                int y_3 = y_2 * (s_num_y*n);
                int z_3 = z_2 * (s_num_z*n);
                glTranslatef(float(x_3), float(y_3), float(z_3));
                glCallList(chunk.call_list);
                glPopMatrix();
            }
        }
    }
*/
}



void world::change_render_distance(double horz_dist, double vert_dist)
{
    double dist_x = horz_dist / double(g_chunk_size_x);
    double dist_y = horz_dist / double(g_chunk_size_y);
    double dist_z = vert_dist / double(g_chunk_size_z);

    // This function computes the list of local chunk array positions including only those chunks whose
    // center point is no further from the center point of the center chunk than the specified render
    // distance.
    //
    // For the purpose of rendering of semi-transparent surfaces, the list is computed such
    // that for any chunk, C, in the local chunk array, and two points, A and B, where A
    // falls inside the center chunk and B falls inside C, any chunk that intersects the
    // line segment from A to B and is neither the center chunk not C occurs in the list
    // after C.
    //
    // First, however, a properly sized local chunk array needs to be created. Its size is
    // determined such that it has a chunk at the center, i.e., the center chunk, and such
    // that it covers any chunk that is within the render distance as explained above.

    constexpr int max_rings_x = 32;
    constexpr int max_rings_y = 32;
    constexpr int max_rings_z = 32;
    static_assert(max_rings_x >= 0);
    static_assert(max_rings_y >= 0);
    static_assert(max_rings_z >= 0);
    static_assert(max_rings_x <= (core::int_max<int>() - 1) / 2);
    static_assert(max_rings_y <= (core::int_max<int>() - 1) / 2);
    static_assert(max_rings_z <= (core::int_max<int>() - 1) / 2);

    constexpr int max_size_x = 1 + 2 * max_rings_x;
    constexpr int max_size_y = 1 + 2 * max_rings_y;
    constexpr int max_size_z = 1 + 2 * max_rings_z;
    static_assert(core::can_int_cast<std::size_t>(max_size_x));
    static_assert(core::can_int_cast<std::size_t>(max_size_y));
    static_assert(core::can_int_cast<std::size_t>(max_size_z));
    static_assert(std::size_t(max_rings_y) < std::size_t(core::int_max<std::size_t>() / max_size_z));
    static_assert(std::size_t(max_rings_x) < std::size_t(core::int_max<std::size_t>() /
                                                         (std::size_t(max_size_z) * max_size_y)));

    int num_rings_x = {}, num_rings_y = {}, num_rings_z = {};
    core::clamped_float_to_int(dist_x, num_rings_x);
    core::clamped_float_to_int(dist_y, num_rings_y);
    core::clamped_float_to_int(dist_z, num_rings_z);
    num_rings_x = std::clamp(num_rings_x, 0, max_rings_x);
    num_rings_y = std::clamp(num_rings_y, 0, max_rings_y);
    num_rings_z = std::clamp(num_rings_z, 0, max_rings_z);

    int size_x = 1 + 2 * num_rings_x;
    int size_y = 1 + 2 * num_rings_y;
    int size_z = 1 + 2 * num_rings_z;
    m_chunk_array_size_x = size_x;
    m_chunk_array_size_y = size_y;
    m_chunk_array_size_z = size_z;

    std::size_t size_2 = std::size_t(size_z * (size_y * std::size_t(size_x)));
    m_chunk_array = std::make_unique<chunk*[]>(size_2); // Throws

    static_assert(core::is_signed<chunk_array_coord_type>());
    static_assert(max_rings_x <= core::int_max<chunk_array_coord_type>());
    static_assert(max_rings_y <= core::int_max<chunk_array_coord_type>());
    static_assert(max_rings_z <= core::int_max<chunk_array_coord_type>());

    chunk_array_coord_type order_x[max_size_x] = {};
    chunk_array_coord_type order_y[max_size_y] = {};
    chunk_array_coord_type order_z[max_size_z] = {};
    for (int i = 0; i < size_x; ++i)
        order_x[i] = chunk_array_coord_type(i < num_rings_x ? i - num_rings_x : size_x - 1 - i);
    for (int i = 0; i < size_y; ++i)
        order_y[i] = chunk_array_coord_type(i < num_rings_y ? i - num_rings_y : size_y - 1 - i);
    for (int i = 0; i < size_z; ++i)
        order_z[i] = chunk_array_coord_type(i < num_rings_z ? i - num_rings_z : size_z - 1 - i);

    m_chunk_order.clear();
    for (int i = 0; i < size_z; ++i) {
        chunk_array_coord_type z = order_z[i];
        for (int j = 0; j < size_y; ++j) {
            chunk_array_coord_type y = order_y[j];
            for (int k = 0; k < size_x; ++k) {
                chunk_array_coord_type x = order_x[k];
                math::Vector3 vec = {
                    double(x) / dist_x,
                    double(y) / dist_y,
                    double(z) / dist_z,
                };
                if (math::sq_sum(vec) <= 1)
                    m_chunk_order.push_back({ x, y, z }); // Throws
            }
        }
    }
}



void world::set_position(const math::Vector3& pos, const block_pos& ref)
{
    if (ARCHON_LIKELY(try_set_position(pos, ref)))
        return;
    throw std::runtime_error("Position out of bounds");
}



bool world::try_set_position(const math::Vector3& pos, const block_pos& ref) noexcept
{
    double x_1 = std::floor(pos[0]);
    double y_1 = std::floor(pos[1]);
    double z_1 = std::floor(pos[2]);

    block_coord_type x_2 = {}, y_2 = {}, z_2 = {};
    bool success_1 = (core::try_float_to_int(x_1, x_2) &&
                      core::try_float_to_int(y_1, y_2) &&
                      core::try_float_to_int(z_1, z_2));

    double x_3 = pos[0] - x_1;
    double y_3 = pos[1] - y_1;
    double z_3 = pos[2] - z_1;
    ARCHON_ASSERT(x_3 >= 0 && x_3 < 1);
    ARCHON_ASSERT(y_3 >= 0 && y_3 < 1);
    ARCHON_ASSERT(y_3 >= 0 && y_3 < 1);

    bool success_2 = (core::try_int_add(x_2, ref.x) &&
                      core::try_int_add(y_2, ref.y) &&
                      core::try_int_add(z_2, ref.z));

    if (ARCHON_UNLIKELY(!success_1 || !success_2))
        return false;

    static_assert(g_chunk_size_x > 0);
    static_assert(g_chunk_size_y > 0);
    static_assert(g_chunk_size_z > 0);

    block_coord_type x_4 = block_coord_type(x_2 / g_chunk_size_x);
    int x_5 = int(x_2 % g_chunk_size_x);
    block_coord_type y_4 = block_coord_type(y_2 / g_chunk_size_y);
    int y_5 = int(y_2 % g_chunk_size_y);
    block_coord_type z_4 = block_coord_type(z_2 / g_chunk_size_z);
    int z_5 = int(z_2 % g_chunk_size_z);

    if (x_5 < 0) {
        x_4 -= 1;
        x_5 += g_chunk_size_x;
    }
    if (y_5 < 0) {
        y_4 -= 1;
        y_5 += g_chunk_size_y;
    }
    if (z_5 < 0) {
        z_4 -= 1;
        z_5 += g_chunk_size_z;
    }

    m_current_chunk.x = x_4;
    m_current_chunk.y = y_4;
    m_current_chunk.z = z_4;

    m_position = {
        x_3 + double(x_5),
        y_3 + double(y_5),
        z_3 + double(z_5),
    };

    return true;
}



void world::render_avatar()
{
    
}



void world::render_opaque(const chunk& chunk)
{
    static_cast<void>(chunk);                 
}



double world::get_eye_height() const noexcept
{
    double normal_height = 1.62;
    double sneak_height  = 1.54;
    if (m_thrust_downwards)
        return sneak_height;
    return normal_height;
}


} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    render::Engine::Config engine_config;
    display::Size window_size = 512;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    std::optional<std::string> optional_display_implementation;

    cli::Spec spec;
    pat("", cli::no_attributes, spec,
        "Lorem ipsum.",
        std::tie()); // Throws

    opt(cli::help_tag, spec); // Throws
    opt(cli::stop_tag, spec); // Throws

    opt("-S, --window-size", "<size>", cli::no_attributes, spec,
        "Set the window size in number of pixels. \"@A\" can be specified either as a pair \"<width>,<height>\", or "
        "as a single value, which is then used as both width and height. The default size is @V.",
        cli::assign(window_size)); // Throws

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

    log::FileLogger root_logger(core::File::get_stderr(), locale); // Throws
    log::LimitLogger logger(root_logger, log_level_limit); // Throws

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
    std::unique_ptr<display::Connection> conn;
    if (ARCHON_UNLIKELY(!impl->try_new_connection(locale, connection_config, conn, error))) { // Throws
        logger.error("Failed to open display connection: %s", error); // Throws
        return EXIT_FAILURE;
    }

    engine_config.allow_window_resize = true;
    engine_config.logger = &logger;

    render::Engine engine;
    world world;

    if (ARCHON_UNLIKELY(!engine.try_create(world, *conn, "Archon Blocks", window_size, locale, engine_config,
                                           error))) { // Throws
        logger.error("Failed to create render engine: %s", error); // Throws
        return EXIT_FAILURE;
    }

    engine.run(); // Throws
}
