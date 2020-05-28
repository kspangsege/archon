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

#ifndef ARCHON_X_BASE_X_TIMESTAMP_LOGGER_HPP
#define ARCHON_X_BASE_X_TIMESTAMP_LOGGER_HPP

/// \file


#include <memory>
#include <utility>
#include <string_view>
#include <filesystem>

#include <archon/base/timestamp_formatter.hpp>
#include <archon/base/logger.hpp>


namespace archon::base {


class TimestampLoggerBase {
public:
    using Precision = TimestampFormatterBase::Precision;
};


/// \brief A logger that adds timestamps.
///
/// This is a logger that builds on top of an existing logger (the base logger)
/// by adding timestamp prefixes to all logged messages.
///
/// An instance of BasicTimestampLogger is thread-safe if the base logger is
/// thread-safe.
///
/// FIXME: Talk about conditions for monotonicity of generated timestamps (each
/// root sink must meet condition: "serialized prefix formatting", meaning that
/// prefixes are formatted one at a time, and in the order that the messages
/// ultimately end up in. A particular message may get different timestamps in
/// different root sinks. Ordering of concurrently submitted messages may differ
/// in different root sinks. Also talk about some of this in doc of `Prefix`.                                    
///
template<class C, class T = std::char_traits<C>> class BasicTimestampLogger :
        private BasicLogger<C, T>::Prefix,
        public TimestampLoggerBase,
        public BasicLogger<C, T> {
public:
    struct Config;

    using logger_type = BasicLogger<C, T>;
    using ostream_type = typename logger_type::ostream_type;
    using formatter_type = BasicTimestampFormatter<C, T>;
    using params_type = typename formatter_type::Params;

    /// \{
    ///
    /// \brief Construct a timestamp logger.
    ///
    /// Construct a timestamp logger that builds on top of the specified base
    /// logger (\p bas_logger) by adding timestamp prefixes to all logged
    /// messages.
    ///
    /// The default timestamp format is `"%FT%T%z: "` unless `utc_time` is true
    /// in the specified configuration, in which case the default format is
    /// `"%FT%TZ: "`.
    ///
    explicit BasicTimestampLogger(logger_type& base_logger);
    BasicTimestampLogger(logger_type& base_logger, Config config);
    /// \}

private:
    const std::locale m_locale;
    const bool m_utc_time;
    const params_type m_params;

    static params_type adjust_format(Config) noexcept;

    // Overriding functions from Prefix
    void format_prefix(ostream_type&) const override final;
};


using TimestampLogger     = BasicTimestampLogger<char>;
using WideTimestampLogger = BasicTimestampLogger<wchar_t>;




template<class C, class T> struct BasicTimestampLogger<C, T>::Config : params_type {
    /// \brief Format timestamps in UTC.
    ///
    /// When set to `true`, the tiemstamp logger will format timestamps in UTC
    /// rather than in the local time zone.
    ///
    bool utc_time = false;
};




template<class C, class T = std::char_traits<C>> class BasicTimestampFileLogger :
        public BasicTimestampLogger<C, T> {
public:
    struct Config : BasicFileLogger<C, T>::Config, BasicTimestampLogger<C, T>::Config {};

    /// \{
    ///
    /// The specified file will be opened in "append" mode (\ref
    /// File::Mode::append).
    ///
    explicit BasicTimestampFileLogger(FilesystemPathRef);
    BasicTimestampFileLogger(FilesystemPathRef, Config);
    /// \}

    explicit BasicTimestampFileLogger(File&);
    BasicTimestampFileLogger(File&, Config);

private:
    std::unique_ptr<BasicFileLogger<C, T>> m_file_logger;

    BasicTimestampFileLogger(std::unique_ptr<BasicFileLogger<C, T>>,
                             typename BasicTimestampLogger<C, T>::Config);
};


using TimestampFileLogger     = BasicTimestampFileLogger<char>;
using WideTimestampFileLogger = BasicTimestampFileLogger<wchar_t>;








// Implementation


// ============================ BasicTimestampLogger ============================


template<class C, class T>
inline BasicTimestampLogger<C, T>::BasicTimestampLogger(logger_type& base_logger) :
    BasicTimestampLogger(base_logger, {}) // throws
{
}


template<class C, class T>
inline BasicTimestampLogger<C, T>::BasicTimestampLogger(logger_type& base_logger, Config config) :
    logger_type(*this, base_logger.get_channel(), base_logger.get_map()),
    m_locale(base_logger.get_locale()),
    m_utc_time(config.utc_time),
    m_params(adjust_format(std::move(config)))
{
}


template<class C, class T>
auto BasicTimestampLogger<C, T>::adjust_format(Config config) noexcept -> params_type
{
    if (config.format.empty()) {
        if (config.utc_time) {
            config.format = "%FT%TZ: ";
        }
        else {
            config.format = "%FT%T%z: ";
        }
    }
    return config;
}


template<class C, class T> void BasicTimestampLogger<C, T>::format_prefix(ostream_type& out) const
{
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


// ============================ BasicTimestampFileLogger ============================


template<class C, class T>
inline BasicTimestampFileLogger<C, T>::BasicTimestampFileLogger(FilesystemPathRef path) :
    BasicTimestampFileLogger(path, {}) // Throws
{
}


template<class C, class T>
inline BasicTimestampFileLogger<C, T>::BasicTimestampFileLogger(FilesystemPathRef path,
                                                                Config config) :
    BasicTimestampFileLogger(std::make_unique<BasicFileLogger<C, T>>(path, std::move(config)),
                             std::move(config)) // Throws
{
}


template<class C, class T>
inline BasicTimestampFileLogger<C, T>::BasicTimestampFileLogger(File& file) :
    BasicTimestampFileLogger(file, {}) // Throws
{
}


template<class C, class T>
inline BasicTimestampFileLogger<C, T>::BasicTimestampFileLogger(File& file, Config config) :
    BasicTimestampFileLogger(std::make_unique<BasicFileLogger<C, T>>(file, std::move(config)),
                             std::move(config)) // Throws
{
}


template<class C, class T>
inline BasicTimestampFileLogger<C, T>::BasicTimestampFileLogger(std::unique_ptr<BasicFileLogger<C, T>> file_logger,
                                                                typename BasicTimestampLogger<C, T>::Config config) :
    BasicTimestampLogger<C, T>(*file_logger, std::move(config)), // Throws
    m_file_logger(std::move(file_logger))
{
}


} // namespace archon::base

#endif // ARCHON_X_BASE_X_TIMESTAMP_LOGGER_HPP
