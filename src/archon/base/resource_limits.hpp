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

#ifndef ARCHON_X_BASE_X_RESOURCE_LIMITS_HPP
#define ARCHON_X_BASE_X_RESOURCE_LIMITS_HPP

/// \file


namespace archon::base {


/// \brief System resources.
///
/// These are the system resources whose limits can potentially be inspected and
/// modified using \ref get_hard_rlimit(), \ref get_soft_rlimit(), and \ref
/// set_soft_rlimit(). A given resouce can be inspected and modified if \ref
/// system_has_rlimit() returns true for that resource.
///
enum class Resource {
    /// \brief Maximum size of core file.
    ///
    /// The maximum size, in bytes, of the core file produced when the memory
    /// image of this process is dumped. If the memory image is larger than the
    /// limit, the core file will not be created. Same as `RLIMIT_CORE` of
    /// POSIX.
    ///
    core_dump_size,

    /// \brief Maximum amount of CPU time.
    ///
    /// The maximum CPU time, in seconds, available to this process. If the
    /// limit is exceeded, the process will be killed. Same as `RLIMIT_CPU` of
    /// POSIX.
    ///
    cpu_time,

    /// \brief Maximum data segment size.
    ///
    /// The maximum size, in bytes, of the data segment of this process. If the
    /// limit is exceede, `std::malloc()` will fail with `errno` equal to
    /// `ENOMEM`. Same as `RLIMIT_DATA` of POSIX.
    ///
    data_segment_size,

    /// \brief Maximum file size.
    ///
    /// The maximum size, in bytes, of a file that is modified by this
    /// process. If the limit is exceede, the process will be killed. Same as
    /// `RLIMIT_FSIZE` of POSIX.
    ///
    file_size,

    /// \brief Maximum number of open files.
    ///
    /// One plus the maximum file descriptor value that can be opened by this
    /// process. Same as `RLIMIT_NOFILE` of POSIX.
    ///
    num_open_files,

    /// \brief Maximum stack size.
    ///
    /// The maximum size, in bytes, of the stack of the main thread of this
    /// process. If the limit is exceede, the process is
    /// killed. Same as `RLIMIT_STACK` of POSIX.
    ///
    stack_size,

    /// \brief Maximum amount of virtual memory.
    ///
    /// The maximum size, in bytes, of the process's virtual memory (address
    /// space). If the limit is exceeded due to heap allocation, `std::malloc()`
    /// will fail with `errno` equal to `ENOMEM`. If the limit is exceeded due
    /// to explicit memory mapping, `mmap()` will fail with `errno` equal to
    /// `ENOMEM`. If the limit is exceeded due to stack expansion, the process
    /// will be killed.  Same as `RLIMIT_AS` of POSIX.
    ///
    virtual_memory_size
};



/// \brief System support for resource limit management.
///
/// Determine wheter the system allows for inspection and modification of the
/// limits of the specified resource.
///
bool system_has_rlimit(Resource) noexcept;



/// \{
///
/// \brief Get or set system resouce limits.
///
/// These functions inspect or modify system resouce limits. The hard limit is
/// fixed, and works like a limit for the solft limit. A negative value means
/// 'unlimited' for both hard and soft limits, and both when getting and when
/// setting.
///
/// \sa \ref system_has_rlimit()
///
long get_hard_rlimit(Resource);
long get_soft_rlimit(Resource);
void set_soft_rlimit(Resource, long value);
/// \}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_RESOURCE_LIMITS_HPP
