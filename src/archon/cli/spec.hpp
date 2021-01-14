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

#ifndef ARCHON_X_CLI_X_SPEC_HPP
#define ARCHON_X_CLI_X_SPEC_HPP

/// \file


#include <memory>
#include <utility>
#include <stdexcept>
#include <initializer_list>
#include <string_view>
#include <vector>
#include <ostream>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/type_traits.hpp>
#include <archon/base/buffer.hpp>
#include <archon/base/char_mapper.hpp>
#include <archon/cli/option_attributes.hpp>
#include <archon/cli/option_actions.hpp>
#include <archon/cli/command_line.hpp>
#include <archon/cli/detail/option_action.hpp>
#include <archon/cli/detail/spec.hpp>
#include <archon/cli/detail/spec_parser.hpp>


namespace archon::cli {


struct SpecConfig {
    /// FIXME: Explain            
    bool allow_cross_pattern_ambiguity = false;

    /// FIXME: Explain            
    bool allow_internal_pattern_ambiguity = false;            
};


/// \brief 
///
/// 
///
template<class C, class T = std::char_traits<C>> class BasicSpec {
public:
    using char_type        = C;
    using traits_type      = T;
    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;

    BasicSpec(const std::locale& = std::locale());

    /// \{
    ///
    /// \brief Add command line options.
    ///
    /// FIXME: Describe valid forms:                                          
    ///   Short: `-x` where `x` is a single character other than `-`.
    ///   Long:  `--xxx` where `xxx` is a sequence of zero or more characters.
    ///
    /// FIXME: Explain what forms are allowed in \a forms.
    ///
    /// FIXME: Explain what forms \a arg can take.
    ///
    /// FIXME: Explain variable substitution scheme for `descr`:
    /// `N` -> arg name (only valid with opt_arg_always() and opt_arg_maybe()).              
    /// `V` -> original value of associated variable (only valid with \ref raise_flag() and \ref assign()).
    /// `A` -> value assigned by default (default argument) (only valid with \ref raise_flag() and \ref assign()).
    ///
    /// FIXME: Explain what forms actions can take.
    ///
    /// FIXME: Explain support for `std::optional<>` in option argument value types. If a variable of type `std::optional<T>` is passed to \ref assign() or a function taking an argument of type `std::optional<T>` is passed as the option action, then any option argument specified on the command line will be passed as the type `T`. In those cases, if the option is specified without argument, the value assigned by \ref assign() or passed to the specified function will be `std::optional<T>()`.
    ///
    /// \sa \ref opt().      
    ///
    void add_option(const char* forms, const char* arg, int attr, const char* descr);
    template<class A>
    void add_option(const char* forms, const char* arg, int attr, const char* descr, A&& action);
    void add_option(string_view_type forms, string_view_type arg, int attr,
                    string_view_type descr);
    template<class A>
    void add_option(string_view_type forms, string_view_type arg, int attr,
                    string_view_type descr, A&& action);
    /// \}

    /// \brief Print out help text.
    ///
    /// This function prints out text that describes the command line interface.
    ///
    /// Patterns will be shown in the order that they are specified (using
    /// \ref add_pattern()).
    ///
    /// Options will be shown in the order that they are specified (using \ref
    /// add_option()).
    ///
    void show_help(const CommandLine&, ostream_type&, long width) const;

private:
    using OptionForm = typename detail::Spec<C, T>::OptionForm;
    using ArgSpec    = typename detail::Spec<C, T>::ArgSpec;

    using option_action_type     = detail::OptionAction<C, T>;
    using string_chunk_type      = base::Buffer<char_type>;
    using option_form_chunk_type = base::Buffer<OptionForm>;

    base::BasicStringWidener<C, T> m_widener;
    std::vector<string_chunk_type> m_string_chunks;
    std::vector<option_form_chunk_type> m_option_form_chunks;
    detail::SpecParser<C, T> m_spec_parser;
    detail::Spec<C, T> m_rep;

    void do_add_option(const char* forms, const char* arg, int attr, const char* descr,
                       std::unique_ptr<option_action_type>);
    void do_add_option(string_view_type forms, string_view_type arg, int attr,
                       string_view_type descr, std::unique_ptr<option_action_type>);
    void do_add_option_2(string_view_type forms, string_view_type arg, int attr,
                         string_view_type descr, std::unique_ptr<option_action_type>);

    void intern(std::initializer_list<string_view_type*>);
};


using Spec     = BasicSpec<char>;
using WideSpec = BasicSpec<wchar_t>;




/// \{
///
/// \brief Add command line options.
///
/// These functions have the same effect as the corresponding `add_otion()`
/// functions in `BasicSpec` (\ref BasicSpec::add_option()). The advantage of
/// these functions over those in `BasicSpec` is that they increase the amount
/// of space available per line of arguments when code is formatted in the style
/// shown here:
///
/// \code{.cpp}
///
///    archon::cli::Spec spec;
///    opt({"-w", "--width"}, "<num>", archon::cli::no_attributes, spec,
///        "Format text to a line length of @N (default is @V).",
///        archon::cli::assign(width));
///
/// \endcode
///
template<class C, class T>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>&, const char* descr);
template<class C, class T, class A>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>&, const char* descr,
         A&& action);
template<class C, class T>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr, BasicSpec<C, T>&,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr);
template<class C, class T, class A>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr, BasicSpec<C, T>&,
         typename base::Wrap<std::basic_string_view<C, T>>::type descr, A&& action);
/// \}








// Implementation


template<class C, class T> inline BasicSpec<C, T>::BasicSpec(const std::locale& locale) :
    m_widener(locale), // Throws
    m_spec_parser(locale), // Throws
    m_rep(locale)
{
}


template<class C, class T>
void BasicSpec<C, T>::add_option(const char* forms, const char* arg, int attr, const char* descr)
{
    do_add_option(forms, arg, attr, descr, nullptr); // Throws
}


template<class C, class T> template<class A>
void BasicSpec<C, T>::add_option(const char* forms, const char* arg, int attr, const char* descr,
                                 A&& action)
{
    std::unique_ptr<option_action_type> action_2 =
        detail::make_option_action<C, T>(std::forward<A>(action)); // Throws
    do_add_option(forms, arg, attr, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::add_option(string_view_type forms, string_view_type arg, int attr,
                                 string_view_type descr)
{
    do_add_option(forms, arg, attr, descr, nullptr); // Throws
}


template<class C, class T> template<class A>
void BasicSpec<C, T>::add_option(string_view_type forms, string_view_type arg, int attr,
                                 string_view_type descr, A&& action)
{
    std::unique_ptr<option_action_type> action_2 =
        detail::make_option_action<C, T>(std::forward<A>(action)); // Throws
    do_add_option(forms, arg, attr, descr, std::move(action_2)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::show_help(const CommandLine&, ostream_type& out, long width) const
{
    static_cast<void>(width);             
    out << "*CLICK*\n";   
}


template<class C, class T>
inline void BasicSpec<C, T>::do_add_option(const char* forms, const char* arg, int attr,
                                           const char* descr,
                                           std::unique_ptr<option_action_type> action)
{
    string_view_type forms_2, arg_2, descr_2;
    typename base::BasicStringWidener<C, T>::Entry entries[] = {
        { forms, &forms_2 },
        { arg,   &arg_2   },
        { descr, &descr_2 }
    };
    m_widener.widen(entries); // Throws
    do_add_option(forms_2, arg_2, attr, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::do_add_option(string_view_type forms, string_view_type arg, int attr,
                                    string_view_type descr,
                                    std::unique_ptr<option_action_type> action)
{
    string_view_type forms_2 = forms;
    string_view_type arg_2   = arg;
    string_view_type descr_2 = descr;
    intern({ &forms_2, &arg_2, &descr_2 }); // Throws
    do_add_option_2(forms_2, arg_2, attr, descr_2, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::do_add_option_2(string_view_type forms, string_view_type arg, int attr,
                                      string_view_type descr,
                                      std::unique_ptr<option_action_type> action)
{
    base::ArraySeededBuffer<OptionForm, 4> buffer;
    std::size_t ndx = m_rep.get_num_options();
    base::Span<OptionForm> forms_2 =
        m_spec_parser.parse_option_forms(ndx, forms, buffer); // Throws
    base::Buffer<OptionForm> chunk(forms_2); // Throws
    m_option_form_chunks.push_back(std::move(chunk)); // Throws
    ArgSpec arg_2 = m_spec_parser.parse_option_arg(ndx, arg); // Throws
    m_rep.add_option(forms_2, arg_2, attr, descr, std::move(action)); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::intern(std::initializer_list<string_view_type*> strings)
{
    std::size_t total_size = 0;
    for (const string_view_type* string : strings)
        base::int_add(total_size, string->size()); // Throws
    base::Buffer<char_type> chunk(total_size); // Throws
    char_type* data = chunk.data();
    for (string_view_type* string : strings) {
        std::size_t size = string->size();
        char_type* data_2 = std::copy_n(string->data(), size, data);
        *string = { data, size };
        data = data_2;
    }
    m_string_chunks.push_back(std::move(chunk)); // Throws
}


template<class C, class T>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>& spec, const char* descr)
{
    spec.add_option(forms, arg, attr, descr); // Throws
}


template<class C, class T, class A>
void opt(const char* forms, const char* arg, int attr, BasicSpec<C, T>& spec, const char* descr,
         A&& action)
{
    spec.add_option(forms, arg, attr, descr, std::forward<A>(action)); // Throws
}


template<class C, class T>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicSpec<C, T>& spec, typename base::Wrap<std::basic_string_view<C, T>>::type descr)
{
    spec.add_option(forms, arg, attr, descr); // Throws
}


template<class C, class T, class A>
void opt(typename base::Wrap<std::basic_string_view<C, T>>::type forms,
         typename base::Wrap<std::basic_string_view<C, T>>::type arg, int attr,
         BasicSpec<C, T>& spec, typename base::Wrap<std::basic_string_view<C, T>>::type descr,
         A&& action)
{
    spec.add_option(forms, arg, attr, descr, std::forward<A>(action)); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_SPEC_HPP
