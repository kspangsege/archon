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

#include <cstdlib>
#include <random>
#include <string>
#include <iostream>

#include <GL/gl.h>

#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/math/vector.hpp>
#include <archon/math/quaternion.hpp>
#include <archon/math/geometry.hpp>
#include <archon/util/hashing.hpp>
#include <archon/util/named_colors.hpp>
#include <archon/display/keysyms.hpp>
#include <archon/render/scene_builder.hpp>
#include <archon/render/app.hpp>
#include <archon/core/cxx.hpp>


using namespace archon::core;
using namespace archon::math;
using namespace archon::util;
using namespace archon::font;
using namespace archon::display;
using namespace archon::render;


namespace {

// The 8 elements of the nonabelian symmetry group of the square (the
// dihedral group of order 8).
enum class SquareTransform {
    ident,        ///< No change (e).
    rot_90_cw,    ///< Rotate by 90 degrees in the clockwise direction (r).
    rot_180,      ///< Rotate by 180 degrees (rr).
    rot_90_ccw,   ///< Rotate by 90 degrees in the counterclockwise direction (rrr).
    horiz_flip,   ///< Swap left and right (s).
    diag_flip,    ///< Swap bottom-left and top-right (rs).
    vert_flip,    ///< Swap bottom and top (rrs).
    antidiag_flip ///< Swap top-left and bottom-right (rrrs).
};

template<class T> void transform_square(SquareTransform transformation,
                                        T& top_left, T& top_right,
                                        T& bottom_left, T& bottom_right)
{
    switch (transformation) {
        case SquareTransform::ident:
            break;
        case SquareTransform::rot_90_cw: {
            T tmp = std::move(bottom_left);
            bottom_left = std::move(bottom_right);
            bottom_right = std::move(top_right);
            top_right = std::move(top_left);
            top_left = std::move(tmp);
            break;
        }
        case SquareTransform::rot_180:
            std::swap(top_left, bottom_right);
            std::swap(bottom_left, top_right);
            break;
        case SquareTransform::rot_90_ccw: {
            T tmp = std::move(bottom_left);
            bottom_left = std::move(top_left);
            top_left = std::move(top_right);
            top_right = std::move(bottom_right);
            bottom_right = std::move(tmp);
            break;
        }
        case SquareTransform::horiz_flip:
            std::swap(top_left, top_right);
            std::swap(bottom_left, bottom_right);
            break;
        case SquareTransform::diag_flip:
            std::swap(bottom_left, top_right);
            break;
        case SquareTransform::vert_flip:
            std::swap(top_left, bottom_left);
            std::swap(top_right, bottom_right);
            break;
        case SquareTransform::antidiag_flip:
            std::swap(top_left, bottom_right);
            break;
    }
}


class Model {
public:
    struct Texture {
        std::string path;
    };

    struct TexRef {
        enum class Type { null, direct, indirect };
        Type type = Type::null;
        std::size_t index = 0;
    };

    struct TexVar {
        std::size_t name_index = 0;
        TexRef ref;
    };

    struct TexCoords {
        std::uint_least8_t s_1 = 0,  t_1 = 0;
        std::uint_least8_t s_2 = 16, t_2 = 16;
    };

    struct BoxFace {
        TexRef tex_ref;
        std::unique_ptr<TexCoords> tex_coords;
        SquareTransform tex_transform = SquareTransform::ident;
/*
        bool render_back_side = false;
*/
    };

    struct Box {
        std::uint_least8_t x_1 = 0,  y_1 = 0,  z_1 = 0;
        std::uint_least8_t x_2 = 16, y_2 = 16, z_2 = 16;
        std::unique_ptr<BoxFace> left_face, right_face;
        std::unique_ptr<BoxFace> bottom_face, top_face;
        std::unique_ptr<BoxFace> back_face, front_face;
    };

    using BoxList = std::vector<Box>;

    struct BlockPrototype {
        BlockPrototype* parent = nullptr;
        std::vector<TexVar> texture_variables;
        std::unique_ptr<BoxList> box_list;
    };

    struct BlockVariant {
        BlockPrototype* prototype = nullptr;
    };

    struct Block {
        std::vector<BlockVariant> variants;
    };

    std::vector<Texture> textures;
    std::vector<Block> blocks;
    std::vector<std::unique_ptr<BlockPrototype>> block_prototypes;
};


Model build_model(const std::string& assets_dir)
{
    Model model;
    Model::BlockPrototype* full_size_solid;
    {
        Model::BlockPrototype prototype;
        Model::BoxList box_list;
        Model::Box box;
        box.left_face.reset(new Model::BoxFace);
        box.left_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.left_face->tex_ref.index = 0;
        box.right_face.reset(new Model::BoxFace);
        box.right_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.right_face->tex_ref.index = 1;
        box.bottom_face.reset(new Model::BoxFace);
        box.bottom_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.bottom_face->tex_ref.index = 2;
        box.top_face.reset(new Model::BoxFace);
        box.top_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.top_face->tex_ref.index = 3;
        box.back_face.reset(new Model::BoxFace);
        box.back_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.back_face->tex_ref.index = 4;
        box.front_face.reset(new Model::BoxFace);
        box.front_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.front_face->tex_ref.index = 5;
        box_list.emplace_back(std::move(box));
        prototype.box_list.reset(new Model::BoxList(std::move(box_list)));
        model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
        full_size_solid = &*model.block_prototypes.back();
    }

    Model::BlockPrototype* full_size_solid_single_texture;
    {
        Model::BlockPrototype prototype;
        prototype.parent = full_size_solid;
        Model::TexVar left_tex_var;
        left_tex_var.name_index = 0;
        left_tex_var.ref.type = Model::TexRef::Type::indirect;
        left_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(left_tex_var));
        Model::TexVar right_tex_var;
        right_tex_var.name_index = 1;
        right_tex_var.ref.type = Model::TexRef::Type::indirect;
        right_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(right_tex_var));
        Model::TexVar bottom_tex_var;
        bottom_tex_var.name_index = 2;
        bottom_tex_var.ref.type = Model::TexRef::Type::indirect;
        bottom_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(bottom_tex_var));
        Model::TexVar top_tex_var;
        top_tex_var.name_index = 3;
        top_tex_var.ref.type = Model::TexRef::Type::indirect;
        top_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(top_tex_var));
        Model::TexVar back_tex_var;
        back_tex_var.name_index = 4;
        back_tex_var.ref.type = Model::TexRef::Type::indirect;
        back_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(back_tex_var));
        Model::TexVar front_tex_var;
        front_tex_var.name_index = 5;
        front_tex_var.ref.type = Model::TexRef::Type::indirect;
        front_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(front_tex_var));
        model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
        full_size_solid_single_texture = &*model.block_prototypes.back();
    }

    Model::BlockPrototype* full_size_solid_mirrored;
    {
        Model::BlockPrototype prototype;
        Model::BoxList box_list;
        Model::Box box;
        box.left_face.reset(new Model::BoxFace);
        box.left_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.left_face->tex_ref.index = 0;
        box.left_face->tex_coords.reset(new Model::TexCoords);
        box.left_face->tex_coords->s_1 = 16;
        box.left_face->tex_coords->s_2 = 0;
        box.right_face.reset(new Model::BoxFace);
        box.right_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.right_face->tex_ref.index = 1;
        box.right_face->tex_coords.reset(new Model::TexCoords);
        box.right_face->tex_coords->s_1 = 16;
        box.right_face->tex_coords->s_2 = 0;
        box.bottom_face.reset(new Model::BoxFace);
        box.bottom_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.bottom_face->tex_ref.index = 2;
        box.bottom_face->tex_coords.reset(new Model::TexCoords);
        box.bottom_face->tex_coords->s_1 = 16;
        box.bottom_face->tex_coords->s_2 = 0;
        box.top_face.reset(new Model::BoxFace);
        box.top_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.top_face->tex_ref.index = 3;
        box.top_face->tex_coords.reset(new Model::TexCoords);
        box.top_face->tex_coords->s_1 = 16;
        box.top_face->tex_coords->s_2 = 0;
        box.back_face.reset(new Model::BoxFace);
        box.back_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.back_face->tex_ref.index = 4;
        box.back_face->tex_coords.reset(new Model::TexCoords);
        box.back_face->tex_coords->s_1 = 16;
        box.back_face->tex_coords->s_2 = 0;
        box.front_face.reset(new Model::BoxFace);
        box.front_face->tex_ref.type = Model::TexRef::Type::indirect;
        box.front_face->tex_ref.index = 5;
        box.front_face->tex_coords.reset(new Model::TexCoords);
        box.front_face->tex_coords->s_1 = 16;
        box.front_face->tex_coords->s_2 = 0;
        box_list.emplace_back(std::move(box));
        prototype.box_list.reset(new Model::BoxList(std::move(box_list)));
        model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
        full_size_solid_mirrored = &*model.block_prototypes.back();
    }

    Model::BlockPrototype* full_size_solid_single_texture_mirrored;
    {
        Model::BlockPrototype prototype;
        prototype.parent = full_size_solid_mirrored;
        Model::TexVar left_tex_var;
        left_tex_var.name_index = 0;
        left_tex_var.ref.type = Model::TexRef::Type::indirect;
        left_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(left_tex_var));
        Model::TexVar right_tex_var;
        right_tex_var.name_index = 1;
        right_tex_var.ref.type = Model::TexRef::Type::indirect;
        right_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(right_tex_var));
        Model::TexVar bottom_tex_var;
        bottom_tex_var.name_index = 2;
        bottom_tex_var.ref.type = Model::TexRef::Type::indirect;
        bottom_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(bottom_tex_var));
        Model::TexVar top_tex_var;
        top_tex_var.name_index = 3;
        top_tex_var.ref.type = Model::TexRef::Type::indirect;
        top_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(top_tex_var));
        Model::TexVar back_tex_var;
        back_tex_var.name_index = 4;
        back_tex_var.ref.type = Model::TexRef::Type::indirect;
        back_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(back_tex_var));
        Model::TexVar front_tex_var;
        front_tex_var.name_index = 5;
        front_tex_var.ref.type = Model::TexRef::Type::indirect;
        front_tex_var.ref.index = 6;
        prototype.texture_variables.emplace_back(std::move(front_tex_var));
        model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
        full_size_solid_single_texture_mirrored = &*model.block_prototypes.back();
    }

    auto add_full_size_solid_single_texture_block = [&](std::string texture_path) {
        std::size_t texture_index;
        {
            texture_index = model.textures.size();
            Model::Texture texture;
            texture.path = assets_dir + texture_path;
            model.textures.emplace_back(std::move(texture));
        }
        Model::BlockPrototype* proto;
        {
            Model::BlockPrototype prototype;
            prototype.parent = full_size_solid_single_texture;
            Model::TexVar tex_var;
            tex_var.name_index = 6;
            tex_var.ref.type = Model::TexRef::Type::direct;
            tex_var.ref.index = texture_index;
            prototype.texture_variables.emplace_back(std::move(tex_var));
            model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
            proto = &*model.block_prototypes.back();
        }
        {
            Model::Block block;
            Model::BlockVariant variant;
            variant.prototype = proto;
            block.variants.emplace_back(std::move(variant));
            model.blocks.emplace_back(std::move(block));
        }
    };

    add_full_size_solid_single_texture_block("minecraft_textures/blocks/bedrock.png");

    // STONE
    std::size_t stone_texture_index;
    {
        stone_texture_index = model.textures.size();
        Model::Texture texture;
        texture.path = assets_dir + "minecraft_textures/blocks/stone.png";
        model.textures.emplace_back(std::move(texture));
    }
    Model::BlockPrototype* stone_proto;
    {
        Model::BlockPrototype prototype;
        prototype.parent = full_size_solid_single_texture;
        Model::TexVar tex_var;
        tex_var.name_index = 6;
        tex_var.ref.type = Model::TexRef::Type::direct;
        tex_var.ref.index = stone_texture_index;
        prototype.texture_variables.emplace_back(std::move(tex_var));
        model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
        stone_proto = &*model.block_prototypes.back();
    }
    Model::BlockPrototype* stone_mirrored_proto;
    {
        Model::BlockPrototype prototype;
        prototype.parent = full_size_solid_single_texture_mirrored;
        Model::TexVar tex_var;
        tex_var.name_index = 6;
        tex_var.ref.type = Model::TexRef::Type::direct;
        tex_var.ref.index = stone_texture_index;
        prototype.texture_variables.emplace_back(std::move(tex_var));
        model.block_prototypes.emplace_back(new Model::BlockPrototype{std::move(prototype)});
        stone_mirrored_proto = &*model.block_prototypes.back();
    }
    {
        Model::Block block;
        Model::BlockVariant variant_1;
        variant_1.prototype = stone_proto;
        block.variants.emplace_back(std::move(variant_1));
        Model::BlockVariant variant_2;
        variant_2.prototype = stone_mirrored_proto;
        block.variants.emplace_back(std::move(variant_2));
        model.blocks.emplace_back(std::move(block));
    }

    add_full_size_solid_single_texture_block("minecraft_textures/blocks/coal_ore.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/iron_ore.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/gold_ore.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/redstone_ore.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/lapis_ore.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/diamond_ore.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/emerald_ore.png");

    add_full_size_solid_single_texture_block("minecraft_textures/blocks/coal_block.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/iron_block.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/gold_block.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/redstone_block.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/lapis_block.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/diamond_block.png");
    add_full_size_solid_single_texture_block("minecraft_textures/blocks/emerald_block.png");

    add_full_size_solid_single_texture_block("minecraft_textures/blocks/dirt.png");

    return model;
}


class BlocksApp: public Application {
public:
    BlocksApp(const Config& cfg):
        Application{"archon::render::Blocks", cfg, std::locale("")},
        m_assets_dir{get_value_of(build_config_param_DataDir) + "render/test/"}
    {
        auto thrust_forwards = [this](bool down) {
            m_thrust_forwards = down;
            return false;
        };
        auto thrust_backwards = [this](bool down) {
            m_thrust_backwards = down;
            if (down)
                m_sprint_mode = false;
            return false;
        };
        auto thrust_leftwards = [this](bool down) {
            m_thrust_leftwards = down;
            return false;
        };
        auto thrust_rightwards = [this](bool down) {
            m_thrust_rightwards = down;
            return false;
        };
        auto thrust_upwards = [this](bool down) {
            m_thrust_upwards = down;
            return false;
        };
        auto thrust_downwards = [this](bool down) {
            m_thrust_downwards = down;
            if (down)
                m_sprint_mode = false;
            return false;
        };
        auto attack = [this](bool down) {
            if (down)
                std::cerr << "Attack\n";
            return false;
        };
        auto use = [this](bool down) {
            if (down)
                std::cerr << "Use\n";
            return false;
        };
        auto toggle_sprint_mode = [this](bool down) {
            m_thrust_forwards = down;
            m_sprint_mode = (m_travel_mode == TravelMode::on_ground && down);
            return false;
        };
        auto toggle_flying = [this](bool down) {
            m_thrust_upwards = down;
            if (down) {
                switch (m_travel_mode) {
                    case TravelMode::falling:
                        m_travel_mode = TravelMode::flying;
                        break;
                    case TravelMode::flying:
                        m_travel_mode = TravelMode::falling;
                        break;
                    case TravelMode::on_ground:
                        break;
                }
            }
            return false;
        };
        bind_key(KeySym_w,       std::move(thrust_forwards),   "Walk forwards");
        bind_key(KeySym_s,       std::move(thrust_backwards),  "Walk backwards");
        bind_key(KeySym_a,       std::move(thrust_leftwards),  "Strafe left");
        bind_key(KeySym_d,       std::move(thrust_rightwards), "Strafe right");
        bind_key(KeySym_space,   std::move(thrust_upwards),    "Jump");
        bind_key(KeySym_Shift_L, std::move(thrust_downwards),  "Sneak");
        bind_button(1, std::move(attack), "Attack / Destroy");
        bind_button(3, std::move(use),    "Use item / Place block");

        bind_key(KeySym_w,     KeyModifier::none, double_tap,
                 std::move(toggle_sprint_mode), "Toggle sprint");
        bind_key(KeySym_space, KeyModifier::none, double_tap,
                 std::move(toggle_flying), "Toggle flying");

        bind_key(KeySym_w, KeyModifier::shift,
                 get_builtin_key_handler(BuiltinKeyHandler::toggle_wireframe));
        bind_key(KeySym_s, KeyModifier::shift,
                 get_builtin_key_handler(BuiltinKeyHandler::toggle_status_hud));
        bind_key(KeySym_a, KeyModifier::shift,
                 get_builtin_key_handler(BuiltinKeyHandler::toggle_show_axes));
        bind_key(KeySym_space, KeyModifier::shift,
                 get_builtin_key_handler(BuiltinKeyHandler::reset_view));

        {
            m_avatar_call_list = glGenLists(1);
            OpenGlSceneBuilder scene_builder(m_avatar_call_list, get_texture_cache(),
                                             &m_texture_use);
            scene_builder.push_matrix();
            scene_builder.scale(s_texels_per_block_length);
            scene_builder.translate(-0.5, 0, -0.5);
            scene_builder.set_color(color::red);
            bool gen_texture_coords = false;
            bool has_front = true, has_back = true, has_right  = true;
            bool has_left  = true, has_top  = true, has_bottom = true;
            int x_steps = 1, y_steps = 1, z_steps = 1;
            build_unit_box(scene_builder, gen_texture_coords,
                           has_front, has_back, has_right, has_left, has_top, has_bottom,
                           x_steps, y_steps, z_steps);
            scene_builder.pop_matrix();
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_RESCALE_NORMAL);
        glEnable(GL_CULL_FACE);

        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);

//        glMaterialf(GL_FRONT, GL_SHININESS, 64);

        glColorMaterial(GL_FRONT, GL_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);

        m_random.seed(4563457);

        {
            using type = std::decay<decltype(m_randomness[0])>::type;
            m_randomness = std::make_unique<type[]>(s_randomness_size); // Throws
            std::uniform_int_distribution<type> distr;
            for (int i = 0; i < s_randomness_size; ++i)
                m_randomness[i] = distr(m_random);
        }

        init_blocks();

        m_null_chunk = std::make_unique<Chunk>();
        m_null_chunk->fill(-1);

        {
            int n_x = std::extent<decltype(m_chunks), 2>::value;
            int n_y = std::extent<decltype(m_chunks), 1>::value;
            int n_z = std::extent<decltype(m_chunks), 0>::value;
            for (int z = 0; z < n_z; ++z) {
                for (int y = 0; y < n_y; ++y) {
                    for (int x = 0; x < n_x; ++x) {
                        m_chunks[z][y][x] = std::make_unique<Chunk>();
                        Chunk& chunk = *m_chunks[z][y][x];
                        if (y < n_y/2) {
                            chunk.fill(1);
                            if (y == n_y/2 - 1) {
                                std::uniform_int_distribution<int> distr_x{0, s_num_x-1};
                                std::uniform_int_distribution<int> distr_z{0, s_num_z-1};
                                for (int i = 0; i < 12; ++i) {
                                    int x_2 = distr_x(m_random);
                                    int z_2 = distr_z(m_random);
                                    chunk.set_block(x_2, s_num_y-1, z_2, 2);
                                }
                            }
                        }
                        else {
                            chunk.fill(-1);
                        }
                        if (y == 0)
                            chunk.fill(0, 0, 0, s_num_x, 1, s_num_z, 0);
                        chunk.call_list = glGenLists(1);
                    }
                }
            }

            get_chunk(0,0,0).set_block(0,         0, s_num_z-1, 11); // Gold
            get_chunk(0,0,0).set_block(s_num_x-1, 0, 0,         13); // Lapiz
            get_chunk(0,0,0).set_block(s_num_x-1, 0, s_num_z-1, 14); // Diamond
        }
    }

    void render() override
    {
        int n = s_texels_per_block_length;
        glScaled(1.0/n, 1.0/n, 1.0/n);
        Vec3 eye_displacement{0,0,0};
        eye_displacement[1] += get_eye_height()*n;
        glTranslated(-eye_displacement[0], -eye_displacement[1], -eye_displacement[2]);
        glDisable(GL_TEXTURE_2D);
        render_avatar();
        glTranslated(-m_position[0], -m_position[1], -m_position[2]);
        glEnable(GL_TEXTURE_2D);
        glColor3d(1,1,1);
        int x = int(std::floor(m_position[0] / (s_num_x*n) - 5.5));
        int y = int(std::floor(m_position[1] / (s_num_y*n) - 5.5));
        int z = int(std::floor(m_position[2] / (s_num_z*n) - 5.5));
        for (int i_z = 0; i_z < 12; ++i_z) {
            for (int i_y = 0; i_y < 12; ++i_y) {
                for (int i_x = 0; i_x < 12; ++i_x) {
                    int x_2 = x + i_x;
                    int y_2 = y + i_y;
                    int z_2 = z + i_z;
                    Chunk& chunk = get_chunk(x_2, y_2, z_2);
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
    }

    bool tick(clock::time_point) override
    {
        double delta_time = 1.0/60; // In seconds

        double texels_per_meter = s_texels_per_block_length;

        // a = wa - wr*v
        // v = v + a * dt
        // x = x + v * dt

        // a_sn = r_g * v_sn
        // a_w  = r_g * v_w
        // a_sp = r_g * v_sp
        // a_fl = r_a * v_fl

        // a_sn = r_g * 1.3m/s
        // a_w  = r_g * 4.3m/s
        // a_sp = r_g * 5.6m/s
        // a_w  = r_a * 10.9m/s
        // a_g  = r_a * 78.4m/s = 32m/s^2

        // b = a_g*dt^2 - r_a*v*dt^2
        // w = w + b
        // x = x + w

        // w = v * dt
        // v = w / dt
        // b = a * dt^2
        // a = b / dt^2

        // a_g*dt^2 = 0.08m
        // a_g = 0.08m / (1s^2/400) = 32m/s^2

        // r_a = 32m/s^2 / 78.4m/s = 0.41/s
        // a_w = 32m/s^2 / 78.4m/s * 10.9m/s = 4.45m/s^2
        // r_g = 32m/s^2 / 78.4m/s * 10.9m/s / 4.3m/s = 1.0346464Hz
        // t_h_g = ln(2) / (32m/s^2 / 78.4m/s * 10.9m/s / 4.3m/s) = 0.67s

        // w = w * (1 - r_a*dt)

        // Fixed drive accel starting from zero speed
        // v(t) = v_t * (1 - e^(-r_a*t))
        // v_t = a_g / r_a

        // Zero drive accel starting from nonzero speed
        // dv/dt = -r_a*v
        // v(t) = v_0 * e^(-r_a*t)

        // Fixed drive accel starting from nonzero speed
        // dv(t)/dt = g - d*v(t)
        // v(t) = k*e^(-d*t) + g/d
        // v(0) = v_0
        // k = v_0 - v_t
        // v_t = g / d
        // v(t) = (v_0 - v_t)*e^(-d*t) + v_t
        //
        // v(t_0) = 0
        // v_t = (v_t - v_0)*e^(-d*t_0)          Solve[w = (w - v)*e^(-d*t), t]
        // t_0 = ln(1-v_0/v_t) / d
        //
        // x(t) = e^(-d*t) * (v_t-v_0)/d + v_t*t + k     Solve[0 = (w-v)/d + k, k]
        // x(0) = 0
        // 0 = (v_t-v_0)/d + k
        // k = (v_0-v_t)/d
        // x(t) = (v_t-v_0)/d * e^(-d*t) - (v_t-v_0)/d + v_t*t
        // x(t) = (v_t-v_0)/d * (e^(-d*t)-1) + v_t*t

        // x(t_0) = x_0
        // x_0 = (v_t-v_0)/d * (e^(-d*t_0)-1) + v_t*t_0
        // x_0 = (v_t-v_0)/d * (e^(-d*(ln(1-v_0/v_t) / d)) - 1) + v_t*t_0
        // x_0 = (v_t-v_0)/d * (e^(-ln(1-v_0/v_t)) - 1) + v_t*t_0
        // x_0 = (v_t-v_0)/d * (v_t / (v_t - v_0) - 1) + v_t*t_0
        // x_0 = (v_t-v_0)/d * (v_t / (v_t - v_0) - (v_t - v_0)/(v_t - v_0)) + v_t*t_0
        // x_0 = (v_t-v_0)/d * (v_t - (v_t - v_0)) / (v_t - v_0) + v_t*t_0
        // x_0 = (v_t-v_0)/d * v_0 / (v_t - v_0) + v_t*t_0
        // x_0 = v_0/d + v_t*t_0
        // x_0 = v_0/d - v_t*ln(1-v_0/v_t) / d
        // x_0*d = v_0 - v_t*ln(1-v_0/v_t)   Solve[x*d = v - w*log(1-v/w), v]
        // v_t*ln(1-v_0/v_t) = v_0 - x_0*d
        // ln(1-v_0/v_t) = v_0/v_t - x_0/v_t*d

        // ln(1-v_0/v_t) = v_0/v_t - x_0*d^2/g

        // ln(1-u) = u - k
        // 1-u = e^(u - k)    Solve[1 - u = e^(u-k) && u < 0 && k < 0, u]
        // u = 1-W(e^(1-k))
        // 1-u = W(e^(1-k))

        // y = x*e^x
        // x = W(y)

        // v_0 = (1 - W(e^(1-x_0*d^2/g))) * v_t    [(1 - ProductLog[e^(1-1.25*(log(2)/1.7)^2/-32)]) * -78.4]


        // t_h = ln(2) / r_a  =  1.6982s

        // r_a = ln(2) / t_h

        // Half lives, in seconds, of exponential velocity decays in various
        // media.
        constexpr double ground_speed_half_life = 0.67 / 16;
        constexpr double air_speed_half_life    = 1.7;

        // Terminal velocities, in meters per second, for various modes of
        // locomotion.
        constexpr double sneak_terminal_velocity     =  1.3; // Thrust (horizontal)
        constexpr double walk_terminal_velocity      =  4.3; // Thrust (horizontal)
        constexpr double sprint_terminal_velocity    =  5.6; // Thrust (horizontal)
        constexpr double fly_vert_terminal_velocity  =  7.8; // Thrust (vertical)
        constexpr double fly_horiz_terminal_velocity = 10.9; // Thrust (horizontal)
        constexpr double fall_terminal_velocity      = 78.4; // Gravity (vertical)

        // Exponential velocity decay constants in inverse seconds.
        constexpr double ground_velocity_decay = std::log(2) / ground_speed_half_life;
        constexpr double air_velocity_decay    = std::log(2) / air_speed_half_life;

        Vec3 accel{0,0,0}; // In texels (16th of a meter) per square second
        double decay = air_velocity_decay;
        switch (m_travel_mode) {
            case TravelMode::falling:
            case TravelMode::flying:
                break;
            case TravelMode::on_ground:
                decay = ground_velocity_decay;
                break;
        }
        accel -= decay * m_velocity; // Exponential decay

        if (m_travel_mode != TravelMode::flying)
            accel[1] -= air_velocity_decay * fall_terminal_velocity * texels_per_meter; // Gravity

        Vec2 thrust{0,0};
        if (m_thrust_forwards)
            thrust[0] += 1;
        if (m_thrust_backwards)
            thrust[0] -= 1;
        if (m_thrust_leftwards)
            thrust[1] += 1;
        if (m_thrust_rightwards)
            thrust[1] -= 1;
        if (thrust != Vec2::zero()) {
            thrust.unit(); // Normalize

            double terminal_velocity = walk_terminal_velocity;
            switch (m_travel_mode) {
                case TravelMode::falling:
                    break;
                case TravelMode::flying:
                    if (m_sprint_mode) {
                       terminal_velocity = sprint_terminal_velocity *
                           (fly_horiz_terminal_velocity / walk_terminal_velocity); // FIXME: Check with Minecraft
                    }
                    else {
                        terminal_velocity = fly_horiz_terminal_velocity;
                    }
                    break;
                case TravelMode::on_ground:
                    if (m_thrust_downwards) {
                        terminal_velocity = sneak_terminal_velocity;
                    }
                    else if (m_sprint_mode) {
                        terminal_velocity = sprint_terminal_velocity;
                    }
                    break;
            }
            thrust *= (decay * terminal_velocity) * texels_per_meter;

            Rotation3 orientation = get_view_orientation();
            Quaternion q{orientation};

            Vec2 direction;
            direction[0] = -2 * (q.v[2]*q.v[2] + q.v[1]*q.v[1]) + 1;
            direction[1] = 2 * (q.w*q.v[1] - q.v[0]*q.v[2]);
            double s = sq_sum(direction);
            if (s > 0.01) {
                direction /= std::sqrt(s);
                Mat2 rot;
                rot.col(0) = direction;
                rot.col(1) = perp(direction);
                thrust = rot * thrust;
                accel[2] -= thrust[0];
                accel[0] -= thrust[1];
            }
        }

        if (m_travel_mode == TravelMode::flying) {
            double vert_thrust = 0;
            if (m_thrust_upwards)
                vert_thrust += 1;
            if (m_thrust_downwards)
                vert_thrust -= 1;
            vert_thrust *= (decay * fly_vert_terminal_velocity) * texels_per_meter;
            accel[1] += vert_thrust;
        }

        m_velocity += accel * delta_time;

        if (m_thrust_upwards && m_travel_mode == TravelMode::on_ground) {
            double jump_veclocity = 10; // In meters per second
            m_velocity[1] += jump_veclocity * texels_per_meter;
        }

        // FIXME: Snap velocity to zero when its magnitude gets below a certain threshold

        // Minecraft: Every tick (1/20 second), non-flying players and mobs have
        // their vertical speed decremented (less upward motion, more downward
        // motion) by 0.08 blocks per tick (1.6 m/s), then multiplied by
        // 0.98. This would produce a terminal velocity of 3.92 blocks per tick,
        // or 78.4 m/s.

        int n = s_texels_per_block_length;
        Box3 moving_box_1{Vec3{-0.5*n, 0, -0.5*n}, Vec3{0.5*n, 1.0*n, 0.5*n}};
        Box3 moving_box_2 = moving_box_1;
        moving_box_2.translate(m_position);

        Vec3 delta_position = m_velocity * delta_time;
        m_position += delta_position;

        if (m_travel_mode == TravelMode::on_ground)
            m_travel_mode = TravelMode::falling;

        int n_x = std::extent<decltype(m_chunks), 2>::value;
        int n_y = std::extent<decltype(m_chunks), 1>::value;
        int n_z = std::extent<decltype(m_chunks), 0>::value;
        Box3 static_box(Vec3{-0.5*n_x*s_num_x*n, -0.5*n_y*s_num_y*n, -0.5*n_z*s_num_z*n},
                        Vec3{+0.5*n_x*s_num_x*n, +0.0*n_y*s_num_y*n, +0.5*n_z*s_num_z*n});
        double time;
        BoxFace static_face;
        if (check_collision(moving_box_2, delta_position, static_box, time, static_face)) {
//            std::cerr << "Hit " << time << " : " << int(static_face) << "\n";
            switch (static_face) {
                case BoxFace::left:
                    m_position[0] = static_box.lower[0] - moving_box_1.upper[0];
                    m_velocity[0] = 0;
                    break;
                case BoxFace::right:
                    m_position[0] = static_box.upper[0] - moving_box_1.lower[0];
                    m_velocity[0] = 0;
                    break;
                case BoxFace::bottom:
                    m_position[1] = static_box.lower[1] - moving_box_1.upper[1];
                    m_velocity[1] = 0;
                    break;
                case BoxFace::top:
                    m_position[1] = static_box.upper[1] - moving_box_1.lower[1];
                    m_velocity[1] = 0;
                    m_travel_mode = TravelMode::on_ground;
                    break;
                case BoxFace::back:
                    m_position[2] = static_box.lower[2] - moving_box_1.upper[2];
                    m_velocity[2] = 0;
                    break;
                case BoxFace::front:
                    m_position[2] = static_box.upper[2] - moving_box_1.lower[2];
                    m_velocity[2] = 0;
                    break;
            }
        }

//        std::cerr << (len(m_velocity)/texels_per_meter) << "\n";

        return true;
    }

private:
    enum class BoxFace { left, right, bottom, top, back, front };

    // Chunk size in number of blocks
    static constexpr int s_num_x = 16;
    static constexpr int s_num_y = 16;
    static constexpr int s_num_z = 16;

    static constexpr int s_texels_per_block_length = 16;

    // In texels (16th of a block length)
    Vec3 m_position{0,0,0}; // Position of feet
    Vec3 m_velocity{0,0,0};

    static const int s_randomness_size = 256;
    std::unique_ptr<std::uint_least64_t[]> m_randomness;

    enum TravelMode { falling, flying, on_ground };
    TravelMode m_travel_mode = TravelMode::falling;

    bool m_thrust_forwards = false, m_thrust_backwards = false;
    bool m_thrust_leftwards = false, m_thrust_rightwards = false;
    bool m_thrust_upwards = false, m_thrust_downwards = false;
    bool m_sprint_mode = false;

    GLuint m_avatar_call_list = 0;

    std::vector<TextureUse> m_texture_use;

    std::string m_assets_dir;

    struct Block {
        // One plus the index in m_block_variants of the last variant of this
        // block. The index of the first variant is `prev.variants_end`, where
        // `prev` is the previous entry in m_blocks, or zero if this is the
        // first entry in m_blocks.
        std::size_t variants_end;
    };

    struct BlockVariant {
        // One plus the index in m_quads of the last quad of this block
        // variant. The index of the first quad is `prev.quads_end`, where
        // `prev` is the previous entry in m_block_variants, or zero if this is
        // the first entry in m_block_variants.
        std::size_t quads_end;
    };

    // Spatial coordinates are in 16th of a block units
    struct Quad {
        BoxFace orientation;
        GLuint texture;
        GLfloat s_1, t_1, x_1, y_1, z_1;
        GLfloat s_2, t_2, x_2, y_2, z_2;
        GLfloat s_3, t_3, x_3, y_3, z_3;
        GLfloat s_4, t_4, x_4, y_4, z_4;
    };

    class Chunk {
    public:
        bool dirty = true;
        GLuint call_list = 0;

        int get_block(int x, int y, int z)
        {
            return int(m_blocks[z][y][x]) - 1;
        }

        void set_block(int x, int y, int z, int i)
        {
            m_blocks[z][y][x] = std::uint_least8_t(1 + i);
        }

        void fill(int i)
        {
            fill(0, 0, 0, s_num_x, s_num_y, s_num_z, i);
        }

        void fill(int x_1, int y_1, int z_1, int x_2, int y_2, int z_2, int i)
        {
            std::uint_least8_t i_2 = std::uint_least8_t(1 + i);
            for (int z = z_1; z < z_2; ++z) {
                for (int y = y_1; y < y_2; ++y) {
                    for (int x = x_1; x < x_2; ++x)
                        m_blocks[z][y][x] = i_2;
                }
            }
        }

    private:
        std::uint_least8_t m_blocks[s_num_z][s_num_y][s_num_x];
    };

    std::vector<Block> m_blocks;
    std::vector<BlockVariant> m_block_variants;
    std::vector<Quad> m_quads;
    std::unique_ptr<Chunk> m_chunks[64][4][64];
    std::unique_ptr<Chunk> m_null_chunk;

    std::mt19937_64 m_random;

    Chunk& get_chunk(int x, int y, int z)
    {
        int n_x = std::extent<decltype(m_chunks), 2>::value;
        int n_y = std::extent<decltype(m_chunks), 1>::value;
        int n_z = std::extent<decltype(m_chunks), 0>::value;
        int x_2 = n_x / 2 + x;
        int y_2 = n_y / 2 + y;
        int z_2 = n_z / 2 + z;
        bool out_of_bounds = ((x_2 < 0 || x_2 >= n_x) ||
                              (y_2 < 0 || y_2 >= n_y) ||
                              (z_2 < 0 || z_2 >= n_z));
        if (out_of_bounds)
            return *m_null_chunk;
        return *m_chunks[z_2][y_2][x_2];
    }

    void init_blocks()
    {
        Model model = build_model(m_assets_dir);
        auto load_texture = [this](const std::string& path) {
            TextureCache& texture_cache = get_texture_cache();
            GLenum wrap_s = GL_REPEAT, wrap_t = GL_REPEAT;
            TextureCache::FilterMode filter_mode = TextureCache::FilterMode::nearest;
            TextureDecl decl = texture_cache.declare(path, wrap_s, wrap_t, filter_mode); // Throws
            TextureUse use = decl.acquire(); // Throws
            GLuint texture_name = use.get_gl_name();
            m_texture_use.emplace_back(std::move(use)); // Throws
            return texture_name;
        };
        std::map<std::size_t, GLuint> texture_names;
        std::size_t num_textures = model.textures.size();
        for (std::size_t i = 0; i < num_textures; ++i) {
            const Model::Texture& tex = model.textures[i];
            GLuint texture_name = load_texture(tex.path); // Throws
            texture_names[i] = texture_name;
        }
        GLuint default_texture_name = 0;
        for (const Model::Block& block: model.blocks) {
            m_blocks.emplace_back(Block{m_block_variants.size()});
            for (const Model::BlockVariant& variant: block.variants) {
                m_block_variants.emplace_back(BlockVariant{m_quads.size()});
                ++m_blocks.back().variants_end;
                Model::BoxList* box_list = nullptr;
                Model::BlockPrototype* prototype = variant.prototype;
                std::map<std::size_t, Model::TexRef> textures;
                while (prototype) {
                    for (const Model::TexVar& tex_var: prototype->texture_variables) {
                        auto p = textures.emplace(tex_var.name_index, Model::TexRef());
                        bool was_inserted = p.second;
                        if (was_inserted)
                            p.first->second = tex_var.ref;
                    }
                    if (!box_list)
                        box_list = prototype->box_list.get();
                    prototype = prototype->parent;
                }
                if (!box_list)
                    continue;
                auto add_quad = [&](const Model::BoxFace& face, Quad& quad) {
                    Model::TexRef tex_ref = face.tex_ref;
                    while (tex_ref.type == Model::TexRef::Type::indirect) {
                        auto i = textures.find(tex_ref.index);
                        if (i == textures.end())
                            throw std::runtime_error("Undefined indirect texture reference");
                        tex_ref = i->second;
                    }
                    if (tex_ref.type == Model::TexRef::Type::direct) {
                        auto i = texture_names.find(tex_ref.index);
                        if (i == texture_names.end())
                            throw std::runtime_error("Undefined direct texture reference");
                        quad.texture = i->second;
                    }
                    else {
                        if (default_texture_name == 0) {
                            std::string path = m_assets_dir + "default.png";
                            default_texture_name = load_texture(path); // Throws
                        }
                        quad.texture = default_texture_name;
                    }
                    if (face.tex_coords) {
                        quad.s_1 = float(face.tex_coords->s_1) / 16;
                        quad.t_1 = float(face.tex_coords->t_1) / 16;
                        quad.s_2 = float(face.tex_coords->s_2) / 16;
                        quad.t_2 = float(face.tex_coords->t_1) / 16;
                        quad.s_3 = float(face.tex_coords->s_2) / 16;
                        quad.t_3 = float(face.tex_coords->t_2) / 16;
                        quad.s_4 = float(face.tex_coords->s_1) / 16;
                        quad.t_4 = float(face.tex_coords->t_2) / 16;
                    }
                    auto bottom_left  = std::make_tuple(quad.s_1, quad.t_1);
                    auto bottom_right = std::make_tuple(quad.s_2, quad.t_2);
                    auto top_right    = std::make_tuple(quad.s_3, quad.t_3);
                    auto top_left     = std::make_tuple(quad.s_4, quad.t_4);
                    transform_square(face.tex_transform,
                                     top_left,    top_right,
                                     bottom_left, bottom_right);
                    std::tie(quad.s_1, quad.t_1) = bottom_left;
                    std::tie(quad.s_2, quad.t_2) = bottom_right;
                    std::tie(quad.s_3, quad.t_3) = top_right;
                    std::tie(quad.s_4, quad.t_4) = top_left;
                    m_quads.emplace_back(std::move(quad)); // Throws
                    ++m_block_variants.back().quads_end;
                };
                for (const Model::Box& box: *box_list) {
                    if (box.left_face) {
                        Quad quad;
                        quad.orientation = BoxFace::left;
                        // Bottom left
                        quad.s_1 = float(box.z_1) / 16;
                        quad.t_1 = float(box.y_1) / 16;
                        quad.x_1 = float(box.x_1);
                        quad.y_1 = float(box.y_1);
                        quad.z_1 = float(box.z_1);
                        // Bottom right
                        quad.s_2 = float(box.z_2) / 16;
                        quad.t_2 = float(box.y_1) / 16;
                        quad.x_2 = float(box.x_1);
                        quad.y_2 = float(box.y_1);
                        quad.z_2 = float(box.z_2);
                        // Top right
                        quad.s_3 = float(box.z_2) / 16;
                        quad.t_3 = float(box.y_2) / 16;
                        quad.x_3 = float(box.x_1);
                        quad.y_3 = float(box.y_2);
                        quad.z_3 = float(box.z_2);
                        // Top left
                        quad.s_4 = float(box.z_1) / 16;
                        quad.t_4 = float(box.y_2) / 16;
                        quad.x_4 = float(box.x_1);
                        quad.y_4 = float(box.y_2);
                        quad.z_4 = float(box.z_1);
                        add_quad(*box.left_face, quad);
                    }
                    if (box.right_face) {
                        Quad quad;
                        quad.orientation = BoxFace::right;
                        // Bottom left
                        quad.s_1 = float(16 - box.z_2) / 16;
                        quad.t_1 = float(box.y_1) / 16;
                        quad.x_1 = float(box.x_2);
                        quad.y_1 = float(box.y_1);
                        quad.z_1 = float(box.z_2);
                        // Bottom right
                        quad.s_2 = float(16 - box.z_1) / 16;
                        quad.t_2 = float(box.y_1) / 16;
                        quad.x_2 = float(box.x_2);
                        quad.y_2 = float(box.y_1);
                        quad.z_2 = float(box.z_1);
                        // Top right
                        quad.s_3 = float(16 - box.z_1) / 16;
                        quad.t_3 = float(box.y_2) / 16;
                        quad.x_3 = float(box.x_2);
                        quad.y_3 = float(box.y_2);
                        quad.z_3 = float(box.z_1);
                        // Top left
                        quad.s_4 = float(16 - box.z_2) / 16;
                        quad.t_4 = float(box.y_2) / 16;
                        quad.x_4 = float(box.x_2);
                        quad.y_4 = float(box.y_2);
                        quad.z_4 = float(box.z_2);
                        add_quad(*box.right_face, quad);
                    }
                    if (box.bottom_face) {
                        Quad quad;
                        quad.orientation = BoxFace::bottom;
                        // Bottom left
                        quad.s_1 = float(box.x_1) / 16;
                        quad.t_1 = float(box.z_1) / 16;
                        quad.x_1 = float(box.x_1);
                        quad.y_1 = float(box.y_1);
                        quad.z_1 = float(box.z_1);
                        // Bottom right
                        quad.s_2 = float(box.x_2) / 16;
                        quad.t_2 = float(box.z_1) / 16;
                        quad.x_2 = float(box.x_2);
                        quad.y_2 = float(box.y_1);
                        quad.z_2 = float(box.z_1);
                        // Top right
                        quad.s_3 = float(box.x_2) / 16;
                        quad.t_3 = float(box.z_2) / 16;
                        quad.x_3 = float(box.x_2);
                        quad.y_3 = float(box.y_1);
                        quad.z_3 = float(box.z_2);
                        // Top left
                        quad.s_4 = float(box.x_1) / 16;
                        quad.t_4 = float(box.z_2) / 16;
                        quad.x_4 = float(box.x_1);
                        quad.y_4 = float(box.y_1);
                        quad.z_4 = float(box.z_2);
                        add_quad(*box.bottom_face, quad);
                    }
                    if (box.top_face) {
                        Quad quad;
                        quad.orientation = BoxFace::top;
                        // Bottom left
                        quad.s_1 = float(box.x_1) / 16;
                        quad.t_1 = float(16 - box.z_2) / 16;
                        quad.x_1 = float(box.x_1);
                        quad.y_1 = float(box.y_2);
                        quad.z_1 = float(box.z_2);
                        // Bottom right
                        quad.s_2 = float(box.x_2) / 16;
                        quad.t_2 = float(16 - box.z_2) / 16;
                        quad.x_2 = float(box.x_2);
                        quad.y_2 = float(box.y_2);
                        quad.z_2 = float(box.z_2);
                        // Top right
                        quad.s_3 = float(box.x_2) / 16;
                        quad.t_3 = float(16 - box.z_1) / 16;
                        quad.x_3 = float(box.x_2);
                        quad.y_3 = float(box.y_2);
                        quad.z_3 = float(box.z_1);
                        // Top left
                        quad.s_4 = float(box.x_1) / 16;
                        quad.t_4 = float(16 - box.z_1) / 16;
                        quad.x_4 = float(box.x_1);
                        quad.y_4 = float(box.y_2);
                        quad.z_4 = float(box.z_1);
                        add_quad(*box.top_face, quad);
                    }
                    if (box.back_face) {
                        Quad quad;
                        quad.orientation = BoxFace::back;
                        // Bottom left
                        quad.s_1 = float(16 - box.x_2) / 16;
                        quad.t_1 = float(box.y_1) / 16;
                        quad.x_1 = float(box.x_2);
                        quad.y_1 = float(box.y_1);
                        quad.z_1 = float(box.z_1);
                        // Bottom right
                        quad.s_2 = float(16 - box.x_1) / 16;
                        quad.t_2 = float(box.y_1) / 16;
                        quad.x_2 = float(box.x_1);
                        quad.y_2 = float(box.y_1);
                        quad.z_2 = float(box.z_1);
                        // Top right
                        quad.s_3 = float(16 - box.x_1) / 16;
                        quad.t_3 = float(box.y_2) / 16;
                        quad.x_3 = float(box.x_1);
                        quad.y_3 = float(box.y_2);
                        quad.z_3 = float(box.z_1);
                        // Top left
                        quad.s_4 = float(16 - box.x_2) / 16;
                        quad.t_4 = float(box.y_2) / 16;
                        quad.x_4 = float(box.x_2);
                        quad.y_4 = float(box.y_2);
                        quad.z_4 = float(box.z_1);
                        add_quad(*box.back_face, quad);
                    }
                    if (box.front_face) {
                        Quad quad;
                        quad.orientation = BoxFace::front;
                        // Bottom left
                        quad.s_1 = float(box.x_1) / 16;
                        quad.t_1 = float(box.y_1) / 16;
                        quad.x_1 = float(box.x_1);
                        quad.y_1 = float(box.y_1);
                        quad.z_1 = float(box.z_2);
                        // Bottom right
                        quad.s_2 = float(box.x_2) / 16;
                        quad.t_2 = float(box.y_1) / 16;
                        quad.x_2 = float(box.x_2);
                        quad.y_2 = float(box.y_1);
                        quad.z_2 = float(box.z_2);
                        // Top right
                        quad.s_3 = float(box.x_2) / 16;
                        quad.t_3 = float(box.y_2) / 16;
                        quad.x_3 = float(box.x_2);
                        quad.y_3 = float(box.y_2);
                        quad.z_3 = float(box.z_2);
                        // Top left
                        quad.s_4 = float(box.x_1) / 16;
                        quad.t_4 = float(box.y_2) / 16;
                        quad.x_4 = float(box.x_1);
                        quad.y_4 = float(box.y_2);
                        quad.z_4 = float(box.z_2);
                        add_quad(*box.front_face, quad);
                    }
                }
            }
        }
    }

    void update_chunk_call_list(Chunk& chunk, int chunk_x, int chunk_y, int chunk_z)
    {
        Chunk& left_chunk   = get_chunk(chunk_x - 1, chunk_y, chunk_z);
        Chunk& right_chunk  = get_chunk(chunk_x + 1, chunk_y, chunk_z);
        Chunk& bottom_chunk = get_chunk(chunk_x, chunk_y - 1, chunk_z);
        Chunk& top_chunk    = get_chunk(chunk_x, chunk_y + 1, chunk_z);
        Chunk& back_chunk   = get_chunk(chunk_x, chunk_y, chunk_z - 1);
        Chunk& front_chunk  = get_chunk(chunk_x, chunk_y, chunk_z + 1);
        glNewList(chunk.call_list, GL_COMPILE);
        GLuint texture_name = 0;
        int i = 0;
        for (int i_z = 0; i_z < s_num_z; ++i_z) {
            for (int i_y = 0; i_y < s_num_y; ++i_y) {
                for (int i_x = 0; i_x < s_num_x; ++i_x, ++i) {
                    int v = chunk.get_block(i_x, i_y, i_z);
                    if (v == -1)
                        continue; // Air
                    float x = i_x * 16;
                    float y = i_y * 16;
                    float z = i_z * 16;
                    auto i_2 = v % m_blocks.size();
                    const Block& block = m_blocks[i_2];
                    auto variants_begin = (i_2 == 0 ? 0 : m_blocks[i_2-1].variants_end);
                    auto variants_end = block.variants_end;
                    auto num_variants = variants_end - variants_begin;
                    ARCHON_ASSERT(num_variants >= 1);
                    auto j = variants_begin;
                    if (num_variants > 1)
                        j = variants_begin + hash(i, num_variants);
                    const BlockVariant& variant = m_block_variants[j];
                    auto quads_begin = (j == 0 ? 0 : m_block_variants[j-1].quads_end);
                    auto quads_end = variant.quads_end;
                    for (auto k = quads_begin; k < quads_end; ++k) {
                        const Quad& q = m_quads[k];
                        int v_2 = -1;
                        GLfloat n_x = 0, n_y = 0, n_z = 0;
                        switch (q.orientation) {
                            case BoxFace::left:
                                if (i_x == 0) {
                                    v_2 = left_chunk.get_block(s_num_x - 1, i_y, i_z);
                                }
                                else {
                                    v_2 = chunk.get_block(i_x - 1, i_y, i_z);
                                }
                                n_x = -1;
                                break;
                            case BoxFace::right:
                                if (i_x == s_num_x - 1) {
                                    v_2 = right_chunk.get_block(0, i_y, i_z);
                                }
                                else {
                                    v_2 = chunk.get_block(i_x + 1, i_y, i_z);
                                }
                                n_x = 1;
                                break;
                            case BoxFace::bottom:
                                if (i_y == 0) {
                                    v_2 = bottom_chunk.get_block(i_x, s_num_y - 1, i_z);
                                }
                                else {
                                    v_2 = chunk.get_block(i_x, i_y - 1, i_z);
                                }
                                n_y = -1;
                                break;
                            case BoxFace::top:
                                if (i_y == s_num_y - 1) {
                                    v_2 = top_chunk.get_block(i_x, 0, i_z);
                                }
                                else {
                                    v_2 = chunk.get_block(i_x, i_y + 1, i_z);
                                }
                                n_y = 1;
                                break;
                            case BoxFace::back:
                                if (i_z == 0) {
                                    v_2 = back_chunk.get_block(i_x, i_y, s_num_z - 1);
                                }
                                else {
                                    v_2 = chunk.get_block(i_x, i_y, i_z - 1);
                                }
                                n_z = -1;
                                break;
                            case BoxFace::front:
                                if (i_z == s_num_z - 1) {
                                    v_2 = front_chunk.get_block(i_x, i_y, 0);
                                }
                                else {
                                    v_2 = chunk.get_block(i_x, i_y, i_z + 1);
                                }
                                n_z = 1;
                                break;
                        }
                        if (v_2 != -1)
                            continue; // Not air
                        if (q.texture != texture_name) {
                            if (texture_name != 0)
                                glEnd();
                            texture_name = q.texture;
                            glBindTexture(GL_TEXTURE_2D, texture_name);
                            glBegin(GL_QUADS);
                        }
                        glNormal3f(n_x, n_y, n_z);
                        glTexCoord2f(q.s_1, q.t_1);
                        glVertex3f(x + q.x_1, y + q.y_1, z + q.z_1);
                        glTexCoord2f(q.s_2, q.t_2);
                        glVertex3f(x + q.x_2, y + q.y_2, z + q.z_2);
                        glTexCoord2f(q.s_3, q.t_3);
                        glVertex3f(x + q.x_3, y + q.y_3, z + q.z_3);
                        glTexCoord2f(q.s_4, q.t_4);
                        glVertex3f(x + q.x_4, y + q.y_4, z + q.z_4);
                    }
                }
            }
        }
        if (texture_name != 0)
            glEnd();
        glEndList();
        chunk.dirty = false;
    }

    // n must be greater than or equal to 1, and less than or equal to 0xFFFF
    int hash(int i, int n)
    {
        int p_1 = 239;
        int p_2 = 251;
        int i_1 = i % p_1;
        int i_2 = (i+1) % p_2;
        auto v_1 = m_randomness[i_1];
        auto v_2 = m_randomness[i_2];
        auto v = v_1 ^ v_2;
        return int(v % n);
    }

    void render_avatar()
    {
        glCallList(m_avatar_call_list);
    }

    // In meters above feet
    double get_eye_height() const noexcept
    {
        constexpr double normal_height = 1.62;
        constexpr double sneak_height  = 1.54;
        if (m_thrust_downwards)
            return sneak_height;
        return normal_height;
    }

    bool check_collision(const Box3& moving_box, const Vec3& displacement, const Box3& static_box,
                         double& time, BoxFace& static_face)
    {
        BoxFace faces[3] { BoxFace::left, BoxFace::bottom, BoxFace::back };
        Vec3 t_0, t_1;
        t_0[0] = (static_box.lower[0] - moving_box.upper[0]) / displacement[0];
        t_1[0] = (static_box.upper[0] - moving_box.lower[0]) / displacement[0];
        if (t_0[0] > t_1[0]) {
            std::swap(t_0[0], t_1[0]);
            faces[0] = BoxFace::right;
        }
        t_0[1] = (static_box.lower[1] - moving_box.upper[1]) / displacement[1];
        t_1[1] = (static_box.upper[1] - moving_box.lower[1]) / displacement[1];
        if (t_0[1] > t_1[1]) {
            std::swap(t_0[1], t_1[1]);
            faces[1] = BoxFace::top;
        }
        t_0[2] = (static_box.lower[2] - moving_box.upper[2]) / displacement[2];
        t_1[2] = (static_box.upper[2] - moving_box.lower[2]) / displacement[2];
        if (t_0[2] > t_1[2]) {
            std::swap(t_0[2], t_1[2]);
            faces[2] = BoxFace::front;
        }

        int i;
        if (t_0[0] > t_0[1]) {
            if (t_0[0] > t_0[2]) {
                i = 0; // X-axis
            }
            else {
                i = 2; // Z-axis
            }
        }
        else {
             if (t_0[1] > t_0[2]) {
                i = 1; // Y-axis
            }
            else {
                i = 2; // Z-axis
            }
        }

        if (t_0[i] < 0)
            return false; // There may have been a collision in the past
        if (t_0[i] >= 1)
            return false; // There may be a collision in the future
        if (t_0[i] > min3(t_1[0], t_1[1], t_1[2]))
            return false; // No concurrent overlap on all 3 axes

        time = t_0[i];
        static_face = faces[i];
        return true;
    }
};

} // unnamed namespace


int main(int argc, const char* argv[])
{
    std::set_terminate(&cxx::terminate_handler);
    try_fix_preinstall_datadir(argv[0], "render/test/");

    Application::Config cfg;
    CommandlineOptions opts;
    opts.add_help("Blocks test application.");
    opts.check_num_args(0,0);
    opts.add_stop_opts();
    opts.add_group(cfg);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    BlocksApp app{cfg};
    app.run();
}
