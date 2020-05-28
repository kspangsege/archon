// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#include <cerrno>
#include <utility>
#include <system_error>
#include <locale>
#include <sstream>
#include <vector>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/buffer.hpp>
#include <archon/base/platform_info.hpp>

#if ARCHON_WINDOWS
#  define NOMINMAX
#  include <windows.h>
#  include <VersionHelpers.h>
#else
#  include <unistd.h>
#  include <sys/utsname.h>
#endif


using namespace archon;
using namespace archon::base;


namespace {

#if ARCHON_WINDOWS

std::error_code make_system_error_code(DWORD err) noexcept
{
    return { int(err), std::system_category() };
}

#else // !ARCHON_WINDOWS

std::error_code make_system_error_code(int err) noexcept
{
    return { err, std::system_category() };
}

#endif // !ARCHON_WINDOWS

} // unnamed namespace



#if ARCHON_WINDOWS

void base::get_platform_info(PlatformInfo& info)
{
    const wchar_t* system = L"kernel32.dll";
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeExW(FILE_VER_GET_NEUTRAL, system, &dummy);
    if (size == 0) {
        DWORD err = GetLastError(); // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "GetFileVersionInfoSizeExW() failed for kernel32.dll");
    }
    Buffer<char> buffer(size); // Throws
    BOOL ret = GetFileVersionInfoExW(FILE_VER_GET_NEUTRAL, system, dummy, DWORD(buffer.size()),
                                     buffer.data());
    if (ARCHON_UNLIKELY(!ret)) {
        DWORD err = GetLastError(); // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "GetFileVersionInfoExW() failed for kernel32.dll");
    }
    void* ptr = nullptr;
    UINT size_2 = 0;
    BOOL ret_2 = VerQueryValueW(buffer.data(), L"\\", &ptr, &size_2);
    if (ARCHON_UNLIKELY(!ret)) {
        DWORD err = GetLastError(); // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "VerQueryValueW() failed");
    }
    ARCHON_ASSERT(size_2 >= sizeof (VS_FIXEDFILEINFO));
    ARCHON_ASSERT(ptr);
    const VS_FIXEDFILEINFO& info_2 = *static_cast<const VS_FIXEDFILEINFO*>(ptr);
    std::ostringstream out; // Throws
    out.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    out.imbue(std::locale::classic()); // Throws
    out << HIWORD(info_2.dwFileVersionMS) << "."
        "" << LOWORD(info_2.dwFileVersionMS) << "."
        "" << HIWORD(info_2.dwFileVersionLS) << "."
        "" << LOWORD(info_2.dwFileVersionLS); // Throws


    PlatformInfo info_3;
    info_3.sysname = "Win32"; // Throws (copy)
    info_3.osname = "Windows"; // Throws (copy)
    if (IsWindowsServer())
        info_3.osname += " Server"; // Throws
    info_3.version = out.str(); // Throws
    info_3.release = "unknown"; // Throws (copy)

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    switch (sysinfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_ARM:
            info_3.machine = "arm"; // Throws (copy)
            break;
        case PROCESSOR_ARCHITECTURE_ARM64:
            info_3.machine = "arm64"; // Throws (copy)
            break;
        case PROCESSOR_ARCHITECTURE_INTEL:
            info_3.machine = "x86"; // Throws (copy)
            break;
        case PROCESSOR_ARCHITECTURE_AMD64:
            info_3.machine = "x86_64"; // Throws (copy)
            break;
        default:
            out.str({});
            out << "unknown-" << sysinfo.wProcessorArchitecture; // Throws
            info_3.machine = out.str(); // Throws
            break;
    }

    info = std::move(info_3);
}

#else // !ARCHON_WINDOWS

void base::get_platform_info(PlatformInfo& info)
{
    ::utsname info_2;
    int ret = ::uname(&info_2);
    if (ARCHON_UNLIKELY(ret == -1)) {
        int err = errno; // Eliminate any risk of clobbering
        std::error_code ec = make_system_error_code(err);
        throw std::system_error(ec, "uname() failed");
    }
    PlatformInfo info_3;

#if ARCHON_MACOS
  info_3.osname  = "macOS"; // Throws (copy)
#elif ARCHON_IOS
  info_3.osname = "iOS"; // Throws (copy)
#elif ARCHON_WATCHOS
  info_3.osname  = "watchOS"; // Throws (copy)
#elif ARCHON_TVOS
  info_3.osname  = "tvOS"; // Throws (copy)
#elif ARCHON_APPLE
  info_3.osname  = "Apple"; // Throws (copy)
#elif ARCHON_ANDROID
    info_3.osname  = "Android"; // Throws (copy)
#elif defined __FreeBSD__
    info_3.osname  = "FreeBSD"; // Throws (copy)
#elif defined __NetBSD__
    info_3.osname  = "NetBSD"; // Throws (copy)
#elif defined __OpenBSD__
    info_3.osname  = "OpenBSD"; // Throws (copy)
#elif __linux__
    info_3.osname  = "Linux"; // Throws (copy)
#elif defined _POSIX_VERSION
    info_3.osname  = "POSIX"; // Throws (copy)
#elif __unix__
    info_3.osname  = "Unix"; // Throws (copy)
#else
    info_3.osname  = "unknown"; // Throws (copy)
#endif

    info_3.sysname = info_2.sysname; // Throws (copy)
    info_3.release = info_2.release; // Throws (copy)
    info_3.version = info_2.version; // Throws (copy)
    info_3.machine = info_2.machine; // Throws (copy)

    info = std::move(info_3);
}

#endif // !ARCHON_WINDOWS
