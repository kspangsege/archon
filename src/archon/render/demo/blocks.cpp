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
#include <vector>
#include <stack>
#include <unordered_map>
#include <locale>
#include <system_error>
#include <filesystem>

#include <archon/core/features.h>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/hash_fnv.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/rotation.hpp>
#include <archon/image.hpp>
#include <archon/display.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/load_texture.hpp>
#include <archon/render/engine.hpp>


using namespace archon;


namespace {

#if ARCHON_DISPLAY_HAVE_OPENGL


constexpr int g_chunk_size_x = 16;
constexpr int g_chunk_size_y = 16;
constexpr int g_chunk_size_z = 16;



using chunk_array_coord_type = std::int_fast8_t;
using block_coord_type = std::int_fast64_t;
using block_index_type = std::uint_least16_t;


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
        core::Hash_FNV_1a_Default hash;
        hash.add_int(pos.x);
        hash.add_int(pos.y);
        hash.add_int(pos.z);
        return std::size_t(hash.get());
    }
};



enum class box_face { left, right, bottom, top, back, front };


struct block {
    // One plus the index in m_block_variants of the last variant of this block. The index
    // of the first variant is `prev.variants_end`, where `prev` is the previous entry in
    // m_blocks, or zero if this is the first entry in m_blocks.
    std::size_t variants_end;
    bool full; // all six faces are aligned with the respective faces of the unit block
    bool solid; // No fully transparent texels
    bool opaque; // No semi-transparent texels
};


struct block_variant {
    // One plus the index in m_quads of the last quad of this block variant. The index of
    // the first quad is `prev.quads_end`, where `prev` is the previous entry in
    // m_block_variants, or zero if this is the first entry in m_block_variants.
    std::size_t quads_end;
};


// Spatial coordinates are in block units and relative to the origin corner of the block
struct quad {
    box_face orientation;
    GLuint texture;
    GLfloat s_1, t_1, x_1, y_1, z_1;
    GLfloat s_2, t_2, x_2, y_2, z_2;
    GLfloat s_3, t_3, x_3, y_3, z_3;
    GLfloat s_4, t_4, x_4, y_4, z_4;
    GLfloat n_x, n_y, n_z;
};



using block_array = block_index_type[g_chunk_size_z][g_chunk_size_y][g_chunk_size_z];
constexpr block_array g_empty_block_array = {};

inline auto get_block(const block_array& arr, int x, int y, int z) noexcept -> block_index_type
{
    return arr[z][y][x];
}

inline void set_block(block_array& arr, int x, int y, int z, block_index_type i) noexcept
{
    arr[z][y][x] = i;
}

void fill(block_array& arr, int x_1, int y_1, int z_1, int x_2, int y_2, int z_2, int i) noexcept
{
    for (int z = z_1; z < z_2; ++z) {
        for (int y = y_1; y < y_2; ++y) {
            for (int x = x_1; x < x_2; ++x)
                set_block(arr, x, y, z, i);
        }
    }
}

inline void fill(block_array& arr, block_index_type i) noexcept
{
    fill(arr, 0, 0, 0, g_chunk_size_x, g_chunk_size_y, g_chunk_size_z, i);
}



struct chunk {
    // FIXME: Consider not having the block array be a statical part of the chunk, but instead have it be allocated separately. This way, air chunks can take up much less memory    
    block_array blocks = {};
    const block_array* blocks_indir = &g_empty_block_array;

    chunk_pos pos = {};

    chunk* next_unused = nullptr;
    chunk* prev_unused = nullptr;

    // States                   | initialized | processed
    // -------------------------|-------------|-----------
    // Uninitialized            | false       | false
    // Initialized              | true        | false
    // Processed                | true        | true
    bool initialized = false;
    bool processed = false;

    bool init_in_progress = false; // Currently exposed to background thread
    bool linked = false; // Referenced from chunk array

    // After the chunk is processed, zero means that nothing is to be rendered during the
    // opaque stage for this chunk
    GLuint call_list = 0;
};



class world final
    : public render::Engine::Scene {
public:
    world(const std::filesystem::path& resource_path, const std::locale& locale);

    bool try_prepare(std::string&) override final;
    void render_init() override final;
    void set_projection(const math::Matrix4F&) override final;
    void render(const math::Matrix4F& view) override final;

private:
    const std::filesystem::path m_resource_path;
    const std::locale m_locale;

    bool m_thrust_forwards = false, m_thrust_backwards = false;
    bool m_thrust_leftwards = false, m_thrust_rightwards = false;
    bool m_thrust_upwards = false, m_thrust_downwards = false;
    bool m_sprint_mode = false;

    std::vector<block> m_blocks;
    std::vector<block_variant> m_block_variants;
    std::vector<quad> m_quads;

    std::unordered_map<chunk_pos, std::unique_ptr<chunk>, chunk_pos_hash> m_chunks;
    chunk* m_unused_chunks = nullptr; // First one is the one that became unused first
    std::size_t m_num_unused_chunks = 0;

    std::stack<GLuint> m_unused_call_lists;

    // Consider the chunk at `m_chunk_array[z][y][x]` to be at X, Y, and Z-coordinates, `x`,
    // `y`, and `z`.
    int m_chunk_array_size_x = {};
    int m_chunk_array_size_y = {};
    int m_chunk_array_size_z = {};
    std::unique_ptr<chunk*[]> m_chunk_array;

    // List of chunks that are within the currently selected render distance. Chunks occur
    // in an order that ensures back-to-front rendering. Chunk array positions are relative
    // to the center chunk (the one containing the player).
    std::vector<chunk_array_pos> m_chunk_order;

    chunk_pos m_current_chunk = { 0, 0, 0 };

    // Position of camera relative to origin of current chunk. Measured in texel units. One
    // texel unit is one 16th of the side length of a block.
    math::Vector3 m_position;

    // Velocity of camera. Measured in texel units.
    math::Vector3 m_velocity;

    void add_empty_block();
    bool try_add_block(std::string_view texture_path, std::string& error);

    // In number of blocks
    void change_render_distance(double horz_dist, double vert_dist);

    // In number of blocks
    void set_position(const math::Vector3& pos, const block_pos& ref = {});
    bool try_set_position(const math::Vector3& pos, const block_pos& ref = {}) noexcept;

    void render_avatar();

    // In meters above feet
    double get_eye_height() const noexcept;

    void process_chunk(chunk& cnk);

    // Position must be inside chunk array
    auto ensure_array_chunk(const chunk_array_pos& pos) -> chunk&;

    auto ensure_chunk(const chunk_pos& pos) -> chunk&;
    auto get_chunk(const chunk_pos& pos) noexcept -> chunk*;

    void mark_linked(chunk&) noexcept;
    void request_initialization(chunk&);
    void mark_initialized(chunk&);
    void on_changed(chunk&);
    void mark_dirty(chunk&);
    void add_unused(chunk&);
    void remove_unused(chunk&);

    auto alloc_call_list() -> GLuint;
    void return_call_list(GLuint list);
};



world::world(const std::filesystem::path& resource_path, const std::locale& locale)
    : m_resource_path(resource_path)
    , m_locale(locale)
{
}



bool world::try_prepare(std::string& error)
{
    add_empty_block(); // Throws
    if (ARCHON_UNLIKELY(!try_add_block("stone.png", error))) // Throws
        return false;
    if (ARCHON_UNLIKELY(!try_add_block("redstone_ore.png", error))) // Throws
        return false;
    return true;
}



void world::render_init()
{
    change_render_distance(64, 64); // Throws
    set_position({ 0, 0, 0 }); // Throws

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

    GLfloat params[4]  = { 0, 0, 0, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, params);
    glEnable(GL_LIGHT0);
}



void world::set_projection(const math::Matrix4F& proj)
{
    glMatrixMode(GL_PROJECTION);
    GLfloat value[16] = {};
    transpose(proj).to_array(value);
    glLoadMatrixf(value);
    glMatrixMode(GL_MODELVIEW);
}



void world::render(const math::Matrix4F& view)
{
    {
        GLfloat value[16] = {};
        transpose(view).to_array(value);
        glLoadMatrixf(value);
    }

    math::Vector3 eye_displacement = { 0, 0, 0 };
    eye_displacement[1] += get_eye_height();
    glTranslated(-eye_displacement[0], -eye_displacement[1], -eye_displacement[2]);
    render_avatar(); // Throws
    glTranslated(GLdouble(-m_position[0]), GLdouble(-m_position[1]), GLdouble(-m_position[2]));

    // Render entities
    

    // Render opaque blocks
//    m_transparent_chunks.clear();           
    for (chunk_array_pos pos : m_chunk_order) {
        // FIXME: Find a way to efficiently skip some of the chunnks that are definitely not intersecting the view frustum      

        chunk& cnk = ensure_array_chunk(pos); // Throws
        request_initialization(cnk); // Throws
        if (ARCHON_UNLIKELY(!cnk.initialized))
            continue;

        // FIXME: Formally require that each of the coordinates of a chunk array position can be represented in `int` as a number of blocks         
        GLfloat x = GLfloat(pos.x * g_chunk_size_x);
        GLfloat y = GLfloat(pos.y * g_chunk_size_y);
        GLfloat z = GLfloat(pos.z * g_chunk_size_z);
        glPushMatrix();
        glTranslatef(x, y, z);
        if (ARCHON_LIKELY(cnk.processed)) {
            if (cnk.call_list != 0)
                glCallList(cnk.call_list);
        }
        else {
            process_chunk(cnk); // Throws
        }
        glPopMatrix();

        // FIXME: If chunk has semi-transparent texels: m_transparent_chunks.push_back(cnk); // Throws    
    }

    // Render transparent blocks
    

    // Render HUD
    

    // Rendering proceeds in three stages:
    //
    //   1. Render entities. These cannot make use of transparent or semi-transaprent textures. This is done with depth test and writing to depth buffer.
    //
    //   2. Render the opaque parts of chunks. This can be done in any chunk order, and make use of a precomputed call list for each chunk. This is done with depth test and writing to depth buffer.
    //
    //   3. Rendering of the surfaces with transparent textures. This is done chunk by chunk in order of closeness to the player (farthest first). For each chunk that has textured surfaces with transparent features, render those in order of closenes to the player. Achieve this by way of a tree (bsp, octree, ...)
    //
    // In order to facilitate the handling of chunks in the right order (farthest to closest), a list of chunk grid positions must be precomputed for a particular rendering distance. Chunks are then visited in this order in step 2, where each chunk with transparency is appended to a list of chunks to be processed in step 3.

    // FIXME: Consider blending parameters to allow for textures with holes as opposed to textures with semitransparent texels      

    // How to arrange the transparent quads in each chunk to allow for fast ordered traversal? --> kd-tree-ish over block positions

    // FIXME: When position is changed: Clear rolled-over part of chunk grid
}



void world::add_empty_block()
{
    std::size_t quads_end = m_quads.size();
    block_variant var = { quads_end };
    m_block_variants.push_back(var); // Throws
    std::size_t variants_end = m_block_variants.size();
    bool full = false;
    bool solid = false;
    bool opaque = false;
    block blk = {
        variants_end,
        full,
        solid,
        opaque,
    };
    m_blocks.push_back(blk); // Throws
}



bool world::try_add_block(std::string_view texture_path, std::string& error)
{
    namespace fs = std::filesystem;
    fs::path texture_path_2 = (m_resource_path / core::make_fs_path_generic(texture_path, m_locale)); // Throws
    std::unique_ptr<image::WritableImage> texture_image;
    image::LoadConfig config;
    std::error_code ec;
    if (ARCHON_UNLIKELY(!image::try_load(texture_path_2, texture_image, m_locale, config, ec))) { // Throws
        error = core::format("Failed to load image %s: %s", texture_path_2, ec.message()); // Throws
        return false;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    bool no_interp = true;
    bool no_mipmap = false;
    render::load_and_configure_texture(*texture_image, no_interp, no_mipmap); // Throws

    auto add_quad = [&](box_face orientation) {
        GLfloat s_1 = 0, t_1 = 0, x_1 = 0, y_1 = 0, z_1 = 0;
        GLfloat s_2 = 1, t_2 = 0, x_2 = 0, y_2 = 0, z_2 = 0;
        GLfloat s_3 = 1, t_3 = 1, x_3 = 0, y_3 = 0, z_3 = 0;
        GLfloat s_4 = 0, t_4 = 1, x_4 = 0, y_4 = 0, z_4 = 0;
        GLfloat n_x = 0, n_y = 0, n_z = 0;
        switch (orientation) {
            case box_face::left:
                x_1 = 0, y_1 = 0, z_1 = 0;
                x_2 = 0, y_2 = 0, z_2 = 1;
                x_3 = 0, y_3 = 1, z_3 = 1;
                x_4 = 0, y_4 = 1, z_4 = 0;
                n_x = -1;
                break;
            case box_face::right:
                x_1 = 1, y_1 = 0, z_1 = 1;
                x_2 = 1, y_2 = 0, z_2 = 0;
                x_3 = 1, y_3 = 1, z_3 = 0;
                x_4 = 1, y_4 = 1, z_4 = 1;
                n_x = 1;
                break;
            case box_face::bottom:
                x_1 = 0, y_1 = 0, z_1 = 0;
                x_2 = 1, y_2 = 0, z_2 = 0;
                x_3 = 1, y_3 = 0, z_3 = 1;
                x_4 = 0, y_4 = 0, z_4 = 1;
                n_y = -1;
                break;
            case box_face::top:
                x_1 = 0, y_1 = 1, z_1 = 1;
                x_2 = 1, y_2 = 1, z_2 = 1;
                x_3 = 1, y_3 = 1, z_3 = 0;
                x_4 = 0, y_4 = 1, z_4 = 0;
                n_y = 1;
                break;
            case box_face::back:
                x_1 = 1, y_1 = 0, z_1 = 0;
                x_2 = 0, y_2 = 0, z_2 = 0;
                x_3 = 0, y_3 = 1, z_3 = 0;
                x_4 = 1, y_4 = 1, z_4 = 0;
                n_z = -1;
                break;
            case box_face::front:
                x_1 = 0, y_1 = 0, z_1 = 1;
                x_2 = 1, y_2 = 0, z_2 = 1;
                x_3 = 1, y_3 = 1, z_3 = 1;
                x_4 = 0, y_4 = 1, z_4 = 1;
                n_z = 1;
                break;
        }
        quad q = {
            orientation,
            texture,
            s_1, t_1, x_1, y_1, z_1,
            s_2, t_2, x_2, y_2, z_2,
            s_3, t_3, x_3, y_3, z_3,
            s_4, t_4, x_4, y_4, z_4,
            n_x, n_y, n_z,
        };
        m_quads.push_back(q); // Throws
    };
    add_quad(box_face::left); // Throws
    add_quad(box_face::right); // Throws
    add_quad(box_face::bottom); // Throws
    add_quad(box_face::top); // Throws
    add_quad(box_face::back); // Throws
    add_quad(box_face::front); // Throws
    std::size_t quads_end = m_quads.size();
    block_variant var = { quads_end };
    m_block_variants.push_back(var); // Throws
    std::size_t variants_end = m_block_variants.size();
    bool full = true;
    bool solid = true;
    bool opaque = true;
    block blk = {
        variants_end,
        full,
        solid,
        opaque,
    };
    m_blocks.push_back(blk); // Throws

    return true;
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



double world::get_eye_height() const noexcept
{
    double normal_height = 1.62;
    double sneak_height  = 1.54;
    if (m_thrust_downwards)
        return sneak_height;
    return normal_height;
}



void world::process_chunk(chunk& cnk)
{
    ARCHON_ASSERT(!cnk.processed);
    auto neighbor = [&](int x, int y, int z) -> const chunk& {
        chunk_pos pos_2 = {
            block_coord_type(cnk.pos.x + x),
            block_coord_type(cnk.pos.y + y),
            block_coord_type(cnk.pos.z + z),
        };
        chunk& cnk_2 = ensure_chunk(pos_2); // Throws
        request_initialization(cnk_2); // Throws
        return cnk_2;
    };
    auto get_block = [](const chunk& cnk, int x, int y, int z) noexcept {
        const block_array& blocks = *cnk.blocks_indir;
        return blocks[z][y][x];
    };
    const chunk& left   = neighbor(-1, 0, 0); // Throws
    const chunk& right  = neighbor(+1, 0, 0); // Throws
    const chunk& bottom = neighbor(0, -1, 0); // Throws
    const chunk& top    = neighbor(0, +1, 0); // Throws
    const chunk& back   = neighbor(0, 0, -1); // Throws
    const chunk& front  = neighbor(0, 0, +1); // Throws
    GLuint call_list = 0;
    GLuint texture = 0;
    std::size_t serial = 0;
    auto pick_variant = [&](std::size_t n) {
        core::Hash_FNV_1a_Default hash;
        hash.add_obj(cnk.pos);
        hash.add_obj(serial);
        return std::size_t(hash.get() % n);
    };
    for (int z = 0; z < g_chunk_size_z; ++z) {
        for (int y = 0; y < g_chunk_size_y; ++y) {
            for (int x = 0; x < g_chunk_size_x; ++x, ++serial) {
                block_index_type i = get_block(cnk, x, y, z);
                GLfloat x_2 = x;
                GLfloat y_2 = y;
                GLfloat z_2 = z;
                ARCHON_ASSERT(i < m_blocks.size());
                const block& blk = m_blocks[i];
                std::size_t variants_begin = (i == 0 ? 0 : m_blocks[i - 1].variants_end);
                std::size_t variants_end = blk.variants_end;
                std::size_t num_variants = std::size_t(variants_end - variants_begin);
                ARCHON_ASSERT(num_variants >= 1);
                std::size_t j = variants_begin;
                if (num_variants > 1)
                    j = std::size_t(variants_begin + pick_variant(num_variants));
                const block_variant& var = m_block_variants[j];
                std::size_t quads_begin = (j == 0 ? 0 : m_block_variants[j - 1].quads_end);
                std::size_t quads_end = var.quads_end;
                for (std::size_t k = quads_begin; k < quads_end; ++k) {
                    const quad& q = m_quads[k];
                    block_index_type i_2 = {};
                    if (blk.full) {
                        switch (q.orientation) {
                            case box_face::left:
                                if (ARCHON_LIKELY(x != 0)) {
                                    i_2 = get_block(cnk, x - 1, y, z);
                                }
                                else {
                                    i_2 = get_block(left, g_chunk_size_x - 1, y, z);
                                }
                                break;
                            case box_face::right:
                                if (ARCHON_LIKELY(x != g_chunk_size_x - 1)) {
                                    i_2 = get_block(cnk, x + 1, y, z);
                                }
                                else {
                                    i_2 = get_block(right, 0, y, z);
                                }
                                break;
                            case box_face::bottom:
                                if (ARCHON_LIKELY(y != 0)) {
                                    i_2 = get_block(cnk, x, y - 1, z);
                                }
                                else {
                                    i_2 = get_block(bottom, x, g_chunk_size_y - 1, z);
                                }
                                break;
                            case box_face::top:
                                if (ARCHON_LIKELY(y != g_chunk_size_y - 1)) {
                                    i_2 = get_block(cnk, x, y + 1, z);
                                }
                                else {
                                    i_2 = get_block(top, x, 0, z);
                                }
                                break;
                            case box_face::back:
                                if (ARCHON_LIKELY(z != 0)) {
                                    i_2 = get_block(cnk, x, y, z - 1);
                                }
                                else {
                                    i_2 = get_block(back, x, y, g_chunk_size_z - 1);
                                }
                                break;
                            case box_face::front:
                                if (ARCHON_LIKELY(z != g_chunk_size_z - 1)) {
                                    i_2 = get_block(cnk, x, y, z + 1);
                                }
                                else {
                                    i_2 = get_block(front, x, y, 0);
                                }
                                break;
                        }
                        const block& blk_2 = m_blocks[i_2];
                        bool elide = (blk_2.full && blk_2.solid && (blk_2.opaque || i_2 == i));
                        if (ARCHON_LIKELY(elide))
                            continue;
                    }
                    if (ARCHON_UNLIKELY(call_list == 0)) {
                        call_list = alloc_call_list(); // Throws
                        glNewList(call_list, GL_COMPILE_AND_EXECUTE);
                        texture = q.texture;
                        glBindTexture(GL_TEXTURE_2D, texture);
                        glBegin(GL_QUADS);
                    }
                    else if (ARCHON_UNLIKELY(q.texture != texture)) {
                        glEnd();
                        texture = q.texture;
                        glBindTexture(GL_TEXTURE_2D, texture);
                        glBegin(GL_QUADS);
                    }
                    glNormal3f(q.n_x, q.n_y, q.n_z);
                    glTexCoord2f(q.s_1, q.t_1);
                    glVertex3f(x_2 + q.x_1, y_2 + q.y_1, z_2 + q.z_1);
                    glTexCoord2f(q.s_2, q.t_2);
                    glVertex3f(x_2 + q.x_2, y_2 + q.y_2, z_2 + q.z_2);
                    glTexCoord2f(q.s_3, q.t_3);
                    glVertex3f(x_2 + q.x_3, y_2 + q.y_3, z_2 + q.z_3);
                    glTexCoord2f(q.s_4, q.t_4);
                    glVertex3f(x_2 + q.x_4, y_2 + q.y_4, z_2 + q.z_4);
                }
            }
        }
    }
    if (call_list != 0) {
        glEnd();
        glEndList();
    }
    cnk.call_list = call_list;
    cnk.processed = true;
}



auto world::ensure_array_chunk(const chunk_array_pos& pos) -> chunk&
{
    // FIXME: Maybe verify that position is inside logical array boundary          
    chunk_pos pos_2 = {
        block_coord_type(m_current_chunk.x + pos.x),
        block_coord_type(m_current_chunk.y + pos.y),
        block_coord_type(m_current_chunk.z + pos.z),
    };
    int x = core::int_periodic_mod(pos_2.x,  m_chunk_array_size_x);
    int y = core::int_periodic_mod(pos_2.y,  m_chunk_array_size_y);
    int z = core::int_periodic_mod(pos_2.z,  m_chunk_array_size_z);
    std::size_t i = std::size_t((std::size_t(z) * m_chunk_array_size_y + y) * m_chunk_array_size_x + x);
    chunk*& cnk = m_chunk_array[i];
    if (ARCHON_UNLIKELY(!cnk)) {
        chunk& cnk_2 = ensure_chunk(pos_2); // Throws
        mark_linked(cnk_2);
        cnk = &cnk_2;
    }
    return *cnk;
}



// FIXME: For the sake of efficiency, consider a ensure_linked_chunk(const chunk_pos& pos) that delivers the chunk in the linked state          
auto world::ensure_chunk(const chunk_pos& pos) -> chunk&
{
    std::unique_ptr<chunk>& cnk = m_chunks[pos]; // Throws
    if (ARCHON_UNLIKELY(!cnk)) {
        // FIXME: If current chunk cache size is now over its soft limit and there are reclaimable chunks, reclaim the least recently used reclaimable chunk. A chunk is reclaimable if it is neither linked from the chunk array nor locked during exposure to background thread       
        cnk = std::make_unique<chunk>(); // Throws
        cnk->pos = pos;
        add_unused(*cnk);
    }
    return *cnk;
}



auto world::get_chunk(const chunk_pos& pos) noexcept -> chunk*
{
    auto i = m_chunks.find(pos);
    if (ARCHON_LIKELY(i != m_chunks.end())) {
        const std::unique_ptr<chunk>& cnk = i->second;
        return cnk.get();
    }
    return nullptr;
}



void world::mark_linked(chunk& cnk) noexcept
{
    ARCHON_ASSERT(!cnk.linked);
    bool was_unused = !cnk.init_in_progress;
    if (was_unused)
        remove_unused(cnk);
    cnk.linked = true;
}



void world::request_initialization(chunk& cnk)
{
    if (ARCHON_LIKELY(cnk.initialized || cnk.init_in_progress))
        return;
    // FIXME: Do not generally initialize chunk here, but instead initiate a request for initialization to the background thread    
    // FIXME: Explain why this cannot overflow   
    block_pos pos = {
        block_coord_type(cnk.pos.x * g_chunk_size_x),
        block_coord_type(cnk.pos.y * g_chunk_size_y),
        block_coord_type(cnk.pos.z * g_chunk_size_z),
    };
    using hash_type = core::Hash_FNV_1a_Default;
    hash_type hash;
    hash.add_obj(pos);
    for (int z = 0; z < g_chunk_size_z; ++z) {
        hast_type hash_z = hash;
        hash_z.add_int(z);
        for (int y = 0; y < g_chunk_size_y; ++y) {
            hast_type hash_y = hash_z;
            hash_y.add_int(y);
            for (int x = 0; x < g_chunk_size_x; ++x) {
                hast_type hash_x = hash_y;
                hash_x.add_int(x);
                
                block_coord_type y_2 = block_coord_type(pos.y + y); // FIXME: What guarantees that this does not overflow?    
                block_index_type air   = 0;
                block_index_type stone = 1;
                block_index_type redstone_ore = 2;
                block_index_type i = (y_2 < 0 ? stone : air);
                set_block(cnk.blocks, x, y, z, i);
            }
        }
    }
    mark_initialized(cnk); // Throws
}



void world::mark_initialized(chunk& cnk)
{
    ARCHON_ASSERT(!cnk.initialized);
    ARCHON_ASSERT(!cnk.processed);
    if (cnk.init_in_progress) {
        cnk.init_in_progress = false;
        bool is_unused = !cnk.linked;
        if (is_unused)
            add_unused(cnk);
    }
    cnk.blocks_indir = &cnk.blocks;
    cnk.initialized = true;
    on_changed(cnk); // Throws
}



void world::on_changed(chunk& cnk)
{
    ARCHON_ASSERT(cnk.initialized);
    mark_dirty(cnk); // Throws
    auto neighbor = [&](int x, int y, int z) {
        chunk_pos pos_2 = {
            block_coord_type(cnk.pos.x + x),
            block_coord_type(cnk.pos.y + y),
            block_coord_type(cnk.pos.z + z),
        };
        if (chunk* cnk_2 = get_chunk(pos_2))
            mark_dirty(*cnk_2); // Throws
    };
    neighbor(-1, 0, 0); // Throws
    neighbor(+1, 0, 0); // Throws
    neighbor(0, -1, 0); // Throws
    neighbor(0, +1, 0); // Throws
    neighbor(0, 0, -1); // Throws
    neighbor(0, 0, +1); // Throws
}



void world::mark_dirty(chunk& cnk)
{
    if (cnk.call_list != 0) {
        return_call_list(cnk.call_list); // Throws
        cnk.call_list = 0;
    }
    cnk.processed = false;
}



void world::add_unused(chunk& cnk)
{
    ARCHON_ASSERT(!cnk.prev_unused);
    ARCHON_ASSERT(!cnk.next_unused);
    if (ARCHON_LIKELY(m_unused_chunks)) {
        chunk* next = m_unused_chunks->next_unused;
        cnk.prev_unused = m_unused_chunks;
        cnk.next_unused = next;
        next->prev_unused = &cnk;
        m_unused_chunks->next_unused = &cnk;
    }
    else {
        cnk.prev_unused = &cnk;
        cnk.next_unused = &cnk;
        m_unused_chunks = &cnk;
    }
    m_num_unused_chunks += 1;
}



void world::remove_unused(chunk& cnk)
{
    ARCHON_ASSERT(cnk.prev_unused);
    ARCHON_ASSERT(cnk.next_unused);
    ARCHON_ASSERT(m_num_unused_chunks > 0);
    if (ARCHON_LIKELY(m_num_unused_chunks > 1)) {
        chunk* prev = m_unused_chunks->prev_unused;
        chunk* next = m_unused_chunks->next_unused;
        if (m_unused_chunks == &cnk)
            m_unused_chunks = next;
        prev->next_unused = next;
        next->prev_unused = prev;
    }
    else {
        m_unused_chunks = nullptr;
    }
    cnk.prev_unused = nullptr;
    cnk.next_unused = nullptr;
    m_num_unused_chunks -= 1;
}



auto world::alloc_call_list() -> GLuint
{
    if (ARCHON_UNLIKELY(m_unused_call_lists.empty())) {
        constexpr int n = 64;
        GLuint offset = glGenLists(n);
        if (ARCHON_UNLIKELY(offset == 0))
            throw std::runtime_error("Call list allocation failed");
        for (int i = 0; i < n; ++i)
            m_unused_call_lists.push(GLuint(offset + i));
    }
    GLuint list = m_unused_call_lists.top();
    m_unused_call_lists.pop();
    return list;
}



inline void world::return_call_list(GLuint list)
{
    m_unused_call_lists.push(list); // Throws
}


#endif // ARCHON_DISPLAY_HAVE_OPENGL

} // unnamed namespace



int main(int argc, char* argv[])
{
    std::locale locale = core::get_default_locale(); // Throws

    bool list_display_implementations = false;
    render::Engine::Config engine_config;
    display::Size window_size = 512;
    log::LogLevel log_level_limit = log::LogLevel::warn;
    std::optional<std::string> optional_display_implementation;
    std::optional<int> optional_screen;
    std::optional<std::string> optional_x11_display;
    std::optional<display::x11_fullscreen_monitors> optional_x11_fullscreen_monitors;

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
    build_env_params.bin_path  = "archon/render/demo/archon-blocks";
    build_env_params.src_path  = "archon/render/demo/blocks.cpp";
    build_env_params.src_root  = "src";
    build_env_params.source_from_build_path = core::archon_source_from_build_path;
    core::BuildEnvironment build_env = core::BuildEnvironment(argv[0], build_env_params, locale); // Throws

    namespace fs = std::filesystem;
    fs::path resource_path = (build_env.get_relative_source_root() /
                              core::make_fs_path_generic("archon/render/demo", locale)); // Throws

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
    connection_config.x11.fullscreen_monitors = optional_x11_fullscreen_monitors;
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

#if ARCHON_DISPLAY_HAVE_OPENGL

    render::Engine engine;
    world world(resource_path, locale); // Throws
    if (ARCHON_UNLIKELY(!engine.try_create(world, *conn, "Archon Blocks", window_size, locale, engine_config,
                                           error))) { // Throws
        logger.error("Failed to create render engine: %s", error); // Throws
        return EXIT_FAILURE;
    }
    engine.run(); // Throws

#else // !ARCHON_DISPLAY_HAVE_OPENGL

    static_cast<void>(engine_config);
    logger.error("OpenGL rendering not available"); // Throws
    return EXIT_FAILURE;

#endif // !ARCHON_DISPLAY_HAVE_OPENGL
}
