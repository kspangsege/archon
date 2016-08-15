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

#include <fstream>

#include <archon/core/cxx.hpp>
#include <archon/core/config.hpp>


using namespace std;
using namespace archon::core;


namespace
{
  wstring const segment_extra_chars = L"*-._";
}


namespace archon
{
  namespace core
  {
    string ConfigBuilder::ParamBase::print_type(type_info const &type) const
    {
      return Cxx::demangle(type.name());
    }


    void ConfigBuilder::add_param(UniquePtr<ParamBase> p)
    {
      config->validate_short_name(p->short_name, "Short parameter name");
      if(p->long_name.empty())
      {
        string msg = "Local parameter name is missing";
        if(!p->short_name.empty())
          msg += " for parameter with short name '"+enc(p->short_name)+"'";
        throw ConfigDefineException(msg+".");
      }
      config->validate_local_name(p->long_name, "Local parameter name");

      bool const has_short_name = !p->short_name.empty();
      NameMap::iterator short_i, long_i;

      if(has_short_name)
      {
        pair<NameMap::iterator, bool> const r = short_map.insert(make_pair(p->short_name, 0));
        if(!r.second)
        {
          ParamBase const *const q = config->get_param(r.first->second);
          throw ConfigDefineException("Short parameter name '"+enc(p->short_name)+"' already "
                                      "in use. Long name of first parameter is "
                                      "'"+enc(q->path+q->long_name)+"'. Long name of second "
                                      "parameter is '"+enc(p->path+p->long_name)+"'.");
        }
        short_i = r.first;
      }

      {
        pair<NameMap::iterator, bool> const r = long_map.insert(make_pair(p->long_name, 0));
        if(!r.second)
        {
          if(has_short_name) short_map.erase(short_i);
          ParamBase const *const q = config->get_param(r.first->second);
          string msg = "Long parameter name '"+enc(p->path+p->long_name)+"' already in use.";
          if(!q->short_name.empty())
            msg += " Short name of first parameter is '"+enc(q->short_name)+"'.";
          if(has_short_name)
            msg += " Short name of second parameter is '"+enc(p->short_name)+"'.";
          throw ConfigDefineException(msg);
        }
        long_i = r.first;
      }

      try
      {
        long_i->second = config->register_param(p);
        if(has_short_name) short_i->second = long_i->second;
      }
      catch(...)
      {
        if(has_short_name) short_map.erase(short_i);
        long_map.erase(long_i);
        throw;
      }
    }


    void ConfigBuilder::add_group(PublisherBase const &p, wstring name)
    {
      size_t const n = name.size();
      if(n == 0) throw ConfigDefineException("Empty parameter group name.");
      for(size_t i=0; i<n; ++i)
      {
        wchar_t const c = name[i];
        if(segment_extra_chars.find(c) == wstring::npos &&
           !config->char_mapper.is(c, ctype_base::alnum))
        throw ConfigDefineException("Parameter group name '"+enc(name)+"' "
                                    "contains illegal characters.");
      }

      if(!groups.insert(name).second)
        throw invalid_argument("Two or more parameter groups named '"+enc(path+name)+"'.");

      ConfigBuilder b(config, path+name+L":");
      p.populate(b);
    }


    string ConfigBuilder::enc(wstring s) const
    {
      return config->enc(s);
    }

    wstring ConfigBuilder::dec(string s) const
    {
      return config->dec(s);
    }




    int Config::get_num_params() const
    {
      return params.size();
    }

    wstring Config::get_param_short_name(int idx) const
    {
      return params.at(idx)->short_name;
    }

    wstring Config::get_param_long_name(int idx) const
    {
      ParamBase const *const p = params.at(idx);
      return p->path + p->long_name;
    }

    wstring Config::get_param_description(int idx) const
    {
      return params.at(idx)->description;
    }

    bool Config::is_param_bool(int idx) const
    {
      return params.at(idx)->is_bool();
    }

    bool Config::is_param_default(int idx) const
    {
      return params.at(idx)->has_default_val();
    }

    wstring Config::get_param_default_val(int idx) const
    {
      return params.at(idx)->get_default_val();
    }

    wstring Config::get_param_type(int idx) const
    {
      return dec(params.at(idx)->get_type());
    }

    wstring Config::get_param_val(int idx) const
    {
      return params.at(idx)->get_val();
    }

    void Config::set_param_val(int idx, wstring val)
    {
      params.at(idx)->set_val(val);
    }

    void Config::test_param_val(int idx, wstring val) const
    {
      params.at(idx)->test_val(val);
    }


    Config::Config(locale const &loc):
      ConfigBuilder(this, L""), char_codec(true, loc), char_mapper(loc),
      value_codec(locale::classic()) {}



    void Config::validate_short_name(wstring name, string what) const
    {
      size_t const n = name.size();
      if(n == 0) return;
      if(1 < n) throw ConfigDefineException(what+" '"+enc(name)+"' has more than one character.");
      if(!char_mapper.is(name[0], ctype_base::graph))
        throw ConfigDefineException(what+" '"+enc(name)+"' contains non-graphical character.");
    }

    void Config::validate_local_name(wstring name, string what) const
    {
      size_t const n = name.size();
      if(n == 1)
        throw ConfigDefineException(what+" '"+enc(name)+"' must be more than one character.");
      for(size_t i=0; i<n; ++i)
      {
        wchar_t const c = name[i];
        if(segment_extra_chars.find(c) == wstring::npos &&
           !char_mapper.is(c, i==0 ? ctype_base::alpha : ctype_base::alnum))
        throw ConfigDefineException(what+" '"+enc(name)+"' contains illegal characters.");
      }
    }


    int Config::register_param(UniquePtr<ParamBase> p)
    {
      int const idx = params.size();
      ParamBase *const q = p.get();
      params.push_back(p);
      try
      {
        on_new_param(q);
        return idx;
      }
      catch(...)
      {
        params.pop_back();
        throw;
      }
    }



    void save_config(ConfigBase const &cfg, string path, locale const &loc)
    {
      int const comment_max_width = 74;
      wstring::size_type const max_default_val_len = max(comment_max_width - 6, 5);
      Text::WideTrimmer const trimmer(loc);
      Text::WideOptionalWordQuoter const default_val_quoter(L".", loc);
      Text::WideOptionalWordQuoter const main_quoter(L"", loc);
      wostringstream out;
      out.imbue(locale::classic());
      bool first = true;
      int const n = cfg.get_num_params();
      for(int i=0; i<n; ++i)
      {
        if(!first) out << L"\n";
        first = false;
        bool const has_default_val = cfg.is_param_default(i);

        // Emit parameter description
        {
          wstring d = trimmer.trim(cfg.get_param_description(i));
          if(!d.empty() && d[d.size()-1] != L'.') d += L".";
          if(!has_default_val)
          {
            wstring s = cfg.get_param_default_val(i);
            wstring t = default_val_quoter.print(s);
            if(t.size() > max_default_val_len)
            {
              if(s.size()+3 >= max_default_val_len)
                s = s.substr(s.size()+3-max_default_val_len);
              for(;;)
              {
                t = L"..." + default_val_quoter.print(s);
                if(t.size() <= max_default_val_len) break;
                s.erase(0, 1);
              }
            }
            d += L" The default is "+t+L".";
          }
          wistringstream in(Text::format(d, comment_max_width-2, loc));
          in.imbue(locale::classic());
          Text::LineReader<wchar_t> reader(in, loc);
          wstring line;
          while(reader.generate(line)) out << "# " << line << "\n";
        }

        // Emit name/value association
        {
          if(has_default_val) out << L"#";
          out << cfg.get_param_long_name(i)<<" "<<main_quoter.print(cfg.get_param_val(i)) << "\n";
        }
      }
      {
        WideLocaleCodec const codec(true, loc);
        ofstream fout(path.c_str());
        if(!fout) throw ConfigFileOpenException("Unable to open '"+path+"' for writing");
        fout.imbue(locale::classic());
        fout << codec.encode(out.str());
      }
    }
  }
}
