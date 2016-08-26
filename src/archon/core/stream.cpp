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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <climits>
#include <limits>

#include <archon/core/atomic.hpp>
#include <archon/core/sys.hpp>
#include <archon/core/file.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/stream.hpp>


using namespace archon::core;

namespace {

class FileInputStream: public InputStream {
public:
    std::size_t read(char* b, std::size_t n) override
    {
        return sys::read(fildes, b, n);
    }

    FileInputStream(int fildes, bool must_close):
        fildes(fildes),
        must_close(must_close ? 1 : 0)
    {
    }

    ~FileInputStream() override
    {
        try {
            if (must_close)
                sys::close(fildes);
        }
        catch (...) {
        }
    }

    const int fildes;
    const bool must_close;
};


class FileOutputStream: public OutputStream {
public:
    void write(const char* b, std::size_t n) override
    {
        std::size_t m = 0;
        while (m < n)
            m += sys::write(fildes, b + m, n - m);
    }

    void flush() override
    {
        // Noop since there is no user space buffer involved.
    }

    FileOutputStream(int fildes, bool must_close):
        fildes(fildes),
        must_close(must_close ? 1 : 0)
    {
    }

    ~FileOutputStream() override
    {
        try {
            if (must_close)
                sys::close(fildes);
        }
        catch (...) {
        }
    }

    const int fildes;
    const bool must_close;
};

} // unnamed namespace



namespace archon {
namespace core {

std::unique_ptr<InputStream> make_stdin_stream(bool close)
{
    return std::make_unique<FileInputStream>(STDIN_FILENO, close); // Throws
}

std::unique_ptr<OutputStream> make_stdout_stream(bool close)
{
    return std::make_unique<FileOutputStream>(STDOUT_FILENO, close); // Throws
}

std::unique_ptr<OutputStream> make_stderr_stream(bool close)
{
    return std::make_unique<FileOutputStream>(STDERR_FILENO, close); // Throws
}


std::unique_ptr<InputStream> make_file_input_stream(int fildes, bool close)
{
    return std::make_unique<FileInputStream>(fildes, close); // Throws
}

std::unique_ptr<InputStream> make_file_input_stream(std::string file_name)
{
    int fildes = file::open(file_name); // Throws
    bool close = true;
    return std::make_unique<FileInputStream>(fildes, close); // Throws
}


std::unique_ptr<OutputStream> make_file_output_stream(int fildes, bool close)
{
    return std::make_unique<FileOutputStream>(fildes, close); // Throws
}

std::unique_ptr<OutputStream> make_file_output_stream(std::string file_name)
{
    int fildes = file::creat(file_name); // Throws
    bool close = true;
    return std::make_unique<FileOutputStream>(fildes, close); // Throws
}

} // namespace core
} // namespace archon
