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

#ifndef ARCHON_WEB_SERVER_HEADER_HPP
#define ARCHON_WEB_SERVER_HEADER_HPP

#include <string>
#include <set>

#include <archon/core/enum.hpp>


namespace archon {
namespace web {
namespace Server {

enum Header {
    // General
    header_Cache_Control,
    header_Connection,
    header_Date,
    header_Pragma,
    header_Trailer,
    header_Transfer_Encoding,
    header_Upgrade,
    header_Via,
    header_Warning,

    // Request
    header_Accept,
    header_Accept_Charset,
    header_Accept_Encoding,
    header_Accept_Language,
    header_Authorization,
    header_Expect,
    header_From,
    header_Host,
    header_If_Match,
    header_If_Modified_Since,
    header_If_None_Match,
    header_If_Range,
    header_If_Unmodified_Since,
    header_Max_Forwards,
    header_Proxy_Authorization,
    header_Range,
    header_Referer,
    header_TE,
    header_User_Agent,

    // Response
    header_Accept_Ranges,
    header_Age,
    header_ETag,
    header_Location,
    header_Proxy_Authenticate,
    header_Retry_After,
    header_Server,
    header_Vary,
    header_WWW_Authenticate,

    // Entity
    header_Allow,
    header_Content_Encoding,
    header_Content_Language,
    header_Content_Length,
    header_Content_Location,
    header_Content_MD5,
    header_Content_Range,
    header_Content_Type,
    header_Expires,
    header_Last_Modified
};


void init_header_sets(std::set<Header>& general, std::set<Header>& request,
                      std::set<Header>& response, std::set<Header>& entity,
                      std::set<Header>& joinable);


namespace _impl {

struct HeaderSpec {
    static core::EnumAssoc map[];
};

} // namespace _impl


using HeaderEnum = core::Enum<Header, _impl::HeaderSpec, true>;

} // namespace Server
} // namespace web
} // namespace archon

#endif // ARCHON_WEB_SERVER_HEADER_HPP
