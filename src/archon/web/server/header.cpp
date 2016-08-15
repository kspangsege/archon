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

#include <archon/web/server/header.hpp>


using namespace std;
using namespace archon::core;


namespace archon
{
  namespace Web
  {
    namespace Server
    {
      namespace _Impl
      {
        EnumAssoc HeaderSpec::map[] =
        {
          // General
          { header_Cache_Control,       "Cache-Control"        },
          { header_Connection,          "Connection"           },
          { header_Date,                "Date"                 },
          { header_Pragma,              "Pragma"               },
          { header_Trailer,             "Trailer"              },
          { header_Transfer_Encoding,   "Transfer-Encoding"    },
          { header_Upgrade,             "Upgrade"              },
          { header_Via,                 "Via"                  },
          { header_Warning,             "Warning"              },

          // Request
          { header_Accept,              "Accept"               },
          { header_Accept_Charset,      "Accept-Charset"       },
          { header_Accept_Encoding,     "Accept-Encoding"      },
          { header_Accept_Language,     "Accept-Language"      },
          { header_Authorization,       "Authorization"        },
          { header_Expect,              "Expect"               },
          { header_From,                "From"                 },
          { header_Host,                "Host"                 },
          { header_If_Match,            "If-Match"             },
          { header_If_Modified_Since,   "If-Modified-Since"    },
          { header_If_None_Match,       "If-None-Match"        },
          { header_If_Range,            "If-Range"             },
          { header_If_Unmodified_Since, "If-Unmodified-Since"  },
          { header_Max_Forwards,        "Max-Forwards"         },
          { header_Proxy_Authorization, "Proxy-Authorization"  },
          { header_Range,               "Range"                },
          { header_Referer,             "Referer"              },
          { header_TE,                  "TE"                   },
          { header_User_Agent,          "User-Agent"           },

          // Response
          { header_Accept_Ranges,       "Accept-Ranges"        },
          { header_Age,                 "Age"                  },
          { header_ETag,                "ETag"                 },
          { header_Location,            "Location"             },
          { header_Proxy_Authenticate,  "Proxy-Authenticate"   },
          { header_Retry_After,         "Retry-After"          },
          { header_Server,              "Server"               },
          { header_Vary,                "Vary"                 },
          { header_WWW_Authenticate,    "WWW-Authenticate"     },

          // Entity
          { header_Allow,               "Allow"                },
          { header_Content_Encoding,    "Content-Encoding"     },
          { header_Content_Language,    "Content-Language"     },
          { header_Content_Length,      "Content-Length"       },
          { header_Content_Location,    "Content-Location"     },
          { header_Content_MD5,         "Content-MD5"          },
          { header_Content_Range,       "Content-Range"        },
          { header_Content_Type,        "Content-Type"         },
          { header_Expires,             "Expires"              },
          { header_Last_Modified,       "Last-Modified"        },

          { 0, 0 }
        };
      }


      void init_header_sets(set<Header> &general, set<Header> &request,
                            set<Header> &response, set<Header> &entity,
                            set<Header> &joinable)
      {
        general.insert(header_Cache_Control);
        general.insert(header_Connection);
        general.insert(header_Date);
        general.insert(header_Pragma);
        general.insert(header_Trailer);
        general.insert(header_Transfer_Encoding);
        general.insert(header_Upgrade);
        general.insert(header_Via);
        general.insert(header_Warning);

        request.insert(header_Accept);
        request.insert(header_Accept_Charset);
        request.insert(header_Accept_Encoding);
        request.insert(header_Accept_Language);
        request.insert(header_Authorization);
        request.insert(header_Expect);
        request.insert(header_From);
        request.insert(header_Host);
        request.insert(header_If_Match);
        request.insert(header_If_Modified_Since);
        request.insert(header_If_None_Match);
        request.insert(header_If_Range);
        request.insert(header_If_Unmodified_Since);
        request.insert(header_Max_Forwards);
        request.insert(header_Proxy_Authorization);
        request.insert(header_Range);
        request.insert(header_Referer);
        request.insert(header_TE);
        request.insert(header_User_Agent);

        response.insert(header_Accept_Ranges);
        response.insert(header_Age);
        response.insert(header_ETag);
        response.insert(header_Location);
        response.insert(header_Proxy_Authenticate);
        response.insert(header_Retry_After);
        response.insert(header_Server);
        response.insert(header_Vary);
        response.insert(header_WWW_Authenticate);

        entity.insert(header_Allow);
        entity.insert(header_Content_Encoding);
        entity.insert(header_Content_Language);
        entity.insert(header_Content_Length);
        entity.insert(header_Content_Location);
        entity.insert(header_Content_MD5);
        entity.insert(header_Content_Range);
        entity.insert(header_Content_Type);
        entity.insert(header_Expires);
        entity.insert(header_Last_Modified);

        joinable.insert(header_Accept);
        joinable.insert(header_Accept_Charset);
        joinable.insert(header_Accept_Encoding);
        joinable.insert(header_Accept_Language);
        joinable.insert(header_Allow);
        joinable.insert(header_Cache_Control);
        joinable.insert(header_Content_Encoding);
        joinable.insert(header_Content_Language);
        joinable.insert(header_Expect);
        joinable.insert(header_Pragma);
        joinable.insert(header_Proxy_Authenticate);
        joinable.insert(header_TE);
        joinable.insert(header_Trailer);
        joinable.insert(header_Transfer_Encoding);
        joinable.insert(header_Upgrade);
        joinable.insert(header_Via);
        joinable.insert(header_Warning);
        joinable.insert(header_WWW_Authenticate);
      }
    }
  }
}
