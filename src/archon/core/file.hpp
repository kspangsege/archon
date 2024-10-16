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

#ifndef ARCHON_X_CORE_X_FILE_HPP
#define ARCHON_X_CORE_X_FILE_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <filesystem>
#include <system_error>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/string_span.hpp>
#include <archon/core/filesystem.hpp>

#if !ARCHON_WINDOWS
#  include <sys/stat.h> // off_t
#endif


namespace archon::core {


/// \brief Encapsulation of file descriptor.
///
/// A file object generally represents an open file. On a POSIX platform, a file object
/// contains a file descriptor, and it offers an API that closely resembles the API offered
/// by POSIX for file descriptors (`read()`, `write()`, etc.). This class does not introduce
/// any buffering for read or write operations.
///
/// A file object is considered *nonempty* when it refers to an open file (or other
/// file-like entity), and *empty* when it does not (\ref is_open()).
///
/// This class does not perform any translation between character representations, nor does
/// it translate newline characters on the Windows platform. See \ref core::GenericTextFile
/// for an alternative that does offer such things.
///
class File {
public:
    /// \brief Major access modes.
    ///
    /// These are the major modes in which a file can be opened, and they can be used with
    /// \ref open(core::FilesystemPathRef, Mode). More fine-grained control is available
    /// using \ref AccessMode, \ref CreateMode, and \ref WriteMode.
    ///
    /// Here is a useful correspondence table:
    ///
    /// | Mode     | `AccessMode` | `CreateMode` | `WriteMode` | `fopen()` in `<cstdio>`
    /// |----------|--------------|--------------|-------------|-------------------------
    /// | `read`   | `read_only`  | `never`      | `normal`    | `"rb"`
    /// | `update` | `read_write` | `never`      | `normal`    | `"rb+"`
    /// | `write`  | `read_write` | `allow`      | `trunc`     | `"wb+"`
    /// | `append` | `read_write` | `allow`      | `append`    | `"ab+"`
    ///
    enum class Mode {
        read,   ///< Read only. Writing not allowed.
        update, ///< Read and write preexisting file.
        write,  ///< Read and write after truncation.
        append, ///< Read and write in append mode.
    };

    /// \{
    ///
    /// \brief Standard streams.
    ///
    /// These functions return files that represent the standard streams, STDIN, STDOUT, and
    /// STDERR. Use \ref is_terminal() to check whether they are associated with a terminal.
    ///
    static auto get_cin() noexcept  -> File&;
    static auto get_cout() noexcept -> File&;
    static auto get_cerr() noexcept -> File&;
    /// \}

    /// \brief Open the specified file.
    ///
    /// Open the file at the specified path in the filesystem and make this file object
    /// refer to it. This is equivalent to calling \ref open(core::FilesystemPathRef, Mode)
    /// on a default constructed file object.
    ///
    explicit File(core::FilesystemPathRef, Mode = Mode::read);

    /// \brief Create a empty file object.
    ///
    /// Create an empty file object, that is, a file object that does not refer to anything.
    ///
    File() noexcept = default;

    /// \brief Open the specified file.
    ///
    /// Open the file at the specified path in the filesystem and make this file object
    /// refer to it. If this file object was already referring to an open file (\ref
    /// is_open()) that file is first closed.
    ///
    /// This function has the same effect as \ref try_open(core::FilesystemPathRef, Mode,
    /// std::error_code&) except that it throws an exception of type `std::system_error` on
    /// failure.
    ///
    void open(core::FilesystemPathRef, Mode = Mode::read);

    /// \brief Close the file.
    ///
    /// If the file object is currently referring to an open file, close that file now. On
    /// POSIX systems, this corresponds to closing the contained file descriptor. This puts
    /// the file object into the empty state (\ref is_open()).
    ///
    /// If the file object is already empty, this function has no effect (idempotency).
    ///
    void close() noexcept;

    /// \{
    ///
    /// \brief Whether file object is nonempty.
    ///
    /// These functions return `true` when , and only when the file object is nonempty.
    ///
    /// A *nonempty file object* is a file object that refers to an open file, or file-like
    /// entity, as opposed to one that does not refer to anything.
    ///
    /// A default constructed file object is empty.
    ///
    bool is_open() const noexcept;
    explicit operator bool() const noexcept;
    /// \}

    /// \brief Available access modes.
    ///
    /// \sa \ref Mode.
    ///
    enum class AccessMode {
        read_only,  ///< Read only. Writing not allowed.
        read_write, ///< Read and write allowed.
    };

    /// \brief Available file creation modes.
    ///
    /// \sa \ref Mode.
    ///
    enum class CreateMode {
        never, ///< Fail if the file does not already exist.
        allow, ///< Create the file if it does not already exist.
        must,  ///< Fail if the file already exists.
    };

    /// \brief Available writing modes.
    ///
    /// \sa \ref Mode.
    ///
    enum class WriteMode {
        normal, ///< Default mode.
        trunc,  ///< Truncate the file if it already exists.
        append, ///< Move to end of file before each write.
    };

    /// \brief Flexible way of opening a file.
    ///
    /// This function has the same effect as \ref try_open(core::FilesystemPathRef,
    /// AccessMode, CreateMode, WriteMode, std::error_code&) except that it throws an
    /// exception of type `std::system_error` on failure.
    ///
    /// \sa \ref open(core::FilesystemPathRef, Mode).
    ///
    void open(core::FilesystemPathRef, AccessMode, CreateMode, WriteMode);

    /// \brief Read a chunk of data.
    ///
    /// This function reads successive bytes from the file or stream associated with this
    /// file object, and places them in the specified buffer. Reading ends when the buffer
    /// is full or the end of input is reached.
    ///
    /// This function has the same effect as \ref try_read() except that, on success, it
    /// returns the number of bytes placed in the buffer, and, on failure, it throws an
    /// exception of type `std::system_error`.
    ///
    /// Note that on failure, some bytes may have been read from the file and placed into
    /// the specified buffer. Consider using \ref try_read() if you need to know how many
    /// bytes were read, even in the event of a failure.
    ///
    auto read(core::Span<char> buffer) -> std::size_t;

    /// \brief Write a chunk of data.
    ///
    /// This function writes all of the specified data to the file or stream associated with
    /// this file object.
    ///
    /// This function has the same effect as \ref try_write() except that, on failure, it
    /// throws an exception of type `std::system_error`.
    ///
    /// Note that on failure, some of the specified bytes may have been written to the
    /// file. Consider using \ref try_write() if you need to know how many bytes were
    /// written, even in the event of a failure.
    ///
    void write(core::StringSpan<char> data);

    /// \brief Read at least one byte.
    ///
    /// This function reads successive bytes from the file or stream associated with this
    /// file object, and places them in the specified buffer. Reading ends when at least one
    /// byte has been read and further reading would require that the calling thread is
    /// blocked, or when the specified buffer has been filled, or when the end of input is
    /// reached.
    ///
    /// This function has the same effect as \ref try_read_some() except that, on success,
    /// it returns the number of bytes placed in the buffer, and, on failure, it throws an
    /// exception of type `std::system_error`.
    ///
    auto read_some(core::Span<char> buffer) -> std::size_t;

    /// \brief Write at least one byte.
    ///
    /// This function writes some, or all of the specified data to the file or stream
    /// associated with this file object. Writing ends when at least one byte has been
    /// written and further writing would require that the calling thread is blocked, or
    /// when all of the specified data has been written.
    ///
    /// This function has the same effect as \ref try_write_some() except that, on success,
    /// it returns the number of written bytes, and, on failure, it throws an exception of
    /// type `std::system_error`.
    ///
    auto write_some(core::StringSpan<char> data) -> std::size_t;

#if ARCHON_WINDOWS
    using offset_type = std::int_fast64_t;
#else
    using offset_type = off_t;
#endif

    /// \brief Possible ways to specify file pointer position.
    ///
    /// These are the possible ways in which one can specify a new file pointer position
    /// when calling \ref seek().
    ///
    enum class Whence {
        set, ///< Specified offset is relative to beginning of file.
        cur, ///< Specified offset is relative to current position.
        end  ///< Specified offset is relative to end of file.
    };

    /// \brief Get current position of file pointer.
    ///
    /// This function returns the current position of the file pointer (see \ref
    /// try_seek()). The position is returned as an offset from the beginning of the
    /// file. This function is a shorthand for `seek(0, Whence::cur)`.
    ///
    auto tell() -> offset_type;

    /// \brief Move file pointer.
    ///
    /// This function moves the file pointer to the specified position. The new position is
    /// specified in terms of an offset, which is interpreted according to the specified
    /// "whence" value (see \ref Whence).
    ///
    /// This function has the same effect as \ref try_seek() except that, on success, the
    /// resulting position is returned, and, on failure, an exception of type
    /// `std::system_error` is thrown.
    ///
    auto seek(offset_type, Whence = Whence::set) -> offset_type;

    /// \{
    ///
    /// \brief Try to open a file.
    ///
    /// Specifying \ref AccessMode::read_only together with a create mode that is not \ref
    /// CreateMode::never, or together with a write mode that is not \ref WriteMode::normal,
    /// results in undefined behavior. Specifying \ref WriteMode::trunc together with \ref
    /// CreateMode::must results in undefined behavior.
    ///
    /// On success, these functions return `true`, and leaves \p ec untouched. On failure,
    /// they return `false` after setting \p ec to an appropriate error code.
    ///
    [[nodiscard]] bool try_open(core::FilesystemPathRef, Mode, std::error_code& ec) noexcept;
    [[nodiscard]] bool try_open(core::FilesystemPathRef, AccessMode, CreateMode, WriteMode,
                                std::error_code& ec) noexcept;
    /// \}

    /// \brief Try to read a chunk of data.
    ///
    /// This function reads successive bytes from the file or stream associated with this
    /// file object, and places them in the specified buffer. Reading ends when the buffer
    /// is full or the end of input is reached.
    ///
    /// This function has the same effect as \ref try_read_some() except that it continues
    /// reading until the buffer is full or the end of input is reached.
    ///
    /// On success, if \p n is less than `buffer.size()`, it means that the end of input has
    /// been reached. On failure, \p n will always be less than `buffer.size()` (provided
    /// that `buffer.size()` is greater than zero), and it will indicate how many bytes were
    /// read, and placed into the buffer before the failure occurred.
    ///
    /// On success, \p ec is left untouched.
    ///
    [[nodiscard]] bool try_read(core::Span<char> buffer, std::size_t& n, std::error_code& ec) noexcept;

    /// \brief Try to write a chunk of data.
    ///
    /// This function writes all of the specified data to the file or stream associated with
    /// this file object.
    ///
    /// This function has the same effect as \ref try_write_some() except that it continues
    /// writing until all of the specified data has been written.
    ///
    /// On success, \p n will always be equal to `data.size()`. On failure, \p n will always
    /// be less than `data.size()` (provided that `data.size()` is greater than zero), and
    /// it will indicate how many bytes were written before the failure occurred.
    ///
    /// On success, \p ec is left untouched.
    ///
    [[nodiscard]] bool try_write(core::StringSpan<char> data, std::size_t& n, std::error_code& ec) noexcept;

    /// \brief Try to read at least one byte.
    ///
    /// This function reads successive bytes from the file or stream associated with this
    /// file object, and places them in the specified buffer. Reading ends when at least one
    /// byte has been read and further reading would require that the calling thread is
    /// blocked, or when the specified buffer has been filled, or when the end of input is
    /// reached.
    ///
    /// This function has the same effect as \ref try_read_some_a() except that, on
    /// interruption due to reception of a system signal, reading is automatically resumed.
    ///
    /// On success, if \p n is zero and the size of the specified buffer is greater than
    /// zero, it means that the end of input has been reached.
    ///
    /// On success, \p ec is left untouched. On failure \p n is left untouched, and no bytes
    /// will have been read from the stream.
    ///
    [[nodiscard]] bool try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec) noexcept;

    /// \brief Try to write at least one byte.
    ///
    /// This function writes some, or all of the specified data to the file or stream
    /// associated with this file object. Writing ends when at least one byte has been
    /// written and further writing would require that the calling thread is blocked, or
    /// when all of the specified data has been written.
    ///
    /// This function has the same effect as \ref try_write_some_a() except that, on
    /// interruption due to reception of a system signal, writing is automatically resumed.
    ///
    /// On success, \p n will always be greater than zero provided `data.size()` is greater
    /// than zero.
    ///
    /// On success, \p ec is left untouched. On failure \p n is left untouched, and no bytes
    /// will have been written to the stream.
    ///
    [[nodiscard]] bool try_write_some(core::StringSpan<char> data, std::size_t& n, std::error_code& ec) noexcept;

    /// \brief Try to read at least one byte.
    ///
    /// This function reads successive bytes from the file or stream associated with this
    /// file object, and places them in the specified buffer.
    ///
    /// Reading ends when at least one byte has been read and further reading would require
    /// that the calling thread is blocked, or when the specified buffer has been filled, or
    /// when the end of input is reached.
    ///
    /// Reading begins at the current position of the file pointer, which will generally be
    /// the position at which the previous read (or write) operation ended, but see \ref
    /// seek() for a way to change that.
    ///
    /// On success, this function sets \p n to the number of bytes placed in the specified
    /// buffer, and then it returns `true`. On failure, it sets \p interrupted to `false`
    /// and \p ec to an error code that reflects the cause of the failure, and then it
    /// returns `false`.
    ///
    /// On POSIX systems, if the calling thread was blocked waiting for the opportunity to
    /// read at least one byte, and the wait was interrupted due to reception of a system
    /// signal, this function sets \p interrupted to `true` and then returns `false`.
    ///
    /// On success, if \p n is zero and the size of the specified buffer is greater than
    /// zero, it means that the end of input has been reached.
    ///
    /// On success, \p interrupted and \p ec are left untouched. On failure \p n is left
    /// untouched, and no bytes will have been read from the stream. On interruption, \p n
    /// and \p ec are left untouched, and no bytes will have been read from the stream.
    ///
    /// If the file object is empty (see \ref is_open()), this function fails with an
    /// unspecified error code.
    ///
    [[nodiscard]] bool try_read_some_a(core::Span<char> buffer, std::size_t& n, bool& interrupted,
                                       std::error_code& ec) noexcept;

    /// \brief Try to write at least one byte.
    ///
    /// This function writes some, or all of the specified data to the file or stream
    /// associated with this file object.
    ///
    /// Writing ends when at least one byte has been written and further writing would
    /// require that the calling thread is blocked, or when all of the specified data has
    /// been written.
    ///
    /// Writing begins at the current position of the file pointer, which will generally be
    /// the position at which the previous write (or read) operation ended, but see \ref
    /// seek() for a way to change that.
    ///
    /// On success, this function sets \p n to the number of bytes written, and then returns
    /// `true`. On failure, it sets \p interrupted to `false` and \p ec to an error code
    /// that reflects the cause of the failure, and then it returns `false`.
    ///
    /// On POSIX systems, if the calling thread was blocked waiting for the opportunity to
    /// write at least one byte, and the wait was interrupted due to reception of a system
    /// signal, this function sets \p interrupted to `true` and then returns `false`.
    ///
    /// On success, \p n will always be greater than zero provided `data.size()` is greater
    /// than zero.
    ///
    /// On success, \p interrupted and \p ec are left untouched. On failure \p n is left
    /// untouched, and no bytes will have been written to the stream. On interruption, \p n
    /// and \p ec are left untouched, and no bytes will have been written to the stream.
    ///
    /// If the referenced file was opened in read-only mode, expect this function to
    /// fail. The produced error code is unspecified.
    ///
    /// If the file object is empty (see \ref is_open()), this function fails with an
    /// unspecified error code.
    ///
    [[nodiscard]] bool try_write_some_a(core::StringSpan<char> data, std::size_t& n, bool& interrupted,
                                        std::error_code& ec) noexcept;

    /// \brief Try to move file pointer.
    ///
    /// This function tries to move the file pointer to the specified position. On success,
    /// this function returns `true` after assigning the resulting position to \p result as
    /// an offset relative to the beginning of the file. On failure, it returns `false`
    /// after setting \p ec to an error code that reflects the cause of the failure.
    ///
    /// The new position is specified in terms of an offset, which is interpreted according
    /// to the specified value for \p whence (see \ref Whence). The absolute position
    /// (offset from beginning of file) of the file pointer determines where the next byte
    /// will be read from, or written to. The file pointer is automatically advanced during
    /// each read or write operation. The file pointer is associated with the open file
    /// descriptor contained within this file object, so, under normal circumstances, when a
    /// file is opened twice, there will be two independent file pointers.
    ///
    /// On a POSIX platform, this function is implemented in terms of the system-level
    /// function `lseek()`.
    ///
    [[nodiscard]] bool try_seek(offset_type offset, Whence whence, offset_type& result, std::error_code& ec) noexcept;

    /// \brief Place exclusive lock on file.
    ///
    /// Place an exclusive lock on this file. This blocks the caller until all other locks
    /// have been released.
    ///
    /// Locks acquired on the same underlying file, but via distinct file objects, have
    /// fully recursive behavior, even if they are acquired in the same process (or thread).
    ///
    /// If the file object is empty (see \ref is_open()), this function fails with an
    /// unspecified error code.
    ///
    /// Calling this function on a file that is already locked via the same file object
    /// (i.e., same file descriptor), has undefined behavior.
    ///
    void lock_exclusive();

    /// \brief Acquire shared lock on file.
    ///
    /// Acquire a shared lock on this file. This blocks the caller until all other exclusive
    /// locks have been released.
    ///
    /// Locks acquired on the same underlying file, but via distinct file objects, have
    /// fully recursive behavior, even if they are acquired in the same process (or thread).
    ///
    /// If the file object is empty (see \ref is_open()), this function fails with an
    /// unspecified error code.
    ///
    /// Calling this function on a file that is already locked via the same file object
    /// (i.e., same file descriptor), has undefined behavior.
    ///
    void lock_shared();

    /// \brief Try to place exclusive lock on file.
    ///
    /// This is a nonblocking version of \ref lock_exclusive(). It returns true if, and only
    /// if it succeeds.
    ///
    [[nodiscard]] bool nb_lock_exclusive();

    /// \brief Try to acquire shared lock on file.
    ///
    /// This is a nonblocking version of \ref lock_shared(). It returns true if, and only if
    /// it succeeds.
    ///
    [[nodiscard]] bool nb_lock_shared();

    /// \brief Relinquish any held lock.
    ///
    /// If an exclusive or shared lock was previously acquired via this file object,
    /// releases now. If no lock is currently held by the file object, or if the file object
    /// is empty, this function has no effect (idempotency).
    ///
    void unlock() noexcept;

    /// \brief Load contents of file.
    ///
    /// This function loads the contents of the specified file, and returns it as a string.
    ///
    static auto load(core::FilesystemPathRef) -> std::string;

    /// \brief Save data to file.
    ///
    /// This function saves the specified data to a file at the specified path. If the file
    /// already exists, it will be truncated (\ref WriteMode::trunc) before the new data is
    /// written.
    ///
    static void save(core::FilesystemPathRef, core::StringSpan<char> data);

    /// \brief Ensure existence of file and mark it as modified now
    ///
    /// This function has the same effect as \ref try_touch() except that it throws an
    /// exception of type `std::system_error` on failure.
    ///
    static void touch(core::FilesystemPathRef);

    /// \brief Try to load contents of file.
    ///
    /// This function tries to load the contents of the specified file.
    ///
    /// On success, it assigns the loaded data to \p data and returns `true`. On failure, it
    /// sets \p ec to an error code that reflects the cause of the failure and returns
    /// `false`.
    ///
    [[nodiscard]] static bool try_load(core::FilesystemPathRef, std::string& data, std::error_code& ec);

    /// \brief Try to save data to file.
    ///
    /// This function tries to save the specified data to a file at the specified path. If
    /// the file already exists, it will be truncated (\ref WriteMode::trunc) before the new
    /// data is written.
    ///
    /// On success, it returns `true`. On failure, it sets \p ec to an error code that
    /// reflects the cause of the failure and returns `false`.
    ///
    [[nodiscard]] static bool try_save(core::FilesystemPathRef, core::StringSpan<char> data,
                                       std::error_code& ec) noexcept;

    /// \brief Try to ensure existence of file and mark it as modified now.
    ///
    /// This function attempts to creates a file at the specified path, if the specified
    /// path does not already refer to a file, and then update its "last modified time" to
    /// the current time.
    ///
    /// On success, it returns `true`. On failure, it sets \p ec to an error code that
    /// reflects the cause of the failure and returns `false`.
    ///
    [[nodiscard]] static bool try_touch(core::FilesystemPathRef, std::error_code& ec) noexcept;

    /// \brief Whether encapsulated descriptor refers to directory.
    ///
    /// This function is shorthand for calling \ref get_file_info() and then returning \ref
    /// Info::is_directory from the resulting file information object.
    ///
    /// \sa \ref get_file_info()
    /// \sa \ref Info::is_directory
    ///
    bool is_directory();

    struct Info;

    /// \brief Get information about open file.
    ///
    /// This function is shorthand for calling `try_get_file_info(info, ec)` and then
    /// returning `info` on success or throwing an exception on error. See \ref
    /// try_get_file_info().
    ///
    /// \sa \ref is_directory()
    /// \sa \ref Info
    /// \sa \ref try_get_file_info()
    ///
    auto get_file_info() -> Info;

    /// \brief Try to get information about open file.
    ///
    /// This function attempts to retreive general information (\ref Info) about the file
    /// descriptor encapsulated by this file object. It is an error to call this function on
    /// an empty file object (\ref is_open()).
    ///
    /// \sa \ref Info
    /// \sa \ref get_file_info()
    ///
    [[nodiscard]] bool try_get_file_info(Info& info, std::error_code& ec) noexcept;

    /// \brief Whether file object refers to a text terminal.
    ///
    /// This function calls \ref try_get_terminal_info() and then returns `true` if the
    /// return value was `true` and \p is_term was set to `true`. Otherwise it returns
    /// `false`.
    ///
    /// This function returns `false` if the file object is empty (see \ref is_open()).
    ///
    bool is_terminal() noexcept;

    struct TerminalInfo;

    /// \brief Get information about text terminal.
    ///
    /// This function has the same effect as \ref try_get_terminal_info() except that it
    /// returns \p is_term on success, and throws an exception of type `std::system_error`
    /// on failure.
    ///
    /// This function fails if the file object is empty (see \ref is_open()).
    ///
    bool get_terminal_info(TerminalInfo&);

    /// \brief Try to get information about text terminal.
    ///
    /// This function attempts to determine whether the file object refers to a text
    /// terminal, and if it does, it attempts to extract information about that terminal.
    ///
    /// On success, this function returns `true` after setting \p is_term and \p info as
    /// described below. On failure, it returns `false` after setting \p ec to an error code
    /// that reflects the cause of the failure.
    ///
    /// If this file object refers to a text terminal, \p is_term is set to `true`, and \p
    /// info is set to describe that terminal. Otherwise, \p is_term is set to `false`, and
    /// \p info is left unchanged.
    ///
    /// On success, \p ec is left unchanged. On failure, \p is_term and \p info are left
    /// unchanged.
    ///
    /// If the file object is empty (see \ref is_open()), this function fails with an
    /// unspecified error code.
    ///
    bool try_get_terminal_info(bool& is_term, TerminalInfo& info, std::error_code& ec) noexcept;

    /// \{
    ///
    /// \brief Movability.
    ///
    /// File objects are movable.
    ///
    File(File&&) noexcept;
    File& operator=(File&&) noexcept;
    /// \}

    ~File() noexcept;

private:
    class StandardStreams;

    static StandardStreams s_standard_streams;

#if ARCHON_WINDOWS
    using native_handle_type = void*;
#else
    using native_handle_type = int;
#endif

#if ARCHON_WINDOWS
    static constexpr void* s_null_handle = nullptr;
#else
    static constexpr int s_null_handle = -1;
#endif

    native_handle_type m_handle = s_null_handle;

#if ARCHON_WINDOWS
    bool m_holds_lock = false; // Only valid when m_handle is not null
#endif

    bool m_no_implicit_close = false;

    bool do_try_open(core::FilesystemPathRef, AccessMode, CreateMode, WriteMode, std::error_code& ec) noexcept;

    void adopt(native_handle_type, bool no_implicit_close = false) noexcept;
    void implicit_close() noexcept;
    void do_close() noexcept;
    void steal_from(File& other) noexcept;

    bool do_lock(bool exclusive, bool nonblocking);
    void do_unlock() noexcept;
};



/// \brief General information about file.
///
/// An object of this type is used to convey general information about a file. \ref
/// get_file_info() returns a object of this type.
///
/// \sa \ref get_file_info()
/// \sa \ref TerminalInfo
///
struct File::Info {
    /// \brief Whether file is a directory.
    ///
    /// This flag is true if, and only if the corresponding file refers to a directory.
    ///
    bool is_directory;
};



/// \brief Information about text terminal associated with file.
///
/// An object of this type is used to convey information about a text terminal. See \ref
/// get_terminal_info().
///
/// \sa \ref get_terminal_info()
/// \sa \ref Info
///
struct File::TerminalInfo {
    /// \brief Size of terminal.
    ///
    /// An object of this type specifies the size of a text terminal.
    ///
    struct Size {
        /// \brief Number of columns.
        ///
        /// The width of the terminal in number of fixed-width characters, i.e., the number
        /// of columns.
        ///
        int width;

        /// \brief Number of rows.
        ///
        /// The height of the terminal in number of fixed-height characters, i.e., the
        /// number of rows.
        ///
        int height;
    };

    /// \brief Size of terminal if known.
    ///
    /// This field specifies the size of the terminal when the information is available.
    ///
    std::optional<Size> size;
};








// Implementation


class File::StandardStreams {
public:
    File cin, cout, cerr;

    StandardStreams() noexcept;
};


inline auto File::get_cin() noexcept -> File&
{
    return s_standard_streams.cin;
}


inline auto File::get_cout() noexcept -> File&
{
    return s_standard_streams.cout;
}


inline auto File::get_cerr() noexcept -> File&
{
    return s_standard_streams.cerr;
}


inline File::File(core::FilesystemPathRef path, Mode mode)
{
    open(path, mode); // Throws
}


inline void File::open(core::FilesystemPathRef path, Mode mode)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_open(path, mode, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to open file", path, ec);
}


inline void File::close() noexcept
{
    if (!is_open())
        return;
    do_close();
    m_handle = s_null_handle;
}


inline bool File::is_open() const noexcept
{
#if !ARCHON_WINDOWS
    ARCHON_ASSERT(m_handle >= -1);
#endif
    return (m_handle != s_null_handle);
}


inline File::operator bool() const noexcept
{
    return is_open();
}


inline void File::open(core::FilesystemPathRef path, AccessMode access_mode,
                       CreateMode create_mode, WriteMode write_mode)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_open(path, access_mode, create_mode, write_mode, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to open file", path, ec);
}


inline auto File::read(core::Span<char> buffer) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


inline void File::write(core::StringSpan<char> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to write to file");
}


inline auto File::read_some(core::Span<char> buffer) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


inline auto File::write_some(core::StringSpan<char> data) -> std::size_t
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_write_some(data, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to write to file");
}


inline auto File::tell() -> offset_type
{
    return seek(0, Whence::cur); // Throws
}


inline auto File::seek(offset_type offset, Whence whence) -> offset_type
{
    offset_type result = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(offset, whence, result, ec)))
        return result; // Success
    throw std::system_error(ec, "Failed to seek");
}


inline bool File::try_open(core::FilesystemPathRef path, Mode mode, std::error_code& ec) noexcept
{
    AccessMode access_mode = AccessMode::read_only;
    CreateMode create_mode = CreateMode::never;
    WriteMode  write_mode  = WriteMode::normal;
    switch (mode) {
        case Mode::read:
            break;
        case Mode::update:
            access_mode = AccessMode::read_write;
            break;
        case Mode::write:
            access_mode = AccessMode::read_write;
            create_mode = CreateMode::allow;
            write_mode  = WriteMode::trunc;
            break;
        case Mode::append:
            access_mode = AccessMode::read_write;
            create_mode = CreateMode::allow;
            write_mode  = WriteMode::append;
            break;
    }
    return try_open(path, access_mode, create_mode, write_mode, ec);
}


inline bool File::try_read_some(core::Span<char> buffer, std::size_t& n, std::error_code& ec) noexcept
{
    bool interrupted = false;
  again:
    if (ARCHON_LIKELY(try_read_some_a(buffer, n, interrupted, ec)))
        return true;
    if (ARCHON_LIKELY(interrupted))
        goto again;
    return false;
}


inline bool File::try_write_some(core::StringSpan<char> data, std::size_t& n, std::error_code& ec) noexcept
{
    bool interrupted = false;
  again:
    if (ARCHON_LIKELY(try_write_some_a(data, n, interrupted, ec)))
        return true;
    if (ARCHON_LIKELY(interrupted))
        goto again;
    return false;
}


inline void File::lock_exclusive()
{
    do_lock(true, false); // Throws
}


inline void File::lock_shared()
{
    do_lock(false, false); // Throws
}


inline bool File::nb_lock_exclusive()
{
    return do_lock(true, true); // Throws
}


inline bool File::nb_lock_shared()
{
    return do_lock(false, true); // Throws
}


inline void File::unlock() noexcept
{
    if (is_open())
        do_unlock();
}


inline auto File::load(core::FilesystemPathRef path) -> std::string
{
    std::error_code ec;
    std::string data;
    if (ARCHON_LIKELY(try_load(path, data, ec)))
        return data; // Success
    throw std::filesystem::filesystem_error("Failed to load file", path, ec);
}


inline void File::save(core::FilesystemPathRef path, core::StringSpan<char> data)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_save(path, data, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to save data to file", path, ec);
}


inline void File::touch(core::FilesystemPathRef path)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_touch(path, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to touch file", path, ec);
}


inline bool File::is_directory()
{
    Info info = get_file_info(); // Throws
    return info.is_directory;
}


inline auto File::get_file_info() -> Info
{
    Info info = {};
    std::error_code ec;
    if (ARCHON_LIKELY(try_get_file_info(info, ec)))
        return info; // Success
    throw std::system_error(ec, "Failed to get general file information");
}


inline bool File::is_terminal() noexcept
{
    if (ARCHON_LIKELY(is_open())) {
        bool is_term = false;
        TerminalInfo info; // Dummy
        std::error_code ec; // Dummy
        bool success = try_get_terminal_info(is_term, info, ec);
        return (success && is_term);
    }
    return false;
}


inline bool File::get_terminal_info(TerminalInfo& info)
{
    bool is_term = false;
    std::error_code ec;
    if (ARCHON_LIKELY(try_get_terminal_info(is_term, info, ec)))
        return is_term; // Success
    throw std::system_error(ec, "Failed to get terminal information");
}


inline File::File(File&& other) noexcept
{
    steal_from(other);
}


inline auto File::operator=(File&& other) noexcept -> File&
{
    implicit_close();
    steal_from(other);
    return *this;
}


inline File::~File() noexcept
{
    implicit_close();
}


inline File::StandardStreams File::s_standard_streams;


inline void File::adopt(native_handle_type handle, bool no_implicit_close) noexcept
{
    implicit_close();
    m_handle = handle;
#if ARCHON_WINDOWS
    m_holds_lock = false;
#endif
    m_no_implicit_close = no_implicit_close;
}


inline void File::implicit_close() noexcept
{
    if (!is_open() || m_no_implicit_close)
        return;
    do_close();
}


inline void File::steal_from(File& other) noexcept
{
    m_handle = other.m_handle;
#if ARCHON_WINDOWS
    m_holds_lock = other.m_holds_lock;
#endif
    other.m_handle = s_null_handle;
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_FILE_HPP
