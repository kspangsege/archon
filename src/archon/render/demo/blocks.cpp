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
#include <type_traits>
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
#include <mutex>
#include <condition_variable>

#include <archon/core/features.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/scope_exit.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/float.hpp>
#include <archon/core/math.hpp>
#include <archon/core/hash_fnv.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/value_parser.hpp>
#include <archon/core/format.hpp>
#include <archon/core/as_int.hpp>
#include <archon/core/build_environment.hpp>
#include <archon/core/thread_guard.hpp>
#include <archon/core/file.hpp>
#include <archon/log.hpp>
#include <archon/cli.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/matrix.hpp>
#include <archon/math/rotation.hpp>
#include <archon/image.hpp>
#include <archon/image/file_format_png.hpp>
#include <archon/gfx/math.hpp>
#include <archon/display.hpp>
#include <archon/display/x11_fullscreen_monitors.hpp>
#include <archon/display/x11_connection_config.hpp>
#include <archon/display/opengl.hpp>
#include <archon/render/opengl.hpp>
#include <archon/render/load_texture.hpp>
#include <archon/render/engine.hpp>


using namespace archon;


namespace {


template<class T> class data_pipe {
public:
    data_pipe();

    void add(T val);

    // Must be called with lock on protecting mutex
    void push();

    // Must be called with lock on protecting mutex
    void pull();

    template<class F> void extract(std::size_t n, F&& func);

private:
    static constexpr std::size_t s_buffer_size = 16384;

    std::vector<std::unique_ptr<T[]>> m_buffers;

    std::vector<T*> m_shared_empty_buffers;
    std::vector<T*> m_shared_full_buffers;
    std::vector<T*> m_writers_empty_buffers;
    std::vector<T*> m_writers_full_buffers;
    std::vector<T*> m_readers_empty_buffers;
    core::Deque<T*> m_readers_nonempty_buffers;

    T* m_shared_buffer = nullptr;
    T* m_writers_buffer = nullptr;
    T* m_readers_last_buffer = nullptr;

    std::size_t m_shared_offset = 0;
    std::size_t m_writers_offset = 0;
    std::size_t m_readers_last_offset = 0;
    std::size_t m_readers_offset = 0;

    auto add_buffer() -> T*;
};


template<class T>
data_pipe<T>::data_pipe()
{
    T* buffer = add_buffer(); // Throws
    m_shared_buffer = buffer;
    m_writers_buffer = buffer;
    m_readers_last_buffer = buffer;
}


template<class T>
void data_pipe<T>::add(T val)
{
    ARCHON_ASSERT(m_writers_offset < s_buffer_size);
    m_writers_buffer[m_writers_offset] = val;
    bool full = (s_buffer_size - m_writers_offset == 1);
    if (ARCHON_LIKELY(!full)) {
        m_writers_offset += 1;
        return;
    }
    if (ARCHON_UNLIKELY(m_writers_empty_buffers.empty())) {
        m_writers_empty_buffers.reserve(m_writers_empty_buffers.size() + 1); // Throws
        T* buffer = add_buffer(); // Throws
        m_writers_empty_buffers.push_back(buffer);
    }
    m_writers_full_buffers.push_back(m_writers_buffer); // Throws
    m_writers_buffer = m_writers_empty_buffers.back();
    m_writers_empty_buffers.pop_back();
    m_writers_offset = 0;
}


template<class T>
void data_pipe<T>::push()
{
    m_writers_empty_buffers.insert(m_writers_empty_buffers.end(), m_shared_empty_buffers.begin(),
                                   m_shared_empty_buffers.end()); // Throws
    m_shared_empty_buffers.clear();
    m_shared_full_buffers.insert(m_shared_full_buffers.end(), m_writers_full_buffers.begin(),
                                 m_writers_full_buffers.end()); // Throws
    m_writers_full_buffers.clear();
    m_shared_buffer = m_writers_buffer;
    m_shared_offset = m_writers_offset;
}


template<class T>
void data_pipe<T>::pull()
{
    m_readers_nonempty_buffers.append(m_shared_full_buffers.begin(), m_shared_full_buffers.end()); // Throws
    m_shared_full_buffers.clear();
    m_shared_empty_buffers.insert(m_shared_empty_buffers.end(), m_readers_empty_buffers.begin(),
                                  m_readers_empty_buffers.end()); // Throws
    m_readers_empty_buffers.clear();
    m_readers_last_buffer = m_shared_buffer;
    m_readers_last_offset = m_shared_offset;
}


template<class T>
template<class F> void data_pipe<T>::extract(std::size_t n, F&& func)
{
    std::size_t offset = 0;
    while (offset < n) {
        T* buffer = m_readers_last_buffer;
        ARCHON_ASSERT(buffer);
        std::size_t n_2 = std::size_t(n - offset);
        bool becomes_empty = false;
        if (ARCHON_LIKELY(m_readers_nonempty_buffers.empty())) {
            buffer = m_readers_last_buffer;
            std::size_t avail = std::size_t(m_readers_last_offset - m_readers_offset);
            ARCHON_ASSERT(avail >= n_2);
        }
        else {
            buffer = m_readers_nonempty_buffers.front();
            std::size_t avail = std::size_t(s_buffer_size - m_readers_offset);
            ARCHON_ASSERT(avail > 0);
            if (ARCHON_UNLIKELY(n_2 >= avail)) {
                becomes_empty = true;
                std::size_t size = std::size_t(m_readers_empty_buffers.size() + 1);
                m_readers_empty_buffers.reserve(size); // Throws
                n_2 = avail;
            }
        }
        core::Span span = { buffer + m_readers_offset, n_2 };
        func(offset, span); // Throws
        if (ARCHON_LIKELY(!becomes_empty)) {
            m_readers_offset += n_2;
        }
        else {
            m_readers_nonempty_buffers.pop_front();
            m_readers_empty_buffers.push_back(buffer);
            m_readers_offset = 0;
        }
        offset += n_2;
    }
}


template<class T>
auto data_pipe<T>::add_buffer() -> T*
{
    std::unique_ptr<T[]> buffer = std::make_unique<T[]>(s_buffer_size); // Throws
    T* buffer_2 = buffer.get();
    m_buffers.push_back(std::move(buffer)); // Throws
    return buffer_2;
}



// If `max` is 2^63 - 1 and M is `max - g_world_border_margin` , then each of the
// coordinates of the player's position (X, Y, and Z), considered as a real numbers, must be
// no less than -M and no greater than M.
//
// This margin allows for rendering of chunks around the player without risking numeric
// overflow. See world::change_render_distance() and ...                                                                          
//
constexpr long g_world_border_margin = 32767;



struct block_basis_spec {
    struct texture {
        std::string_view path;
    };
    struct block {
        std::size_t texture;
    };
    std::vector<texture> textures;
    std::vector<block> blocks;
};


struct gl_vertex_array {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
};



constexpr int g_chunk_size_x = 16;
constexpr int g_chunk_size_y = 16;
constexpr int g_chunk_size_z = 16;



using block_coord_type = std::int_fast64_t;
using chunk_coord_type = block_coord_type;
using block_index_type = std::uint_least16_t;


struct rel_block_pos {
    int x, y, z;

    constexpr auto operator+(const rel_block_pos& other) const noexcept -> rel_block_pos;
    constexpr auto operator+=(const rel_block_pos& other) noexcept -> rel_block_pos&;
};

constexpr auto rel_block_pos::operator+(const rel_block_pos& other) const noexcept -> rel_block_pos
{
    return { x + other.x, y + other.y, z + other.z };
}

constexpr auto rel_block_pos::operator+=(const rel_block_pos& other) noexcept -> rel_block_pos&
{
    *this = *this + other;
    return *this;
}



struct rel_chunk_pos {
    using comp_type = std::int_fast8_t;
    comp_type x, y, z;
};



struct block_pos {
    block_coord_type x, y, z;

    auto operator+(const rel_block_pos& other) const noexcept -> block_pos;
};

inline auto block_pos::operator+(const rel_block_pos& other) const noexcept -> block_pos
{
    return {
        block_coord_type(x + other.x),
        block_coord_type(y + other.y),
        block_coord_type(z + other.z),
    };
}



struct chunk_pos {
    chunk_coord_type x, y, z;

    constexpr auto operator<=>(const chunk_pos& other) const noexcept = default;

    auto operator+(const rel_chunk_pos& other) const noexcept -> chunk_pos;
};

inline auto chunk_pos::operator+(const rel_chunk_pos& other) const noexcept -> chunk_pos
{
    return {
        chunk_coord_type(x + other.x),
        chunk_coord_type(y + other.y),
        chunk_coord_type(z + other.z),
    };
}



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



enum class face_orientation { left, right, bottom, top, back, front };



struct directional_info {
    GLfloat light_level;
    rel_block_pos opposing_position;
    rel_block_pos lateral_positions[8];
};


constexpr directional_info g_direction_west = {
    0.6,
    { -1, 0, 0 },
    {
        {  0,  0, -1 },
        {  0, -1, -1 },
        {  0, -1,  0 },
        {  0, -1,  1 },
        {  0,  0,  1 },
        {  0,  1,  1 },
        {  0,  1,  0 },
        {  0,  1, -1 },
    },
};

constexpr directional_info g_direction_east = {
    0.6,
    { +1, 0, 0 },
    {
        {  0,  0,  1 },
        {  0, -1,  1 },
        {  0, -1,  0 },
        {  0, -1, -1 },
        {  0,  0, -1 },
        {  0,  1, -1 },
        {  0,  1,  0 },
        {  0,  1,  1 },
    },
};

constexpr directional_info g_direction_down = {
    0.5,
    { 0, -1, 0 },
    {
        { -1,  0,  0 },
        { -1,  0, -1 },
        {  0,  0, -1 },
        {  1,  0, -1 },
        {  1,  0,  0 },
        {  1,  0,  1 },
        {  0,  0,  1 },
        { -1,  0,  1 },
    },
};

constexpr directional_info g_direction_up = {
    1.0,
    { 0, +1, 0 },
    {
        { -1,  0,  0 },
        { -1,  0,  1 },
        {  0,  0,  1 },
        {  1,  0,  1 },
        {  1,  0,  0 },
        {  1,  0, -1 },
        {  0,  0, -1 },
        { -1,  0, -1 },
    },
};

constexpr directional_info g_direction_north = {
    0.8,
    { 0, 0, -1 },
    {
        {  1,  0,  0 },
        {  1, -1,  0 },
        {  0, -1,  0 },
        { -1, -1,  0 },
        { -1,  0,  0 },
        { -1,  1,  0 },
        {  0,  1,  0 },
        {  1,  1,  0 },
    },
};

constexpr directional_info g_direction_south = {
    0.8,
    { 0, 0, +1 },
    {
        { -1,  0,  0 },
        { -1, -1,  0 },
        {  0, -1,  0 },
        {  1, -1,  0 },
        {  1,  0,  0 },
        {  1,  1,  0 },
        {  0,  1,  0 },
        { -1,  1,  0 },
    },
};



struct block_basis {
    struct block;
    struct block_variant;
    struct quad;

    std::vector<block> blocks;
    std::vector<block_variant> block_variants;
    std::vector<quad> quads;
};


struct block_basis::block {
    // One plus the index in block_basis::block_variants of the last variant of this
    // block. The index of the first variant is `prev.variants_end`, where `prev` is the
    // previous entry in block_basis::blocks, or zero if this is the first entry in
    // block_basis::blocks.
    std::size_t variants_end;
    bool full; // all six faces are coplanar with the respective faces of the block space
    bool solid; // No fully transparent texels
    bool opaque; // No semi-transparent texels
};


struct block_basis::block_variant {
    // One plus the index in block_basis::quads of the last quad of this block variant. The
    // index of the first quad is `prev.quads_end`, where `prev` is the previous entry in
    // block_basis::block_variants, or zero if this is the first entry in
    // block_basis::block_variants.
    std::size_t quads_end;
};


// Spatial coordinates are in block units and relative to the origin corner of the block.
//
//     Face   | First vertex |
//     -------|--------------|
//     left   | bottom-back  |
//     right  | bottom-front |
//     bottom | back-left    |
//     top    | front-left   |
//     back   | bottom-right |
//     front  | bottom-left  |
//
struct block_basis::quad {
    const directional_info* dir;
    GLfloat texture;
    GLfloat x_1, y_1, z_1, s_1, t_1;
    GLfloat x_2, y_2, z_2, s_2, t_2;
    GLfloat x_3, y_3, z_3, s_3, t_3;
    GLfloat x_4, y_4, z_4, s_4, t_4;
};



using block_array = block_index_type[g_chunk_size_z][g_chunk_size_y][g_chunk_size_z];
// constexpr block_array g_empty_block_array = {};    

inline auto get_block(const block_array& arr, const rel_block_pos& pos) noexcept -> block_index_type
{
    return arr[pos.z][pos.y][pos.x];
}

inline void set_block(block_array& arr, int x, int y, int z, block_index_type i) noexcept
{
    arr[z][y][x] = i;
}

/*    
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
*/



struct chunk {
    ::block_array blocks = {};

    ::chunk_pos pos = {};

    chunk* next_unreferenced = nullptr;
    chunk* prev_unreferenced = nullptr;

    using reference_count_type = std::uint_least8_t;
    reference_count_type reference_count = 0;

    // True while the chunk is in the subscription from teh worker thread's point of
    // view. It is possible for a chunk update to be received and processed by the rendering
    // thread after the chunk has been removed from the subscription.
    //
    bool in_subscription = false;

    // True while the chunk is in the chunk processing queue.
    //
    bool update_requested = false;

    // Set true after the chunk has been processed, and remains true until the chunk's block
    // contents is modified.
    //
    bool processed = false;

    // While `reference_count` is nonzero, `num_indices` and `vertex_array` may be accessed
    // by the rendering thread, and only by the rendering thread.
    //
    // When `vertex_array.vao` is zero, it means that a vertex array (VAO, VBO, EBO) is not
    // currently allocated for this chunk.
    //
    // When `num_indices` is greater than zero, `vertex_array.vao` must be nonzero.
    //
    GLsizei num_indices = 0;
    ::gl_vertex_array vertex_array = {};
};



struct chunk_update_size {
    std::size_t num_vertex_components;
    std::size_t num_vertex_indices;
};

struct chunk_update {
    ::chunk* chunk;
    bool subscribing;
    bool has_data;
    ::chunk_update_size data_size;
};



const char* g_vertex_shader_source = R"(
    #version 410 core

    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aTexCoord;
    layout (location = 2) in float aLightLevel;

    out vec3 vTexCoord;
    out float vLightLevel;

    uniform mat4 uProjectionMatrix;
    uniform mat4 uViewMatrix;
    uniform mat4 uModelMatrix;

    void main() {
        gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPos, 1.0);
        vTexCoord = aTexCoord;
        vLightLevel = aLightLevel;
    }
)";

const char* g_fragment_shader_source = R"(
    #version 410 core

    in vec3 vTexCoord;
    in float vLightLevel;

    out vec4 FragColor;

    uniform sampler2DArray uTextureArray;

    void main() {
        FragColor = vLightLevel * texture(uTextureArray, vTexCoord);
    }
)";



class worker {
public:
    worker(log::Logger&, const ::block_basis&) noexcept;

    void run();
    void quit() noexcept;

    // `chunk_updates` must be empty
    void pull_chunk_updates(std::vector<::chunk_update>& chunk_updates);

    void push_unsubscriptions_and_chunk_update_acks(core::Span<::chunk* const> unsubscriptions,
                                                    core::Span<::chunk* const> chunk_update_acks);

    void update_subscription_requests(std::vector<::chunk_pos>& subscription_requests) noexcept;

    template<class F> void get_vertex_components(std::size_t n, F&& func);
    template<class F> void get_vertex_indices(std::size_t n, F&& func);

private:
    // log::Logger& m_logger;    
    const ::block_basis& m_block_basis;

    static constexpr int s_max_chunk_updates_in_progress = 32;

    std::mutex m_mutex;
    std::condition_variable m_condvar; // Protected by `m_mutex`
    bool m_quit = false; // Protected by `m_mutex`

    std::vector<::chunk*> m_unsubscriptions; // Protected by `m_mutex`
    std::vector<::chunk_pos> m_subscription_requests; // Protected by `m_mutex`
    core::Deque<std::pair<::chunk*, bool>> m_chunk_processing_queue;

    ::data_pipe<GLfloat> m_vertex_components; // Partially protected by `m_mutex`
    ::data_pipe<GLuint> m_vertex_indices; // Partially protected by `m_mutex`
    std::vector<::chunk_update> m_chunk_updates; // Protected by `m_mutex`
    std::vector<::chunk*> m_chunk_update_acks; // Protected by `m_mutex`
    int m_num_chunk_updates_in_progress = 0;

    // When chunks become unreferenced, they are appended to the end of the list of
    // unreferenced chunks. When `m_unreferenced_chunks` is not null, it points to the first
    // unreferenced chunk in the list, which is the one that became unreferenced first, and
    // the one that should be recycled first.
    std::unordered_map<::chunk_pos, std::unique_ptr<::chunk>, ::chunk_pos_hash> m_chunks;
    ::chunk* m_unreferenced_chunks = nullptr;
    std::size_t m_soft_max_chunks = 16384; // FIXME: Must be set in accordance with render distance. Set equal to number of chunks in local chunk grid.                                                                                                                                                                           
    std::size_t m_num_chunks = 0;
    std::size_t m_num_unreferenced_chunks = 0;

    void add_subscription(::chunk&);
    void remove_subscription(::chunk&) noexcept;
    void request_chunk_update(::chunk&, bool subscribing);
    auto process_chunk(chunk&) -> ::chunk_update_size;

    auto ensure_and_reference_chunk(const ::chunk_pos&) -> ::chunk&;
    bool try_recycle_unreferenced_chunk(std::unique_ptr<::chunk>&) noexcept;

    void reference_potentially_unreferenced_chunk(::chunk&) noexcept;
    void reference_chunk(::chunk&) noexcept;
    void unreference_chunk(::chunk&) noexcept;

    void append_unreferenced_chunk(::chunk&) noexcept;
    void remove_unreferenced_chunk(::chunk&) noexcept;

    void initialize_chunk_contents(::chunk&) noexcept;
};


inline worker::worker(log::Logger&, const ::block_basis& block_basis) noexcept
//    : m_logger(logger)    
    : m_block_basis(block_basis)
{
}


void worker::run()
{
    bool quit = false;
    std::vector<::chunk*> unsubscriptions, chunk_update_acks;
    constexpr int max_subscriptions = 4;
    std::array<chunk_pos, max_subscriptions> subscriptions;
    int num_subscriptions = 0;
    std::vector<::chunk_update> chunk_updates;
    while (!quit) {
        for (::chunk* chunk : unsubscriptions) {
            reference_chunk(*chunk);
            remove_subscription(*chunk);
            unreference_chunk(*chunk);
        }
        unsubscriptions.clear();

        for (::chunk* chunk : chunk_update_acks) {
            ARCHON_ASSERT(m_num_chunk_updates_in_progress > 0);
            m_num_chunk_updates_in_progress -= 1;
            unreference_chunk(*chunk);
        }
        chunk_update_acks.clear();

        for (int i = 0; i < num_subscriptions; ++i) {
            ::chunk_pos pos = subscriptions[i];
            ::chunk& chunk = ensure_and_reference_chunk(pos); // Throws
            if (!chunk.in_subscription)
                add_subscription(chunk); // Throws
            unreference_chunk(chunk);
        }
        num_subscriptions = 0;

        chunk_updates.clear();
        for (;;) {
            bool more = (!m_chunk_processing_queue.empty() &&
                         m_num_chunk_updates_in_progress < s_max_chunk_updates_in_progress);
            if (!more)
                break;
            auto pair = m_chunk_processing_queue.front();
            m_chunk_processing_queue.pop_front();
            ::chunk& chunk = *pair.first;
            bool subscribing = pair.second;
            ARCHON_ASSERT(chunk.update_requested);
            chunk.update_requested = false;
            bool has_data = false;
            ::chunk_update_size data_size;
            if (!chunk.processed) {
                has_data = true;
                data_size = process_chunk(chunk); // Throws
            }
            ::chunk_update update = {
                &chunk,
                subscribing,
                has_data,
                data_size,
            };
            chunk_updates.push_back(update); // Throws
            m_num_chunk_updates_in_progress += 1;
        }

        std::unique_lock lock(m_mutex);
        m_vertex_components.push(); // Throws
        m_vertex_indices.push(); // Throws
        m_chunk_updates.insert(m_chunk_updates.end(), chunk_updates.begin(), chunk_updates.end()); // Throws
        for (;;) {
            bool have_work = (m_quit ||
                              !m_unsubscriptions.empty() ||
                              !m_chunk_update_acks.empty() ||
                              (m_chunk_processing_queue.empty() && !m_subscription_requests.empty()));
            if (ARCHON_LIKELY(have_work))
                break;
            m_condvar.wait(lock);
        }
        quit = m_quit;
        swap(unsubscriptions, m_unsubscriptions);
        swap(chunk_update_acks, m_chunk_update_acks);
        if (m_chunk_processing_queue.empty()) {
            while (!m_subscription_requests.empty() && num_subscriptions < max_subscriptions) {
                ::chunk_pos pos = m_subscription_requests.back();
                int offset = num_subscriptions;
                subscriptions[offset] = pos;
                num_subscriptions += 1;
                m_subscription_requests.pop_back();
            }
        }
    }
}


void worker::quit() noexcept
{
    std::lock_guard lock(m_mutex);
    m_quit = true;
    m_condvar.notify_all();
}


void worker::pull_chunk_updates(std::vector<::chunk_update>& chunk_updates)
{
    ARCHON_ASSERT(chunk_updates.empty());
    std::lock_guard lock(m_mutex);
    m_vertex_components.pull(); // Throws
    m_vertex_indices.pull(); // Throws
    swap(m_chunk_updates, chunk_updates);
}


void worker::push_unsubscriptions_and_chunk_update_acks(core::Span<::chunk* const> unsubscriptions,
                                                        core::Span<::chunk* const> chunk_update_acks)
{
    std::lock_guard lock(m_mutex);
    m_unsubscriptions.insert(m_unsubscriptions.end(), unsubscriptions.begin(), unsubscriptions.end()); // Throws
    m_chunk_update_acks.insert(m_chunk_update_acks.end(), chunk_update_acks.begin(),
                               chunk_update_acks.end()); // Throws
    m_condvar.notify_all();
}


void worker::update_subscription_requests(std::vector<chunk_pos>& subscription_requests) noexcept
{
    std::lock_guard lock(m_mutex);
    swap(m_subscription_requests, subscription_requests);
    m_condvar.notify_all();
}


template<class F> inline void worker::get_vertex_components(std::size_t n, F&& func)
{
    m_vertex_components.extract(n, std::forward<F>(func)); // Throws
}


template<class F> inline void worker::get_vertex_indices(std::size_t n, F&& func)
{
    m_vertex_indices.extract(n, std::forward<F>(func)); // Throws
}


void worker::add_subscription(::chunk& chunk)
{
    ARCHON_ASSERT(!chunk.in_subscription);
    // Obtain a counted reference and keep it until the chunk is removed from the
    // subscription in remove_subscription()
    reference_chunk(chunk);
    chunk.in_subscription = true;
    bool subscribing = true;
    request_chunk_update(chunk, subscribing); // Throws
}


inline void worker::remove_subscription(::chunk& chunk) noexcept
{
    ARCHON_ASSERT(chunk.in_subscription);
    chunk.in_subscription = false;
    // Drop the counted reference that was obtained in add_subscription()
    unreference_chunk(chunk);
}


void worker::request_chunk_update(::chunk& chunk, bool subscribing)
{
    ARCHON_ASSERT(chunk.in_subscription);
    if (!chunk.update_requested) {
        m_chunk_processing_queue.emplace_back(&chunk, subscribing); // Throws
        // Obtained a counted reference and keep it during the processing of the chunk, and
        // until the acknowledgement from the rendering thread of the application of the
        // produced chunk update.
        reference_chunk(chunk);
        chunk.update_requested = true;
    }
}


auto worker::process_chunk(::chunk& chunk) -> ::chunk_update_size
{
    ARCHON_ASSERT(!chunk.processed);

    ::chunk* local_chunks[3][3][3] = {};
    ARCHON_SCOPE_EXIT {
        for (int z = 0; z < 3; ++z) {
            for (int y = 0; y < 3; ++y) {
                for (int x = 0; x < 3; ++x) {
                    ::chunk* chunk = local_chunks[z][y][x];
                    if (ARCHON_LIKELY(!chunk))
                        continue;
                    unreference_chunk(*chunk);
                }
            }
        }
    };
    reference_chunk(chunk);
    local_chunks[1][1][1] = &chunk;

    auto get_block = [](const ::chunk& chunk, const ::rel_block_pos& pos) noexcept {
        return ::get_block(chunk.blocks, pos);
    };

    auto get_block_a = [&, chunk_pos = chunk.pos](const ::rel_block_pos& pos) {
        ::rel_block_pos pos_2 = {
            core::int_periodic_mod(pos.x, g_chunk_size_x),
            core::int_periodic_mod(pos.y, g_chunk_size_y),
            core::int_periodic_mod(pos.z, g_chunk_size_z),
        };
        int x_2 = (pos.x - pos_2.x) / g_chunk_size_x;
        int y_2 = (pos.y - pos_2.y) / g_chunk_size_y;
        int z_2 = (pos.z - pos_2.z) / g_chunk_size_z;
        ARCHON_ASSERT(x_2 >= -1 && x_2 <= 1);
        ARCHON_ASSERT(y_2 >= -1 && y_2 <= 1);
        ARCHON_ASSERT(z_2 >= -1 && z_2 <= 1);
        ::chunk*& chunk = local_chunks[1 + z_2][1 + y_2][1 + x_2];
        if (ARCHON_UNLIKELY(!chunk)) {
            ::chunk_pos pos_2 = {
                chunk_coord_type(chunk_pos.x + x_2),
                chunk_coord_type(chunk_pos.y + y_2),
                chunk_coord_type(chunk_pos.z + z_2),
            };
            chunk = &ensure_and_reference_chunk(pos_2); // Throws
        }
        return get_block(*chunk, pos_2);
    };

    std::size_t serial = 0;
    auto pick_variant = [&](std::size_t n) {
        core::Hash_FNV_1a_Default hash;
        hash.add_obj(chunk.pos);
        hash.add_obj(serial);
        return std::size_t(hash.get() % n);
    };
    std::size_t num_components = 0;
    std::size_t num_indices = 0;
    GLuint num_vertices = 0;
    auto add_component = [&](GLfloat value) {
        m_vertex_components.add(value); // Throws
        core::int_add(num_components, 1); // Throws
    };
    auto add_components = [&](GLfloat a, GLfloat b, GLfloat c) {
        add_component(a); // Throws
        add_component(b); // Throws
        add_component(c); // Throws
    };
    auto add_vertex = [&](GLfloat x, GLfloat y, GLfloat z, GLfloat s, GLfloat t, GLfloat r, GLfloat light_level) {
        add_components(x, y, z); // Throws
        add_components(s, t, r); // Throws
        add_component(light_level); // Throws
        num_vertices += 1;
    };
    auto add_index = [&](GLuint offset, int i) {
        GLuint index = GLuint(offset + i);
        m_vertex_indices.add(index); // Throws
        core::int_add(num_indices, 1); // Throws
    };
    auto add_tri = [&](GLuint offset, int a, int b, int c) {
        add_index(offset, a); // Throws
        add_index(offset, b); // Throws
        add_index(offset, c); // Throws
    };
    auto add_quad = [&](GLuint offset, int a, int b, int c, int d) {
        add_tri(offset, a, b, c); // Throws
        add_tri(offset, c, d, a); // Throws
    };
    for (int z = 0; z < g_chunk_size_z; ++z) {
        for (int y = 0; y < g_chunk_size_y; ++y) {
            for (int x = 0; x < g_chunk_size_x; ++x, ++serial) {
                GLfloat x_2 = GLfloat(x);
                GLfloat y_2 = GLfloat(y);
                GLfloat z_2 = GLfloat(z);
                ::rel_block_pos pos = { x, y, z };
                ::block_index_type index = get_block(chunk, pos);
                ARCHON_ASSERT(index < m_block_basis.blocks.size());
                const ::block_basis::block& block = m_block_basis.blocks[index];
                std::size_t variants_begin = (index == 0 ? 0 : m_block_basis.blocks[index - 1].variants_end);
                std::size_t variants_end = block.variants_end;
                std::size_t num_variants = std::size_t(variants_end - variants_begin);
                ARCHON_ASSERT(num_variants >= 1);
                std::size_t variant_index = variants_begin;
                if (num_variants > 1)
                    variant_index = std::size_t(variants_begin + pick_variant(num_variants));
                const ::block_basis::block_variant& variant = m_block_basis.block_variants[variant_index];
                std::size_t quads_begin = (variant_index == 0 ? 0 :
                                           m_block_basis.block_variants[variant_index - 1].quads_end);
                std::size_t quads_end = variant.quads_end;
                for (std::size_t k = quads_begin; k < quads_end; ++k) {
                    const ::block_basis::quad& quad = m_block_basis.quads[k];
                    const ::directional_info& dir = *quad.dir;
                    ::rel_block_pos pos_2 = pos;
                    if (block.full)
                        pos_2 += dir.opposing_position;
                    ::block_index_type index_2 = get_block_a(pos_2); // Throws
                    const ::block_basis::block& block_2 = m_block_basis.blocks[index_2];
                    bool elide = (block.full && block_2.full && block_2.solid && (block_2.opaque || index_2 == index));
                    if (ARCHON_LIKELY(elide))
                        continue;
                    //  7  6  5
                    //  0     4
                    //  1  2  3
                    static_assert(std::extent_v<decltype(dir.lateral_positions)> == 8);
                    bool occlusions[8] = {};
                    for (int i = 0; i < 8; ++i) {
                        ::rel_block_pos pos_3 = pos_2 + dir.lateral_positions[i];
                        ::block_index_type index_3 = get_block_a(pos_3); // Throws
                        const ::block_basis::block& block_3 = m_block_basis.blocks[index_3];
                        occlusions[i] = (block_3.full && block_3.solid && block_3.opaque);
                    }
                    // Ambient occlusuion: Assuming all blocks are either empty or full
                    // solid blocks, and given a particular corner, C, of a particular
                    // visible face, F, of a particular nonempty block, B, the ambient
                    // occlusion value for C is determined as follows: First, imagine
                    // looking at F and consider the empty block, E, in front of it. It must
                    // be empty in order for F to be visible. Next, let P and Q refer to the
                    // two blocks that are face-sharing neighbors of E, have a corner that
                    // touches C, and are not B. Also, let R refer to the block that is a
                    // face-sharing neighbor of both P and Q and is not E. Finally, if both
                    // P and Q are nonempty, the occlusion value for C is 3. Otherwise, if
                    // either P or Q is nonempty, and R is also nonempty, the occlusion
                    // value for C is 2. Otherwise, if either P, Q, or R is nonempty, the
                    // occlusion value for C is 1, Otherwise, the occlusion value for C is
                    // 0.
                    auto ambient_occlusion_factor = [&](int i_1, int i_2, int i_3) noexcept -> GLfloat {
                        bool occl_1 = occlusions[i_1];
                        bool occl_2 = occlusions[i_2];
                        bool occl_3 = occlusions[i_3];
                        if (ARCHON_LIKELY(!occl_1 || !occl_3)) {
                            int n = int(occl_1) + int(occl_2) + int(occl_3);
                            switch (n) {
                                case 0:
                                    return 1.0;
                                case 1:
                                    return 0.8;
                            }
                            return 0.6;
                        }
                        return 0.4;
                    };
                    GLfloat l_1 = dir.light_level * ambient_occlusion_factor(0, 1, 2);
                    GLfloat l_2 = dir.light_level * ambient_occlusion_factor(2, 3, 4);
                    GLfloat l_3 = dir.light_level * ambient_occlusion_factor(4, 5, 6);
                    GLfloat l_4 = dir.light_level * ambient_occlusion_factor(6, 7, 0);
                    GLfloat r = quad.texture;
                    GLuint offset = num_vertices;
                    add_vertex(x_2 + quad.x_1, y_2 + quad.y_1, z_2 + quad.z_1, quad.s_1, quad.t_1, r, l_1); // Throws
                    add_vertex(x_2 + quad.x_2, y_2 + quad.y_2, z_2 + quad.z_2, quad.s_2, quad.t_2, r, l_2); // Throws
                    add_vertex(x_2 + quad.x_3, y_2 + quad.y_3, z_2 + quad.z_3, quad.s_3, quad.t_3, r, l_3); // Throws
                    add_vertex(x_2 + quad.x_4, y_2 + quad.y_4, z_2 + quad.z_4, quad.s_4, quad.t_4, r, l_4); // Throws
                    add_quad(offset, 0, 1, 2, 3); // Throws
                }
            }
        }
    }

    chunk.processed = true;
    return {
        num_components,
        num_indices,
    };
}


auto worker::ensure_and_reference_chunk(const ::chunk_pos& pos) -> ::chunk&
{
    std::unique_ptr<::chunk>& chunk = m_chunks[pos]; // Throws
    if (ARCHON_LIKELY(chunk)) {
        reference_potentially_unreferenced_chunk(*chunk);
        return *chunk;
    }
    if (ARCHON_UNLIKELY(m_num_chunks < m_soft_max_chunks || !try_recycle_unreferenced_chunk(chunk)))
        chunk = std::make_unique<::chunk>(); // Throws
    chunk->pos = pos;
    chunk->reference_count = 1;
    m_num_chunks += 1;
    initialize_chunk_contents(*chunk);
    return *chunk;
}


bool worker::try_recycle_unreferenced_chunk(std::unique_ptr<::chunk>& chunk) noexcept
{
    if (ARCHON_LIKELY(m_num_unreferenced_chunks > 0)) {
        ARCHON_ASSERT(m_num_chunks > 0);
        ::chunk& chunk_2 = *m_unreferenced_chunks;
        remove_unreferenced_chunk(chunk_2);
        auto i = m_chunks.find(chunk_2.pos);
        ARCHON_ASSERT(i != m_chunks.end());
        chunk = std::move(i->second);
        m_chunks.erase(i);
        m_num_chunks -= 1;
        gl_vertex_array vertex_array = chunk_2.vertex_array;
        chunk_2 = ::chunk();
        chunk_2.vertex_array = vertex_array;
        return true;
    }
    return false;
}


void worker::reference_potentially_unreferenced_chunk(::chunk& chunk) noexcept
{
    if (chunk.reference_count == 0) {
        remove_unreferenced_chunk(chunk);
        chunk.reference_count = 1;
        return;
    }
    reference_chunk(chunk);
}


inline void worker::reference_chunk(::chunk& chunk) noexcept
{
    ARCHON_ASSERT(chunk.reference_count > 0);
    ARCHON_ASSERT(chunk.reference_count < core::int_max<decltype(chunk.reference_count)>());
    chunk.reference_count += 1;
}


void worker::unreference_chunk(::chunk& chunk) noexcept
{
    ARCHON_ASSERT(chunk.reference_count > 0);
    chunk.reference_count -= 1;
    if (chunk.reference_count == 0)
        append_unreferenced_chunk(chunk);
}


void worker::append_unreferenced_chunk(::chunk& chunk) noexcept
{
    // This function appends the specified chunk to the end of the list of unreferenced
    // chunks
    ARCHON_ASSERT(chunk.reference_count == 0);
    ARCHON_ASSERT(!chunk.prev_unreferenced);
    ARCHON_ASSERT(!chunk.next_unreferenced);
    if (ARCHON_LIKELY(m_unreferenced_chunks)) {
        ::chunk* prev = m_unreferenced_chunks->prev_unreferenced;
        chunk.prev_unreferenced = prev;
        chunk.next_unreferenced = m_unreferenced_chunks;
        prev->next_unreferenced = &chunk;
        m_unreferenced_chunks->prev_unreferenced = &chunk;
    }
    else {
        chunk.prev_unreferenced = &chunk;
        chunk.next_unreferenced = &chunk;
        m_unreferenced_chunks = &chunk;
    }
    m_num_unreferenced_chunks += 1;
}


void worker::remove_unreferenced_chunk(::chunk& chunk) noexcept
{
    ARCHON_ASSERT(chunk.reference_count == 0);
    ARCHON_ASSERT(chunk.prev_unreferenced);
    ARCHON_ASSERT(chunk.next_unreferenced);
    ARCHON_ASSERT(m_num_unreferenced_chunks > 0);
    if (ARCHON_LIKELY(m_num_unreferenced_chunks > 1)) {
        ::chunk* prev = chunk.prev_unreferenced;
        ::chunk* next = chunk.next_unreferenced;
        if (m_unreferenced_chunks == &chunk)
            m_unreferenced_chunks = next;
        prev->next_unreferenced = next;
        next->prev_unreferenced = prev;
    }
    else {
        m_unreferenced_chunks = nullptr;
    }
    chunk.prev_unreferenced = nullptr;
    chunk.next_unreferenced = nullptr;
    m_num_unreferenced_chunks -= 1;
}


void worker::initialize_chunk_contents(::chunk& chunk) noexcept
{
    // FIXME: Explain why this cannot overflow   
    block_pos pos = {
        block_coord_type(chunk.pos.x * g_chunk_size_x),
        block_coord_type(chunk.pos.y * g_chunk_size_y),
        block_coord_type(chunk.pos.z * g_chunk_size_z),
    };
    using hash_type = core::Hash_FNV_1a_64;
    hash_type hash;
    int seed = 7342245;
    hash.add_int(seed);
    hash.add_obj(pos);
    std::size_t serial = 0;
    for (int z = 0; z < g_chunk_size_z; ++z) {
        for (int y = 0; y < g_chunk_size_y; ++y) {
            for (int x = 0; x < g_chunk_size_x; ++x, ++serial) {
                hash_type hash_2 = hash;
                hash_2.add_int(serial);
                hash_2.add_int(476238522);
                double f_1 = hash_2.get_as_float<double>();
                hash_2.add_int(524276385);
                double f_2 = hash_2.get_as_float<double>();
                block_coord_type y_2 = block_coord_type(pos.y + y); // FIXME: What guarantees that this does not overflow?    
                block_index_type air   = 0;
                block_index_type stone = 1;
                block_index_type coal_ore = 2;
                block_index_type redstone_ore = 3;
                block_index_type i = (y_2 < (f_1 < 0.001 ? 1 : 0) ? (f_2 < 0.0005 ? redstone_ore : (f_2 < 0.005 ? coal_ore : stone)) : air);
                set_block(chunk.blocks, x, y, z, i);
            }
        }
    }
}



class world final
    : public render::Engine::Scene {
public:
    using clock = render::Engine::Clock;

    world(const std::filesystem::path& resource_path, const std::locale&, log::Logger&, render::Engine&,
          ::block_basis&, ::worker&) noexcept;

    bool try_prepare(std::string&) override final;
    void render_init() override final;
    void set_projection(const math::Matrix4F&) override final;
    void render(const math::Matrix4F&) override final;
    bool tick(clock::time_point) override final;

private:
    const std::filesystem::path m_resource_path;
    const std::locale m_locale;
    log::Logger& m_logger;
//    render::Engine& m_engine;    
    ::block_basis& m_block_basis;
    ::worker& m_worker;

    std::vector<chunk*> m_unsubscriptions;
    std::vector<chunk_pos> m_new_subscription_requests;

/*    
    bool m_thrust_forwards = false, m_thrust_backwards = false;
    bool m_thrust_leftwards = false, m_thrust_rightwards = false;
    bool m_thrust_upwards = false, m_thrust_downwards = false;
    bool m_sprint_mode = false;
*/
    bool m_thrust_downwards = false;    

    GLuint m_texture = {};
    GLuint m_shader_program = {};
    GLint m_proj_loc = {};
    GLint m_view_loc = {};
    GLint m_model_loc = {};
    GLint m_texture_loc = {};

    std::stack<::gl_vertex_array> m_unused_vertex_arrays;
    std::vector<::chunk_update> m_chunk_updates;
    std::vector<::chunk*> m_chunk_update_acks;

    // Consider the chunk at `m_chunk_array[z][y][x]` to be at X, Y, and Z-coordinates, `x`,
    // `y`, and `z`.
    int m_chunk_array_size_x = {};
    int m_chunk_array_size_y = {};
    int m_chunk_array_size_z = {};
    std::unique_ptr<chunk*[]> m_chunk_array;

    // List of chunks that are within the currently selected render distance. Chunks occur
    // in an order that ensures back-to-front rendering. Chunk array positions are relative
    // to the center chunk (the one containing the player).
    std::vector<rel_chunk_pos> m_chunk_order;

    ::chunk_pos m_current_chunk = { 0, 0, 0 };

    // Position of camera relative to origin of current chunk. Measured in block units.
    math::Vector3F m_position;

    // Velocity of camera. Measured in blocks per tick.
    math::Vector3F m_velocity = { 0, 0, -0.05 };                                                                                                                                                                

    bool try_init_block_basis(const ::block_basis_spec&, std::string& error);
    void add_empty_block();
    void add_block(int texture);

    // Maximum distance in number of blocks between center of central chunk and center of
    // rendered chunk.
    void change_render_distance(double horz_dist, double vert_dist);

    // In number of blocks
    void set_position(const math::Vector3& pos, const block_pos& ref = {});
    bool try_set_position(const math::Vector3& pos, const block_pos& ref = {}) noexcept;

    // In meters above feet
    float get_eye_height() const noexcept;

    bool inside_chunk_array(const chunk_pos&) const noexcept;

    // Position must be inside chunk array
    auto get_array_chunk(const chunk_pos&) noexcept -> chunk*&;

    auto array_chunk_index(int x, int y, int z) const noexcept -> std::size_t;

    void apply_chunk_update(::chunk&, const ::chunk_update_size&);

    auto alloc_vertex_array() -> ::gl_vertex_array;
    void return_vertex_array(::gl_vertex_array);
};



world::world(const std::filesystem::path& resource_path, const std::locale& locale, log::Logger& logger,
             render::Engine&, ::block_basis& block_basis, ::worker& worker) noexcept
    : m_resource_path(resource_path)
    , m_locale(locale)
    , m_logger(logger)
//    , m_engine(engine)    
    , m_block_basis(block_basis)
    , m_worker(worker)
{
}



bool world::try_prepare(std::string& error)
{
    ::block_basis_spec spec;

    auto add_texture = [&](std::string_view path) {
        ::block_basis_spec::texture texture = {
            path,
        };
        std::size_t index = spec.textures.size();
        spec.textures.push_back(texture); // Throws
        return index;
    };

    auto add_block = [&](std::string_view path) {
        std::size_t texture = add_texture(path); // Throws
        ::block_basis_spec::block block = {
            texture,
        };
        spec.blocks.push_back(block); // Throws
    };

    add_block("stone.png"); // Throws
    add_block("coal_ore.png"); // Throws
    add_block("redstone_ore.png"); // Throws

    if (ARCHON_UNLIKELY(!try_init_block_basis(spec, error))) // Throws
        return false;

    GLuint vertex_shader = {};
    GLuint fragment_shader = {};
    if (ARCHON_UNLIKELY(!render::compile_shader(GL_VERTEX_SHADER, "vertex shader", g_vertex_shader_source,
                                                m_logger, vertex_shader))) { // Throws
        error = "Faield to compile vertex shader"; // Throws
        return false;
    }
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
    m_view_loc = glGetUniformLocation(m_shader_program, "uViewMatrix");
    m_model_loc = glGetUniformLocation(m_shader_program, "uModelMatrix");
    m_texture_loc = glGetUniformLocation(m_shader_program, "uTextureArray");
    if (m_proj_loc < 0 || m_view_loc < 0 || m_model_loc < 0 || m_texture_loc < 0) {
        error = "Failed to get uniform locations in shader program"; // Throws
        return false;
    }

    return true;
}



void world::render_init()
{
    change_render_distance(192, 192); // Throws
    set_position({ 0, 0, 0 }); // Throws

    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
    glUseProgram(m_shader_program);
    glUniform1i(m_texture_loc, 0); // Use texture unit 0
}



void world::set_projection(const math::Matrix4F& proj)
{
    render::set_uniform_matrix(m_proj_loc, proj); // Throws
}



void world::render(const math::Matrix4F& view)
{
    math::Vector3F eye_displacement = { 0, 0, 0 };
    eye_displacement[1] += get_eye_height();

    math::Matrix4F view_2 = view;
    gfx::translate(view_2, math::Vector3F(-(eye_displacement + m_position)));
    render::set_uniform_matrix(m_view_loc, view_2); // Throws

    // Render entities
    

    // Render opaque blocks
    m_new_subscription_requests.clear();
//    m_transparent_chunks.clear();           
    for (::rel_chunk_pos pos : m_chunk_order) {
        // FIXME: Find a way to efficiently skip some of the chunnks that are definitely not intersecting the view frustum      
        // FIXME: Maybe verify that position is inside logical array boundary          
        // FIXME: Explain why this cannot overflow (world border margin)    
        ::chunk_pos pos_2 = m_current_chunk + pos;
        ::chunk* chunk = get_array_chunk(pos_2);
        if (ARCHON_LIKELY(chunk)) {
            if (chunk->num_indices > 0) {
                // FIXME: Formally require that each of the coordinates of a chunk array position can be represented in `int` as a number of blocks         
                float x = GLfloat(pos.x * g_chunk_size_x);
                float y = GLfloat(pos.y * g_chunk_size_y);
                float z = GLfloat(pos.z * g_chunk_size_z);
                render::set_uniform_matrix(m_model_loc, gfx::make_translation({ x, y, z })); // Throws

                glBindVertexArray(chunk->vertex_array.vao);
                GLsizei count = chunk->num_indices;
                GLenum type = GL_UNSIGNED_INT;
                std::size_t offset = 0;
                glDrawElements(GL_TRIANGLES, count, type, reinterpret_cast<void*>(offset));
            }

            // FIXME: If chunk has semi-transparent texels: m_transparent_chunks.push_back(cnk); // Throws    

            continue;
        }

        m_new_subscription_requests.push_back(pos_2); // Throws
    }

    m_worker.update_subscription_requests(m_new_subscription_requests);


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



bool world::tick(clock::time_point)
{
    bool need_redraw = false;

    m_unsubscriptions.clear();
    auto unsubscribe = [&](::chunk& chunk) {
        m_unsubscriptions.push_back(&chunk); // Throws
    };

    if (m_velocity != math::Vector3F(0, 0, 0)) {
        m_position += m_velocity;

        // FIXME: Find a way to prevent movement across world boundary (g_world_border_margin)                  
        auto adjust = [](float& pos, float chunk_size, ::chunk_coord_type& curr_chunk, auto&& clear) {
            if (ARCHON_LIKELY(pos >= 0)) {
                for (;;) {
                    if (ARCHON_LIKELY(pos < chunk_size))
                        return;
                    pos -= chunk_size;
                    curr_chunk += 1;
                    clear(+1); // Throws
                }
            }
            for (;;) {
                pos += chunk_size;
                curr_chunk -= 1;
                clear(-1); // Throws
                if (ARCHON_LIKELY(pos >= 0))
                    return;
            }
        };

        ARCHON_ASSERT(m_chunk_array_size_x % 2 == 1);
        ARCHON_ASSERT(m_chunk_array_size_y % 2 == 1);
        ARCHON_ASSERT(m_chunk_array_size_z % 2 == 1);

        auto clear_x = [&](int direction) {
            ::chunk_coord_type x_1 = chunk_coord_type(m_current_chunk.x + direction * (m_chunk_array_size_x / 2));
            int x_2 = core::int_periodic_mod(x_1, m_chunk_array_size_x);
            for (int z = 0; z < m_chunk_array_size_z; ++z) {
                for (int y = 0; y < m_chunk_array_size_y; ++y) {
                    std::size_t i = array_chunk_index(x_2, y, z);
                    ::chunk*& chunk = m_chunk_array[i];
                    if (ARCHON_LIKELY(chunk)) {
                        unsubscribe(*chunk); // Throws
                        chunk = nullptr;
                    }
                }
            }
        };

        auto clear_y = [&](int direction) {
            ::chunk_coord_type y_1 = chunk_coord_type(m_current_chunk.y + direction * (m_chunk_array_size_y / 2));
            int y_2 = core::int_periodic_mod(y_1, m_chunk_array_size_y);
            for (int z = 0; z < m_chunk_array_size_z; ++z) {
                for (int x = 0; x < m_chunk_array_size_x; ++x) {
                    std::size_t i = array_chunk_index(x, y_2, z);
                    ::chunk*& chunk = m_chunk_array[i];
                    if (ARCHON_LIKELY(chunk)) {
                        unsubscribe(*chunk); // Throws
                        chunk = nullptr;
                    }
                }
            }
        };

        auto clear_z = [&](int direction) {
            ::chunk_coord_type z_1 = chunk_coord_type(m_current_chunk.z + direction * (m_chunk_array_size_z / 2));
            int z_2 = core::int_periodic_mod(z_1, m_chunk_array_size_z);
            for (int y = 0; y < m_chunk_array_size_y; ++y) {
                for (int x = 0; x < m_chunk_array_size_x; ++x) {
                    std::size_t i = array_chunk_index(x, y, z_2);
                    ::chunk*& chunk = m_chunk_array[i];
                    if (ARCHON_LIKELY(chunk)) {
                        unsubscribe(*chunk); // Throws
                        chunk = nullptr;
                    }
                }
            }
        };

        constexpr float chunk_size_x = float(g_chunk_size_x);
        constexpr float chunk_size_y = float(g_chunk_size_y);
        constexpr float chunk_size_z = float(g_chunk_size_z);
        adjust(m_position[0], chunk_size_x, m_current_chunk.x, clear_x); // Throws
        adjust(m_position[1], chunk_size_y, m_current_chunk.y, clear_y); // Throws
        adjust(m_position[2], chunk_size_z, m_current_chunk.z, clear_z); // Throws

        need_redraw = true;
    }

    m_chunk_updates.clear();
    m_worker.pull_chunk_updates(m_chunk_updates); // Throws

    m_chunk_update_acks.clear();
    for (const ::chunk_update& update : m_chunk_updates) {
        ::chunk& chunk = *update.chunk;
        if (update.subscribing) {
            if (ARCHON_LIKELY(inside_chunk_array(chunk.pos))) {
                ::chunk*& slot = get_array_chunk(chunk.pos);
                ARCHON_ASSERT(!slot);
                slot = &chunk;
            }
            else {
                unsubscribe(chunk); // Throws
            }
        }
        if (update.has_data)
            apply_chunk_update(chunk, update.data_size); // Throws
        m_chunk_update_acks.push_back(&chunk); // Throws
        need_redraw = true;
    }

    if (!m_unsubscriptions.empty() || !m_chunk_update_acks.empty())
        m_worker.push_unsubscriptions_and_chunk_update_acks(m_unsubscriptions, m_chunk_update_acks); // Throws

    return need_redraw;
}



bool world::try_init_block_basis(const block_basis_spec& spec, std::string& error)
{
    constexpr image::Size texture_size = 16;

    int num_textures = {};
    core::int_cast(spec.textures.size(), num_textures); // Throws

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_texture);
    {
        GLint level = 0;
        GLint internal_format = GL_SRGB8_ALPHA8;
        GLsizei width = texture_size.width;
        GLsizei height = texture_size.height;
        GLsizei depth = {};
        GLint border = 0;
        GLenum format = GL_BGRA; // Immaterial
        GLenum type = GL_UNSIGNED_BYTE; // Immaterial
        const void* data = nullptr;
        core::int_cast(num_textures, depth); // Throws
        glTexImage3D(GL_TEXTURE_2D_ARRAY, level, internal_format, width, height, depth, border, format, type, data);
    }
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    core::StringFormatter string_formatter(m_locale); // Throws
    for (int i = 0; i < num_textures; ++i) {
        const block_basis_spec::texture& texture = spec.textures[i];

        namespace fs = std::filesystem;
        fs::path path = (m_resource_path / core::make_fs_path_generic(texture.path, m_locale)); // Throws
        std::unique_ptr<image::WritableImage> image;
        log::PrefixLogger sublogger(m_logger, string_formatter.format("Load texture %s: ", texture.path)); // Throws
        image::PNGLoadConfig png_load_config;
        png_load_config.expand_indirect_color = true;
        png_load_config.expand_lum_to_rgb = true;
        png_load_config.ensure_alpha_channel = true;
        image::FileFormat::SpecialLoadConfigRegistry special_load_config_registry;
        special_load_config_registry.set(png_load_config); // Throws
        image::LoadConfig load_config;
        load_config.vertical_flip = true;
        load_config.logger = &sublogger;
        load_config.special = &special_load_config_registry;
        std::error_code ec;
        if (ARCHON_UNLIKELY(!image::try_load(path, image, m_locale, load_config, ec))) { // Throws
            error = core::format("Failed to load texture image %s: %s", path, ec.message()); // Throws
            return false;
        }

        image::Size size = image->get_size();;
        if (ARCHON_UNLIKELY(size != texture_size)) {
            error = core::format("Texture image %s has wrong size: %s", path, size); // Throws
            return false;
        }

        int layer = i;
        bool require_format_match = true;
        bool success = render::try_load_texture_layer(*image, layer, require_format_match); // Throws
        if (ARCHON_UNLIKELY(!success)) {
            error = core::format("Unmatchable buffer format of texture image %s", path); // Throws
            return false;
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    add_empty_block(); // Throws
    for (const block_basis_spec::block& block : spec.blocks)
        add_block(int(block.texture)); // Throws

    return true;
}



void world::add_empty_block()
{
    std::size_t quads_end = m_block_basis.quads.size();
    ::block_basis::block_variant variant = { quads_end };
    m_block_basis.block_variants.push_back(variant); // Throws
    std::size_t variants_end = m_block_basis.block_variants.size();
    bool full = false;
    bool solid = false;
    bool opaque = false;
    ::block_basis::block block = {
        variants_end,
        full,
        solid,
        opaque,
    };
    m_block_basis.blocks.push_back(block); // Throws
}



void world::add_block(int texture)
{
    auto add_quad = [&](::face_orientation orientation) {
        const ::directional_info* dir = {};
        GLfloat s_1 = 0, t_1 = 0, x_1 = 0, y_1 = 0, z_1 = 0;
        GLfloat s_2 = 1, t_2 = 0, x_2 = 0, y_2 = 0, z_2 = 0;
        GLfloat s_3 = 1, t_3 = 1, x_3 = 0, y_3 = 0, z_3 = 0;
        GLfloat s_4 = 0, t_4 = 1, x_4 = 0, y_4 = 0, z_4 = 0;
        switch (orientation) {
            case ::face_orientation::left:
                dir = &g_direction_west;
                x_1 = 0, y_1 = 0, z_1 = 0;
                x_2 = 0, y_2 = 0, z_2 = 1;
                x_3 = 0, y_3 = 1, z_3 = 1;
                x_4 = 0, y_4 = 1, z_4 = 0;
                break;
            case ::face_orientation::right:
                dir = &g_direction_east;
                x_1 = 1, y_1 = 0, z_1 = 1;
                x_2 = 1, y_2 = 0, z_2 = 0;
                x_3 = 1, y_3 = 1, z_3 = 0;
                x_4 = 1, y_4 = 1, z_4 = 1;
                break;
            case ::face_orientation::bottom:
                dir = &g_direction_down;
                x_1 = 0, y_1 = 0, z_1 = 0;
                x_2 = 1, y_2 = 0, z_2 = 0;
                x_3 = 1, y_3 = 0, z_3 = 1;
                x_4 = 0, y_4 = 0, z_4 = 1;
                break;
            case ::face_orientation::top:
                dir = &g_direction_up;
                x_1 = 0, y_1 = 1, z_1 = 1;
                x_2 = 1, y_2 = 1, z_2 = 1;
                x_3 = 1, y_3 = 1, z_3 = 0;
                x_4 = 0, y_4 = 1, z_4 = 0;
                break;
            case ::face_orientation::back:
                dir = &g_direction_north;
                x_1 = 1, y_1 = 0, z_1 = 0;
                x_2 = 0, y_2 = 0, z_2 = 0;
                x_3 = 0, y_3 = 1, z_3 = 0;
                x_4 = 1, y_4 = 1, z_4 = 0;
                break;
            case ::face_orientation::front:
                dir = &g_direction_south;
                x_1 = 0, y_1 = 0, z_1 = 1;
                x_2 = 1, y_2 = 0, z_2 = 1;
                x_3 = 1, y_3 = 1, z_3 = 1;
                x_4 = 0, y_4 = 1, z_4 = 1;
                break;
        }
        ::block_basis::quad quad = {
            dir,
            GLfloat(texture),
            x_1, y_1, z_1, s_1, t_1,
            x_2, y_2, z_2, s_2, t_2,
            x_3, y_3, z_3, s_3, t_3,
            x_4, y_4, z_4, s_4, t_4,
        };
        m_block_basis.quads.push_back(quad); // Throws
    };
    add_quad(::face_orientation::left); // Throws
    add_quad(::face_orientation::right); // Throws
    add_quad(::face_orientation::bottom); // Throws
    add_quad(::face_orientation::top); // Throws
    add_quad(::face_orientation::back); // Throws
    add_quad(::face_orientation::front); // Throws
    std::size_t quads_end = m_block_basis.quads.size();
    ::block_basis::block_variant variant = { quads_end };
    m_block_basis.block_variants.push_back(variant); // Throws
    std::size_t variants_end = m_block_basis.block_variants.size();
    bool full = true;
    bool solid = true;
    bool opaque = true;
    ::block_basis::block block = {
        variants_end,
        full,
        solid,
        opaque,
    };
    m_block_basis.blocks.push_back(block); // Throws
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
    // determined such that it has a chunk at the center, i.e., the center chunk in which
    // the player is currently positioned, and such that it covers any chunk that is within
    // the render distance as explained above.

    constexpr long max_horz_render_distance = 512;
    constexpr long max_vert_render_distance = 512;

    // Ensure that every rendered block has an absolute position that is representable as a
    // block_pos
    static_assert(max_horz_render_distance <= g_world_border_margin - g_chunk_size_x);
    static_assert(max_vert_render_distance <= g_world_border_margin - g_chunk_size_y);
    static_assert(max_horz_render_distance <= g_world_border_margin - g_chunk_size_z);

    constexpr int max_rings_x = int(max_horz_render_distance / g_chunk_size_x);
    constexpr int max_rings_y = int(max_vert_render_distance / g_chunk_size_y);
    constexpr int max_rings_z = int(max_horz_render_distance / g_chunk_size_z);
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

    using comp_type = rel_chunk_pos::comp_type;
    static_assert(core::is_signed<comp_type>());
    static_assert(max_rings_x <= core::int_max<comp_type>());
    static_assert(max_rings_y <= core::int_max<comp_type>());
    static_assert(max_rings_z <= core::int_max<comp_type>());

    comp_type order_x[max_size_x] = {};
    comp_type order_y[max_size_y] = {};
    comp_type order_z[max_size_z] = {};
    for (int i = 0; i < size_x; ++i)
        order_x[i] = comp_type(i < num_rings_x ? i - num_rings_x : size_x - 1 - i);
    for (int i = 0; i < size_y; ++i)
        order_y[i] = comp_type(i < num_rings_y ? i - num_rings_y : size_y - 1 - i);
    for (int i = 0; i < size_z; ++i)
        order_z[i] = comp_type(i < num_rings_z ? i - num_rings_z : size_z - 1 - i);

    m_chunk_order.clear();
    for (int i = 0; i < size_z; ++i) {
        comp_type z = order_z[i];
        for (int j = 0; j < size_y; ++j) {
            comp_type y = order_y[j];
            for (int k = 0; k < size_x; ++k) {
                comp_type x = order_x[k];
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
    // FIXME: Take world border margin into account here (g_world_border_margin)                                                             

    double x_1 = std::floor(pos[0]);
    double y_1 = std::floor(pos[1]);
    double z_1 = std::floor(pos[2]);

    block_coord_type x_2 = {}, y_2 = {}, z_2 = {};
    bool success_1 = (core::try_float_to_int(x_1, x_2) &&
                      core::try_float_to_int(y_1, y_2) &&
                      core::try_float_to_int(z_1, z_2));

    float x_3 = float(pos[0] - x_1);
    float y_3 = float(pos[1] - y_1);
    float z_3 = float(pos[2] - z_1);
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

    chunk_coord_type x_4 = chunk_coord_type(x_2 / g_chunk_size_x);
    int x_5 = int(x_2 % g_chunk_size_x);
    chunk_coord_type y_4 = chunk_coord_type(y_2 / g_chunk_size_y);
    int y_5 = int(y_2 % g_chunk_size_y);
    chunk_coord_type z_4 = chunk_coord_type(z_2 / g_chunk_size_z);
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

    m_current_chunk = { x_4, y_4, z_4 };

    m_position = {
        x_3 + float(x_5),
        y_3 + float(y_5),
        z_3 + float(z_5),
    };

    return true;
}



float world::get_eye_height() const noexcept
{
    float normal_height = 1.62;
    float sneak_height  = 1.54;
    if (m_thrust_downwards)
        return sneak_height;
    return normal_height;
}



bool world::inside_chunk_array(const chunk_pos& pos) const noexcept
{
    ARCHON_ASSERT(m_chunk_array_size_x % 2 == 1);
    ARCHON_ASSERT(m_chunk_array_size_y % 2 == 1);
    ARCHON_ASSERT(m_chunk_array_size_z % 2 == 1);
    int num_rings_x = m_chunk_array_size_x / 2;
    int num_rings_y = m_chunk_array_size_y / 2;
    int num_rings_z = m_chunk_array_size_z / 2;
    return (pos.x >= m_current_chunk.x - num_rings_x && pos.x <= m_current_chunk.x + num_rings_x &&
            pos.y >= m_current_chunk.y - num_rings_y && pos.y <= m_current_chunk.y + num_rings_y &&
            pos.z >= m_current_chunk.z - num_rings_z && pos.z <= m_current_chunk.z + num_rings_z);
}



auto world::get_array_chunk(const chunk_pos& pos) noexcept -> chunk*&
{
    int x = core::int_periodic_mod(pos.x, m_chunk_array_size_x);
    int y = core::int_periodic_mod(pos.y, m_chunk_array_size_y);
    int z = core::int_periodic_mod(pos.z, m_chunk_array_size_z);
    std::size_t i = array_chunk_index(x, y, z);
    return m_chunk_array[i];
}



inline auto world::array_chunk_index(int x, int y, int z) const noexcept -> std::size_t
{
    return std::size_t((std::size_t(z) * m_chunk_array_size_y + y) * m_chunk_array_size_x + x);
}



void world::apply_chunk_update(::chunk& chunk, const ::chunk_update_size& data_size)
{
    if (data_size.num_vertex_indices > 0) {
        if (chunk.vertex_array.vao == 0)
            chunk.vertex_array = alloc_vertex_array(); // Throws
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, chunk.vertex_array.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.vertex_array.ebo);
        auto handle_vertex_components = [&](std::size_t offset, core::Span<GLfloat> subdata) {
            if (ARCHON_LIKELY(offset == 0)) {
                GLsizeiptr total_size = {};
                core::int_cast(data_size.num_vertex_components, total_size); // Throws
                core::int_mul(total_size, sizeof (GLfloat)); // Throws
                if (ARCHON_LIKELY(subdata.size() == data_size.num_vertex_components)) {
                    glBufferData(GL_ARRAY_BUFFER, total_size, subdata.data(), GL_DYNAMIC_DRAW);
                    return;
                }
                void* data = nullptr;
                glBufferData(GL_ARRAY_BUFFER, total_size, data, GL_DYNAMIC_DRAW);
            }
            GLintptr offset_2 = {};
            core::int_cast(offset, offset_2); // Throws
            core::int_mul(offset_2, sizeof (GLfloat)); // Throws
            GLsizeiptr size = {};
            core::int_cast(subdata.size(), size); // Throws
            core::int_mul(size, sizeof (GLfloat)); // Throws
            glBufferSubData(GL_ARRAY_BUFFER, offset_2, size, subdata.data());
        };
        auto handle_vertex_indices = [&](std::size_t offset, core::Span<GLuint> subdata) {
            if (ARCHON_LIKELY(offset == 0)) {
                GLsizeiptr total_size = {};
                core::int_cast(data_size.num_vertex_indices, total_size); // Throws
                core::int_mul(total_size, sizeof (GLuint)); // Throws
                if (ARCHON_LIKELY(subdata.size() == data_size.num_vertex_indices)) {
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_size, subdata.data(), GL_DYNAMIC_DRAW);
                    return;
                }
                void* data = nullptr;
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_size, data, GL_DYNAMIC_DRAW);
            }
            GLintptr offset_2 = {};
            core::int_cast(offset, offset_2); // Throws
            core::int_mul(offset_2, sizeof (GLuint)); // Throws
            GLsizeiptr size = {};
            core::int_cast(subdata.size(), size); // Throws
            core::int_mul(size, sizeof (GLuint)); // Throws
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset_2, size, subdata.data());
        };
        m_worker.get_vertex_components(data_size.num_vertex_components, std::move(handle_vertex_components)); // Throws
        m_worker.get_vertex_indices(data_size.num_vertex_indices, std::move(handle_vertex_indices)); // Throws
    }
    else {
        if (chunk.vertex_array.vao != 0) {
            return_vertex_array(chunk.vertex_array); // Throws
            chunk.vertex_array = {};
        }
    }
    core::int_cast(data_size.num_vertex_indices, chunk.num_indices); // Throws
}



auto world::alloc_vertex_array() -> ::gl_vertex_array
{
    if (ARCHON_UNLIKELY(m_unused_vertex_arrays.empty())) {
        struct vertex_attrib {
            GLint  size;     // Number of components
            GLuint location; // Attribute location
        };
        vertex_attrib attribs[] = {
            { 3, 0 }, // Vertex coordinate
            { 3, 1 }, // Texture coordinate
            { 1, 2 }, // Light level
        };
        constexpr int num_attribs = std::size(attribs);
        GLsizei stride = {};
        std::size_t offsets[num_attribs] = {};
        {
            std::size_t offset = 0;
            for (int i = 0; i < num_attribs; ++i) {
                const vertex_attrib& attrib = attribs[i];
                offsets[i] = offset;
                offset += attrib.size * sizeof (GLfloat);
            }
            stride = GLsizei(offset);
        }
        constexpr int n = 64;
        GLuint vao_names[n] = {};
        GLuint vbo_names[n] = {};
        GLuint ebo_names[n] = {};
        glGenVertexArrays(n, vao_names);
        glGenBuffers(n, vbo_names);
        glGenBuffers(n, ebo_names);
        for (int i = 0; i < n; ++i) {
            GLuint vao = vao_names[i];
            GLuint vbo = vbo_names[i];
            GLuint ebo = ebo_names[i];
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            GLenum type = GL_FLOAT;
            GLboolean normalized = GL_FALSE;
            for (int i = 0; i < num_attribs; ++i) {
                const vertex_attrib& attrib = attribs[i];
                GLuint index = attrib.location;
                GLint size = attrib.size;
                std::size_t offset = offsets[i];
                glVertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<void*>(offset));
                glEnableVertexAttribArray(index);
            }
            m_unused_vertex_arrays.push({ vao, vbo, ebo }); // Throws
        }
    }
    ::gl_vertex_array vertex_array = m_unused_vertex_arrays.top();
    m_unused_vertex_arrays.pop();
    return vertex_array;
}



inline void world::return_vertex_array(::gl_vertex_array vertex_array)
{
    m_unused_vertex_arrays.push(vertex_array); // Throws
}


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
    engine_config.minimum_window_size = 128;
    engine_config.immersive_mode = true;

    ::block_basis block_basis;
    ::worker worker(logger, block_basis);

    render::Engine engine;
    world world(resource_path, locale, logger, engine, block_basis, worker);
    if (ARCHON_UNLIKELY(!engine.try_create(world, *conn, "Archon Blocks", window_size, locale, engine_config,
                                           error))) { // Throws
        logger.error("Failed to create render engine: %s", error); // Throws
        return EXIT_FAILURE;
    }

    core::ThreadGuard worker_thread;
    {
        ARCHON_SCOPE_EXIT {
            worker.quit();
        };

        core::ThreadGuard::Config thread_config;
        thread_config.thread_name = "worker";
        thread_config.block_signals = true;
        worker_thread = core::ThreadGuard([&]() {
            ARCHON_SCOPE_EXIT {
                conn->generate_quit_event();
            };
            worker.run(); // Throws
        }, thread_config); // Throws

        engine.run(); // Throws
    }
    worker_thread.join_and_rethrow(); // Throws
}
