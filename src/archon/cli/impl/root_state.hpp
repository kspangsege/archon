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

#ifndef ARCHON_X_CLI_X_IMPL_X_ROOT_STATE_HPP
#define ARCHON_X_CLI_X_IMPL_X_ROOT_STATE_HPP


#include <cstddef>
#include <memory>
#include <utility>
#include <stdexcept>
#include <optional>
#include <string_view>
#include <string>
#include <vector>
#include <locale>

#include <archon/core/features.h>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/string_buffer_contents.hpp>
#include <archon/core/text_codec.hpp>
#include <archon/cli/string_holder.hpp>
#include <archon/cli/error_handler.hpp>
#include <archon/cli/config.hpp>


namespace archon::cli::impl {


template<class C, class T> class RootState {
public:
    using string_view_type   = std::basic_string_view<C, T>;
    using string_type        = std::basic_string<C, T>;
    using string_holder_type = cli::BasicStringHolder<C, T>;
    using error_handler_type = cli::BasicErrorHandler<C, T>;
    using config_type        = cli::BasicConfig<C, T>;

private:
    std::unique_ptr<string_holder_type> m_string_holder;

public:
    core::Slab<string_view_type> args;
    std::locale locale;
    string_holder_type& string_holder;
    std::size_t show_arg_max_size;
    error_handler_type* error_handler;
    std::optional<string_type> argv0_override;

    RootState(const std::locale&, config_type&&);

    void set_args(int argc, const char* const argv[]);

private:
    static auto make_string_holder(config_type&&) -> std::unique_ptr<string_holder_type>;
};








// Implementation


template<class C, class T>
inline RootState<C, T>::RootState(const std::locale& l, config_type&& config)
    : m_string_holder(make_string_holder(std::move(config))) // Throws
    , locale(l)
    , string_holder(m_string_holder ? *m_string_holder : *config.string_holder)
    , show_arg_max_size(config.show_arg_max_size)
    , error_handler(config.error_handler)
{
}


template<class C, class T>
void RootState<C, T>::set_args(int argc, const char* const argv[])
{
    if (ARCHON_UNLIKELY(argc < 1))
        throw std::invalid_argument("Too few arguments (argc < 1)");
    std::size_t num_args = core::int_cast<std::size_t>(argc); // Throws
    args.recreate(num_args); // Throws
    using text_codec_type = core::BasicTextCodec<C, T>;
    if constexpr (text_codec_type::is_degen) {
        //
        // In this case, no decoding is needed. This means that we can copy command-line
        // arguments by reference provided that the user did not specify a custom string
        // holder.
        //
        // If the user did specify a custom string holder, we do have to copy by value,
        // because string view objects, that are passed as argument to pattern or option
        // functions, must remain valid until the custom string holder is destroyed, and
        // that could happen later than the destruction of the command-line arguments as
        // they are passed to this function.
        //
        bool user_specified_string_holder = !m_string_holder;
        if (!user_specified_string_holder) {
            for (int i = 0; i < argc; ++i) {
                std::string_view arg = argv[i];
                args.add(arg);
            }
        }
        else {
            core::Buffer<char> buffer;
            core::StringBufferContents strings(buffer);
            std::vector<std::size_t> ends;
            auto flush = [&] {
                ARCHON_ASSERT(strings.size() > 0);
                std::string_view string = string_holder.add_encoded({ strings.data(), strings.size() }); // Throws
                std::size_t begin = 0;
                for (std::size_t end : ends) {
                    std::size_t size = (end - begin);
                    args.add(string.substr(begin, size));
                    begin = end;
                }
                strings.clear();
                ends.clear();
            };
            std::size_t string_size_soft_limit = 8192;
            for (int i = 0; i < argc; ++i) {
                std::string_view arg = argv[i];
                strings.append(arg); // Throws
                ends.push_back(strings.size()); // Throws
                bool overflow = (strings.size() > string_size_soft_limit);
                if (ARCHON_LIKELY(!overflow))
                    continue;
                flush(); // Throws
            }
            if (ARCHON_LIKELY(!ends.empty()))
                flush(); // Throws
        }
    }
    else {
        //
        // In this case, decoding is needed.
        //
        text_codec_type text_codec(locale); // Throws
        core::Buffer<C> buffer;
        std::size_t buffer_offset = 0;
        std::vector<std::size_t> ends;
        auto flush = [&] {
            ARCHON_ASSERT(buffer_offset > 0);
            string_view_type string = string_holder.add({ buffer.data(), buffer_offset }); // Throws
            std::size_t begin = 0;
            for (std::size_t end : ends) {
                std::size_t size = (end - begin);
                args.add(string.substr(begin, size));
                begin = end;
            }
            buffer_offset = 0;
            ends.clear();
        };
        std::size_t buffer_size_soft_limit = 8192;
        for (int i = 0; i < argc; ++i) {
            std::string_view arg = argv[i];
            text_codec.decode_a(arg, buffer, buffer_offset); // Throws
            ends.push_back(buffer_offset); // Throws
            bool overflow = (buffer_offset > buffer_size_soft_limit);
            if (ARCHON_LIKELY(!overflow))
                continue;
            flush(); // Throws
        }
        if (ARCHON_LIKELY(!ends.empty()))
            flush(); // Throws
    }
    ARCHON_ASSERT(args.size() == num_args);
}


template<class C, class T>
inline auto RootState<C, T>::make_string_holder(config_type&& config) -> std::unique_ptr<string_holder_type>
{
    if (!config.string_holder)
        return std::make_unique<string_holder_type>(); // Throws
    return nullptr;
}


} // namespace archon::cli::impl

#endif // ARCHON_X_CLI_X_IMPL_X_ROOT_STATE_HPP
