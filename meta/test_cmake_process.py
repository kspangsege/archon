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


def test_CMakeProcess_Message(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      message("Foo")
      meSsaGe("Bar")  # Mixed case in command name
      message(WARNING "Baz")
      if(${x})
        message(FATAL_ERROR "Qux")  # Uncertain
      endif()
    """
    path = pathlib.Path("test-1.cmake")
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
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.level, expected[1])
        subcontext.check_equal(not message.occurrence_uncertainty, expected[2])
        subcontext.check_equal(message.file_pos.text_pos, expected[3])

    # Uncertainty
    text = r"""
      message("--${u}--")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(1, 11)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_SetAndUnset(context: _t.Context) -> None:
    expected: typing.Any

    # set()
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
      set(_x "")                               # Regular var still overrides cache
      message("5: ${_x}-$CACHE{_x}-$ENV{_x}")
      set(_x)                                  # Regular no longer overrides cache
      message("6: ${_x}-$CACHE{_x}-$ENV{_x}")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: A-B-C",
        "2: A2-B-C",
        "3: A2-B2-C",
        "4: A2-B2-C2",
        "5: -B2-C2",
        "6: B2-B2-C2",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_equal(message.level, _cp.MessageLevel.NOTICE)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # unset()
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
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "d1x",
        "d1y",
        "d1z",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from unset to empty
    text = r"""
      unset(r)
      unset(CACHE{r})  # Prevent uncertainty from cache fallback
      unset(CACHE{c})
      unset(ENV{e})
      if(NOT DEFINED r)
        message("1-1: r")
      endif()
      if(NOT DEFINED CACHE{c})
        message("1-1: c")
      endif()
      if(NOT DEFINED ENV{e})
        message("1-1: e")
      endif()
      set(r "")
      set(CACHE{c} FORCE VALUE "")
      set(ENV{e} "x")  # CMake quirk: Cannot change environement variable directly from unset to empty
      set(ENV{e} "")
      if(DEFINED r AND r STREQUAL "")
        message("1-2: r")
      endif()
      set(val "$CACHE{c}")
      if(DEFINED CACHE{c} AND val STREQUAL "")
        message("1-2: c")
      endif()
      set(val "$ENV{e}")
      if(DEFINED ENV{e} AND val STREQUAL "")
        message("1-2: e")
      endif()
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1-1: r",
        "1-1: c",
        "1-1: e",
        "1-2: r",
        "1-2: c",
        "1-2: e",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from unset to nonempty
    text = r"""
      unset(r)
      unset(CACHE{r})  # Prevent uncertainty from cache fallback
      unset(CACHE{c})
      unset(ENV{e})
      if(NOT DEFINED r)
        message("2-1: r")
      endif()
      if(NOT DEFINED CACHE{c})
        message("2-1: c")
      endif()
      if(NOT DEFINED ENV{e})
        message("2-1: e")
      endif()
      set(r "x")
      set(CACHE{c} FORCE VALUE "x")
      set(ENV{e} "x")
      if(r STREQUAL "x")
        message("2-2: r")
      endif()
      set(val "$CACHE{c}")
      if(val STREQUAL "x")
        message("2-2: c")
      endif()
      set(val "$ENV{e}")
      if(val STREQUAL "x")
        message("2-2: e")
      endif()
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "2-1: r",
        "2-1: c",
        "2-1: e",
        "2-2: r",
        "2-2: c",
        "2-2: e",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from empty to unset via unset()
    text = r"""
      set(r "")
      set(CACHE{c} FORCE VALUE "")
      set(ENV{e} "x")  # CMake quirk: Cannot change environement variable directly from unset to empty
      set(ENV{e} "")
      if(DEFINED r AND r STREQUAL "")
        message("3-1: r")
      endif()
      set(val "$CACHE{c}")
      if(DEFINED CACHE{c} AND val STREQUAL "")
        message("3-1: c")
      endif()
      set(val "$ENV{e}")
      if(DEFINED ENV{e} AND val STREQUAL "")
        message("3-1: e")
      endif()
      unset(r)
      unset(CACHE{r})  # Prevent uncertainty from cache fallback
      unset(CACHE{c})
      unset(ENV{e})
      if(NOT DEFINED r)
        message("3-2: r")
      endif()
      if(NOT DEFINED CACHE{c})
        message("3-2: c")
      endif()
      if(NOT DEFINED ENV{e})
        message("3-2: e")
      endif()
    """
    path = pathlib.Path("test-5.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "3-1: r",
        "3-1: c",
        "3-1: e",
        "3-2: r",
        "3-2: c",
        "3-2: e",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from empty to unset via set()
    text = r"""
      set(r "")
      set(CACHE{c} FORCE VALUE "")
      set(ENV{e} "x")  # CMake quirk: Cannot change environement variable directly from unset to empty
      set(ENV{e} "")
      if(DEFINED r AND r STREQUAL "")
        message("4-1: r")
      endif()
      set(val "$CACHE{c}")
      if(DEFINED CACHE{c} AND val STREQUAL "")
        message("4-1: c")
      endif()
      set(val "$ENV{e}")
      if(DEFINED ENV{e} AND val STREQUAL "")
        message("4-1: e")
      endif()
      set(r)
      unset(CACHE{r})  # Prevent uncertainty from cache fallback
      set(CACHE{c} FORCE VALUE)
      set(ENV{e})
      if(NOT DEFINED r)
        message("4-2: r")
      endif()
      if(DEFINED CACHE{c})
        message("4-2: c - FAILED")
      endif()
      if(DEFINED ENV{e})
        message("4-2: e - FAILED")
      endif()
    """
    path = pathlib.Path("test-6.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "4-1: r",
        "4-1: c",
        "4-1: e",
        "4-2: r",
        "4-2: c - FAILED",  # In CMake, `set(CACHE{c} FORCE VALUE)` sets `c` to the empty string
        "4-2: e - FAILED",  # CMake quirk: Cannot change environement variable directly from empty to unset
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from empty to nonempty
    text = r"""
      set(r "")
      set(CACHE{c} FORCE VALUE "")
      set(ENV{e} "x")  # CMake quirk: Cannot change environement variable directly from unset to empty
      set(ENV{e} "")
      if(DEFINED r AND r STREQUAL "")
        message("5-1: r")
      endif()
      set(val "$CACHE{c}")
      if(DEFINED CACHE{c} AND val STREQUAL "")
        message("5-1: c")
      endif()
      set(val "$ENV{e}")
      if(DEFINED ENV{e} AND val STREQUAL "")
        message("5-1: e")
      endif()
      set(r "x")
      set(CACHE{c} FORCE VALUE "x")
      set(ENV{e} "x")
      if(r STREQUAL "x")
        message("5-2: r")
      endif()
      set(val "$CACHE{c}")
      if(val STREQUAL "x")
        message("5-2: c")
      endif()
      set(val "$ENV{e}")
      if(val STREQUAL "x")
        message("5-2: e")
      endif()
    """
    path = pathlib.Path("test-7.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "5-1: r",
        "5-1: c",
        "5-1: e",
        "5-2: r",
        "5-2: c",
        "5-2: e",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from nonempty to unset via unset()
    text = r"""
      set(r "x")
      set(CACHE{c} FORCE VALUE "x")
      set(ENV{e} "x")
      if(r STREQUAL "x")
        message("6-1: r")
      endif()
      set(val "$CACHE{c}")
      if(val STREQUAL "x")
        message("6-1: c")
      endif()
      set(val "$ENV{e}")
      if(val STREQUAL "x")
        message("6-1: e")
      endif()
      unset(r)
      unset(CACHE{r})  # Prevent uncertainty from cache fallback
      unset(CACHE{c})
      unset(ENV{e})
      if(NOT DEFINED r)
        message("6-2: r")
      endif()
      if(NOT DEFINED CACHE{c})
        message("6-2: c")
      endif()
      if(NOT DEFINED ENV{e})
        message("6-2: e")
      endif()
    """
    path = pathlib.Path("test-8.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "6-1: r",
        "6-1: c",
        "6-1: e",
        "6-2: r",
        "6-2: c",
        "6-2: e",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from nonempty to unset via set()
    text = r"""
      set(r "x")
      set(CACHE{c} FORCE VALUE "x")
      set(ENV{e} "x")
      if(r STREQUAL "x")
        message("7-1: r")
      endif()
      set(val "$CACHE{c}")
      if(val STREQUAL "x")
        message("7-1: c")
      endif()
      set(val "$ENV{e}")
      if(val STREQUAL "x")
        message("7-1: e")
      endif()
      set(r)
      unset(CACHE{r})  # Prevent uncertainty from cache fallback
      set(CACHE{c} FORCE VALUE)
      set(ENV{e})
      if(NOT DEFINED r)
        message("7-2: r")
      endif()
      if(DEFINED CACHE{c})
        message("7-2: c - FAILED")
      endif()
      if(DEFINED ENV{e})
        message("7-2: e - FAILED")
      endif()
    """
    path = pathlib.Path("test-9.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "7-1: r",
        "7-1: c",
        "7-1: e",
        "7-2: r",
        "7-2: c - FAILED",  # In CMake, `set(CACHE{c} FORCE VALUE)` sets `c` to the empty string
        "7-2: e - FAILED",  # CMake quirk: Cannot change environement variable directly from empty to unset
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Change from nonempty to empty
    text = r"""
      set(r "x")
      set(CACHE{c} FORCE VALUE "x")
      set(ENV{e} "x")
      if(r STREQUAL "x")
        message("8-1: r")
      endif()
      set(val "$CACHE{c}")
      if(val STREQUAL "x")
        message("8-1: c")
      endif()
      set(val "$ENV{e}")
      if(val STREQUAL "x")
        message("8-1: e")
      endif()
      set(r "")
      set(CACHE{c} FORCE VALUE "")
      set(ENV{e} "")
      if(DEFINED r AND r STREQUAL "")
        message("8-2: r")
      endif()
      set(val "$CACHE{c}")
      if(DEFINED CACHE{c} AND val STREQUAL "")
        message("8-2: c")
      endif()
      set(val "$ENV{e}")
      if(DEFINED ENV{e} AND val STREQUAL "")
        message("8-2: e")
      endif()
    """
    path = pathlib.Path("test-10.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "8-1: r",
        "8-1: c",
        "8-1: e",
        "8-2: r",
        "8-2: c",
        "8-2: e",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty propagation
    text = r"""
      set(u2 "${u1}")
      set(u3 "--${u2}--")
      message("${u3}")
    """
    path = pathlib.Path("test-11.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u3"), _tp.TextPos(3, 9)),
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u2"),  _tp.TextPos(2, 10)),
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u1"),  _tp.TextPos(1, 8)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_VariableExpansion(context: _t.Context) -> None:
    expected: typing.Any

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
    path = pathlib.Path("test-1.cmake")
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
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Empty variable name
    text = r"""
      set(empty "")
      set("" "empty")
      message("1: (${})")
      message("2: (${${empty}})")
      message("3: (${${empty}${empty}})")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: (empty)",
        "2: (empty)",
        "3: (empty)",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

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
    path = pathlib.Path("test-3.cmake")
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
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        ("Invalid domain", _tp.TextPos(1, 14)),
        ("Invalid domain", _tp.TextPos(2, 14)),
        ("Invalid domain", _tp.TextPos(3, 16)),
        ("Invalid literal character", _tp.TextPos(10, 16)),
        ("Invalid literal character", _tp.TextPos(14, 25)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_in(expected[0], error.message)
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Uncertainty
    text = r"""
      set(x "")
      message("--${u}--")
      message("--${a${u}b}--")
      message("--${a${x}c${u}c}--")
      message("--${a${x}c${a${u}b}c}--")
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(2, 11)),
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(3, 14)),
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(4, 19)),
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(5, 22)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_String(context: _t.Context) -> None:
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
    context.check_equal(message.message, "foo-FOO")
    context.check_equal(message.file_pos, _tp.FilePos(path, 4, 0))
    context.check_equal(message.level, _cp.MessageLevel.NOTICE)
    context.check_is_none(message.occurrence_uncertainty)


def test_CMakeProcess_List(context: _t.Context) -> None:
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
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-A-B-A;B",
        "A;B;C\\;D;E;F;G\\;H",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Special cases involving unset lists, lists being the empty string, no elements
    # appended, and the empty string being appended
    text = r"""
      unset(CACHE{l}) # Avoid uncertainty from cache fallback

      # List is initially unset
      set(l)
      list(APPEND l)
      if(NOT DEFINED l)
        message("1")
      endif()
      set(l)
      list(APPEND l "")
      if(DEFINED l AND l STREQUAL "")
        message("2")
      endif()
      set(l)
      list(APPEND l "x")
      if(l STREQUAL "x")
        message("3")
      endif()

      # List is initially the empty string
      set(l "")
      list(APPEND l)
      if(DEFINED l AND l STREQUAL "")
        message("4")
      endif()
      set(l "")
      list(APPEND l "")
      if(DEFINED l AND l STREQUAL "")
        message("5")
      endif()
      set(l "")
      list(APPEND l "x")
      if(l STREQUAL "x")
        message("6")
      endif()

      # List is initially nonempty
      set(l "x")
      list(APPEND l)
      if(l STREQUAL "x")
        message("7")
      endif()
      set(l "x")
      list(APPEND l "")
      if(l STREQUAL "x;")
        message("8")
      endif()
      set(l "x")
      list(APPEND l "y")
      if(l STREQUAL "x;y")
        message("9")
      endif()
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [ "1", "2", "3", "4", "5", "6", "7", "8", "9" ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)


def test_CMakeProcess_If(context: _t.Context) -> None:
    expected_messages: typing.Any

    text = r"""
      if(FALSE)
        message("1")
      endif()
      if(TRUE)
        message("2")
      endif()

      if(FALSE)
        message("3")
      else()
        message("4")
      endif()
      if(TRUE)
        message("5")
      else()
        message("6")
      endif()

      if(FALSE)
        message("7")
      elseif(FALSE)
        message("8")
      endif()
      if(FALSE)
        message("9")
      elseif(TRUE)
        message("10")
      endif()
      if(TRUE)
        message("11")
      elseif(FALSE)
        message("12")
      endif()
      if(TRUE)
        message("13")
      elseif(TRUE)
        message("14")
      endif()

      if(FALSE)
        message("15")
      elseif(FALSE)
        message("16")
      else()
        message("17")
      endif()
      if(FALSE)
        message("18")
      elseif(TRUE)
        message("19")
      else()
        message("20")
      endif()
      if(TRUE)
        message("21")
      elseif(FALSE)
        message("22")
      else()
        message("23")
      endif()
      if(TRUE)
        message("24")
      elseif(TRUE)
        message("25")
      else()
        message("26")
      endif()
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = ["2", "4", "5", "10", "11", "13", "17", "19", "21", "24"]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(x1 "")
      set(y1 "")
      set(x2 "ON")
      set(y2 "ON")

      if(x3)
        message("1") # U
      endif()

      if(x3)
        message("2") # U
      else()
        message("3") # U
      endif()

      if(x1)
        message("4")
      elseif(y3)
        message("5") # U
      endif()
      if(x2)
        message("6") # C
      elseif(y3)
        message("7")
      endif()
      if(x3)
        message("8") # U
      elseif(y1)
        message("9")
      endif()
      if(x3)
        message("10") # U
      elseif(y2)
        message("11") # U
      endif()
      if(x3)
        message("12") # U
      elseif(y3)
        message("13") # U
      endif()

      if(x1)
        message("14")
      elseif(y3)
        message("15") # U
      else()
        message("16") # U
      endif()
      if(x2)
        message("17") # C
      elseif(y3)
        message("18")
      else()
        message("19")
      endif()
      if(x3)
        message("20") # U
      elseif(y1)
        message("21")
      else()
        message("22") # U
      endif()
      if(x3)
        message("23") # U
      elseif(y2)
        message("24") # U
      else()
        message("25")
      endif()

      if(x3)
        if(y1)
          message("26")
        endif()
        if(y2)
          message("27") # U
        endif()
        if(y3)
          message("28") # U
        endif()
      endif()
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        ("1",  False),
        ("2",  False),
        ("3",  False),
        ("5",  False),
        ("6",  True),
        ("8",  False),
        ("10", False),
        ("11", False),
        ("12", False),
        ("13", False),
        ("15", False),
        ("16", False),
        ("17", True),
        ("20", False),
        ("22", False),
        ("23", False),
        ("24", False),
        ("27", False),
        ("28", False),
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(not message.occurrence_uncertainty, expected[1])

    # Tainting caused by occurrence uncertainty
    text = r"""
      set(r1 "foo")
      set(r2 "foo")
      set(r3 "foo")
      set(r4 "foo")
      set(CACHE{c1} FORCE VALUE "foo")
      set(CACHE{c2} FORCE VALUE "foo")
      set(CACHE{c3} FORCE VALUE "foo")
      set(CACHE{c4} FORCE VALUE "foo")
      set(ENV{e1} "foo")
      set(ENV{e2} "foo")
      set(ENV{e3} "foo")
      set(ENV{e4} "foo")
      if(u1)
        set(r1 "bar")
        set(CACHE{c1} FORCE VALUE "bar")
        set(ENV{e1} "bar")
      elseif(u2)
        set(r2 "bar")
        set(CACHE{c2} FORCE VALUE "bar")
        set(ENV{e2} "bar")
      else()
        set(r3 "bar")
        set(CACHE{c3} FORCE VALUE "bar")
        set(ENV{e3} "bar")
      endif()
      if(u1)
      elseif(TRUE)
        set(r4 "bar")
        set(CACHE{c4} FORCE VALUE "bar")
        set(ENV{e4} "bar")
      endif()
      message("1:  ${r1}")
      message("2:  ${r2}")
      message("3:  ${r3}")
      message("4:  ${r4}")
      message("5:  $CACHE{c1}")
      message("6:  $CACHE{c2}")
      message("7:  $CACHE{c3}")
      message("8:  $CACHE{c4}")
      message("9:  $ENV{e1}")
      message("10: $ENV{e2}")
      message("11: $ENV{e3}")
      message("12: $ENV{e4}")
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "r1"),   _tp.TextPos(32, 13)), #  1
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(14, 2)),  #  2
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),     _tp.TextPos(13, 3)),  #  3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "r2"),   _tp.TextPos(33, 13)), #  4
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(18, 2)),  #  5
        (expansion_uncertainty_cause("elseif", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(17, 7)),  #  6
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "r3"),   _tp.TextPos(34, 13)), #  7
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(22, 2)),  #  8
        (expansion_uncertainty_cause("elseif", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(17, 7)),  #  9
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "r4"),   _tp.TextPos(35, 13)), # 10
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(28, 2)),  # 11
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),     _tp.TextPos(26, 3)),  # 12
        (invoke_uncertainty_error("message", _cur.ParamType.CACHE_VAR, "c1"),     _tp.TextPos(36, 13)), # 13
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(15, 2)),  # 14
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),     _tp.TextPos(13, 3)),  # 15
        (invoke_uncertainty_error("message", _cur.ParamType.CACHE_VAR, "c2"),     _tp.TextPos(37, 13)), # 16
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(19, 2)),  # 17
        (expansion_uncertainty_cause("elseif", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(17, 7)),  # 18
        (invoke_uncertainty_error("message", _cur.ParamType.CACHE_VAR, "c3"),     _tp.TextPos(38, 13)), # 19
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(23, 2)),  # 20
        (expansion_uncertainty_cause("elseif", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(17, 7)),  # 21
        (invoke_uncertainty_error("message", _cur.ParamType.CACHE_VAR, "c4"),     _tp.TextPos(39, 13)), # 22
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(29, 2)),  # 23
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),     _tp.TextPos(26, 3)),  # 24
        (invoke_uncertainty_error("message", _cur.ParamType.ENV_VAR, "e1"),       _tp.TextPos(40, 13)), # 25
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(16, 2)),  # 26
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),     _tp.TextPos(13, 3)),  # 27
        (invoke_uncertainty_error("message", _cur.ParamType.ENV_VAR, "e2"),       _tp.TextPos(41, 13)), # 28
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(20, 2)),  # 29
        (expansion_uncertainty_cause("elseif", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(17, 7)),  # 30
        (invoke_uncertainty_error("message", _cur.ParamType.ENV_VAR, "e3"),       _tp.TextPos(42, 13)), # 31
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(24, 2)),  # 32
        (expansion_uncertainty_cause("elseif", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(17, 7)),  # 33
        (invoke_uncertainty_error("message", _cur.ParamType.ENV_VAR, "e4"),       _tp.TextPos(43, 13)), # 34
        (occurrence_uncertainty_cause("set"),                                     _tp.TextPos(30, 2)),  # 35
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),     _tp.TextPos(26, 3)),  # 36
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Cancellation of tainting caused by occurrence uncertainty
    text = r"""
      set(r1 "foo")
      set(r2 "foo")
      set(r3 "foo")
      set(CACHE{c1} FORCE VALUE "foo")
      set(CACHE{c2} FORCE VALUE "foo")
      set(CACHE{c3} FORCE VALUE "foo")
      set(ENV{e1} "foo")
      set(ENV{e2} "foo")
      set(ENV{e3} "foo")
      if(u1)
        set(r1 "foo")                     # No change
        set(CACHE{c1} FORCE VALUE "foo")  # No change
        set(ENV{e1} "foo")                # No change
      elseif(u2)
        set(r2 "foo")                     # No change
        set(CACHE{c2} FORCE VALUE "foo")  # No change
        set(ENV{e2} "foo")                # No change
      else()
        set(r3 "bar")
        set(CACHE{c3} FORCE VALUE "bar")
        set(ENV{e3} "bar")
        set(r3 "foo")                     # Original value restored
        set(CACHE{c3} FORCE VALUE "foo")  # Original value restored
        set(ENV{e3} "foo")                # Original value restored
      endif()
      message("-${r1}-${r2}-${r3}-$CACHE{c1}-$CACHE{c2}-$CACHE{c3}-$ENV{e1}-$ENV{e2}-$ENV{e3}-")
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-foo-foo-foo-foo-foo-foo-foo-foo-foo-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Parent scope tainting caused by occurrence uncertainty
    text = r"""
      set(x1 "foo")
      set(x2 "foo")
      block()
        block()
          if(u)
            set(x1 "bar" PARENT_SCOPE)
            set(x2 "bar" PARENT_SCOPE)
            set(x2 "foo" PARENT_SCOPE)  # Restore original value
          endif()
          message("1: -${x1}-${x2}-")
        endblock()
        message("2: -${x1}-")
        message("3: -${x2}-")
      endblock()
      message("4: -${x1}-${x2}-")
    """
    path = pathlib.Path("test-5.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    expected_messages = [
        "1: -foo-foo-",
        "3: -foo-",
        "4: -foo-foo-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x1"), _tp.TextPos(12, 15)),
        (occurrence_uncertainty_cause("set"),                                   _tp.TextPos(6, 6)),
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u"),    _tp.TextPos(5, 7)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_Foreach(context: _t.Context) -> None:
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
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_equal(message.file_pos, _tp.FilePos(path, 3, 2))
        subcontext.check_equal(message.level, _cp.MessageLevel.NOTICE)
        subcontext.check_is_none(message.occurrence_uncertainty)


def test_CMakeProcess_Macro(context: _t.Context) -> None:
    expected_messages: typing.Any

    text = r"""
      macro(foo _x)
        message("Foo ${_x}")
      endmacro()
      foo("Bar")
      Foo("Baz")  # Mixed case in command name
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "Foo Bar",
        "Foo Baz",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    text = r"""
      macro(foo _x _y)
        message("${_x}${_y}aa} -- ${_y}${_x}" [[ -- ${_x}${_y}]])
      endmacro()
      set(foo "bar")
      set(faa "boo")
      foo("oo}" "\${f")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.message, "oo}boo -- bar -- ${_x}${_y}")
    context.check_equal(message.file_pos, _tp.FilePos(path, 2, 2))
    context.check_is_none(message.occurrence_uncertainty)

    # That special macro variables work correctly
    text = r"""
      set(ARGV1 "v1")
      set(ARGV2 "v2")
      macro(foo x)
        message("1: ${ARGC}-${ARGN}-${ARGV}-${ARGV0}-${ARGV1}-${ARGV2}")
      endmacro()
      foo("x")
      foo("x" "y")
      foo("x" "y" "z")
      foo("a;b" "c;d" "e;f")

      set(ARGC "c")
      set(ARGN "n")
      set(ARGV "v")
      set(ARGV0 "v0")
      macro(bar)
        message("2: ${ARGC}-${ARGN}-${ARGV}-${ARGV0}")
      endmacro()
      bar()
      bar("x")
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        ("1: 1--x-x-v1-v2",                      _tp.TextPos(4, 2)),  # 1
        ("1: 2-y-x;y-x-y-v2",                    _tp.TextPos(4, 2)),  # 2
        ("1: 3-y;z-x;y;z-x-y-z",                 _tp.TextPos(4, 2)),  # 3
        ("1: 3-c;d;e;f-a;b;c;d;e;f-a;b-c;d-e;f", _tp.TextPos(4, 2)),  # 4
        ("2: 0---v0",                            _tp.TextPos(16, 2)), # 5
        ("2: 1-x-x-x",                           _tp.TextPos(16, 2)), # 6
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos.text_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)

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
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.message, "a-b-d")
    context.check_equal(message.file_pos, _tp.FilePos(path, 3, 4))
    context.check_is_none(message.occurrence_uncertainty)

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
    path = pathlib.Path("test-5.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.message, "click 1")
    context.check_is_none(message.occurrence_uncertainty)
    context.check_equal(len(result.errors), 1)
    error = result.errors[0]
    context.check_in("Regular expression syntax error: Invalid range", error.message)
    context.check_equal(error.file_pos, _tp.FilePos(path, 5, 53))

    # Argument uncertainty
    text = r"""
      macro(foo a b)
        message("x${a}y")
        macro(bar a)
          message("x${a}y")
          message("x${b}y")
        endmacro()
        bar("*")
        macro(baz c)
          message("x${c}y")
        endmacro()
        baz("*")
        baz("${a}")
        baz("-${b}-")
      endmacro()
      foo("${_1}" "${_2}")
    """
    path = pathlib.Path("test-6.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    expected_messages = [
        ("x*y", _tp.TextPos(9, 4)),
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos.text_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.MACRO_PARAM, "a"), _tp.TextPos(2, 12)),  #  1
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_1"), _tp.TextPos(15, 5)),  #  2
        (invoke_uncertainty_error("message", _cur.ParamType.MACRO_PARAM, "a"), _tp.TextPos(4, 14)),  #  3
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_1"), _tp.TextPos(15, 5)),  #  4
        (invoke_uncertainty_error("message", _cur.ParamType.MACRO_PARAM, "b"), _tp.TextPos(5, 14)),  #  5
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_2"), _tp.TextPos(15, 13)), #  6
        (invoke_uncertainty_error("message", _cur.ParamType.MACRO_PARAM, "c"), _tp.TextPos(9, 14)),  #  7
        (expansion_uncertainty_cause("baz", _cur.ParamType.MACRO_PARAM, "a"),  _tp.TextPos(12, 7)),  #  8
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_1"), _tp.TextPos(15, 5)),  #  9
        (invoke_uncertainty_error("message", _cur.ParamType.MACRO_PARAM, "c"), _tp.TextPos(9, 14)),  # 10
        (expansion_uncertainty_cause("baz", _cur.ParamType.MACRO_PARAM, "b"),  _tp.TextPos(13, 8)),  # 11
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_2"), _tp.TextPos(15, 13)), # 12
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_Block(context: _t.Context) -> None:
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
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: A-B",
        "2: C-B",
        "3: A-D",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Invalid arguments
    text = r"""
      block(  Foo  )
      endblock()
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    context.check_equal(len(result.errors), 1)
    error = result.errors[0]
    context.check_in("Unrecognized first argument", error.message)
    context.check_equal(error.file_pos, _tp.FilePos(path, 1, 8))


def invoke_uncertainty_error(command_name: str, param_type: _cur.ParamType, param_name: str) -> str:
    return 'Failed to invoke %s() due to expansion of %s with uncertain value' % \
        (command_name, _cur.get_qual_param_ref(param_type, param_name))

def expansion_uncertainty_cause(command_name: str, param_type: _cur.ParamType, param_name: str) -> str:
    return 'Caused by expansion of %s with uncertain value in invocation of %s()' % \
        (_cur.get_qual_param_ref(param_type, param_name), command_name)

def occurrence_uncertainty_cause(command_name: str) -> str:
    return 'Caused by execution of %s() with uncertain occurrence' % command_name


def _trim_cmake_text(text: str) -> str:
    return textwrap.dedent(text.removeprefix("\n"))


def _process(cmake_text: str, cmake_path: pathlib.Path, context: _t.Context) -> tuple[bool, _Result]:
    input_ = io.StringIO(cmake_text)
    cmake_source = _cp.Source(input_, cmake_path)
    pos_resolver = _cp.PositionResolver()
    result = _Result()
    application = _Application(pos_resolver, result, context.logger)
    config = _cp.Config()
    config.define_breakpoint_command = True
    success = _cp.process(cmake_source, application, pos_resolver, config)
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
