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

#include <archon/web/server/response.hpp>


using namespace Archon::Core;


namespace Archon
{
  namespace Web
  {
    namespace Server
    {
      namespace _Impl
      {
        EnumAssoc StatusSpec::map[] =
        {
          // Informational
          { status_100_Continue,                        "Continue"                        },
          { status_101_Switching_Protocols,             "Switching Protocols"             },

          // Successful
          { status_200_OK,                              "OK"                              },
          { status_201_Created,                         "Created"                         },
          { status_202_Accepted,                        "Accepted"                        },
          { status_203_Non_Authoritative_Information,   "Non-Authoritative Information"   },
          { status_204_No_Content,                      "No Content"                      },
          { status_205_Reset_Content,                   "Reset Content"                   },
          { status_206_Partial_Content,                 "Partial Content"                 },

          // Redirection
          { status_300_Multiple_Choices,                "Multiple Choices"                },
          { status_301_Moved_Permanently,               "Moved Permanently"               },
          { status_302_Found,                           "Found"                           },
          { status_303_See_Other,                       "See Other"                       },
          { status_304_Not_Modified,                    "Not Modified"                    },
          { status_305_Use_Proxy,                       "Use Proxy"                       },
          { status_307_Temporary_Redirect,              "Temporary Redirect"              },

          // Client error
          { status_400_Bad_Request,                     "Bad Request"                     },
          { status_401_Unauthorized,                    "Unauthorized"                    },
          { status_402_Payment_Required,                "Payment Required"                },
          { status_403_Forbidden,                       "Forbidden"                       },
          { status_404_Not_Found,                       "Not Found"                       },
          { status_405_Method_Not_Allowed,              "Method Not Allowed"              },
          { status_406_Not_Acceptable,                  "Not Acceptable"                  },
          { status_407_Proxy_Authentication_Required,   "Proxy Authentication Required"   },
          { status_408_Request_Timeout,                 "Request Timeout"                 },
          { status_409_Conflict,                        "Conflict"                        },
          { status_410_Gone,                            "Gone"                            },
          { status_411_Length_Required,                 "Length Required"                 },
          { status_412_Precondition_Failed,             "Precondition Failed"             },
          { status_413_Request_Entity_Too_Large,        "Request Entity Too Large"        },
          { status_414_Request_URI_Too_Long,            "Request-URI Too Long"            },
          { status_415_Unsupported_Media_Type,          "Unsupported Media Type"          },
          { status_416_Requested_Range_Not_Satisfiable, "Requested Range Not Satisfiable" },
          { status_417_Expectation_Failed,              "Expectation Failed"              },

          // Server error
          { status_500_Internal_Server_Error,           "Internal Server Error"           },
          { status_501_Not_Implemented,                 "Not Implemented"                 },
          { status_502_Bad_Gateway,                     "Bad Gateway"                     },
          { status_503_Service_Unavailable,             "Service Unavailable"             },
          { status_504_Gateway_Timeout,                 "Gateway Timeout"                 },
          { status_505_HTTP_VersionNot_Supported,       "HTTP Version Not Supported"      },

          { 0, 0 }
        };
      }
    }
  }
}
