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

#include <vector>
#include <iostream>

#include <archon/core/types.hpp>
#include <archon/core/random.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/text_table.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/math/functions.hpp>
#include <archon/image/color_space.hpp>

using namespace archon::core;
using namespace archon::util;
using namespace archon::image;


/*

  Also test with various alpha manipulations

*/


namespace {

template<class T, bool is_int> class HasError2 {};
template<class T> class HasError2<T, true> {
public:
    // Find the float type that the color space converter uses internally
    using Float = typename FastestFloatCover<T>::type;
    static T max_error()
    {
        return std::max(frac_any_to_any<Float, T>(32 * std::numeric_limits<Float>::epsilon()),
                        T(7));
    }
};
template<class T> class HasError2<T, false> {
public:
    static T max_error()
    {
        return 148 * std::numeric_limits<T>::epsilon();
    }
};
template<class T> class HasError: public HasError2<T, std::numeric_limits<T>::is_integer> {
public:
    bool operator()(T a, T b) const
    {
        return (this->max_error() < (a<b ? b-a : a-b));
    }
};


bool errors_seen = false;

Random random;

template<class T> void test(WordType word_type)
{
    for (int type = 0; type < 12; ++type) {
        ColorSpace::ConstRef c;
        switch (type) {
            case 0:
                c = ColorSpace::get_Lum();
                break;
            case 1:
                c = ColorSpace::get_RGB();
                break;
            case 2:
                c = ColorSpace::get_XYZ();
                break;
            case 3:
                c = ColorSpace::get_LAB();
                break;
            case 4:
                c = ColorSpace::get_HSV();
                break;
            case 5:
                c = ColorSpace::get_YCbCr();
                break;
            case 6:
                c = ColorSpace::get_CMYK();
                break;
            case 7:
                c = ColorSpace::new_custom(1);
                break;
            case 8:
                c = ColorSpace::new_custom(2);
                break;
            case 9:
                c = ColorSpace::new_custom(3);
                break;
            case 10:
                c = ColorSpace::new_custom(4);
                break;
            case 11:
                c = ColorSpace::new_custom(5);
                break;
        }

        // Do not test XYZ and LAB for integer types since the floating point
        // components are not confined to the unit range, so the errors are
        // necessarily too grave.
        if (  ((c == ColorSpace::get_XYZ() || c == ColorSpace::get_LAB()) &&
               !archon::image::is_floating_point(word_type)))
            continue;

        ColorSpace::ConstRef source_color_space = ColorSpace::get_RGB();
        ColorSpace::ConstRef target_color_space = c;

        if (target_color_space->get_num_primaries() < source_color_space->get_num_primaries())
            swap(source_color_space, target_color_space);

        bool source_has_alpha = false;
        bool target_has_alpha = false;

        std::string source_mnemonic = source_color_space->get_mnemonic(source_has_alpha);
        std::string target_mnemonic = target_color_space->get_mnemonic(target_has_alpha);

        const ColorSpace::Converter* forward =
            source_color_space->to_any(target_color_space, word_type, source_has_alpha, target_has_alpha);
        const ColorSpace::Converter* backward =
            target_color_space->to_any(source_color_space, word_type, target_has_alpha, source_has_alpha);

        int num_source_channels =
            source_color_space->get_num_primaries() + (source_has_alpha?1:0);
        int num_target_channels =
            target_color_space->get_num_primaries() + (target_has_alpha?1:0);

        int pixels_per_test = 100;
        int buffer_size = pixels_per_test * std::max(num_source_channels, num_target_channels);

        std::unique_ptr<T[]> buffer1 = std::make_unique<T[]>(buffer_size);
        std::unique_ptr<T[]> buffer2 = std::make_unique<T[]>(buffer_size);
        std::unique_ptr<T[]> buffer3 = std::make_unique<T[]>(buffer_size);
        bool error = false;

        for (int i = 0; i < 1000; ++i) {
            for (int j = 0; j < buffer_size; ++j)
                buffer1[j] = frac_any_to_any<double, T>(random.get_uniform());

            forward->cvt(buffer1.get(), buffer2.get(), pixels_per_test);
            backward->cvt(buffer2.get(), buffer3.get(), pixels_per_test);

            for (int j = 0; j < pixels_per_test; ++j) {
                for (int k = 0; k < num_source_channels; ++k) {
                    int index = j*num_source_channels + k;
                    if (HasError<T>()(buffer1[index], buffer3[index])) {
                        error = true;
                        break;
                    }
                }
                if (!error)
                    continue;

                errors_seen = true;
                std::cout << "\n"<<Term::AnsiAttributes::get_reverse_seq()<<"ERROR word type = "<<
                    get_word_type_name(word_type)<<", repeat index = "<<i<<", pixel index = "<<j<<
                    Term::AnsiAttributes::get_reverse_seq(false)<<std::endl;
                Text::Table table;
                table.get_col(0).set_bold();
                table.get_row(0).set_bg_color(Term::color_White);
                table.get_row(3).set_bg_color(Term::color_White);
                table.get_cell(1, 0).set_text(source_mnemonic+"-1");
                table.get_cell(2, 0).set_text(source_mnemonic+"-2");
                table.get_cell(4, 0).set_text(target_mnemonic);
                for (int k = 0; k < num_source_channels; ++k) {
                    int index = j*num_source_channels + k;
                    T a = buffer1[index], b = buffer3[index];
                    table.get_cell(0, k+1).set_text(source_color_space->get_channel_name(k));
                    table.get_cell(1, k+1).set_val(to_num(a));
                    table.get_cell(2, k+1).set_val(to_num(b));
                    if (HasError<T>()(a,b))
                        table.get_cell(1, k+1).set_bg_color(Term::color_Red);
                }
                for (int k = 0; k < num_target_channels; ++k) {
                    table.get_cell(3, k+1).set_text(target_color_space->get_channel_name(k));
                    table.get_cell(4, k+1).set_val(to_num(buffer2[j*num_target_channels + k]));
                }
                std::cout << table.print();
                break;
            }
            if (error)
                break;
        }
    }
}

template<class T, WordType type> class Test {
public:
    void operator()() const
    {
        test<T>(type);
    }
};

} // unnamed namespace


int main()
{
    WordTypeSwitch<Test, void, void> s;
    int n = get_num_word_types();
    for (int i = 0; i < n; ++i)
        s(get_word_type_by_index(i));

    if (!errors_seen)
        std::cout << "OK"<<std::endl;
}
