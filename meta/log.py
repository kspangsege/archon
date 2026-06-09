from __future__ import annotations
from typing import Any, Protocol, TextIO
from abc import ABC, abstractmethod

import enum
import sys
import pathlib

import ansi


class LogLevel(enum.Enum):
    FATAL  = 1
    ERROR  = 2
    WARN   = 3
    INFO   = 4
    DETAIL = 5
    DEBUG  = 6
    TRACE  = 7


class Logger:
    def __init__(self, limit: Limit, sink: Sink) -> None:
        self._limit = limit
        self._sink  = sink

    def fatal(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.FATAL, pattern, *args)

    def error(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.ERROR, pattern, *args)

    def warn(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.WARN, pattern, *args)

    def info(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.INFO, pattern, *args)

    def detail(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.DETAIL, pattern, *args)

    def debug(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.DEBUG, pattern, *args)

    def trace(self, pattern: str, *args: Any) -> None:
        self.log(LogLevel.TRACE, pattern, *args)

    def log(self, level: LogLevel, pattern: str, *args: Any) -> None:
        if self.will_log(level):
            prefix = ""
            message = pattern % args
            self._sink.log(level, prefix, message)

    def will_log(self, level: LogLevel) -> bool:
        return self._limit.will_log(level)

    def get_limit(self) -> Limit:
        return self._limit

    def get_sink(self) -> Sink:
        return self._sink


class RootLogger(Logger):
    def __init__(self, output_stream: TextIO = sys.stdout) -> None:
        limit = Nonlimit()
        sink = RootSink(output_stream)
        Logger.__init__(self, limit, sink)


class LimitLogger(Logger):
    def __init__(self, base_logger: Logger, limit_level: LogLevel) -> None:
        limit = Sublimit(base_logger.get_limit(), limit_level)
        Logger.__init__(self, limit, base_logger.get_sink())


class FileContextLogger(Logger):
    def __init__(self, base_logger: Logger, file_context: FileContext) -> None:
        sink = FileContextSink(base_logger.get_sink(), file_context)
        Logger.__init__(self, base_logger.get_limit(), sink)
        self._base_logger = base_logger

    def for_alt_line(self, line_no: int) -> FileContextLogger:
        sink = self.get_sink()
        assert isinstance(sink, FileContextSink)
        return self.for_alt_context(sink.get_path(), line_no)

    def for_alt_context(self, path: pathlib.Path, line_no: int) -> FileContextLogger:
        file_context = FileContext(path, line_no)
        return FileContextLogger(self._base_logger, file_context)


class FileContext:
    def __init__(self, path: pathlib.Path, line_no: int = 0) -> None:
        self.path = path
        self.line_no = line_no


class Limit(ABC):
    @abstractmethod
    def will_log(self, level: LogLevel) -> bool:
        ...


class Nonlimit(Limit):
    def will_log(self, level: LogLevel) -> bool:
        return True


class Sublimit(Limit):
    def __init__(self, base_limit: Limit, limit_level: LogLevel) -> None:
        self._base_limit  = base_limit
        self._limit_level = limit_level

    def will_log(self, level: LogLevel) -> bool:
        return level.value <= self._limit_level.value and self._base_limit.will_log(level)


class Sink(ABC):
    @abstractmethod
    def log(self, level: LogLevel, prefix: str, message: str) -> None:
        ...


class RootSink(Sink):
    def __init__(self, output_stream: TextIO) -> None:
        self._output_stream = output_stream
        self._is_ansi_term = ansi.is_ansi_term(self._output_stream)

    def log(self, level: LogLevel, prefix: str, message: str) -> None:
        label = None
        color = None
        if level == LogLevel.FATAL:
            label = "FATAL"
            color = ansi.AnsiTermColor.RED
        elif level == LogLevel.ERROR:
            label = "ERROR"
            color = ansi.AnsiTermColor.RED
        elif level == LogLevel.WARN:
            label = "WARNING"
            color = ansi.AnsiTermColor.YELLOW
        if label and color is not None and self._is_ansi_term:
            label = ansi.ansi_term_set_color(color) + label + ansi.ansi_term_reset_color()
        prefix_2 = prefix
        if label:
            prefix_2 = "%s%s: " % (prefix, label)
        block = ""
        for line in message.split("\n"):
            block += "%s%s\n" % (prefix_2, line)
        self._output_stream.write(block)


class FileContextSink(Sink):
    def __init__(self, base_sink: Sink, file_context: FileContext) -> None:
        self._base_sink    = base_sink
        self._file_context = file_context

    def log(self, level: LogLevel, prefix: str, message: str) -> None:
        path    = self._file_context.path
        line_no = self._file_context.line_no
        context = "%s:%s" % (path, line_no) if line_no != 0 else path
        prefix_2 = "%s: %s" % (context, prefix)
        self._base_sink.log(level, prefix_2, message)

    def get_path(self) -> pathlib.Path:
        return self._file_context.path


def parse_log_level(string: str) -> LogLevel:
    map_ = {
        "fatal":  LogLevel.FATAL,
        "error":  LogLevel.ERROR,
        "warn":   LogLevel.WARN,
        "info":   LogLevel.INFO,
        "detail": LogLevel.DETAIL,
        "debug":  LogLevel.DEBUG,
        "trace":  LogLevel.TRACE,
    }
    level = map_.get(string)
    if level is not None:
        return level
    raise ValueError
