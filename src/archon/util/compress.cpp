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

#include <algorithm>

#include <archon/platform.hpp> // Never include in other header files
#include <archon/core/types.hpp>
#include <archon/core/memory.hpp>
#include <archon/util/compress.hpp>
#include <archon/util/inc_conv.hpp>


using namespace std;
using namespace archon::Core;
using namespace archon::Util;

namespace
{
  // Must be in the range [2;8]. Codec design/definition limits this
  // to 2 bits downwards.
  int const min_bits_per_byte = 2;

  // Must be in the range [2;8]. Compressor hash table design limits
  // this to 8 bits.
  int const max_bits_per_byte = 8;

  // Must be in the range [9;12]. Compressor hash table design limits
  // this to 12 bits.
  int const max_bits_per_code = 12;

  unsigned const max_num_codes = 1u << max_bits_per_code;

  unsigned const nil_code = max_num_codes;


  struct LempelZivWelchCompressor
  {
    typedef char source_char_type;
    typedef char target_char_type;

    static int const min_source_buffer_size = 1;
    static int const min_target_buffer_size = 1;

    struct State;

    LempelZivWelchCompressor(int bits_per_byte): bits_per_byte(bits_per_byte) {}

    int const bits_per_byte;
  };

  /**
   * Compression algorithm:
   *
   * <pre>
   *
   *   For ever:
   *     While byte assembler has at least 8 bits:
   *       If output buffer is full: Return false
   *       Extract byte from assembler and add it to the output buffer
   *
   *     If EOF code was added to assembler:
   *       If output buffer is full: Return false
   *       Use last bits of assembler as lower order bits of last byte
   *         and add it to the output buffer
   *       Return true
   *
   *     For ever:
   *       If input buffer is empty:
   *         If 'eoi' flag is low: Return true
   *         Add bits of current prefix to byte assembler
   *         Add bits of EOF code to byte assembler
   *         Break
   *       If first byte of input:
   *         Set current prefix equal to first input byte
   *         Continue
   *       Set new suffix equal to next input byte
   *       If the concattenation of the current prefix and the new suffix
   *         is not in the dictionary:
   *         Add the bits of the current prefix to the byte assembler
   *         If dictionary is full:
   *           Add bits of CLEAR code to the byte assembler
   *           Reset dictionary and code size
   *         Else: Register (size of dictionary) -> concattenation of
   *           the current prefix and the new suffix
   *         Set current prefix equal to new suffix
   *         Break
   *
   *       Set current prefix equal to the concattenation of
   *         the current prefix and the new suffix
   *
   * </pre>
   *
   * Returns false when byte assmebler has 8 or more bits and the
   * ouput buffer is full.
   *
   * Returns true when byte assmebler has less than 8 bits and the
   * input buffer is empty.
   *
   * Need at least 31 bits for byte assembler (2*12+7)
   *
   * The first code that is generated, is always equal to the value of
   * the first input byte.
   *
   * The last code that we can register in the hash table has value
   * 4095 (assuming max 12 bits per code), this code must refer to a
   * prefix code whose value is strictly less than 4095. Thus, the
   * value 4095 is never used as a prefix code when generating the
   * hash key, so if we add one to the prefix code when generating the
   * hash key, we can use zero to detect empty entries.
   *
   * \sa http://www.w3.org/Graphics/GIF/spec-gif89a.txt
   * \sa http://marknelson.us/1989/10/01/lzw-data-compression/
   * \sa Nelson, M.R.: "LZW Data Compression", Dr. Dobb's Journal, October 1989.
   */
  struct LempelZivWelchCompressor::State
  {
    bool conv(char const *&in, char const *in_end, char *&out, char *out_end, bool eoi)
    {
      for(;;)
      {
        // Extract as many complete bytes as possible from assembler
        while(8 <= bits_in_assembler)
        {
          if(out_end == out) return false; // Output buffer is full
          *out++ = static_cast<unsigned>(assembler) & 0xFFu;
          assembler >>= 8;
          bits_in_assembler -= 8;
        }

        // Flush the last bits in the assembler if the EOF code has
        // been added
        if(eof_code_sent)
        {
          if(bits_in_assembler)
          {
            if(out_end == out) return false; // Output buffer is full
            *out++ = static_cast<unsigned>(assembler) & 0xFFu;
          }
          return true;
        }

        for(;;)
        {
          // If there is no more data in input buffer
          if(in_end == in)
          {
            if(!eoi) return true;
            // At end-of-input emit code for current prefix, then EOF
            // code
            add_code_and_check_dict_size(prefix);
            add_code(eof_code);
            eof_code_sent = true;
            break;
          }

          unsigned suffix = static_cast<unsigned char>(*in++);
          if(max_byte_value < suffix)
            throw IncConvException("Byte value out of range");

          // If this is the first input byte
          if(prefix == nil_code)
          {
            prefix = suffix;
            continue;
          }

          // Check if the concattenation of prefix and suffix is in
          // the dictionary
          UIntMin32 key = hash_key(prefix, suffix);
          unsigned code = hash_lookup(key);
          if(code == nil_code)
          {
            // Not in the dictionary
            add_code_and_check_dict_size(prefix);

            // If dictionary is full
            if(dictionary_size == max_num_codes)
            {
              add_code(clear_code);
              reset();
            }
            else hash_insert(key, dictionary_size++);

            prefix = suffix;
            break;
          }

          prefix = code;
        }
      }
    }

    void add_code(unsigned code)
    {
      assembler |= static_cast<UIntFast32>(code) << bits_in_assembler;
      bits_in_assembler += bits_per_code;
    }

    void add_code_and_check_dict_size(unsigned code)
    {
      add_code(code);

      // The next code can be as large as one minus the dictionary
      // size. If this is too much for the current number of bits,
      // increase the number of bits by one
      if(code_mask < dictionary_size && dictionary_size < max_num_codes)
        code_mask = (1u << ++bits_per_code) - 1u;
    }

    unsigned hash_lookup(UIntMin32 key)
    {
      unsigned i = hash_index(key);
      for(;;)
      {
        UIntMin32 v = hash_table[i];
        if(!v) return nil_code; // Not found
        if((v & static_cast<UIntMin32>(0xFFFFFul)) == key)
          return v >> 20;
        if(++i == hash_table_size) i = 0;
      }
    }

    void hash_insert(UIntMin32 key, unsigned code)
    {
      unsigned i = hash_index(key);
      for(;;)
      {
        UIntMin32 v = hash_table[i];
        if(!v)
        {
          hash_table[i] = static_cast<UIntMin32>(code) << 20 | key;
          return;
        }
        if(++i == hash_table_size) i = 0;
      }
    }

    UIntMin32 hash_key(unsigned prefix, unsigned suffix)
    {
      return (static_cast<UIntMin32>(prefix+1u) << 8) | suffix;
    }

    unsigned hash_index(UIntMin32 key)
    {
      return static_cast<unsigned>((key >> 12) ^ key) & hash_index_mask;
    }

    State(LempelZivWelchCompressor const &c):
      bits_per_byte(c.bits_per_byte), max_byte_value((1u << bits_per_byte) - 1u),
      clear_code(max_byte_value + 1u), eof_code(clear_code + 1u),
      assembler(0), bits_in_assembler(0), eof_code_sent(false), prefix(nil_code)
    {
      reset();
      add_code(clear_code);
    }

    void reset()
    {
      bits_per_code   = bits_per_byte + 1;
      code_mask       = (1u << bits_per_code) - 1u;
      dictionary_size = eof_code + 1u;
      fill(hash_table, hash_table+hash_table_size, 0);
    }

    static int const bits_per_hash_index  = max_bits_per_code+1;
    static unsigned const hash_table_size = 1u <<bits_per_hash_index;
    static unsigned const hash_index_mask =  hash_table_size - 1u;

    int const bits_per_byte;
    unsigned const max_byte_value;
    unsigned const clear_code; // = 1 << bits_per_byte
    unsigned const eof_code;   // = clear_code + 1

    UIntFast32 assembler;
    int bits_in_assembler, bits_per_code;
    bool eof_code_sent;
    unsigned code_mask, prefix, dictionary_size;

    UIntMin32 hash_table[hash_table_size];
  };



  struct LempelZivWelchDecompressor
  {
    typedef char source_char_type;
    typedef char target_char_type;

    static int const min_source_buffer_size = 1;
    static int const min_target_buffer_size = 1;

    struct State;

    LempelZivWelchDecompressor(int bits_per_byte): bits_per_byte(bits_per_byte) {}

    int bits_per_byte;
  };

  /**
   * Decompression algorithm:
   *
   * <pre>
   *
   *   Initially:
   *     Register 2^(bits per bytes) codes in dictionary to map to themselves
   *     Set CLEAR to dictionary size
   *     Register CLEAR -> NIL in dictionary
   *     Set EOF to dictionary size
   *     Register EOF -> NIL in dictionary
   *     Set bits per code = bits per byte + 1
   *     Set previous code to NIL
   *
   *   For each call to 'conv':
   *     If EOI code seen:
   *       If callers source buffer is not empty: Fail
   *       Return true
   *
   *     For ever:
   *       While output stack not empty and callers target buffer not full:
   *         Pop a byte from output stack and add it to callers target buffer
   *       If output stack is not empty: Return false
   *       Output stack is now empty
   *
   *       While more bits are needed to build the next code:
   *         If callers source buffer is empty:
   *           If 'eoi' argument is true: Fail
   *           Return true
   *         Add next input byte to code assembler
   *
   *       If code is CLEAR:
   *         Reset dictionary, bits per code, and previous code
   *         Continue
   *
   *       If code is EOI:
   *         If callers source buffer not empty or
   *           code assembler has more bits set: Fail
   *         Return true
   *
   *       If code is a byte:
   *         Set latest suffix to code
   *         If space left in callers target buffer:
   *           Add code to callers target buffer
   *         Else: Push code onto output stack
   *
   *       Else:
   *         If code is not in dictionary:
   *           If previous code is NIL or
   *             code not equal to dictionary length: Fail
   *           Push suffix character onto output stack
   *           Push characters of dictionary entry of previous code
   *             in reverse order onto output stack
   *
   *         Else:
   *           Push characters of dictionary entry of current code
   *             in reverse order onto output stack
   *
   *         Set latest suffix to top of output stack
   *
   *       If previous code is not NIL and dictionary is not full:
   *         Register (size of dictionary) -> concattenation of
   *           dictionary entry of previous code and latest suffix
   *
   *         If dictionary size exceeds current number of bits per code:
   *           Increment number of bits per code
   *
   *       Set previous code to current code
   *
   * </pre>
   *
   * \sa http://www.w3.org/Graphics/GIF/spec-gif89a.txt
   * \sa http://marknelson.us/1989/10/01/lzw-data-compression/
   * \sa Nelson, M.R.: "LZW Data Compression", Dr. Dobb's Journal, October 1989.
   */
  struct LempelZivWelchDecompressor::State
  {
    bool conv(char const *&source_begin, char const *source_end,
              char *&target_begin, char *target_end, bool eoi)
    {
      if(eof_code_seen)
      {
        if(source_end - source_begin)
          throw IncConvException("Extraneous data after EOF code");
        return true;
      }

      for(;;)
      {
        // First transfer any previously generated output to callers
        // target buffer
        if(out_stack_dirty)
        {
          size_t n = out_stack_bottom - out_stack_top, m = target_end - target_begin;
          bool partial = m < n;
          if(partial) n = m;
          if(n)
          {
            char *e = out_stack_top + n;
            target_begin = copy(out_stack_top, e, target_begin);
            out_stack_top = e;
          }
          if(partial) return false; // Output stack not yet empty
          out_stack_dirty = false;
        }

        // Output stack is now empty

        // Add bits to the code assembler until we have enough
        while(bits_in_assembler < bits_per_code)
        {
          // If source buffer is dry
          if(source_begin == source_end)
          {
            if(eoi) throw IncConvException("Premature end of input");
            return true;
          }

          // Fetch and validate next input byte
          unsigned char c = *source_begin++;
          if(int_less_than(255, c))
            throw IncConvException("Byte value exceeds 8 bit range");

          // Add byte to assembler
          assembler |= static_cast<UIntFast32>(c) << bits_in_assembler;
          bits_in_assembler += 8;
        }

        // Extract next code from assembler
        unsigned code = static_cast<unsigned>(assembler) & code_mask;
        assembler >>= bits_per_code;
        bits_in_assembler -= bits_per_code;

        if(code == clear_code)
        {
          reset();
          continue;
        }
        if(code == eof_code)
        {
          if(source_begin < source_end || assembler)
            throw IncConvException("Extraneous data after EOF code");
          eof_code_seen = true;
          return true;
        }

        // Is this code an immediate byte
        if(code < clear_code)
        {
          // Bypass output stack when possible
          if(target_begin < target_end)
            *target_begin++ = static_cast<unsigned char>(code);
          else *--out_stack_top = static_cast<unsigned char>(code);
          last_suffix = static_cast<unsigned char>(code);
        }
        else
        {
          if(code < dictionary_size) last_suffix = push_dictionary_string(code);
          else
          {
            if(code != dictionary_size || prev_code == nil_code)
              throw IncConvException("LZW code out of range");

            // Special case where code is not yet in dictionary
            *--out_stack_top = last_suffix;
            last_suffix = push_dictionary_string(prev_code);
          }
        }
        out_stack_dirty = true;

        // Unless the dictionary is full or this is the first code
        // after a reset, register dict(prev_code) + last_suffix in
        // the dictionary.
        if(prev_code != nil_code && dictionary_size < max_num_codes)
        {
          prefix[dictionary_size] = prev_code;
          suffix[dictionary_size] = last_suffix;
          ++dictionary_size;

          // If dictionary size exceeds current number of bits per
          // code, then increment number of bits per code
          if(code_mask < dictionary_size && dictionary_size < max_num_codes)
            code_mask = (1u << ++bits_per_code) - 1u;
        }

        prev_code = code;
      }
    }

    char push_dictionary_string(unsigned code)
    {
      while(first_compound_code <= code)
      {
        *--out_stack_top = suffix[code];
        code = prefix[code];
      }
      return *--out_stack_top = static_cast<unsigned char>(code);
    }

    void reset()
    {
      bits_per_code   = bits_per_byte + 1;
      code_mask       = (1u << bits_per_code) - 1u;
      dictionary_size = first_compound_code;
      prev_code       = nil_code;
    }

    State(LempelZivWelchDecompressor const &d):
      bits_per_byte(d.bits_per_byte), clear_code(1u << bits_per_byte),
      eof_code(clear_code + 1), first_compound_code(eof_code + 1),
      out_stack_dirty(false), eof_code_seen(false),
      assembler(0), bits_in_assembler(0),
      out_stack_bottom(out_stack+sizeof(out_stack)), out_stack_top(out_stack_bottom)
    {
      reset();
    }

    int const bits_per_byte;
    unsigned const clear_code;          // = 1 << bits_per_byte
    unsigned const eof_code;            // = clear_code + 1
    unsigned const first_compound_code; // = clear_code + 2

    bool out_stack_dirty, eof_code_seen;
    UIntFast32 assembler;
    int bits_in_assembler, bits_per_code;
    unsigned code_mask, prev_code, dictionary_size;
    char last_suffix;

    /**
     * Together with <tt>suffix</tt>, this table defines the
     * dictionary in the following way:
     *
     * <pre>
     *
     *   dict(code) = code < first_compound_code ? chr(code) :
     *                dict(prefix[code]) + suffix[code]
     *
     * </pre>
     */
    UIntMin16 prefix[max_num_codes];

    /**
     * See <tt>prefix</tt>.
     */
    char suffix[max_num_codes];

    /**
     * The maximum number of bytes we need to be able to push to the
     * stack is 1 + the longest possible string in the dictionary,
     * which can be derived using:
     *
     * <pre>
     *
     *   max_len(code) = code < first_compound_code ? 1 : max_len(code-1) + 1
     *
     *   max_len(code) = max(code - first_compound_code + 2, 1)
     *
     * </pre>
     *
     * Since <tt>code < max_num_codes</tt> and
     * <tt>2**min_bits_per_byte + 2 <= first_compound_code</tt>, the
     * maximum length of a dictionary string must be
     * <tt>max_num_codes - 2**min_bits_per_byte - 1</tt>.
     */
    char out_stack[max_num_codes - (1u<<min_bits_per_byte)];

    char *const out_stack_bottom, *out_stack_top;
  };
}


namespace archon
{
  namespace Util
  {
    UniquePtr<Codec const> get_lempel_ziv_welch_codec(int bits_per_byte)
    {
      if(bits_per_byte < min_bits_per_byte || max_bits_per_byte < bits_per_byte)
        throw invalid_argument("'Bits per byte' is out of range");
      typedef LempelZivWelchCompressor   Enc;
      typedef LempelZivWelchDecompressor Dec;
      Enc enc(bits_per_byte);
      Dec dec(bits_per_byte);
      UniquePtr<Codec const> c(new IncConvCodec<Enc, Dec>(enc, dec));
      return c;
    }
  }
}
