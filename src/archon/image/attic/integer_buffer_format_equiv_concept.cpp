#include <stdexcept>
#include <algorithm>
#include <list>
#include <deque>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <memory>
#include <cmath>

using namespace std;

/*

strip_bit_index   Index into bit strip

memory_bit_index  Index into memory of bit which is byte_index * bits_per_byte + byte_bit_index  where byte_bit_index is the bit index in the byte counting from the least significant bit


Going from memory_bit_index to strip_bit_index:
-----------------------------------------------



Going from strip_bit_index to memory_bit_index:
-----------------------------------------------

word_bit_index   Index of bit in word counting from least significant bit

word_byte_index  Index of byte in word counting from least significant byte

byte_bit_index   Index of bit in byte from least significant bit

word_index       Index of word in strip

byte_index       Index of byte in strip


word_index       =  floor(strip_bit_index / bits_per_word)

word_bit_index   =  bits_per_word - strip_bit_index mod bits_per_word - 1  if most_sig_bit_first,  else strip_bit_index mod bits_per_word

word_byte_index  =  floor(word_bit_index / bits_per_byte)

byte_index       =  word_index * bytes_per_word + byte_perm(word_byte_index)

byte_bit_index   =  word_bit_index mod bits_per_byte

*/

namespace
{
  struct Channel
  {
    int bit_offset;
    int bit_width;
    Channel(int o, int w): bit_offset(o), bit_width(w) {}
  };

  typedef std::vector<Channel> ChannelLayout;
  ChannelLayout channels;

  int  const bits_per_byte = 8;
  int  bytes_per_word      = 1;
  int  bits_per_pixel      = 7;
  bool most_sig_bit_first  = false;
  int  endianness;

  int strip_to_mem_bit_index(int strip_bit_index)
  {
    int const bits_per_word    =  bytes_per_word * bits_per_byte;
    int const word_index       =  strip_bit_index / bits_per_word;
    int const word_bit_index   =  most_sig_bit_first ? bits_per_word - strip_bit_index % bits_per_word - 1 : strip_bit_index % bits_per_word;
    int const word_byte_index  =  word_bit_index / bits_per_byte;
    int const byte_index       =  word_index * bytes_per_word + (endianness ^ word_byte_index);
    int const byte_bit_index   =  word_bit_index % bits_per_byte;
    int const mem_bit_index    =  byte_index * bits_per_byte + byte_bit_index;
    return mem_bit_index;
  }


  int used_to_strip_bit_index(int used_bit_index)
  {
    // First construct pixel_index, pixel_channel_index, and channel_bit_index
    int used_bits_per_pixel = 0;
    for(size_t i=0; i<channels.size(); ++i) used_bits_per_pixel += channels[i].bit_width;
    int const pixel_index          = used_bit_index / used_bits_per_pixel;
    int const pixel_used_bit_index = used_bit_index % used_bits_per_pixel;
    int pixel_channel_index;
    int channel_bit_index;
    {
      int p = 0;
      for(size_t i=0; i<channels.size(); ++i)
      {
        int q = p + channels[i].bit_width;
        if(pixel_used_bit_index < q)
        {
          pixel_channel_index = i;
          channel_bit_index   = pixel_used_bit_index - p;
          break;
        }
        p = q;
      }
    }

    // The reast is a piece of cake
    int const strip_bit_index = pixel_index * bits_per_pixel + channels[pixel_channel_index].bit_offset +
      (most_sig_bit_first ? channels[pixel_channel_index].bit_width - channel_bit_index - 1 : channel_bit_index);

    return strip_bit_index;
  }


  int find_most_sig_bit(int v)
  {
    if(!v) return -1;

    int const n = sizeof(int) * std::numeric_limits<unsigned char>::digits;

    // Negative values are assumed to be represented as two's
    // complement. This is not guaranteed by standard C++.
    if(v < 0) return n - 1;

    int const u = 1;

    // Handle unlikely widths of more than 128 bits by searching
    // iteratively from above for the 128 bit chunk containing the most
    // significant bit
    int i;
    if(128 < n)
    {
      i = (n-1)/128*128;
      while(!(v & ((u << 128%n) - u) << i)) i -= 128;
      v >>= i;
    }
    else i = 0;

    // The modulo operations are there only to silence compiler
    // warnings. They will have no effect on the running program.
    if(64 < n && v & ((u << 64%n) - u) << 64%n) { v >>= 64%n; i |= 64; }
    if(32 < n && v & ((u << 32%n) - u) << 32%n) { v >>= 32%n; i |= 32; }
    if(16 < n && v & ((u << 16%n) - u) << 16%n) { v >>= 16%n; i |= 16; }
    if( 8 < n && v & ((u <<  8%n) - u) <<  8%n) { v >>=  8%n; i |=  8; }
    if( 4 < n && v & ((u <<  4%n) - u) <<  4%n) { v >>=  4%n; i |=  4; }
    if( 2 < n && v & ((u <<  2%n) - u) <<  2%n) { v >>=  2%n; i |=  2; }
    if( 1 < n && v & ((u <<  1%n) - u) <<  1%n) {             i |=  1; }
    return i;
  }
}


int main()
{
  channels.push_back(Channel(5,1));

  int const max_bits_per_pixel = 256;
  int const max_level = 4;
  int const num_pixels = 65;
  long num_matches = 0, num_mismatches = 0;

  for(int bit_order=0; bit_order<4; ++bit_order)
  {
    bool const msb_first1 = bit_order & 1;
    bool const msb_first2 = bit_order & 2;
    for(int bpp=1; bpp<max_bits_per_pixel; ++bpp)
    {
      bits_per_pixel = bpp;
      for(int level1=0; level1<=max_level; ++level1)
      {
        int const bpw1 = 1 << level1;
        for(int end1=0; end1<bpw1; ++end1)
        {
          int const comp1 = msb_first1 ? ~0 << level1 | end1 : end1;
          for(int level2=0; level2<=max_level; ++level2)
          {
            int const bpw2 = 1 << level2;
            for(int end2=0; end2<bpw2; ++end2)
            {
              int const comp2 = msb_first2 ? ~0 << level2 | end2 : end2;
              int const disc = comp1 ^ comp2;
              bool const match_expected = disc == 0 || bpp % (bits_per_byte << (1+find_most_sig_bit(disc<0 ? ~disc : disc))) == 0;
              for(int pos1=0; pos1<bpp; ++pos1)
              {
                for(int pos2=0; pos2<bpp; ++pos2)
                {
                  for(int pixel=0; pixel<num_pixels; ++pixel)
                  {
                    bytes_per_word = bpw1;
                    channels[0].bit_offset = pos1;
                    most_sig_bit_first = msb_first1;
                    endianness = end1;
                    int const res1 = strip_to_mem_bit_index(used_to_strip_bit_index(pixel));

                    bytes_per_word = bpw2;
                    channels[0].bit_offset = pos2;
                    most_sig_bit_first = msb_first2;
                    endianness = end2;
                    int const res2 = strip_to_mem_bit_index(used_to_strip_bit_index(pixel));
              
                    if(res1 != res2) goto next_pos;
                  }

                  ++num_matches;
                  if(!match_expected)
                  {
                    cout << "Unexpected match: bpp="<<bpp<<", bpw1="<<bpw1<<", bpw2="<<bpw2<<", msb1="<<msb_first1<<", msb2="<<msb_first2<<", pos1="<<pos1<<", pos2="<<pos2;
                    if(level1)
                    {
                      cout <<", end1=";
                      for(int i=0; i<level1; ++i) cout << (end1 & 1<<i ? 'm' : 'l');
                    }
                    if(level2)
                    {
                      cout <<", end2=";
                      for(int i=0; i<level2; ++i) cout << (end2 & 1<<i ? 'm' : 'l');
                    }
                    cout << endl;
                  }
                  goto next_end;

                next_pos:
                  continue;
                }
              }

              ++num_mismatches;
              if(match_expected)
              {
                cout << "Unexpected mismatch: bpp="<<bpp<<", bpw1="<<bpw1<<", bpw2="<<bpw2<<", msb1="<<msb_first1<<", msb2="<<msb_first2;
                if(level1)
                {
                  cout <<", end1=";
                  for(int i=0; i<level1; ++i) cout << (end1 & 1<<i ? 'm' : 'l');
                }
                if(level2)
                {
                  cout <<", end2=";
                  for(int i=0; i<level2; ++i) cout << (end2 & 1<<i ? 'm' : 'l');
                }
                cout << endl;
              }

            next_end:
              continue;
            }
          }
        }
      }
    }
  }

  long const num_bit_seq_comps = num_matches + num_mismatches;
  cout << "Number of tested bit sequence compositions: "<<num_bit_seq_comps<< endl;
  cout << "Matches:    "<<num_matches<<" ("<<(floor(10000.0*num_matches/num_bit_seq_comps)/100)<<"%)" << endl;
  cout << "Mismatches: "<<num_mismatches<<" ("<<(floor(10000.0*num_mismatches/num_bit_seq_comps)/100)<<"%)" << endl;

  return 0;
}
