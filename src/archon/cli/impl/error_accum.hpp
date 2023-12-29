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

#ifndef ARCHON_X_CLI_X_IMPL_X_ERROR_ACCUM_HPP
#define ARCHON_X_CLI_X_IMPL_X_ERROR_ACCUM_HPP


#include <cstddef>
#include <string_view>
#include <locale>
#include <vector>
#include <ios>

#include <archon/core/buffer_contents.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/format.hpp>
#include <archon/cli/proc_error.hpp>
#include <archon/cli/error_handler.hpp>


namespace archon::cli::impl {


template<class C, class T> class ErrorAccum {
public:
    ErrorAccum(const std::locale&);

    template<class... P> void add_error(std::size_t arg_index, cli::ProcError, const char* message,
                                        const P&... params);

    using error_handler_type = cli::BasicErrorHandler<C, T>;

    using ErrorEntry = typename error_handler_type::ErrorEntry;

    void get_errors(core::BufferContents<ErrorEntry>&) const;

private:
    struct Entry;

    core::BasicSeedMemoryOutputStream<C, T> m_messages_out;
    std::vector<Entry> m_errors;
};








// Implementation


template<class C, class T>
inline ErrorAccum<C, T>::ErrorAccum(const std::locale& locale)
{
    m_messages_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_messages_out.imbue(locale); // Throws
}


template<class C, class T>
struct ErrorAccum<C, T>::Entry {
    std::size_t arg_index;
    cli::ProcError code;
    std::size_t end;
};


template<class C, class T>
template<class... P>
inline void ErrorAccum<C, T>::add_error(std::size_t arg_index, cli::ProcError code, const char* message,
                                        const P&... params)
{
    core::format(m_messages_out, message, params...); // Throws
    std::size_t end = m_messages_out.view().size();
    m_errors.push_back({ arg_index, code, end }); // Throws
}


template<class C, class T>
void ErrorAccum<C, T>::get_errors(core::BufferContents<ErrorEntry>& errors) const
{
    using string_view_type = std::basic_string_view<C, T>;
    string_view_type messages = m_messages_out.view();
    std::size_t offset = 0;
    for (const Entry& entry : m_errors) {
        std::size_t size = std::size_t(entry.end - offset);
        string_view_type message = messages.substr(offset, size);
        errors.push_back({ entry.arg_index, entry.code, message }); // Throws
        offset = entry.end;
    }
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_ERROR_ACCUM_HPP
