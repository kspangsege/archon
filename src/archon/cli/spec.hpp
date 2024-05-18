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

#ifndef ARCHON_X_CLI_X_SPEC_HPP
#define ARCHON_X_CLI_X_SPEC_HPP

/// \file


/*


                                                                                                                            
                                                                                                                            
                                                                                                                            
                                                                                                                            
                                                                                                                            
                                                                                                                            
//
// SOME NEW IDEAS:
//
// - All complex pattern elements must have corresponding tuple elements
// - Simple pattern elements that are not value slots have no corresponding tuple elements
//
// Example: `(<foo> [-a])...` matches func(std::vector<std::pair<int, std::optional<std::monostate>>>)
// Example: `(<foo> -a...)...` matches func(std::vector<std::pair<int, std::vector<std::monostate>>>)
// Example: `(<foo> [-a...])...` matches func(std::vector<std::pair<int, std::vector<std::monostate>>>)
//
// AND EVEN (collapsed):
//
// Example: `(<foo> [-a])...` matches func(std::vector<std::pair<int, bool>>)
// Example: `(<foo> -a...)...` matches func(std::vector<std::pair<int, std::size_t>>)
// Example: `(<foo> [-a...])...` matches func(std::vector<std::pair<int, std::size_t>>)
//
// Example: `(<foo> (-a | -b | -c))...` matches func(std::vector<std::pair<int, std::size_t>>)
// Example: `(<foo> [-a | -b | -c])...` matches func(std::vector<std::pair<int, std::optional<std::size_t>>>)
// Example: `(<foo> (-a | -b | -c)...)...` matches func(std::vector<std::pair<int, std::vector<std::size_t>>>)
//
// ----------------------------->> Add test cases for these examples       
//
// Alleged rules:
//
// - In an optionality construct, if the sub-pattern is associated with std::monostate (or empty tuple type), then the type associated with the optionality construct is `bool`.
// - In a repetition construct, if the sub-pattern is associated with std::monostate (or empty tuple type), then the type associated with the repetition construct is `std::size_t`.
// - In an alternatives construct, if all branches are associated with std::monostate (or empty tuple type), then the type associated with the alternatives construct is `std::size_t`.
//
// A command-line argument that is matched with a value slot in the pattern will be parsed according to the type of the corresponding parameter of the pattern action function.
//
// The correspondence between pattern elements and function parameters is established as follows: ...
//
// In general, `T` can be used in place of `std::tuple<T>`, `std::pair<T, U>` can be used in place of std::tuple<T, U>, `std::array<T, N>` can be used in place of a `std::tuple<T, T, T, ...>` when the size of the tuple is `N`, and `std::monostate` can be used in place of `std::tuple<>`.
//
// In general, `std::vector<T>` can be used in place of `std::optional<std::vector<T>>`.
//
// These rules apply recursively, so `std::vector<T>` can effectively be used in place of `std::optional<std::tuple<std::vector<T>>>`, for example.
//
//
// A pattern is said to be *nullable* if the empty sequence of symbols match the pattern.
//
// A pattern is said to have *repeatable matches* if there exists a sequence of symbols, S, such that S itself and all repetitions of S match the pattern.
//
// The following restrictions apply in order to avoid some ambiguities:
//
// - In an optionality construct, the sub-pattern must not be nullable. For example, `[[<foo>]]` is not allowed (use `[<foo>]` instead).
// - In a repetition construct, the sub-pattern must not be nullable. For example, `[<foo>]...` is not allowed (use `[<foo>...]` instead).
// - In an alternatives construction, at most one branch is allowed to be nullable. For example, `[<foo>] | [<bar>]` is not allowed (use `[<foo> | <bar>]` instead).
// - In a repetition construct, the sub-pattern must not have repeatable matches. For example, `(<foo>...)...` is not allowed (use `<foo>...` instead).
//
// ----------------------------->> Add test cases to verify that these 4 rules are enforced.          
//
                                                                                                                            
                                                                                                                            
                                                                                                                            
                                                                                                                            
                                                                                                                            
                                                                                                                            




Explain operator precedence    





OUTDATED:

A pattern is said to be *nullable* if the empty sequence of symbols match the pattern.

A pattern is said to have *repeatable matches* if there exists a sequence of symbols, S, such that S itself and all repetitions of S match the pattern.

The following restrictions apply in order to avoid some ambiguities:

- In an optionality construct, the sub-pattern must not be nullable. For example, `[[<foo>]]` is not allowed.
- In a repetition construct, the sub-pattern must not be nullable. For example, `[<foo>]...` is not allowed (use `[<foo>...]` instead).
- In an alternatives construction, no branch must be nullable. For example, `[<foo>] | <bar>` is not allowed (use `[<foo> | <bar>]` instead).
- In a repetition construct, the sub-pattern must not have repeatable matches. For example, `(<foo>...)...` is not allowed.

----------------------------->> Add test cases to verify that these 4 rules are enforced.          









STILL SOMEWHAT CORRECT:

At the top level, a pattern is a sequence of elements. These top-level elements are either simple or complex.
A simple top-level element is a value slot (`<foo>`), an option (`--foo`), or a keyword (`foo`).
A complex top-level element is an optionality construct, `[foo]`; a repetition construct, `foo...`; or an alternatives construct, `foo | bar | ...`; where `foo` and `bar` are arbitrary sub-patterns.
The pattern function must have one parameter for each top-level element that contain at least one value-slot.

For a simple top-level element that is a value slot, the type of the corresponding pattern function parameter determines how the matching command-line argument is parsed; for example, if it is an integer parameter, the matching command-line argument will be parsed as an integer.

For a complex top-level element that is an optionality construct, `[foo]`, containing at least one value slot, and where `foo` itself is not a repetition construct (see below for the case where it is), the type of the corresponding pattern function parameter must be `std::optional<T>` where `T` is the type associated with the sub-pattern, `foo` (see below for a treatment of the type associated with a sub-pattern).

For a complex top-level element that is a repetition construct, `foo...`, containing at least one value slot, or an optionality construct containing such a repetition construct, `[foo...]`, the type of the corresponding pattern function parameter must be `std::vector<T>` where `T` is the type associated with the sub-pattern, `foo` (see below for a treatment of the type associated with a sub-pattern).

For a complex top-level element that is an alternatives construct, `foo | bar | ...`, containing at least one value slot in at least two of its branches, the type of the corresponding pattern function parameter must be `std::variant<T, U, ...>` where `T`, `U`, ... are the types associated with the sub-patterns that do contain value slots (see below for a treatment of the type associated with a sub-pattern).

For a complex top-level element that is an alternatives construct, `foo | bar | ...`, where one branch contains at least one value slot, and where all the other branches contain no value slots, the type of the corresponding pattern function parameter must be `std::optional<T>` where `T` is the type associated with the sub-pattern that contains the value slot (see below for a treatment of the type associated with a sub-pattern).

Just like a top-level pattern, a sub-pattern is a sequence of elements, which again are either simple of complex.

If at least two of the elements of a sub-pattern contain at least one value slot, the type associated with the sub-pattern must be `std::tuple<T, U, ...>` where `T`, `U`, ... are the types associated with the elements that do contain value slots. The rules that apply to the types associated with sub-pattern elements are similar to those that apply to top-level elements except that the tuple element plays the role of the pattern function parameter.

If one element of a sub-pattern contains at least one value slot, and the all the other elements contain no value slots, the type, `T`, associated with the sub-pattern must be the same as the one associated with the element that does contain a value slot, rather than `std::tuple<T>`. The rules that apply to `T` are otherwise as for the case above where there are multiple elements with value slots.

In general, `std::pair` can be used in place of a tuple with two element, and `std::array` can be used in place of a tuple with uniform element type.





-------> Give examples of patterns and list the top-level subpatterns for each.    



-------> Explain internal and cross-pattern ambiguities.




*/


#include <utility>
#include <memory>
#include <variant>
#include <string_view>
#include <vector>

#include <archon/core/type.hpp>
#include <archon/core/var_string_ref.hpp>
#include <archon/cli/attributes.hpp>
#include <archon/cli/spec_support.hpp>
#include <archon/cli/help_config.hpp>
#include <archon/cli/impl/option_action.hpp>
#include <archon/cli/processor_fwd.hpp>
#include <archon/cli/impl/pattern_action.hpp>
#include <archon/cli/impl/pattern_matcher.hpp>


namespace archon::cli {


/// \brief Command line interface specification.
///
/// FIXME: Explain: If `char_type` is not `char`, it is still possible use a variable of
/// type `std::string` or `std::string_view` as assignment target (cli::assign()), or a
/// function with a parameter of type `std::string` or `std::string_view` as pattern or
/// option action. In such cases, the produced sting values will be encoded as if by \ref
/// base::StringCodec and according to the selected locale.            
///
/// FIXME: Explain: If the action of a pattern or option is an assignment, and the target is
/// a variable of string view type (`std::string_view` or `string_view_type`), or if the
/// action is a function having parameters of string view type, then the memory referenced
/// by the produced string view objects is owned by a string holder (\ref
/// cli::BasicStringHolder). If no string holder is specified by the application (\ref
/// cli::Config::string_holder), then the effective string holder is owned by the root
/// command-line object, and its lifetime ends when the that object dies. The application
/// can extend the lifetime of the memory referenced by string views by creating its own
/// string holder, and specifying it through \ref cli::Config::string_holder.           
///
///
/// Older stuff below:
///
/// If `-f` or `--foo` takes an optional argument (opt_arg_maybe()), then an argument,         
/// `<val>`, **must** be specified as `-f<val>` or `--foo=<val>`. If, instead, `-f` or
/// `--foo` takes a mandatory argument (opt_arg_always()), then the argument can also be
/// specified as `-f <val>` or `--foo <val>`.
///
/// In `-f -b`, `-b` is interpreted as an argument to `-f`, if `-f` takes a mandatory    
/// argument (opt_arg_always()), otherwise `-b` is interpreted as a new option. Similarly
/// for `-f --bar`, `--foo -b`, and `--foo --bar`.
///
/// If `-f` takes an argument (opt_arg_always() or opt_arg_maybe()), then `-fbg` means pass        
/// argument `bg` to `-f`. Otherwise, it is equivalent to `-f -bg`. FIXME: Mention special
/// rule for `-f-`.
///
/// If `-f` requires an argument, then an empty argument can be specified as `-f ""`.
///
/// It is not possible to specify an empty argument for a short form option taking an
/// optional argument.
///
/// Single `-` followed by nothing, or by a space is interpreted as a positional argument.
///
/// FIXME: Explain how to control the locale.          
///
/// FIXME: Consider config param: Whether `x` should be considered an argument for `-foo` in
/// `--foo x`: `auto`, `no`, `yes`: `auto` means that it will be accepted if, and only if
/// `--foo` is declared with a mandatory argument, as opposed to an optional one.              
///
/// FIXME: Find an appropriate way to deal with specification of boolean values (yes/no?
/// true/false? 1/0?, case sensitive?)              
///
template<class C, class T = std::char_traits<C>> class BasicSpec {
public:
    using char_type   = C;
    using traits_type = T;

    using var_string_ref_type = core::BasicVarStringRef<C, T>;
    using help_config_type    = cli::BasicHelpConfig<C, T>;

    /// \brief Add command-line pattern.
    ///
    /// FIXME: By default, that is, if no patterns are added explicitly, \ref
    /// cli::Processor::process() acts as if one empty pattern was added with no attributes,
    /// empty description, and no associated action (as if action was `[] {}`).                                                           
    ///
    /// If a function is passed as pattern action, i.e., a *pattern function*, it must have
    /// return type `void` or `int`. If it has return type `void`, then \ref
    /// cli::Processor::process() will set `exit_status` to `EXIT_SUCCESS` when that pattern
    /// gets executed. Otherwise it will set `exit_status` to the value returned by the
    /// pattern function.
    ///
    /// FIXME: Explain what the pattern action can be other than a function (\ref
    /// cli::no_action, an l-value reference to a tuple, or a tuple-like object)                                
    ///
    /// \param attr Bitwise OR of pattern attributes, or zero for "no attributes". See \ref
    /// cli::PatternAttributes, and \ref cli::no_attributes.
    ///
    /// FIXME: Explain: Unparenthesized disjunctions are not allowed. This is because \ref
    /// cli::BasicProcessor::show_help() needs to be able to construct an unambiguous
    /// synopsis by taking the pattern string, exactly as it is specified, then prepend the
    /// program name (`argv[0]`) followed by a single space. This gives the application
    /// straightforward control over the exact appearance of the synopsis as it will be
    /// displayed by `show_help()`.                                                       
    ///
    /// FIXME: Explain implicit option specifications (through being mention in patterns)
    /// and the the associated rules                                 
    ///
    /// \sa \ref cli::pat().
    ///
    template<class A> void add_pattern(var_string_ref_type pattern, int attr, var_string_ref_type descr, A&& action);

    /// \brief Add command-line option.
    ///
    /// \param attr Bitwise OR of option attributes, or zero for "no attributes". See \ref
    /// cli::OptionAttributes, and \ref cli::no_attributes.
    ///
    /// FIXME: Describe valid forms:                                          
    ///   Short: `-x` where `x` is a single character other than `-`.
    ///   Long:  `--xxx` where `xxx` is a sequence of zero or more characters.
    ///
    /// FIXME: Explain what forms are allowed in \a forms.              
    ///
    /// FIXME: Explain what forms \a arg can take. Empty string means, no argument accepted;
    /// `<foo>` means mandatory argument named `foo`; `[<foo>]` means optional argument
    /// named `foo`. Space allowed around `[` and `]`?                 
    ///
    /// FIXME: Explain parameter substitution scheme for `descr` when formatted by \ref cli::BasicProcessor::show_help(). These are the available parameters (parameter references):
    /// `A` -> arg lexeme (`<foo>`) (only valid for options specifying an argument (mandatory or optional).
    /// `V` -> original value of associated variable (only valid with some option action types, \ref raise_flag(), \ref lower_flag(), and \ref assign()).
    /// `W` -> value assigned/passed by default (default argument) (only valid with some option action types, \ref raise_flag(), \ref lower_flag(), \ref assign(), and \ref exec()). Hmm, only some versions of exec()!?!?!?                                         
    /// `Q` -> same as `V`, but quoted as if by \ref core::quoted() with `max_size` set to 32. If value type is `std::option<T>`, then quoting only takes place when there is a value.
    /// `R` -> same as `W`, but quoted as if by \ref core::quoted() with `max_size` set to 32. If value type is `std::option<T>`, then quoting only takes place when there is a value.
    /// `E` -> list of possible enumeration values for options associated with a value type that is an enumeration. The values are formatted as `x`, `x and x`, or `x, x, ..., and x` depending on how many possible values there are.
    /// `F` -> sames as `E` but using a disjunctive form (`or` instead of `and`).
    /// `G` -> sames as `E` but with each possible value quoted as if by \ref core::quoted().
    /// `H` -> sames as `G` but using a disjunctive form (`or` instead of `and`).
    ///
    /// FIXME: Explain what forms the action can take. It can be `cli::no_action`, a tuple
    /// of one variable reference (`std::tie(var)`), any of the objects returned by the
    /// functions provided in \ref archon/cli/option_actions.hpp, or a function taking one
    /// argument.                                                           
    ///
    /// FIXME: Explain support for `std::optional<>` in option argument value types. If a
    /// variable of type `std::optional<T>` is passed to \ref assign() or a function taking
    /// an argument of type `std::optional<T>` is passed as the option action, then any
    /// option argument specified on the command line will be parsed as the type `T`. In
    /// those cases, if the option is specified without argument, the value assigned by \ref
    /// assign() or passed to the specified function will be `std::optional<T>()`. When an
    /// option argument value of type `std::optional<T>` is formatted as part of the help
    /// text, \ref cli::BasicHelpConfig::absent_value_label will be shown in place of an
    /// absent value.                                                                             
    ///
    /// \sa \ref cli::opt().
    ///
    template<class A> void add_option(var_string_ref_type forms, var_string_ref_type arg, int attr,
                                      var_string_ref_type descr, A&& action);

    /// \brief Add standard help command-line option.
    ///
    /// This function adds a command-line option that writes a description of the
    /// command-line interface to STDOUT with the help of \ref
    /// cli::BasicProcessor::show_help(). The option will use the standard option forms `-h`
    /// and `--help`. It will be a short-circuiting option (see \ref
    /// cli::optioAttributes::short_circuit).
    ///
    /// \sa \ref cli::BasicProcessor::show_help().
    /// \sa \ref cli::opt().
    /// \sa \ref cli::help_tag.
    ///
    void add_help_option(help_config_type, bool unlisted = false);

    /// \brief Add option that disables option parsing.
    ///
    /// This function adds a command-line option that causes all subsequent command-line
    /// arguments to be interpreted as positional arguments rather than as options or as
    /// keywords. See option attribute \ref cli::further_args_are_values for additional
    /// details on this kind of option.  The stop option will use the standard option form
    /// `--`.
    ///
    /// \sa \ref cli::opt().
    /// \sa \ref cli::stop_tag.
    ///
    void add_stop_option(bool unlisted = false);

    /// \brief Allow cross-pattern ambiguities.
    ///
    /// Call this function if you want to allow for the specification to have cross-pattern
    /// ambiguities. By default, cross-pattern ambiguities will not be allowed.
    ///
    /// A specification has cross-pattern ambiguities if there is some sequence of
    /// command-line arguments that will match more than one pattern.
    ///
    /// If you choose to allow cross-pattern ambiguities by calling this function, and there
    /// are more than one pattern that matches a specific command line, then the pattern
    /// than was specified first will be invoked.
    ///
    /// \sa \ref cli::SpecError::cross_pattern_ambiguity
    ///
    void allow_cross_pattern_ambiguity() noexcept;

    /// \brief Allow pattern-internal positional ambiguities.
    ///
    /// Call this function if you want to allow for the specification to have
    /// pattern-internal positional ambiguities. By default, such ambiguities will not be
    /// allowed.
    ///
    /// There are two kinds of pattern-internal ambiguities, *positional* and *structural*.
    ///
    /// Positional ambiguity is when it is unclear which of two occurrences of a symbol in
    /// the pattern correspond to a particular command-line argument. If you choose to allow
    /// for positional ambiguities by calling this function, no guarantees are given as to
    /// which of the possible possible positions each command-line argument is matched to.
    ///
    /// Structural ambiguity is when the structure (or value) of the arguments to be passed
    /// to the pattern function is not uniquely determined by the pattern given a particular
    /// matching command line, even when there is no positional ambiguity. Examples are
    /// `[[-x]]`, `(-x...)...`, and `([-x] | [-y])`. Structural ambiguity is never allowed.
    ///
    /// \sa \ref cli::SpecError::positional_ambiguity
    /// \sa \ref cli::SpecError::structural_ambiguity
    ///
    void allow_pattern_internal_positional_ambiguity() noexcept;

private:
    struct Pattern;
    struct Option;

    using pattern_action_type = impl::PatternAction<C, T>;
    using option_action_type  = impl::OptionAction<C, T>;

    std::vector<Pattern> m_patterns;
    std::vector<Option> m_options;
    impl::PatternMatcherConfig m_pattern_matcher_config;

    friend class cli::BasicProcessor<C, T>;
};


using Spec     = BasicSpec<char>;
using WideSpec = BasicSpec<wchar_t>;




/// \brief Add command-line pattern.
///
/// This function has the same effect as \ref cli::BasicSpec::add_pattern(). The advantage
/// of using this function is that it will increase the amount of space available per line
/// of arguments (i.e., decrease indentation) when code is formatted in the style shown
/// here:
///
/// \code{.cpp}
///
///    archon::cli::Spec spec;
///    pat("copy <origin path> <target path>", archon::cli::no_attributes, spec,
///        "Copy the file at the specified origin path to specified target part.",
///        [&](std::filesystem::path origin_path, std::filesystem::path target_path) {
///            std::filesystem::copy_file(origin_path, target_path);
///        });
///
/// \endcode
///
template<class C, class T, class A>
void pat(core::Type<core::BasicVarStringRef<C, T>> pattern, int attr, cli::BasicSpec<C, T>&,
         core::Type<core::BasicVarStringRef<C, T>> descr, A&& action);






/// \{
///
/// \brief Add command-line option.
///
/// These functions have the same effect as \ref cli::BasicSpec::add_option(), \ref
/// cli::BasicSpec::add_help_option(), and \ref cli::BasicSpec::add_stop_option()
/// respectively.
///
/// The advantage of using the appropriate overload of `opt()` over the corresponding member
/// function in \ref cli::BasicSpec, is that it will increase the amount of space available
/// per line of arguments (i.e., decrease indentation) when code is formatted in the style
/// shown here:
///
/// \code{.cpp}
///
///    archon::cli::Spec spec;
///    opt("-w, --width", "<num>", archon::cli::no_attributes, spec,
///        "Format text to a line length of @A (default is @V).",
///        archon::cli::assign(width));
///
/// \endcode
///
/// The overloads of `opt()` involving \ref cli::HelpTag and \ref cli::StopTag are provided
/// as alternatives to directly calling \ref cli::Spec::add_help_option() and \ref
/// cli::Spec::add_stop_option(). Using these tags with `opt()` can make it a little bit
/// clearer that one needs to count such options among the other specified options when
/// interpreting an error message that refers to the n'th option specification. These
/// overloads are intended to be used as follows (see \ref cli::help_tag and \ref
/// cli::stop_tag):
///
/// \code{.cpp}
///
///    archon::cli::Spec spec;
///    opt(archon::cli::help_tag, spec); // Same effect as spec.add_help_option({})
///    opt(archon::cli::stop_tag, spec); // Same effect as spec.add_stop_option()
///
/// \endcode
///
template<class C, class T, class A>
void opt(core::Type<core::BasicVarStringRef<C, T>> forms,
         core::Type<core::BasicVarStringRef<C, T>> arg, int attr, cli::BasicSpec<C, T>&,
         core::Type<core::BasicVarStringRef<C, T>> descr, A&& action);
template<class C, class T> void opt(cli::HelpTag, cli::BasicSpec<C, T>&,
                                    cli::BasicHelpConfig<C, T> = {}, bool unlisted = false);
template<class C, class T> void opt(cli::StopTag, cli::BasicSpec<C, T>&, bool unlisted = false);
/// \}








// Implementation


template<class C, class T>
struct BasicSpec<C, T>::Pattern {
    var_string_ref_type pattern;
    int attr;
    var_string_ref_type descr;
    std::unique_ptr<pattern_action_type> action; // Can be null
};


template<class C, class T>
struct BasicSpec<C, T>::Option {
    var_string_ref_type forms;
    var_string_ref_type arg;
    int attr;
    var_string_ref_type descr;
    std::variant<std::unique_ptr<option_action_type>, help_config_type> action; // Can be null
};


template<class C, class T>
template<class A> inline void BasicSpec<C, T>::add_pattern(var_string_ref_type pattern, int attr,
                                                           var_string_ref_type descr, A&& action)
{
    m_patterns.push_back({ std::move(pattern), attr, std::move(descr),
                           impl::make_pattern_action<C, T>(std::forward<A>(action)) }); // Throws
}


template<class C, class T>
template<class A>
inline void BasicSpec<C, T>::add_option(var_string_ref_type forms, var_string_ref_type arg, int attr,
                                        var_string_ref_type descr, A&& action)
{
    m_options.push_back({ std::move(forms), std::move(arg), attr, std::move(descr),
                          impl::make_option_action<C, T>(std::forward<A>(action)) }); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::add_help_option(help_config_type help_config, bool unlisted)
{
    int attr = cli::short_circuit;
    if (unlisted)
        attr |= cli::unlisted;
    m_options.push_back({ "-h, --help", "", attr,
                          "Show command synopsis and the list of available options.",
                          std::move(help_config) }); // Throws
}


template<class C, class T>
void BasicSpec<C, T>::add_stop_option(bool unlisted)
{
    int attr = cli::further_args_are_values;
    if (unlisted)
        attr |= cli::unlisted;
    add_option("--", "", attr,
               "All subsequent command-line arguments will be interpreted as positional "
               "arguments, even if they have a leading dash (\"-\").",
               cli::no_action); // Throws
}


template<class C, class T>
inline void BasicSpec<C, T>::allow_cross_pattern_ambiguity() noexcept
{
    m_pattern_matcher_config.allow_cross_pattern_ambiguity = true;
}


template<class C, class T>
inline void BasicSpec<C, T>::allow_pattern_internal_positional_ambiguity() noexcept
{
    m_pattern_matcher_config.allow_pattern_internal_positional_ambiguity = true;
}


template<class C, class T, class A>
inline void pat(core::Type<core::BasicVarStringRef<C, T>> pattern, int attr, cli::BasicSpec<C, T>& spec,
                core::Type<core::BasicVarStringRef<C, T>> descr, A&& action)
{
    spec.add_pattern(std::move(pattern), attr, std::move(descr), std::move(action)); // Throws
}


template<class C, class T, class A>
inline void opt(core::Type<core::BasicVarStringRef<C, T>> forms, core::Type<core::BasicVarStringRef<C, T>> arg,
                int attr, cli::BasicSpec<C, T>& spec, core::Type<core::BasicVarStringRef<C, T>> descr, A&& action)
{
    spec.add_option(std::move(forms), std::move(arg), attr, std::move(descr),
                    std::move(action)); // Throws
}


template<class C, class T> inline void opt(cli::HelpTag, cli::BasicSpec<C, T>& spec,
                                           cli::BasicHelpConfig<C, T> help_config, bool unlisted)
{
    spec.add_help_option(std::move(help_config), unlisted); // Throws
}


template<class C, class T> inline void opt(cli::StopTag, cli::BasicSpec<C, T>& spec, bool unlisted)
{
    spec.add_stop_option(unlisted); // Throws
}


} // namespace archon::cli

#endif // ARCHON_X_CLI_X_SPEC_HPP
