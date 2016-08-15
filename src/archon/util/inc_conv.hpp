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

#ifndef ARCHON_UTIL_INC_CONV_HPP
#define ARCHON_UTIL_INC_CONV_HPP

#include <stdexcept>
#include <algorithm>
#include <string>

#include <archon/core/stream.hpp>
#include <archon/core/codec.hpp>


namespace archon
{
  namespace util
  {
    struct IncConvException: std::runtime_error
    {
      IncConvException(std::string m): std::runtime_error(m) {}
    };


    /**
     * Create an input stream wrapper that uses the specified
     * incremental converter to process the data that it reads from
     * the wrapped input stream.
     *
     * The converter class must define all of the following:
     *
     * - A type \c source_char_type that specifies the type of
     *   characters that the conversion function expects as
     *   input. This also defines type of characters read from the
     *   wrapped input stream.
     *
     * - A type \c target_char_type that specifies the type of
     *   characters that the the conversion function produces as
     *   output. This also defines type of characters read from the
     *   new wrapper stream.
     *
     * - A constant \c min_source_buffer_size that specifies the
     *   minimum number of characters of type \c source_char_type that
     *   must be made available to the conversion function to
     *   guarantee that it can advance its conversion state. See below
     *   for more details on this.
     *
     * - A constant \c min_target_buffer_size that specifies the
     *   minimum number of characters of type \c target_char_type that
     *   must be made available to the conversion function to
     *   guarantee that it can advance its conversion state. See below
     *   for more details on this.
     *
     * - A class type \c State that encapsulates the state of the
     *   conversion.
     *
     * The \c State class must define the following:
     *
     * - A constructor that takes <tt>Conv const &</tt> as argument,
     *   where \c Conv is the converter class. This constructor is
     *   expected to initialize the conversion state.
     *
     * - A method \c conv that will be called by the stream wrapper to
     *   convert a chunk of data. See below for further details on
     *   this.
     *
     * The \c State::conv method must have the following signature:
     *
     * <pre>
     *
     *   bool conv(source_char_type const *&source_begin,
     *             source_char_type const *source_end,
     *             target_char_type *&target_begin,
     *             target_char_type *target_end, bool eoi)
     *
     * </pre>
     *
     * It is called repeatedly by the wrapper stream to convert a
     * sequence of data chunks. It must convert as much as possible,
     * each time it is called. Conversion can stop for three reasons:
     * Either there is not enough data left in the source buffer to
     * continue, or there is not enough space left in the output
     * buffer to continue, or the input data is invalid. If it runs
     * out of input, it must return true. If it needs more space for
     * output, it must return false. Otherwise, in case of invalid
     * input, it must throw a <tt>IncConvException</tt>. The meaning
     * of each argument is as follows:
     *
     * - \c source_begin and \c source_end specifies a chunk of source
     *   data as read from the wrapped stream. At entry \c source_begin
     *   will point to the first available source character, and \c
     *   source_end will point one beyond the last available source
     *   character. Before it returns, the conversion function must
     *   update the former to reflect the extent of successful
     *   conversion. The conversion function must be able to handle an
     *   empty source chunk.
     *
     * - \c target_begin and \c target_end specifies a chunk of free
     *   space in which the converted data must be stored. At entry \c
     *   target_begin will point to the first character of this chunk,
     *   and \c source_end will point one beyond the last. Before it
     *   returns, the conversion function must update the former to
     *   reflect the extent of successful conversion. The conversion
     *   function must be able to handle an empty target chunk.
     *
     * - \c eoi is a flag that signals the end of input. If it is set,
     *   the accompanying source chunk holds the final character from
     *   the wrapped input stream. The reverse is not guaranteed, that
     *   is, the \c eoi flag may be false even when the source chunk
     *   holds the final character, but the flag will become true
     *   eventually. That is, it is guaranteed that the conversion
     *   function will be called at least once with this flag set to
     *   true. See below for further details.
     *
     * The conversion function must guarantee that its conversion
     * state is strictly advanced if at the same time the size of the
     * source and target chunks respect the minimum specifications of
     * the conversion class (<tt>min_source_buffer_size</tt> and
     * <tt>min_target_buffer_size</tt>). At the end of input (when \c
     * eoi is true), it must guarantee strict advancement as long as
     * the target chunk respect its minimum specification. This is
     * important for eliminating the possibility of infinite looping.
     *
     * When the \c eoi argument is true, a return value of true will
     * be interpreted by the wrapper stream as 'successful completion
     * of conversion', and in this case the conversion function will
     * not be called again. Also, when the \c eoi flag is true, then
     * it will also be true on all successive calls.
     *
     * The following is an example of a converter class whose function
     * is to replace the character at a specific stream offset with
     * the NUL character. It is crafted to illustrate most of the
     * available features:
     *
     * \code
     *
     *   struct ReplaceWithNulAtOffset
     *   {
     *     size_t offset; // Replace with NUL char at this stream offset
     *
     *     typedef char source_char_type;
     *     typedef char target_char_type;
     *
     *     static int const min_source_buffer_size = 1;
     *     static int const min_target_buffer_size = 1;
     *
     *     struct State
     *     {
     *       State(ReplaceWithNullAtOffset const &c):
     *         replace_offset(c.offset), chunk_offset(0), replaced(false) {}
     *
     *       bool conv(char const *&source_begin, char const *&source_end,
     *                 char *&target_begin, char *target_end, bool eoi)
     *       {
     *         size_t n = std::min(source_end - source_begin,
     *                             target_end - target_begin);
     *         std::copy(source_begin, source_begin+n, target_begin);
     *
     *         if(!replaced && replace_offset < chunk_offset+n)
     *         {
     *           target_begin[replace_offset-chunk_offset] = 0;
     *           replaced = true;
     *         }
     *
     *         if(eoi && !replaced)
     *           throw IncConvException("Premature end of input");
     *
     *         source_begin += n;
     *         target_begin += n;
     *         chunk_offset += n;
     *
     *         return source_begin == source_end;
     *       }
     *
     *       size_t replace_offset, chunk_offset;
     *       bool replaced;
     *     };
     *   };
     *
     * \endcode
     *
     *
     * \note It is also possible to construct an output stream that
     * uses the same type of incremental converter as the one used
     * here. See \c make_inc_conv_out_stream for details.
     *
     * \sa make_inc_conv_out_stream
     */
    template<class Conv>
    core::UniquePtr<core::BasicInputStream<typename Conv::target_char_type> >
    make_inc_conv_in_stream(Conv const &converter,
                            core::BasicInputStream<typename
                            Conv::source_char_type> &in);


    /**
     * Create an output stream wrapper that uses the specified
     * incremental converter to process the data before it is written
     * to the wrapped output stream.
     *
     * Please see <tt>make_inc_conv_in_stream</tt> for information
     * about the converter class. This method has exactly the same
     * requirements towards the converter class, as
     * <tt>make_inc_conv_in_stream</tt> has.
     *
     * This output stream does not support writing after a
     * flush. Thaat is, a flush is effectively a close.
     */
    template<class Conv>
    core::UniquePtr<core::BasicOutputStream<typename Conv::source_char_type> >
    make_inc_conv_out_stream(Conv const &converter,
                             core::BasicOutputStream<typename
                             Conv::target_char_type> &out);


    template<class Conv>
    core::UniquePtr<core::BasicInputStream<typename Conv::target_char_type> >
    make_inc_conv_in_stream(Conv const &converter,
                            core::SharedPtr<core::BasicInputStream<typename
                            Conv::source_char_type> > const &in);

    template<class Conv>
    core::UniquePtr<core::BasicOutputStream<typename Conv::source_char_type> >
    make_inc_conv_out_stream(Conv const &converter,
                             core::SharedPtr<core::BasicOutputStream<typename
                             Conv::target_char_type> > const &out);



    /**
     * Convert a string using an incremental converter as defined in
     * <tt>make_inc_conv_out_stream</tt>.
     */
    template<class Conv> std::basic_string<typename Conv::target_char_type>
    inc_convert(Conv const &conv,
                std::basic_string<typename Conv::source_char_type>);



    /**
     * A codec object where encoding and decoding are specified as two
     * incremental converter objects, <tt>enc</tt> and
     * <tt>dec</tt>. Each object must be an instance of an incremental
     * converter class as defined in
     * <tt>make_inc_conv_out_stream</tt>. Further more, the
     * <tt>source_char_type</tt> of the encoder must be equal to the
     * <tt>target_char_type</tt> of the decoder, and both the
     * <tt>target_char_type</tt> of the encoder and the
     * <tt>source_char_type</tt> of the decoder must be equal to plain
     * <tt>char</tt>.
     */
    template<class Enc, class Dec>
    struct IncConvCodec: core::BasicCodec<typename Enc::source_char_type>
    {
      typedef core::BasicCodec<typename Enc::source_char_type> Base;
      typedef typename Base::StringType       StringType;
      typedef typename Base::InputStreamType  InputStreamType;
      typedef typename Base::OutputStreamType OutputStreamType;

      IncConvCodec(Enc enc, Dec dec): enc(enc), dec(dec) {}

      std::string encode(StringType const &) const;
      StringType decode(std::string const &) const;


      core::UniquePtr<OutputStreamType> get_enc_out_stream(core::OutputStream &) const;

      core::UniquePtr<InputStreamType> get_dec_in_stream(core::InputStream &) const;

      core::UniquePtr<core::InputStream> get_enc_in_stream(InputStreamType &) const;

      core::UniquePtr<core::OutputStream> get_dec_out_stream(OutputStreamType &) const;


      core::UniquePtr<OutputStreamType>
      get_enc_out_stream(core::SharedPtr<core::OutputStream> const &) const;

      core::UniquePtr<InputStreamType>
      get_dec_in_stream(core::SharedPtr<core::InputStream> const &) const;

      core::UniquePtr<core::InputStream>
      get_enc_in_stream(core::SharedPtr<InputStreamType> const &) const;

      core::UniquePtr<core::OutputStream>
      get_dec_out_stream(core::SharedPtr<OutputStreamType> const &) const;


    private:
      Enc const enc;
      Dec const dec;
    };








    // Implementations:

    namespace _impl
    {
      template<class C>
      struct IncConvInputStream:
        core::BasicInputStream<typename C::target_char_type>
      {
        typedef C Converter;
        typedef typename C::source_char_type SourceChar;
        typedef typename C::target_char_type TargetChar;
        typedef core::BasicInputStream<SourceChar> SourceStream;


        // Accepts NULL in place of the buffer for discarding input
        size_t read(TargetChar *b, size_t n)
        {
          // Besides adhering to the rule of 'minimum number of
          // sub-reads' described in 'BasicInputStream', we also want to
          // promote 'long' conversions and long reads from the wrapped
          // source. This gives rise to the following algorithm:
          //
          // For ever:
          //   Transfer as much data as possible from output buffer to
          //     callers buffer
          //   If callers buffer is full: Return
          //   Now output buffer must be empty
          //   If previous conversion did not stop due to lack of space in output:
          //     If we already did read from the wrapped source and
          //       at least one char was previously transferred  to the callers
          //       buffer: Return
          //     Copy remaining data in input buffer back to start
          //     Read as much as possible from the wrapped source
          //   Convert as much as possible

          if (!n) return 0;
          try {
            if (eoi) return 0;
            size_t n_orig = n;
            bool did_read = false;
            for (;;) {
              // First consume anything that was previously emitted by
              // the converter
              size_t left = out_end - out;
              if (left)
              {
                bool partial = n < left;
                size_t m = partial ? n : left;
                TargetChar *end = out + m;
                std::copy(out, end, b);
                // If we emptied the output buffer, we must reset it
                // to make room for more data
                if (partial) out = end;
                else out_end = out = out_buf;
                n -= m;
                // Return now if we filled callers buffer
                if (!n) return n_orig;
                b += m;
              }

              // Output buffer must be empty now, since we did not
              // fill callers buffer

              // Return now if nothing more can be emitted to the
              // output buffer
              if (out_closed) {
                eoi = true;
                return n_orig - n;
              }

              // Read some more if previous conversion stopped due to
              // lack of input
              if (in_lack) {
                // If input stream is dry, this was the last emission to
                // the output buffer
                if (in_closed) {
                  out_closed = true;
                  continue;
                }

                // We only want to read multiple times if we have to
                if (did_read && n < n_orig) return n_orig - n;

                // Copy remaining data in input buffer back to start
                size_t in_left = in_end - in;
                SourceChar *end = in_buf + in_left;
                in = std::copy_backward(in, in_end, end);
                in_end = end;

                // Read as much as possible from the wrapped source
                size_t m = source.read(in_end, in_buf_size - in_left);
                if (m) {
                  in_end += m;
                  did_read = true;
                }
                else in_closed = true; // No more emission to in_buf
                in_lack = false;
              }

              // Convert as much as possible
              in_lack = converter.conv(const_cast<SourceChar const *&>(in),
                                       in_end, out_end, out_buf_end, in_closed);
            }
          }
          catch (IncConvException &e) {
            throw core::ReadException(e.what());
          }
        }


        IncConvInputStream(SourceStream &src, core::SharedPtr<SourceStream> const &owner,
                           Converter const &c):
          source(src), source_owner(owner), converter(c), in(in_buf), in_end(in),
          out(out_buf), out_end(out), out_buf_end(out_buf+out_buf_size),
          in_lack(true), in_closed(false), out_closed(false), eoi(false) {}


        // At least 1024 bytes in both input and output buffer
        static size_t const in_buf_default_size =
          (1024 + sizeof(SourceChar)-1) / sizeof(SourceChar);
        static size_t const out_buf_default_size =
          (1024 + sizeof(TargetChar)-1) / sizeof(TargetChar);

        static size_t const in_buf_size = in_buf_default_size <
          static_cast<size_t>(Converter::min_source_buffer_size) ?
          Converter::min_source_buffer_size : in_buf_default_size;
        static size_t const out_buf_size = out_buf_default_size <
          static_cast<size_t>(Converter::min_target_buffer_size) ?
          Converter::min_target_buffer_size : out_buf_default_size;

        SourceStream &source;
        core::SharedPtr<SourceStream> const source_owner;

        typename Converter::State converter;
        SourceChar in_buf[in_buf_size];
        TargetChar out_buf[out_buf_size];
        SourceChar *in,  *in_end;
        TargetChar *out, *out_end, *const out_buf_end;
        bool in_lack, in_closed, out_closed, eoi;
      };



      template<class C>
      struct IncConvOutputStream:
        core::BasicOutputStream<typename C::source_char_type>
      {
        typedef C Converter;
        typedef typename C::source_char_type SourceChar;
        typedef typename C::target_char_type TargetChar;
        typedef core::BasicOutputStream<TargetChar> TargetStream;


        void write(SourceChar const *b, size_t n)
        {
          // Besides adhering to the rule of 'minimum number of
          // sub-writes' described in 'BasicOutputStream', we also
          // want to promote 'long' conversions and long writes to the
          // wrapped destination. This gives rise to the following
          // algorithm:
          //
          // For ever:
          //   Transfer as much data as possible from callers buffer to
          //     input buffer
          //   If callers buffer is empty: return
          //   Now input buffer must be full
          //   For ever:
          //     Convert as much as possible
          //     If transcoder stopped due to lack of input: break
          //     Write all data in output buffer to wrapped destination
          //   Copy remaining data in input buffer back to start

          if (!n) return;
          try {
            if (closed) throw std::runtime_error("Write to closed stream");

            for (;;) {
              // Transfer as much data as possible from callers buffer to
              // input buffer
              size_t in_free = in_buf_end - in_end;
              if (in_free) {
                size_t m = std::min(n, in_free);
                in_end = std::copy(b, b+m, in_end);
                n -= m;
                if (!n) return; // Callers buffer is empty
                b += m;
              }

              // Input buffer is now completely full

              flush_in_buf(false);
            }
          }
          catch (IncConvException &e) {
            throw core::WriteException(e.what());
          }
        }


        void flush()
        {
          try
          {
            if(closed) return;
            flush_in_buf(true);
            flush_out_buf();
            closed = true;
          }
          catch(IncConvException &e)
          {
            throw core::WriteException(e.what());
          }
          target.flush();
        }


        void flush_in_buf(bool eoi)
        {
          SourceChar *in = in_buf;

          // Keep flushing output untill more input is required
          while(!converter.conv(const_cast<SourceChar const *&>(in),
                                in_end, out_end, out_buf_end, eoi))
            flush_out_buf();

          // Copy remaining data in input buffer back to start
          SourceChar *end = in_buf + (in_end - in);
          std::copy_backward(in, in_end, end);
          in_end = end;
        }


        void flush_out_buf()
        {
          size_t n = out_end - out_buf;
          if(n)
          {
            target.write(out_buf, n);
            out_end = out_buf;
          }
        }


        IncConvOutputStream(TargetStream &tgt, core::SharedPtr<TargetStream> const &owner,
                            Converter const &c):
          target(tgt), target_owner(owner), converter(c), closed(false),
          in_end(in_buf), in_buf_end(in_buf+in_buf_size),
          out_end(out_buf), out_buf_end(out_buf+out_buf_size) {}

        ~IncConvOutputStream()
        {
          try { flush(); } catch(...) {}
        }


        // At least 1024 bytes in both input and output buffer
        static size_t const in_buf_default_size =
          (1024 + sizeof(SourceChar)-1) / sizeof(SourceChar);
        static size_t const out_buf_default_size =
          (1024 + sizeof(TargetChar)-1) / sizeof(TargetChar);

        static size_t const in_buf_size = in_buf_default_size <
          static_cast<size_t>(Converter::min_source_buffer_size) ?
          Converter::min_source_buffer_size : in_buf_default_size;
        static size_t const out_buf_size = out_buf_default_size <
          static_cast<size_t>(Converter::min_target_buffer_size) ?
          Converter::min_target_buffer_size : out_buf_default_size;

        TargetStream &target;
        core::SharedPtr<TargetStream> const target_owner;

        typename Converter::State converter;
        bool closed;
        SourceChar in_buf[in_buf_size];
        TargetChar out_buf[out_buf_size];
        SourceChar *in_end,  *const in_buf_end;
        TargetChar *out_end, *const out_buf_end;
      };
    }


    template<class C>
    core::UniquePtr<core::BasicInputStream<typename C::target_char_type> >
    make_inc_conv_in_stream(C const &converter,
                            core::BasicInputStream<typename
                            C::source_char_type> &in)
    {
      typedef _impl::IncConvInputStream<C>                         Stream1;
      typedef core::BasicInputStream<typename C::source_char_type> Stream2;
      typedef core::BasicInputStream<typename C::target_char_type> Stream3;
      core::UniquePtr<Stream3> s(new Stream1(in, core::SharedPtr<Stream2>(), converter));
      return s;
    }


    template<class C>
    core::UniquePtr<core::BasicOutputStream<typename C::source_char_type> >
    make_inc_conv_out_stream(C const &converter,
                             core::BasicOutputStream<typename
                             C::target_char_type> &out)
    {
      typedef _impl::IncConvOutputStream<C> Stream1;
      typedef core::BasicOutputStream<typename C::source_char_type> Stream2;
      typedef core::BasicOutputStream<typename C::target_char_type> Stream3;
      core::UniquePtr<Stream2> s(new Stream1(out, core::SharedPtr<Stream3>(), converter));
      return s;
    }


    template<class C>
    core::UniquePtr<core::BasicInputStream<typename C::target_char_type> >
    make_inc_conv_in_stream(C const &converter,
                            core::SharedPtr<core::BasicInputStream<typename
                            C::source_char_type> > const &in)
    {
      typedef _impl::IncConvInputStream<C>                         Stream1;
      typedef core::BasicInputStream<typename C::target_char_type> Stream2;
      core::UniquePtr<Stream2> s(new Stream1(*in, in, converter));
      return s;
    }


    template<class C>
    core::UniquePtr<core::BasicOutputStream<typename C::source_char_type> >
    make_inc_conv_out_stream(C const &converter,
                             core::SharedPtr<core::BasicOutputStream<typename
                             C::target_char_type> > const &out)
    {
      typedef _impl::IncConvOutputStream<C> Stream1;
      typedef core::BasicOutputStream<typename C::source_char_type> Stream2;
      core::UniquePtr<Stream2> s(new Stream1(*out, out, converter));
      return s;
    }



    template<class Conv> std::basic_string<typename Conv::target_char_type>
    inline inc_convert(Conv const &conv,
                       std::basic_string<typename Conv::source_char_type> s)
    {
      typedef typename Conv::source_char_type SourceChar;
      typedef typename Conv::target_char_type TargetChar;

      SourceChar const *in = s.data(), *const in_end = in + s.size();

      // At least 512 bytes in buffer
      size_t const buf_default_size =
        (512 + sizeof(TargetChar) - 1) / sizeof(TargetChar);
      size_t const buf_size =
        buf_default_size < static_cast<size_t>(Conv::min_target_buffer_size) ?
        Conv::min_target_buffer_size : buf_default_size;
      TargetChar buffer[buf_size];
      TargetChar *out = buffer, *const out_end = out + buf_size;

      typename Conv::State converter(conv);
      std::basic_string<TargetChar> t;
      for(;;)
      {
        bool last = converter.conv(in, in_end, out, out_end, true);
        t.append(buffer, out - buffer);
        if(last) return t;
        out = buffer;
      }
    }


    template<class Enc, class Dec> std::string
    IncConvCodec<Enc, Dec>::encode(StringType const &s) const
    {
      try
      {
        return inc_convert(enc, s);
      }
      catch(IncConvException &e)
      {
        throw core::EncodeException(e.what());
      }
    }


    template<class Enc, class Dec> typename IncConvCodec<Enc, Dec>::StringType
    IncConvCodec<Enc, Dec>::decode(std::string const &s) const
    {
      try
      {
        return inc_convert(dec, s);
      }
      catch(IncConvException &e)
      {
        throw core::DecodeException(e.what());
      }
    }


    template<class Enc, class Dec> core::UniquePtr<typename IncConvCodec<Enc, Dec>::OutputStreamType>
    IncConvCodec<Enc, Dec>::get_enc_out_stream(core::OutputStream &out) const
    {
      core::UniquePtr<typename IncConvCodec<Enc, Dec>::OutputStreamType>
        s(make_inc_conv_out_stream(enc, out).release());
      return s;
    }

    template<class Enc, class Dec> core::UniquePtr<typename IncConvCodec<Enc, Dec>::InputStreamType>
    IncConvCodec<Enc, Dec>::get_dec_in_stream(core::InputStream &in) const
    {
      core::UniquePtr<typename IncConvCodec<Enc, Dec>::InputStreamType>
        s(make_inc_conv_in_stream(dec, in).release());
      return s;
    }

    template<class Enc, class Dec> core::UniquePtr<core::InputStream> IncConvCodec<Enc, Dec>::
    get_enc_in_stream(InputStreamType &in) const
    {
      core::UniquePtr<core::InputStream> s(make_inc_conv_in_stream(enc, in).release());
      return s;
    }

    template<class Enc, class Dec> core::UniquePtr<core::OutputStream> IncConvCodec<Enc, Dec>::
    get_dec_out_stream(OutputStreamType &out) const
    {
      core::UniquePtr<core::OutputStream> s(make_inc_conv_out_stream(dec, out).release());
      return s;
    }


    template<class Enc, class Dec> core::UniquePtr<typename IncConvCodec<Enc, Dec>::OutputStreamType>
    IncConvCodec<Enc, Dec>::get_enc_out_stream(core::SharedPtr<core::OutputStream> const &out) const
    {
      core::UniquePtr<typename IncConvCodec<Enc, Dec>::OutputStreamType>
        s(make_inc_conv_out_stream(enc, out).release());
      return s;
    }

    template<class Enc, class Dec> core::UniquePtr<typename IncConvCodec<Enc, Dec>::InputStreamType>
    IncConvCodec<Enc, Dec>::get_dec_in_stream(core::SharedPtr<core::InputStream> const &in) const
    {
      core::UniquePtr<typename IncConvCodec<Enc, Dec>::InputStreamType>
        s(make_inc_conv_in_stream(dec, in).release());
      return s;
    }

    template<class Enc, class Dec> core::UniquePtr<core::InputStream>
    IncConvCodec<Enc, Dec>::get_enc_in_stream(core::SharedPtr<InputStreamType> const &in) const
    {
      core::UniquePtr<core::InputStream> s(make_inc_conv_in_stream(enc, in).release());
      return s;
    }

    template<class Enc, class Dec> core::UniquePtr<core::OutputStream>
    IncConvCodec<Enc, Dec>::get_dec_out_stream(core::SharedPtr<OutputStreamType> const &out) const
    {
      core::UniquePtr<core::OutputStream> s(make_inc_conv_out_stream(dec, out).release());
      return s;
    }
  }
}

#endif // ARCHON_UTIL_INC_CONV_HPP
