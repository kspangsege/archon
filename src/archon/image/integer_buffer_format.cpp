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

#include <sstream>




#include <iostream>




#include <archon/platform.hpp> // Never include in other header files
#include <archon/core/memory.hpp>
#include <archon/core/text.hpp>
#include <archon/core/functions.hpp>
#include <archon/util/hashing.hpp>
#include <archon/util/range_map.hpp>
#include <archon/util/unit_frac.hpp>
#include <archon/image/integer_buffer_format.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Util;
using namespace archon::Imaging;


namespace
{
  struct ChannelOverlapDetector
  {
    bool operator()(bool &v)
    {
      if(v) throw invalid_argument("Overlapping channels");
      v = true;
      return true;
    }
  };


  struct MemoryField
  {
    /**
     * -1 indicates that this is an unused field
     */
    int channel_index;

    int bit_width;

    MemoryField(int i, int w): channel_index(i), bit_width(w) {}
  };


  int derive_mem_fields(IntegerBufferFormat::ChannelLayout const &channel_layout,
                        vector<MemoryField> &fields)
  {
    fields.clear();
    int const num_channels = channel_layout.channels.size();
    RangeMap<int, int> bit_map;
    bit_map.assign(0, channel_layout.bits_per_pixel-1, -1);
    for(int i=0; i<num_channels; ++i)
    {
      IntegerBufferFormat::Channel const &c = channel_layout.channels[i];
      bit_map.assign(c.bit_offset, c.bit_offset+c.bit_width-1, i);
    }
    RangeMap<int, int>::RangeSeq r = bit_map.get_ranges();
    int principal_bit_offset = 0;
    if(r && r->get_value() < 0)
    {
      principal_bit_offset = r->get_last()+1;
      ++r;
    }
    while(r)
    {
      fields.push_back(MemoryField(r->get_value(), r->get_last()+1-r->get_first()));
      ++r;
    }
    if(principal_bit_offset != 0)
    {
      if(fields.back().channel_index < 0) fields.back().bit_width += principal_bit_offset;
      else fields.push_back(MemoryField(-1, principal_bit_offset));
    }

    return principal_bit_offset;
  }
}


namespace archon
{
  namespace Imaging
  {
    IntegerBufferFormat::Ref
    IntegerBufferFormat::get_format(WordType word_type, ChannelLayout const &channel_layout,
                                    bool most_sig_bit_first, bool word_align_strips,
                                    vector<bool> const &endianness)
    {
      if(channel_layout.channels.empty()) throw invalid_argument("No channels");

      int const bytes_per_word = Imaging::get_bytes_per_word(word_type);
      int const bits_per_word  = bytes_per_word * numeric_limits<unsigned char>::digits;
      int const endianness_levels = find_most_sig_bit(bytes_per_word);

      if(bytes_per_word != 1<<endianness_levels)
        throw runtime_error("Cannot handle word types with number of bytes "
                            "("+Text::print(bytes_per_word)+") not being "
                            "a power of two");

      if(static_cast<int>(native_endianness.size()) < endianness_levels)
        throw runtime_error("Word type is too wide");

      // Normalize the endianness description
      vector<bool> normalized_endianness;
      if(!compare_endianness(endianness, native_endianness, endianness_levels))
      {
        normalized_endianness.resize(endianness_levels);
        copy(endianness.begin(), endianness.begin()+endianness_levels,
             normalized_endianness.begin());
      }

      // Verify channel layout, and detect the special condition where
      // all channels coincide with a word.
      int const bits_per_pixel = channel_layout.bits_per_pixel;
      ChannelLayout::Channels const &channels = channel_layout.channels;
      RangeMap<int, bool> m;
      bool word_coincident_channels = bits_per_pixel % bits_per_word == 0;
      {
        ChannelLayout::Channels::const_iterator const end = channels.end();
        for(ChannelLayout::Channels::const_iterator i=channels.begin(); i!=end; ++i)
        {
          int const o = i->bit_offset, w = i->bit_width;
          if(w < 1) throw invalid_argument("Bad channel width");
          if(o < 0 || bits_per_pixel < o+w) throw invalid_argument("Channels escape pixel bits");
          m.update(o, o+w-1, ChannelOverlapDetector());
          if(word_coincident_channels && (o % bits_per_word != 0 || w % bits_per_word != 0))
            word_coincident_channels = false;
        }
      }

      // When all channels coincide with an integral number of words,
      // the bit order is immaterial. Always setting it to 'lsb' in
      // this case, helps in the format equivalence testing.
      if(word_coincident_channels) most_sig_bit_first = false;

      // When each pixel consists of an integral number of words,
      // strips are always word aligned, so the word_align_strips flag
      // becomes irrelevant. Always setting it false in this case,
      // helps in the format equivalence testing.
      if(bits_per_pixel % bits_per_word == 0) word_align_strips = false;

      // Determine byte permutation if a custom endianness is requested.
      vector<int> byte_perm;
      if(!normalized_endianness.empty())
        byte_perm = compute_byte_perm(normalized_endianness, endianness_levels);

      return Ref(new IntegerBufferFormat(word_type, bytes_per_word, bits_per_word, channel_layout,
                                         most_sig_bit_first, word_align_strips,
                                         normalized_endianness, byte_perm));
    }


    size_t IntegerBufferFormat::get_required_buffer_size(int width, int height) const
    {
      if(height < 1) throw invalid_argument("Illegal image height");
      // Be carefull about overflow and require that the total number
      // of bits in the buffer can be represented in a 'long' and that
      // the number of bytes can be represented in a 'size_t'.
      unsigned long max = numeric_limits<long>::max(),
        bits_per_strip = get_bits_per_strip(width);
      unsigned long total_bits = bits_per_strip * height;
      if(total_bits/height == bits_per_strip && total_bits <= max)
      {
        unsigned long rem = total_bits % bits_per_word;
        if(rem) total_bits += bits_per_word - rem;
        if(total_bits <= max)
        {
          unsigned long total_bytes = total_bits /
            numeric_limits<unsigned char>::digits;
          if(total_bytes <= numeric_limits<size_t>::max()) return total_bytes;
        }
      }
      throw ImageSizeException("Image is too large for this buffer format");
    }


    bool IntegerBufferFormat::is_equiv_to(BufferFormat::ConstRefArg f, int width, int height) const
    {
      IntegerBufferFormat const *g = dynamic_cast<IntegerBufferFormat const *>(f.get());
      if(!g) return false;

      if(!equiv_strip_layouts(bit_seq_comp, channel_layout,
                              g->bit_seq_comp, g->channel_layout)) return false;

      // Check strip alignment
      if(1 < height && (word_align_strips || g->word_align_strips))
      {
        if(bytes_per_word == g->bytes_per_word)
        {
          // Note: word_align_strips is only true when it makes a difference
          if(word_align_strips != g->word_align_strips &&
             get_net_bits_per_strip(width) % bits_per_word != 0) return false;
        }
        else
        {
          if(get_gross_bits_per_strip(width) != g->get_gross_bits_per_strip(width)) return false;
        }
      }

      return true;
    }


    unsigned IntegerBufferFormat::derive_bit_seq_comp(WordType word_type,
                                                      vector<bool> const &endianness,
                                                      bool most_sig_bit_first)
    {
      int const bytes_per_word = Imaging::get_bytes_per_word(word_type);
      int const endianness_levels = find_most_sig_bit(bytes_per_word);
      unsigned compact_endianness = 0;
      {
        vector<bool> const &e = endianness.empty() ? native_endianness : endianness;
        int const max = e.size() - 1;
        for(int i=0; i<endianness_levels; ++i) if(e[min(i, max)]) compact_endianness |= 1u<<i;
      }
      return most_sig_bit_first ?
        compact_endianness | ~0u << endianness_levels : compact_endianness;
    }


    int IntegerBufferFormat::derive_canon_channel_offset(unsigned bit_seq_comp, Channel const &c)
    {
      if(bit_seq_comp == 0)
        return c.bit_offset;
      bool most_sig_bit_first = ((bit_seq_comp & (1u << (std::numeric_limits<unsigned>::digits-1))) != 0);
      int offset = most_sig_bit_first ? c.bit_offset + c.bit_width - 1 : c.bit_offset;
      int bits_per_byte = std::numeric_limits<unsigned char>::digits;
      div_t div_res = div(offset, bits_per_byte);
      unsigned byte_index = unsigned(div_res.quot) ^ bit_seq_comp;
      unsigned temp = byte_index * bits_per_byte + div_res.rem;
      return int(most_sig_bit_first ? ~temp : temp);
    }


    int IntegerBufferFormat::derive_strip_layout_hash(unsigned bit_seq_comp,
                                                      ChannelLayout const &channel_layout)
    {
      Hash_FNV_1a_32 hash;
      hash.add_int(channel_layout.bits_per_pixel);
      typedef ChannelLayout::Channels Channels;
      Channels::const_iterator const end = channel_layout.channels.end();
      for(Channels::const_iterator i = channel_layout.channels.begin(); i!=end; ++i)
      {
        hash.add_int(i->bit_width);
        hash.add_int(derive_canon_channel_offset(bit_seq_comp, *i));
      }
      return hash.get_hash(strip_layout_hash_size);
    }


    bool IntegerBufferFormat::equiv_strip_layouts(unsigned bit_seq_comp1, ChannelLayout const &layout1,
                                                  unsigned bit_seq_comp2, ChannelLayout const &layout2)
    {
      int const bits_per_pixel = layout1.bits_per_pixel;
      if(bits_per_pixel != layout2.bits_per_pixel) return false;

      size_t const num_channels = layout1.channels.size();
      if(num_channels != layout2.channels.size()) return false;

      unsigned bit_seq_diff = bit_seq_comp1 ^ bit_seq_comp2;

      // If the bit sequence composition is the same in both formats,
      // then we need only compare the immediate channel layouts.
      if(bit_seq_diff == 0) return layout1.channels == layout2.channels;

      bool same_bit_order = ((bit_seq_diff & (1u << (std::numeric_limits<unsigned>::digits-1))) == 0);
      int byte_seq_diff = int(same_bit_order ? bit_seq_diff : ~bit_seq_diff);

      // So, the bit sequence compositions do not agree exactly. Let's
      // find out what the level of disagreement is, that is, the
      // highest byte combination level where there is a
      // disagreement. Zero means agreement at all byte combination
      // levels. One means no agreement on byte level, but agreement
      // for every pair of bytes.
      int max_disagree_level = 1+find_most_sig_bit(byte_seq_diff);
      int bits_per_disagree_unit = numeric_limits<unsigned char>::digits << max_disagree_level;

      if(bits_per_pixel % bits_per_disagree_unit != 0)
      {
        // When each pixel does not cover an integer number of units
        // of disagreement, then we consider compatibility between the
        // two formats as impossible. The truth is that there are
        // cases where a sufficiently small image would result in
        // equivalence, but it is rather hard to determine what the
        // limit is in each case.

        // It is far from obvious why formats are necessarily
        // incompatible just because the unit of disagreement does not
        // divide the pixel size. It really deserves a formal proof,
        // but so far I have been unsuccessfull in finding one. I have
        // done extensive testing though, and found that equivalence
        // is impossible for an arbitrary number of pixels. That is,
        // in many cases a finite number of initial pixels can match,
        // but there is always a limit.
        return false;
      }

      // Since bits per pixel is an integer multiple of the unit of
      // disagreement, all pixels are subject to exactly the same
      // disagreement, so it suffices to check that corresponding
      // channels map to the same bits in memory.

      int max_contig_level = find_least_sig_bit(bit_seq_diff);
      int bits_per_contig_unit = numeric_limits<unsigned char>::digits << max_contig_level;
      for(size_t i=0; i<num_channels; ++i)
      {
        Channel const &a = layout1.channels[i];
        Channel const &b = layout2.channels[i];

        int const width = a.bit_width;
        if(width != b.bit_width) return false;
        if(bits_per_contig_unit < a.bit_offset % bits_per_contig_unit + width) return false;

        // Find the offset of each channel within the first pixel of
        // the bit sequence. If this format (as opposed to the one
        // passed as argument) has most significant bits first, then
        // both offsets will mark the position of the most significant
        // bit of the channel, otherwise both offsets will mark the
        // least significant bit.
        int const offset1 = a.bit_offset;
        int const offset2 = same_bit_order ? b.bit_offset : b.bit_offset + width - 1;

        // Check that the two channels have the same bit offset
        // within a byte
        if((same_bit_order ? offset1 - offset2 : offset1 + offset2 + 1) %
           numeric_limits<unsigned char>::digits != 0) return false;

        // Check that the two channels start off in the same byte
        int const byte1 = offset1 / numeric_limits<unsigned char>::digits;
        int const byte2 = offset2 / numeric_limits<unsigned char>::digits;
        if((byte1 ^ byte2) != byte_seq_diff) return false;
      }

      return true;
    }


    string IntegerBufferFormat::print(WordType word_type, std::vector<bool> const &endianness,
                                      bool most_sig_bit_first, ChannelLayout const &channel_layout,
                                      ColorSpace::ConstRef color_space, bool has_alpha)
    {
      int const num_channels = channel_layout.channels.size();
      if(!color_space)
      {
        switch(num_channels)
        {
        case 1: color_space = ColorSpace::get_Lum(); has_alpha = false; break;
        case 2: color_space = ColorSpace::get_Lum(); has_alpha = true;  break;
        case 3: color_space = ColorSpace::get_RGB(); has_alpha = false; break;
        case 4: color_space = ColorSpace::get_RGB(); has_alpha = true;  break;
        default: color_space = ColorSpace::new_custom(num_channels); has_alpha = false;
        }
      }
      else if(num_channels != color_space->get_num_channels(has_alpha))
        throw invalid_argument("Channel number mismatch");

      int bytes_per_word = Imaging::get_bytes_per_word(word_type);
      int bits_per_word = bytes_per_word * numeric_limits<unsigned char>::digits;
      unsigned bit_seq_comp = derive_bit_seq_comp(word_type, endianness, most_sig_bit_first);

      ostringstream out;
      if(bits_per_word != 8)
      {
        out << "UINT"<<bits_per_word;
        int levels = find_most_sig_bit(bytes_per_word);
        if(bit_seq_comp & ((1u << levels) - 1))
        {
          while(1 < levels &&
                ((bit_seq_comp & (1u << (levels-1))) != 0) ==
                ((bit_seq_comp & (1u << (levels-2))) != 0))
            --levels;
          for(int i=0; i<levels; ++i) out << (bit_seq_comp & (1u << i) ? "M" : "L");
        }
        out <<"_";
      }
      vector<MemoryField> fields;
      derive_mem_fields(channel_layout, fields);
      int const n = fields.size();
      for(int i=0; i<n; ++i)
      {
        MemoryField const &f = fields[i];
        if(i) out << '_';
        if(0 <= f.channel_index) out << color_space->get_channel_id(f.channel_index);
        out << f.bit_width;
      }
      if(most_sig_bit_first) out << "_REV";
      return out.str();
    }



    BufferCodec *IntegerBufferFormat::choose_codec(void *buffer, int width, int height) const
    {
      switch(word_type)
      {
      case word_type_UChar:
	return choose_codec<unsigned char, unsigned int>(buffer, width, height);

      case word_type_UShort:
	return choose_codec<unsigned short, unsigned int>(buffer, width, height);

      case word_type_UInt:
	return choose_codec<unsigned int, unsigned int>(buffer, width, height);

      case word_type_ULong:
	return choose_codec<unsigned long, unsigned long>(buffer, width, height);

      case word_type_Float:
      case word_type_Double:
      case word_type_LngDbl:
        break; // Mention all values explicitely such that we are notified if new ones are defined
      }
      throw runtime_error("Got unexpected word type");
    }


    template<class Word, class WordAssemble>
    BufferCodec *IntegerBufferFormat::choose_codec(void *buffer, int width, int height) const
    {
      // Determine the bit-width of the widest channel
      int w = 0;
      {
        ChannelLayout::Channels::const_iterator const end = channel_layout.channels.end();
        for(ChannelLayout::Channels::const_iterator i=channel_layout.channels.begin(); i!=end; ++i)
          if(w < i->bit_width) w = i->bit_width;
      }

      // Determine the number of bytes needed to hold the widest channel
      int const m = numeric_limits<unsigned char>::digits;
      size_t const n = (w + m - 1) / m;

      if(n <= sizeof(unsigned char)) return choose_codec<Word, WordAssemble,
        unsigned char, unsigned int>(buffer, width, height, word_type_UChar);
      if(n <= sizeof(unsigned short)) return choose_codec<Word, WordAssemble,
        unsigned short, unsigned int>(buffer, width, height, word_type_UShort);
      if(n <= sizeof(unsigned int)) return choose_codec<Word, WordAssemble,
        unsigned int, unsigned int>(buffer, width, height, word_type_UInt);
      else return choose_codec<Word, WordAssemble,
        unsigned long, unsigned long>(buffer, width, height, word_type_ULong);
    }


    template<class Word, class WordAssemble, class TrayWord, class ChannelAssemble>
    BufferCodec *IntegerBufferFormat::choose_codec(void *buffer, int width, int /* height */,
                                                   WordType tray_word_type) const
    {
      return byte_perm.empty() ?
        static_cast<BufferCodec *>(new FallbackCodec<Word, WordAssemble,
                                   TrayWord, ChannelAssemble, true>(this, buffer, width,
                                                                    tray_word_type)) :
        static_cast<BufferCodec *>(new FallbackCodec<Word, WordAssemble,
                                   TrayWord, ChannelAssemble, false>(this, buffer, width,
                                                                     tray_word_type));
    }




    /**
     * \tparam Word The integral integer type into which pixels data
     * is encoded. This type determines how many bytes are read from
     * or written to memory at a time and the order in which those
     * bytes are combined to produced wider integers. The order is
     * also affected by the chosen endianness however.
     *
     * \tparam WordAssemble The integral integer type used to hold a
     * single word while manipulating bits. It must be at least as
     * wide as Word and at least as wide as an 'int', and it should be
     * no wider than neccessary.
     *
     * \tparam ChannelAssemble The integral integer type used to hold
     * a single channel value while manipulating bits. It must be at
     * least as wide as the widest channel and at least as wide as an
     * 'int', and it should be no wider than neccessary.
     */
    template<class Word, class WordAssemble, class TrayWord, class ChannelAssemble,
             bool nativeEndianness>
    struct IntegerBufferFormat::FallbackCodec: BufferCodec
    {
      void *getBufferPtr() const
      {
        return buffer;
      }

      WordType getTrayWordType() const
      {
        return tray_word_type;
      }


      /**
       * \todo FIXME: Document that a strip can be at most MAX_INT bits wide.
       */
      void decode(TupleGrid const &grid, int width, int height, int x, int y) const
      {
        // Cast the tray to be based on a pointer to TrayWord rather than char
        TrayWord *t = reinterpret_cast<TrayWord *>(grid.origin);
        ssize_t pitch = grid.pitch / sizeof(TrayWord), stride = grid.stride / sizeof(TrayWord);

        int residualBitsPerStrip = bits_per_strip - width * bits_per_pixel;
        long residualStride = stride - width * static_cast<long>(pitch);

        // Determine bit and word offsets of the lower left pixel in array
        long wordOffset;
        int bitOffset = static_cast<int>(modulo<long>(principal_bit_offset + x *
                                                      bits_per_pixel + y *
                                                      static_cast<long>(bits_per_strip),
                                                      bitsPerWord, wordOffset));

        int n = width;
        Word const *p = buffer + wordOffset;
        typename vector<MemoryField>::const_iterator field = mem_fields.begin();

        // Load the first word from memory
        WordAssemble word = readWord(p);

        // Prepare for assembly of first channel
        int channelBitWidth = field->bit_width;
        int channelBitOffset = 0;
        ChannelAssemble channel = 0;

        // Iterate over bit chunk transfers
        for(;;)
        {
          // The number of unparsed bits remaining in the current input word
          int remainingWordBits = bitsPerWord - bitOffset;
          int remainingChannelBits = channelBitWidth - channelBitOffset;

          // The number of bits we can transfer as one chunk
          int bitAdvance = min(remainingWordBits, remainingChannelBits);

          // Special trick to support shifts by N bits where N is the
          // number of bits in WordAssemble
          WordAssemble mask = (static_cast<WordAssemble>(1)<<bitAdvance-1<<1) - 1;
          // Transfer bitAdvance bits from word to channel
          if(most_sig_bit_first)
            channel |= static_cast<ChannelAssemble>(word>>remainingWordBits-
                                                    bitAdvance & mask) <<
              remainingChannelBits-bitAdvance;
          else channel |= static_cast<ChannelAssemble>(word>>bitOffset & mask) <<
                 channelBitOffset;

          // If the channel is not yet complete the the word must be complete
          if(bitAdvance < remainingChannelBits)
          {
            // Advance to next word of the image data
            ++p;
            word = readWord(p);
            bitOffset = 0;
            channelBitOffset += bitAdvance;
            continue;
          }

          // Channel is complete (and maybe word is complete too)

          // Store completed channel after adjusting its bit width
          t[field->channel_index] =
            frac_adjust_bit_width<ChannelAssemble>(channel, field->bit_width, bitsPerTrayWord);

          // Skip to next used bit field
          if(++field == mem_fields.end()) goto next_pixel;
          if(field->channel_index < 0)
          {
            bitAdvance += field->bit_width;
            if(++field == mem_fields.end())
            {
            next_pixel:
              if(!--n)
              {
                // Next strip (row)
                if(!--height) return; // No more strips
                t += residualStride;
                bitAdvance += residualBitsPerStrip;
                n = width;
              }
              field = mem_fields.begin();
              t += pitch;
            }
          }

          long wordAdvance;
          bitOffset = modulo<int>(bitOffset+bitAdvance, bitsPerWord, wordAdvance);
          if(wordAdvance)
          {
            p += wordAdvance;

            // Load the next word from memory
            word = readWord(p);
          }

          // Prepare for assembly of next channel
          channelBitWidth = field->bit_width;
          channelBitOffset = 0;
          channel = 0;
        }
      }


      /**
       * \todo FIXME: Working with word masks like here is not always
       * what you want. Currently any unused bit-fields will be
       * untouched in the target buffer. Sometimes it will be
       * important that the skipped bits are actually cleared to
       * zero. If we want to clear those bits we can improve
       * performance since we would only have to load original words
       * at the first and at the last word in tight formats. In any
       * case we can avoid the reading if the buffer format is tight,
       * ie. without unused bits.
       */
      void encode(ConstTupleGrid const &grid, int width, int height, int x, int y)
      {
        // Cast the tray to be based on a pointer to TrayWord rather than char
        TrayWord const *t = reinterpret_cast<TrayWord const *>(grid.origin);
        ssize_t pitch = grid.pitch / sizeof(TrayWord), stride = grid.stride / sizeof(TrayWord);

        int residualBitsPerStrip = bits_per_strip - width * bits_per_pixel;
        long residualStride = stride - width * static_cast<long>(pitch);

        // Determine bit and word offsets of the lower left pixel in array
        long wordOffset;
        int bitOffset = static_cast<int>(modulo<long>(principal_bit_offset + x *
                                                      bits_per_pixel + y *
                                                      static_cast<long>(bits_per_strip),
                                                      bitsPerWord, wordOffset));

        int n = width;
        Word *p = buffer + wordOffset;
        typename vector<MemoryField>::const_iterator field = mem_fields.begin();

        // Prepare for assembly of first word
        WordAssemble word = 0;
        WordAssemble wordMask = 0;

        // Load the first channel from the tray
        int channelBitWidth = field->bit_width;
        int channelBitOffset = 0;
        ChannelAssemble channel =
          frac_adjust_bit_width<ChannelAssemble>(t[field->channel_index],
                                                 bitsPerTrayWord, field->bit_width);

        // Iterate over bit chunk transfers
        for(;;)
        {
          // The number of unparsed bits remaining in the current input channel
          int remainingChannelBits = channelBitWidth - channelBitOffset;
          int remainingWordBits = bitsPerWord - bitOffset;

          // The number of bits we can transfer as one chunk
          int bitAdvance = min(remainingChannelBits, remainingWordBits);

          // Special trick to support shifts by N bits where N is the
          // number of bits in WordAssemble
          WordAssemble mask = (static_cast<WordAssemble>(1)<<bitAdvance-1<<1) - 1;
          // Transfer bitAdvance bits from channel to word
          if(most_sig_bit_first)
          {
            word |= (static_cast<WordAssemble>(channel>>remainingChannelBits-bitAdvance) & mask) << remainingWordBits-bitAdvance;
            wordMask |= mask << remainingWordBits-bitAdvance;
          }
          else
          {
            word |= (static_cast<WordAssemble>(channel>>channelBitOffset) & mask) << bitOffset;
            wordMask |= mask << bitOffset;
          }

          // If the channel is not yet complete the the word must be complete
          if(bitAdvance < remainingChannelBits)
          {
            // Store completed word
            if(wordMask != fullWordMask) word |= readWord(p) & ~wordMask;
            writeWord(word, p);

            // Advance to next word of the image data
            ++p;
            bitOffset = 0;
            word = wordMask = 0;
            channelBitOffset += bitAdvance;
            continue;
          }

          // Channel is complete (and maybe word is complete too)

          // 'word' always contains unwritten bits at this point

          // Skip to next used bit field
          if(++field == mem_fields.end()) goto next_pixel;
          if(field->channel_index < 0)
          {
            bitAdvance += field->bit_width;
            if(++field == mem_fields.end())
            {
            next_pixel:
              if(!--n)
              {
                // Store last word in strip
                if(wordMask != fullWordMask)
                  word |= readWord(p) & ~wordMask;
                writeWord(word, p);

                // Next strip (row)
                if(!--height) return; // No more strips
                t += residualStride;
                bitAdvance += residualBitsPerStrip;
                n = width;
              }
              field = mem_fields.begin();
              t += pitch;
            }
          }

          long wordAdvance;
          bitOffset = modulo<int>(bitOffset+bitAdvance, bitsPerWord, wordAdvance);
          if(wordAdvance)
          {
            // Store completed word
            if(wordMask != fullWordMask)
              word |= readWord(p) & ~wordMask;
            writeWord(word, p);

            p += wordAdvance;

            // Load the next word from memory
            word = wordMask = 0;
          }

          // Load next channel
          channelBitWidth = field->bit_width;
          channelBitOffset = 0;
          channel = frac_adjust_bit_width<ChannelAssemble>(t[field->channel_index],
                                                           bitsPerTrayWord, field->bit_width);
        }
      }


      Word readWord(Word const *p) const
      {
        // The 'switch' is intended to be compiletime evaluated
        return nativeEndianness ? *p :
          readWithBytePermutation<Word>(p, byte_perm);
      }

      void writeWord(Word v, Word *p) const
      {
        // The 'switch' is intended to be compiletime evaluated
        if(nativeEndianness) *p = v;
        else writeWithBytePermutation<Word>(v, p, byte_perm);
      }


      FallbackCodec(IntegerBufferFormat const *format, void *buffer,
                    int width, WordType tray_word_type):
        buffer(reinterpret_cast<Word *>(buffer)),
        bits_per_pixel(format->channel_layout.bits_per_pixel),
        most_sig_bit_first(format->most_sig_bit_first),
        byte_perm(format->byte_perm), tray_word_type(tray_word_type)
      {
        principal_bit_offset = derive_mem_fields(format->channel_layout, mem_fields);
        bits_per_strip = format->get_gross_bits_per_strip(width);
      }

      static int const bitsPerWord = numeric_limits<unsigned char>::digits * sizeof(Word);
      static int const bitsPerTrayWord = numeric_limits<unsigned char>::digits * sizeof(TrayWord);
      static WordAssemble const fullWordMask = (static_cast<WordAssemble>(1)<<bitsPerWord-1<<1) - 1;

      Word *buffer;

      /**
       * The accumulated bit width must be equal to \c bits_per_pixel
       * and leading unused bits are shifted to the end such that the
       * format always starts with a used bit field.
       */
      vector<MemoryField> mem_fields;

      /**
       * The number of leading unused bits in a pixel.
       */
      int principal_bit_offset;

      int bits_per_pixel;

      int bits_per_strip;

      bool most_sig_bit_first;

      /**
       * Zero length indicates native endianness. Otherwise the size
       * of this permuation is equal to the number of bytes per word
       * of the specified word type. This permutation is always a
       * symmetric map. That is, if you apply it twice the result will
       * be identical to the original.
       */
      vector<int> byte_perm;

      WordType tray_word_type;
    };
  }
}
