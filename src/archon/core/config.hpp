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

#ifndef ARCHON_CORE_CONFIG_HPP
#define ARCHON_CORE_CONFIG_HPP

#include <typeinfo>
#include <stdexcept>
#include <set>
#include <map>
#include <locale>
#include <string>

#include <archon/core/meta.hpp>
#include <archon/core/unique_ptr.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/text.hpp>


namespace Archon
{
  namespace Core
  {
    /**
     * Thrown by the configuration builder when it sees invalid
     * definitions of configuration parameters.
     */
    struct ConfigDefineException;

    /**
     * Thrown when a string representation of a parameter value is
     * invalid and cannot be parsed.
     */
    struct ConfigDecodeException;



    /**
     * An abstract base class for a set of configuration parameters.
     *
     * This class defines the methods that are necessary to discover
     * the relevant details about each parameter, it defines methods
     * for reading and writing parameter values in the form of
     * strings.
     *
     * Each parameter has a type which restricts the set of valid
     * values that it can attain. If one tries to set an invalid
     * value, \c ConfigDecodeException will be thrown.
     *
     * The methods of this class need not be thread-safe. However, it
     * must be safe for multiple threads to access multiple instances
     * as long as one instance is only accessed by one thread at any
     * time.
     *
     * Long names are like filesystem paths except that ':' is used in
     * place of '/'. In this fully qualified form, each parameter has
     * a unique long name.
     *
     * The fully qualified long name establishes the group to which
     * the parameter belongs in the same way as a file system path
     * establishes the directory of a file.
     *
     * The local name of a parameter is defined as the last name
     * segement of the fully qualified long name.
     *
     * A local name must contain at least two characters. Other
     * segments of a long name are required to have at least one
     * character.
     *
     * Short names are optional, that is, they are considered absent
     * if the string is empty. A present short name must contain
     * precisely one character. Local names are mandatory, and cannot
     * be empty.
     *
     * Short names are not unique across the entire set of
     * parameters. They are unique however, within a specific group as
     * determined by the fully qualified long name.
     *
     * The single character of a short parameter name must be
     * graphical, that is, the character must fall into the \c graph
     * class for the locale in effect.
     *
     * Local parameter names must consist entirely of characters that
     * are either alpha numeric, or are one of ".-_*", and the first
     * character must not be a digit.
     *
     * The other segments of a long name must consist entirely of
     * characters that are either alpha numeric, or are one of ".-_*".
     */
    struct ConfigBase
    {
      virtual int get_num_params() const = 0;
      virtual std::wstring get_param_short_name(int idx) const = 0;
      virtual std::wstring get_param_long_name(int idx) const = 0;
      virtual std::wstring get_param_description(int idx) const = 0;
      virtual bool is_param_bool(int idx) const = 0;
      virtual bool is_param_default(int idx) const = 0;
      virtual std::wstring get_param_default_val(int idx) const = 0;
      virtual std::wstring get_param_type(int idx) const = 0; // Short informative description of type.
      virtual std::wstring get_param_val(int idx) const = 0;
      virtual void set_param_val(int idx, std::wstring val) = 0;
      virtual void test_param_val(int idx, std::wstring val) const = 0; // Does not modify the value of the parameter
      virtual ~ConfigBase() {}
    };


    /**
     * Thrown if a configuration file could not be opened.
     */
    struct ConfigFileOpenException;

    /**
     * Thrown if a problem arises with an open configuration file
     * preventing further reading and/or writing to it.
     */
    struct ConfigFileReadWriteException;

    /**
     * Thrown when sytactical problems are encountered while parsing a
     * configuration file in non-lenient mode.
     */
    struct ConfigFileParseException;


    /**
     * Read the specified configuration file and assign the result to
     * the specified configuration.
     *
     * \param log Pass a pointer to a stream on which you want
     * messages about parse errors. Without a stream (the default)
     * these messages will not be displayed anywhere.
     *
     * \param lenient By default (when false) this method fails if
     * anything goes wrong while parsing the contents of the
     * file. This includes syntactic as well as semantic
     * errors. Examples of semantic errors are unrecognized parameter
     * names, or rejected parameter values. In such a case the
     * specified configuration is left unchanged. If true, this method
     * will act in a lenient manner, which means that it will not fail
     * no matter what the contents of the file is. Instead it will, in
     * an oppertunistic manner, extract as much information as
     * possible from the file, and use that, and ignore the
     * rest. Error messages will still be displayed in the log, if a
     * log stream is specified.
     *
     * \return True if \c lenient is true and syntactical, and/or
     * semantical errors occured during parsing. True otherwise.
     *
     * \throw ConfigFileOpenException If the file could not be opened.
     *
     * \throw ConfigFileReadWriteException If a problem arises with
     * the open file preventing any further reading from it. The kind
     * of prebelms refered to here does not include any that are
     * related to the contents of the file.
     *
     * \throw ConfigFileParseException If the contents of the
     * specified file could not be parsed due to syntactic or semantic
     * errors. This exception is never thrown if the <tt>lenient</tt>
     * flag is set to true.
     */
    bool load_config(ConfigBase &, std::string path, std::ostream *log = 0,
                     bool lenient = false, std::locale const &loc = std::locale(""));

    /**
     * Write the specified configuration to the specified file.
     *
     * \throw ConfigFileOpenException If the file could not be opened.
     *
     * \throw ConfigFileReadWriteException If a problem arises with
     * the open file preventing any further writing to it. The kind
     * of prebelms refered to here does not include any that are
     * related to the contents being written.
     */
    void save_config(ConfigBase const &, std::string path,
                     std::locale const &loc = std::locale(""));



    struct Config;

    /**
     * Build a group of paramters. Ensure that each parameter in the
     * group has a unique short name, and a unique long name. Neither
     * type of name may contain ':' (colon).
     */
    struct ConfigBuilder
    {
      /**
       * Add a new 'reference' parameter to this configuration.
       *
       * The added parameter is in essence a reflection of the
       * specified variable.
       *
       * \throw ConfigDefineException If the specified paraemter is
       * invalid.
       */
      template<class T>
      void add_param(std::string short_name, std::string local_name, T &var,
                     std::string description);


      /**
       * Add a new named sub-group of parameters to this
       * configuration.
       *
       * All this method does, is to call the \c populate method on the
       * specified \c group_script instance, which is assumed to be a
       * structure of configuration parameters. For this to work, the
       * populate method must have the following signature:
       *
       * <pre>
       *
       *   void populate(ConfigBuilder &)
       *
       * </pre>
       */
      template<class S> void add_group(S &group_struct, std::string name = "");


    protected:
      struct ParamBase
      {
        std::wstring path, short_name, long_name, description;
        ParamBase(std::wstring p, std::wstring s, std::wstring l, std::wstring d):
          path(p), short_name(s), long_name(l), description(d) {}

        std::string print_type(std::type_info const &type) const;

        virtual bool is_bool() const = 0;
        virtual bool has_default_val() const = 0;
        virtual std::wstring get_default_val() const = 0;
        virtual std::string get_type() const = 0;
        virtual std::wstring get_val() const = 0;
        virtual void set_val(std::wstring) = 0;
        virtual void test_val(std::wstring) const = 0;
        virtual ~ParamBase() {}
      };

      struct PublisherBase
      {
        virtual void populate(ConfigBuilder &) const = 0;
        virtual ~PublisherBase() {}
      };

      typedef std::map<std::wstring, int> NameMap;

      ConfigBuilder(Config *c, std::wstring p): config(c), path(p) {}

    private:
      template<class T, class Codec> struct ProxyParam: ParamBase
      {
        bool is_bool() const { return SameType<T, bool>::value; }
        bool has_default_val() const { return var == init_val; }
        std::wstring get_default_val() const { return codec.encode(init_val); }
        std::string get_type() const { return print_type(typeid(T)); }
        std::wstring get_val() const { return codec.encode(var); }
        void set_val(std::wstring val) { codec.decode(val, var); }
        void test_val(std::wstring val) const { T v; codec.decode(val, v); }

        ProxyParam(T &v, std::wstring p, std::wstring s, std::wstring l,
                   std::wstring d, Codec const &c):
          ParamBase(p, s,l,d), var(v), init_val(var), codec(c) {}

        T &var;
        T const init_val;
        Codec const codec;
      };

      template<class S> struct Publisher: PublisherBase
      {
        void populate(ConfigBuilder &b) const { group_struct.populate(b); }
        Publisher(S &s): group_struct(s) {}
        S &group_struct;
      };

      void add_param(Core::UniquePtr<ParamBase> p);
      void add_group(PublisherBase const &p, std::wstring name);

      std::string enc(std::wstring s) const;
      std::wstring dec(std::string s) const;

      Config *const config;
      std::wstring const path;
      NameMap short_map, long_map;
      std::set<std::wstring> groups;
    };



    /**
     * Simple implementation of ConfigBase and ConfigBuilder allowing
     * for easy incremental addition of parameters of arbitrary type.
     *
     * It is safe for multiple threads to access multiple instances as
     * long as one instance is only accessed by one thread at any
     * time.
     *
     * \sa Config::Builder
     */
    struct Config: ConfigBase, ConfigBuilder
    {
      int get_num_params() const;
      std::wstring get_param_short_name(int idx) const;
      std::wstring get_param_long_name(int idx) const;
      std::wstring get_param_description(int idx) const;
      bool is_param_bool(int idx) const;
      bool is_param_default(int idx) const;
      std::wstring get_param_default_val(int idx) const;
      std::wstring get_param_type(int idx) const;
      std::wstring get_param_val(int idx) const;
      void set_param_val(int idx, std::wstring val);
      void test_param_val(int idx, std::wstring val) const;

      Config(std::locale const &loc = std::locale(""));

    protected:
      virtual void on_new_param(ParamBase *) {}

      ParamBase const *get_param(int idx) const { return params[idx]; }
      ParamBase *get_param(int idx) { return params[idx]; }

      void validate_short_name(std::wstring name, std::string what) const;
      void validate_local_name(std::wstring name, std::string what) const;

      std::string enc(std::wstring s) const { return char_codec.encode(s); }
      std::wstring dec(std::string s) const { return char_codec.decode(s); }

      WideLocaleCodec const char_codec;
      WideLocaleCharMapper const char_mapper;
      Text::WideValueCodec const value_codec;

    private:
      friend struct ConfigBuilder;
      template<class> friend struct ConfigCodec;

      int register_param(Core::UniquePtr<ParamBase> p);

      DeletingVector<ParamBase> params;
    };



    /**
     * The default codec used to convert parameter values to and from
     * string representation.
     */
    template<class T> struct ConfigCodec
    {
      std::wstring encode(T const &v) const;
      void decode(std::wstring s, T &v) const;

      ConfigCodec(Config const *c): codec(c->value_codec) {}

    private:
      Text::WideValueCodec const &codec;
    };


    /**
     * Specialization of the default codec for ordinary characters.
     */
    template<> struct ConfigCodec<char>
    {
      std::wstring encode(char v) const;
      void decode(std::wstring s,  char &v) const;

      ConfigCodec(Config const *c): sub_codec(c), char_mapper(c->char_mapper) {}

    private:
      ConfigCodec<wchar_t> const sub_codec;
      WideLocaleCharMapper const &char_mapper;
    };


    /**
     * Specialization of the default codec for non-wide strings.
     */
    template<> struct ConfigCodec<std::string>
    {
      std::wstring encode(std::string v) const;
      void decode(std::wstring s, std::string &v) const;

      ConfigCodec(Config const *c): codec(c->char_codec) {}

    private:
      WideLocaleCodec const &codec;
    };






    // Implementation:

    struct ConfigDefineException: std::runtime_error
    {
      ConfigDefineException(std::string m): std::runtime_error(m) {}
    };

    struct ConfigDecodeException: std::runtime_error
    {
      ConfigDecodeException(std::string m): std::runtime_error(m) {}
    };

    struct ConfigFileOpenException: std::runtime_error
    {
      ConfigFileOpenException(std::string m): std::runtime_error(m) {}
    };

    struct ConfigFileReadWriteException: std::runtime_error
    {
      ConfigFileReadWriteException(std::string m): std::runtime_error(m) {}
    };

    struct ConfigFileParseException: std::runtime_error
    {
      ConfigFileParseException(std::string m): std::runtime_error(m) {}
    };


    template<class T>
    inline void ConfigBuilder::add_param(std::string short_name, std::string long_name, T &var,
                                         std::string description)
    {
      Core::UniquePtr<ParamBase> p;
      p.reset(new ProxyParam<T, ConfigCodec<T> >(var, path, dec(short_name), dec(long_name),
                                                 dec(description), ConfigCodec<T>(config)));
      add_param(p);
    }


    template<class S>
    inline void ConfigBuilder::add_group(S &group_struct, std::string name)
    {
      if(name.empty()) group_struct.populate(*this);
      else add_group(Publisher<S>(group_struct), dec(name));
    }


    template<class T> inline std::wstring ConfigCodec<T>::encode(T const &v) const
    {
      return codec.print(v);
    }

    template<class T> inline void ConfigCodec<T>::decode(std::wstring s, T &v) const
    {
      try
      {
        v = codec.parse<T>(s);
      }
      catch(Text::ParseException &e)
      {
        throw ConfigDecodeException(e.what());
      }
    }


    inline std::wstring ConfigCodec<char>::encode(char v) const
    {
      return sub_codec.encode(char_mapper.widen(v));
    }

    inline void ConfigCodec<char>::decode(std::wstring s,  char &v) const
    {
      wchar_t w;
      sub_codec.decode(s,w);
      try
      {
        v = char_mapper.narrow_checked(w);
      }
      catch(NarrowException &e)
      {
        throw ConfigDecodeException(e.what());
      }
    }


    inline std::wstring ConfigCodec<std::string>::encode(std::string v) const
    {
      return codec.decode(v);
    }

    inline void ConfigCodec<std::string>::decode(std::wstring s, std::string &v) const
    {
      try
      {
        v = codec.encode(s);
      }
      catch(EncodeException &e)
      {
        throw ConfigDecodeException(e.what());
      }
    }
  }
}

#endif // ARCHON_CORE_CONFIG_HPP
