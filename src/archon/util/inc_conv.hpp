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

#ifndef ARCHON_UTIL_INC_CONV_HPP
#define ARCHON_UTIL_INC_CONV_HPP

#include <stdexcept>
#include <algorithm>
#include <string>

#include <archon/core/stream.hpp>
#include <archon/core/codec.hpp>


namespace archon {
namespace util {

class IncConvException: public std::runtime_error {
public:
    IncConvException(const std::string& message):
        std::runtime_error(message)
    {
    }
};


/// Create an input stream wrapper that uses the specified incremental converter
/// to process the data that it reads from the wrapped input stream.
///
/// The converter class must define all of the following:
///
/// - A type \c source_char_type that specifies the type of characters that the
///   conversion function expects as input. This also defines type of characters
///   read from the wrapped input stream.
///
/// - A type \c target_char_type that specifies the type of characters that the
///   the conversion function produces as output. This also defines type of
///   characters read from the new wrapper stream.
///
/// - A constant \c min_source_buffer_size that specifies the minimum number of
///   characters of type \c source_char_type that must be made available to the
///   conversion function to guarantee that it can advance its conversion
///   state. See below for more details on this.
///
/// - A constant \c min_target_buffer_size that specifies the minimum number of
///   characters of type \c target_char_type that must be made available to the
///   conversion function to guarantee that it can advance its conversion
///   state. See below for more details on this.
///
/// - A class type \c State that encapsulates the state of the conversion.
///
/// The \c State class must define the following:
///
/// - A constructor that takes <tt>const Conv&</tt> as argument, where \c Conv
///   is the converter class. This constructor is expected to initialize the
///   conversion state.
///
/// - A method \c conv that will be called by the stream wrapper to convert a
///   chunk of data. See below for further details on this.
///
/// The \c State::conv method must have the following signature:
///
/// <pre>
///
///   bool conv(const source_char_type*& source_begin,
///             const source_char_type* source_end,
///             target_char_type*& target_begin,
///             target_char_type* target_end, bool eoi)
///
/// </pre>
///
/// It is called repeatedly by the wrapper stream to convert a sequence of data
/// chunks. It must convert as much as possible, each time it is
/// called. Conversion can stop for three reasons: Either there is not enough
/// data left in the source buffer to continue, or there is not enough space
/// left in the output buffer to continue, or the input data is invalid. If it
/// runs out of input, it must return true. If it needs more space for output,
/// it must return false. Otherwise, in case of invalid input, it must throw a
/// <tt>IncConvException</tt>. The meaning of each argument is as follows:
///
/// - \c source_begin and \c source_end specifies a chunk of source data as read
///   from the wrapped stream. At entry \c source_begin will point to the first
///   available source character, and \c source_end will point one beyond the
///   last available source character. Before it returns, the conversion
///   function must update the former to reflect the extent of successful
///   conversion. The conversion function must be able to handle an empty source
///   chunk.
///
/// - \c target_begin and \c target_end specifies a chunk of free space in which
///   the converted data must be stored. At entry \c target_begin will point to
///   the first character of this chunk, and \c source_end will point one beyond
///   the last. Before it returns, the conversion function must update the
///   former to reflect the extent of successful conversion. The conversion
///   function must be able to handle an empty target chunk.
///
/// - \c eoi is a flag that signals the end of input. If it is set, the
///   accompanying source chunk holds the final character from the wrapped input
///   stream. The reverse is not guaranteed, that is, the \c eoi flag may be
///   false even when the source chunk holds the final character, but the flag
///   will become true eventually. That is, it is guaranteed that the conversion
///   function will be called at least once with this flag set to true. See
///   below for further details.
///
/// The conversion function must guarantee that its conversion state is strictly
/// advanced if at the same time the size of the source and target chunks
/// respect the minimum specifications of the conversion class
/// (<tt>min_source_buffer_size</tt> and <tt>min_target_buffer_size</tt>). At
/// the end of input (when \c eoi is true), it must guarantee strict advancement
/// as long as the target chunk respect its minimum specification. This is
/// important for eliminating the possibility of infinite looping.
///
/// When the \c eoi argument is true, a return value of true will be interpreted
/// by the wrapper stream as 'successful completion of conversion', and in this
/// case the conversion function will not be called again. Also, when the \c eoi
/// flag is true, then it will also be true on all successive calls.
///
/// The following is an example of a converter class whose function is to
/// replace the character at a specific stream offset with the NUL character. It
/// is crafted to illustrate most of the available features:
///
/// \code{.cpp}
///
///   class ReplaceWithNulAtOffset {
///   public:
///       std::size_t offset; // Replace with NUL char at this stream offset
///
///       using source_char_type = char;
///       using target_char_type = char;
///
///       static constexpr int min_source_buffer_size = 1;
///       static constexpr int min_target_buffer_size = 1;
///
///       class State {
///       public:
///           State(const ReplaceWithNullAtOffset& c):
///               replace_offset(c.offset)
///           {
///           }
///
///           bool conv(const char*& source_begin, const char*& source_end,
///                     char*& target_begin, char* target_end, bool eoi)
///           {
///               std::size_t n = std::min(source_end - source_begin,
///                                        target_end - target_begin);
///               std::copy(source_begin, source_begin+n, target_begin);
///
///               if (!replaced && replace_offset < chunk_offset+n) {
///                   target_begin[replace_offset-chunk_offset] = 0;
///                   replaced = true;
///               }
///
///               if (eoi && !replaced)
///                   throw IncConvException("Premature end of input");
///
///               source_begin += n;
///               target_begin += n;
///               chunk_offset += n;
///
///               return source_begin == source_end;
///           }
///
///           std::size_t replace_offset, chunk_offset = 0;
///           bool replaced = false;
///       };
///   };
///
/// \endcode
///
///
/// \note It is also possible to construct an output stream that uses the same
/// type of incremental converter as the one used here. See \c
/// make_inc_conv_out_stream for details.
///
/// \sa make_inc_conv_out_stream
template<class Conv>
std::unique_ptr<core::BasicInputStream<typename Conv::target_char_type>>
make_inc_conv_in_stream(const Conv& converter,
                        core::BasicInputStream<typename Conv::source_char_type>& in);


/// Create an output stream wrapper that uses the specified incremental
/// converter to process the data before it is written to the wrapped output
/// stream.
///
/// Please see <tt>make_inc_conv_in_stream</tt> for information about the
/// converter class. This method has exactly the same requirements towards the
/// converter class, as <tt>make_inc_conv_in_stream</tt> has.
///
/// This output stream does not support writing after a flush. Thaat is, a flush
/// is effectively a close.
template<class Conv>
std::unique_ptr<core::BasicOutputStream<typename Conv::source_char_type>>
make_inc_conv_out_stream(const Conv& converter,
                         core::BasicOutputStream<typename Conv::target_char_type>& out);


template<class Conv>
std::unique_ptr<core::BasicInputStream<typename Conv::target_char_type>>
make_inc_conv_in_stream(const Conv& converter,
                        const std::shared_ptr<core::BasicInputStream<typename
                        Conv::source_char_type>>& in);

template<class Conv>
std::unique_ptr<core::BasicOutputStream<typename Conv::source_char_type>>
make_inc_conv_out_stream(const Conv& converter,
                         const std::shared_ptr<core::BasicOutputStream<typename
                         Conv::target_char_type>>& out);



/// Convert a string using an incremental converter as defined in
/// <tt>make_inc_conv_out_stream</tt>.
template<class Conv> std::basic_string<typename Conv::target_char_type>
inc_convert(const Conv&, std::basic_string<typename Conv::source_char_type>);



/// A codec object where encoding and decoding are specified as two incremental
/// converter objects, <tt>enc</tt> and <tt>dec</tt>. Each object must be an
/// instance of an incremental converter class as defined in
/// <tt>make_inc_conv_out_stream</tt>. Further more, the
/// <tt>source_char_type</tt> of the encoder must be equal to the
/// <tt>target_char_type</tt> of the decoder, and both the
/// <tt>target_char_type</tt> of the encoder and the <tt>source_char_type</tt>
/// of the decoder must be equal to plain <tt>char</tt>.
template<class Enc, class Dec>
class IncConvCodec: public core::BasicCodec<typename Enc::source_char_type> {
public:
    using Base = core::BasicCodec<typename Enc::source_char_type>;
    using StringType = typename Base::StringType;
    using InputStreamType = typename Base::InputStreamType;
    using OutputStreamType = typename Base::OutputStreamType;

    IncConvCodec(Enc enc, Dec dec):
        enc(enc),
        dec(dec)
    {
    }

    std::string encode(const StringType&) const;
    StringType decode(const std::string&) const;


    std::unique_ptr<OutputStreamType> get_enc_out_stream(core::OutputStream&) const;

    std::unique_ptr<InputStreamType> get_dec_in_stream(core::InputStream&) const;

    std::unique_ptr<core::InputStream> get_enc_in_stream(InputStreamType&) const;

    std::unique_ptr<core::OutputStream> get_dec_out_stream(OutputStreamType&) const;


    std::unique_ptr<OutputStreamType>
    get_enc_out_stream(const std::shared_ptr<core::OutputStream>&) const;

    std::unique_ptr<InputStreamType>
    get_dec_in_stream(const std::shared_ptr<core::InputStream>&) const;

    std::unique_ptr<core::InputStream>
    get_enc_in_stream(const std::shared_ptr<InputStreamType>&) const;

    std::unique_ptr<core::OutputStream>
    get_dec_out_stream(const std::shared_ptr<OutputStreamType>&) const;


private:
    const Enc enc;
    const Dec dec;
};




// Implementations

namespace _impl {

template<class C> class IncConvInputStream:
        public core::BasicInputStream<typename C::target_char_type> {
public:
    using Converter = C;
    using SourceChar = typename C::source_char_type;
    using TargetChar = typename C::target_char_type;
    using SourceStream = core::BasicInputStream<SourceChar>;

    // Accepts NULL in place of the buffer for discarding input
    std::size_t read(TargetChar *b, std::size_t n) override
    {
        // Besides adhering to the rule of 'minimum number of sub-reads'
        // described in 'BasicInputStream', we also want to promote 'long'
        // conversions and long reads from the wrapped source. This gives rise
        // to the following algorithm:
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

        if (n == 0)
            return 0;
        try {
            if (eoi)
                return 0;
            std::size_t n_orig = n;
            bool did_read = false;
            for (;;) {
                // First consume anything that was previously emitted by the
                // converter
                std::size_t left = out_end - out;
                if (left) {
                    bool partial = (n < left);
                    size_t m = (partial ? n : left);
                    TargetChar* end = out + m;
                    std::copy(out, end, b);
                    // If we emptied the output buffer, we must reset it to make
                    // room for more data
                    if (partial) {
                        out = end;
                    }
                    else {
                        out_end = out = out_buf;
                    }
                    n -= m;
                    // Return now if we filled callers buffer
                    if (!n)
                        return n_orig;
                    b += m;
                }

                // Output buffer must be empty now, since we did not fill
                // callers buffer

                // Return now if nothing more can be emitted to the output
                // buffer
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
                    if (did_read && n < n_orig)
                        return n_orig - n;

                    // Copy remaining data in input buffer back to start
                    std::size_t in_left = in_end - in;
                    SourceChar* end = in_buf + in_left;
                    in = std::copy_backward(in, in_end, end);
                    in_end = end;

                    // Read as much as possible from the wrapped source
                    std::size_t m = source.read(in_end, in_buf_size - in_left);
                    if (m) {
                        in_end += m;
                        did_read = true;
                    }
                    else {
                        in_closed = true; // No more emission to in_buf
                    }
                    in_lack = false;
                }

                // Convert as much as possible
                in_lack = converter.conv(const_cast<const SourceChar*&>(in),
                                         in_end, out_end, out_buf_end, in_closed);
            }
        }
        catch (IncConvException& e) {
            throw core::ReadException(e.what());
        }
    }


    IncConvInputStream(SourceStream& src, const std::shared_ptr<SourceStream>& owner,
                       const Converter& c):
        source(src),
        source_owner(owner),
        converter(c)
    {
    }


    // At least 1024 bytes in both input and output buffer
    static constexpr std::size_t in_buf_default_size =
        (1024 + sizeof (SourceChar) - 1) / sizeof (SourceChar);
    static constexpr std::size_t out_buf_default_size =
        (1024 + sizeof (TargetChar) - 1) / sizeof (TargetChar);

    static constexpr std::size_t in_buf_size = in_buf_default_size <
        static_cast<std::size_t>(Converter::min_source_buffer_size) ?
        Converter::min_source_buffer_size : in_buf_default_size;
    static constexpr std::size_t out_buf_size = out_buf_default_size <
        static_cast<std::size_t>(Converter::min_target_buffer_size) ?
        Converter::min_target_buffer_size : out_buf_default_size;

    SourceStream& source;
    const std::shared_ptr<SourceStream> source_owner;

    typename Converter::State converter;
    SourceChar in_buf[in_buf_size];
    TargetChar out_buf[out_buf_size];
    SourceChar* in = in_buf;
    SourceChar* in_end = in;
    TargetChar* out = out_buf;
    TargetChar* out_end = out;
    TargetChar* const out_buf_end = out_buf + out_buf_size;
    bool in_lack = true, in_closed = false, out_closed = false, eoi = false;
};



template<class C> class IncConvOutputStream:
        public core::BasicOutputStream<typename C::source_char_type> {
public:
    using Converter = C;
    using SourceChar = typename C::source_char_type;
    using TargetChar = typename C::target_char_type;
    using TargetStream = core::BasicOutputStream<TargetChar>;

    void write(const SourceChar* b, std::size_t n) override
    {
        // Besides adhering to the rule of 'minimum number of sub-writes'
        // described in 'BasicOutputStream', we also want to promote 'long'
        // conversions and long writes to the wrapped destination. This gives
        // rise to the following algorithm:
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

        if (n == 0)
            return;
        try {
            if (closed)
                throw std::runtime_error("Write to closed stream");

            for (;;) {
                // Transfer as much data as possible from callers buffer to
                // input buffer
                std::size_t in_free = in_buf_end - in_end;
                if (in_free) {
                    std::size_t m = std::min(n, in_free);
                    in_end = std::copy(b, b+m, in_end);
                    n -= m;
                    if (n == 0)
                        return; // Callers buffer is empty
                    b += m;
                }

                // Input buffer is now completely full

                flush_in_buf(false);
            }
        }
        catch (IncConvException& e) {
            throw core::WriteException(e.what());
        }
    }

    void flush() override
    {
        try {
            if (closed)
                return;
            flush_in_buf(true);
            flush_out_buf();
            closed = true;
        }
        catch (IncConvException& e) {
            throw core::WriteException(e.what());
        }
        target.flush();
    }

    void flush_in_buf(bool eoi)
    {
        SourceChar* in = in_buf;

        // Keep flushing output untill more input is required
        while (!converter.conv(const_cast<const SourceChar*&>(in),
                               in_end, out_end, out_buf_end, eoi))
            flush_out_buf();

        // Copy remaining data in input buffer back to start
        SourceChar* end = in_buf + (in_end - in);
        std::copy_backward(in, in_end, end);
        in_end = end;
    }


    void flush_out_buf()
    {
        std::size_t n = out_end - out_buf;
        if (n != 0) {
            target.write(out_buf, n);
            out_end = out_buf;
        }
    }


    IncConvOutputStream(TargetStream& tgt, const std::shared_ptr<TargetStream>& owner,
                        const Converter& c):
        target(tgt),
        target_owner(owner),
        converter(c)
    {
    }

    ~IncConvOutputStream()
    {
        try {
            flush();
        }
        catch (...) {
        }
    }


    // At least 1024 bytes in both input and output buffer
    static constexpr std::size_t in_buf_default_size =
        (1024 + sizeof (SourceChar) - 1) / sizeof (SourceChar);
    static constexpr std::size_t out_buf_default_size =
        (1024 + sizeof (TargetChar) - 1) / sizeof (TargetChar);

    static constexpr std::size_t in_buf_size = in_buf_default_size <
        static_cast<std::size_t>(Converter::min_source_buffer_size) ?
        Converter::min_source_buffer_size : in_buf_default_size;
    static constexpr std::size_t out_buf_size = out_buf_default_size <
        static_cast<std::size_t>(Converter::min_target_buffer_size) ?
        Converter::min_target_buffer_size : out_buf_default_size;

    TargetStream& target;
    const std::shared_ptr<TargetStream> target_owner;

    typename Converter::State converter;
    bool closed = false;
    SourceChar in_buf[in_buf_size];
    TargetChar out_buf[out_buf_size];
    SourceChar* in_end = in_buf;
    SourceChar* const in_buf_end = in_buf + in_buf_size;
    TargetChar* out_end = out_buf;
    TargetChar* const out_buf_end = out_buf + out_buf_size;
};

} // unnamed namespace


template<class C> std::unique_ptr<core::BasicInputStream<typename C::target_char_type>>
make_inc_conv_in_stream(const C& converter,
                        core::BasicInputStream<typename C::source_char_type>& in)
{
    return std::make_unique<_impl::IncConvInputStream<C>>(in, nullptr, converter); // Throws
}


template<class C> std::unique_ptr<core::BasicOutputStream<typename C::source_char_type>>
make_inc_conv_out_stream(const C& converter,
                         core::BasicOutputStream<typename C::target_char_type>& out)
{
    return std::make_unique<_impl::IncConvOutputStream<C>>(out, nullptr, converter); // Throws
}


template<class C> std::unique_ptr<core::BasicInputStream<typename C::target_char_type>>
make_inc_conv_in_stream(const C& converter, const std::shared_ptr<core::BasicInputStream<typename
                        C::source_char_type>>& in)
{
    return std::make_unique<_impl::IncConvInputStream<C>>(*in, in, converter); // Throws
}


template<class C> std::unique_ptr<core::BasicOutputStream<typename C::source_char_type>>
make_inc_conv_out_stream(const C& converter, const std::shared_ptr<core::BasicOutputStream<typename
                         C::target_char_type>>& out)
{
    return std::make_unique<_impl::IncConvOutputStream<C>>(*out, out, converter); // Throws
}



template<class Conv> std::basic_string<typename Conv::target_char_type>
inline inc_convert(const Conv& conv,
                   std::basic_string<typename Conv::source_char_type> s)
{
    using SourceChar = typename Conv::source_char_type;
    using TargetChar = typename Conv::target_char_type;

    const SourceChar* in = s.data();
    const SourceChar* in_end = in + s.size();

    // At least 512 bytes in buffer
    constexpr std::size_t buf_default_size =
        (512 + sizeof (TargetChar) - 1) / sizeof (TargetChar);
    constexpr std::size_t buf_size =
        (buf_default_size < static_cast<std::size_t>(Conv::min_target_buffer_size) ?
         Conv::min_target_buffer_size : buf_default_size);
    TargetChar buffer[buf_size];
    TargetChar* out = buffer;
    TargetChar* out_end = out + buf_size;

    typename Conv::State converter{conv};
    std::basic_string<TargetChar> t;
    for (;;) {
        bool last = converter.conv(in, in_end, out, out_end, true);
        t.append(buffer, out - buffer);
        if (last)
            return t;
        out = buffer;
    }
}


template<class Enc, class Dec> std::string
IncConvCodec<Enc, Dec>::encode(const StringType& s) const
{
    try {
        return inc_convert(enc, s);
    }
    catch(IncConvException& e) {
        throw core::EncodeException(e.what());
    }
}


template<class Enc, class Dec> typename IncConvCodec<Enc, Dec>::StringType
IncConvCodec<Enc, Dec>::decode(const std::string& s) const
{
    try {
        return inc_convert(dec, s);
    }
    catch(IncConvException& e) {
        throw core::DecodeException(e.what());
    }
}


template<class Enc, class Dec> std::unique_ptr<typename IncConvCodec<Enc, Dec>::OutputStreamType>
IncConvCodec<Enc, Dec>::get_enc_out_stream(core::OutputStream& out) const
{
    return make_inc_conv_out_stream(enc, out); // Throws
}

template<class Enc, class Dec> std::unique_ptr<typename IncConvCodec<Enc, Dec>::InputStreamType>
IncConvCodec<Enc, Dec>::get_dec_in_stream(core::InputStream& in) const
{
    return make_inc_conv_in_stream(dec, in); // Throws
}

template<class Enc, class Dec> std::unique_ptr<core::InputStream> IncConvCodec<Enc, Dec>::
get_enc_in_stream(InputStreamType& in) const
{
    return make_inc_conv_in_stream(enc, in); // Throws
}

template<class Enc, class Dec> std::unique_ptr<core::OutputStream> IncConvCodec<Enc, Dec>::
get_dec_out_stream(OutputStreamType& out) const
{
    return make_inc_conv_out_stream(dec, out); // Throws
}


template<class Enc, class Dec> std::unique_ptr<typename IncConvCodec<Enc, Dec>::OutputStreamType>
IncConvCodec<Enc, Dec>::get_enc_out_stream(const std::shared_ptr<core::OutputStream>& out) const
{
    return make_inc_conv_out_stream(enc, out); // Throws
}

template<class Enc, class Dec> std::unique_ptr<typename IncConvCodec<Enc, Dec>::InputStreamType>
IncConvCodec<Enc, Dec>::get_dec_in_stream(const std::shared_ptr<core::InputStream>& in) const
{
    return make_inc_conv_in_stream(dec, in); // Throws
}

template<class Enc, class Dec> std::unique_ptr<core::InputStream>
IncConvCodec<Enc, Dec>::get_enc_in_stream(const std::shared_ptr<InputStreamType>& in) const
{
    return make_inc_conv_in_stream(enc, in); // Throws
}

template<class Enc, class Dec> std::unique_ptr<core::OutputStream>
IncConvCodec<Enc, Dec>::get_dec_out_stream(const std::shared_ptr<OutputStreamType>& out) const
{
    return make_inc_conv_out_stream(dec, out); // Throws
}

} // namespace util
} // namespace archon

#endif // ARCHON_UTIL_INC_CONV_HPP
