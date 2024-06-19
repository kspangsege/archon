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

#ifndef ARCHON_X_LOG_X_TIMESTAMP_LOGGER_HPP
#define ARCHON_X_LOG_X_TIMESTAMP_LOGGER_HPP

/// \file


#include <memory>
#include <utility>
#include <string_view>
#include <filesystem>

#include <archon/core/timestamp_formatter.hpp>
#include <archon/log/prefix.hpp>
#include <archon/log/logger.hpp>


namespace archon::log {


class TimestampLoggerBase {
public:
    using Precision = core::TimestampFormatterBase::Precision;
};


/// \brief A logger that adds timestamps.
///
/// This is a logger that builds on top of an existing logger (the base logger) by adding
/// timestamp prefixes to all logged messages.
///
/// An instance of BasicTimestampLogger is thread-safe if the base logger is thread-safe.
///
/// FIXME: Talk about conditions for monotonicity of generated timestamps (each root sink
/// must meet condition: "serialized prefix formatting", meaning that prefixes are formatted
/// one at a time, and in the order that the messages ultimately end up in. A particular
/// message may get different timestamps in different root sinks. Ordering of concurrently
/// submitted messages may differ in different root sinks. Also talk about some of this in
/// doc of `BasicPrefix`.                                    
///
template<class C, class T = std::char_traits<C>> class BasicTimestampLogger
    : public log::TimestampLoggerBase
    , public log::BasicLogger<C, T> {
public:
    struct Config;

    using formatter_type = core::BasicTimestampFormatter<C, T>;
    using logger_type    = log::BasicLogger<C, T>;

    using Params = typename formatter_type::Params;

    /// \{
    ///
    /// \brief Construct a timestamp logger.
    ///
    /// Construct a timestamp logger that builds on top of the specified base logger (\p
    /// bas_logger) by adding timestamp prefixes to all logged messages.
    ///
    /// The default timestamp format is `"%FT%T%z: "` unless `utc_time` is true in the
    /// specified configuration, in which case the default format is `"%FT%TZ: "`.
    ///
    explicit BasicTimestampLogger(logger_type& base_logger);
    BasicTimestampLogger(logger_type& base_logger, Config config);
    /// \}

    class PrefixImpl final
        : public log::BasicPrefix<C, T> {
    public:
        using ostream_type = std::basic_ostream<C, T>;
        using prefix_type  = log::BasicPrefix<C, T>;

        PrefixImpl(const prefix_type& parent_prefix, const std::locale& locale, Config&& config);

        // Overriding functions from log::BasicPrefix<C, T>
        void format_prefix(ostream_type&) const override;

    private:
        const prefix_type& m_parent_prefix;
        const std::locale m_locale;
        const bool m_utc_time;
        const Params m_params;
    };

private:
    PrefixImpl m_prefix;

    static auto adjust_format(Config) -> Params;
};


using TimestampLogger     = BasicTimestampLogger<char>;
using WideTimestampLogger = BasicTimestampLogger<wchar_t>;




template<class C, class T> struct BasicTimestampLogger<C, T>::Config
    : Params {

    /// \brief Format timestamps in UTC.
    ///
    /// When set to `true`, the tiemstamp logger will format timestamps in UTC rather than
    /// in the local time zone.
    ///
    bool utc_time = false;
};




template<class C, class T = std::char_traits<C>, class I = core::TextCodecImpl<C, T>>
class BasicTimestampFileLogger
    : public log::BasicFileLogger<C, T, I> {
public:
    struct Config;

    /// \{
    ///
    /// The specified file will be opened in "append" mode (\ref core::File::Mode::append).
    ///
    explicit BasicTimestampFileLogger(core::FilesystemPathRef);
    BasicTimestampFileLogger(core::FilesystemPathRef, const std::locale&);
    BasicTimestampFileLogger(core::FilesystemPathRef, const std::locale&, Config);
    /// \}

    explicit BasicTimestampFileLogger(core::File&);
    BasicTimestampFileLogger(core::File&, const std::locale&);
    BasicTimestampFileLogger(core::File&, const std::locale&, Config);

private:
    using PrefixImpl = typename log::BasicTimestampLogger<C, T>::PrefixImpl;

    PrefixImpl m_prefix;
};


using TimestampFileLogger     = BasicTimestampFileLogger<char>;
using WideTimestampFileLogger = BasicTimestampFileLogger<wchar_t>;




template<class C, class T, class I> struct BasicTimestampFileLogger<C, T, I>::Config
    : log::BasicFileLogger<C, T, I>::Config
    , log::BasicTimestampLogger<C, T>::Config {
};







// Implementation


// ============================ BasicTimestampLogger ============================


template<class C, class T>
inline BasicTimestampLogger<C, T>::BasicTimestampLogger(logger_type& base_logger)
    : BasicTimestampLogger(base_logger, {}) // throws
{
}


template<class C, class T>
inline BasicTimestampLogger<C, T>::BasicTimestampLogger(logger_type& base_logger, Config config)
    : logger_type(m_prefix, base_logger.get_channel(), base_logger.get_channel_map())
    , m_prefix(base_logger.get_prefix(), base_logger.get_locale(), std::move(config)) // Throws
{
}


template<class C, class T>
inline BasicTimestampLogger<C, T>::PrefixImpl::PrefixImpl(const prefix_type& parent_prefix, const std::locale& locale,
                                                          Config&& config)
    : m_parent_prefix(parent_prefix)
    , m_locale(locale)
    , m_utc_time(config.utc_time)
    , m_params(adjust_format(std::move(config))) // Throws
{
}


template<class C, class T>
void BasicTimestampLogger<C, T>::PrefixImpl::format_prefix(ostream_type& out) const
{
    m_parent_prefix.format_prefix(out); // Throws
    formatter_type formatter(m_locale); // Throws
    auto now = std::chrono::system_clock::now();
    typename formatter_type::string_view_type string;
    if (m_utc_time) {
        string = formatter.format_utc(now, m_params); // Throws
    }
    else {
        string = formatter.format_local(now, m_params); // Throws
    }
    out << string; // Throws
}


template<class C, class T>
auto BasicTimestampLogger<C, T>::adjust_format(Config config) -> Params
{
    if (config.format.empty()) {
        if (config.utc_time) {
            config.format = "%FT%TZ: ";
        }
        else {
            config.format = "%FT%T%z: ";
        }
    }
    return config; // Throws
}


// ============================ BasicTimestampFileLogger ============================


template<class C, class T, class I>
inline BasicTimestampFileLogger<C, T, I>::BasicTimestampFileLogger(core::FilesystemPathRef path)
    : BasicTimestampFileLogger(path, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTimestampFileLogger<C, T, I>::BasicTimestampFileLogger(core::FilesystemPathRef path,
                                                                   const std::locale& locale)
    : BasicTimestampFileLogger(path, locale, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTimestampFileLogger<C, T, I>::BasicTimestampFileLogger(core::FilesystemPathRef path,
                                                                   const std::locale& locale, Config config)
    : log::BasicFileLogger<C, T, I>(path, locale, std::move(config)) // Throws
    , m_prefix(this->get_prefix(), locale, std::move(config)) // Throws
{
    this->set_prefix(m_prefix);
}


template<class C, class T, class I>
inline BasicTimestampFileLogger<C, T, I>::BasicTimestampFileLogger(core::File& file)
    : BasicTimestampFileLogger(file, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTimestampFileLogger<C, T, I>::BasicTimestampFileLogger(core::File& file, const std::locale& locale)
    : BasicTimestampFileLogger(file, locale, {}) // Throws
{
}


template<class C, class T, class I>
inline BasicTimestampFileLogger<C, T, I>::BasicTimestampFileLogger(core::File& file, const std::locale& locale,
                                                                   Config config)
    : log::BasicFileLogger<C, T, I>(file, locale, std::move(config)) // Throws
    , m_prefix(this->get_prefix(), locale, std::move(config)) // Throws
{
    this->set_prefix(m_prefix);
}


} // namespace archon::log

#endif // ARCHON_X_LOG_X_TIMESTAMP_LOGGER_HPP
