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

/// \file
///
/// \author Kristian Spangsege

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <sstream>
#include <iostream>

#include <archon/core/memory.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/enum.hpp>
#include <archon/core/text.hpp>
#include <archon/thread/thread.hpp>
#include <archon/util/circular_buffer.hpp>
#include <archon/util/remem_order_map.hpp>
#include <archon/web/server/server.hpp>
#include <archon/web/server/socket_server.hpp>


using namespace archon::core;
using namespace archon::thread;
using namespace archon::util;
using namespace archon::web::server;


namespace {

void find_non_sp_ht(const std::string& s, std::string::size_type from,
                    std::string::size_type &begin, std::string::size_type &end)
{
    std::string::size_type n = s.size(), i, j;
    for (i = from; i < n; ++i) {
        char c = s[i];
        if (c != ' ' && c != '\t') {
            for (j = n; i+1 < j; --j) {
                char d = s[j-1];
                if (d != ' ' && d != '\t')
                    break;
            }
            break;
        }
    }
    begin = i;
    end   = j;
}


class LineAssembler {
public:
    LineAssembler(CircularBuffer<char>& b):
        buffer(b)
    {
        reset();
    }

    void reset()
    {
        need_lf = false;
        offset = 0;
    }

    bool scan(std::string& line)
    {
        const char* begin;
        std::size_t n;
        while (buffer.get_span(begin, n, offset)) {
            if (need_lf) {
                if (begin[0] == '\n') {
                    std::size_t n = offset - 1;
                    line.clear();
                    line.reserve(n);
                    buffer.copy_to(back_inserter(line), 0, n);
                    buffer.discard(n+2);
                    reset();
                    return true;
                }
                need_lf = false;
            }

            const char* end = begin + n;
            const char* pos = std::find(begin, end, '\r');
            if (pos != end) {
                need_lf = true;
                offset += pos+1 - begin;
                continue;
            }

            offset += n;
        }

        return false;
    }

private:
    CircularBuffer<char>& buffer;
    std::size_t offset;
    bool need_lf;
};


enum RequestMode {
    request_mode_First,  ///< Next line is Request-Line (first line)
    request_mode_Header, ///< Next line is a request header or the empty line that terminates the header
    request_mode_Body,
    request_mode_Closed
};

enum ResponseMode {
    response_mode_Wait,   ///< Head section not yet generated
    response_mode_Head,   ///< Some of the head section remains to be written
    response_mode_Body,   ///< Some or the response body remains to be written
    response_mode_Closed  ///< The response body stream is closed
};


using Headers = RememOrderMap<Header, std::string>;


class ServerImpl;
class ConnectionImpl;


class ErrorResource: public Resource {
public:
    ErrorResource(ConnectionImpl* c):
        conn(c)
    {
    }
    ~ErrorResource() {}

    void reset(Status s, std::string msg)
    {
        status  = s;
        message = msg;
    }

    void activate(Request& req, Response& res) throw(RequestException);

    void write(OutputStreamNew& out) throw(WriteException)
    {
        pos += out.write(data+pos, size-pos);
        if (pos == size)
            out.close();
    }

private:
    ConnectionImpl* const conn;
    StatusEnum status;
    std::string message;
    const char* data;
    size_t size, pos;
};


class RequestImpl: public Request {
public:
    Method get_method() const
    {
        return method;
    }
    std::string get_uri() const
    {
        return uri;
    }
    Protocol get_protocol() const
    {
        return protocol;
    }
    int get_num_headers() const
    {
        return headers.size();
    }
    Header get_header_name(int i) const
    {
        return headers.remem_order_begin()[i].first;
    }
    std::string get_header_value(int i) const
    {
        return headers.remem_order_begin()[i].second;
    }
    std::string find_header(Header h) const
    {
        Headers::const_iterator i = headers.find(h);
        return i == headers.end() ? std::string() : i->second;
    }

    MethodEnum   method;
    std::string  uri;
    ProtocolEnum protocol;
    Headers      headers;
};


class ResponseImpl: public Response {
public:
    ResponseImpl(ConnectionImpl* c):
        conn(c) {}

    void reset();
    void serialize();
    bool write(OutputStreamNew& out);

    void set_status(Status s)
    {
        status = s;
    }
    void set_header(Header n, std::string v)
    {
        headers.insert(std::make_pair(n,v));
    }
    void set_header(Header n, long v);

    ProtocolEnum protocol;
    StatusEnum   status;
    Headers      headers;

    ConnectionImpl* const conn;

private:
    // Serialized
    std::string str;
    const char* data;
    std::size_t size, pos;
};


class RequestBodyStream: public InputStreamNew {
public:
    virtual void close_request_body() = 0;
    void close() throw()
    {
        close_request_body();
    }
};


class ResponseBodyStream: public OutputStreamNew {
public:
    virtual void flush_respone_body() = 0;
    virtual void close_respone_body() = 0;
    void flush() throw()
    {
        flush_respone_body();
    }
    void close() throw()
    {
        close_respone_body();
    }
};


class Context {
public:
    using SubContexts = DeletingMap<std::string, Context>;
    SubContexts sub_contexts;
    Resolver* resolver = nullptr;
};


class ServerImpl: public Server, public SocketServer {
public:
    ServerImpl(int p):
        SocketServer(p)
    {
        init_header_sets(general_headers, request_headers, response_headers,
                         entity_headers, joinable_headers);

        all_request_headers.insert(general_headers.begin(), general_headers.end());
        all_request_headers.insert(request_headers.begin(), request_headers.end());
        all_request_headers.insert(entity_headers.begin(),  entity_headers.end());

        all_response_headers.insert(general_headers.begin(),  general_headers.end());
        all_response_headers.insert(response_headers.begin(), response_headers.end());
        all_response_headers.insert(entity_headers.begin(),   entity_headers.end());

        server_string = "Archon/0.0.1";

        {
            std::ostringstream out;
            out << sys::get_hostname();
            if (p != 80)
                out << ":"<<p;
            host_string = out.str();
        }
    }

    void register_context(std::string p, Resolver* r);
    void serve()
    {
        SocketServer::serve();
    }

    bool is_request_header(Header h) const
    {
        return all_request_headers.find(h)  !=  all_request_headers.end();
    }

    bool is_response_header(Header h) const
    {
        return all_response_headers.find(h) !=  all_response_headers.end();
    }

    bool is_joinable_header(Header h) const
    {
        return joinable_headers.find(h) !=  joinable_headers.end();
    }

    Context& resolve(std::string& p);

    std::string get_server_string() const
    {
        return server_string;
    }
    std::string get_host_string() const
    {
        return host_string;
    }
    Time get_time() const
    {
        return Time::now() + time_adjust;
    }

private:
    UniquePtr<SocketServer::Connection> new_connection();

    std::string server_string, host_string;
    Time time_adjust;

    // A header field value is joinable if, and only if the entire
    // field-value for that header field is defined in the HTTP
    // protocol specification as a comma-separated list.
    std::set<Header> general_headers, request_headers, response_headers, entity_headers,
                       joinable_headers, all_request_headers, all_response_headers;

    Context root_context;
};



// FIXME: How to make sure the connection does not hold on to vast amounts of stale memory
class ConnectionImpl: public SocketServer::Connection, public RequestBodyStream,
                      public ResponseBodyStream {
public:
    ConnectionImpl(ServerImpl* s, std::size_t max_header_line_length = 4096):
        server(s),
        input_buffer(max_header_line_length+2),
        line_assembler(input_buffer),
        response(this),
        error_resource(this)
    {
    }

    ~ConnectionImpl()
    {
        std::cerr << "~ConnectionImpl" << std::endl;
    }

    void reset(); // Reset for new request and response

    ServerImpl* const server;

    Text::ValuePrinter value_printer;
    Text::ValueParser  value_parser;

private:
    void handle_read()  throw(ReadException);
    void handle_write() throw(WriteException);

    std::size_t read(char* b, std::size_t n) throw(ReadException, InterruptException)
    {
        if (n == 0)
            return 0;
        if (request_body_remain < n) {
            if (request_body_remain == 0) {
                request_mode = request_mode_Closed; // Auto-close
                return 0;
            }
            n = request_body_remain;
        }
        std::size_t m = input_buffer.copy_to(b, 0, n) - b;
        if (m == 0) {
            if (n != 0 && !end_of_input)
                throw InterruptException("Would block");
            request_mode = request_mode_Closed; // Auto-close
        }
        else {
            request_body_remain -= m;
        }
        return m;
    }

    void close_request_body()
    {
        request_mode = request_mode_Closed;
    }

    std::size_t write(const char* b, std::size_t n) throw(WriteException, InterruptException)
    {
        return get_output_stream().write(b,n);
    }

    void flush_respone_body()
    {
        get_output_stream().flush();
    }

    void close_respone_body()
    {
        response_mode = response_mode_Closed;
        if (response_mode == response_mode_Closed && true)
            get_output_stream().close();
    }

    void parse_request_header();
    void request_header_flush();
    void activate_resource();

    CircularBuffer<char> input_buffer;
    bool end_of_input = false;

    RequestMode  request_mode;
    ResponseMode response_mode;

    LineAssembler line_assembler;
    std::string line;

    RequestImpl request;
    ResponseImpl response;

    std::string request_header_name;
    std::string request_header_value;
    bool request_header_dirty;

    std::size_t request_body_remain; // Number of response body bytes remaining to be read by the associated resource

    Resource* resource;
    UniquePtr<Resource> resource_holder;

    ErrorResource error_resource;
};


void ServerImpl::register_context(std::string p, Resolver* r)
{
    if (p[0] != '/')
        throw invalid_argument("Path '"+p+"' must have a leading slash");
    std::string q = p;
    Context* c = &resolve(q); // Modifies 'q'
    if (q.empty()) {
        if (c->resolver)
            throw std::invalid_argument("Path '"+p+"' already has a resolver");
    }
    else {
        std::string::size_type i = 1;
        for (;;) {
            auto j = q.find('/', i);
            UniquePtr<Context> ctx(new Context);
            Context* d = ctx.get();
            c->sub_contexts.set_at(q.substr(i, j-i), ctx);
            c = d;
            if (j == std::string::npos)
                break;
            i = j+1;
        }
    }
    std::cerr << "Adding resolver for context path: " << p << std::endl;
    c->resolver = r;
}


Context& ServerImpl::resolve(std::string& p)
{
    Context* c = &root_context;
    std::string::size_type i = 1;
    for (;;) {
        auto j = p.find('/', i);
        auto k = c->sub_contexts.find(p.substr(i, j-i));
        if (k == c->sub_contexts.end()) {
            p = p.substr(i-1);
            return *c;
        }
        c = k->second;
        if (j == std::string::npos) {
            p.clear();
            return *c;
        }
        i = j+1;
    }
}


UniquePtr<SocketServer::Connection> ServerImpl::new_connection()
{
    // FIXME: Reuse connection objects
    ConnectionImpl* c = new ConnectionImpl(this);
    UniquePtr<SocketServer::Connection> d(c);
    c->init();
    return d;
}


void ConnectionImpl::reset()
{
    request_mode  = request_mode_First;
    response_mode = response_mode_Wait;
    line_assembler.reset();
}


/// Is called whenever reading from the input stream of the connection can
/// continue.
///
/// Read once, then do as much as possible with what we have before returning
///
/// 1) Read Request-Line
///    and call Resource *r = resolver->resolve(path)
///
/// 2) resource->
void ConnectionImpl::handle_read() throw(ReadException)
{
    if (input_buffer.full())
        return;
    if (input_buffer.fill_from_stream(get_input_stream()))
        end_of_input = true;

    for (;;) {
        switch (request_mode) {
            case request_mode_First:
            case request_mode_Header:
                try {
                    if (!parse_request_header()) {
                        if (end_of_input &&
                            (request_mode == request_mode_First || request_mode == request_mode_Header)
                            throw RequestException("Incomplete request header");
                            return;
                        }
                }
                catch(RequestException& e) {
                    std::cerr << "ERROR: "<<int(e.status)<<" "<<StatusEnum(e.status)<<": "<<e.what() << std::endl;
                    get_input_stream().close();
                    resource_holder.reset(); // FIXME: This should also be done when there is no error, and as soon as possible
                    error_resource.reset(e.status, e.what());
                    resource = &error_resource;
                    activate_resource();
                    return;
                }
                break;

            case request_mode_Body:
                resource->read(*this);
                break;

            case request_mode_Closed:
                break;
        }
    }
}


void ConnectionImpl::handle_write() throw(WriteException)
{
    switch (response_mode) {
        case response_mode_Wait:
            break;

        case response_mode_Head:
            if (!response.write(get_output_stream()))
                break;
            response_mode = response_mode_Body;
            // Fall through

        case response_mode_Body:
            resource->write(*this);
            break;

        case response_mode_Closed:
            if()
                break;
    }
}


void ConnectionImpl::parse_request_header()
{
    if (!line_assembler.scan(line)) {
        if (input_buffer.full())
            throw RequestException("Overlong line in request header");
        return;
    }

    for (;;) {
        if (request_mode == request_mode_First) {
            // FIXME: HTTP/1.1 Specification requires that we accept a number of leading empty lines

            // Parse Request-Line
            auto i = line.find(' ');
            std::string t = line.substr(0, i);
            if (!request.method.parse(t))
                throw RequestException("Unknown method '"+t+"'");
            if (i == std::string::npos) {
                i = line.size();
            }
            else {
                ++i;
            }
            auto k = i;
            i = line.find(' ', k);
            t = line.substr(k, i-k);
            request.uri = t;
            if (i == std::string::npos) {
                i = line.size();
            }
            else {
                ++i;
            }
            t = line.substr(i);
            if (!request.protocol.parse(t))
                throw RequestException("Unknown protocol '"+t+"'");
            // FIXME: Accept also HTTP/1.0 which will trigger special compatibility features
            if (request.protocol != protocol_HTTP_1_1)
                throw RequestException("Invalid protocol version '"+t+"'");

            {
                std::string path = request.uri;
                Context& ctx = server->resolve(path);
                UniquePtr<Resource> r(ctx.resolver ? ctx.resolver->resolve(path).release() : 0);
                if (!r)
                    throw RequestException(status_404_Not_Found, "Unresolvable path '"+request.uri+"'");
                resource = r.get();
                resource_holder = r;
            }

            request_mode = request_mode_Header;
            request_header_dirty = false;
        }
        else {
            if (line.empty()) { // End of request header
                if (request_header_dirty)
                    request_header_flush();

                bool exit = false;
                Headers::iterator i;
                if ((i=request.headers.find(header_Transfer_Encoding)) != request.headers.end()) {
                    throw RequestException(status_501_Not_Implemented,
                                           "Request with Transfer-Encoding '"+i->second+"'");
                }
                else if ((i=request.headers.find(header_Content_Length)) != request.headers.end()) {
                    try {
                        request_body_remain = value_parser.parse<size_t>(i->second);
                    }
                    catch(Text::ParseException& e) {
                        throw RequestException("Bad value of Content-Length header '"+i->second+"': "
                                               ""+e.what());
                    }
                }
                else {
                    get_input_stream().close();
                    exit = true;
                }

                activate_resource();

                if (!exit)
                    request_mode = request_mode_Body;
                return;
            }

            if (line[0] == ' ' || line[0] == '\t') {
                if (!request_header_dirty)
                    throw RequestException("Malformed request header '"+line+"'");
                std::string::size_type j,k;
                find_non_sp_ht(line, 1, j, k);
                request_header_value += ' ';
                request_header_value += line.substr(j, k-j);
            }
            else {
                if (request_header_dirty)
                    request_header_flush();

                // Extract name from first line of header field
                auto i = line.find(':');
                if (i == std::string::npos)
                    throw RequestException("Malformed request header '"+line+"'");
                std::string::size_type j,k;
                find_non_sp_ht(line, i+1, j, k);
                request_header_name = line.substr(0, i);
                request_header_value = line.substr(j, k-j);
                request_header_dirty = true;
            }
        }

        if (!line_assembler.scan(line))
            return;
    }
}


void ConnectionImpl::request_header_flush()
{
    HeaderEnum header;
    if (!header.parse(request_header_name) || !server->is_request_header(header)) {
        std::cerr << "Unrecognized request header '"<<request_header_name<<":'" << std::endl;
    }
    else {
        std::pair<Headers::iterator, bool> r =
            request.headers.insert(make_pair(header, std::string()));
        if (r.second) {
            r.first->second = request_header_value;
        }
        else {
            if (!server->is_joinable_header(header))
                throw RequestException("Cannot concatenate multiple "
                                       "request headers '"+request_header_name+":'");
            r.first->second += ",";
            r.first->second += request_header_value;
        }
    }
    request_header_dirty = false;
}


void ConnectionImpl::activate_resource()
{
    response.reset();
    resource->activate(request, response);
    response.serialize();
    response_mode = response_mode_Head;
    resume_write();
}


void ErrorResource::activate(Request& req, Response& res) throw(RequestException)
{
    res.set_status(status);
    res.set_header(header_Content_Type, "text/html");
    std::string host = req.find_header(header_Host);
    if (host.empty())
        host = conn->server->get_host_string();
    std::string server = conn->server->get_server_string();
    std::ostringstream out;
    out <<
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
        "<html>\n"
        "<head>\n"
        "<title>"<<int(status)<<" "<<status<<"</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>"<<int(status)<<" "<<status<<"</h1>\n";
    if (!message.empty())
        out << "<p>"<<message<<"</p>\n";
    out <<
        "<hr>\n"
        "<address>"<<server<<" at "<<host<<"</address>\n"
        "</body>\n"
        "</html>\n";
    message = out.str();
    data = message.data();
    size = message.size();
    pos = 0;
    res.set_header(header_Content_Length, size);
}


void ResponseImpl::reset()
{
    protocol = protocol_HTTP_1_1;
    status   = status_200_OK;
    headers.clear();
    headers.insert(make_pair(header_Date,   conn->server->get_time().format_rfc_1123()));
    headers.insert(make_pair(header_Server, conn->server->get_server_string()));
}


void ResponseImpl::serialize()
{
    std::ostringstream out;
    out << protocol<<" "<<int(status)<<" "<<status<<"\r\n";
    typedef Headers::remem_order_iterator iterator;
    iterator begin(headers.remem_order_begin()), end(headers.remem_order_end());
    for (iterator i = begin; i != end; ++i)
        out << HeaderEnum(i->first)<<": "<<i->second<<"\r\n";
    out << "\r\n";
    headers.clear();
    str = out.str();
    data = str.data();
    size = str.size();
    pos = 0;
}


bool ResponseImpl::write(OutputStreamNew& out)
{
    pos += out.write(data+pos, size-pos);
    return pos == size;
}


void ResponseImpl::set_header(Header n, long v)
{
    headers.insert(make_pair(n, conn->value_printer.print(v)));
}

} // unnamed namespace



namespace archon {
namespace web {
namespace server {

std::unique_ptr<Server> make_default_server(int port)
{
    return std::make_unique<ServerImpl>(port);
}

} // namespace server
} // namespace web
} // namespace archon
