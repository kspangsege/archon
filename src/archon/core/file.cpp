// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <cstddef>
#include <cerrno>
#include <cstring>
#include <type_traits>
#include <limits>
#include <utility>
#include <algorithm>
#include <string>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/terminate.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/array_seeded_buffer.hpp>
#include <archon/core/filesystem.hpp>
#include <archon/core/platform_support.hpp>
#include <archon/core/file.hpp>


#if ARCHON_WINDOWS
#  if !defined NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <limits.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <termios.h>
#  include <sys/types.h>
#  include <sys/ioctl.h>
#  include <sys/stat.h>
#  include <sys/file.h> // BSD / Linux flock()
#  if defined _POSIX_VERSION && _POSIX_VERSION >= 200809L // POSIX.1-2008
#    define HAVE_POSIX_FUTIMENS 1
#  else
#    define HAVE_POSIX_FUTIMENS 0
#  endif
#  if !HAVE_POSIX_FUTIMENS
#    include <utime.h>
#  endif
#endif


using namespace archon;
using core::File;


bool File::try_open(FilesystemPathRef path, AccessMode access_mode, CreateMode create_mode, WriteMode write_mode,
                    std::error_code& ec) noexcept
{
    if (ARCHON_LIKELY(do_try_open(path, access_mode, create_mode, write_mode, ec))) {
        Info info = {};
        if (ARCHON_LIKELY(try_get_file_info(info, ec))) {
            if (ARCHON_LIKELY(!info.is_directory))
                return true;
            ec = make_error_code(std::errc::is_a_directory);
        }
    }
    return false;
}



bool File::try_read(core::Span<char> buffer, std::size_t& n, std::error_code& ec) noexcept
{
    core::Span<char> buffer_2 = buffer;
    std::size_t n_2 = 0;
  again:
    if (ARCHON_LIKELY(try_read_some(buffer_2, n_2, ec))) {
        ARCHON_ASSERT(n_2 <= buffer_2.size());
        if (ARCHON_LIKELY(n_2 != 0 && n_2 < buffer_2.size())) {
            buffer_2 = buffer_2.subspan(n_2);
            goto again;
        }
        n = std::size_t(buffer_2.data() + n_2 - buffer.data());
        return true;
    }
    n = std::size_t(buffer_2.data() - buffer.data());
    return false;
}



bool File::try_write(core::StringSpan<char> data, std::size_t& n, std::error_code& ec) noexcept
{
    core::StringSpan<char> data_2 = data;
    std::size_t n_2 = 0;
  again:
    if (ARCHON_LIKELY(try_write_some(data_2, n_2, ec))) {
        ARCHON_ASSERT(n_2 <= data_2.size());
        if (ARCHON_LIKELY(n_2 < data_2.size())) {
            data_2 = data_2.subspan(n_2);
            goto again;
        }
        n = data.size();
        return true;
    }
    n = std::size_t(data_2.data() - data.data());
    return false;
}



bool File::try_read_some_a(core::Span<char> buffer, std::size_t& n, bool& interrupted, std::error_code& ec) noexcept
{
#if ARCHON_WINDOWS

    DWORD n_2 = std::numeric_limits<DWORD>::max();
    if (ARCHON_LIKELY(core::int_less(buffer.size(), n_2)))
        n_2 = static_cast<DWORD>(buffer.size());
    DWORD ret = 0;
    if (ARCHON_LIKELY(ReadFile(m_handle, buffer.data(), n_2, &ret, 0))) {
        n = std::size_t(ret);
        ARCHON_ASSERT(n <= n_2);
        return true;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    interrupted = false;
    ec = core::make_system_error(int(err));
    return false;

#else // !ARCHON_WINDOWS

    // POSIX version

    // POSIX requires that chunk size is less than or equal to SSIZE_MAX
    std::size_t n_2 = std::min(buffer.size(), std::size_t(SSIZE_MAX));
    ssize_t ret = ::read(m_handle, buffer.data(), n_2);
    if (ARCHON_LIKELY(ret != -1)) {
        ARCHON_ASSERT(ret >= 0);
        n = std::size_t(ret);
        ARCHON_ASSERT(n <= n_2);
        return true;
    }
    int err = errno; // Eliminate any risk of clobbering
    if (ARCHON_LIKELY(err == EINTR)) {
        interrupted = true;
        return false;
    }
    interrupted = false;
    ec = core::make_system_error(err);
    return false;

#endif // !ARCHON_WINDOWS
}



bool File::try_write_some_a(core::StringSpan<char> data, std::size_t& n, bool& interrupted,
                            std::error_code& ec) noexcept
{
#if ARCHON_WINDOWS

    // Windows version

    DWORD n_2 = std::numeric_limits<DWORD>::max();
    if (ARCHON_LIKELY(core::int_less(data.size(), n_2)))
        n_2 = static_cast<DWORD>(data.size());
    DWORD ret = 0;
    if (ARCHON_LIKELY(WriteFile(m_handle, data.data(), n_2, &ret, 0))) {
        n = std::size_t(ret);
        ARCHON_ASSERT(n <= n_2);
        return true;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    interrupted = false;
    ec = core::make_system_error(err);
    return false;

#else // !ARCHON_WINDOWS

    // POSIX version

    // POSIX requires that chunk size is less than or equal to SSIZE_MAX
    std::size_t n_2 = std::min(data.size(), std::size_t(SSIZE_MAX));
    ssize_t ret = ::write(m_handle, data.data(), n_2);
    if (ARCHON_LIKELY(ret != -1)) {
        ARCHON_ASSERT(ret >= 0);
        n = std::size_t(ret);
        ARCHON_ASSERT(n <= n_2);
        return true;
    }
    int err = errno; // Eliminate any risk of clobbering
    if (ARCHON_LIKELY(err == EINTR)) {
        interrupted = true;
        return false;
    }
    interrupted = false;
    ec = core::make_system_error(err);
    return false;

#endif // !ARCHON_WINDOWS
}



bool File::try_seek(offset_type offset, Whence whence, offset_type& result, std::error_code& ec) noexcept
{
#if ARCHON_WINDOWS

    // Windows version

    DWORD whence_2 = FILE_BEGIN;
    switch (whence) {
        case Whence::set:
            break;
        case Whence::cur:
            whence_2 = FILE_CURRENT;
            break;
        case Whence::end:
            whence_2 = FILE_END;
            break;
    }

    LARGE_INTEGER offset_2, result_2;
    offset_2.QuadPart = offset;
    result_2.QuadPart = 0;

    if (ARCHON_LIKELY(SetFilePointerEx(m_handle, offset_2, &result_2, whence_2))) {
        ARCHON_ASSERT(result_2.QuadPart >= 0);
        ARCHON_ASSERT(core::can_int_cast<offset_type>(result_2.QuadPart));
        result = offset_type(result_2.QuadPart);
        return true;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    ec = core::make_system_error(int(err));
    return false;

#else // !ARCHON_WINDOWS

    // POSIX version

    int whence_2 = SEEK_SET;
    switch (whence) {
        case Whence::set:
            break;
        case Whence::cur:
            whence_2 = SEEK_CUR;
            break;
        case Whence::end:
            whence_2 = SEEK_END;
            break;
    }

    off_t ret = ::lseek(m_handle, offset, whence_2);
    if (ARCHON_LIKELY(ret != static_cast<off_t>(-1))) {
        ARCHON_ASSERT(ret >= 0);
        result = ret;
        return true;
    }
    int err = errno; // Eliminate any risk of clobbering
    ec = core::make_system_error(err);
    return false;

#endif
}



bool File::try_load(core::FilesystemPathRef path, std::string& data, std::error_code& ec)
{
    core::File file;
    if (ARCHON_LIKELY(file.try_open(path, AccessMode::read_only, CreateMode::never, WriteMode::normal, ec))) {
        core::ArraySeededBuffer<char, 512> buffer;
        std::size_t size = 0;
        std::size_t n = 0;
        std::size_t min_extra_capacity = 256;
      again:
        buffer.reserve_extra(min_extra_capacity, size); // Throws
        if (ARCHON_LIKELY(file.try_read(core::Span(buffer).subspan(size), n, ec))) {
            if (ARCHON_LIKELY(n > 0)) {
                ARCHON_ASSERT(n <= std::size_t(buffer.size() - size));
                size += n;
                goto again;
            }
            data = std::string(buffer.data(), size); // Throws
            return true;
        }
    }
    return false;
}



bool File::try_save(core::FilesystemPathRef path, core::StringSpan<char> data,
                    std::error_code& ec) noexcept
{
    core::File file;
    if (ARCHON_LIKELY(file.try_open(path, AccessMode::read_write, CreateMode::allow, WriteMode::trunc, ec))) {
        std::size_t n; // Dummy
        if (ARCHON_LIKELY(file.try_write(data, n, ec)))
            return true;
    }
    return false;
}



bool File::try_touch(core::FilesystemPathRef path, std::error_code& ec) noexcept
{
    File file;
    if (ARCHON_LIKELY(file.try_open(path, AccessMode::read_write, CreateMode::allow, WriteMode::normal, ec))) {
#if ARCHON_WINDOWS

        // Windows version

        SYSTEMTIME st;
        GetSystemTime(&st);
        FILETIME ft;
        if (ARCHON_LIKELY(SystemTimeToFileTime(&st, &ft))) {
            if (ARCHON_LIKELY(SetFileTime(file.m_handle, nullptr, nullptr, &ft)))
                return true;
        }
        else {
            bool system_time_to_file_time_failed = true;
            ARCHON_ASSERT(!system_time_to_file_time_failed);
        }
        DWORD err = GetLastError(); // Eliminate any risk of clobbering
        ec = core::make_system_error(int(err));

#elif HAVE_POSIX_FUTIMENS

        // POSIX.1-2008 version

        int ret = ::futimens(file.m_handle, nullptr);
        if (ARCHON_LIKELY(ret != -1))
            return true;
        int err = errno; // Eliminate any risk of clobbering
        ec = core::make_system_error(int(err));

#else // !ARCHON_WINDOWS && !HAVE_POSIX_FUTIMENS

        // POSIX.1-2001 version

        int ret = ::utime(path.c_str(), nullptr);
        if (ARCHON_LIKELY(ret != -1))
            return true;
        int err = errno; // Eliminate any risk of clobbering
        // If the file was not found, we assume that it was deleted after we created it, in
        // which case it should just remain deleted.
        if (ARCHON_LIKELY(err == ENOENT))
            return true;
        ec = core::make_system_error(int(err));

#endif // !ARCHON_WINDOWS && !HAVE_POSIX_FUTIMENS
    }
    return false;
}



bool File::try_get_file_info(Info& info, std::error_code& ec) noexcept
{
#if ARCHON_WINDOWS

    BY_HANDLE_FILE_INFORMATION info_2 = {};
    if (ARCHON_LIKELY(GetFileInformationByHandle(m_handle, &info_2))) {
        Info info_3 = {};
        info_3.is_directory = bool(info_2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        info = info_3;
        return true;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    ec = core::make_system_error(int(err));
    return false; // Failure

#else // !ARCHON_WINDOWS

    struct stat statbuf;
    int ret = ::fstat(m_handle, &statbuf);
    if (ARCHON_LIKELY(ret != -1)) {
        Info info_2 = {};
        info_2.is_directory = bool(S_ISDIR(statbuf.st_mode));
        info = info_2;
        return true;
    }
    int err = errno; // Eliminate any risk of clobbering
    ec = core::make_system_error(int(err));
    return false;

#endif // !ARCHON_WINDOWS
}



bool File::try_get_terminal_info(bool& is_term, TerminalInfo& info, std::error_code& ec) noexcept
{
#if ARCHON_WINDOWS

    // This scheme detects the standard Command Prompt (`cmd.exe`) as well as the Visual
    // Studio Debug Console and Developer Command Prompt. It does not detect the MinGW
    // terminal.
    CONSOLE_SCREEN_BUFFER_INFO info_2 = {};
    if (ARCHON_LIKELY(GetConsoleScreenBufferInfo(m_handle, &info_2))) {
        TerminalInfo info_3 = {};
        {
            auto width  = info_2.srWindow.Right - info_2.srWindow.Left;
            auto height = info_2.srWindow.Bottom - info_2.srWindow.Top;
            TerminalInfo::Size size;
            using type = int;
            static_assert(std::is_same_v<decltype(size.width), type>);
            static_assert(std::is_same_v<decltype(size.height), type>);
            int max = core::int_max<type>();
            static_assert(std::is_signed_v<type>);
            core::int_cast_clamp(width,  size.width,  -1, max - 1);
            core::int_cast_clamp(height, size.height, -1, max - 1);
            size.width  += 1;
            size.height += 1;
            info_3.size = size;
        }
        is_term = true;
        info = info_3;
        return true; // Success
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    if (err == ERROR_INVALID_HANDLE) {
        is_term = false;
        return true; // Success
    }
    ec = core::make_system_error(int(err));
    return false; // Failure

#else // !ARCHON_WINDOWS

    // UNIX version

    int ret = ::isatty(m_handle);
    if (ARCHON_LIKELY(ret == 1)) {
        TerminalInfo info_2 = {};
#  if 1
        winsize size = {};
        int ret_2 = ::ioctl(m_handle, TIOCGWINSZ, &size);
        if (ARCHON_LIKELY(ret_2 != -1)) {
            TerminalInfo::Size size_2;
            using type = int;
            static_assert(std::is_same_v<decltype(size_2.width), type>);
            static_assert(std::is_same_v<decltype(size_2.height), type>);
            int max = core::int_max<type>();
            core::int_cast_clamp(size.ws_col, size_2.width,  0, max);
            core::int_cast_clamp(size.ws_row, size_2.height, 0, max);
            info_2.size = size_2;
        }
        else {
            int err = errno; // Eliminate any risk of clobbering
            if (ARCHON_UNLIKELY(err != ENOTTY)) {
                ec = core::make_system_error(err);
                return false; // Failure
            }
        }
#  endif
        is_term = true;
        info = std::move(info_2);
    }
    else {
        is_term = false;
    }
    return true; // Success

#endif // !ARCHON_WINDOWS
}



bool File::do_try_open(FilesystemPathRef path, AccessMode access_mode, CreateMode create_mode, WriteMode write_mode,
                       std::error_code& ec) noexcept
{
#if ARCHON_WINDOWS

    // Windows version

    DWORD desired_access = GENERIC_READ;
    switch (access_mode) {
        case AccessMode::read_only:
            break;
        case AccessMode::read_write:
            if (write_mode ==  WriteMode::append) {
                desired_access = FILE_APPEND_DATA;
            }
            else {
                desired_access |= GENERIC_WRITE;
            }
            break;
    }
    DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD creation_disposition = 0;
    switch (create_mode) {
        case CreateMode::never:
            if (write_mode == WriteMode::trunc) {
                creation_disposition = TRUNCATE_EXISTING;
            }
            else {
                creation_disposition = OPEN_EXISTING;
            }
            break;
        case CreateMode::allow:
            if (write_mode == WriteMode::trunc) {
                creation_disposition = CREATE_ALWAYS;
            }
            else {
                creation_disposition = OPEN_ALWAYS;
            }
            break;
        case CreateMode::must:
            creation_disposition = CREATE_NEW;
            break;
    }
    HANDLE ret = CreateFile2(path.c_str(), desired_access, share_mode, creation_disposition, nullptr);
    if (ARCHON_LIKELY(ret != INVALID_HANDLE_VALUE)) {
        HANDLE handle = ret;
        adopt(handle);
        return true;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    ec = core::make_system_error(int(err));
    return false;

#else // !ARCHON_WINDOWS

    // POSIX version

    int flags = 0;
    switch (access_mode) {
        case AccessMode::read_only:
            flags = O_RDONLY;
            break;
        case AccessMode::read_write:
            flags = O_RDWR;
            break;
    }
    switch (create_mode) {
        case CreateMode::never:
            break;
        case CreateMode::allow:
            flags |= O_CREAT;
            break;
        case CreateMode::must:
            flags |= O_CREAT | O_EXCL;
            break;
    }
    switch (write_mode) {
        case WriteMode::normal:
            break;
        case WriteMode::trunc:
            flags |= O_TRUNC;
            break;
        case WriteMode::append:
            flags |= O_APPEND;
            break;
    }
    int ret = ::open(path.c_str(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (ARCHON_LIKELY(ret != -1)) {
        int handle = ret;
        adopt(handle);
        return true;
    }
    int err = errno; // Eliminate any risk of clobbering
    ec = core::make_system_error(err);
    return false;

#endif // !ARCHON_WINDOWS
}



void File::do_close() noexcept
{
    ARCHON_ASSERT(is_open());

#if ARCHON_WINDOWS

    // Windows version

    if (m_holds_lock)
        unlock();
    BOOL ret = CloseHandle(m_handle);
    if (ARCHON_LIKELY(ret))
        return;
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    std::error_code ec = core::make_system_error(int(err));
    ARCHON_TERMINATE_2("CloseHandle() failed", ec, ec.message());

#else // !ARCHON_WINDOWS

    // POSIX version

    int ret = ::close(m_handle);
    if (ARCHON_LIKELY(ret != -1))
        return;
    int err = errno; // Eliminate any risk of clobbering
    std::error_code ec = core::make_system_error(err);
    ARCHON_TERMINATE_2("close() failed", ec, ec.message());

#endif // !ARCHON_WINDOWS
}



bool File::do_lock(bool exclusive, bool nonblocking)
{
    ARCHON_ASSERT(is_open());

#if ARCHON_WINDOWS

    ARCHON_ASSERT(!m_holds_lock);

    // Under Windows, a file lock must be explicitely released before the file is closed. It
    // will eventually be released by the system, but there is no guarantees on the timing.

    DWORD flags = 0;
    if (exclusive)
        flags |= LOCKFILE_EXCLUSIVE_LOCK;
    if (nonblocking)
        flags |= LOCKFILE_FAIL_IMMEDIATELY;
    OVERLAPPED overlapped;
    std::memset(&overlapped, 0, sizeof overlapped);
    overlapped.Offset = 0;     // Just for clarity
    overlapped.OffsetHigh = 0; // Just for clarity
    BOOL ret = LockFileEx(m_handle, flags, 0, 1, 0, &overlapped);
    if (ARCHON_LIKELY(ret)) {
        m_holds_lock = true;
        return true;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    if (err == ERROR_LOCK_VIOLATION)
        return false;
    core::throw_system_error(int(err), "LockFileEx() failed"); // Throws

#else // !ARCHON_WINDOWS

    // BSD / Linux flock()

    // NOTE: It would probably have been more portable to use fcntl() based POSIX locks,
    // however these locks are not recursive within a single process, and since a second
    // attempt to acquire such a lock will always appear to succeed, one will easily suffer
    // the 'spurious unlocking issue'. It remains to be determined whether this also applies
    // across distinct threads inside a single process.
    //
    // To make matters worse, flock() may be a simple wrapper around fcntl() based locks on
    // some systems. This is bad news, because means that file locks cannot be relied on
    // 100%. Fortunately, on both Linux and Darwin, flock() does not suffer from this
    // 'spurious unlocking issue'.

    int operation = (exclusive ? LOCK_EX : LOCK_SH);
    if (nonblocking)
        operation |= LOCK_NB;
    for (;;) {
        int ret = ::flock(m_handle, operation);
        if (ARCHON_LIKELY(ret != -1))
            return true;
        if (ARCHON_LIKELY(errno == EINTR))
            continue;
        break;
    }
    if (errno == EWOULDBLOCK)
        return false;
    int err = errno; // Eliminate any risk of clobbering
    core::throw_system_error(err, "flock() failed"); // Throws

#endif // !ARCHON_WINDOWS
}



void File::do_unlock() noexcept
{
    ARCHON_ASSERT(is_open());

#if ARCHON_WINDOWS

    if (!m_holds_lock)
        return;
    OVERLAPPED overlapped;
    overlapped.hEvent = 0;
    overlapped.OffsetHigh = 0;
    overlapped.Offset = 0;
    overlapped.Pointer = 0;
    BOOL ret = UnlockFileEx(m_handle, 0, 1, 0, &overlapped);
    if (ARCHON_LIKELY(ret)) {
        m_holds_lock = false;
        return;
    }
    DWORD err = GetLastError(); // Eliminate any risk of clobbering
    std::error_code ec = core::make_system_error(int(err));
    ARCHON_TERMINATE_2("CloseHandle() failed", ec, ec.message());

#else // !ARCHON_WINDOWS

    // BSD / Linux flock()

    // The Linux man page for flock() does not state explicitely that unlocking is
    // idempotent, however, we will assume it since there is no mention of the error that
    // would be reported if a non-locked file were unlocked.
    for (;;) {
        int ret = ::flock(m_handle, LOCK_UN);
        if (ARCHON_LIKELY(ret != -1))
            return;
        if (ARCHON_LIKELY(errno == EINTR))
            continue;
        break;
    }
    int err = errno; // Eliminate any risk of clobbering
    std::error_code ec = core::make_system_error(err);
    ARCHON_TERMINATE_2("flock(fd, LOCK_UN) failed", ec, ec.message());

#endif // !ARCHON_WINDOWS
}



namespace {

#if ARCHON_WINDOWS

auto get_standard_stream_handle(int which) noexcept -> HANDLE
{
    DWORD which_2;
    switch (which) {
        case 0:
            which_2 = STD_INPUT_HANDLE;
            break;
        case 1:
            which_2 = STD_OUTPUT_HANDLE;
            break;
        case 2:
            which_2 = STD_ERROR_HANDLE;
            break;
        default:
            return nullptr;
    }
    HANDLE ret = GetStdHandle(which_2);
    if (ARCHON_LIKELY(ret != INVALID_HANDLE_VALUE))
        return ret;
    return nullptr;
}

#else // !ARCHON_WINDOWS

int get_standard_stream_handle(int which) noexcept
{
    switch (which) {
        case 0:
            return STDIN_FILENO;
        case 1:
            return STDOUT_FILENO;
        case 2:
            return STDERR_FILENO;
    }
    return -1;
}

#endif // !ARCHON_WINDOWS


} // unnamed namespace



File::StandardStreams::StandardStreams() noexcept
{
    bool no_implicit_close = true;
    stdin_.adopt(get_standard_stream_handle(0), no_implicit_close);
    stdout_.adopt(get_standard_stream_handle(1), no_implicit_close);
    stderr_.adopt(get_standard_stream_handle(2), no_implicit_close);
}
