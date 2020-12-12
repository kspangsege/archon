// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ARCHON_X_BASE_X_FILE_STREAM_HPP
#define ARCHON_X_BASE_X_FILE_STREAM_HPP

/// \file


#include <type_traits>
#include <optional>
#include <istream>

#include <archon/base/assert.hpp>
#include <archon/base/span.hpp>
#include <archon/base/seed_memory_buffer.hpp>
#include <archon/base/char_codec.hpp>
#include <archon/base/file.hpp>


namespace archon::base {


template<class C, class T> class BasicFileStreambuf;




/// \brief 
///
/// 
///
template<class C, class T = std::char_traits<C>> class BasicFileStream :
        public std::basic_iostream<C, T> {
public:
    struct Config;

    using char_type   = C;
    using traits_type = T;

    BasicFileStream(base::FilesystemPathRef);
    BasicFileStream(base::FilesystemPathRef, Config, bool allow_create = false);
    BasicFileStream(base::File);
    BasicFileStream(base::File, Config);

private:
    base::File m_file;
    BasicFileStreambuf<C, T> m_streambuf;

    static base::File open(base::FilesystemPathRef, bool allow_create);
    static typename BasicFileStreambuf<C, T>::Config upgrade_config(Config);
};


template<class C, class T> struct BasicFileStream<C, T>::Config {
    bool lenient_codec = false;     
    bool stable_eof = false;     
};


using FileStream     = BasicFileStream<char>;
using WideFileStream = BasicFileStream<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicFileStreambuf :
        public std::basic_streambuf<C, T> {
public:
    struct Config;

    using char_type    = C;
    using traits_type  = T;
    using int_type     = typename T::int_type;
    using pos_type     = typename T::pos_type;
    using off_type     = typename T::off_type;

    BasicFileStreambuf(base::File&);
    BasicFileStreambuf(base::File&, Config);

protected:
    void imbue(const std::locale&) override;

    pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode) override;
    pos_type seekpos(pos_type, std::ios_base::openmode) override;

    int_type underflow() override;

private:
    using ext_buffer_type = base::SeedMemoryBuffer<char>;
    using int_buffer_type = base::SeedMemoryBuffer<C>;

    static constexpr std::size_t s_default_ext_buffer_size = 1024;
    static constexpr std::size_t s_default_int_buffer_size = 256;

    std::locale m_locale;
    base::File& m_file;
    ext_buffer_type m_ext_buffer;
    int_buffer_type m_int_buffer;
    std::optional<base::BasicCharCodec<C, T>> m_codec;
    char* m_ext_begin = nullptr;
    char* m_ext_curr  = nullptr;
    char* m_ext_end   = nullptr;
    std::mbstate_t m_codec_state_begin = {};
    std::mbstate_t m_codec_state_curr = {};
    const bool m_lenient_codec;
    const bool m_stable_eof;
    bool m_eof = false;
    bool m_ext_unexhausted = false;

    void ensure_codec();
};


template<class C, class T> struct BasicFileStreambuf<C, T>::Config :
        BasicFileStream<C, T>::Config {
    std::optional<base::Span<char>> ext_buffer;
    std::optional<base::Span<C>> int_buffer;
};


using FileStreambuf     = BasicFileStreambuf<char>;
using WideFileStreambuf = BasicFileStreambuf<wchar_t>;








// Implementation


// ============================ BasicFileStream ============================


template<class C, class T>
inline BasicFileStream<C, T>::BasicFileStream(base::FilesystemPathRef path) :
    BasicFileStream(path, {}, false) // Throws
{
}


template<class C, class T>
inline BasicFileStream<C, T>::BasicFileStream(base::FilesystemPathRef path, Config config,
                                              bool allow_create) :
    BasicFileStream(open(path, allow_create), std::move(config)) // Throws
{
}


template<class C, class T> inline BasicFileStream<C, T>::BasicFileStream(base::File file) :
    BasicFileStream(std::move(file), {}) // Throws
{
}


template<class C, class T>
inline BasicFileStream<C, T>::BasicFileStream(base::File file, Config config) :
    std::basic_iostream<C, T>(nullptr), // Throws
    m_file(std::move(file)),
    m_streambuf(m_file, upgrade_config(config)) // Throws
{
    this->rdbuf(&m_streambuf); // Throws
}


template<class C, class T>
inline base::File BasicFileStream<C, T>::open(base::FilesystemPathRef path, bool allow_create)
{
    base::File::AccessMode access_mode = base::File::AccessMode::read_write;
    base::File::CreateMode create_mode = base::File::CreateMode::never;
    base::File::WriteMode write_mode   = base::File::WriteMode::normal;
    if (allow_create)
        create_mode = base::File::CreateMode::allow;
    base::File file;
    file.open(path, access_mode, create_mode, write_mode); // Throws
    return file;
}


template<class C, class T> inline auto BasicFileStream<C, T>::upgrade_config(Config config) ->
    typename BasicFileStreambuf<C, T>::Config
{
    typename BasicFileStreambuf<C, T>::Config config_2;
    static_cast<Config&>(config_2) = config;
    return config_2;
}


// ============================ BasicFileStreambuf ============================


template<class C, class T> inline BasicFileStreambuf<C, T>::BasicFileStreambuf(base::File& file) :
    BasicFileStreambuf(file, {}) // Throws
{
}


template<class C, class T>
inline BasicFileStreambuf<C, T>::BasicFileStreambuf(base::File& file, Config config) :
    std::basic_streambuf<C, T>(), // Throws
    m_file(file),
    m_lenient_codec(config.lenient_codec),
    m_stable_eof(config.stable_eof)
{
    // While the external buffer will be expanded if necessary, the internal
    // buffer remains at its initial size, which is why we ensure that it is not
    // empty initially.

    if (!config.ext_buffer.has_value()) {
        m_ext_buffer = ext_buffer_type(s_default_ext_buffer_size); // Throws
    }
    else {
        m_ext_buffer = ext_buffer_type(*config.ext_buffer);
    }

    if (!config.int_buffer.has_value()) {
        m_int_buffer = int_buffer_type(s_default_int_buffer_size); // Throws
    }
    else {
        m_int_buffer = int_buffer_type(*config.int_buffer);
        m_int_buffer.reserve(1); // Throws
    }
}


template<class C, class T> void BasicFileStreambuf<C, T>::imbue(const std::locale& locale)
{
    m_codec.reset();
    m_locale = locale;
}


template<class C, class T>
auto BasicFileStreambuf<C, T>::seekoff(off_type off, std::ios_base::seekdir dir,
                                       std::ios_base::openmode mode) -> pos_type
{
    std::cerr << "{**seekoff**}\n";      
    
    return std::basic_streambuf<C, T>::seekoff(off, dir, mode);    
}


template<class C, class T>
auto BasicFileStreambuf<C, T>::seekpos(pos_type pos, std::ios_base::openmode mode) -> pos_type
{
    std::cerr << "{**seekpos**}\n";      
    
    return std::basic_streambuf<C, T>::seekpos(pos, mode);    
}


template<class C, class T> auto BasicFileStreambuf<C, T>::underflow() -> int_type
{
//    std::cerr << "{**CLICK**}\n";      

    ensure_codec(); // Throws

/*
    if (ARCHON_UNLIKELY(m_is_writing))            
        flush();
*/

    if (ARCHON_UNLIKELY(m_eof && m_stable_eof))
        return T::eof();

    m_eof = false;

    if (ARCHON_LIKELY(m_ext_unexhausted)) {
        m_ext_begin = m_ext_curr;
        m_codec_state_begin = m_codec_state_curr;
        goto convert;
    }

    // Read from file
  read:
    {
        // Copy bytes of incomplete character to beginning of external buffer
        m_ext_begin = m_ext_buffer.data();
        m_ext_end   = std::copy(m_ext_curr, m_ext_end, m_ext_begin);
        m_ext_curr  = m_ext_begin;
        m_codec_state_begin = m_codec_state_curr;
        std::size_t ext_used = std::size_t(m_ext_end - m_ext_begin);
        if (ARCHON_UNLIKELY(ext_used == m_ext_buffer.size())) {
            // We need to fit more bytes into external buffer in order to
            // construct the next character.
            m_ext_buffer.reserve_extra(1, ext_used); // Throws
            m_ext_begin = m_ext_buffer.data();
            m_ext_end   = m_ext_begin + ext_used;
            m_ext_curr  = m_ext_begin;
        }
        char* ext_buffer_end = m_ext_buffer.data() + m_ext_buffer.size();
        ARCHON_ASSERT(ext_buffer_end > m_ext_end);
        std::size_t n = m_file.read_some({ m_ext_end, ext_buffer_end }); // Throws
        std::cout << "read(" << (ext_buffer_end - m_ext_end) << ") -> " << n << "\n";             
        if (ARCHON_LIKELY(n > 0)) {
            m_ext_end += n;
            m_ext_unexhausted = true;
        }
        else {
            m_eof = true;
        }
    }

    // Consider push back                                             

    // Convert bytes -> characters
  convert:
    {
        // FIXME: consider always_noconv mode         
        ARCHON_ASSERT(m_ext_begin == m_ext_curr);
        const char*& ext_curr = const_cast<const char*&>(m_ext_curr);
        C* int_base  = m_int_buffer.data();
        C* int_begin = int_base;
        C* int_end   = int_begin + m_int_buffer.size();
        bool flush = (m_eof && m_stable_eof);
        std::codecvt_base::result result =
            m_codec->inc_decode(m_codec_state_curr, ext_curr, m_ext_end,
                                int_begin, int_end, flush); // Throws
        switch (result) {
            case std::codecvt_base::noconv: {
                std::cout << "-----------------------------> noconv\n";               
                constexpr bool regular_char = std::is_same_v<C, char>;
                if constexpr (regular_char) {
                    ARCHON_ASSERT(m_ext_curr == m_ext_begin);
                    ARCHON_ASSERT(int_begin == int_base);
                    m_ext_unexhausted = false;
                    // FIXME: Consider case where m_ext_begin == m_ext_end                   
                    this->setg(m_ext_begin, m_ext_begin, m_ext_end);
                    ARCHON_ASSERT(m_ext_begin < m_ext_end);
                    return T::to_int_type(*m_ext_begin);
                }
                else {
                    ARCHON_ASSERT_UNREACHABLE;
                    return T::eof();
                }
            }
            case std::codecvt_base::ok: {
                std::cout << "-----------------------------> ok\n";               
                // More input is needed (The decoder has consumed as much as it can from the input buffer (which may be less than everything) and providing more output buffer space would not help. In order to produce another character, or detect an error, more input will have to be provided).            
                m_ext_unexhausted = false;
                bool something_was_produced = (int_begin != int_base);
                if (ARCHON_LIKELY(something_was_produced))
                    goto done;
                if (!m_eof)
                    goto read;
                return T::eof();
            }
            case std::codecvt_base::partial:
                std::cout << "-----------------------------> partial\n";               
                // Insufficient space in output buffer (The output buffer is full, and providing more space might allow for additional characters to be produced with, or without the already provided input).                 
                ARCHON_ASSERT(int_begin == int_end);
                goto done;
            case std::codecvt_base::error: {
                std::cout << "-----------------------------> error\n";               
                bool something_was_produced = (int_begin != int_base);
                if (ARCHON_LIKELY(something_was_produced))
                    goto done;
                return T::eof();
            }
        }
        ARCHON_ASSERT_UNREACHABLE;
        return T::eof();

      done:
        this->setg(int_base, int_base, int_begin);
        ARCHON_ASSERT(int_base < int_begin);
        return T::to_int_type(*int_base);
    }
}


template<class C, class T> inline void BasicFileStreambuf<C, T>::ensure_codec()
{
    if (ARCHON_LIKELY(m_codec.has_value()))
        return;
    m_codec.emplace(m_locale, m_lenient_codec); // Throws
    // FIXME: If not always_noconv, don't
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_FILE_STREAM_HPP
