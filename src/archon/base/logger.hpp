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

#ifndef ARCHON_X_BASE_X_LOGGER_HPP
#define ARCHON_X_BASE_X_LOGGER_HPP

/// \file


#include <cstddef>
#include <limits>
#include <algorithm>
#include <utility>
#include <array>
#include <string_view>
#include <locale>
#include <stdexcept>
#include <ostream>
#include <mutex>
#include <filesystem>

#include <archon/base/features.h>
#include <archon/base/assert.hpp>
#include <archon/base/span.hpp>
#include <archon/base/buffer.hpp>
#include <archon/base/string_codec.hpp>
#include <archon/base/enum.hpp>
#include <archon/base/string_formatter.hpp>
#include <archon/base/file.hpp>
#include <archon/base/terminal.hpp>


namespace archon::base {


/// \brief Available log levels.
///
/// This is the list of available log levels. When passed to \ref
/// BasicLogger::log(), one should think of it as a verbosity level, or inverse
/// criticality level. When returned from \ref
/// BasicLogger::get_log_level_limit(), it should be thought of as a limit on
/// verbosity (threshold on criticality).
///
enum class LogLevel {
    /// Do not log anything. This level should only be used as a limit, never as
    /// a level passed to \ref BasicLogger::log().
    off,

    /// Be silent except when there is a fatal error.
    fatal,

    /// Be silent except when there is an error.
    error,

    /// Be silent except when there is an error or a warning.
    warn,

    /// Reveal information about what is going on, but in a minimalistic fashion
    /// to avoid general overhead from logging, and to ease the burden on
    /// readers.
    info,

    /// Like `info`, but prioritize completeness over minimalism.
    detail,

    /// Reveal information that can aid debugging, no longer paying attention to
    /// efficiency.
    debug,

    /// A version of `debug` that allows for a very high volume of output.
    trace,

    /// Log everything. This level should only be used as a limit, never as a
    /// level passed to \ref BasicLogger::log().
    all
};


/// \brief Provide for easy stream input and output.
///
/// This enumeration traits specialization makes it easy to read and write log
/// levels from input streams and to output streams.
///
template<> struct EnumTraits<LogLevel> {
    static constexpr bool is_specialized = true;
    struct Spec { static EnumAssoc map[]; };
    static constexpr bool ignore_case = false;
};


/// \brief Check log level value validity.
///
/// Returns true if, and only if the specifed integer value corresponds to one
/// of the named values in LogLevel.
///
bool is_valid_log_level(int value);




/// \brief A general purpose logger.
///
/// Messages are submitted to the logger with an accompanying log level (\ref
/// LogLevel). The log level can be thought of as specifying the verbosity level
/// of the message. The logger checks this log level against an effective limit
/// (\ref get_log_level_limit()), and suppresses the message if the level is too
/// high. For example, a message logged at level `detail` with be suppressed if
/// the effective limit is `info`, but will not be suppressed if the limit is
/// `detail`. For the sake of performance, this filtering happens before
/// messages are formatted. This ensures that the cost of message submission is
/// very low for messages that end up being discarded.
///
/// Examples:
///
/// \code{.cpp}
///
///   logger.error("Overlong message from master coordinator");
///   logger.info("Listening for peers on %s:%s", listen_address, listen_port);
///
/// \endcode
///
/// FIXME: Explain level limiting and refer to \ref BasicLimitLogger.
///
/// FIXME: Explain fixed versus variable log level limit.
///
/// FIXME: Explain low cost of not logging when using fixed log level limit.
///
/// FIXME: Explain prefixes and refer to \ref BasicPrefixLogger.
///
/// FIXME: Explain channels rougly. Full explanation can be deferred to \ref
/// BasicChannelLogger.
///
/// FIXME: Make reference to \ref BasicRootLogger.
///
/// FIXME: Explain thread safety.
///
/// FIXME: Explain that in general, when a logger is used as base logger during
/// the construction of a new logger, it is the responsibility of the
/// application to ensure that the life of the base logger extends at least
/// until the end of life of the new logger.
///
template<class C, class T = std::char_traits<C>> class BasicLogger {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;

    /// \brief A null logger.
    ///
    /// This is a logger that discards all logged messages. This logger is
    /// implemented in terms of \ref BasicNullLogger.
    ///
    static BasicLogger null;

    /// \{
    ///
    /// \brief Console loggers for STDOUT and STDERR.
    ///
    /// These loggers send log messages to STDOUT and STDERR respectively. They
    /// are implemented in terms of \ref BasicFileLogger with respective
    /// arguments of \ref File::cout and \ref File::cerr, and with colorization
    /// mode set to \ref BasicFileLogger::Colorize::detect.
    ///
    /// The first time \ref get_cout() or \ref get_cerr() is called, both
    /// streams are created based on the currently selected global
    /// locale. Therefore, if the application needs these loggers to use a
    /// particular locale, it must update the global locale before calling any
    /// of these functions.
    ///
    static BasicLogger& get_cout();
    static BasicLogger& get_cerr();
    /// \}

    /// \{
    ///
    /// \brief Log a null-terminated parameterized message.
    ///
    /// These functions are shorthands for passing the corresponding log level
    /// (\ref LogLevel) to \ref log(LogLevel, const char*, const P&...).
    ///
    /// The characters of the specified message will be widened as if by
    /// `widen()` of an output stream with the same character type as this
    /// logger (`std::basic_ostream<C, T>`), and imbued with the locale
    /// associated with this logger (the locale of the sink of the channel of
    /// this logger). It is therefore only safe to use characters from the basic
    /// source character set in \p message. The intention is that \p message is
    /// always a string literal. This mimics the intention of `widen()`.
    ///
    template<class... P> void fatal(const char* message, const P&... params);
    template<class... P> void error(const char* message, const P&... params);
    template<class... P> void warn(const char* message, const P&... params);
    template<class... P> void info(const char* message, const P&... params);
    template<class... P> void detail(const char* message, const P&... params);
    template<class... P> void debug(const char* message, const P&... params);
    template<class... P> void trace(const char* message, const P&... params);
    /// \}

    /// \{
    ///
    /// \brief Log a parameterized message.
    ///
    /// These functions are shorthands for passing the corresponding log level
    /// (\ref LogLevel) to \ref log(LogLevel, string_view_type, const P&...).
    ///
    template<class... P> void fatal(string_view_type message, const P&... params);
    template<class... P> void error(string_view_type message, const P&... params);
    template<class... P> void warn(string_view_type message, const P&... params);
    template<class... P> void info(string_view_type message, const P&... params);
    template<class... P> void detail(string_view_type message, const P&... params);
    template<class... P> void debug(string_view_type message, const P&... params);
    template<class... P> void trace(string_view_type message, const P&... params);
    /// \}

    /// \brief Log a null-terminated parameterized message at any level.
    ///
    /// This function has the same effect as \ref log(LogLevel,
    /// string_view_type, const P&...) except that the message (\p message) is
    /// specified as a null-terminated string (a string literal).
    ///
    /// The characters of the specified message will be widened as if by
    /// `widen()` of an output stream with the same character type as this
    /// logger (`std::basic_ostream<C, T>`), and imbued with the locale
    /// associated with this logger (the locale of the sink of the channel of
    /// this logger). It is therefore only safe to use characters from the basic
    /// source character set in \p message. The intention is that \p message is
    /// always a string literal. This mimics the intention of `widen()`.
    ///
    template<class... P> void log(LogLevel, const char* message, const P&... params);

    /// \brief Log a parameterized message at any level.
    ///
    /// This function logs the specified message at the specified log level
    /// provided that \ref will_log() would return true for the specified log
    /// level. What it means to log a message depends on what kind of logger
    /// this is. If this is a \ref StreamLogger, for instance, logging a message
    /// means writing that message to the stream, e.g., to `std::cerr`.
    ///
    /// In any case, the specified parameterized message (\p message) is
    /// formatted as if it was passed to \ref base::format(std::basic_ostream<C,
    /// T>&, std::basic_string_view<C, T>, const P&...) along with the specified
    /// parameter values (\p params) for an output stream imbued with the locale
    /// associated with this logger (the locale of the sink of the channel of
    /// this logger).
    ///
    template<class... P> void log(LogLevel, string_view_type message, const P&... params);

    /// \brief Will log at specified level?
    ///
    /// This function returns true if, and only if the specified log level is
    /// less than, or equal to the effective log level limit of this logger
    /// (\ref get_log_level_limit).
    ///
    bool will_log(LogLevel) const noexcept;

    /// \brief Get effective log level limit.
    ///
    /// This function returns the effective log level limit of this logger.
    ///
    LogLevel get_log_level_limit() const noexcept;

    /// \brief Construct logger targetting STDERR.
    ///
    /// Construct a logger that is equivalent to the one returned by
    /// BasicLogger::get_cerr().
    ///
    /// This constructor is a shorthand for
    /// `BasicLogger(BasicLogger::get_cerr())`.
    ///
    BasicLogger();

    /// \brief Construct equivalent logger.
    ///
    /// Construct a logger that is equivalent to the specified base logger (\p
    /// base_logger).
    ///
    /// This constructor is a shorthand for
    /// `BasicLogger(base_logger.get_prefix(), base_logger.get_channel(),
    /// base_logger.get_map())`.
    ///
    explicit BasicLogger(BasicLogger& base_logger) noexcept;

    /// \brief Construct logger with different selected channel.
    ///
    /// Construct a copy of the specified base logger (\p base_logger), but make
    /// the new logger log to the specified channel instead of to the channel
    /// selected in the base logger.
    ///
    /// This constructor is a shorthand for
    /// `BasicLogger(base_logger.get_prefix(),
    /// base_logger.find_channel(channel), base_logger.get_map())`.
    ///
    BasicLogger(BasicLogger& base_logger, std::string_view channel);

    class Limit;
    class RootLimit;
    class NullLimit;
    class Prefix;
    class NullPrefix;
    class CompoundPrefix;
    class Channel;
    class Map;
    class Sink;

    BasicLogger(const Prefix&, Channel&, Map&) noexcept;

    const Prefix& get_prefix() const noexcept;
    Channel& get_channel() noexcept;
    const Channel& get_channel() const noexcept;
    Map& get_map() noexcept;
    const Map& get_map() const noexcept;

    Channel& find_channel(std::string_view name);
    const Channel& find_channel(std::string_view name) const;

    /// \brief Get locale of associated sink.
    ///
    /// This function is a shorthand for calling
    /// `get_channel().get_sink().get_locale()`.
    ///
    const std::locale& get_locale() const noexcept;

    virtual ~BasicLogger() noexcept = default;

protected:
    BasicLogger(const Limit&, const Prefix&, Channel&, Map&) noexcept;
    BasicLogger(int fixed_limit, const Limit&, const Prefix&, Channel&, Map&) noexcept;

private:
    const int m_fixed_limit;
    const Limit& m_limit;
    const Prefix& m_prefix;
    Channel& m_channel;
    Map& m_map;
};


using Logger     = BasicLogger<char>;
using WideLogger = BasicLogger<wchar_t>;




template<class C, class T> class BasicLogger<C, T>::Limit {
public:
    virtual int get_fixed_limit() const noexcept = 0;
    virtual LogLevel get_level_limit() const noexcept = 0;

protected:
    ~Limit() = default;
};




template<class C, class T> class BasicLogger<C, T>::RootLimit :
        public Limit {
public:
    // Overriding functions from Limit
    int get_fixed_limit() const noexcept override final;
    LogLevel get_level_limit() const noexcept override final;
};




template<class C, class T> class BasicLogger<C, T>::NullLimit :
        public Limit {
public:
    // Overriding functions from Limit
    int get_fixed_limit() const noexcept override final;
    LogLevel get_level_limit() const noexcept override final;
};




template<class C, class T> class BasicLogger<C, T>::Prefix {
public:
    virtual bool is_null_prefix() const noexcept;
    virtual void format_prefix(ostream_type&) const = 0;

protected:
    ~Prefix() = default;
};



template<class C, class T> class BasicLogger<C, T>::NullPrefix :
        public Prefix {
public:
    // Overriding functions from Prefix
    bool is_null_prefix() const noexcept override final;
    void format_prefix(ostream_type&) const override final;
};




template<class C, class T> class BasicLogger<C, T>::CompoundPrefix final :
        public Prefix {
public:
    CompoundPrefix(const Prefix& left, const Prefix& right) noexcept;

    const Prefix& get_simplified() const noexcept;

    // Overriding functions from Prefix
    void format_prefix(ostream_type&) const override final;

private:
    const Prefix& m_left;
    const Prefix& m_right;
};




template<class C, class T> class BasicLogger<C, T>::Channel {
public:
    template<class... P>
    void channel_log(LogLevel, const Prefix&, const char* message, const P&... params);
    template<class... P>
    void channel_log(LogLevel, const Prefix&, string_view_type message, const P&... params);

    Channel(std::string_view name, const Limit&, const Prefix&, Sink&) noexcept;

    std::string_view get_name() const noexcept;
    const Limit& get_limit() const noexcept;
    const Prefix& get_prefix() const noexcept;
    Sink& get_sink() noexcept;
    const Sink& get_sink() const noexcept;

private:
    static constexpr std::size_t s_format_seed_memory_size = 2048;

    std::string_view m_name;
    const Limit& m_limit;
    const Prefix& m_prefix;
    Sink& m_sink;
};




template<class C, class T> class BasicLogger<C, T>::Map :
        public NullPrefix {
public:
    Channel& find_channel(std::string_view name);
    const Channel& find_channel(std::string_view name) const;

    // Sorted ascendingly by channel name
    Span<Channel> get_channels() noexcept;
    Span<const Channel> get_channels() const noexcept;

protected:
    ~Map() noexcept = default;

    static const Channel* do_find_channel(Span<const Channel>, std::string_view name) noexcept;

    virtual Span<const Channel> do_get_channels() const noexcept = 0;
};




template<class C, class T> class BasicLogger<C, T>::Sink {
public:
    static const char* get_level_prefix(LogLevel);

    const std::locale& get_locale() const noexcept;

    virtual void sink_log(LogLevel, const Prefix& channel_prefix, const Prefix& message_prefix,
                          string_view_type message) = 0;

protected:
    Sink(const std::locale&) noexcept;

    ~Sink() noexcept = default;

private:
    const std::locale m_locale;
};




template<class C, class T = std::char_traits<C>> class BasicRootLogger :
        protected BasicLogger<C, T>::Sink, // Not private due to VisualStudio bug
        private BasicLogger<C, T>::Map,
        private BasicLogger<C, T>::RootLimit,
        public BasicLogger<C, T> {
public:
    using logger_type = BasicLogger<C, T>;

    using string_view_type = typename logger_type::string_view_type;
    using ostream_type     = typename logger_type::ostream_type;

    using Prefix  = typename logger_type::Prefix;
    using Channel = typename logger_type::Channel;
    using Sink    = typename logger_type::Sink;

protected:
    BasicRootLogger(const std::locale&);

    virtual void format_log_level(LogLevel, ostream_type&) const;
    virtual void root_log(string_view_type message) = 0;

private:
    Channel m_channel;
    std::mutex m_mutex;
    Buffer<C> m_buffer; // Protected by `m_mutex`
    BasicBufferOutputStreambuf<C, T> m_streambuf; // Protected by `m_mutex`
    ostream_type m_out; // Protected by `m_mutex`
    const C m_newline;
    Buffer<C> m_assembly_buffer;

    // Overriding functions from Sink
    void sink_log(LogLevel, const Prefix&, const Prefix&, string_view_type) override final;

    // Overriding functions from Map
    Span<const Channel> do_get_channels() const noexcept override final;
};


using RootLogger     = BasicRootLogger<char>;
using WideRootLogger = BasicRootLogger<wchar_t>;




/// \brief A logger that sends messages to a stream.
///
/// This is a root logger that sends messages to the specified stream.
///
/// An instance of \ref BasicStreamLogger is thread-safe in so far as the                       
/// targeted stream is not accessed in ways other than through the logger
/// instance during the entire lifetime of the logger instance.
///
template<class C, class T = std::char_traits<C>> class BasicStreamLogger final :
        public BasicRootLogger<C, T> {
public:
    using root_logger_type = BasicRootLogger<C, T>;

    using string_view_type = typename root_logger_type::string_view_type;
    using ostream_type     = typename root_logger_type::ostream_type;

    explicit BasicStreamLogger(ostream_type&);

protected:
    // Overriding functions from BasicRootLogger<C, T>
    void root_log(string_view_type message) override final;

private:
    ostream_type& m_out;
};


using StreamLogger     = BasicStreamLogger<char>;
using WideStreamLogger = BasicStreamLogger<wchar_t>;




class FileLoggerBase {
public:
    enum class Colorize { detect, no, yes };
};




template<class C, class T = std::char_traits<C>> class BasicFileLogger final :
        public FileLoggerBase,
        public BasicRootLogger<C, T> {
public:
    struct Config;

    using root_logger_type = BasicRootLogger<C, T>;

    using string_view_type = typename root_logger_type::string_view_type;
    using ostream_type     = typename root_logger_type::ostream_type;

    using Sink = typename root_logger_type::Sink;

    /// \{
    ///
    /// The specified file will be opened in "append" mode (\ref
    /// File::Mode::append).
    ///
    explicit BasicFileLogger(FilesystemPathRef);
    BasicFileLogger(FilesystemPathRef, Config);
    /// \}

    explicit BasicFileLogger(File&);
    BasicFileLogger(File&, Config);

protected:
    // Overriding functions from BasicRootLogger<C, T>
    void format_log_level(LogLevel, ostream_type&) const override final;
    void root_log(string_view_type) override final;

private:
    File m_owned_file;
    File& m_file;
    const bool m_colorize;
    BasicStringEncoder<C, T> m_encoder;

    BasicFileLogger(File owned_file, File& file, Config);
};


using FileLogger     = BasicFileLogger<char>;
using WideFileLogger = BasicFileLogger<wchar_t>;




template<class C, class T> struct BasicFileLogger<C, T>::Config {
    std::locale locale;
    Colorize colorize = Colorize::detect;
};




template<class C, class T = std::char_traits<C>> class BasicNullLogger final :
        private BasicLogger<C, T>::Sink,
        private BasicLogger<C, T>::Map,
        private BasicLogger<C, T>::NullLimit,
        public BasicLogger<C, T> {
public:
    using logger_type = BasicLogger<C, T>;

    using string_view_type = typename logger_type::string_view_type;
    using ostream_type     = typename logger_type::ostream_type;

    using Prefix  = typename logger_type::Prefix;
    using Channel = typename logger_type::Channel;
    using Sink    = typename logger_type::Sink;

    explicit BasicNullLogger(const std::locale& = {}) noexcept;

private:
    Channel m_channel;

    // Overriding functions from Sink
    void sink_log(LogLevel, const Prefix&, const Prefix&, string_view_type) override final;

    // Overriding functions from Map
    Span<const Channel> do_get_channels() const noexcept override final;
};


using NullLogger     = BasicNullLogger<char>;
using WideNullLogger = BasicNullLogger<wchar_t>;








// Implementation


inline EnumAssoc EnumTraits<LogLevel>::Spec::map[] = {
    { int(LogLevel::off),    "off"    },
    { int(LogLevel::fatal),  "fatal"  },
    { int(LogLevel::error),  "error"  },
    { int(LogLevel::warn),   "warn"   },
    { int(LogLevel::info),   "info"   },
    { int(LogLevel::detail), "detail" },
    { int(LogLevel::debug),  "debug"  },
    { int(LogLevel::trace),  "trace"  },
    { int(LogLevel::all),    "all"    }
};


// ============================ BasicLogger ============================


namespace detail {

template<class C, class T> class Loggers {
public:
    static BasicNullLogger<C, T> null;
    static Loggers& get()
    {
        static Loggers loggers; // Throws
        return loggers;
    }
    BasicFileLogger<C, T> cout, cerr;
    Loggers() :
        cout(File::cout), // Throws
        cerr(File::cerr)  // Throws
    {
    }
};

template<class C, class T> BasicNullLogger<C, T> Loggers<C, T>::null;

} // namespace detail


template<class C, class T> BasicLogger<C, T> BasicLogger<C,T>::null(detail::Loggers<C, T>::null);


template<class C, class T> BasicLogger<C, T>& BasicLogger<C,T>::get_cout()
{
    auto& loggers = detail::Loggers<C, T>::get(); // Throws
    return loggers.cout;
}


template<class C, class T> BasicLogger<C, T>& BasicLogger<C,T>::get_cerr()
{
    auto& loggers = detail::Loggers<C, T>::get(); // Throws
    return loggers.cerr;
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::fatal(const char* message, const P&... params)
{
    log(LogLevel::fatal, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::error(const char* message, const P&... params)
{
    log(LogLevel::error, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::warn(const char* message, const P&... params)
{
    log(LogLevel::warn, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::info(const char* message, const P&... params)
{
    log(LogLevel::info, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::detail(const char* message, const P&... params)
{
    log(LogLevel::detail, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::debug(const char* message, const P&... params)
{
    log(LogLevel::debug, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::trace(const char* message, const P&... params)
{
    log(LogLevel::trace, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::fatal(string_view_type message, const P&... params)
{
    log(LogLevel::fatal, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::error(string_view_type message, const P&... params)
{
    log(LogLevel::error, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::warn(string_view_type message, const P&... params)
{
    log(LogLevel::warn, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::info(string_view_type message, const P&... params)
{
    log(LogLevel::info, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::detail(string_view_type message, const P&... params)
{
    log(LogLevel::detail, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::debug(string_view_type message, const P&... params)
{
    log(LogLevel::debug, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C,T>::trace(string_view_type message, const P&... params)
{
    log(LogLevel::trace, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C, T>::log(LogLevel level, const char* message, const P&... params)
{
    if (ARCHON_LIKELY(!will_log(level)))
        return;
    m_channel.channel_log(level, m_prefix, message, params...); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C, T>::log(LogLevel level, string_view_type message, const P&... params)
{
    if (ARCHON_LIKELY(!will_log(level)))
        return;
    m_channel.channel_log(level, m_prefix, message, params...); // Throws
}


template<class C, class T> inline bool BasicLogger<C, T>::will_log(LogLevel level) const noexcept
{
    int level_2 = int(level);
    if (ARCHON_LIKELY(level_2 > m_fixed_limit))
        return false;
    bool has_fixed_limit = (m_fixed_limit != std::numeric_limits<int>::max());
    if (ARCHON_LIKELY(has_fixed_limit))
        return true;
    return (level_2 <= int(m_limit.get_level_limit()));
}


template<class C, class T> inline LogLevel BasicLogger<C, T>::get_log_level_limit() const noexcept
{
    bool has_fixed_limit = (m_fixed_limit != std::numeric_limits<int>::max());
    if (ARCHON_LIKELY(has_fixed_limit))
        return LogLevel(m_fixed_limit);
    return m_limit.get_level_limit();
}


template<class C, class T> inline BasicLogger<C, T>::BasicLogger() :
    BasicLogger(BasicLogger::get_cerr()) // Throws
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(BasicLogger& base_logger) noexcept :
    BasicLogger(base_logger.get_prefix(), base_logger.get_channel(), base_logger.get_map())
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(BasicLogger& base_logger, std::string_view channel) :
    BasicLogger(base_logger.get_prefix(), base_logger.find_channel(channel),
                base_logger.get_map()) // Throws
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(const Prefix& prefix, Channel& channel, Map& map) noexcept :
    BasicLogger(channel.get_limit(), prefix, channel, map)
{
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_prefix() const noexcept -> const Prefix&
{
    return m_prefix;
}


template<class C, class T> inline auto BasicLogger<C, T>::get_channel() noexcept -> Channel&
{
    return m_channel;
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_channel() const noexcept -> const Channel&
{
    return m_channel;
}


template<class C, class T> inline auto BasicLogger<C, T>::get_map() noexcept -> Map&
{
    return m_map;
}


template<class C, class T> inline auto BasicLogger<C, T>::get_map() const noexcept -> const Map&
{
    return m_map;
}


template<class C, class T>
inline auto BasicLogger<C, T>::find_channel(std::string_view name) -> Channel&
{
    return get_map().find_channel(name); // Throws
}


template<class C, class T>
inline auto BasicLogger<C, T>::find_channel(std::string_view name) const -> const Channel&
{
    return get_map().find_channel(name); // Throws
}


template<class C, class T> inline const std::locale& BasicLogger<C, T>::get_locale() const noexcept
{
    return get_channel().get_sink().get_locale();
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(const Limit& limit, const Prefix& prefix, Channel& channel,
                                      Map& map) noexcept :
    BasicLogger(limit.get_fixed_limit(), limit, prefix, channel, map)
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(int fixed_limit, const Limit& limit, const Prefix& prefix,
                                      Channel& channel, Map& map) noexcept :
    m_fixed_limit(fixed_limit),
    m_limit(limit),
    m_prefix(prefix),
    m_channel(channel),
    m_map(map)
{
}


// ============================ BasicLogger::RootLimit ============================


template<class C, class T> int BasicLogger<C, T>::RootLimit::get_fixed_limit() const noexcept
{
    return int(LogLevel::all);
}


template<class C, class T> LogLevel BasicLogger<C, T>::RootLimit::get_level_limit() const noexcept
{
    return LogLevel::all;
}


// ============================ BasicLogger::NullLimit ============================


template<class C, class T> int BasicLogger<C, T>::NullLimit::get_fixed_limit() const noexcept
{
    return int(LogLevel::off);
}


template<class C, class T> LogLevel BasicLogger<C, T>::NullLimit::get_level_limit() const noexcept
{
    return LogLevel::off;
}


// ============================ BasicLogger::Prefix ============================


template<class C, class T> bool BasicLogger<C, T>::Prefix::is_null_prefix() const noexcept
{
    return false;
}


// ============================ BasicLogger::NullPrefix ============================


template<class C, class T> bool BasicLogger<C, T>::NullPrefix::is_null_prefix() const noexcept
{
    return true;
}


template<class C, class T> void BasicLogger<C, T>::NullPrefix::format_prefix(ostream_type&) const
{
}


// ============================ BasicLogger::CompoundPrefix ============================


template<class C, class T>
inline BasicLogger<C, T>::CompoundPrefix::CompoundPrefix(const Prefix& left,
                                                         const Prefix& right) noexcept :
    m_left(left),
    m_right(right)
{
}


template<class C, class T>
auto BasicLogger<C, T>::CompoundPrefix::get_simplified() const noexcept -> const Prefix&
{
    if (m_right.is_null_prefix())
        return m_left;
    if (m_left.is_null_prefix())
        return m_right;
    return *this;
}


template<class C, class T>
void BasicLogger<C, T>::CompoundPrefix::format_prefix(ostream_type& out) const
{
    m_left.format_prefix(out); // Throws
    m_right.format_prefix(out); // Throws
}


// ============================ BasicLogger::Channel ============================


template<class C, class T> template<class... P>
inline void BasicLogger<C, T>::Channel::channel_log(LogLevel level, const Prefix& prefix,
                                                    const char* message, const P&... params)
{
    std::array<C, s_format_seed_memory_size> seed_memory;
    BasicStringFormatter<C, T> formatter(seed_memory, m_sink.get_locale()); // Throws
    string_view_type message_2 = formatter.format(message, params...); // Throws
    m_sink.sink_log(level, m_prefix, prefix, message_2); // Throws
}


template<class C, class T> template<class... P>
inline void BasicLogger<C, T>::Channel::channel_log(LogLevel level, const Prefix& prefix,
                                                    string_view_type message, const P&... params)
{
    std::array<C, s_format_seed_memory_size> seed_memory;
    BasicStringFormatter<C, T> formatter(seed_memory, m_sink.get_locale()); // Throws
    string_view_type message_2 = formatter.format(message, params...); // Throws
    m_sink.sink_log(level, m_prefix, prefix, message_2); // Throws
}


template<class C, class T>
inline BasicLogger<C, T>::Channel::Channel(std::string_view name, const Limit& limit,
                                           const Prefix& prefix, Sink& sink) noexcept :
    m_name(name),
    m_limit(limit),
    m_prefix(prefix),
    m_sink(sink)
{
}


template<class C, class T>
inline std::string_view BasicLogger<C, T>::Channel::get_name() const noexcept
{
    return m_name;
}


template<class C, class T>
inline auto BasicLogger<C, T>::Channel::get_limit() const noexcept -> const Limit&
{
    return m_limit;
}


template<class C, class T>
inline auto BasicLogger<C, T>::Channel::get_prefix() const noexcept -> const Prefix&
{
    return m_prefix;
}


template<class C, class T> inline auto BasicLogger<C, T>::Channel::get_sink() noexcept -> Sink&
{
    return m_sink;
}


template<class C, class T>
inline auto BasicLogger<C, T>::Channel::get_sink() const noexcept -> const Sink&
{
    return m_sink;
}


// ============================ BasicLogger::Map ============================


template<class C, class T>
inline auto BasicLogger<C, T>::Map::find_channel(std::string_view name) -> Channel&
{
    const Channel& channel = std::as_const(*this).find_channel(name); // Throws
    return const_cast<Channel&>(channel); // Throws
}


template<class C, class T>
inline auto BasicLogger<C, T>::Map::find_channel(std::string_view name) const -> const Channel&
{
    Span<const Channel> channels = get_channels();
    const Channel* channel = do_find_channel(channels, name);
    if (ARCHON_LIKELY(channel))
        return *channel;
    throw std::invalid_argument("No such channel");
}


template<class C, class T>
inline auto BasicLogger<C, T>::Map::get_channels() noexcept -> Span<Channel>
{
    auto channels = do_get_channels();
    return { const_cast<Channel*>(channels.data()), channels.size() };
}


template<class C, class T>
inline auto BasicLogger<C, T>::Map::get_channels() const noexcept -> Span<const Channel>
{
    return do_get_channels();
}


template<class C, class T>
auto BasicLogger<C, T>::Map::do_find_channel(Span<const Channel> channels,
                                             std::string_view name) noexcept -> const Channel*
{
    auto comp = [](const Channel& channel, const std::string_view& name) noexcept {
        return (channel.get_name() < name);
    };
    auto i = std::lower_bound(channels.begin(), channels.end(), name, comp);
    bool found = (i != channels.end() && i->get_name() == name);
    if (ARCHON_LIKELY(found))
        return &*i;
    return nullptr;
}


// ============================ BasicLogger::Sink ============================


template<class C, class T> const char* BasicLogger<C, T>::Sink::get_level_prefix(LogLevel level)
{
    switch (level) {
        case LogLevel::all:
        case LogLevel::trace:
        case LogLevel::debug:
        case LogLevel::detail:
        case LogLevel::info:
        case LogLevel::off:
            return "";
        case LogLevel::warn:
            return "WARNING: ";
        case LogLevel::error:
            return "ERROR: ";
        case LogLevel::fatal:
            return "FATAL: ";
    }
    ARCHON_ASSERT(false);
    return nullptr;
}


template<class C, class T>
inline const std::locale& BasicLogger<C, T>::Sink::get_locale() const noexcept
{
    return m_locale;
}


template<class C, class T>
inline BasicLogger<C, T>::Sink::Sink(const std::locale& locale) noexcept :
    m_locale(locale)
{
}


// ============================ BasicRootLogger ============================

template<class C, class T> BasicRootLogger<C, T>::BasicRootLogger(const std::locale& locale) :
    Sink(locale),
    logger_type(*this, *this, m_channel, *this),
    m_channel("", *this, *this, *this),
    m_streambuf(m_buffer), // Throws
    m_out(&m_streambuf), // Throws
    m_newline(m_out.widen('\n')) // Throws
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out.imbue(locale); // Throws
}


template<class C, class T>
void BasicRootLogger<C, T>::format_log_level(LogLevel level, ostream_type& out) const
{
    out << Sink::get_level_prefix(level); // Throws
}


template<class C, class T>
void BasicRootLogger<C, T>::sink_log(LogLevel level, const Prefix& channel_prefix,
                                     const Prefix& message_prefix, string_view_type message)
{
    std::lock_guard lock(m_mutex);
    m_streambuf.set_offset(0);
    m_out.clear();
    channel_prefix.format_prefix(m_out); // Throws
    message_prefix.format_prefix(m_out); // Throws
    format_log_level(level, m_out); // Throws
    C newline = m_newline;
    string_view_type message_2;
    std::size_t i = 0;
    std::size_t j = message.find(newline);
    if (ARCHON_LIKELY(j == string_view_type::npos)) {
        m_out << message << newline; // Throws
        message_2 = m_streambuf.view();
    }
    else {
        std::size_t size = 0;
        auto append = [&](string_view_type string) {
            m_assembly_buffer.reserve_extra(string.size(), size); // Throws
            std::copy_n(string.data(), string.size(), m_assembly_buffer.data() + size);
            size += string.size();
        };
        string_view_type prefix = m_streambuf.view();
        do {
            ++j;
            append(prefix); // Throws
            append(message.substr(i, j - i)); // Throws
            i = j;
            j = message.find(newline, i);
        }
        while (j != string_view_type::npos);
        append(prefix); // Throws
        append(message.substr(i)); // Throws
        append({ &newline, 1 }); // Throws
        message_2 = { m_assembly_buffer.data(), size };
    }
    root_log(message_2); // throws
}


template<class C, class T>
auto BasicRootLogger<C, T>::do_get_channels() const noexcept -> Span<const Channel>
{
    return { &m_channel, 1 };
}


// ============================ BasicStreamLogger ============================


template<class C, class T> inline BasicStreamLogger<C, T>::BasicStreamLogger(ostream_type& out) :
    root_logger_type(out.getloc()), // Throws
    m_out(out)
{
}


template<class C, class T> void BasicStreamLogger<C, T>::root_log(string_view_type message)
{
    m_out.write(message.data(), message.size()); // Throws
    m_out.flush(); // Throws
}


// ============================ BasicFileLogger ============================


template<class C, class T> inline BasicFileLogger<C,T>::BasicFileLogger(FilesystemPathRef path) :
    BasicFileLogger(path, {}) // Throws
{
}


template<class C, class T>
inline BasicFileLogger<C, T>::BasicFileLogger(FilesystemPathRef path, Config config) :
    BasicFileLogger(File(path, File::Mode::append), m_owned_file, std::move(config)) // Throws
{
}


template<class C, class T> inline BasicFileLogger<C,T>::BasicFileLogger(File& file) :
    BasicFileLogger(file, {}) // Throws
{
}


template<class C, class T>
inline BasicFileLogger<C, T>::BasicFileLogger(File& file, Config config) :
    BasicFileLogger({}, file, std::move(config)) // Throws
{
}


template<class C, class T>
void BasicFileLogger<C, T>::format_log_level(LogLevel level, ostream_type& out) const
{
    using namespace archon::base::terminal;
    bool colorize = false;
    Color color = Color::red;
    Weight weight = Weight::normal;
    if (m_colorize) {
        switch (level) {
            case LogLevel::all:
            case LogLevel::trace:
            case LogLevel::debug:
            case LogLevel::detail:
            case LogLevel::info:
            case LogLevel::off:
                break;
            case LogLevel::warn:
                color = Color::yellow;
                colorize = true;
                break;
            case LogLevel::error:
            case LogLevel::fatal:
                weight = Weight::bold;
                colorize = true;
                break;
        }
    }
    if (ARCHON_LIKELY(!colorize)) {
        out << Sink::get_level_prefix(level); // Throws
    }
    else {
        using namespace archon::base::terminal;
        out << seq::set_weight(weight) << seq::set_color(color); // Throws
        out << Sink::get_level_prefix(level); // Throws
        out << seq::reset_color() << seq::set_weight(Weight::normal); // Throws
    }
}


template<class C, class T> void BasicFileLogger<C,T>::root_log(string_view_type message)
{
    std::string_view message_2 = m_encoder.encode(message); // Throws
    m_file.write(message_2); // Throws
}


template<class C, class T>
inline BasicFileLogger<C, T>::BasicFileLogger(File owned_file, File& file, Config config) :
    root_logger_type(config.locale), // Throws
    m_owned_file(std::move(owned_file)),
    m_file(file),
    m_colorize(config.colorize == Colorize::yes ||
               (config.colorize == Colorize::detect && file.is_terminal())),
    m_encoder(config.locale)
{
}


// ============================ BasicNullLogger ============================


template<class C, class T>
inline BasicNullLogger<C, T>::BasicNullLogger(const std::locale& locale) noexcept :
    Sink(locale),
    logger_type(*this, *this, m_channel, *this),
    m_channel("", *this, *this, *this)
{
}


template<class C, class T>
void BasicNullLogger<C, T>::sink_log(LogLevel, const Prefix&, const Prefix&, string_view_type)
{
    // Intended no-op
}


template<class C, class T>
auto BasicNullLogger<C, T>::do_get_channels() const noexcept -> Span<const Channel>
{
    return { &m_channel, 1 };
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_LOGGER_HPP
