from __future__ import annotations

import typing
import dataclasses
import textwrap
import pathlib
import io
import unittest

import archon.text_pos as _tp
import archon.test as _t
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.process as _cp


def test_Message(context: _t.Context) -> None:
    text = """\
      message("Foo")
    """
    path = pathlib.Path("test.cmake")
    result = _process(textwrap.dedent(text), path, context)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.file_context.path, path)
    context.check_equal(message.file_context.pos, _tp.FullTextPos(1))
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.level, _cp.MessageLevel.NOTICE)
    context.check_equal(message.message, "Foo")


def test_Set(context: _t.Context) -> None:
    text = """\
      set(_x "Bar")
      message("Foo ${_x}")
    """
    path = pathlib.Path("test.cmake")
    result = _process(textwrap.dedent(text), path, context)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.file_context.path, path)
    context.check_equal(message.file_context.pos, _tp.FullTextPos(2))
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.level, _cp.MessageLevel.NOTICE)
    context.check_equal(message.message, "Foo Bar")


def test_Foreach(context: _t.Context) -> None:
    text = """\
      set(l "Foo" "Bar" "Baz")
      foreach(_x IN LISTS l)
        message("Foo ${_x}")
      endforeach()
    """
    path = pathlib.Path("test.cmake")
    result = _process(textwrap.dedent(text), path, context)
    expected_messages = [
        "Foo Foo",
        "Foo Bar",
        "Foo Baz",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for message, expected in zip(result.messages, expected_messages):
        context.check_equal(message.file_context.path, path)
        context.check_equal(message.file_context.pos, _tp.FullTextPos(3, 2))
        context.check_is_none(message.occurrence_uncertainty)
        context.check_equal(message.level, _cp.MessageLevel.NOTICE)
        context.check_equal(message.message, expected)


def test_Macro(context: _t.Context) -> None:
    text = """\
      macro(foo _x)
        message("Foo ${_x}")
      endmacro()
      foo("Bar")
    """
    path = pathlib.Path("test.cmake")
    result = _process(textwrap.dedent(text), path, context)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.file_context.path, path)
    context.check_equal(message.file_context.pos, _tp.FullTextPos(2, 20))
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(message.level, _cp.MessageLevel.NOTICE)
    context.check_equal(message.message, "Foo Bar")


def _process(cmake_text: str, cmake_path: pathlib.Path, context: _t.Context) -> _Result:
    input_ = io.StringIO(cmake_text)
    cmake_source = _cp.Source(input_, cmake_path)
    pos_resolver = _cp.PositionResolver()
    result = _Result()
    application = _Application(pos_resolver, result)
    _cp.process(cmake_source, application, pos_resolver, context.logger)
    return result


class _Result:
    def __init__(self) -> None:
        self.messages = list[_Message]()


class _Application(_cp.Application):
    def __init__(self, pos_resolver: _cp.PositionResolver, result: _Result) -> None:
        self._pos_resolver = pos_resolver
        self._result       = result

    @typing.override
    def message(self, pos: _cur.Position, occurrence_uncertainty: _cp.OccurrenceUncertainty, level: _cp.MessageLevel,
                message: str) -> None:
        file_context = self._pos_resolver.resolve_file_context(pos)
        self._result.messages.append(_Message(file_context, occurrence_uncertainty, level, message))


@dataclasses.dataclass(slots=True, frozen=True)
class _Message:
    file_context:           _tp.FileContext
    occurrence_uncertainty: _cp.OccurrenceUncertainty
    level:                  _cp.MessageLevel
    message:                str


# Bridge to Python's native testing framework
def load_tests(loader: unittest.TestLoader, standard_tests: unittest.TestSuite,
               pattern: str | None) -> unittest.TestSuite:
    return _t.generate_native_tests(__name__)


if __name__ == '__main__':
    _t.run_module_tests(__name__)
