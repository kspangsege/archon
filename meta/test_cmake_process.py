from __future__ import annotations

import typing
import dataclasses
import textwrap
import pathlib
import io
import unittest

import archon.text_pos as _tp
import archon.log as _l
import archon.test as _t
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.process as _cp


def test_Message(context: _t.Context) -> None:
    text = r"""
      message("Foo")
      meSsaGe("Bar")  # Mixed case in command name
      message(WARNING "Baz")
      if(${x})
        message(FATAL_ERROR "Qux")  # Uncertain
      endif()
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        ("Foo", _cp.MessageLevel.NOTICE,      True,  _tp.TextPos(1, 0)),
        ("Bar", _cp.MessageLevel.NOTICE,      True,  _tp.TextPos(2, 0)),
        ("Baz", _cp.MessageLevel.WARNING,     True,  _tp.TextPos(3, 0)),
        ("Qux", _cp.MessageLevel.FATAL_ERROR, False, _tp.TextPos(5, 2)),
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.level, expected[1])
        subcontext.check_equal(not message.occurrence_uncertainty, expected[2])
        subcontext.check_equal(message.file_pos.text_pos, expected[3])


def test_Set(context: _t.Context) -> None:
    text = r"""
      set(_x "A")
      set(CACHE{_x} FORCE VALUE "B")
      set(ENV{_x} "C")
      message("1: ${_x}-$CACHE{_x}-$ENV{_x}")
      set(_x "A2")
      message("2: ${_x}-$CACHE{_x}-$ENV{_x}")
      set(CACHE{_x} FORCE VALUE "B2")
      message("3: ${_x}-$CACHE{_x}-$ENV{_x}")
      set(ENV{_x} "C2")
      message("4: ${_x}-$CACHE{_x}-$ENV{_x}")
      set(_x)
      message("5: ${_x}-$CACHE{_x}-$ENV{_x}")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: A-B-C",
        "2: A2-B-C",
        "3: A2-B2-C",
        "4: A2-B2-C2",
        "5: B2-B2-C2",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.level, _cp.MessageLevel.NOTICE)
        subcontext.check_equal(message.message, expected)


def test_Unset(context: _t.Context) -> None:
    text = r"""
      unset(CACHE{_x})
      set(_x                    "x")
      set(CACHE{_y} FORCE VALUE "y")
      set(ENV{_z}               "z")
      if(DEFINED _x)
        message("d1x")
      endif()
      if(DEFINED CACHE{_y})
        message("d1y")
      endif()
      if(DEFINED ENV{_z})
        message("d1z")
      endif()
      unset(_x)
      unset(CACHE{_y})
      unset(ENV{_z})
      if(DEFINED _x)
        message("d2x")
      endif()
      if(DEFINED CACHE{_y})
        message("d2y")
      endif()
      if(DEFINED ENV{_z})
        message("d2z")
      endif()
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "d1x",
        "d1y",
        "d1z",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)


def test_VariableExpansion(context: _t.Context) -> None:
    # Recursive
    text = r"""
      set(foo "A")
      set(bar "B")
      set(baz "C")
      set(_foo "foo")
      set(_b "b")
      set(_f "f")
      set(_o "o")
      set(_r "r")
      set(_z "z")
      message("1: x${${_foo}}y")
      message("2: x${${_f}oo}y")
      message("3: x${ba${_r}}y")
      message("4: x${${_b}a${_z}}y")
      message("5: x${${_f}${_o}o}y")
      message("6: x${f${_o}${_o}}y")
      message("7: x${${_f}${_o}${_o}}y")
      message("8: x${f${_${_o}}o}y")  # Twice recursive
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: xAy",
        "2: xAy",
        "3: xBy",
        "4: xCy",
        "5: xAy",
        "6: xAy",
        "7: xAy",
        "8: xAy",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)

    # Empty variable name
    text = r"""
      set(empty "")
      set("" "empty")
      message("1: (${})")
      message("2: (${${empty}})")
      message("3: (${${empty}${empty}})")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: (empty)",
        "2: (empty)",
        "3: (empty)",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)

    # Invalids and weirds
    text = r"""
      message("1: x$FOO{bar}y")
      message("2: x$F+O{bar}y")
      message("3: x$F$O{bar}y")
      message("4: x$F=O{bar}y")  # Not a variable expansion
      set("f+o" "(+)")
      set("f$o" "($)")
      set("f=o" "(=)")
      message("5: x${f+o}y")   # Valid
      message("6: x${f$o}y")   # Spuriously valid (see CMake policy CMP0053)
      message("7: x${f=o}y")
      message("8: x${f\=o}y")  # Valid
      set(empty "")
      set(equal "=")
      message("9:  x${${empty}f=o}y")
      message("10: x${${empty}f\=o}y")  # Valid
      message("11: x${f${equal}o}y")    # Valid
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    expected_messages = [
        "1: xy",
        "2: xy",
        "3: x$Fy",
        "4: x$F=O{bar}y",
        "5: x(+)y",
        "6: x($)y",
        "7: xy",
        "8: x(=)y",
        "9:  xy",
        "10: x(=)y",
        "11: x(=)y",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)
    expected_errors = [
        ("Invalid domain", _tp.TextPos(1, 14)),
        ("Invalid domain", _tp.TextPos(2, 14)),
        ("Invalid domain", _tp.TextPos(3, 16)),
        ("Invalid literal character", _tp.TextPos(10, 16)),
        ("Invalid literal character", _tp.TextPos(14, 25)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(i)
        subcontext.check_in(expected[0], error.message)
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_String(context: _t.Context) -> None:
    text = r"""
      set(_x "Foo")
      string(TOLOWER "${_x}" _y)
      string(TOUPPER "${_x}" _z)
      message("${_y}-${_z}")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.file_pos, _tp.FilePos(path, 4, 0))
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.level, _cp.MessageLevel.NOTICE)
    context.check_equal(message.message, "foo-FOO")


def test_List(context: _t.Context) -> None:
    text = r"""
      set(_a "")
      set(_b "A")
      set(_c "")
      set(_d "A")
      list(APPEND _a)
      list(APPEND _b)
      list(APPEND _c "B")
      list(APPEND _d "B")
      message("${_a}-${_b}-${_c}-${_d}")
      set(_l "A;B" "C\;D")
      list(APPEND _l "E;F" "G\;H")
      message("${_l}")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-A-B-A;B",
        "A;B;C\\;D;E;F;G\\;H",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)


def test_Foreach(context: _t.Context) -> None:
    text = r"""
      set(l "Foo" "Bar" "Baz")
      foreach(_x IN LISTS l)
        message("Foo ${_x}")
      endforeach()
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "Foo Foo",
        "Foo Bar",
        "Foo Baz",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_equal(message.file_pos, _tp.FilePos(path, 3, 2))
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.level, _cp.MessageLevel.NOTICE)
        subcontext.check_equal(message.message, expected)


def test_Macro(context: _t.Context) -> None:
    text = r"""
      macro(foo _x)
        message("Foo ${_x}")
      endmacro()
      foo("Bar")
      Foo("Baz")  # Mixed case in command name
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "Foo Bar",
        "Foo Baz",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)

    text = r"""
      macro(foo _x _y)
        message("${_x}${_y}aa} -- ${_y}${_x}" [[ -- ${_x}${_y}]])
      endmacro()
      set(foo "bar")
      set(faa "boo")
      foo("oo}" "\${f")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.file_pos, _tp.FilePos(path, 2, 2))
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.message, "oo}boo -- bar -- ${_x}${_y}")

    # That special macro variables work correctly
    text = r"""
      set(ARGV1 "1")
      set(ARGV2 "2")
      macro(foo _x)
        message("${ARGC}-${ARGN}-${ARGV}-${ARGV0}-${ARGV1}-${ARGV2}")
      endmacro()
      foo("x")
      foo("x" "y")
      foo("x" "y" "z")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1--x-x-1-2",
        "2-y-x;y-x-y-2",
        "3-y;z-x;y;z-x-y-z",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for message, expected in zip(result.messages, expected_messages):
        subcontext = context.subcontext(i)
        subcontext.check_equal(message.file_pos, _tp.FilePos(path, 4, 2))
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.level, _cp.MessageLevel.NOTICE)
        subcontext.check_equal(message.message, expected)

    # That expansion of outer macro parameters reaches into inner macro body
    text = r"""
      macro(foo _x _y)
        macro(bar _y _z)
          message("${_x}-${_y}-${_z}")
        endmacro()
        bar("c" "d")
      endmacro()
      foo("a" "b")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.file_pos, _tp.FilePos(path, 3, 4))
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.message, "a-b-d")

    # Check determination of error position with correction for macro substitution and
    # regular expression unescaping
    text = r"""
      macro(foo _x)
        if("--${_x}----${_x}--" MATCHES "${_x}\\-\\-\\-\\-x")
          message("click 1")
        endif()
        if("--${_x}----${_x}--" MATCHES "${_x}\\-\\-\\-\\-[2-1]")
          message("click 2")
        endif()
      endmacro()
      foo("x")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.message, "click 1")
    context.check_equal(len(result.errors), 1)
    error = result.errors[0]
    context.check_in("Regular expression syntax error: Invalid range", error.message)
    context.check_equal(error.file_pos, _tp.FilePos(path, 5, 53))


def test_Block(context: _t.Context) -> None:
    text = r"""
      set(_x "A")
      set(_y "B")
      message("1: ${_x}-${_y}")
      block()
        set(_x "C")
        set(_y "D" PARENT_SCOPE)
        message("2: ${_x}-${_y}")
      endblock()
      message("3: ${_x}-${_y}")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: A-B",
        "2: C-B",
        "3: A-D",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(i)
        subcontext.check_is_none(message.occurrence_uncertainty)
        subcontext.check_equal(message.message, expected)

    text = r"""
      block(  Foo  )
      endblock()
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    context.check_equal(len(result.errors), 1)
    error = result.errors[0]
    context.check_in("Unrecognized first argument", error.message)
    context.check_equal(error.file_pos, _tp.FilePos(path, 1, 8))


def _trim_cmake_text(text: str) -> str:
    return textwrap.dedent(text.removeprefix("\n"))


def _process(cmake_text: str, cmake_path: pathlib.Path, context: _t.Context) -> tuple[bool, _Result]:
    input_ = io.StringIO(cmake_text)
    cmake_source = _cp.Source(input_, cmake_path)
    pos_resolver = _cp.PositionResolver()
    result = _Result()
    application = _Application(pos_resolver, result, context.logger)
    success = _cp.process(cmake_source, application, pos_resolver)
    return success, result


class _Result:
    def __init__(self) -> None:
        self.messages = list[_CMakeMessage]()
        self.warnings = list[_LogMessage]()
        self.errors   = list[_LogMessage]()


class _Application(_cp.Application):
    def __init__(self, pos_resolver: _cp.PositionResolver, result: _Result, logger: _l.Logger) -> None:
        self._pos_resolver = pos_resolver
        self._result       = result
        self._logger       = logger

    @typing.override
    def message(self, pos: _cur.Position, occurrence_uncertainty: _cp.OccurrenceUncertainty, level: _cp.MessageLevel,
                message: str) -> None:
        file_pos = self._pos_resolver.resolve_file_pos(pos)
        self._result.messages.append(_CMakeMessage(file_pos, occurrence_uncertainty, level, message))
        context = _l.FileContext(file_pos.path, _l.FullTextPos(file_pos.line_no, file_pos.pos_on_line))
        certainty = "Uncertain" if occurrence_uncertainty else "Certain"
        _l.FileContextLogger(self._logger, context).info("%s: Message(%s): %s", certainty, level.name, message)

    @typing.override
    def warn(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        file_pos = self._pos_resolver.resolve_file_pos(pos)
        self._result.warnings.append(_LogMessage(file_pos, message % args))
        context = _l.FileContext(file_pos.path, _l.FullTextPos(file_pos.line_no, file_pos.pos_on_line))
        _l.FileContextLogger(self._logger, context).warn(message, *args)

    @typing.override
    def error(self, pos: _cur.Position, message: str, *args: typing.Any) -> None:
        file_pos = self._pos_resolver.resolve_file_pos(pos)
        self._result.errors.append(_LogMessage(file_pos, message % args))
        context = _l.FileContext(file_pos.path, _l.FullTextPos(file_pos.line_no, file_pos.pos_on_line))
        _l.FileContextLogger(self._logger, context).error(message, *args)


@dataclasses.dataclass(slots=True, frozen=True)
class _CMakeMessage:
    file_pos:               _tp.FilePos
    occurrence_uncertainty: _cp.OccurrenceUncertainty
    level:                  _cp.MessageLevel
    message:                str


@dataclasses.dataclass(slots=True, frozen=True)
class _LogMessage:
    file_pos: _tp.FilePos
    message:  str


# Bridge to Python's native testing framework
def load_tests(loader: unittest.TestLoader, standard_tests: unittest.TestSuite,
               pattern: str | None) -> unittest.TestSuite:
    return _t.generate_native_tests(__name__)


if __name__ == '__main__':
    _t.run_module_tests(__name__)
