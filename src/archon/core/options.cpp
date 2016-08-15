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

#include <list>
#include <locale>
#include <sstream>
#include <iostream>

#include <archon/core/assert.hpp>
#include <archon/core/functions.hpp>
#include <archon/core/term.hpp>
#include <archon/core/text_table.hpp>
#include <archon/core/options.hpp>


using namespace std;
using namespace archon::core;







//Make the config file reading feature

// Write up a short list of features and characteristics of the options impl.








namespace archon
{
  namespace core
  {
    CommandlineOptions::CommandlineOptions(bool long_has_one_dash, bool allow_numeric_names,
                                           locale const &l):
      Config(l), long_has_one_dash(long_has_one_dash), allow_numeric_names(allow_numeric_names),
      loc(l), long_prefix(long_has_one_dash?L"-":L"--"), enable_check_num_args(false),
      config_file_enable(false) {}


    struct CommandlineOptions::DefConfigParam: Def
    {
      DefConfigParam(ParamBase *p):
        Def(p->short_name, p->path+p->long_name, p->description, true), param(p) {}

      ParamBase *const param;

      void execute(wstring arg)
      {
        param->set_val(arg.empty() && param->is_bool() ? L"1" : arg);
      }

      wstring get_val() const { return param->get_val(); }
      bool has_default_val() const { return param->has_default_val(); }
      wstring get_default_val() const { return param->get_default_val(); }
    };


    void CommandlineOptions::on_new_param(ParamBase *p)
    {
      UniquePtr<Def> o(new DefConfigParam(p));
      if(p->path.empty()) add_top_level_option(o);
      else options.push_back(o);
    }


    void CommandlineOptions::add_switch(UniquePtr<Def> o)
    {
      bool const has_short_name = !o->short_name.empty();
      bool const has_long_name  = !o->long_name.empty();
      if(!has_short_name && !has_long_name)
        throw ConfigDefineException("Switch must have short and/or long name");
      if(!allow_numeric_names && !has_long_name &&
         char_mapper.is(o->short_name[0], ctype_base::digit))
        throw ConfigDefineException("Numeric switch name '"+enc(o->short_name)+"' is not allowed "
                                    "unless there is also a non-numeric name");
      validate_short_name(o->short_name, "Short switch name");
      validate_local_name(o->long_name, "Long switch name");
      add_top_level_option(o);
    }


    void CommandlineOptions::add_top_level_option(UniquePtr<Def> o)
    {
      bool const has_short_name = !o->short_name.empty();
      bool const has_long_name  = !o->long_name.empty();
      NameMap::iterator short_i, long_i;

      if(has_short_name)
      {
        pair<NameMap::iterator, bool> const r =
          top_level_short_map.insert(make_pair(o->short_name, options.size()));
        if(!r.second)
        {
          Def const *const p = options[r.first->second];
          string msg = "Short option name '"+enc(o->short_name)+"' already in use.";
          if(!p->long_name.empty())
            msg += " Long name of first option is '"+enc(p->long_name)+"'.";
          if(has_long_name) msg += " Long name of second option is '"+enc(o->long_name)+"'.";
          throw ConfigDefineException(msg);
        }
        short_i = r.first;
      }

      if(has_long_name)
      {
        pair<NameMap::iterator, bool> const r =
          top_level_long_map.insert(make_pair(o->long_name, options.size()));
        if(!r.second)
        {
          if(has_short_name) top_level_short_map.erase(short_i);
          Def const *const p = options[r.first->second];
          string msg = "Long option name '"+enc(o->long_name)+"' already in use.";
          if(!p->short_name.empty())
            msg += " Short name of first option is '"+enc(p->short_name)+"'.";
          if(has_short_name) msg += " Short name of second option is '"+enc(o->short_name)+"'.";
          throw ConfigDefineException(msg);
        }
        long_i = r.first;
      }

      try
      {
        options.push_back(o);
      }
      catch(...)
      {
        if(has_short_name) top_level_short_map.erase(short_i);
        if(has_long_name)  top_level_long_map.erase(long_i);
        throw;
      }
    }


    void CommandlineOptions::add_help(string descr, string args,
                                      string short_name, string long_name)
    {
      add_switch(short_name, long_name, &CommandlineOptions::opt_help, this, true,
                 "Display command-line synopsis followed by the list of available options");
      help_descr = dec(descr);
      help_args = dec(args);
      help_disp = long_name.empty() ? L"-"+dec(short_name) : long_prefix+dec(long_name);
    }


    void CommandlineOptions::check_num_args(int min, int max)
    {
      enable_check_num_args = true;
      min_num_args = min;
      max_num_args = max;
    }


    void CommandlineOptions::add_version(string ver,
                                         string short_name, string long_name)
    {
      add_switch(short_name, long_name, &CommandlineOptions::opt_version, this, true,
                 "Print the version number to the standard output and quit");
      version = dec(ver);
    }


    struct CommandlineOptions::DefStopOpts:
      DefSwitchMemb<bool, CommandlineOptions, ConfigCodec<bool> >
    {
      typedef DefSwitchMemb<bool, CommandlineOptions, ConfigCodec<bool> > Base;
      DefStopOpts(wstring short_name, wstring long_name,
                  wstring description, CommandlineOptions *opts):
        Base(short_name, long_name, description, false, &CommandlineOptions::opt_stop_opts,
             opts, true, ConfigCodec<bool>(opts)) {}
    };


    void CommandlineOptions::add_stop_opts(string short_name, string long_name)
    {
      core::UniquePtr<Def> o;
      o.reset(new DefStopOpts(dec(short_name), dec(long_name),
                              L"Stop any further command-line arguments "
                              L"from being intepreted as options", this));
      add_switch(o);
    }


    void CommandlineOptions::handle_config_file(string path, string path_opt_name,
                                                string save_opt_name)
    {
      config_file_enable = true;
      config_file_opt_path = config_file_default_path = path;
      add_switch("", path_opt_name, &CommandlineOptions::config_file_opt_path, this, string(),
                 "Set an alternative path for the configuration file", true);
      add_switch("", save_opt_name, &CommandlineOptions::config_file_opt_save, this, true,
                 "Save the modified configuration into the configuration file");
    }


    struct CommandlineOptions::Lookup
    {
      Lookup(CommandlineOptions *);

      // Returns true iff one or more long names have optional segments
      bool list_options(wostream &out, int max_width);

    protected:
      Def *lookup_short(wstring name);
      Def *lookup_long(wstring name, int *how = 0);

      CommandlineOptions *const opts;

    private:
      struct LongMapNode
      {
        int opt_idx; // -1 for none
        map<wstring, LongMapNode> super_segments;
        LongMapNode(): opt_idx(-1) {}
        void disp(int lev) const
        {
          for(map<wstring, LongMapNode>::const_iterator i=super_segments.begin();
              i!=super_segments.end(); ++i)
          {
            for(int j=0; j<lev; ++j) wcerr << L" ";
            wcerr << L"'"<<i->first<<L"'";
            if(0 <= i->second.opt_idx) wcerr << L" (option:"<<i->second.opt_idx<<L")";
            wcerr << endl;
            i->second.disp(lev+2);
          }
        }
      };

      typedef map<wstring, int>         ShortMap;
      typedef map<wstring, LongMapNode> LongMap;

      LongMap  long_map;
      ShortMap short_map;
    };


    struct CommandlineOptions::Interpreter: Lookup
    {
      Interpreter(CommandlineOptions *o): Lookup(o), error(false), stop_opts(false) {}

      int interpret(int &argc, char const *argv[]);

    private:
      struct OptOccur
      {
        Def *const opt;
        wstring const name;
        wstring const val;
        OptOccur(Def *o, wstring n, wstring v): opt(o), name(n), val(v) {}
      };

      bool not_option(wstring arg) const;

      Def *lookup_short(wstring name);
      Def *lookup_long(wstring name);

      void push_opt(Def *opt, wstring name, wstring val);
      bool execute_opt(OptOccur const &);

      wstring format_args(wstring s);
      void split_args(wstring s, vector<wstring> &args);

      typedef std::list<OptOccur> OptOccurs;
      OptOccurs opt_occurs;

      set<wstring> unfound, ambig;
      wostringstream log;

      bool error;
      bool stop_opts;
    };


    int CommandlineOptions::process(int &argc, char const *argv[])
    {
      opt_help = opt_version = opt_stop_opts = false;
      config_file_opt_path = config_file_default_path;
      config_file_opt_save = false;

      // 1) Build a lookup map over all options
      // 2) Filter the command line arguments while generateing a simple intermediate representation of the options and their arguments.
      // 3) Process all switch-type options in order of occurance on command-line
      // 4) Execute in-between handlers
      // 5) Process all remaining options in order of occurance on command-line
      // 6) Execute after handlers

      return Interpreter(this).interpret(argc, argv);
    }


    string CommandlineOptions::list_options(int width)
    {
      if(width < 1) width = Term::get_terminal_size().first;
      wostringstream out;
      Lookup(this).list_options(out, width);
      return enc(out.str());
    }




    CommandlineOptions::Lookup::Lookup(CommandlineOptions *o): opts(o)
    {
      typedef map<wstring, wstring> ShortPaths; // Maps short name to path
      ShortPaths short_paths;
      size_t n = opts->options.size();
      for(size_t i=0; i<n; ++i)
      {
        Def *const opt = opts->options[i];
        wstring const long_name = opt->long_name;

        // Register the short name if it has not already been
        // registered, or overwrite the previous registration unless
        // the path of the previous registration is a proper prefix of
        // the path of the new option. A proper prefix of S is a
        // prefix that is not equal to S.
        {
          wstring const short_name = opt->short_name;
          // Do not add it if it is numeric, and we disallow numeric
          // names.
          if(!short_name.empty() &&
             (opts->allow_numeric_names ||
              !opts->char_mapper.is(short_name[0], ctype_base::digit)))
          {
            // The long name may be empty, but only if it is a
            // top-level option, and in that case the path is empty
            // anyway.
            wstring const path = Text::get_prefix(wstring(L":"), long_name, true);
            pair<ShortPaths::iterator, bool> const r =
              short_paths.insert(make_pair(short_name, path));
            if(r.second || r.first->second == path || !Text::is_prefix(r.first->second, path))
            {
              r.first->second = path;
              short_map[short_name] = i;
            }
          }
        }

        LongMapNode *node = 0;
        wstring::size_type j = long_name.size();
        for(;;)
        {
          wstring::size_type const k = 0 < j ? long_name.rfind(L':', j-1) : wstring::npos;
          wstring::size_type const a = k == wstring::npos ? 0 : k+1;
          wstring const segment(long_name, a, j-a);

          node = node ? &node->super_segments[segment] : &long_map[segment];

          if(k == wstring::npos) break;
          j = k;
        }
        ARCHON_ASSERT_1(node->opt_idx == -1, "Unexpected option slot occupation");
        node->opt_idx = i;
      }
    }


    bool CommandlineOptions::Lookup::list_options(wostream &out, int width)
    {
      wstring::size_type const max_val_len = max(3*width/5, 5); // Must be at least 5
      Text::WideTrimmer const trimmer(opts->loc);
      Text::WideOptionalWordQuoter const quoter(L"(:)", opts->loc);
      Text::WideTable table;
      table.get_col(0).set_width(1);
      table.get_col(1).set_width(2.5);
      bool optional_segments = false;
      for(size_t i=0; i<opts->options.size(); ++i)
      {
        Def const *opt = opts->options[i];
        wstring short_name = opt->short_name;
        if(lookup_short(short_name) != opt) short_name.clear();
        wstring const long_name = opt->long_name;
        {
          wstring s;
          if(!short_name.empty()) s = L"-"+short_name;
          if(!long_name.empty() && !short_name.empty()) s += L", ";
          if(!long_name.empty())
          {
            s += opts->long_prefix;
            int how;
            lookup_long(long_name, &how);
            if(0 < how)
            {
              optional_segments = true;
              s += L"["+long_name.substr(0, how)+L"]"+long_name.substr(how);
            }
            else s += long_name;
          }
          table.get_cell(i,0).set_text(s);
        }
        wstring d = trimmer.trim(opt->description);
        if(!d.empty() && opts->char_mapper.is(d[d.size()-1], ctype_base::alnum)) d += L".";
        if(opt->accept_val)
        {
          {
            wstring s = opt->get_default_val();
            wstring t = quoter.print(s);
            if(t.size() > max_val_len)
            {
              if(s.size()+3 >= max_val_len)
                s = s.substr(s.size()+3-max_val_len);
              for(;;)
              {
                t = L"..." + quoter.print(s);
                if(t.size() <= max_val_len) break;
                s.erase(0, 1);
              }
            }
            d += L" (default: "+t+L")";
          }
          if(!opt->has_default_val())
          {
            wstring s = opt->get_val();
            wstring t = quoter.print(s);
            if(t.size() > max_val_len)
            {
              if(s.size()+3 >= max_val_len)
                s = s.substr(s.size()+3-max_val_len);
              for(;;)
              {
                t = L"..." + quoter.print(s);
                if(t.size() <= max_val_len) break;
                s.erase(0, 1);
              }
            }
            d += L" (current: "+t+L")";
          }
        }
        table.get_cell(i,1).set_text(d);
      }

      out << table.print(width, 2, false, opts->loc);
      return optional_segments;
    }


    CommandlineOptions::Def *CommandlineOptions::Lookup::lookup_short(wstring name)
    {
      ShortMap::const_iterator const i = short_map.find(name);
      return i == short_map.end() ? 0 : opts->options[i->second];
    }


    CommandlineOptions::Def *CommandlineOptions::Lookup::lookup_long(wstring name, int *how)
    {
      LongMap const *map = &long_map;
      LongMapNode const *node = 0;
      wstring::size_type i = name.size();
      wstring::size_type unique = 0;
      for(;;)
      {
        wstring::size_type const j = 0 < i ? name.rfind(L':', i-1) : wstring::npos;
        wstring::size_type const k = j == wstring::npos ? 0 : j+1;
        wstring const segment(name, k, i-k);

        LongMap::const_iterator const e = map->find(segment);
        if(e == map->end())
        {
          if(how) *how = 0;
          return 0;
        }

        if(!node || 0 <= node->opt_idx || 1 < map->size()) unique = k;
        node = &e->second;
        map = &node->super_segments;

        if(j == wstring::npos) break;
        i = j;
      }

      for(;;)
      {
        if(0 <= node->opt_idx)
        {
          if(how) *how = unique;
          return opts->options[node->opt_idx];
        }
        ARCHON_ASSERT_1(!node->super_segments.empty(),
                        "Unexpected 'dead' leaf in reverse map of long options");
        if(1 < node->super_segments.size())
        {
          if(how) *how = 1;
          return 0;
        }
        node = &node->super_segments.begin()->second;
      }
    }




    int CommandlineOptions::Interpreter::interpret(int &argc, char const *argv[])
    {
      vector<wstring> wide_args;
      wide_args.resize(argc-1);
      for(int i=1; i<argc; ++i) wide_args[i-1] = opts->dec(argv[i]);

      {
        int j = 1;
        int const n = wide_args.size();
        for(int i=0; i<n; ++i)
        {
          wstring arg = wide_args[i];

          // Skip positional arguments
          if(not_option(arg))
          {
            argv[j++] = argv[i+1];
            continue;
          }

          if(opts->long_has_one_dash)
          {
            wstring::size_type const k = arg.find(L'=', 2);
            wstring const name = arg.substr(1, k == wstring::npos ? wstring::npos : k-1);
            if(Def *opt = name.size() < 2 ? lookup_short(name) : lookup_long(name))
            {
              if(opt->accept_val)
              {
                wstring const val = k != wstring::npos ? arg.substr(k+1) :
                  i+1<n && not_option(wide_args[i+1]) ? wide_args[++i] : L"";
                push_opt(opt, name, val);
              }
              else if(k != wstring::npos)
              {
                log << L"Commandline error: No value allowed for '-"<<name<<L"'\n";
                error = true;
              }
              else push_opt(opt, name, L"");
            }
            else continue;
          }
          else // Long option names require two leading dashes
          {
            if(arg[1] == L'-' && 2 < arg.size() && arg[2] != L'=')
            {
              wstring::size_type const k = arg.find(L'=', 3);
              wstring const name = arg.substr(2, k == wstring::npos ? wstring::npos : k-2);
              if(Def *opt = lookup_long(name))
              {
                if(opt->accept_val)
                {
                  wstring const val = k != wstring::npos ? arg.substr(k+1) :
                    i+1<n && not_option(wide_args[i+1]) ? wide_args[++i] : L"";
                  push_opt(opt, name, val);
                }
                else if(k != wstring::npos)
                {
                  log << L"Commandline error: No value allowed for '--"<<name<<L"'\n";
                  error = true;
                }
                else push_opt(opt, name, L"");
              }
              else continue;
            }
            else // Short option name (or contraction of multiple short names)
            {
              arg = arg.substr(1);
              do
              {
                wstring const name = arg.substr(0,1);
                arg = arg.substr(1);
                if(Def *opt = lookup_short(name))
                {
                  wstring val;
                  if(opt->accept_val)
                  {
                    if(!arg.empty())
                    {
                      if(arg[0] == L'=') arg = arg.substr(1);
                      swap(val, arg);
                    }
                    else if(i+1<n && not_option(wide_args[i+1])) val = wide_args[++i];
                  }
                  push_opt(opt, name, val);
                }
                else
                {
                  arg.clear();
                  continue;
                }
              }
              while(!arg.empty());
            }
          }
        }
        argc = j;
      }


      // Process all switch-type options in order of occurance on command-line
      {
        OptOccurs::iterator i = opt_occurs.begin();
        while(i != opt_occurs.end())
        {
          if(!i->opt->is_switch())
          {
            ++i;
            continue;
          }

          if(!execute_opt(*i)) error = true;

          i = opt_occurs.erase(i);
        }
      }


      // Load the config file if possible
      if(opts->config_file_enable)
      {
      }


      // Process all remaining options in order of occurance on command-line
      {
        OptOccurs::const_iterator const e = opt_occurs.end();
        for(OptOccurs::const_iterator i=opt_occurs.begin(); i!=e; ++i)
          if(!execute_opt(*i)) error = true;
      }


      if(opts->opt_help)
      {
        int const max_width = Term::get_terminal_size().first;
        wostringstream out;
        {
          wostringstream o;
          if(!opts->help_descr.empty()) o << opts->help_descr<<L"\n\n";
          wstring const app_name =
            Text::get_suffix<wchar_t>(L"/", L"/"+opts->dec(argv[0]));
          wstring args = opts->help_args;
          if(opts->enable_check_num_args) args = format_args(args);
          else if(!opts->help_args.empty() && opts->help_args[0] != L' ') args = L" " + args;
          o << L"Synopsis: "<<app_name<<args<< L"\n\nAvailable options:";
          out << Text::format(o.str(), max_width, opts->loc) << L"\n";
        }
        if(list_options(out, max_width))
        {
          wostringstream o;
          o << L"\n";
          o <<
            L"Where '"<<opts->long_prefix<<L"[alpha:beta:]gamma:delta' means that 'gamma:delta' "
            L"is the shortest usable form of the option,\nand that 'beta:gamma:delta' and "
            L"'alpha:beta:gamma:delta' are also valid forms.";
          out << Text::format(o.str(), max_width, opts->loc) << L"\n";
        }
        cout << opts->enc(out.str()) << flush;
        return 2;
      }

      if(opts->opt_version)
      {
        int const max_width = Term::get_terminal_size().first;
        wostringstream out;
        out << Text::format(opts->version, max_width, opts->loc) << L"\n";
        cout << opts->enc(out.str()) << flush;
        return 2;
      }

      if(opts->enable_check_num_args)
      {
        int min = opts->min_num_args, max = opts->max_num_args;
        if(min < 0)
        {
          vector<wstring> args;
          split_args(opts->help_args, args);
          min = args.size();
        }
        if(argc-1 < min)
        {
          log << L"Too few arguments on command-line\n";
          error = true;
        }
        else if(0 <= max && std::max(min, max) < argc-1)
        {
          log << L"Too many arguments on command-line\n";
          error = true;
        }
      }

      if(!error && opts->config_file_enable && opts->config_file_opt_save)
      {
        save_config(*opts, opts->config_file_opt_path, opts->loc);
        cout << "Configuration saved in '"+opts->config_file_opt_path+"'\n" << flush;
        return 2;
      }

      if(error && !opts->help_disp.empty()) log << L"Try "<<opts->help_disp<<L"\n";

      {
        wstring const l = log.str();
        if(!l.empty()) cerr << opts->enc(l) << flush;
      }

      return error ? 1 : 0;
    }


    bool CommandlineOptions::Interpreter::not_option(wstring arg) const
    {
      if(stop_opts || arg.empty() || arg[0] != L'-' || arg.size() == 1) return true;
      if(opts->allow_numeric_names) return false;
      try
      {
        opts->value_codec.parse<double>(arg);
        return true;
      }
      catch(Text::ParseException &)
      {
        return false;
      }
    }


    CommandlineOptions::Def *CommandlineOptions::Interpreter::lookup_short(wstring name)
    {
      Def *opt = Lookup::lookup_short(name);
      if(opt) return opt;
      if(unfound.find(name) == unfound.end())
      {
        log << L"Commandline error: Unrecogninzed option '-"<<name<<L"'\n";
        unfound.insert(name);
      }
      error = true;
      return 0;
    }


    CommandlineOptions::Def *CommandlineOptions::Interpreter::lookup_long(wstring name)
    {
      int how;
      Def *opt = Lookup::lookup_long(name, &how);
      if(opt) return opt;
      if(how == 0)
      {
        if(unfound.find(name) == unfound.end())
        {
          log << L"Commandline error: Unrecogninzed option '"<<opts->long_prefix<<name<<L"'\n";
          unfound.insert(name);
        }
      }
      else
      {
        if(ambig.find(name) == ambig.end())
        {
          log << L"Commandline error: Ambiguous option '"<<opts->long_prefix<<name<<L"'\n";
          ambig.insert(name);
        }
      }
      error = true;
      return 0;
    }


    void CommandlineOptions::Interpreter::push_opt(Def *opt, wstring name, wstring val)
    {
      if(dynamic_cast<DefStopOpts *>(opt))
      {
        stop_opts = true;
        return;
      }
      opt_occurs.push_back(OptOccur(opt, name, val));
    }


    bool CommandlineOptions::Interpreter::execute_opt(OptOccur const &o)
    {
      try
      {
        o.opt->execute(o.val);
        return true;
      }
      catch(ConfigDecodeException &e)
      {
        wstring const prefix = o.name.size() < 2 ? L"-" : opts->long_prefix;
        log << L"Commandline error: Invalid value '"<<o.val<<L"' "
          L"for option '"<<prefix<<o.name<<L"': "<<opts->dec(e.what())<<L"\n";
        return false;
      }
    }


    wstring CommandlineOptions::Interpreter::format_args(wstring s)
    {
      vector<wstring> args;
      split_args(s, args);

      int min = opts->min_num_args, max = opts->max_num_args;
      if(min < 0) min = args.size();

      // The number of arguments to be displayed
      int const n = std::max(min, max<0 ? std::max<int>(1, args.size()) : max);

      wostringstream out;
      for(int i=0; i<n; ++i)
      {
        wstring const arg = args.empty() ? L"STRING" :
          opts->char_mapper.toupper(args[std::min<int>(i, args.size()-1)]);
        bool const optional = min <= i;
        out << L"  ";
        if(optional) out << L"["<<arg<<L"]";
        else out << arg;
      }
      if(max < 0) out << L"...";

      return out.str();
    }


    void CommandlineOptions::Interpreter::split_args(wstring s, vector<wstring> &args)
    {
      wistringstream in(s);
      Text::SimpleTokenizer<wchar_t> t(in, L"", Text::SimpleTokenizer<wchar_t>::regular,
                                       opts->loc);
      wstring arg;
      while(t.generate(arg)) args.push_back(arg);
    }




/*
    void stripLeadSpace(string &s)
    {
      string::size_type i = 0;
      while(i<s.size() && isspace(s[i])) ++i;
      s.erase(0, i);
    }



    bool CommandlineOptions::processConfigFile(string path)
    {
      bool error = false;
      ifstream in(path.c_str());
      Text::LineReader<char> lineReader(in);
      unsigned long n = 0;
      for(;;)
      {
        string l;
        if(!lineReader.generate(l)) break;
        ++n;
        stripLeadSpace(l);
        if(l.empty() || l[0] == '#') continue;
        try
        {
          string name = eatQuoteWord(l);
          if(name.empty())
          {
            cerr << path+":"+Text::print(n)+": Empty option name\n" << flush;
            error = true;
            continue;
          }
          stripLeadSpace(l);
          char const *optionArg = 0;
          string value;
          if(!l.empty())
          {
            for(;;)
            {
              string v = eatQuoteWord(l);
              if(v.empty()) break;
              if(!value.empty()) value += " ";
              value += v;
            }
            optionArg = value.c_str();
          }

          Def *d = 0;
          for(unsigned i=0; i<options.size(); ++i)
            if(options[i]->long_name.size() &&
               options[i]->long_name == name)
            {
              d = options[i];
              break;
            }

          if(!d)
          {
            cerr << path+":"+Text::print(n)+": Unrecogninzed option '"+name+"'\n" << flush;
            error = true;
            continue;
          }

          if(d->want_arg == want_arg_Never && optionArg)
          {
            cerr << path+":"+Text::print(n)+": Option '"+name+"' accepts no value\n" << flush;
            error = true;
            continue;
          }

          if(d->want_arg == want_arg_Always && !optionArg)
          {
            cerr << path+":"+Text::print(n)+": Option '"+name+"' requires a value\n" << flush;
            error = true;
            continue;
          }

          try
          {
            d->execute(optionArg);
          }
          catch(Text::ParseException &e)
          {
            cerr << path+":"+Text::print(n)+": "+e.what()+"\n" << flush;
            error = true;
          }
        }
        catch(EatException &e)
        {
          cerr << path+":"+Text::print(n)+": "+e.what()+"\n" << flush;
          error = true;
        }
      }
      return error;
    }
*/
  }
}
