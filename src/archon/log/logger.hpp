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

#ifndef ARCHON_X_LOG_X_LOGGER_HPP
#define ARCHON_X_LOG_X_LOGGER_HPP

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
#include <iostream>    

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/locale.hpp>
#include <archon/core/text_codec.hpp>
#include <archon/core/seed_memory_output_stream.hpp>
#include <archon/core/string_formatter.hpp>
#include <archon/core/file.hpp>
#include <archon/core/terminal.hpp>
#include <archon/log/log_level.hpp>
#include <archon/log/limit.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/sink.hpp>
#include <archon/log/channel.hpp>
#include <archon/log/channel_map.hpp>


namespace archon::log {


/// \brief A general purpose logger.
///
/// Messages are submitted to the logger with an accompanying log level (\ref
/// log::LogLevel). The log level can be thought of as specifying the verbosity level of the
/// message. The logger checks this log level against an effective limit (\ref
/// get_log_level_limit()), and suppresses the message if the level is too high. For
/// example, a message logged at level `detail` will be suppressed if the effective limit is
/// `info`, but will not be suppressed if the limit is `detail`. For the sake of
/// performance, this filtering happens before messages are formatted. This ensures that the
/// cost of message submission is very low for messages that end up being discarded.
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
/// FIXME: Explain level limiting and refer to \ref log::BasicLimitLogger.
///
/// FIXME: Explain fixed versus variable log level limit.
///
/// FIXME: Explain low cost of not logging when using fixed log level limit.
///
/// FIXME: Explain prefixes and refer to \ref log::BasicPrefixLogger.
///
/// FIXME: Explain channels rougly. Full explanation can be deferred to \ref
/// log::BasicChannelLogger.
///
/// FIXME: Make reference to \ref log::BasicRootLogger.
///
/// FIXME: Explain thread safety.
///
/// FIXME: Explain that in general, when a logger is used as base logger during the
/// construction of a new logger, it is the responsibility of the application to ensure that
/// the life of the base logger extends at least until the end of life of the new logger.
///
template<class C, class T = std::char_traits<C>> class BasicLogger {
public:
    using char_type   = C;
    using traits_type = T;

    using string_view_type = std::basic_string_view<C, T>;
    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;
    using channel_map_type = log::BasicChannelMap<C, T>;

    /// \brief A null logger.
    ///
    /// This function returns returns to a null logger, which is a logger that discards all
    /// logged messages. More concretely, this function returns a reference to an instance
    /// of \ref log::BasicNullLogger.
    ///
    static auto get_null() noexcept -> BasicLogger&;

    /// \brief Use null logger as fallback.
    ///
    /// If a logger is specified, this function returns that logger, otherwise it returns
    /// the null logger (\ref get_null()).
    ///
    static auto or_null(BasicLogger*) noexcept -> BasicLogger&;

    /// \{
    ///
    /// \brief Console loggers for STDOUT and STDERR.
    ///
    /// These loggers send log messages to STDOUT and STDERR respectively. They are
    /// implemented in terms of \ref log::BasicFileLogger with respective arguments of \ref
    /// core::File::get_cout() and \ref core::File::get_cerr(), and with colorization mode
    /// set to \ref log::BasicFileLogger::Colorize::detect.
    ///
    /// The first time \ref get_cout() or \ref get_cerr() is called, both streams are
    /// created based on the currently selected global locale. Therefore, if the application
    /// needs these loggers to use a particular locale, it must update the global locale
    /// before any of these functions are called.
    ///
    static auto get_cout() -> BasicLogger&;
    static auto get_cerr() -> BasicLogger&;
    /// \}

    /// \{
    ///
    /// \brief Log a null-terminated parameterized message.
    ///
    /// These functions are shorthands for passing the corresponding log level (\ref
    /// log::LogLevel) to \ref log(LogLevel, const char*, const P&...).
    ///
    /// The characters of the specified message will be widened as if by `widen()` of an
    /// output stream with the same character type as this logger (`std::basic_ostream<C,
    /// T>`), and imbued with the locale associated with this logger (the locale of the sink
    /// of the channel of this logger). It is therefore only safe to use characters from the
    /// basic source character set in \p message. The intention is that \p message is always
    /// a string literal. This mimics the intention of `widen()`.
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
    /// These functions are shorthands for passing the corresponding log level (\ref
    /// log::LogLevel) to \ref log(log::LogLevel, string_view_type, const P&...).
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
    /// This function has the same effect as \ref log(log::LogLevel, string_view_type, const
    /// P&...) except that the message (\p message) is specified as a null-terminated string
    /// (a string literal).
    ///
    /// The characters of the specified message will be widened as if by `widen()` of an
    /// output stream with the same character type as this logger (`std::basic_ostream<C,
    /// T>`), and imbued with the locale associated with this logger (the locale of the sink
    /// of the channel of this logger). It is therefore only safe to use characters from the
    /// basic source character set in \p message. The intention is that \p message is always
    /// a string literal. This mimics the intention of `widen()`.
    ///
    template<class... P> void log(log::LogLevel, const char* message, const P&... params);

    /// \brief Log a parameterized message at any level.
    ///
    /// This function logs the specified message at the specified log level provided that
    /// \ref will_log() would return true for the specified log level. What it means to log
    /// a message depends on what kind of logger this is. If this logger is the logger
    /// returned by \ref get_cerr(), for instance, logging a message means writing that
    /// message to STDERR.
    ///
    /// In any case, the specified parameterized message (\p message) is formatted as if it
    /// was passed to \ref core::format(std::basic_ostream<C, T>&, std::basic_string_view<C,
    /// T>, const P&...) along with the specified parameter values (\p params) for an output
    /// stream imbued with the locale associated with this logger (the locale of the sink of
    /// the channel of this logger).
    ///
    template<class... P> void log(log::LogLevel, string_view_type message, const P&... params);

    /// \brief Will log at specified level?
    ///
    /// This function returns true if, and only if the specified log level is less than, or
    /// equal to the effective log level limit of this logger (\ref get_log_level_limit).
    ///
    bool will_log(log::LogLevel) const noexcept;

    /// \brief Get effective log level limit.
    ///
    /// This function returns the effective log level limit of this logger.
    ///
    auto get_log_level_limit() const noexcept -> log::LogLevel;

    /// \brief Construct logger targetting STDERR.
    ///
    /// Construct a logger that is equivalent to the one returned by
    /// BasicLogger::get_cerr().
    ///
    /// This constructor is a shorthand for `BasicLogger(BasicLogger::get_cerr())`.
    ///
    BasicLogger();

    /// \brief Construct equivalent logger.
    ///
    /// Construct a logger that is equivalent to the specified base logger (\p base_logger).
    ///
    /// This constructor is a shorthand for `BasicLogger(base_logger.get_prefix(),
    /// base_logger.get_channel(), base_logger.get_channel_map())`.
    ///
    explicit BasicLogger(BasicLogger& base_logger) noexcept;

    /// \brief Construct logger with different selected channel.
    ///
    /// Construct a copy of the specified base logger (\p base_logger), but make the new
    /// logger log to the specified channel instead of to the channel selected in the base
    /// logger.
    ///
    /// This constructor is a shorthand for `BasicLogger(base_logger.get_prefix(),
    /// base_logger.find_channel(channel), base_logger.get_channel_map())`.
    ///
    BasicLogger(BasicLogger& base_logger, std::string_view channel);

    BasicLogger(const prefix_type&, channel_type&, channel_map_type&) noexcept;

    auto get_prefix() const noexcept -> const prefix_type&;
    auto get_channel() noexcept -> channel_type&;
    auto get_channel() const noexcept -> const channel_type&;
    auto get_channel_map() noexcept -> channel_map_type&;
    auto get_channel_map() const noexcept -> const channel_map_type&;

    auto find_channel(std::string_view name)       -> channel_type&;
    auto find_channel(std::string_view name) const -> const channel_type&;

    /// \brief Get locale of associated sink.
    ///
    /// This function is a shorthand for calling `get_channel().get_sink().get_locale()`.
    ///
    auto get_locale() const noexcept -> const std::locale&;

    virtual ~BasicLogger() noexcept = default;

//protected:
    BasicLogger(const log::Limit&, const prefix_type&, channel_type&, channel_map_type&) noexcept;
    BasicLogger(int fixed_limit, const log::Limit&, const prefix_type&, channel_type&, channel_map_type&) noexcept;

    /// \brief Change prefix of logger (caution).
    ///
    /// This function changes the prefix of the logger.
    ///
    /// CAUTION: To ensure thread safety, this function must not be called while other
    /// threads might access this logger.
    ///
    void set_prefix(const prefix_type&) noexcept;

//private:
    const int m_fixed_limit;
    const log::Limit& m_limit;
    const prefix_type* m_prefix;
    channel_type& m_channel;
    channel_map_type& m_channel_map;
};


using Logger     = BasicLogger<char>;
using WideLogger = BasicLogger<wchar_t>;




// `log::BasicSink<C, T>` should have been a private base, but needs to be non-private
// because of a bug in Visual Studio (2019 and 2022).
template<class C, class T = std::char_traits<C>> class BasicRootLogger
    : public log::BasicSink<C, T>
    , public log::BasicChannelMap<C, T>
    , public log::RootLimit
    , public log::BasicLogger<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;
    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;

//protected:  
    BasicRootLogger(const std::locale&);

    virtual void format_log_level(log::LogLevel, ostream_type&) const;
    virtual void root_log(string_view_type message) = 0;

//private:  
    using logger_type = log::BasicLogger<C, T>;

    channel_type m_channel;
    std::mutex m_mutex;
    core::BasicSeedMemoryOutputStream<C, T> m_out; // Protected by `m_mutex`
    core::Buffer<C> m_assembly_buffer; // Protected by `m_mutex`
    C m_newline;

    // Overriding functions from log::BasicSink<C, T>
    void sink_log(log::LogLevel, const prefix_type&, const prefix_type&, string_view_type) override final;

    // Overriding functions from log::BasicChannelMap<C, T>
    auto do_get_channels() const noexcept -> core::Span<const channel_type> override final;
};


using RootLogger     = BasicRootLogger<char>;
using WideRootLogger = BasicRootLogger<wchar_t>;




class FileLoggerBase {
public:
    /// \brief Choices for enabling of colorization.
    ///
    /// The possible choices for \ref log::BasicFileLogger::Config::colorize.
    ///
    enum class Colorize {
        detect, ///< Auto-detect
        no,     ///< Disable colorization
        yes,    ///< Enable colorization
    };
};


template<class C, class T = std::char_traits<C>, class I = core::TextCodecImpl<C, T>>
class BasicFileLogger
    : public log::FileLoggerBase
    , public log::BasicRootLogger<C, T> {
public:
    struct Config;

    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;
    using sink_type        = log::BasicSink<C, T>;

    using text_codec_type = core::GenericTextCodec<I>;

    /// \{
    ///
    /// The specified file will be opened in "append" mode (\ref core::File::Mode::append).
    ///
    explicit BasicFileLogger(core::FilesystemPathRef);
    BasicFileLogger(core::FilesystemPathRef, const std::locale&);
    BasicFileLogger(core::FilesystemPathRef, const std::locale&, Config);
    /// \}

    explicit BasicFileLogger(core::File&);
    BasicFileLogger(core::File&, const std::locale&);
    BasicFileLogger(core::File&, const std::locale&, Config);

protected:
    // Overriding functions from BasicRootLogger<C, T>
    void format_log_level(log::LogLevel, ostream_type&) const override final;
    void root_log(string_view_type) override final;

private:
    core::File m_owned_file;
    core::File& m_file;
    const bool m_colorize;
    const text_codec_type m_text_codec;
    typename text_codec_type::ShortCircuitEncodeBuffer m_encode_buffer;

    BasicFileLogger(core::File owned_file, core::File& file, const std::locale&, Config);
};


using FileLogger     = BasicFileLogger<char>;
using WideFileLogger = BasicFileLogger<wchar_t>;




/// \brief File logger configuration parameters.
///
/// These are the available parameters for configuring a file logger (\ref
/// core::BasicFileLogger).
///
template<class C, class T, class I> struct BasicFileLogger<C, T, I>::Config {
    /// \brief Control when colorization is enabled.
    ///
    /// This parameter controls the enabling of colorization, and other text styling of log
    /// messages using ANSI terminal escape sequences (see \ref
    /// core::terminal::seq::set_weight()).
    ///
    /// If set to `Colorize::yes`, colorization is eabled provided that \ref
    /// core::assume_locale_has_escape() returns `true` for the logger's locale.
    ///
    /// If set to `Colorize::no`, colorization is disabled.
    ///
    /// If set to `Colorize::detect`, colorization is enabled if \ref
    /// core::File::is_terminal() returns `true` for the associated file object and \ref
    /// core::assume_locale_has_escape() returns `true` for the logger's locale.
    ///
    Colorize colorize = Colorize::detect;

    typename text_codec_type::Config text_codec;
};




template<class C, class T = std::char_traits<C>> class BasicNullLogger final
    : private log::BasicSink<C, T>
    , private log::BasicChannelMap<C, T>
    , private log::NullLimit
    , public log::BasicLogger<C, T> {
public:
    using string_view_type = std::basic_string_view<C, T>;
    using prefix_type      = log::BasicPrefix<C, T>;
    using sink_type        = log::BasicSink<C, T>;
    using channel_type     = log::BasicChannel<C, T>;

    explicit BasicNullLogger(const std::locale& = {}) noexcept;

private:
    using logger_type = log::BasicLogger<C, T>;

    channel_type m_channel;

    // Overriding functions from log::BasicSink<C, T>
    void sink_log(log::LogLevel, const prefix_type&, const prefix_type&, string_view_type) override final;

    // Overriding functions from log::BasicChannelMap<C, T>
    auto do_get_channels() const noexcept -> core::Span<const channel_type> override final;
};


using NullLogger     = BasicNullLogger<char>;
using WideNullLogger = BasicNullLogger<wchar_t>;








// Implementation


// ============================ BasicLogger ============================


namespace impl {

template<class C, class T> class Loggers {
public:
    static auto get() -> Loggers&
    {
        static Loggers loggers; // Throws
        return loggers;
    }
    log::BasicFileLogger<C, T> cout, cerr;
    Loggers()
        : cout(core::File::get_cout()) // Throws
        , cerr(core::File::get_cerr()) // Throws
    {
    }
};

} // namespace impl


template<class C, class T>
auto BasicLogger<C, T>::get_null() noexcept -> BasicLogger&
{
    static log::BasicNullLogger<C, T> logger;
    return logger;
}


template<class C, class T>
inline auto BasicLogger<C, T>::or_null(BasicLogger* logger) noexcept -> BasicLogger&
{
    return (logger ? *logger : get_null());
}


template<class C, class T>
auto BasicLogger<C, T>::get_cout() -> BasicLogger&
{
    auto& loggers = impl::Loggers<C, T>::get(); // Throws
    return loggers.cout;
}


template<class C, class T>
auto BasicLogger<C, T>::get_cerr() -> BasicLogger&
{
    auto& loggers = impl::Loggers<C, T>::get(); // Throws
    return loggers.cerr;
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::fatal(const char* message, const P&... params)
{
    log(log::LogLevel::fatal, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::error(const char* message, const P&... params)
{
    log(log::LogLevel::error, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::warn(const char* message, const P&... params)
{
    log(log::LogLevel::warn, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::info(const char* message, const P&... params)
{
    log(log::LogLevel::info, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::detail(const char* message, const P&... params)
{
    log(log::LogLevel::detail, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::debug(const char* message, const P&... params)
{
    log(log::LogLevel::debug, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::trace(const char* message, const P&... params)
{
    log(log::LogLevel::trace, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::fatal(string_view_type message, const P&... params)
{
    log(log::LogLevel::fatal, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::error(string_view_type message, const P&... params)
{
    log(log::LogLevel::error, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::warn(string_view_type message, const P&... params)
{
    log(log::LogLevel::warn, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::info(string_view_type message, const P&... params)
{
    log(log::LogLevel::info, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::detail(string_view_type message, const P&... params)
{
    log(log::LogLevel::detail, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::debug(string_view_type message, const P&... params)
{
    log(log::LogLevel::debug, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::trace(string_view_type message, const P&... params)
{
    log(log::LogLevel::trace, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::log(log::LogLevel level, const char* message, const P&... params)
{
    std::cerr << "Logger::log_1() - 1 ("<<static_cast<void*>(this)<<")\n";    
    if (ARCHON_LIKELY(!will_log(level)))
        return;
    std::cerr << "Logger::log_1() - 2\n";    
    m_channel.channel_log(level, *m_prefix, message, params...); // Throws
}


template<class C, class T>
template<class... P> inline void BasicLogger<C, T>::log(log::LogLevel level, string_view_type message,
                                                        const P&... params)
{
    std::cerr << "Logger::log_2() - 1\n";    
    if (ARCHON_LIKELY(!will_log(level)))
        return;
    std::cerr << "Logger::log_2() - 2\n";    
    m_channel.channel_log(level, *m_prefix, message, params...); // Throws
}


template<class C, class T>
inline bool BasicLogger<C, T>::will_log(log::LogLevel level) const noexcept
{
    int level_2 = int(level);
    if (ARCHON_LIKELY(level_2 > m_fixed_limit))
        return false;
    bool has_fixed_limit = (m_fixed_limit != std::numeric_limits<int>::max());
    if (ARCHON_LIKELY(has_fixed_limit))
        return true;
    return (level_2 <= int(m_limit.get_level_limit()));
}


template<class C, class T>
inline log::LogLevel BasicLogger<C, T>::get_log_level_limit() const noexcept
{
    bool has_fixed_limit = (m_fixed_limit != std::numeric_limits<int>::max());
    if (ARCHON_LIKELY(has_fixed_limit))
        return LogLevel(m_fixed_limit);
    return m_limit.get_level_limit();
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger()
    : BasicLogger(BasicLogger::get_cerr()) // Throws
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(BasicLogger& base_logger) noexcept
    : BasicLogger(base_logger.get_prefix(), base_logger.get_channel(), base_logger.get_channel_map())
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(BasicLogger& base_logger, std::string_view channel)
    : BasicLogger(base_logger.get_prefix(), base_logger.find_channel(channel), base_logger.get_channel_map()) // Throws
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(const prefix_type& prefix, channel_type& channel,
                                      channel_map_type& channel_map) noexcept
    : BasicLogger(channel.get_limit(), prefix, channel, channel_map)
{
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_prefix() const noexcept -> const prefix_type&
{
    return *m_prefix;
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_channel() noexcept -> channel_type&
{
    return m_channel;
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_channel() const noexcept -> const channel_type&
{
    return m_channel;
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_channel_map() noexcept -> channel_map_type&
{
    return m_channel_map;
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_channel_map() const noexcept -> const channel_map_type&
{
    return m_channel_map;
}


template<class C, class T>
inline auto BasicLogger<C, T>::find_channel(std::string_view name) -> channel_type&
{
    return get_channel_map().find_channel(name); // Throws
}


template<class C, class T>
inline auto BasicLogger<C, T>::find_channel(std::string_view name) const -> const channel_type&
{
    return get_channel_map().find_channel(name); // Throws
}


template<class C, class T>
inline auto BasicLogger<C, T>::get_locale() const noexcept -> const std::locale&
{
    return get_channel().get_sink().get_locale();
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(const log::Limit& limit, const prefix_type& prefix, channel_type& channel,
                                      channel_map_type& channel_map) noexcept
    : BasicLogger(limit.get_fixed_limit(), limit, prefix, channel, channel_map)
{
}


template<class C, class T>
inline BasicLogger<C, T>::BasicLogger(int fixed_limit, const log::Limit& limit, const prefix_type& prefix,
                                      channel_type& channel, channel_map_type& channel_map) noexcept
    : m_fixed_limit(fixed_limit)
    , m_limit(limit)
    , m_prefix(&prefix)
    , m_channel(channel)
    , m_channel_map(channel_map)
{
}


template<class C, class T>
inline void BasicLogger<C, T>::set_prefix(const prefix_type& prefix) noexcept
{
    m_prefix = &prefix;
}



// ============================ BasicRootLogger ============================


template<class C, class T>
BasicRootLogger<C, T>::BasicRootLogger(const std::locale& locale)
    : sink_type(locale)
    , logger_type(*this, *this, m_channel, *this)
    , m_channel("", *this, *this, *this)
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out.imbue(locale); // Throws
    m_newline = m_out.widen('\n'); // Throws
}


template<class C, class T>
void BasicRootLogger<C, T>::format_log_level(log::LogLevel level, ostream_type& out) const
{
    out << sink_type::get_level_prefix(level); // Throws
}


template<class C, class T>
void BasicRootLogger<C, T>::sink_log(log::LogLevel level, const prefix_type& channel_prefix,
                                     const prefix_type& message_prefix, string_view_type message)
{
    std::cerr << "RootLogger::sink_log() - 1 ("<<static_cast<void*>(&m_mutex)<<")\n";    
    std::cerr << "RootLogger::sink_log() - 1.1\n";    
    std::lock_guard lock(m_mutex);
    std::cerr << "RootLogger::sink_log() - 2\n";    

    static_cast<void>(level);    
    static_cast<void>(channel_prefix);    
    static_cast<void>(message_prefix);    
    static_cast<void>(message);    

    m_out.full_clear();
    channel_prefix.format_prefix(m_out); // Throws
    message_prefix.format_prefix(m_out); // Throws
    format_log_level(level, m_out); // Throws
    std::cerr << "RootLogger::sink_log() - 3\n";    
    C newline = m_newline;
    string_view_type message_2;
//    std::size_t i = 0;
    std::size_t j = message.find(newline);
    std::cerr << "RootLogger::sink_log() - 4\n";    
    if (ARCHON_LIKELY(j == string_view_type::npos)) {
        std::cerr << "RootLogger::sink_log() - 5\n";    
/*
        m_out << message << newline; // Throws
        message_2 = m_out.view();
*/
    }
/*
    else {
        std::size_t size = 0;
        auto append = [&](string_view_type string) {
            m_assembly_buffer.reserve_extra(string.size(), size); // Throws
            std::copy_n(string.data(), string.size(), m_assembly_buffer.data() + size);
            size += string.size();
        };
        string_view_type prefix = m_out.view();
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
*/
/*
    std::cerr << "RootLogger::sink_log() - 6\n";    
    root_log(message_2); // throws
*/
}


template<class C, class T>
auto BasicRootLogger<C, T>::do_get_channels() const noexcept -> core::Span<const channel_type>
{
    return { &m_channel, 1 };
}



// ============================ BasicFileLogger ============================


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::FilesystemPathRef path)
    : BasicFileLogger(path, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::FilesystemPathRef path, const std::locale& locale)
    : BasicFileLogger(path, locale, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::FilesystemPathRef path, const std::locale& locale,
                                                 Config config)
    : BasicFileLogger(core::File(path, core::File::Mode::append), m_owned_file, locale, std::move(config)) // Throws
{
}


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::File& file)
    : BasicFileLogger(file, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::File& file, const std::locale& locale)
    : BasicFileLogger(file, locale, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::File& file, const std::locale& locale, Config config)
    : BasicFileLogger({}, file, locale, std::move(config)) // Throws
{
}


template<class C, class T, class I>
void BasicFileLogger<C, T, I>::format_log_level(log::LogLevel level, ostream_type& out) const
{
    using namespace archon::core::terminal;
    bool colorize = false;
    Color color = Color::red;
    Weight weight = Weight::normal;
    if (m_colorize) {
        switch (level) {
            case log::LogLevel::all:
            case log::LogLevel::trace:
            case log::LogLevel::debug:
            case log::LogLevel::detail:
            case log::LogLevel::info:
            case log::LogLevel::off:
                break;
            case log::LogLevel::warn:
                color = Color::yellow;
                colorize = true;
                break;
            case log::LogLevel::error:
            case log::LogLevel::fatal:
                weight = Weight::bold;
                colorize = true;
                break;
        }
    }
    if (ARCHON_LIKELY(!colorize)) {
        out << sink_type::get_level_prefix(level); // Throws
    }
    else {
        out << seq::set_weight(weight) << seq::set_color(color); // Throws
        out << sink_type::get_level_prefix(level); // Throws
        out << seq::reset_color() << seq::set_weight(Weight::normal); // Throws
    }
}


template<class C, class T, class I>
void BasicFileLogger<C, T, I>::root_log(string_view_type message)
{
    std::cerr << "FileLogger::root_log() - 1\n";    
    std::string_view message_2 = m_text_codec.encode_sc(message, m_encode_buffer); // Throws
    std::cerr << "FileLogger::root_log() - 2\n";    
    m_file.write(message_2); // Throws
}


template<class C, class T, class I>
inline BasicFileLogger<C, T, I>::BasicFileLogger(core::File owned_file, core::File& file, const std::locale& locale,
                                                 Config config)
    : log::BasicRootLogger<C, T>(locale) // Throws
    , m_owned_file(std::move(owned_file))
    , m_file(file)
    , m_colorize(core::assume_locale_has_escape(locale) &&
                 (config.colorize == Colorize::yes || (config.colorize == Colorize::detect && file.is_terminal())))
    , m_text_codec(locale, std::move(config.text_codec)) // Throws
{
}



// ============================ BasicNullLogger ============================


template<class C, class T>
inline BasicNullLogger<C, T>::BasicNullLogger(const std::locale& locale) noexcept
    : sink_type(locale)
    , logger_type(*this, *this, m_channel, *this)
    , m_channel("", *this, *this, *this)
{
}


template<class C, class T>
void BasicNullLogger<C, T>::sink_log(log::LogLevel, const prefix_type&, const prefix_type&, string_view_type)
{
    // Intended no-op
}


template<class C, class T>
auto BasicNullLogger<C, T>::do_get_channels() const noexcept -> core::Span<const channel_type>
{
    return { &m_channel, 1 };
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_LOGGER_HPP
