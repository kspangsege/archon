// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_CORE_X_CHAR_MAPPER_HPP
#define ARCHON_X_CORE_X_CHAR_MAPPER_HPP

/// \file


#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <array>
#include <string_view>
#include <string>
#include <stdexcept>
#include <locale>
#include <ios>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>


namespace archon::core {


namespace impl {
template<class C> class WidenBufferBase;
template<class C> class NarrowBufferBase;
template<class C, std::size_t N> class WidenBufferArraySeed;
template<class C, std::size_t N> class NarrowBufferArraySeed;
template<class C, class T> class CharMapperBase;
} // namespace impl




/// \brief Locale dependent character mapping.
///
/// This class provides services based on those provided by `std::ctype<C>`. In general, an
/// instance of this class encapsulates a locale, and its associated `std::ctype<C>`
/// facet. However, in the interest of efficiency, on some platforms, and with some
/// character types, it may store less, because less is needed. For instance, if `C` is
/// `char`, it may not store anything at all (it may be an empty class).
///
template<class C, class T = std::char_traits<C>> class BasicCharMapper
    : private impl::CharMapperBase<C, T> {
public:
    class WidenBuffer;
    class NarrowBuffer;
    template<std::size_t N> class ArraySeededWidenBuffer;
    template<std::size_t N> class ArraySeededNarrowBuffer;
    struct WidenEntry;

    using char_type = C;
    using traits_type = T;
    using string_view_type = std::basic_string_view<C, T>;

    /// \brief Construct mapper from global locale.
    ///
    /// Construct a character type mapper from the global locale.
    ///
    BasicCharMapper();

    /// \brief Construct mapper from specified locale facet.
    ///
    /// Construct a character type mapper from the specified locale facet. The application
    /// must ensure that the facet outlasts the mapper.
    ///
    explicit BasicCharMapper(const std::ctype<C>&) noexcept;

    /// \brief Construct mapper from specified locale.
    ///
    /// Construct a character type mapper from the specified locale. The locale is copied as
    /// needed, so the specified locale object does not need to outlast the mapper.
    ///
    explicit BasicCharMapper(const std::locale&);

    /// \brief Construct mapper from stream locale.
    ///
    /// Construct a character type mapper from the locale with which the specified stream is
    /// imbued. The specified stream object does not need to outlast the mapper.
    ///
    explicit BasicCharMapper(const std::ios_base&);

    ~BasicCharMapper() noexcept = default;

    /// \brief True iff mappings are trivial.
    ///
    /// This variable is `true` if, and only if character mapping is trivial for the
    /// specified character type `C`. Trivial character mapping means that both the widening
    /// and the narrowing operations are identity transformations.
    ///
    static constexpr bool is_trivial = impl::CharMapperBase<C, T>::is_trivial;

    /// \brief Widen the specified character.
    ///
    /// Given a character from the basic source character set, widen it as if by
    /// `std::ctype<C>::widen(ch)`.
    ///
    /// On some platforms, and when `C` is `char`, this function reduces to simply returning
    /// the specified character.
    ///
    auto widen(char ch) const -> C;

    /// \brief Place widened string in specified destination buffer.
    ///
    /// Given a string of character from the basic source character set, produce a version
    /// of the string in \p dest where those characters are individually widened as if by
    /// `std::ctype<C>::widen(ch)`.
    ///
    /// Clearly, \p dest must point to a chunk of memory with at least as many wide
    /// characters as the size of the specified string.
    ///
    /// \sa widen(std::string_view, WidenBuffer&)
    ///
    void widen(std::string_view string, C* dest) const;

    /// \brief Widen the specified string.
    ///
    /// Given a string of characters from the basic source character set, produce a version
    /// of the string where those characters are individually widened as if by
    /// `std::ctype<C>::widen(ch)`.
    ///
    /// On some platforms, and when `C` is `char`, this function reduces to returning a
    /// string view that refers to the same memory as the specified string.
    ///
    auto widen(std::string_view string, WidenBuffer&) const -> string_view_type;

    /// \brief Widen the specified string.
    ///
    /// This function has the same effect as \ref widen(std::string_view, WidenBuffer&),
    /// except that is will use the specified seed memory as widening buffer when the size
    /// of the specified string is small enough, i.e., no greater than the seed memory size.
    ///
    /// If the seed memory is allocated on the stack, this function can be used to avoid
    /// dynamic allocation in the general case, provided that the seed memory is sized
    /// appropriately.
    ///
    auto widen(std::string_view string, WidenBuffer&, core::Span<C> seed_memory) const -> string_view_type;

    /// \brief Widen the specified set of strings.
    ///
    /// This function widens multiple strings at once using a single widening buffer. Each
    /// entry specifies a string that will be widened as if by \ref widen(std::string_view,
    /// WidenBuffer&).
    ///
    void widen(core::Span<WidenEntry>, WidenBuffer&) const;

    /// \brief Narrow the specified character.
    ///
    /// Given a character from the basic source character set, narrow it as if by
    /// `std::ctype<C>::narrow(ch, replacement)`.
    ///
    /// On some platforms, and when `C` is `char`, this function reduces to simply returning
    /// the specified character.
    ///
    char narrow(C ch, char replacement) const;

    /// \brief Place narrowed string in specified destination buffer.
    ///
    /// Given a string of character from the basic source character set, produce a version
    /// of the string in \p dest where those characters are individually narrowed as if by
    /// `std::ctype<C>::narrow(ch, replacement)`.
    ///
    /// Clearly, \p dest must point to a chunk of memory with at least as many characters as
    /// the size of the specified string.
    ///
    /// \sa narrow(string_view_type, char, NarrowBuffer&)
    ///
    void narrow(string_view_type string, char replacement, char* dest) const;

    /// \brief Narrow the specified string.
    ///
    /// Given a string of character from the basic source character set, produce a version
    /// of the string where those characters are individually narrowed as if by
    /// `std::ctype<C>::narrow(ch, replacement)`.
    ///
    /// On some platforms, and when `C` is `char`, this function reduces to returning a
    /// string view that refers to the same memory as the specified string.
    ///
    auto narrow(string_view_type string, char replacement, NarrowBuffer&) const -> std::string_view;
};


template<class C, class T> explicit BasicCharMapper(std::basic_ios<C, T>&) -> BasicCharMapper<C, T>;


using CharMapper     = BasicCharMapper<char>;
using WideCharMapper = BasicCharMapper<wchar_t>;




template<class C, class T> class BasicCharMapper<C, T>::WidenBuffer
    : public impl::WidenBufferBase<C> {
public:
    explicit WidenBuffer(core::Span<C> seed_memory = {}) noexcept;
    ~WidenBuffer() noexcept = default;
};




template<class C, class T> class BasicCharMapper<C, T>::NarrowBuffer
    : public impl::NarrowBufferBase<C> {
public:
    explicit NarrowBuffer(core::Span<char> seed_memory = {}) noexcept;
    ~NarrowBuffer() noexcept = default;
};




template<class C, class T>
template<std::size_t N> class BasicCharMapper<C, T>::ArraySeededWidenBuffer
    : private impl::WidenBufferArraySeed<C, N>
    , public WidenBuffer {
public:
    ArraySeededWidenBuffer() noexcept;
    ~ArraySeededWidenBuffer() noexcept = default;
};




template<class C, class T>
template<std::size_t N> class BasicCharMapper<C, T>::ArraySeededNarrowBuffer
    : private impl::NarrowBufferArraySeed<C, N>
    , public NarrowBuffer {
public:
    ArraySeededNarrowBuffer() noexcept;
    ~ArraySeededNarrowBuffer() noexcept = default;
};




template<class C, class T>
struct BasicCharMapper<C, T>::WidenEntry {
    std::string_view string;
    string_view_type* result;
};




template<class C, class T = std::char_traits<C>> class BasicStringWidener
    : private BasicCharMapper<C, T>
    , private BasicCharMapper<C, T>::WidenBuffer {
public:
    using string_view_type = typename BasicCharMapper<C, T>::string_view_type;
    using Entry = typename BasicCharMapper<C, T>::WidenEntry;

    explicit BasicStringWidener(const std::locale&, core::Span<C> seed_memory = {});
    ~BasicStringWidener() noexcept = default;

    auto widen(std::string_view string) -> string_view_type;

    auto widen(std::string_view string, core::Span<C> seed_memory) -> string_view_type;

    void widen(core::Span<Entry>);
};


template<class C, std::size_t N> explicit BasicStringWidener(const std::locale&, C(&)[N]) -> BasicStringWidener<C>;
template<class C, std::size_t N> explicit BasicStringWidener(const std::locale&, std::array<C, N>&) ->
    BasicStringWidener<C>;


using StringWidener     = BasicStringWidener<char>;
using WideStringWidener = BasicStringWidener<wchar_t>;




template<class C, class T = std::char_traits<C>> class BasicStringNarrower
    : private BasicCharMapper<C, T>
    , private BasicCharMapper<C, T>::NarrowBuffer {
public:
    using string_view_type = typename BasicCharMapper<C, T>::string_view_type;

    explicit BasicStringNarrower(const std::locale&, core::Span<char> seed_memory = {});
    ~BasicStringNarrower() noexcept = default;

    auto narrow(string_view_type string, char replacement) -> std::string_view;
};


using StringNarrower     = BasicStringNarrower<char>;
using WideStringNarrower = BasicStringNarrower<wchar_t>;








// Implementation


// ============================ BasicCharMapper ============================


namespace impl {


template<class C> class WidenBufferBase
    : public core::Buffer<C> {
public:
    WidenBufferBase(core::Span<C> seed_memory) noexcept
        : core::Buffer<C>(seed_memory)
    {
    }
};


template<> class WidenBufferBase<char> {
public:
    WidenBufferBase(core::Span<char>) noexcept
    {
    }
};


template<class C> class NarrowBufferBase
    : public core::Buffer<char> {
public:
    NarrowBufferBase(core::Span<char> seed_memory) noexcept
        : core::Buffer<char>(seed_memory)
    {
    }
};


template<> class NarrowBufferBase<char> {
public:
    NarrowBufferBase(core::Span<char>) noexcept
    {
    }
};


template<class C, std::size_t N> class WidenBufferArraySeed
    : private std::array<C, N> {
public:
    auto seed_span() noexcept -> core::Span<C>
    {
        return static_cast<std::array<C, N>&>(*this);
    }
};


template<std::size_t N> class WidenBufferArraySeed<char, N> {
public:
    auto seed_span() noexcept -> core::Span<char>
    {
        return {};
    }
};


template<class C, std::size_t N> class NarrowBufferArraySeed
    : private std::array<char, N> {
public:
    auto seed_span() noexcept -> core::Span<char>
    {
        return static_cast<std::array<char, N>&>(*this);
    }
};


template<std::size_t N> class NarrowBufferArraySeed<char, N> {
public:
    auto seed_span() noexcept -> core::Span<char>
    {
        return {};
    }
};


template<class C, class T> class CharMapperBase {
public:
    using string_view_type = std::basic_string_view<C, T>;
    static constexpr bool is_trivial = false;
    CharMapperBase()
        : m_locale() // I.e., copy of global locale
        , m_ctype(std::use_facet<std::ctype<C>>(m_locale)) // Throws
    {
    }
    CharMapperBase(const std::ctype<C>& ctype) noexcept
        : m_locale() // Unused in this case
        , m_ctype(ctype)
    {
    }
    CharMapperBase(const std::locale& locale)
        : m_locale(locale)
        , m_ctype(std::use_facet<std::ctype<C>>(m_locale)) // Throws
    {
    }
    CharMapperBase(const std::ios_base& ios)
        : m_locale(ios.getloc())
        , m_ctype(std::use_facet<std::ctype<C>>(m_locale)) // Throws
    {
    }
    auto widen(char ch) const -> C
    {
        return m_ctype.widen(ch); // Throws
    }
    void widen(std::string_view string, C* dest) const
    {
        const char* data = string.data();
        std::size_t size = string.size();
        m_ctype.widen(data, data + size, dest); // Throws
    }
    auto widen(std::string_view string, impl::WidenBufferBase<C>& buffer) const -> string_view_type
    {
        const char* data = string.data();
        std::size_t size = string.size();
        buffer.reserve(size); // Throws
        C* dest = buffer.data();
        m_ctype.widen(data, data + size, dest); // Throws
        return { dest, size };
    }
    auto widen(std::string_view string, impl::WidenBufferBase<C>& buffer,
               core::Span<C> seed_memory) const -> string_view_type
    {
        if (ARCHON_LIKELY(string.size() <= seed_memory.size())) {
            C* dest = seed_memory.data();
            widen(string, dest); // Throws
            return { dest, string.size() };
        }
        return widen(string, buffer); // Throws
    }
    template<class E> void widen(core::Span<E> entries, impl::WidenBufferBase<C>& buffer) const
    {
        std::size_t offset = 0;
        for (E& entry : entries) {
            const char* data = entry.string.data();
            std::size_t size = entry.string.size();
            buffer.reserve_extra(size, offset); // Throws
            m_ctype.widen(data, data + size, buffer.data() + offset); // Throws
            offset += size;
        }
        offset = 0;
        for (E& entry : entries) {
            std::size_t size = entry.string.size();
            *entry.result = { buffer.data() + offset, size };
            offset += size;
        }
    }
    char narrow(C ch, char replacement) const
    {
        return m_ctype.narrow(ch, replacement); // Throws
    }
    void narrow(string_view_type string, char replacement, char* dest) const
    {
        const C* data = string.data();
        std::size_t size = string.size();
        m_ctype.narrow(data, data + size, replacement, dest); // Throws
    }
    auto narrow(string_view_type string, char replacement, impl::NarrowBufferBase<C>& buffer) const -> std::string_view
    {
        const C* data = string.data();
        std::size_t size = string.size();
        buffer.reserve(size); // Throws
        char* dest = buffer.data();
        m_ctype.narrow(data, data + size, replacement, dest); // Throws
        return { dest, size };
    }
private:
    const std::locale m_locale;
    const std::ctype<C>& m_ctype;
};


template<> class CharMapperBase<char, std::char_traits<char>> {
public:
    using string_view_type = std::string_view;
    static constexpr bool is_trivial = true;
    CharMapperBase() noexcept = default;
    CharMapperBase(const std::ctype<char>&) noexcept
    {
    }
    CharMapperBase(const std::locale&) noexcept
    {
    }
    CharMapperBase(const std::ios_base&) noexcept
    {
    }
    static char widen(char ch) noexcept
    {
        return ch;
    }
    static void widen(std::string_view string, char* dest) noexcept
    {
        std::copy_n(string.data(), string.size(), dest);
    }
    static auto widen(std::string_view string, impl::WidenBufferBase<char>&) noexcept -> string_view_type
    {
        return string;
    }
    static auto widen(std::string_view string, impl::WidenBufferBase<char>&,
                      core::Span<char>) noexcept -> string_view_type
    {
        return string;
    }
    template<class E> static void widen(core::Span<E> entries, impl::WidenBufferBase<char>&) noexcept
    {
        for (E& entry : entries)
            *entry.result = entry.string;
    }
    static char narrow(char ch, char) noexcept
    {
        return ch;
    }
    static void narrow(string_view_type string, char, char* dest) noexcept
    {
        std::copy_n(string.data(), string.size(), dest);
    }
    static auto narrow(string_view_type string, char, impl::NarrowBufferBase<char>&) noexcept -> std::string_view
    {
        return string;
    }
};


#if ARCHON_WCHAR_IS_UNICODE


template<class T> class CharMapperBase<wchar_t, T> {
public:
    using string_view_type = std::basic_string_view<wchar_t, T>;
    static constexpr bool is_trivial = false;
    CharMapperBase()
        : m_ctype(std::use_facet<std::ctype<wchar_t>>(std::locale::classic())) // Throws
    {
    }
    CharMapperBase(const std::ctype<wchar_t>& ctype) noexcept
        : m_ctype(ctype) // Throws
    {
    }
    CharMapperBase(const std::locale&)
        : CharMapperBase() // Throws
    {
    }
    CharMapperBase(const std::ios_base&)
        : CharMapperBase() // Throws
    {
    }
    wchar_t widen(char ch) const
    {
        return m_ctype.widen(ch); // Throws
    }
    void widen(std::string_view string, wchar_t* dest) const
    {
        const char* data = string.data();
        std::size_t size = string.size();
        m_ctype.widen(data, data + size, dest); // Throws
    }
    auto widen(std::string_view string, impl::WidenBufferBase<wchar_t>& buffer) const -> string_view_type
    {
        const char* data = string.data();
        std::size_t size = string.size();
        buffer.reserve(size); // Throws
        wchar_t* dest = buffer.data();
        m_ctype.widen(data, data + size, dest); // Throws
        return { dest, size };
    }
    auto widen(std::string_view string, impl::WidenBufferBase<wchar_t>& buffer,
               core::Span<wchar_t> seed_memory) const -> string_view_type
    {
        if (ARCHON_LIKELY(string.size() <= seed_memory.size())) {
            wchar_t* dest = seed_memory.data();
            widen(string, dest); // Throws
            return { dest, string.size() };
        }
        return widen(string, buffer); // Throws
    }
    template<class E> void widen(core::Span<E> entries, impl::WidenBufferBase<wchar_t>& buffer) const
    {
        std::size_t offset = 0;
        for (E& entry : entries) {
            const char* data = entry.string.data();
            std::size_t size = entry.string.size();
            buffer.reserve_extra(size, offset); // Throws
            m_ctype.widen(data, data + size, buffer.data() + offset); // Throws
            offset += size;
        }
        offset = 0;
        for (E& entry : entries) {
            std::size_t size = entry.string.size();
            *entry.result = { buffer.data() + offset, size };
            offset += size;
        }
    }
    char narrow(wchar_t ch, char replacement) const
    {
        return m_ctype.narrow(ch, replacement); // Throws
    }
    void narrow(string_view_type string, char replacement, char* dest) const
    {
        const wchar_t* data = string.data();
        std::size_t size = string.size();
        m_ctype.narrow(data, data + size, replacement, dest); // Throws
    }
    auto narrow(string_view_type string, char replacement,
                impl::NarrowBufferBase<wchar_t>& buffer) const -> std::string_view
    {
        const wchar_t* data = string.data();
        std::size_t size = string.size();
        buffer.reserve(size); // Throws
        char* dest = buffer.data();
        m_ctype.narrow(data, data + size, replacement, dest); // Throws
        return { dest, size };
    }
private:
    const std::ctype<wchar_t>& m_ctype;
};


#endif // ARCHON_WCHAR_IS_UNICODE


} // namespace impl


template<class C, class T>
inline BasicCharMapper<C, T>::BasicCharMapper()
    : impl::CharMapperBase<C, T>() // Throws
{
}


template<class C, class T>
inline BasicCharMapper<C, T>::BasicCharMapper(const std::ctype<C>& ctype) noexcept
    : impl::CharMapperBase<C, T>(ctype)
{
}


template<class C, class T>
inline BasicCharMapper<C, T>::BasicCharMapper(const std::locale& locale)
    : impl::CharMapperBase<C, T>(locale) // Throws
{
}


template<class C, class T>
inline BasicCharMapper<C, T>::BasicCharMapper(const std::ios_base& ios)
    : impl::CharMapperBase<C, T>(ios) // Throws
{
}


template<class C, class T>
inline auto BasicCharMapper<C, T>::widen(char ch) const -> C
{
    return impl::CharMapperBase<C, T>::widen(ch); // Throws
}


template<class C, class T>
inline void BasicCharMapper<C, T>::widen(std::string_view string, C* dest) const
{
    impl::CharMapperBase<C, T>::widen(string, dest); // Throws
}


template<class C, class T>
inline auto BasicCharMapper<C, T>::widen(std::string_view string, WidenBuffer& buffer) const -> string_view_type
{
    return impl::CharMapperBase<C, T>::widen(string, buffer); // Throws
}


template<class C, class T>
inline auto BasicCharMapper<C, T>::widen(std::string_view string, WidenBuffer& buffer,
                                         core::Span<C> seed_memory) const -> string_view_type
{
    return impl::CharMapperBase<C, T>::widen(string, buffer, seed_memory); // Throws
}


template<class C, class T>
inline void BasicCharMapper<C, T>::widen(core::Span<WidenEntry> entries, WidenBuffer& buffer) const
{
    impl::CharMapperBase<C, T>::widen(entries, buffer); // Throws
}


template<class C, class T>
inline char BasicCharMapper<C, T>::narrow(C ch, char replacement) const
{
    return impl::CharMapperBase<C, T>::narrow(ch, replacement); // Throws
}


template<class C, class T>
inline void BasicCharMapper<C, T>::narrow(string_view_type string, char replacement, char* dest) const
{
    impl::CharMapperBase<C, T>::narrow(string, replacement, dest); // Throws
}


template<class C, class T>
inline auto BasicCharMapper<C, T>::narrow(string_view_type string, char replacement, NarrowBuffer& buffer) const ->
    std::string_view
{
    return impl::CharMapperBase<C, T>::narrow(string, replacement, buffer); // Throws
}


// ============================ BasicCharMapper::WidenBuffer ============================


template<class C, class T>
inline BasicCharMapper<C, T>::WidenBuffer::WidenBuffer(core::Span<C> seed_memory) noexcept
    : impl::WidenBufferBase<C>(seed_memory)
{
}


// ============================ BasicCharMapper::NarrowBuffer ============================


template<class C, class T>
inline BasicCharMapper<C, T>::NarrowBuffer::NarrowBuffer(core::Span<char> seed_memory) noexcept
    : impl::NarrowBufferBase<C>(seed_memory)
{
}


// ============================ BasicCharMapper::ArraySeededWidenBuffer ============================


template<class C, class T>
template<std::size_t N> inline BasicCharMapper<C, T>::ArraySeededWidenBuffer<N>::ArraySeededWidenBuffer() noexcept
    : WidenBuffer(this->seed_span())
{
}


// ============================ BasicCharMapper::ArraySeededNarrowBuffer ============================


template<class C, class T>
template<std::size_t N> inline BasicCharMapper<C, T>::ArraySeededNarrowBuffer<N>::ArraySeededNarrowBuffer() noexcept
    : NarrowBuffer(this->seed_span())
{
}


// ============================ BasicStringWidener ============================


template<class C, class T>
inline BasicStringWidener<C, T>::BasicStringWidener(const std::locale& locale, core::Span<C> seed_memory)
    : BasicCharMapper<C, T>(locale) // Throws
    , BasicCharMapper<C, T>::WidenBuffer(seed_memory)
{
}


template<class C, class T>
inline auto BasicStringWidener<C, T>::widen(std::string_view string) -> string_view_type
{
    return BasicCharMapper<C, T>::widen(string, *this); // Throws
}


template<class C, class T>
inline auto BasicStringWidener<C, T>::widen(std::string_view string, core::Span<C> seed_memory) -> string_view_type
{
    return BasicCharMapper<C, T>::widen(string, *this, seed_memory); // Throws
}


template<class C, class T>
inline void BasicStringWidener<C, T>::widen(core::Span<Entry> entries)
{
    BasicCharMapper<C, T>::widen(entries, *this); // Throws
}


// ============================ BasicStringNarrower ============================


template<class C, class T>
inline BasicStringNarrower<C, T>::BasicStringNarrower(const std::locale& locale,
                                                      core::Span<char> seed_memory)
    : BasicCharMapper<C, T>(locale) // Throws
    , BasicCharMapper<C, T>::NarrowBuffer(seed_memory)
{
}


template<class C, class T>
inline auto BasicStringNarrower<C, T>::narrow(string_view_type string, char replacement) -> std::string_view
{
    return BasicCharMapper<C, T>::narrow(string, replacement, *this); // Throws
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_CHAR_MAPPER_HPP
