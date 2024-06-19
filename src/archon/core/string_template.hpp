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

#ifndef ARCHON_X_CORE_X_STRING_TEMPLATE_HPP
#define ARCHON_X_CORE_X_STRING_TEMPLATE_HPP

/// \file


#include <cstddef>
#include <utility>
#include <functional>
#include <string_view>
#include <array>
#include <tuple>
#include <map>
#include <locale>
#include <ostream>
#include <stdexcept>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/utility.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/string.hpp>
#include <archon/core/string_codec.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/stream_output.hpp>
#include <archon/core/string_formatter.hpp>


namespace archon::core {


namespace impl {
template<class S, class... A> struct ExpandStringTemplate;
} // namespace impl




/// \brief A parameterized string template.
///
/// This class offers a mechanism for substituting values into a string template, and in a
/// way that makes it efficient to do this repeatedly for the same template with varying
/// values.
///
/// A template object is constructed from a string containing parameter references such as
/// `"@{address}:@{port}"`.
///
/// A parameter reference generally has the form `@{<name>}`, where `<name>` is the
/// parameter name. For instance, if the parameter name is `foo`, then `@{foo}` is a
/// reference to that parameter. If the parameter name consists of a single character, then
/// a shorter form, `@<name>`, is available. I.e., `@x` is a reference to the parameter
/// named `x`. As a special rule, `@@` is substituted by `@`.
///
/// Example without context:
///
/// \code{.cpp}
///
///   int x = 7;
///   StringTemplate<>::Parameters params;
///   params["x"] = &x;
///   StringTemplate<> templ("<@x>", params);
///   std::cout << expand(templ) << "\n";
///   x = 8;
///   std::cout << expand(templ) << "\n";
///
/// \endcode
///
/// This should produce the following output:
///
///     <7>
///     <8>
///
/// Example with context:
///
/// \code{.cpp}
///
///   int x = 7;
///   struct Context {
///       int y;
///   };
///   using Template = StringTemplate<Context>;
///   Template::Parameters params;
///   params["x"] = &x;
///   params["y"] = &Context::y;
///   params["z"] = [](std::ostream& out, const Context& ctx) {
///       out << (ctx.y + 1);
///   };
///   Template templ("<@x|@y|@z>", params);
///
///   Context ctx_1, ctx_2;
///   ctx_1.y = 4;
///   ctx_2.y = 5;
///   std::cout << expand(templ, ctx_1) << "\n";
///   x = 8;
///   std::cout << expand(templ, ctx_2) << "\n";
///
/// \endcode
///
/// This should produce the following output:
///
///     <7|4|5>
///     <8|5|6>
///
template<class S, class... A> class BasicStringTemplate {
public:
    using char_type = typename S::value_type;
    using traits_type = typename S::traits_type;
    using string_view_type = S;
    using ostream_type = std::basic_ostream<char_type, traits_type>;
    using EvalFunc = void(ostream_type&, const A&...);

    class Parameters;
    class Parser;
    class Expander;

    /// \brief Construct an empty template.
    ///
    /// This constructor construct an empty template.
    ///
    BasicStringTemplate() noexcept = default;

    /// \{
    ///
    /// \brief Construct template from string.
    ///
    /// These constructors are shorthands for constructing a template parser using the
    /// specified locale, then calling \ref Parser::parse() with the specified string and
    /// parameter definitions.
    ///
    /// When specifying a null-terminated string (\p c_str), if the character type of the
    /// string is `char`, but `char_type` is not `char`, the characters of the string will
    /// be widened as if by `widen()` of an output stream whose character type is
    /// `char_type`, and which is imbued with the specified locale. It is therefore only
    /// safe to use characters from the basic source character set in the string in this
    /// case.
    ///
    BasicStringTemplate(const char* c_str, const Parameters&, const std::locale& = {});
    BasicStringTemplate(string_view_type string, const Parameters&, const std::locale& = {});
    /// \}

    /// \brief Expand this template.
    ///
    /// Write the expansion of this template to the specified output stream using parameter
    /// values from the specified context objects.
    ///
    /// The field width property of the specified stream should be zero.
    ///
    void expand(ostream_type&, const A&...) const;

    /// \brief Whether template refers to specified parameter
    ///
    /// This function checks whether this template refers to the specified parameter.
    ///
    bool refers_to(const char* param_name) const noexcept;

    // Movability
    BasicStringTemplate(BasicStringTemplate&&) noexcept = default;
    auto operator=(BasicStringTemplate&&) noexcept -> BasicStringTemplate& = default;

private:
    struct Substitution;

    std::basic_string<char_type, traits_type> m_string;
    core::Slab<Substitution> m_substitutions;
};


template<class... A> using StringTemplate     = BasicStringTemplate<std::string_view, A...>;
template<class... A> using WideStringTemplate = BasicStringTemplate<std::wstring_view, A...>;




/// \brief A set of parameter definitions.
///
/// Objects this type hold a set of parameter definitions that must be used to give meaning
/// to parameter references when parsing templates.
///
template<class S, class... A> class BasicStringTemplate<S, A...>::Parameters {
public:
    class Proto;

    /// \brief Prepare for parameter definition.
    ///
    /// This function allows for the definition of a parameter with the specified name. This
    /// function does not itself define the parameter. Instead, it is the subsequent
    /// assignment of a variable reference or function object to the returned \ref Proto
    /// object that causes a new parameter to be defined.
    ///
    /// \param param_name The name of the parameter to be defined. The referenced memory in
    /// which the name is stored, must outlive the \ref Parameters object. The parameter
    /// name must consist entirely of characters from the basic source character set.
    ///
    auto operator[](const char* param_name) noexcept -> Proto;

private:
    using Map = std::map<std::string_view, std::function<EvalFunc>>;

    Map m_map;

    void define(std::string_view param_name, std::function<EvalFunc>);

    friend class BasicStringTemplate<S, A...>;
};




/// \brief Precursor for parameter definition.
///
/// Assignment of a variable reference or function object to a \ref Proto object causes a
/// parameter definition to be added to the \ref Parameters object from which the \ref Proto
/// object was produced using \ref Parameters::operator[]().
///
template<class S, class... A> class BasicStringTemplate<S, A...>::Parameters::Proto {
public:
    template<class T> void operator=(T*);
    template<class T, class C> void operator=(T C::*);
    void operator=(std::function<EvalFunc>);

private:
    Parameters& m_parameters;
    const std::string_view m_param_name;

    Proto(Parameters&, std::string_view param_name) noexcept;

    friend class Parameters;
};




/// \brief String template parser.
///
/// This class implements a parser that can be used to construct template objects from
/// string representations. If multiple templates need to be constructed, it is more
/// efficient to use a single parser object than to construct them individually without
/// explicitly using a parser object.
///
template<class S, class... A> class BasicStringTemplate<S, A...>::Parser
    : private core::BasicCharMapper<char_type, traits_type>
    , private core::BasicCharMapper<char_type, traits_type>::WidenBuffer
    , private core::BasicCharMapper<char_type, traits_type>::template ArraySeededNarrowBuffer<32> {
public:
    explicit Parser(const std::locale& = {});

    /// \{
    ///
    /// \brief Produce template from specified string.
    ///
    /// These functions produce a template from the specified string and the specified
    /// parameter definitions.
    ///
    /// If there are any syntax errors, or references to undefined parameters, these
    /// functions fail by throwing an exception of unspecified type, but with a message that
    /// describes the problem.
    ///
    /// When specifying a null-terminated string (\p c_str), if the character type of the
    /// string is `char`, but `char_type` is not `char`, the characters of the string will
    /// be widened as if by `widen()` of an output stream whose character type is
    /// `char_type`, and which is imbued with the locale of the parser. It is therefore only
    /// safe to use characters from the basic source character set in the string in this
    /// case.
    ///
    auto parse(const char* c_str, const Parameters&) -> BasicStringTemplate;
    auto parse(string_view_type string, const Parameters&) -> BasicStringTemplate;
    /// \}

    /// \{
    ///
    /// \brief Try to construct template from specified string.
    ///
    /// These functions attempt to construct a template from the specified string and the
    /// specified parameter definitions.
    ///
    /// If there is a syntax error, or a reference to an undefined parameter, these
    /// functions call the specified error handler with two argument, an error code of type
    /// \ref Error, and the error message of type `string_view_type`. If the error handler
    /// returns `false`, the parsing operation is aborted immediately, and these functions
    /// return false. Otherwise, the parsing operation continues. If new problems are
    /// encountered, the error handler will be called again. If parsing succeeds, either
    /// because there were no problems, or because the error handler returned `true` every
    /// time it was called, these functions assign the resulting template object to the
    /// specified template object variable, and return true. When these functions return
    /// false, they leave the specified template object variable untouched.
    ///
    /// When specifying a null-terminated string (\p c_str), if the character type of the
    /// string is `char`, but `char_type` is not `char`, the characters of the string will
    /// be widened as if by `widen()` of an output stream whose character type is
    /// `char_type`, and which is imbued with the locale of the parser. It is therefore only
    /// safe to use characters from the basic source character set in the string in this
    /// case.
    ///
    template<class H> bool try_parse(const char* c_str, const Parameters&, BasicStringTemplate&, H error_handler);
    template<class H> bool try_parse(string_view_type string, const Parameters&, BasicStringTemplate&,
                                     H error_handler);
    /// \}

    /// \brief String template parse errors.
    ///
    /// These are the errors that can occur while a string template is being parsed.
    ///
    enum class Error {
        /// An unterminated parameter reference was encountered.
        bad_syntax,

        /// A reference to an undefined parameter was encountered.
        bad_param_ref,
    };

private:
    class ErrorHandler;
    class ThrowingErrorHandler;
    template<class F> class FuncCallErrorHandler;

    core::BasicStringFormatter<char_type, traits_type> m_string_formatter;
    const std::locale& m_locale;
    core::ArraySeededBuffer<Substitution, 8> m_buffer;

    void do_parse(const char* c_str, const Parameters&, BasicStringTemplate&);
    void do_parse(string_view_type string, const Parameters&, BasicStringTemplate&);

    bool do_try_parse(const char* c_str, const Parameters&, BasicStringTemplate&, ErrorHandler&);
    bool do_try_parse(string_view_type string, const Parameters&, BasicStringTemplate&, ErrorHandler&);

    friend class BasicStringTemplate<S, A...>;
};




/// \brief Efficient expansion of string templates.
///
/// This class offers an easy way to efficiently expand multiple string templates. Memory
/// allocated during one expansion operation will be reused during the next.
///
template<class S, class... A> class BasicStringTemplate<S, A...>::Expander {
public:
    explicit Expander(const std::locale& = {});
    explicit Expander(core::Span<char_type> seed_memory, const std::locale& = {});

    auto expand(const BasicStringTemplate&, const A&...) -> string_view_type;

private:
    core::BasicSeedMemoryOutputStream<char_type, traits_type> m_out;
};




/// \brief Prepare for expansion of string template.
///
/// Construct an object that, if written to an output stream, expands the specified string
/// template using the specified context objects. The field width specified for that output
/// stream will be respected, and the effect will be as if the complete expansion was passed
/// to the output operator as a single string object.
///
/// If `out` is an output stream with field width set to zero, then `out << expand(templ,
/// args...)` has the same effect as `templ.expand(out, args...)`.
///
template<class S, class... A> auto expand(const BasicStringTemplate<S, A...>& templ, const A&... args) ->
    impl::ExpandStringTemplate<S, A...>;








// Implementation


// ============================ BasicStringTemplate ============================


template<class S, class... A>
struct BasicStringTemplate<S, A...>::Substitution {
    std::size_t begin, end;
    const typename Parameters::Map::value_type* param;
};


template<class S, class... A>
inline BasicStringTemplate<S, A...>::BasicStringTemplate(const char* c_str, const Parameters& parameters,
                                                         const std::locale& locale)
{
    Parser parser(locale); // Throws
    parser.do_parse(c_str, parameters, *this); // Throws
}


template<class S, class... A>
inline BasicStringTemplate<S, A...>::BasicStringTemplate(string_view_type string, const Parameters& parameters,
                                                         const std::locale& locale)
{
    Parser parser(locale); // Throws
    parser.do_parse(string, parameters, *this); // Throws
}


template<class S, class... A>
void BasicStringTemplate<S, A...>::expand(ostream_type& out, const A&... args) const
{
    string_view_type string(m_string);
    std::ios_base::fmtflags flags = out.flags();
    try {
        std::size_t i = 0;
        for (const Substitution& subst : m_substitutions) {
            out << string.substr(i, subst.begin - i); // Throws
            if (subst.param) {
                const std::function<EvalFunc>& eval_func = subst.param->second;
                eval_func(out, args...); // Throws
                out.flags(flags);
            }
            else {
                out << "@"; // Throws
            }
            i = subst.end;
        }
        out << m_string.substr(i); // Throws
    }
    catch (...) {
        out.flags(flags);
        throw;
    }
}


template<class S, class... A>
bool BasicStringTemplate<S, A...>::refers_to(const char* param_name) const noexcept
{
    std::string_view param_name_2 = param_name;
    for (const Substitution& subst : m_substitutions) {
        if (ARCHON_LIKELY(subst.param)) {
            if (ARCHON_LIKELY(param_name_2 != subst.param->first))
                continue;
            return true;
        }
    }
    return false;
}


// ============================ BasicStringTemplate::Parameters ============================


template<class S, class... A>
inline auto BasicStringTemplate<S, A...>::Parameters::operator[](const char* param_name) noexcept -> Proto
{
    std::string_view param_name_2 = param_name;
    return Proto(*this, param_name_2);
}


template<class S, class... A>
void BasicStringTemplate<S, A...>::Parameters::define(std::string_view param_name, std::function<EvalFunc> func)
{
    auto p = m_map.emplace(param_name, std::move(func)); // Throws
    bool was_inserted = p.second;
    if (ARCHON_LIKELY(was_inserted))
        return;
    throw std::runtime_error(core::format("Multiple definitions for same parameter name `%s`", param_name));
}


// ============================ BasicStringTemplate::Parameters::Proto ============================


template<class S, class... A>
template<class T> inline void BasicStringTemplate<S, A...>::Parameters::Proto::operator=(T* var)
{
    *this = [var](ostream_type& out, const A&...) {
        out << *var; // Throws
    };
}


template<class S, class... A> template<class T, class C>
inline void BasicStringTemplate<S, A...>::Parameters::Proto::operator=(T C::* var)
{
    *this = [var](ostream_type& out, const A&... args) {
        const C& obj = core::get_unique_arg_by_type<const C>(args...);
        out << obj.*var; // Throws
    };
}


template<class S, class... A>
inline void BasicStringTemplate<S, A...>::Parameters::Proto::operator=(std::function<EvalFunc> func)
{
    m_parameters.define(m_param_name, std::move(func)); // Throws
}


template<class S, class... A>
inline BasicStringTemplate<S, A...>::Parameters::Proto::Proto(Parameters& parameters,
                                                              std::string_view param_name) noexcept
    : m_parameters(parameters)
    , m_param_name(param_name)
{
}


// ============================ BasicStringTemplate::Parser ============================


template<class S, class... A>
class BasicStringTemplate<S, A...>::Parser::ErrorHandler {
public:
    virtual bool handle(Error, string_view_type message) = 0;
protected:
    ~ErrorHandler() noexcept = default;
};


template<class S, class... A>
class BasicStringTemplate<S, A...>::Parser::ThrowingErrorHandler final
    : public ErrorHandler {
public:
    ThrowingErrorHandler(const std::locale& locale) noexcept
        : m_locale(locale)
    {
    }
    bool handle(Error, string_view_type message) override
    {
        std::string message_2 = core::encode_string(message, m_locale); // Throws
        std::string message_3 = core::concat(std::string_view("Bad template: "),
                                             std::string_view(message_2)); // Throws
        throw std::invalid_argument(message_3);
    }
private:
    std::locale m_locale;
};


template<class S, class... A>
template<class F> class BasicStringTemplate<S, A...>::Parser::FuncCallErrorHandler final
    : public ErrorHandler {
public:
    FuncCallErrorHandler(F&& func)
        : m_func(std::move(func)) // Throws
    {
    }
    bool handle(Error code, string_view_type message) override
    {
        return m_func(code, message); // Throws
    }
private:
    F m_func;
};


template<class S, class... A>
inline BasicStringTemplate<S, A...>::Parser::Parser(const std::locale& locale)
    : core::BasicCharMapper<char_type, traits_type>(locale) // Throws
    , m_string_formatter(locale) // Throws
    , m_locale(locale)
{
}


template<class S, class... A>
inline auto BasicStringTemplate<S, A...>::Parser::parse(const char* c_str, const Parameters& parameters) ->
    BasicStringTemplate
{
    BasicStringTemplate templ;
    do_parse(c_str, parameters, templ); // Throws
    return templ;
}


template<class S, class... A>
inline auto BasicStringTemplate<S, A...>::Parser::parse(string_view_type string, const Parameters& parameters) ->
    BasicStringTemplate
{
    BasicStringTemplate templ;
    do_parse(string, parameters, templ); // Throws
    return templ;
}


template<class S, class... A>
template<class H>
inline bool BasicStringTemplate<S, A...>::Parser::try_parse(const char* c_str, const Parameters& parameters,
                                                            BasicStringTemplate& templ, H error_handler)
{
    FuncCallErrorHandler<H> error_handler_2(std::move(error_handler)); // Throws
    return do_try_parse(c_str, parameters, templ, error_handler_2); // Throws
}


template<class S, class... A>
template<class H>
inline bool BasicStringTemplate<S, A...>::Parser::try_parse(string_view_type string, const Parameters& parameters,
                                                            BasicStringTemplate& templ, H error_handler)
{
    FuncCallErrorHandler<H> error_handler_2(std::move(error_handler)); // Throws
    return do_try_parse(string, parameters, templ, error_handler_2); // Throws
}


template<class S, class... A>
inline void BasicStringTemplate<S, A...>::Parser::do_parse(const char* c_str, const Parameters& parameters,
                                                           BasicStringTemplate& templ)
{
    ThrowingErrorHandler error_handler(m_locale);
    do_try_parse(c_str, parameters, templ, error_handler); // Throws
}


template<class S, class... A>
inline void BasicStringTemplate<S, A...>::Parser::do_parse(string_view_type string, const Parameters& parameters,
                                                           BasicStringTemplate& templ)
{
    ThrowingErrorHandler error_handler(m_locale);
    do_try_parse(string, parameters, templ, error_handler); // Throws
}


template<class S, class... A>
inline bool BasicStringTemplate<S, A...>::Parser::do_try_parse(const char* c_str, const Parameters& parameters,
                                                               BasicStringTemplate& templ, ErrorHandler& error_handler)
{
    string_view_type string = this->widen(c_str, *this); // Throws
    return do_try_parse(string, parameters, templ, error_handler); // Throws
}


template<class S, class... A>
bool BasicStringTemplate<S, A...>::Parser::do_try_parse(string_view_type string, const Parameters& parameters,
                                                        BasicStringTemplate& templ, ErrorHandler& error_handler)
{
    char_type lead_char  = this->widen('@'); // Throws
    char_type open_char  = this->widen('{'); // Throws
    char_type close_char = this->widen('}'); // Throws
    bool abort = false;
    auto error = [&](Error code, const char* message, const auto&... params) {
        string_view_type message_2 = m_string_formatter.format(message, params...); // Throws
        bool go_on = error_handler.handle(code, message_2); // Throws
        abort = (abort || !go_on);
    };
    core::BufferContents substitutions(m_buffer);
    string_view_type param_name;
    std::size_t curr = 0;
    std::size_t end = string.size();
    for (;;) {
        std::size_t i = string.find(lead_char, curr);
        if (i == std::size_t(-1))
            break;
        if (ARCHON_UNLIKELY(i + 1 == end)) {
            error(Error::bad_syntax, "Unterminated parameter reference `%s` at end of string", lead_char); // Throws
            if (abort)
                return false;
            curr = i + 1;
            continue;
        }
        char_type ch = string[i + 1];
        if (ch == open_char) {
            std::size_t j = string.find(close_char, i + 2);
            if (ARCHON_UNLIKELY(j == std::size_t(-1))) {
                error(Error::bad_syntax, "Unterminated parameter reference `%s%s` at offset %s",
                      lead_char, open_char, i); // Throws
                if (abort)
                    return false;
                curr = i + 2;
                continue;
            }
            param_name = string.substr(i + 2, j - (i + 2));
            curr = j + 1;
        }
        else {
            param_name = string.substr(i + 1, 1);
            curr = i + 2;
        }
        const typename Parameters::Map::value_type* param = nullptr;
        if (ch != lead_char) {
            std::string_view param_name_2 = this->narrow(param_name, '\0', *this); // Throws
            auto k = parameters.m_map.find(param_name_2);
            if (k == parameters.m_map.end()) {
                error(Error::bad_param_ref, "Undefined parameter `%s` in parameter reference `%s`",
                      param_name, string.substr(i, curr - i)); // Throws
                if (abort)
                    return false;
                continue;
            }
            param = &*k;
        }
        substitutions.push_back({ i, curr, param }); // Throws
    }
    std::basic_string<char_type, traits_type> string_2(string); // Throws
    core::Slab<Substitution> substitutions_2 { core::Span(substitutions) }; // Throws
    templ.m_string = std::move(string_2);
    templ.m_substitutions = std::move(substitutions_2);
    return true;
}


// ============================ BasicStringTemplate::Expander ============================


template<class S, class... A>
inline BasicStringTemplate<S, A...>::Expander::Expander(const std::locale& locale)
    : Expander({}, locale) // Throws
{
}


template<class S, class... A>
inline BasicStringTemplate<S, A...>::Expander::Expander(core::Span<char_type> seed_memory, const std::locale& locale)
    : m_out(seed_memory) // Throws
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out.imbue(locale); // Throws
}


template<class S, class... A>
auto BasicStringTemplate<S, A...>::Expander::expand(const BasicStringTemplate& templ, const A&... args) ->
    string_view_type
{
    m_out.full_clear();
    templ.expand(m_out, args...); // Throws
    return m_out.view();
}


// ============================ * * * ============================


namespace impl {


template<class S, class... A> struct ExpandStringTemplate {
    const BasicStringTemplate<S, A...>& templ;
    std::tuple<const A&...> args;
};


template<class C, class T, class S, class... A>
auto operator<<(std::basic_ostream<C, T>& out, const ExpandStringTemplate<S, A...>& pod) -> std::basic_ostream<C, T>&
{
    std::array<C, 64> seed_memory;
    core::BasicStreamOutputAltHelper helper(out, seed_memory); // Throws
    auto func = [&](const auto&... args) {
        pod.templ.expand(helper.out, args...); // Throws
    };
    std::apply(func, pod.args); // Throws
    helper.flush(); // Throws
    return out;
}


} // namespace impl


template<class S, class... A>
inline auto expand(const BasicStringTemplate<S, A...>& templ, const A&... args) -> impl::ExpandStringTemplate<S, A...>
{
    return { templ, { args... } };
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_STRING_TEMPLATE_HPP
