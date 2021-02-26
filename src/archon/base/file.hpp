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

#ifndef ARCHON_X_BASE_X_FILE_HPP
#define ARCHON_X_BASE_X_FILE_HPP

/// \file


#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/span.hpp>
#include <archon/base/filesystem.hpp>

#if !ARCHON_WINDOWS
#  include <sys/stat.h> // off_t
#endif


namespace archon::base {


/// \brief Encapsulation of a file descriptor.
///
/// A file object is an encapsulation of a file descriptor from the point of
/// view of a POSIX platform. In other words, the offered API works at the
/// lowest level. For example, reading (\ref read()) and writing (\ref write())
/// occur in an unbuffered manner.
///
/// A file object is considered open (\ref is_open()) when the contained file
/// descriptor refers to an open file, or to some other file-like entity that
/// the system associates with a file descriptor, such as STDOUT.
///
class File {
public:
    /// \brief Major access modes.
    ///
    /// These are the major modes in which a file can be opened, and they can be
    /// used with \ref open(base::FilesystemPathRef, Mode). More fine-grained
    /// control is available using \ref AccessMode, \ref CreateMode, and \ref
    /// WriteMode.
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
    /// These are the standard streams, STDIN, STDOUT, and STDERR. Use \ref
    /// is_terminal() to check whether they are associated with a terminal.
    ///
    static File cin;
    static File cout;
    static File cerr;
    /// \}

    /// \brief Open the specified file.
    ///
    /// Open the file at the specified path in the filesystem and bind this file
    /// object to the open file. This is equivalent to calling \ref
    /// open(base::FilesystemPathRef, Mode) on a default constructed file
    /// object.
    ///
    explicit File(base::FilesystemPathRef, Mode = Mode::read);

    /// \brief Create a closed file object.
    ///
    /// Create a closed file object, that is, a file object that is not
    /// currently associated with an open file.
    ///
    File() noexcept = default;

    /// \brief Open the specified file.
    ///
    /// Open the file at the specified path in the filesystem and bind this file
    /// object to the open file. If this file object was already bound to an
    /// open file (\ref is_open()) that bond is first broken (that file is
    /// closed).
    ///
    void open(base::FilesystemPathRef, Mode = Mode::read);

    /// \brief Close the file.
    ///
    /// If the file object is currently associated with an open file, break that
    /// association now. On POSIX systems, this corresponds to closing the
    /// contained file descriptor. This puts the file object into the closed
    /// state.
    ///
    /// If the file object is already in the closed state, this function has no
    /// effect (idempotency).
    ///
    void close() noexcept;

    /// \{
    ///
    /// \brief Has association with open file.
    ///
    /// Returns true if, and only if this file object is currently associated
    /// with an open file, or file-like entity.
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
    /// This function has the same effect as \ref try_open() except that it
    /// throws an exception of type `std::system_error` on failure.
    ///
    /// \sa \ref open(base::FilesystemPathRef, Mode).
    ///
    void open(base::FilesystemPathRef, AccessMode, CreateMode, WriteMode);

    /// \brief Read a chunk of data.
    ///
    /// This function reads successive bytes from the file or stream associated
    /// with this file object, and places them in the specified buffer. Reading
    /// ends when the buffer is full or the end of input is reached.
    ///
    /// This function has the same effect as \ref try_read() except that, on
    /// success, it returns the number of bytes placed in the buffer, and, on
    /// failure, it throws an exception of type `std::system_error`.
    ///
    /// Note that on failure, some bytes may have been read from the file and
    /// placed into the specified buffer. Consider using \ref try_read() if you
    /// need to know how many bytes were read, even in the event of a failure.
    ///
    std::size_t read(base::Span<char> buffer);

    /// \brief Write a chunk of data.
    ///
    /// This function writes all of the specified data to the file or stream
    /// associated with this file object.
    ///
    /// This function has the same effect as \ref try_write() except that, on
    /// failure, it throws an exception of type `std::system_error`.
    ///
    /// Note that on failure, some if the specified bytes may have been written
    /// to the file. Consider using \ref try_write() if you need to know how
    /// many bytes were written, even in the event of a failure.
    ///
    void write(base::Span<const char> data);

    /// \brief Read at least one byte.
    ///
    /// This function reads successive bytes from the file or stream associated
    /// with this file object, and places them in the specified buffer. Reading
    /// ends when at least one byte has been read and further reading would
    /// require that the calling thread is blocked, or when the specified buffer
    /// has been filled, or when the end of input is reached.
    ///
    /// This function has the same effect as \ref try_read_some() except that,
    /// on success, it returns the number of bytes placed in the buffer, and, on
    /// failure, it throws an exception of type `std::system_error`.
    ///
    std::size_t read_some(base::Span<char> buffer);

    /// \brief Write at least one byte.
    ///
    /// This function writes some, or all of the specified data to the file or
    /// stream associated with this file object. Writing ends when at least one
    /// byte has been written and further writing would require that the calling
    /// thread is blocked, or when all of the specified data has been written.
    ///
    /// This function has the same effect as \ref try_write_some() except
    /// that, on success, it returns the number of written bytes, and, on
    /// failure, it throws an exception of type `std::system_error`.
    ///
    std::size_t write_some(base::Span<const char> data);

#if ARCHON_WINDOWS
    using offset_type = std::int_fast64_t;
#else
    using offset_type = off_t;
#endif

    /// \brief Get current read/write position.
    ///
    /// This function is a shorthand for `seek(0, Whence::cur)`.
    ///
    offset_type get_offset();

    /// \brief Set current read/write position.
    ///
    /// This function has the same effect as `seek(offset)`.
    ///
    void set_offset(offset_type offset);

    /// \brief Possible ways to specify read/write position.
    ///
    /// The possible ways in which one can specify the new read/write position
    /// when calling \ref seek().
    ///
    enum class Whence {
        set, ///< Specified offset is relative to beginning of file.
        cur, ///< Specified offset is relative to current position.
        end  ///< Specified offset is relative to end of file.
    };

    /// \brief Manipulate current read/write position.
    ///
    /// This function manipulates the read/write position associated with the
    /// file descriptor contained in this file object. This is the position that
    /// controls where the next read or write operation takes place. POSIX calls
    /// this, the "file offset".
    ///
    /// This function has the same effect as \ref try_seek() except that, on
    /// success, the resulting read/write position is returned, and, on failure,
    /// an exception of type `std::system_error` is thrown.
    ///
    offset_type seek(offset_type, Whence = Whence::set);

    /// \brief Try to open a file.
    ///
    /// Specifying \ref AccessMode::read_only together with a create mode that
    /// is not \ref CreateMode::never, or together with a write mode that is not
    /// \ref WriteMode::normal, results in undefined behavior. Specifying \ref
    /// WriteMode::trunc together with \ref CreateMode::must results in
    /// undefined behavior.
    ///
    /// On success, this function returns `true`, and leaves \p ec untouched. On
    /// failure, this function sets \p ec to the corresponding error code, and
    /// returns `false`.
    ///
    [[nodiscard]] bool try_open(base::FilesystemPathRef, AccessMode, CreateMode, WriteMode,
                                std::error_code& ec) noexcept;

    /// \brief Try to read a chunk of data.
    ///
    /// This function reads successive bytes from the file or stream associated
    /// with this file object, and places them in the specified buffer. Reading
    /// ends when the buffer is full or the end of input is reached.
    ///
    /// This function has the same effect as \ref try_read_some() except that it
    /// continues reading until the buffer is full or the end of input is
    /// reached.
    ///
    /// On success, if \p n is less than `buffer.size()`, it means that the end
    /// of input has been reached. On failure, \p n will always be less than
    /// `buffer.size()` (provided that `buffer.size()` is greater than zero),
    /// and it will indicate how many bytes were read, and placed into the
    /// buffer before the failure occurred.
    ///
    /// On success, \p ec is left untouched.
    ///
    [[nodiscard]] bool try_read(base::Span<char> buffer, std::size_t& n,
                                std::error_code& ec) noexcept;

    /// \brief Try to write a chunk of data.
    ///
    /// This function writes all of the specified data to the file or stream
    /// associated with this file object.
    ///
    /// This function has the same effect as \ref try_write_some() except that
    /// it continues writing until all of the specified data has been written.
    ///
    /// On success, \p n will always be equal to `data.size()`. On failure, \p n
    /// will always be less than `data.size()` (provided that `data.size()` is
    /// greater than zero), and it will indicate how many bytes were written
    /// before the failure occurred.
    ///
    /// On success, \p ec is left untouched.
    ///
    [[nodiscard]] bool try_write(base::Span<const char> data, std::size_t& n,
                                 std::error_code& ec) noexcept;

    /// \brief Try to read at least one byte.
    ///
    /// This function reads successive bytes from the file or stream associated
    /// with this file object, and places them in the specified buffer. Reading
    /// ends when at least one byte has been read and further reading would
    /// require that the calling thread is blocked, or when the specified buffer
    /// has been filled, or when the end of input is reached.
    ///
    /// This function has the same effect as \ref try_read_some_a() except that,
    /// on interruption due to recepetion of a system signal, reading is
    /// automatically resumed.
    ///
    /// On success, if \p n is zero and the size of the specified buffer is
    /// greater than zero, it means that the end of input has been reached.
    ///
    /// On success, \p ec is left untouched. On failure \p n is left untouched,
    /// and no bytes will have been read from the stream.
    ///
    [[nodiscard]] bool try_read_some(base::Span<char> buffer, std::size_t& n,
                                     std::error_code& ec) noexcept;

    /// \brief Try to write at least one byte.
    ///
    /// This function writes some, or all of the specified data to the file or
    /// stream associated with this file object. Writing ends when at least one
    /// byte has been written and further writing would require that the calling
    /// thread is blocked, or when all of the specified data has been written.
    ///
    /// This function has the same effect as \ref try_write_some_a() except
    /// that, on interruption due to recepetion of a system signal, writing is
    /// automatically resumed.
    ///
    /// On success, \p n will always be greater than zero provided `data.size()`
    /// is greater than zero.
    ///
    /// On success, \p ec is left untouched. On failure \p n is left untouched,
    /// and no bytes will have been written to the stream.
    ///
    [[nodiscard]] bool try_write_some(base::Span<const char> data, std::size_t& n,
                                      std::error_code& ec) noexcept;

    /// \brief Try to read at least one byte.
    ///
    /// This function reads successive bytes from the file or stream associated
    /// with this file object, and places them in the specified buffer.
    ///
    /// Reading ends when at least one byte has been read and further reading
    /// would require that the calling thread is blocked, or when the specified
    /// buffer has been filled, or when the end of input is reached.
    ///
    /// Reading begins at the current file offset, which will generally be the
    /// offset at which the previous read (or write) operation ended, but see
    /// \ref set_offset() for a way to change that.
    ///
    /// On success, this function sets \p n to the number of bytes placed in the
    /// specified buffer, and then it returns `true`. On failure, it sets \p
    /// interrupted to `false` and \p ec to an error code that reflects the
    /// cause of the failure, and then it returns `false`.
    ///
    /// On POSIX systems, if the calling thread was blocked waiting for the
    /// opportunity to read at least one byte, and the wait was interrupted due
    /// to reception of a system signal, this function sets \p interrupted to
    /// `true` and then returns `false`.
    ///
    /// On success, if \p n is zero and the size of the specified buffer is
    /// greater than zero, it means that the end of input has been reached.
    ///
    /// On success, \p interrupted and \p ec are left untouched. On failure \p n
    /// is left untouched, and no bytes will have been read from the stream. On
    /// interruption, \p n and \p ec are left untouched, and no bytes will have
    /// been read from the stream.
    ///
    /// Calling this function on a file object, that is not currently associated
    /// with an open file or stream, has undefined behavior.
    ///
    [[nodiscard]] bool try_read_some_a(base::Span<char> buffer, std::size_t& n,
                                       bool& interrupted, std::error_code& ec) noexcept;

    /// \brief Try to write at least one byte.
    ///
    /// This function writes some, or all of the specified data to the file or
    /// stream associated with this file object.
    ///
    /// Writing ends when at least one byte has been written and further writing
    /// would require that the calling thread is blocked, or when all of the
    /// specified data has been written.
    ///
    /// Writing begins at the current file offset, which will generally be the
    /// offset at which the previous write (or read) operation ended, but see
    /// \ref set_offset() for a way to change that.
    ///
    /// On success, this function sets \p n to the number of bytes written, and
    /// then returns `true`. On failure, it sets \p interrupted to `false` and
    /// \p ec to an error code that reflects the cause of the failure, and then
    /// it returns `false`.
    ///
    /// On POSIX systems, if the calling thread was blocked waiting for the
    /// opportunity to write at least one byte, and the wait was interrupted due
    /// to reception of a system signal, this function sets \p interrupted to
    /// `true` and then returns `false`.
    ///
    /// On success, \p n will always be greater than zero provided `data.size()`
    /// is greater than zero.
    ///
    /// On success, \p interrupted and \p ec are left untouched. On failure \p n
    /// is left untouched, and no bytes will have been written to the stream. On
    /// interruption, \p n and \p ec are left untouched, and no bytes will have
    /// been written to the stream.
    ///
    /// Calling this function for a file, that was opened in read-only mode, has
    /// undefined behavior.
    ///
    /// Calling this function on a file object, that is not currently associated
    /// with an open file or stream, has undefined behavior.
    ///
    [[nodiscard]] bool try_write_some_a(base::Span<const char> data, std::size_t& n,
                                        bool& interrupted, std::error_code& ec) noexcept;

    /// \brief Manipulate current read/write position.
    ///
    /// This function manipulates the position that controls where the next read
    /// or write operation takes place. The specified offset (\p offset) is
    /// interpreted as specified by \p whence.
    ///
    /// On success, this function returns `true` after assigning the resulting
    /// read/write position to \p result as an offset relative to the beginning
    /// of the file. On failure, it returns `false` after setting \p ec to an
    /// error code that reflects the cause of the failure.
    ///
    /// On a POSIX platform, this function is implemented in terms of the
    /// system-level function `lseek()`.
    ///
    [[nodiscard]] bool try_seek(offset_type offset, Whence whence, offset_type& result,
                                std::error_code& ec) noexcept;

    /// \brief Place exclusive lock on file.
    ///
    /// Place an exclusive lock on this file. This blocks the caller
    /// until all other locks have been released.
    ///
    /// Locks acquired on the same underlying file, but via distinct file
    /// objects, have fully recursive behavior, even if they are acquired in the
    /// same process (or thread).
    ///
    /// Calling this function on an object that is not currently associated with
    /// an open file, or on an object that is already locked, has undefined
    /// behavior.
    ///
    void lock_exclusive();

    /// \brief Acquire shared lock on file.
    ///
    /// Acquire a shared lock on this file. This blocks the caller
    /// until all other exclusive locks have been released.
    ///
    /// Locks acquired on the same underlying file, but via distinct file
    /// objects, have fully recursive behavior, even if they are acquired in the
    /// same process (or thread).
    ///
    /// Calling this function on an object that is not currently associated with
    /// an open file, or on an object that is already locked, has undefined
    /// behavior.
    ///
    void lock_shared();

    /// \brief Try to place exclusive lock on file.
    ///
    /// This is a nonblocking version of \ref lock_exclusive(). It returns true
    /// if, and only if it succeeds.
    ///
    [[nodiscard]] bool nb_lock_exclusive();

    /// \brief Try to acquire shared lock on file.
    ///
    /// This is a nonblocking version of \ref lock_shared(). It returns true if,
    /// and only if it succeeds.
    ///
    [[nodiscard]] bool nb_lock_shared();

    /// \brief Relinquish any held lock.
    ///
    /// If an exclusive or shared lock was previously acquired via this file
    /// object, releases now. If no lock is currently held by the file object,
    /// or if the file object is in the closed state, this function has no
    /// effect (idempotency).
    ///
    void unlock() noexcept;

    /// \brief Check terminal association.
    ///
    /// Returns true only if this file object is associated with a terminal. On
    /// POSIX systems, this corresponds to calling the system-level function
    /// `isatty()`.
    ///
    bool is_terminal() const noexcept;

    /// \brief Load contents of file.
    ///
    /// This function loads the contents of the specified file, and returns it
    /// as a string.
    ///
    static std::string load(base::FilesystemPathRef);

    /// \brief Load contents of file and remove final newline.
    ///
    /// This function loads the contents of the specified file, and returns it
    /// as a string, although, if the last character in the file is a newline
    /// character, it will be removed before the contents is returned.
    ///
    static std::string load_and_chomp(base::FilesystemPathRef);

    /// \brief Save data to file.
    ///
    /// This function saves the specified data to a file at the specified
    /// path. If the file already exists, it will be truncated (\ref
    /// WriteMode::trunc) before the new data is written.
    ///
    static void save(base::FilesystemPathRef, base::Span<const char> data);

    /// \brief Ensure existence of file and mark it as modified now
    ///
    /// This function has the same effect as \ref try_touch() except that it
    /// throws an exception of type `std::system_error` on failure.
    ///
    static void touch(base::FilesystemPathRef);

    /// \brief Try to Load contents of file.
    ///
    /// This function tries to load the contents of the specified file.
    ///
    /// On success, it assigns the loaded data to \p data and returns `true`. On
    /// failure, it sets \p ec to an error code that reflects the cause of the
    /// failure and returns `false`.
    ///
    [[nodiscard]] static bool try_load(base::FilesystemPathRef, std::string& data,
                                       std::error_code& ec);

    /// \brief Try to save data to file.
    ///
    /// This function tries to save the specified data to a file at the specified
    /// path. If the file already exists, it will be truncated (\ref
    /// WriteMode::trunc) before the new data is written.
    ///
    /// On success, it returns `true`. On failure, it sets \p ec to an error
    /// code that reflects the cause of the failure and returns `false`.
    ///
    [[nodiscard]] static bool try_save(base::FilesystemPathRef, base::Span<const char> data,
                                       std::error_code& ec) noexcept;

    /// \brief Try to ensure existence of file and mark it as modified now.
    ///
    /// This function attempts to creates a file at the specified path, if the
    /// specified path does not already refer to a file, and then update its
    /// "last modified time" to the current time.
    ///
    /// On success, it returns `true`. On failure, it sets \p ec to an error
    /// code that reflects the cause of the failure and returns `false`.
    ///
    [[nodiscard]] static bool try_touch(base::FilesystemPathRef, std::error_code& ec) noexcept;

    /// \{
    ///
    /// \brief Movability.
    ///
    /// File obejcts are movable.
    ///
    File(File&&) noexcept;
    File& operator=(File&&) noexcept;
    /// \}

    ~File() noexcept;

private:
    struct InitStandardStreams;
    static InitStandardStreams s_init_standard_streams;

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

    void adopt(native_handle_type, bool no_implicit_close = false) noexcept;
    void implicit_close() noexcept;
    void do_close() noexcept;
    void steal_from(File& other) noexcept;

    bool do_lock(bool exclusive, bool nonblocking);
    void do_unlock() noexcept;
};








// Implementation


inline File::File(base::FilesystemPathRef path, Mode mode)
{
    open(path, mode); // Throws
}


inline void File::open(base::FilesystemPathRef path, Mode mode)
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
    open(path, access_mode, create_mode, write_mode); // Throws
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


inline void File::open(base::FilesystemPathRef path, AccessMode access_mode,
                       CreateMode create_mode, WriteMode write_mode)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_open(path, access_mode, create_mode, write_mode, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to open file", path, ec);
}


inline std::size_t File::read(base::Span<char> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


inline void File::write(base::Span<const char> data)
{
    std::size_t n; // Dummy
    std::error_code ec;
    if (ARCHON_LIKELY(try_write(data, n, ec)))
        return; // Success
    throw std::system_error(ec, "Failed to write to file");
}


inline std::size_t File::read_some(base::Span<char> buffer)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_read_some(buffer, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to read from file");
}


inline std::size_t File::write_some(base::Span<const char> data)
{
    std::size_t n = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_write_some(data, n, ec)))
        return n; // Success
    throw std::system_error(ec, "Failed to write to file");
}


inline auto File::get_offset() -> offset_type
{
    return seek(0, Whence::cur); // Throws
}


inline void File::set_offset(offset_type offset)
{
    seek(offset); // Throws
}


inline auto File::seek(offset_type offset, Whence whence) -> offset_type
{
    offset_type result = 0;
    std::error_code ec;
    if (ARCHON_LIKELY(try_seek(offset, whence, result, ec)))
        return result; // Success
    throw std::system_error(ec, "Failed to seek");
}


inline bool File::try_read_some(base::Span<char> buffer, std::size_t& n,
                                std::error_code& ec) noexcept
{
    bool interrupted = false;
  again:
    if (ARCHON_LIKELY(try_read_some_a(buffer, n, interrupted, ec)))
        return true;
    if (ARCHON_LIKELY(interrupted))
        goto again;
    return false;
}


inline bool File::try_write_some(base::Span<const char> data, std::size_t& n,
                                 std::error_code& ec) noexcept
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


inline std::string File::load(base::FilesystemPathRef path)
{
    std::error_code ec;
    std::string data;
    if (ARCHON_LIKELY(try_load(path, data, ec)))
        return data; // Success
    throw std::filesystem::filesystem_error("Failed to load file", path, ec);
}


inline std::string File::load_and_chomp(base::FilesystemPathRef path)
{
    std::string data = load(path); // Throws
    if (!data.empty() && data.back() == '\n')
        data.pop_back();
    return data;
}


inline void File::save(base::FilesystemPathRef path, base::Span<const char> data)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_save(path, data, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to save data to file", path, ec);
}


inline void File::touch(base::FilesystemPathRef path)
{
    std::error_code ec;
    if (ARCHON_LIKELY(try_touch(path, ec)))
        return; // Success
    throw std::filesystem::filesystem_error("Failed to touch file", path, ec);
}


inline File::File(File&& other) noexcept
{
    steal_from(other);
}


inline File& File::operator=(File&& other) noexcept
{
    implicit_close();
    steal_from(other);
    return *this;
}


inline File::~File() noexcept
{
    implicit_close();
}


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


} // namespace archon::base

#endif // ARCHON_X_BASE_X_FILE_HPP
