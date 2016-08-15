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

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <sstream>

#include <archon/core/assert.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/logger.hpp>

using namespace std;
using namespace archon::core;

namespace {

class Flock {
public:
    Flock(int fd):
        m_fd(fd)
    {
        while (flock(m_fd, LOCK_EX) < 0) {
            if (errno == EINTR)
                continue;
            int errno_2 = errno;
            throw runtime_error("`flock()` failed while aquiring lock: "+sys::error(errno_2));
        }
    }
    ~Flock()
    {
        int ret = flock(m_fd, LOCK_UN);
        ARCHON_ASSERT(ret != -1);
    }

    int m_fd;
};

class FlockLogger: public Logger {
public:
    void log(const string& msg) ARCHON_OVERRIDE
    {
        string msg_2 = msg + "\n";

        Flock l(m_fd);

        const char* p = msg_2.data();
        const char* q = p + msg_2.size();
        while (p < q) {
            ssize_t n = write(m_fd, p, q-p);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                int errno_2 = errno;
                throw runtime_error("`write()` failed: "+sys::error(errno_2));
            }
            p += n;
        }
    }

    locale getloc() const  ARCHON_OVERRIDE
    {
        return m_locale;
    }

    FlockLogger(const locale& loc, int fd, bool close_in_dtor):
        m_locale(loc),
        m_fd(fd),
        m_close_in_dtor(close_in_dtor)
    {
    }

    ~FlockLogger()
    {
        if (m_close_in_dtor)
            close(m_fd);
    }

    locale m_locale;
    int m_fd;
    bool m_close_in_dtor;
};

class TimePidLogger: public Logger {
public:
    void log(const string& msg) ARCHON_OVERRIDE
    {
        // Create a timestamp

        locale loc = m_superlogger->getloc();
        ostringstream out;
        out.imbue(loc);

        {
            time_t time_1 = time(0);
            tm time_2;
            if (!localtime_r(&time_1, &time_2))
                throw runtime_error("`localtime_r()` failed");

            typedef time_put<char> time_put;
            use_facet<time_put>(loc).put(ostreambuf_iterator<char>(out), out, ' ', &time_2, 'c');
        }

        out << " [" << getpid() << "] " << msg;
        m_superlogger->log(out.str());
    }

    locale getloc() const  ARCHON_OVERRIDE
    {
        return m_superlogger->getloc();
    }

    TimePidLogger(UniquePtr<Logger> superlogger):
        m_superlogger(superlogger)
    {
    }

    UniquePtr<Logger> m_superlogger;
};

} // anonymous namespace


namespace archon {
namespace core {

Logger& Logger::get_default_logger()
{
    static FlockLogger logger(locale(), STDERR_FILENO, false);
    return logger;
}

UniquePtr<Logger> Logger::new_flock_logger(int fd, const locale& loc)
{
    UniquePtr<Logger> logger(new FlockLogger(loc, fd, false));
    return logger;
}

UniquePtr<Logger> Logger::new_flock_logger(const std::string& path, const locale& loc)
{
    int fd = open(path.c_str(), O_WRONLY|O_APPEND|O_CREAT|O_SYNC,
                  S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fd < 0) {
        int errno_2 = errno;
        throw runtime_error("`open()` failed for '"+path+"': "+sys::error(errno_2));
    }
    UniquePtr<Logger> l(new FlockLogger(loc, fd, true));
    return l;
}

UniquePtr<Logger> Logger::new_time_pid_logger(UniquePtr<Logger> superlogger)
{
    UniquePtr<Logger> logger(new TimePidLogger(superlogger));
    return logger;
}

} // namespace core
} // namespace archon
