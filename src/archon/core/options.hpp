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

#ifndef ARCHON_CORE_OPTIONS_HPP
#define ARCHON_CORE_OPTIONS_HPP

#include <set>
#include <map>
#include <string>
#include <ostream>

#include <archon/core/config.hpp>


namespace archon
{
  namespace core
  {
    /**
     * This class can help you interpret the command-line.
     *
     * Based on a list of known command-line options, it will process
     * each entry in the specified list of command-line arguments. Any
     * argument that is an option is processed according to its
     * definition and removed from the list.
     *
     * When processing is done, only the positional arguments remain
     * in the command-line argument list. Any argument that is not an
     * option name or value is a positional argument, that is, an
     * argument whose meaning depends on its position in the list.
     *
     * Any argument in the list whose first character is '-' (dash)
     * and which is not a valid negative number, is considered to be a
     * command-line option. Further more, if \c no_negative is set to
     * true when calling the constructor, then any argument with a
     * leading dash, is an option, even if it looks like a negative
     * number.
     *
     * There are two kinds of options, those whose name is a single
     * character, and those whose names are longer than one
     * character. Short options can always be specified as '-x' where
     * x stands for the name. Long options are specified eaither as
     * '-dump' or as '--dump'. It is configurable wheter one or two
     * leading dashes are required - see below.
     *
     * An option does not need to have both a short and a long
     * name. The only requirement is that it has either a short or a
     * long name. Pass an empty string in place of a name you do not
     * want to assign.
     *
     * Options may or may not be assigned a value. There are two
     * equally valid ways to assign a value to an option. One way is
     * like this, <tt>'--size=7'</tt> or <tt>'-s=7'</tt>, where the
     * option name and value reside in the same command-line argument,
     * separated by '='. The other way is like this <tt>'--size
     * 7'</tt> or <tt>'-s 7'</tt>, where the value resides in a
     * spearate command-line argument. Optionally, the form
     * <tt>'-s7'</tt> may also be interpreted as the assignment of '7'
     * to '-s'.
     *
     * Whenever the form '--foo <arg>' is seen, then '<arg>' is
     * interpreted as an option value percisely when '--foo' accepts a
     * value, and '<arg>' is not itself an option.
     *
     * There are several possible meanings of '-dump' (signgle dash
     * followed by multiple character), and it is configurable which
     * one you want. On a per interpreter basis, you choose whether
     * this form must be interpreted as a short or as a long option
     * name. If it is interpreted as a short name, the following
     * characters 'ump' is either a value to be assigned to the
     * option, or it is an abbreviated form of '-ump', that is, '-d
     * -ump' were abbreviated as '-dump'. Whether 'ump' will be
     * interpreted as a value or as another option depends on whether
     * '-d' accepts a value assignment. 'ump' will be interpreted as a
     * value if '-d' accepts an argument, or if 'ump' has a leading
     * decimal digit and \c allow_numeric_names is false. This is so
     * even if 'ump' contains '='.
     *
     * An argument equal to '-' has no special meaning. In particular
     * it will not be interpreted as an option, although it may end up
     * as an option value.
     *
     * NOT NECESSARILY, ONLY IF REGISTERED AS SUCH (WHAT IS ALWAYS
     * TRUE, IS THAT '--' is intepreted as the short name '-'): An
     * argument equal to '--' has the special meaning of disabling
     * option intepretation in the remainder of the command-line
     * arguments. This allows you to pass positional arguments even
     * though they look like options. If \c long_has_one_dash is
     * false, '--' can be combined with other short names as in
     * '-x-y'. In such cases the effect is to disable option
     * interpretation for the following command line argument, meaning
     * that it cannot even become an option value.
     *
     * An argument beginning with '-=' is handled in the same
     * syntactical manner as one beginning with '-x'. Thus, in this
     * case '=' is either a short name, or if \c long_has_one_dash is
     * true, it can also be the first character of a long name.
     *
     * An argument beginning with '--=' is handled in the same
     * syntactical manner as one beginning with '-x='. Thus, it is an
     * assignment to the option whose short name is '-'.
     */
    struct CommandlineOptions: Config
    {
      /**
       * \param long_has_one_dash Set to true if options with long
       * names are specified with a single leading dash. Otherwise
       * they are specified with two leading dashes.
       *
       * \param allow_numeric_names Set to true if you want any
       * command-line argument with a leading dash to be interpreted
       * as an option. Otherwise an argument will be interpreted as an
       * argument only if it is not a valid negative integer or
       * floating point value.
       */
      CommandlineOptions(bool long_has_one_dash = false, bool allow_numeric_names = false,
                         std::locale const &loc = std::locale(""));


      /**
       * Add a configuration parameter that can be used both as a
       * command-line switch and in a configuration file.
       *
       * \param shortName A one character name for this option when
       * used as a command-line switch with one hyphen prepended. Use
       * the empty string if this option has no short name.
       *
       * \param longName A multi character name for this option that
       * can be used as a command-line switch with two hyphens
       * prepended or as an option identifier in a configuration
       * file. Use the empty string if this option has no long name,
       * but note that this implies that this option cannot be
       * referenced from a configuration file.
       *
       * \param var A reference to the variable in which the parameter
       * value will be stored when parsing the command-line and
       * configuration files.
       *
       * \param val The value that is stored in 'var' for options that
       * do not require a value when specified and when no value is
       * specified.
       *
       * \param description The description for this option that is
       * included when an option listing is produced.
       *
       * \param domainChecker An instance of a class with a 'check'
       * method like Unrestricted. This method will be called to
       * determine wether the value is acceptable (within the allowed
       * domain).
       *
       * \note You can add options with variables of any type T as
       * long as specializations of Parser<T>::parse and
       * Parser<T>::format exists. Specializations are provided in
       * options.cpp for the following types: bool, int, double,
       * string.
       */
/*
      template<class T, class D>
      void addConfig(std::string shortName, std::string longName,
                     T &var, T val, std::string description,
                     WantArg want_arg, D domainChecker)
        throw(DefinitionException)
      {
        verify(shortName, longName);
        std::unique_ptr<Def> d;
        d.reset(new DefVar<T, ConfigCodec<T>, D>(false, shortName, longName, var, val,
                                                 want_arg, ConfigCodec<T>(value_codec),
                                                 domainChecker, description));
        defs.append(std::move(d));
      }
*/

      /**
       * Same as template<class T, class D> addConfig(string,
       * string, T &, T, string, WantArg, D) except that the domain
       * checker is defaulted to Unrestricted<T>.
       *
       * \sa addConfig(std::string, std::string, T &, T, std::string, WantArg, D)
       */
/*
      template<class T>
      void addConfig(std::string shortName, std::string longName,
                     T &var, T val, std::string description,
                     WantArg want_arg = want_arg_Never)
        throw(DefinitionException)
      {
        verify(shortName, longName);
        std::unique_ptr<Def> d;
        d.reset(new DefVar<T, ConfigCodec<T>, Unrestricted<T> >
                (false, shortName, longName, var, val, want_arg,
                 ConfigCodec<T>(value_codec), Unrestricted<T>(), description));
        defs.append(std::move(d));
      }
*/

      /**
       * Add an option that can be used only as a command-line switch.
       * The meaning of the parameters are the same as for Config::add_param().
       *
       * \sa addConfig(std::string, std::string, T &, T, std::string, WantArg, D)
       */
      template<class T>
      void add_switch(std::string short_name, std::string long_name, T &var, T val,
                      std::string description, bool accept_val = false)
      {
        std::unique_ptr<Def> o;
        o.reset(new DefSwitchVar<T, ConfigCodec<T> >(dec(short_name), dec(long_name),
                                                     dec(description), accept_val, var,
                                                     val, ConfigCodec<T>(this)));
        add_switch(std::move(o));
      }


      template<class T, class Obj>
      void add_switch(std::string short_name, std::string long_name,
                      T Obj::*memb, Obj *obj, T val,
                      std::string description, bool accept_val = false)
      {
        std::unique_ptr<Def> o;
        o.reset(new DefSwitchMemb<T, Obj, ConfigCodec<T> >(dec(short_name), dec(long_name),
                                                           dec(description), accept_val, memb, obj,
                                                           val, ConfigCodec<T>(this)));
        add_switch(std::move(o));
      }


      /**
       * Add automatic handling of command-line help.
       *
       * \param args A description of the positional arguments
       * expected by the application. This discription will be
       * displayed as part of the command synopsis when help is
       * requested by the user. If the positional arguments check is
       * added (see <tt>check_num_args</tt>) then this list is
       * assumed to be a space separated list of named arguments, and
       * it will be displayed in a manipulated fashion to reveal
       * information about the number of required arguments.
       *
       * \sa check_num_args
       */
      void add_help(std::string description = "", std::string args = "",
                    std::string short_name = "h", std::string long_name = "help");

      /**
       * Add a check on the number of positional arguments.
       *
       * If the number of positional arguments (those arguments that
       * remain after option names and values have been removed) is
       * not within the specified acceptable range, then an error
       * message will be displayed.
       *
       * If automatic help is also added (see <tt>add_help</tt>) then
       * this method will impact the display of the description of
       * positional arguments as follows:
       *
       * - Each arguement will be displayed in capitalized form.
       *
       * - An argument, whose zero-based index within the \c args
       *   argument of the \c add_help method is greater than or equal
       *   to <tt>min_num_args</tt>, is taken to be an optional
       *   argument and is displayed inside a pair of square brackets.
       *
       * - If there is no upper limit to the number of accepted
       *   positional arguments (negative <tt>max_num_args</tt>) then
       *   an elipsis is added after the last displayed argument.
       *
       * Also, when automatic help is added, the \c process method
       * will check that the number of positional arguments does not
       * exceed the number of arguments mentioned in this string.
       *
       * \param min The minimum number of positional arguments that
       * must be available. A negative value means that the minimum
       * number is set to the number of space separated entries in the
       * \c args argument of the \c add_help method if that method is
       * called, otherwise it means zero.
       *
       * \param max The maximum number of positional arguments that
       * are accepted. If the specified value is negative, it means
       * that an unlimited number of arguments are accepted,
       * otherwise, if the specified number is less than the effective
       * value of <tt>min</tt>, it is taken to be equal to the
       * effective value of <tt>min</tt>.
       *
       * \sa add_help
       */
      void check_num_args(int min = -1, int max = 0);

      void add_version(std::string version,
                       std::string short_name = "v", std::string long_name = "version");

      void add_stop_opts(std::string short_name = "-", std::string long_name = "");

      void handle_config_file(std::string path,
                              std::string path_opt_name = "config-file",
                              std::string save_opt_name = "save-config");


      /**
       * Apply the effects of the switches on the command-line. 'argc'
       * and 'argv' are adjusted so that all processed switches are
       * filtered out.
       *
       * \param switchesOnly If true then only the options that were
       * added with 'addSwitch' will be processed. Other command-line
       * switches will be left in the command-line. Also, no parse
       * errors are reported for unknown switches and for left out
       * switches. When this call is followed by a call with
       * switchesOnly = false, then errors are reported for unknown
       * switches.
       *
       * \return Zero if all is good and application should proceed
       * normally. One if an error has occured during command-line
       * interpretation, and the application should quit immediately
       * with a non-zero status. If automatic help options or version
       * options have been requested, then 'two' is returned to
       * indicate that help was displayed, and the application should
       * exit immediately with a status of zero.
       */
      int process(int &argc, char const *argv[]);

      /**
       * Apply the effects of a configuration file.
       *
       * \return True if a parse error occured.
       */
//      bool processConfigFile(std::string path);

      /**
       * Save all the options and their values into the specified file
       * in a format that is readable by processConfigFile. Options
       * that are added with addSwitch and options without a long name
       * are not included.
       */
//      void saveConfigFile(std::string path);

      /**
       * Print a description of all known options.
       *
       * \param width The maximum width of each line of the output
       * measured in number of characters. A value less than 1 means
       * that the maximum width must be determined automatically.
       */
      std::string list_options(int max_width = 0);

    private:
      struct Def;
      struct DefConfigParam;
      template<class, class> struct DefSwitchVar;
      template<class, class, class> struct DefSwitchMemb;
      struct DefStopOpts;

      struct Lookup;
      struct Interpreter;

      void on_new_param(ParamBase *p); // Overriding Config::on_new_param
        void add_switch(std::unique_ptr<Def>);
        void add_top_level_option(std::unique_ptr<Def>);

      bool const long_has_one_dash;
      bool const allow_numeric_names;

      std::locale const loc;

      std::wstring const long_prefix; // Either "-" or "--"

      std::vector<std::unique_ptr<Def>> options;
      NameMap top_level_short_map, top_level_long_map;

      bool opt_help;
      std::wstring help_descr;
      std::wstring help_args;
      std::wstring help_disp;

      bool enable_check_num_args;
      int min_num_args, max_num_args;

      bool opt_version;
      std::wstring version;

      bool opt_stop_opts;

      bool config_file_enable;
      std::string config_file_default_path;
      std::string config_file_opt_path;
      bool config_file_opt_save;
    };







    // Implementation:

    struct CommandlineOptions::Def
    {
      std::wstring const short_name;   // eg. -v
      std::wstring const long_name;    // eg. --version
      std::wstring const description;
      bool const accept_val;

      Def(std::wstring short_name, std::wstring long_name,
          std::wstring description, bool accept_val):
        short_name(short_name), long_name(long_name),
        description(description), accept_val(accept_val) {}

      virtual std::wstring get_val() const { return L""; }
      virtual bool has_default_val() const { return true; }
      virtual std::wstring get_default_val() const { return L""; }

      /**
       * Return true on error
       *
       * \throw ConfigDecodeException When option value could not be parsed.
       */
      virtual void execute(std::wstring arg) = 0;

      virtual bool is_switch() const { return false; }

      virtual ~Def() {}
    };


    template<class T, class Codec>
    struct CommandlineOptions::DefSwitchVar: Def
    {
      DefSwitchVar(std::wstring short_name, std::wstring long_name,
                   std::wstring description, bool accept_val,
                   T &var, T val, Codec const &c):
        Def(short_name, long_name, description, accept_val),
        var(var), init_val(var), new_val(val), codec(c) {}

      void execute(std::wstring arg)
      {
        if(arg.empty())
        {
          var = new_val;
          return;
        }

        T v;
        codec.decode(arg, v);
        var = v;
      }

      std::wstring get_val() const { return codec.encode(var); }
      bool has_default_val() const { return var == init_val; }
      std::wstring get_default_val() const { return codec.encode(init_val); }
      bool is_switch() const { return true; }

      T &var;
      T const init_val, new_val;
      Codec const codec;
    };


    template<class T, class Obj, class Codec>
    struct CommandlineOptions::DefSwitchMemb: Def
    {
      DefSwitchMemb(std::wstring short_name, std::wstring long_name,
                    std::wstring description, bool accept_val,
                    T Obj::*m, Obj *o, T val, Codec const &c):
        Def(short_name, long_name, description, accept_val),
        memb(m), obj(o), init_val(obj->*memb), new_val(val), codec(c) {}

      void execute(std::wstring arg)
      {
        if(arg.empty())
        {
          obj->*memb = new_val;
          return;
        }

        T v;
        codec.decode(arg, v);
        obj->*memb = v;
      }

      std::wstring get_val() const { return codec.encode(obj->*memb); }
      bool has_default_val() const { return obj->*memb == init_val; }
      std::wstring get_default_val() const { return codec.encode(init_val); }
      bool is_switch() const { return true; }

      T Obj::*const memb;
      Obj *const obj;
      T const init_val, new_val;
      Codec const codec;
    };
  }
}

#endif // ARCHON_CORE_OPTIONS_HPP
