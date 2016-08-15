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
 *
 * Testing the web server.
 */

#include <iostream>

#include <archon/core/time.hpp>
#include <archon/core/options.hpp>
#include <archon/core/sys.hpp>
#include <archon/util/circular_buffer.hpp>
#include <archon/util/mime_magic.hpp>
#include <archon/web/server/server.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::util;
using namespace archon::web::Server;

/*

Ambition: Highly efficient and easily embeddable HTTP/1.0 and HTTP/1.1 server

Server behavior:

For each incoming request:

  Read request line and all request headers

  Resolve the request which is also supposed to validates it


Limits:

Maximum number of simultaneous connections
Maximum number of queued requests per connection
Maximum number of queued requests altogether

 */



namespace
{
  struct FileResource: Resource
  {
    FileResource(string p, long s, string t):
      path(p), size(s), type(t), buffer(buffer_size), eoi(false) {}
    ~FileResource() throw() {}

    void activate(Request &, Response &res) throw(RequestException)
    {
      res.set_header(header_Content_Type,   type);
      res.set_header(header_Content_Length, size);
      in.open(path);
    }

    void write(OutputStreamNew &out) throw(WriteException)
    {
      if(buffer.size() <= low_warter_mark && !eoi)
      {
        eoi = buffer.fill_from_stream(in);
        if(eoi) in.close();
      }
      if(buffer.empty_to_stream(out) && eoi) out.close();
    }

  private:
    string const path;
    long   const size;
    string const type;
    static size_t const buffer_size = 1024;
    static size_t const low_warter_mark = 256;
    CircularBuffer<char> buffer;
    bool eoi;
    FileInputStream in;
  };


  struct DirResolver: Resolver
  {
    DirResolver(string b): base(b)
    {
      mime_magic.reset(new_mime_magician().release());
    }

    UniquePtr<Resource> resolve(string path) throw(RequestException)
    {
      UniquePtr<Resource> r;
      path = base + path;
      try
      {
        File::Stat stat(path);
        switch(stat.get_type())
        {
        case File::Stat::type_Regular:
          {
            string const type = mime_magic->check(path);
            r.reset(new FileResource(path, stat.get_size(), type));
          }
          break;

        default:
          break;
        }
      }
      catch(File::AccessException &e)
      {
        throw RequestException(status_404_Not_Found,
                               "Unable to access '"+path+"': "+string(e.what()));
      }
      return r;
    }

  private:
    string base;
    UniquePtr<MimeMagician> mime_magic;
  };


  struct RequestDumpResource: Resource
  {
    RequestDumpResource(string): buffer(buffer_size) {}

    void activate(Request &req, Response &) throw(RequestException)
    {
      cerr << "Request-Method: "   << MethodEnum(req.get_method())     << endl;
      cerr << "Request-URI: "      << req.get_uri()                    << endl;
      cerr << "Request-Protocol: " << ProtocolEnum(req.get_protocol()) << endl;
      {
        int const n = req.get_num_headers();
        for(int i=0; i<n; ++i)
          cerr << HeaderEnum(req.get_header_name(i)).str() << ": " <<
            req.get_header_value(i) << endl;
      }
      cerr << "---------- BODY BEGIN ----------" << endl;
    }

    void read(InputStreamNew &in) throw(ReadException)
    {
      size_t const n = in.read(buffer.get(), buffer_size);
      if(n == 0) cerr << "---------- BODY END ----------" << endl;
      else cerr.write(buffer.get(), n);
    }

  private:
    static size_t const buffer_size = 1024;
    Array<char> buffer;
  };


  struct EchoUploadResource: Resource
  {
    EchoUploadResource(string): buffer(1024), eoi(false) {}

    void read(InputStreamNew &in) throw(ReadException)
    {
      eoi = buffer.fill_from_stream(in);
    }

    void write(OutputStreamNew &out) throw(WriteException)
    {
      if(buffer.empty_to_stream(out) && eoi) out.close();
    }

  private:
    CircularBuffer<char> buffer;
    bool eoi;
  };
}


int main(int argc, char const *argv[]) throw()
{
  int opt_port = 8008;

  CommandlineOptions o;
  o.add_help("Test Application for the Archon web server");
  o.check_num_args();

  o.add_param("p", "port", opt_port, "Select the port number to bind to");

  if(int stop = o.process(argc, argv)) return stop == 2 ? 0 : 1;

  sys::signal::ignore_signal(SIGPIPE); // Required by the web server

  UniquePtr<Server> const serv(new_default_server(opt_port).release());

  DirResolver tmp_resolver("/tmp");
  serv->register_context("/dir/tmp", &tmp_resolver);

  DirResolver home_resolver("/home");
  serv->register_context("/dir/home", &home_resolver);

  SimpleResolver<RequestDumpResource> dump_resolver;
  serv->register_context("/dump", &dump_resolver);

  SimpleResolver<EchoUploadResource> echo_resolver;
  serv->register_context("/echo", &echo_resolver);

  serv->serve();

  return 0;
}
