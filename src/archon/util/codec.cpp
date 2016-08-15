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

#include <archon/util/codec.hpp>


using namespace std;
using namespace archon::Core;

namespace
{
  struct BlockDecodeInputStream: InputStream
  {
    size_t read(char_type *b, size_t n)
    {
      if(!n || eoi) return 0;

      size_t const n_orig = n;
      for(;;)
      {
        // Transfer remaining bytes of current block
        while(left)
        {
          size_t m = min(n, left);
          size_t r = in.read(b, m);
          if(!r) throw ReadException("BlockDecodeInputStream: "
                                     "Premature end of input");
          left -= r;
          n -= r;
          if(!n) return n_orig;
          if(b) b += r;
        }

        // Get size of next block
        {
          char c;
          size_t r = in.read(&c, 1);
          if(!r) throw ReadException("BlockDecodeInputStream: "
                                     "Premature end of input");
          if(!c)
          {
            eoi = true;
            return n_orig - n;
          }
          left = static_cast<unsigned char>(c);
        }
      }
    }

    BlockDecodeInputStream(InputStream &i, SharedPtr<InputStream> const &owner):
      in(i), in_owner(owner), left(0), eoi(false) {}

    InputStream &in;
    SharedPtr<InputStream> const in_owner;

    size_t left;
    bool eoi;
  };


  struct BlockEncodeOutputStream: OutputStream
  {
    void write(char const *b, size_t n)
    {
      if (!chunk) throw WriteException("Write after flush is not supported");
      for(;;)
      {
        // Transfer callers data to buffer
        size_t m = min(n, left);
        char const *e = b + m;
        copy(b, e, chunk_size - left + chunk);
        left -= m;
        n -= m;
        if(!n) return;
        b = e;

        // Close the filled chunk, and go to next
        chunk[0] = '\xFF';
        chunk += chunk_size;
        left = chunk_size-1;

        // Write to wrapped stream if buffer is full
        if(chunk == buffer + sizeof(buffer))
        {
          out.write(buffer, sizeof(buffer));
          chunk = buffer;
        }
      }
    }

    void flush()
    {
      if(!chunk) return;
      flush2();
      buffer[0] = '\0';
      out.write(buffer, 1);
      chunk = 0; // Force a fail on next write
    }

    void flush2()
    {
      size_t n = chunk_size-left-1;
      // If n is zero, the buffer is empty
      if(n)
      {
        chunk[0] = static_cast<unsigned char>(n);
        out.write(buffer, chunk+(256-left) - buffer);
        left = chunk_size-1;
        chunk = buffer;
      }
    }

    BlockEncodeOutputStream(OutputStream &o, SharedPtr<OutputStream> const &owner):
      out(o), out_owner(owner), chunk(buffer), left(chunk_size-1) {}

    ~BlockEncodeOutputStream()
    {
      try { flush(); } catch(...) {}
    }

    static size_t const chunk_size = 256; // Must never exceed 256

    OutputStream &out;
    SharedPtr<OutputStream> const out_owner;

    char buffer[4*chunk_size]; // Must be an integer multiple of chunk_size
    char *chunk;
    size_t left;
  };


  struct BlockCodec: Codec
  {
    string encode(string const &) const
    {
      throw runtime_error("Not implemented");
    }

    string decode(string const &) const
    {
      throw runtime_error("Not implemented");
    }

    UniquePtr<OutputStream> get_enc_out_stream(OutputStream &out) const
    {
      UniquePtr<OutputStream> s(new BlockEncodeOutputStream(out, SharedPtr<OutputStream>()));
      return s;
    }

    UniquePtr<InputStream> get_dec_in_stream(InputStream &in) const
    {
      UniquePtr<InputStream> s(new BlockDecodeInputStream(in, SharedPtr<InputStream>()));
      return s;
    }

    UniquePtr<InputStream> get_enc_in_stream(InputStream &) const
    {
      throw runtime_error("Not implemented");
    }

    UniquePtr<OutputStream> get_dec_out_stream(OutputStream &) const
    {
      throw runtime_error("Not implemented");
    }

    UniquePtr<OutputStream> get_enc_out_stream(SharedPtr<OutputStream> const &out) const
    {
      UniquePtr<OutputStream> s(new BlockEncodeOutputStream(*out, out));
      return s;
    }

    UniquePtr<InputStream> get_dec_in_stream(SharedPtr<InputStream> const &in) const
    {
      UniquePtr<InputStream> s(new BlockDecodeInputStream(*in, in));
      return s;
    }

    UniquePtr<InputStream> get_enc_in_stream(SharedPtr<InputStream> const &) const
    {
      throw runtime_error("Not implemented");
    }

    UniquePtr<OutputStream> get_dec_out_stream(SharedPtr<OutputStream> const &) const
    {
      throw runtime_error("Not implemented");
    }
  };
}

namespace archon
{
  namespace Util
  {
    UniquePtr<Codec const> get_block_codec()
    {
      UniquePtr<Codec const> c(new BlockCodec());
      return c;
    }
  }
}
