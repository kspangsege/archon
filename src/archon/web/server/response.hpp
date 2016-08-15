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

#ifndef ARCHON_WEB_SERVER_RESPONSE_HPP
#define ARCHON_WEB_SERVER_RESPONSE_HPP

#include <archon/web/server/header.hpp>


namespace archon {
namespace web {
namespace Server {

enum Status {
    // Informational
    status_100_Continue                        = 100,
    status_101_Switching_Protocols             = 101,

    // Successful
    status_200_OK                              = 200,
    status_201_Created                         = 201,
    status_202_Accepted                        = 202,
    status_203_Non_Authoritative_Information   = 203,
    status_204_No_Content                      = 204,
    status_205_Reset_Content                   = 205,
    status_206_Partial_Content                 = 206,

    // Redirection
    status_300_Multiple_Choices                = 300,
    status_301_Moved_Permanently               = 301,
    status_302_Found                           = 302,
    status_303_See_Other                       = 303,
    status_304_Not_Modified                    = 304,
    status_305_Use_Proxy                       = 305,
    status_307_Temporary_Redirect              = 307,

    // Client error
    status_400_Bad_Request                     = 400,
    status_401_Unauthorized                    = 401,
    status_402_Payment_Required                = 402,
    status_403_Forbidden                       = 403,
    status_404_Not_Found                       = 404,
    status_405_Method_Not_Allowed              = 405,
    status_406_Not_Acceptable                  = 406,
    status_407_Proxy_Authentication_Required   = 407,
    status_408_Request_Timeout                 = 408,
    status_409_Conflict                        = 409,
    status_410_Gone                            = 410,
    status_411_Length_Required                 = 411,
    status_412_Precondition_Failed             = 412,
    status_413_Request_Entity_Too_Large        = 413,
    status_414_Request_URI_Too_Long            = 414,
    status_415_Unsupported_Media_Type          = 415,
    status_416_Requested_Range_Not_Satisfiable = 416,
    status_417_Expectation_Failed              = 417,

    // Server error
    status_500_Internal_Server_Error           = 500,
    status_501_Not_Implemented                 = 501,
    status_502_Bad_Gateway                     = 502,
    status_503_Service_Unavailable             = 503,
    status_504_Gateway_Timeout                 = 504,
    status_505_HTTP_VersionNot_Supported       = 505
};


class Response {
public:
    virtual void set_status(Status) = 0;

    virtual void set_header(Header name, std::string value) = 0;
    virtual void set_header(Header name, long value) = 0;

    virtual ~Response() {}
};


namespace _Impl {

struct StatusSpec {
    static core::EnumAssoc map[];
};

} // namespace _Impl


using StatusEnum = core::Enum<Status, _Impl::StatusSpec>;

} // namespace Server
} // namespace web
} // namespace archon

#endif // ARCHON_WEB_SERVER_RESPONSE_HPP
