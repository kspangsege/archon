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

#include <stdexcept>
#include <limits>
#include <vector>
#include <sstream>
#include <iostream>

#include <archon/core/memory.hpp>
#include <archon/core/text_table.hpp>
#include <archon/image/integer_buffer_format.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::Core;
using namespace archon::Imaging;


namespace
{
  int endianness_vec2int(vector<bool> const &endianness, int bytes_per_word)
  {
    int const levels = find_most_sig_bit(bytes_per_word);
    int compact_endianness = 0;
    vector<bool> const &end = endianness.empty() ? native_endianness : endianness;
    int const max = end.size()-1;
    for(int i=0; i<levels; ++i) if(end[min(i, max)]) compact_endianness |= 1 << i;
    return compact_endianness;
  }


  vector<bool> endianness_int2vec(int compact_endianness, int bytes_per_word)
  {
    vector<bool> endianness;
    if(0 <= compact_endianness &&
       compact_endianness != endianness_vec2int(native_endianness, bytes_per_word))
    {
      int const levels = find_most_sig_bit(bytes_per_word);
      for(int i=0; i<levels; ++i) endianness.push_back(compact_endianness & 1 << i);
    }
    return endianness;
  }

  inline int bit_seq2mem_index(int bit_seq_index, int bytes_per_word, bool most_sig_bit_first, int compact_endianness)
  {
    int const bits_per_byte    =  numeric_limits<unsigned char>::digits;
    int const bits_per_word    =  bytes_per_word * bits_per_byte;
    int const word_index       =  bit_seq_index / bits_per_word;
    int const word_bit_index   =  most_sig_bit_first ? bits_per_word - bit_seq_index % bits_per_word - 1 : bit_seq_index % bits_per_word;
    int const word_byte_index  =  word_bit_index / bits_per_byte;
    int const byte_index       =  word_index * bytes_per_word + (compact_endianness ^ word_byte_index);
    int const byte_bit_index   =  word_bit_index % bits_per_byte;
    int const bit_mem_index    =  byte_index * bits_per_byte + byte_bit_index;
    return bit_mem_index;
  }


  inline int bit_mem2seq_index(int bit_mem_index, int bytes_per_word, bool most_sig_bit_first, int compact_endianness)
  {
    int const bits_per_byte    =  numeric_limits<unsigned char>::digits;
    int const bits_per_word    =  bytes_per_word * bits_per_byte;
    int const word_index       =  bit_mem_index / bits_per_word;
    int const word_byte_index  =  compact_endianness ^ (bit_mem_index % bits_per_word) / bits_per_byte; // According to significance
    int const byte_bit_index   =  bit_mem_index % bits_per_byte; // According to significance
    int const word_bit_index   =  bits_per_byte * word_byte_index + byte_bit_index; // According to significance
    int const bit_seq_index    =  bits_per_word * word_index + (most_sig_bit_first ? bits_per_word - word_bit_index - 1 : word_bit_index);
    return bit_seq_index;
  }


  int diff_formats(IntegerBufferFormat const *fmt1, IntegerBufferFormat const *fmt2, int num_pixels)
  {
    int const num_channels = fmt1->get_num_channels();
    if(fmt2->get_num_channels() != num_channels) return 0;
    int const bits_per_pixel = fmt1->get_bits_per_pixel();
    if(fmt2->get_bits_per_pixel() != bits_per_pixel) return 0;

    int const bytes_per_word1 = fmt1->get_bytes_per_word();
    int const bytes_per_word2 = fmt2->get_bytes_per_word();

    bool const most_sig_bit_first1 = fmt1->get_most_sig_bit_first();
    bool const most_sig_bit_first2 = fmt2->get_most_sig_bit_first();

    int const compact_endianness1 = endianness_vec2int(fmt1->get_endianness(), bytes_per_word1);
    int const compact_endianness2 = endianness_vec2int(fmt2->get_endianness(), bytes_per_word2);

    for(int i=0; i<num_channels; ++i)
    {
      int const width = fmt1->get_channel_width(i);
      if(fmt2->get_channel_width(i) != width) return 0;
      int const offset1 = fmt1->get_channel_offset(i);
      int const offset2 = fmt2->get_channel_offset(i);
      for(int j=0; j<num_pixels; ++j)
      {
        int const pixel_offset = j * bits_per_pixel;
        for(int k=0; k<width; ++k)
        {
          int const bit_seq_index1 =
            pixel_offset + offset1 + (most_sig_bit_first1 ? width-k-1 : k);
          int const bit_seq_index2 =
            pixel_offset + offset2 + (most_sig_bit_first2 ? width-k-1 : k);
          if(bit_seq2mem_index(bit_seq_index1, bytes_per_word1,
                               most_sig_bit_first1, compact_endianness1) !=
             bit_seq2mem_index(bit_seq_index2, bytes_per_word2,
                               most_sig_bit_first2, compact_endianness2)) return j;
        }
      }
    }

    return -1;
  }


  vector<int> mk_channel_map(IntegerBufferFormat const *fmt)
  {
    vector<int> map(fmt->get_bits_per_pixel(), -1);
    int const n = fmt->get_num_channels();
    for(int i=0; i<n; ++i)
    {
      int const o = fmt->get_channel_offset(i);
      int const w = fmt->get_channel_width(i);
      fill(map.begin()+o, map.begin()+o+w, i);
    }
    return map;
  }


  void display_format_diff(IntegerBufferFormat const *fmt1,
                           IntegerBufferFormat const *fmt2, int pixel_index)
  {
    int const bits_per_pixel = fmt1->get_bits_per_pixel();
    if(fmt2->get_bits_per_pixel() != bits_per_pixel)
      throw runtime_error("Cannot display format comparison when number of bits per pixel differ");

    int const bytes_per_word1 = fmt1->get_bytes_per_word();
    int const bytes_per_word2 = fmt2->get_bytes_per_word();
    int const bytes_per_word  = max(bytes_per_word1, bytes_per_word2);
    int const bits_per_byte   = numeric_limits<unsigned char>::digits;
    int const bits_per_word   = bytes_per_word * bits_per_byte;

    int const num_pixels = 2;
    int const first_word = ((pixel_index+0)          * long(bits_per_pixel) - 0) / bits_per_word;
    int const  last_word = ((pixel_index+num_pixels) * long(bits_per_pixel) - 1) / bits_per_word;
    int const num_words = last_word - first_word + 1;

    bool const most_sig_bit_first1 = fmt1->get_most_sig_bit_first();
    bool const most_sig_bit_first2 = fmt2->get_most_sig_bit_first();

    int const compact_endianness1 = endianness_vec2int(fmt1->get_endianness(), bytes_per_word1);
    int const compact_endianness2 = endianness_vec2int(fmt2->get_endianness(), bytes_per_word2);

    vector<int> const channel_map1 = mk_channel_map(fmt1);
    vector<int> const channel_map2 = mk_channel_map(fmt2);

    cerr << "Bits per pixel: "<<bits_per_pixel << endl;
    cerr << "Word type:      "<<get_word_type_name(fmt1->get_word_type())<<"/"<<get_word_type_name(fmt2->get_word_type()) << endl;
    cerr << "Endianness:     '";
    {
      int const levels = find_most_sig_bit(bytes_per_word1);
      for(int i=0; i<levels; ++i) cerr << (compact_endianness1 & 1<<i ? 'm' : 'l');
    }
    cerr << "'/'";
    {
      int const levels = find_most_sig_bit(bytes_per_word2);
      for(int i=0; i<levels; ++i) cerr << (compact_endianness2 & 1<<i ? 'm' : 'l');
    }
    cerr << "'" << endl;
    cerr << "Bit order:      "<<(most_sig_bit_first1?"most":"least")<<" significant first/"<<(most_sig_bit_first2?"most":"least")<<" significant first" << endl;

    Text::Table table;
    table.get_row(0).set_bg_color(Term::color_Default).set_reverse().set_bold();
    table.get_cell(0,0).set_text("Byte/bit");
    for(int j=0; j<bits_per_byte; ++j) table.get_cell(0,j+1).set_val(j);

    int const first_byte = first_word * bytes_per_word;
    int const num_bytes  = num_words  * bytes_per_word;
    for(int i=0; i<num_bytes; ++i)
    {
      int const byte_index = first_byte + i;
      int const row = i*2;
      if(i % 2 == 1)
      {
        table.get_row(row+1).set_bg_color(Term::color_White);
        table.get_row(row+2).set_bg_color(Term::color_White);
      }
      table.get_row(row+2).set_bold();
      table.get_cell(row+1, 0).set_val(byte_index);

      for(int j=0; j<bits_per_byte; ++j)
      {
        int const bit_mem_index = byte_index * bits_per_byte + j;

        int const bit_seq_index1 =
          bit_mem2seq_index(bit_mem_index, bytes_per_word1,
                            most_sig_bit_first1, compact_endianness1);
        int const bit_seq_index2 =
          bit_mem2seq_index(bit_mem_index, bytes_per_word2,
                            most_sig_bit_first2, compact_endianness2);

        int const pixel_index1 = bit_seq_index1 / bits_per_pixel; // According to position within bit sequence
        int const pixel_index2 = bit_seq_index2 / bits_per_pixel; // According to position within bit sequence

        int const pixel_bit_index1 = bit_seq_index1 % bits_per_pixel; // According to position within bit sequence
        int const pixel_bit_index2 = bit_seq_index2 % bits_per_pixel; // According to position within bit sequence

        int const channel_index1 = channel_map1[pixel_bit_index1];
        int const channel_index2 = channel_map2[pixel_bit_index2];

        int const channel_bit_index1 = channel_index1 < 0 ? -1 : most_sig_bit_first1 ?
          fmt1->get_channel_width(channel_index1) - pixel_bit_index1 +
          fmt1->get_channel_offset(channel_index1) - 1:
          pixel_bit_index1 - fmt1->get_channel_offset(channel_index1);
        int const channel_bit_index2 = channel_index2 < 0 ? -1 : most_sig_bit_first2 ?
          fmt2->get_channel_width(channel_index2) - pixel_bit_index2 +
          fmt2->get_channel_offset(channel_index2) - 1:
          pixel_bit_index2 - fmt2->get_channel_offset(channel_index2);

        Term::AnsiColor const colors[] =
          { Term::color_Red,  Term::color_Green,   Term::color_Blue,
            Term::color_Yellow, Term::color_Magenta, Term::color_Cyan };
        int const num_colors = sizeof(colors) / sizeof(*colors);

        Term::AnsiColor const color1 =
          channel_index1 < 0 ? Term::color_Default : colors[channel_index1 % num_colors];
        Term::AnsiColor const color2 =
          channel_index2 < 0 ? Term::color_Default : colors[channel_index2 % num_colors];

        ostringstream out1, out2;
        out1 << pixel_index1;
        out2 << pixel_index2;
        if(0 <= channel_index1) out1 << "/"<<channel_index1<<"/"<<channel_bit_index1;
        if(0 <= channel_index2) out2 << "/"<<channel_index2<<"/"<<channel_bit_index2;

        if(out1.str() != out2.str())
        {
          table.get_cell(row+1, j+1).set_reverse(true);
          table.get_cell(row+2, j+1).set_reverse(true);
        }
        table.get_cell(row+1, j+1).set_fg_color(color1).set_text(out1.str());
        table.get_cell(row+2, j+1).set_fg_color(color2).set_text(out2.str());
      }
    }

    cerr << table.print() << endl;
  }
}



void test(WordType word_type1, int endianness1, bool most_sig_bit_first1,
          WordType word_type2, int endianness2, bool most_sig_bit_first2,
          int bits_per_pixel,
          int ch1_width, int ch1_offset1, int ch1_offset2,
          int ch2_width, int ch2_offset1, int ch2_offset2,
          int ch3_width, int ch3_offset1, int ch3_offset2,
          int ch4_width, int ch4_offset1, int ch4_offset2)
{
  bool word_align_strips = true;

  IntegerBufferFormat::Ref fmt1;
  {
    IntegerBufferFormat::ChannelLayout channels;
    channels.bits_per_pixel = bits_per_pixel;
    if(0 < ch1_width) channels.add(IntegerBufferFormat::Channel(ch1_offset1, ch1_width));
    if(0 < ch2_width) channels.add(IntegerBufferFormat::Channel(ch2_offset1, ch2_width));
    if(0 < ch3_width) channels.add(IntegerBufferFormat::Channel(ch3_offset1, ch3_width));
    if(0 < ch4_width) channels.add(IntegerBufferFormat::Channel(ch4_offset1, ch4_width));
    vector<bool> const endianness =
      endianness_int2vec(endianness1, get_bytes_per_word(word_type1));
    fmt1 = IntegerBufferFormat::get_format(word_type1, channels, most_sig_bit_first1,
                                           word_align_strips, endianness);
  }

  IntegerBufferFormat::Ref fmt2;
  {
    IntegerBufferFormat::ChannelLayout channels;
    channels.bits_per_pixel = bits_per_pixel;
    if(0 < ch1_width) channels.add(IntegerBufferFormat::Channel(ch1_offset2, ch1_width));
    if(0 < ch2_width) channels.add(IntegerBufferFormat::Channel(ch2_offset2, ch2_width));
    if(0 < ch3_width) channels.add(IntegerBufferFormat::Channel(ch3_offset2, ch3_width));
    if(0 < ch4_width) channels.add(IntegerBufferFormat::Channel(ch4_offset2, ch4_width));
    vector<bool> const endianness =
      endianness_int2vec(endianness2, get_bytes_per_word(word_type2));
    fmt2 = IntegerBufferFormat::get_format(word_type2, channels, most_sig_bit_first2,
                                           word_align_strips, endianness);
  }

  int const diff_pixel = diff_formats(fmt1.get(), fmt2.get(), 128);

  if(fmt1->is_equiv_to(fmt2, 1024, 1024) == (diff_pixel < 0))
  {
//    cerr << "agreement about '"<<(diff_pixel < 0?"equality":"difference")<<"'"<< endl;
    return;
  }

  string msg = diff_pixel < 0 ?
    "Unexpected format equality detected" :
    "Unexpected format difference detected";
  cerr << msg<<":" << endl;

  display_format_diff(fmt1.get(), fmt2.get(), diff_pixel < 0 ? 0 : diff_pixel);

  TEST_MSG(false, msg);
}


int main() throw()
{
  test(word_type_ULong, 0, false,                word_type_ULong, 1,  false,
       64,       8,  0, 8,      0,  0,  0,      0,  0,  0,      0,  0,  0);
  test(word_type_ULong, 0, false,                word_type_ULong, 1,  false,
       64,       9,  0, 8,      0,  0,  0,      0,  0,  0,      0,  0,  0);

  test(word_type_ULong, 1, false,                word_type_ULong, 1,  true,
       128,     32,  0, 32,     32, 32,  0,     32, 64, 96,     31, 97, 64);
  test(word_type_ULong, 1, false,                word_type_ULong, 1,  true,
       128,     32,  0, 32,      0,  0,  0,      0,  0,  0,      0,  0,  0);
  test(word_type_ULong, 1, false,                word_type_ULong, 1,  true,
       128,     32,  0, 33,      0,  0,  0,      0,  0,  0,      0,  0,  0);

  int const num_word_types = get_num_word_types();
  int const max_bits_per_pixel = 32;
  for(int bits_per_pixel=1; bits_per_pixel<max_bits_per_pixel; ++bits_per_pixel)
  {
    cerr << bits_per_pixel<<"/"<<max_bits_per_pixel << endl;
    for(int i_word_type1=0; i_word_type1<num_word_types; ++i_word_type1)
    {
      WordType const word_type1 = get_word_type_by_index(i_word_type1);
      if(archon::Imaging::is_floating_point(word_type1)) continue;
      int const bytes_per_word1 = get_bytes_per_word(word_type1);

      for(int i_word_type2=0; i_word_type2<num_word_types; ++i_word_type2)
      {
        WordType const word_type2 = get_word_type_by_index(i_word_type2);
        if(archon::Imaging::is_floating_point(word_type2)) continue;
        int const bytes_per_word2 = get_bytes_per_word(word_type2);

        for(int bit_order=0; bit_order<4; ++bit_order)
        {
          bool const most_sig_bit_first1 = bit_order & 1;
          bool const most_sig_bit_first2 = bit_order & 2;

          for(int end1=0; end1<bytes_per_word1; ++end1)
          {
            for(int end2=0; end2<bytes_per_word2; ++end2)
            {
              for(int width=1; width<=bits_per_pixel; ++width)
              {
                for(int offset1=0; offset1+width<=bits_per_pixel; ++offset1)
                {
                  for(int offset2=0; offset2+width<=bits_per_pixel; ++offset2)
                  {
                    test(word_type1, end1, most_sig_bit_first1,                word_type2, end2, most_sig_bit_first2,
                         bits_per_pixel,       width,  offset1, offset2,      0,  0,  0,      0,  0,  0,      0,  0,  0);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return 0;
}
