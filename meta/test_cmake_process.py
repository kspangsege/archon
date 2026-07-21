from __future__ import annotations

import typing
import dataclasses
import textwrap
import pathlib
import errno
import os
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


def test_CMakeProcess_MatchesOperator(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      if("x" MATCHES "x")
        message("1: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
      endif()

      # Only needs to find match on substring
      if("xyx" MATCHES "y")
        message("2: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
      endif()

      if("xyx" MATCHES "y|z")
        message("3: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
      endif()

      if("xyx" MATCHES "x(y|z)x")
        message("4: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-${CMAKE_MATCH_1}-")
      endif()

      # Variables are also visible after endif()
      message("5: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-${CMAKE_MATCH_1}-")

      # Variables for unused capture groups are unaffected
      set(CMAKE_MATCH_1 "value 1")
      set(CMAKE_MATCH_2 "value 2")
      if("x" MATCHES "x")
      endif()
      message("6: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-${CMAKE_MATCH_1}-${CMAKE_MATCH_2}-")
      if("x" MATCHES "(x)")
      endif()
      message("7: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-${CMAKE_MATCH_1}-${CMAKE_MATCH_2}-")

      # Strangely, in CMake, variables for capture groups with empty matches are unaffected
      set(CMAKE_MATCH_1 "value 1")
      set(CMAKE_MATCH_2 "value 2")
      if("xyx" MATCHES "(z*)y(x)")
      endif()
      message("8: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-${CMAKE_MATCH_1}-${CMAKE_MATCH_2}-")

      # CMAKE_MATCH_COUNT is set to the number associated with the highest capture group that has a nonempty match
      set(CMAKE_MATCH_1 "value 1")
      set(CMAKE_MATCH_2 "value 2")
      if("xyx" MATCHES "(x)y(z*)")
      endif()
      message("9: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-${CMAKE_MATCH_1}-${CMAKE_MATCH_2}-")

      # Strangely, CMake sets CMAKE_MATCH_COUNT to the empty string when the full match is the empty string
      set(CMAKE_MATCH_COUNT "value 1")
      set(CMAKE_MATCH_0     "value 2")
      if("xyx" MATCHES "z*")
        message("10: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
      endif()

      # Empty regular expression allowed
      set(CMAKE_MATCH_COUNT "value 1")
      set(CMAKE_MATCH_0     "value 2")
      if("xyx" MATCHES "")
        message("11: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
      endif()

      # Non-match
      set(CMAKE_MATCH_COUNT "value 1")
      set(CMAKE_MATCH_0     "value 2")
      if("xyx" MATCHES "x(a|b)x")
      else()
        message("12: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
      endif()
      message("13: -${CMAKE_MATCH_COUNT}-${CMAKE_MATCH_0}-")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: -0-x-",                 #  1
        "2: -0-y-",                 #  2
        "3: -0-y-",                 #  3
        "4: -1-xyx-y-",             #  4
        "5: -1-xyx-y-",             #  5
        "6: -0-x-value 1-value 2-", #  6
        "7: -1-x-x-value 2-",       #  7
        "8: -2-yx-value 1-x-",      #  8
        "9: -1-xy-x-value 2-",      #  9
        "10: --value 2-",           # 10
        "11: --value 2-",           # 11
        "12: -0-value 2-",          # 12
        "13: -0-value 2-",          # 13
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Invalidity
    text = """
      if("x" MATCHES "[2-1]")  # Invalaid range
      endif()
      if("x" MATCHES "x**")  # Invalaid double quantification
      endif()
      set(e "xx[2-1]xx")
      if("x" MATCHES "yyyy${e}yyyy")  # Invalaid range inside variable
      endif()
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    error_prefix = "Failed to evaluate if() condition: Regular expression syntax error: "
    expected_errors = [
        (error_prefix + 'Invalid range ("2-1")',           _tp.TextPos(1, 17)), # 1
        (error_prefix + 'Invalid use of quantifier ("*")', _tp.TextPos(3, 18)), # 2
        (error_prefix + 'Invalid range ("2-1")',           _tp.TextPos(6, 20)), # 3
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Strict mode uncertainty
    text = """
      if("x${u1}x" MATCHES "x(y|z)x")
        set(u2 "v")
      endif()
      message("${u2}")
      set(u4 "${u3}")
      if("xyx" MATCHES "x${u3}x")
      endif()
      if("xyx" MATCHES "x${u4}x")
      endif()
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(4, 9)),  # 1
        (occurrence_uncertainty_cause("set"),                                   _tp.TextPos(2, 2)),  # 2
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),   _tp.TextPos(1, 5)),  # 3
        (invoke_uncertainty_error("if", _cur.ParamType.REGULAR_VAR, "u3"),      _tp.TextPos(6, 19)), # 4
        (invoke_uncertainty_error("if", _cur.ParamType.REGULAR_VAR, "u4"),      _tp.TextPos(8, 19)), # 5
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u3"),  _tp.TextPos(5, 8)),  # 6
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Lenient mode uncertainty
    text = """
      if("x${u1}x" MATCHES "x(y|z)x")
        set(u2 "v")
      endif()
      message("${u2}")
      set(u4 "${u3}")
      if("xyx" MATCHES "x${u3}x")
        set(u5 "v")
      endif()
      message("${u5}")
      if("xyx" MATCHES "x${u4}x")
        set(u6 "v")
      endif()
      message("${u6}")
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context, lenient_mode=True)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(4, 9)),   #  1
        (occurrence_uncertainty_cause("set"),                                   _tp.TextPos(2, 2)),   #  2
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u1"),   _tp.TextPos(1, 5)),   #  3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u5"), _tp.TextPos(9, 9)),   #  4
        (occurrence_uncertainty_cause("set"),                                   _tp.TextPos(7, 2)),   #  5
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u3"),   _tp.TextPos(6, 19)),  #  6
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u6"), _tp.TextPos(13, 9)),  #  7
        (occurrence_uncertainty_cause("set"),                                   _tp.TextPos(11, 2)),  #  8
        (expansion_uncertainty_cause("if", _cur.ParamType.REGULAR_VAR, "u4"),   _tp.TextPos(10, 19)), #  9
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u3"),  _tp.TextPos(5, 8)),   # 10
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_StringAppend(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      # Avoid uncertainty from cache fallback
      unset(CACHE{x1})
      unset(CACHE{x2})
      unset(CACHE{x3})
      unset(CACHE{x4})

      # Variable is initially unset
      unset(x1)
      unset(x2)
      unset(x3)
      unset(x4)
      string(APPEND x1)
      string(APPEND x2 "")
      string(APPEND x3 "a")
      string(APPEND x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("1: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")

      # Variable is initially empty
      set(x1 "")
      set(x2 "")
      set(x3 "")
      set(x4 "")
      string(APPEND x1)
      string(APPEND x2 "")
      string(APPEND x3 "a")
      string(APPEND x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("2: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")

      # Variable is initially nonempty
      set(x1 "v")
      set(x2 "v")
      set(x3 "v")
      set(x4 "v")
      string(APPEND x1)
      string(APPEND x2 "")
      string(APPEND x3 "a")
      string(APPEND x4 "a" "b")
      message("3: -${x1}-${x2}-${x3}-${x4}-")

      # Cache involvement
      set(CACHE{x1} FORCE VALUE "v")
      set(CACHE{x2} FORCE VALUE "v")
      set(CACHE{x3} FORCE VALUE "v")
      set(CACHE{x4} FORCE VALUE "v")
      unset(x1)
      unset(x2)
      unset(x3)
      unset(x4)
      string(APPEND x1)
      string(APPEND x2 "")
      string(APPEND x3 "a")
      string(APPEND x4 "a" "b")
      message("4: -${x1}:$CACHE{x1}-${x2}:$CACHE{x1}-${x3}:$CACHE{x1}-${x4}:$CACHE{x1}-")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: -n:-y:-a-ab-",
        "2: -y:-y:-a-ab-",
        "3: -v-v-va-vab-",
        "4: -v:v-v:v-va:v-vab:v-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(x "v")
      set(u1 "${u}")
      set(u2 "${u}")
      string(APPEND x "${u1}")
      string(APPEND u2 "v")
      string(APPEND u3 "${u1}")
      message("-${x}-")
      message("-${u2}-")
      message("-${u3}-")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x"),    _tp.TextPos(7, 10)), # 1
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u1"), _tp.TextPos(4, 17)), # 2
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(2, 8)),  # 3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u2"),   _tp.TextPos(8, 10)), # 4
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(3, 8)),  # 5
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u3"),   _tp.TextPos(9, 10)), # 6
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_StringPrepend(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      # Avoid uncertainty from cache fallback
      unset(CACHE{x1})
      unset(CACHE{x2})
      unset(CACHE{x3})
      unset(CACHE{x4})

      # Variable is initially unset
      unset(x1)
      unset(x2)
      unset(x3)
      unset(x4)
      string(PREPEND x1)
      string(PREPEND x2 "")
      string(PREPEND x3 "a")
      string(PREPEND x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("1: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")

      # Variable is initially empty
      set(x1 "")
      set(x2 "")
      set(x3 "")
      set(x4 "")
      string(PREPEND x1)
      string(PREPEND x2 "")
      string(PREPEND x3 "a")
      string(PREPEND x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("2: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")

      # Variable is initially nonempty
      set(x1 "v")
      set(x2 "v")
      set(x3 "v")
      set(x4 "v")
      string(PREPEND x1)
      string(PREPEND x2 "")
      string(PREPEND x3 "a")
      string(PREPEND x4 "a" "b")
      message("3: -${x1}-${x2}-${x3}-${x4}-")

      # Cache involvement
      set(CACHE{x1} FORCE VALUE "v")
      set(CACHE{x2} FORCE VALUE "v")
      set(CACHE{x3} FORCE VALUE "v")
      set(CACHE{x4} FORCE VALUE "v")
      unset(x1)
      unset(x2)
      unset(x3)
      unset(x4)
      string(PREPEND x1)
      string(PREPEND x2 "")
      string(PREPEND x3 "a")
      string(PREPEND x4 "a" "b")
      message("4: -${x1}:$CACHE{x1}-${x2}:$CACHE{x1}-${x3}:$CACHE{x1}-${x4}:$CACHE{x1}-")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: -n:-y:-a-ab-",
        "2: -y:-y:-a-ab-",
        "3: -v-v-av-abv-",
        "4: -v:v-v:v-av:v-abv:v-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(x "v")
      set(u1 "${u}")
      set(u2 "${u}")
      string(PREPEND x "${u1}")
      string(PREPEND u2 "v")
      string(PREPEND u3 "${u1}")
      message("-${x}-")
      message("-${u2}-")
      message("-${u3}-")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x"),    _tp.TextPos(7, 10)), # 1
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u1"), _tp.TextPos(4, 18)), # 2
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(2, 8)),  # 3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u2"),   _tp.TextPos(8, 10)), # 4
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(3, 8)),  # 5
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u3"),   _tp.TextPos(9, 10)), # 6
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_StringConcat(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      # Avoid uncertainty from cache fallback
      unset(CACHE{x1})
      unset(CACHE{x2})
      unset(CACHE{x3})
      unset(CACHE{x4})

      # Variable is initially unset
      unset(x1)
      unset(x2)
      unset(x3)
      unset(x4)
      string(CONCAT x1)
      string(CONCAT x2 "")
      string(CONCAT x3 "a")
      string(CONCAT x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("1: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")

      # Variable is initially empty
      set(x1 "")
      set(x2 "")
      set(x3 "")
      set(x4 "")
      string(CONCAT x1)
      string(CONCAT x2 "")
      string(CONCAT x3 "a")
      string(CONCAT x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("2: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")

      # Variable is initially nonempty
      set(x1 "v")
      set(x2 "v")
      set(x3 "v")
      set(x4 "v")
      string(CONCAT x1)
      string(CONCAT x2 "")
      string(CONCAT x3 "a")
      string(CONCAT x4 "a" "b")
      if(DEFINED x1)
        set(dx1 "y")
      else()
        set(dx1 "n")
      endif()
      if(DEFINED x2)
        set(dx2 "y")
      else()
        set(dx2 "n")
      endif()
      message("3: -${dx1}:${x1}-${dx2}:${x2}-${x3}-${x4}-")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: -y:-y:-a-ab-",
        "2: -y:-y:-a-ab-",
        "3: -y:-y:-a-ab-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(u2 "${u1}")
      string(CONCAT x1 "${u2}")
      string(CONCAT x2 "v" "${u2}")
      string(CONCAT x3 "${u1}" "v")
      string(CONCAT x4 "${u2}" "${u1}")
      message("${x1}")
      message("${x2}")
      message("${x3}")
      message("${x4}")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x1"),   _tp.TextPos(6, 9)),  #  1
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(2, 18)), #  2
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u1"),    _tp.TextPos(1, 8)),  #  3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x2"),   _tp.TextPos(7, 9)),  #  4
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(3, 22)), #  5
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u1"),    _tp.TextPos(1, 8)),  #  6
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x3"),   _tp.TextPos(8, 9)),  #  7
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u1"), _tp.TextPos(4, 18)), #  8
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x4"),   _tp.TextPos(9, 9)),  #  9
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(5, 18)), # 10
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u1"),    _tp.TextPos(1, 8)),  # 11
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_StringTolowerAndToupper(context: _t.Context) -> None:
    text = r"""
      set(_x "Foo")
      string(TOLOWER "${_x}" _y)
      string(TOUPPER "${_x}" _z)
      message("${_y}-${_z}")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.message, "foo-FOO")
    context.check_equal(message.file_pos, _tp.FilePos(path, 4, 0))
    context.check_equal(message.level, _cp.MessageLevel.NOTICE)
    context.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(u1 "${u}")
      string(TOLOWER "${u1}" x)
      string(TOUPPER "${u2}" y)
      string(TOLOWER ${u2} z)    # Unquoted uncertain value expansion is fatal with this signature
      message("-${x}-")
      message("-${y}-")
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("string", _cur.ParamType.REGULAR_VAR, "u2"),    _tp.TextPos(4, 15)), # 1
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x"),    _tp.TextPos(5, 10)), # 2
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u1"), _tp.TextPos(2, 16)), # 3
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(1, 8)),  # 4
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "y"),    _tp.TextPos(6, 10)), # 5
        (expansion_uncertainty_cause("string", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(3, 16)), # 6
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_ListGet(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      set(l "a" "b")
      list(GET l 0 x)
      list(GET l 1 y)
      list(GET l +0 1 -2 " -1" z)  # Weirdly, CMake does allow for leading whitespace
      message("${x}-${y}-${z}")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "a-b-a;b;a;b",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Invalidity
    text = r"""
      unset(CACHE{l1})  # Prevent uncertainty from cache fallback
      set(l1)
      set(l2 "a" "b")
      list(GET l1      x)  # Too few indexes
      list(GET l2  i   x)  # Invalid integer
      list(GET l2 "1 " x)  # CMake does not allow for trailing whitespace
      list(GET l1  0   x)
      list(GET l1 -1   x)
      list(GET l2  2   x)
      list(GET l2 -3   x)
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        ("Too few indexes in list(GET) invocation",            _tp.TextPos(4, 17)),  # 1
        ('Invalid index ("i") in list(GET) invocation',        _tp.TextPos(5, 13)),  # 2
        ('Invalid index ("1 ") in list(GET) invocation',       _tp.TextPos(6, 12)),  # 3
        ("Index (0) is out of range in list(GET) invocation",  _tp.TextPos(7, 13)),  # 4
        ("Index (-1) is out of range in list(GET) invocation", _tp.TextPos(8, 12)),  # 5
        ("Index (2) is out of range in list(GET) invocation",  _tp.TextPos(9, 13)),  # 6
        ("Index (-3) is out of range in list(GET) invocation", _tp.TextPos(10, 12)), # 7
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Strict mode uncertainty
    text = r"""
      set(l "v")
      list(GET ${u}           )  # Maximal uncertainty
      list(GET ${u}          x)  # Certain target name
      list(GET ${u}   0 1    x)  # Uncertain separation between variable name and indexes
      list(GET "${u}" 0      x)  # Uncertain list variable name
      list(GET u      0      x)  # Uncertain list value
      list(GET l      "${u}" x)  # Uncertain index
      list(GET l      ${u}   x)  # Uncertain number of indexes
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(2, 9)),  # 1
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(3, 9)),  # 2
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(4, 9)),  # 3
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(5, 10)), # 4
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(6, 9)),  # 5
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(7, 17)), # 6
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"), _tp.TextPos(8, 16)), # 7
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Lenient mode uncertainty
    text = r"""
      set(l "v")
      list(GET ${u}            )  # Maximal uncertainty
      list(GET ${u}           x)  # Certain target name
      list(GET ${u}   0 1     x)  # Uncertain separation between variable name and indexes
      list(GET "${u}" 0      x1)  # Uncertain list variable name
      list(GET u      0      x2)  # Uncertain list value
      list(GET l      "${u}" x3)  # Uncertain index
      list(GET l      ${u}   x4)  # Uncertain number of indexes
      message("${x1}")
      message("${x2}")
      message("${x3}")
      message("${x4}")
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context, lenient_mode=True)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(2, 9)),   #  1
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(3, 9)),   #  2
        (invoke_uncertainty_error("list", _cur.ParamType.REGULAR_VAR, "u"),     _tp.TextPos(4, 9)),   #  3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x1"), _tp.TextPos(9, 9)),   #  4
        (expansion_uncertainty_cause("list", _cur.ParamType.REGULAR_VAR, "u"),  _tp.TextPos(5, 10)),  #  5
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x2"), _tp.TextPos(10, 9)),  #  6
        (expansion_uncertainty_cause("list", _cur.ParamType.REGULAR_VAR, "u"),  _tp.TextPos(6, 9)),   #  7
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x3"), _tp.TextPos(11, 9)),  #  8
        (expansion_uncertainty_cause("list", _cur.ParamType.REGULAR_VAR, "u"),  _tp.TextPos(7, 17)),  #  9
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x4"), _tp.TextPos(12, 9)),  # 10
        (expansion_uncertainty_cause("list", _cur.ParamType.REGULAR_VAR, "u"),  _tp.TextPos(8, 16)),  # 11
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_ListAppend(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      set(_a "")
      set(_b "A")
      set(_c "")
      set(_d "A")
      list(APPEND _a)
      list(APPEND _b)
      list(APPEND _c "B" "C")
      list(APPEND _d "B" "C")
      message("${_a}-${_b}-${_c}-${_d}")
      set(_l "A;B" "C\;D")
      list(APPEND _l "E;F" "G\;H")
      message("${_l}")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-A-B;C-A;B;C",
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

      # Cache involvement
      set(CACHE{l} FORCE VALUE "x")
      set(l)
      list(APPEND l)
      if(l STREQUAL "x")
        message("10")
      endif()
      set(l)
      list(APPEND l "")
      if(l STREQUAL "x;")
        message("11")
      endif()
      set(l)
      list(APPEND l "y")
      if(l STREQUAL "x;y")
        message("12")
      endif()
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(x "v")
      set(u1 "${u}")
      set(u2 "${u}")
      list(APPEND x "${u1}")
      list(APPEND u2 "v")
      list(APPEND u3 "${u1}")
      message("-${x}-")
      message("-${u2}-")
      message("-${u3}-")
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x"),  _tp.TextPos(7, 10)), # 1
        (expansion_uncertainty_cause("list", _cur.ParamType.REGULAR_VAR, "u1"), _tp.TextPos(4, 15)), # 2
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),   _tp.TextPos(2, 8)),  # 3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(8, 10)), # 4
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),   _tp.TextPos(3, 8)),  # 5
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u3"), _tp.TextPos(9, 10)), # 6
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_ListPrepend(context: _t.Context) -> None:
    expected: typing.Any

    text = r"""
      set(_a "")
      set(_b "A")
      set(_c "")
      set(_d "A")
      list(PREPEND _a)
      list(PREPEND _b)
      list(PREPEND _c "B" "C")
      list(PREPEND _d "B" "C")
      message("${_a}-${_b}-${_c}-${_d}")
      set(_l "A;B" "C\;D")
      list(PREPEND _l "E;F" "G\;H")
      message("${_l}")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-A-B;C-B;C;A",
        "E;F;G\\;H;A;B;C\\;D",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Special cases involving unset lists, lists being the empty string, no elements
    # prepended, and the empty string being prepended
    text = r"""
      unset(CACHE{l}) # Avoid uncertainty from cache fallback

      # List is initially unset
      set(l)
      list(PREPEND l)
      if(NOT DEFINED l)
        message("1")
      endif()
      set(l)
      list(PREPEND l "")
      if(DEFINED l AND l STREQUAL "")
        message("2")
      endif()
      set(l)
      list(PREPEND l "x")
      if(l STREQUAL "x")
        message("3")
      endif()

      # List is initially the empty string
      set(l "")
      list(PREPEND l)
      if(DEFINED l AND l STREQUAL "")
        message("4")
      endif()
      set(l "")
      list(PREPEND l "")
      if(DEFINED l AND l STREQUAL "")
        message("5")
      endif()
      set(l "")
      list(PREPEND l "x")
      if(l STREQUAL "x")
        message("6")
      endif()

      # List is initially nonempty
      set(l "x")
      list(PREPEND l)
      if(l STREQUAL "x")
        message("7")
      endif()
      set(l "x")
      list(PREPEND l "")
      if(l STREQUAL ";x")
        message("8")
      endif()
      set(l "x")
      list(PREPEND l "y")
      if(l STREQUAL "y;x")
        message("9")
      endif()

      # Cache involvement
      set(CACHE{l} FORCE VALUE "x")
      set(l)
      list(PREPEND l)
      if(l STREQUAL "x")
        message("10")
      endif()
      set(l)
      list(PREPEND l "")
      if(l STREQUAL ";x")
        message("11")
      endif()
      set(l)
      list(PREPEND l "y")
      if(l STREQUAL "y;x")
        message("12")
      endif()
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [ "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Uncertainty
    text = r"""
      set(x "v")
      set(u1 "${u}")
      set(u2 "${u}")
      list(PREPEND x "${u1}")
      list(PREPEND u2 "v")
      list(PREPEND u3 "${u1}")
      message("-${x}-")
      message("-${u2}-")
      message("-${u3}-")
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "x"),  _tp.TextPos(7, 10)), # 1
        (expansion_uncertainty_cause("list", _cur.ParamType.REGULAR_VAR, "u1"), _tp.TextPos(4, 16)), # 2
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),   _tp.TextPos(2, 8)),  # 3
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u2"), _tp.TextPos(8, 10)), # 4
        (expansion_uncertainty_cause("set", _cur.ParamType.REGULAR_VAR, "u"),   _tp.TextPos(3, 8)),  # 5
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "u3"), _tp.TextPos(9, 10)), # 6
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


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
      set(r1 "foo:r1")
      set(r2 "foo:r2")
      set(r3 "foo:r3")
      set(r4 "foo:r4")                     # Set to same new value in all branches
      set(CACHE{c1} FORCE VALUE "foo:c1")
      set(CACHE{c2} FORCE VALUE "foo:c2")
      set(CACHE{c3} FORCE VALUE "foo:c3")
      set(CACHE{c4} FORCE VALUE "foo:c4")  # Set to same new value in all branches
      set(ENV{e1} "foo:e1")
      set(ENV{e2} "foo:e2")
      set(ENV{e3} "foo:e3")
      set(ENV{e4} "foo:e4")                # Set to same new value in all branches
      if(u1)
        set(r1 "foo:r1")                     # No change
        set(CACHE{c1} FORCE VALUE "foo:c1")  # No change
        set(ENV{e1} "foo:e1")                # No change
        set(r4 "bar:r4")
        set(CACHE{c4} FORCE VALUE "bar:c4")
        set(ENV{e4} "bar:e4")
      elseif(u2)
        set(r2 "foo:r2")                     # No change
        set(CACHE{c2} FORCE VALUE "foo:c2")  # No change
        set(ENV{e2} "foo:e2")                # No change
        set(r4 "bar:r4")
        set(CACHE{c4} FORCE VALUE "bar:c4")
        set(ENV{e4} "bar:e4")
      else()
        set(r3 "bar:r3")
        set(CACHE{c3} FORCE VALUE "bar:c2")
        set(ENV{e3} "bar:e3")
        set(r3 "foo:r3")                     # Original value restored
        set(CACHE{c3} FORCE VALUE "foo:c3")  # Original value restored
        set(ENV{e3} "foo:e3")                # Original value restored
        set(r4 "bar:r4")
        set(CACHE{c4} FORCE VALUE "bar:c4")
        set(ENV{e4} "bar:e4")
      endif()
      message("-${r1}-${r2}-${r3}-${r4}-")
      message("-$CACHE{c1}-$CACHE{c2}-$CACHE{c3}-$CACHE{c4}-")
      message("-$ENV{e1}-$ENV{e2}-$ENV{e3}-$ENV{e4}-")
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-foo:r1-foo:r2-foo:r3-bar:r4-",
        "-foo:c1-foo:c2-foo:c3-bar:c4-",
        "-foo:e1-foo:e2-foo:e3-bar:e4-",
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
      set(x "x")
      foreach(x IN LISTS l)
        message("1: -${x}-")
      endforeach()
      message("2: -${x}-")
      set(y "y")
      if(u)
        foreach(y IN LISTS l)
          message("3: -${y}-")
        endforeach()
        message("4: -${y}-")
      endif()
      message("5: -${y}-")
    """
    path = pathlib.Path("test.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        ("1: -Foo-", True),
        ("1: -Bar-", True),
        ("1: -Baz-", True),
        ("2: -x-",   True),
        ("3: -Foo-", False),
        ("3: -Bar-", False),
        ("3: -Baz-", False),
        ("4: -y-",   False),
        ("5: -y-",   True),
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(not message.occurrence_uncertainty, expected[1])


def test_CMakeProcess_Macro(context: _t.Context) -> None:
    expected_messages: typing.Any

    text = r"""
      macro(foo x)
        message("1: -${x}-")
        set(a "+${x}+")
        macro(bar y)
          message("2: -${y}-")
          set(b "+${y}+")
        endmacro()
        bar("*${x}*")
      endmacro()
      set(a "a")
      set(b "b")
      foo("foo")
      message("3: -${a}-${b}-")
      set(a "a")
      set(b "b")
      Foo("bar")                 # Mixed case in command name
      message("4: -${a}-${b}-")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        ("1: -foo-",           _tp.FilePos(path,  2, 2)),  # 1
        ("2: -*foo*-",         _tp.FilePos(path,  5, 4)),  # 2
        ("3: -+foo+-+*foo*+-", _tp.FilePos(path, 13, 0)),  # 3
        ("1: -bar-",           _tp.FilePos(path,  2, 2)),  # 4
        ("2: -*bar*-",         _tp.FilePos(path,  5, 4)),  # 5
        ("4: -+bar+-+*bar*+-", _tp.FilePos(path, 17, 0)),  # 6
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Multiple parameters
    text = r"""
      macro(foo x y z)
        macro(bar a b)
          message("-${a}-${b}-")
        endmacro()
        bar("${x}+${z}" "${y}")
      endmacro()
      foo("X" "Y" "Z")
      set(l "X" "Y" "Z")
      foo(${l})
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-X+Z-Y-",
        "-X+Z-Y-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Too few arguments
    text = r"""
      macro(foo x y)
      endmacro()
      foo()
      foo("x")
      foo("x" "y")
      unset(CACHE{l2})  # Prevent uncertainty from cache fallback
      set(l1 "x")
      set(l2)
      foo(${l1})
      foo(${l1} ${l2})
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        ("Too few arguments in invocation of macro foo()", _tp.TextPos(3, 4)),   # 1
        ("Definition of macro foo()",                      _tp.TextPos(1, 0)),   # 2
        ("Too few arguments in invocation of macro foo()", _tp.TextPos(4, 7)),   # 3
        ("Definition of macro foo()",                      _tp.TextPos(1, 0)),   # 4
        ("Too few arguments in invocation of macro foo()", _tp.TextPos(9, 9)),   # 5
        ("Definition of macro foo()",                      _tp.TextPos(1, 0)),   # 6
        ("Too few arguments in invocation of macro foo()", _tp.TextPos(10, 15)), # 7
        ("Definition of macro foo()",                      _tp.TextPos(1, 0)),   # 8
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Special parameter substitution
    text = r"""
      macro(foo _x _y)
        message("${_x}${_y}aa} -- ${_y}${_x}" [[ -- ${_x}${_y}]])
      endmacro()
      set(foo "bar")
      set(faa "boo")
      foo("oo}" "\${f")
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.message, "oo}boo -- bar -- ${_x}${_y}")
    context.check_equal(message.file_pos, _tp.FilePos(path, 2, 2))
    context.check_is_none(message.occurrence_uncertainty)

    # Special invocation parameters
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
    path = pathlib.Path("test-5.cmake")
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
    path = pathlib.Path("test-6.cmake")
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
    path = pathlib.Path("test-7.cmake")
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
      foo(${_3})
      foo(${_3} ${_4})
    """
    path = pathlib.Path("test-8.cmake")
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
        ("Uncertain number of arguments in invocation of macro foo()",         _tp.TextPos(16, 0)),  # 13
        ("Uncertain number of arguments in invocation of macro foo()",         _tp.TextPos(17, 0)),  # 14
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Reassignment of original command
    text = r"""
      macro(foo)
        message("Foo 1")
      endmacro()
      foo()
      _foo()
      macro(foo)
        message("Foo 2")
      endmacro()
      foo()
      _foo()
      __foo()
      macro(foo)
        message("Foo 3")
      endmacro()
      foo()
      _foo()
      __foo()
      macro(_foo)
        message("Foo 4")
      endmacro()
      foo()
      _foo()
      __foo()
    """
    path = pathlib.Path("test-9.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    expected_messages = [
        "Foo 1",  # 1
        "Foo 2",  # 2
        "Foo 1",  # 3
        "Foo 3",  # 4
        "Foo 2",  # 5
        "Foo 3",  # 6
        "Foo 4",  # 7
        "Foo 2",  # 8
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        ("Invocation failed due to uncertain definition of _foo()",  _tp.TextPos(5, 0)),   # 1
        ("Invocation failed due to uncertain definition of __foo()", _tp.TextPos(11, 0)),  # 2
        ("Invocation failed due to uncertain definition of __foo()", _tp.TextPos(17, 0)),  # 3
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Rejection of flow-control command names
    text = r"""
      macro(foreach)
      endmacro()
    """
    path = pathlib.Path("test-10.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        ("Failed to define macro foreach(): Built-in flow control commands cannot be overridden", _tp.TextPos(1, 6)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])


def test_CMakeProcess_Function(context: _t.Context) -> None:
    expected_messages: typing.Any
    expected:          typing.Any

    text = r"""
      function(foo x)
        set(a1 "a1")
        set(a2 "a2")
        function(bar y)
          message("1: -${y}-")
          set(a1 "aa1")
          set(a2 "aa2" PARENT_SCOPE)
        endfunction()
        bar("*${x}*")
        message("2: -${x}-${a1}-${a2}-")
        set(b1 "bb1")
        set(b2 "bb2" PARENT_SCOPE)
      endfunction()
      set(b1 "b1")
      set(b2 "b2")
      foo("foo")
      message("3: -${b1}-${b2}-")
      set(b1 "b1")
      set(b2 "b2")
      Foo("bar")                   # Mixed case in command name
      message("4: -${b1}-${b2}-")
    """
    path = pathlib.Path("test-1.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        ("1: -*foo*-",      _tp.FilePos(path,  5, 4)),  # 1
        ("2: -foo-a1-aa2-", _tp.FilePos(path, 10, 2)),  # 2
        ("3: -b1-bb2-",     _tp.FilePos(path, 17, 0)),  # 3
        ("1: -*bar*-",      _tp.FilePos(path,  5, 4)),  # 4
        ("2: -bar-a1-aa2-", _tp.FilePos(path, 10, 2)),  # 5
        ("4: -b1-bb2-",     _tp.FilePos(path, 21, 0)),  # 6
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Multiple parameters
    text = r"""
      function(foo x y z)
        function(bar a b)
          message("-${a}-${b}-")
        endfunction()
        bar("${x}+${z}" "${y}")
      endfunction()
      foo("X" "Y" "Z")
      set(l "X" "Y" "Z")
      foo(${l})
    """
    path = pathlib.Path("test-2.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "-X+Z-Y-",
        "-X+Z-Y-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Too few arguments
    text = r"""
      function(foo x y)
      endfunction()
      foo()
      foo("x")
      foo("x" "y")
      unset(CACHE{l2})  # Prevent uncertainty from cache fallback
      set(l1 "x")
      set(l2)
      foo(${l1})
      foo(${l1} ${l2})
    """
    path = pathlib.Path("test-3.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        ("Too few arguments in invocation of function foo()", _tp.TextPos(3, 4)),   # 1
        ("Definition of function foo()",                      _tp.TextPos(1, 0)),   # 2
        ("Too few arguments in invocation of function foo()", _tp.TextPos(4, 7)),   # 3
        ("Definition of function foo()",                      _tp.TextPos(1, 0)),   # 4
        ("Too few arguments in invocation of function foo()", _tp.TextPos(9, 9)),   # 5
        ("Definition of function foo()",                      _tp.TextPos(1, 0)),   # 6
        ("Too few arguments in invocation of function foo()", _tp.TextPos(10, 15)), # 7
        ("Definition of function foo()",                      _tp.TextPos(1, 0)),   # 8
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # No macro-like parameter substitution
    text = r"""
      function(foo x y)
        message("-${x}${y}-")
      endfunction()
      set(bar "bar")
      foo("\${b" "ar}")
    """
    path = pathlib.Path("test-4.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    context.check_equal(len(result.messages), 1)
    message = result.messages[0]
    context.check_equal(message.message, "-${bar}-")
    context.check_equal(message.file_pos, _tp.FilePos(path, 2, 2))
    context.check_is_none(message.occurrence_uncertainty)

    # Special invocation parameters
    text = r"""
      set(ARGV1 "v1")
      set(ARGV2 "v2")
      function(foo x)
        message("1: ${ARGC}-${ARGN}-${ARGV}-${ARGV0}-${ARGV1}-${ARGV2}")
      endfunction()
      foo("x")
      foo("x" "y")
      foo("x" "y" "z")
      foo("a;b" "c;d" "e;f")

      set(ARGC "c")
      set(ARGN "n")
      set(ARGV "v")
      set(ARGV0 "v0")
      function(bar)
        message("2: ${ARGC}-${ARGN}-${ARGV}-${ARGV0}")
      endfunction()
      bar()
      bar("x")
    """
    path = pathlib.Path("test-5.cmake")
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

    # Special introspection parameters
    text = r"""
      function(foo)
        message("1: -${CMAKE_CURRENT_FUNCTION}-")
        message("2: -${CMAKE_CURRENT_FUNCTION_LIST_LINE}-")
      endfunction()
      foo()
      function(Foo)
        message("3: -${CMAKE_CURRENT_FUNCTION}-")
        message("4: -${CMAKE_CURRENT_FUNCTION_LIST_DIR}-")
        message("5: -${CMAKE_CURRENT_FUNCTION_LIST_FILE}-")
        message("6: -${CMAKE_CURRENT_FUNCTION_LIST_LINE}-")
      endfunction()
      FOO()   # Invocation case should not affect CMAKE_CURRENT_FUNCTION
      _foo()
    """
    path = pathlib.Path("test-6.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check(success)
    expected_messages = [
        "1: -foo-",              # 1
        "2: -1-",                # 2
        "3: -Foo-",              # 3
        "4: -/root-",            # 4
        "5: -/root/%s-" % path,  # 5
        "6: -6-",                # 6
        "1: -foo-",              # 7
        "2: -1-",                # 8
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)

    # Argument uncertainty
    text = r"""
      function(foo a b)
        message("x${a}y")
        function(bar a)
          message("x${a}y")
          message("x${b}y")
        endfunction()
        bar("*")
        function(baz c)
          message("x${c}y")
        endfunction()
        baz("*")
        baz("${a}")
        baz("-${b}-")
      endfunction()
      foo("${_1}" "${_2}")
      foo(${_3})
      foo(${_3} ${_4})
    """
    path = pathlib.Path("test-7.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    expected_messages = [
        ("x*y", _tp.TextPos(4, 4)),
        ("x*y", _tp.TextPos(9, 4)),
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos.text_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "a"), _tp.TextPos(2, 12)),  #  1
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_1"), _tp.TextPos(15, 5)),  #  2
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "b"), _tp.TextPos(5, 14)),  #  3
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_2"), _tp.TextPos(15, 13)), #  4
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "c"), _tp.TextPos(9, 14)),  #  5
        (expansion_uncertainty_cause("baz", _cur.ParamType.REGULAR_VAR, "a"),  _tp.TextPos(12, 7)),  #  6
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_1"), _tp.TextPos(15, 5)),  #  7
        (invoke_uncertainty_error("message", _cur.ParamType.REGULAR_VAR, "c"), _tp.TextPos(9, 14)),  #  8
        (expansion_uncertainty_cause("baz", _cur.ParamType.REGULAR_VAR, "b"),  _tp.TextPos(13, 8)),  #  9
        (expansion_uncertainty_cause("foo", _cur.ParamType.REGULAR_VAR, "_2"), _tp.TextPos(15, 13)), # 10
        ("Uncertain number of arguments in invocation of function foo()",      _tp.TextPos(16, 0)),  # 11
        ("Uncertain number of arguments in invocation of function foo()",      _tp.TextPos(17, 0)),  # 12
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Reassignment of original command
    text = r"""
      function(foo)
        message("Foo 1")
      endfunction()
      foo()
      _foo()
      function(foo)
        message("Foo 2")
      endfunction()
      foo()
      _foo()
      __foo()
      function(foo)
        message("Foo 3")
      endfunction()
      foo()
      _foo()
      __foo()
      function(_foo)
        message("Foo 4")
      endfunction()
      foo()
      _foo()
      __foo()
    """
    path = pathlib.Path("test-8.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    expected_messages = [
        "Foo 1",  # 1
        "Foo 2",  # 2
        "Foo 1",  # 3
        "Foo 3",  # 4
        "Foo 2",  # 5
        "Foo 3",  # 6
        "Foo 4",  # 7
        "Foo 2",  # 8
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        ("Invocation failed due to uncertain definition of _foo()",  _tp.TextPos(5, 0)),   # 1
        ("Invocation failed due to uncertain definition of __foo()", _tp.TextPos(11, 0)),  # 2
        ("Invocation failed due to uncertain definition of __foo()", _tp.TextPos(17, 0)),  # 3
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos.text_pos, expected[1])

    # Rejection of flow-control command names
    text = r"""
      function(foreach)
      endfunction()
    """
    path = pathlib.Path("test-9.cmake")
    success, result = _process(_trim_cmake_text(text), path, context)
    context.check_not(success)
    context.check_equal(len(result.messages), 0)
    expected_errors = [
        ("Failed to define function foreach(): Built-in flow control commands cannot be overridden",
         _tp.TextPos(1, 9)),
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


def test_CMakeProcess_Include(context: _t.Context) -> None:
    foo_text = r"""
      message("1: -${x}-")
      set(x "X2")
      set(y "Y")
    """
    bar_text = r"""
      message("2: -${x}-${y}-")
      set(x "X3")
      set(y "Y2")
    """
    subdir_baz_text = r"""
      include(bar.cmake)
    """
    root_text = r"""
      set(x "X")
      include(foo.cmake)
      message("3: -${x}-${y}-")
      include(subdir/baz.cmake)
      message("4: -${x}-${y}-")
      include(qux.cmake)
    """
    root_path = pathlib.Path("test-1.cmake")
    def resolve(path: str) -> str:
        match path:
            case "src/foo.cmake":
                return _trim_cmake_text(foo_text)
            case "src/bar.cmake":
                return _trim_cmake_text(bar_text)
            case "src/subdir/baz.cmake":
                return _trim_cmake_text(subdir_baz_text)
        raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT))
    success, result = _process(_trim_cmake_text(root_text), root_path, context, resolve)
    context.check_not(success)
    expected_messages = [
        ("1: -X-",     _tp.FilePos(pathlib.Path("src/foo.cmake"), 1, 0)),  # 1
        ("3: -X2-Y-",  _tp.FilePos(root_path, 3, 0)),                      # 2
        ("2: -X2-Y-",  _tp.FilePos(pathlib.Path("src/bar.cmake"), 1, 0)),  # 3
        ("4: -X3-Y2-", _tp.FilePos(root_path, 5, 0)),                      # 4
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        ('Failed to include "qux.cmake" ("src/qux.cmake"): No such file or directory', _tp.FilePos(root_path, 6, 8)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos, expected[1])


def test_CMakeProcess_AddSubdirectory(context: _t.Context) -> None:
    foo_text = r"""
      message("1: -${x}-${y}-")
      set(x "X2")
      set(y "Y2" PARENT_SCOPE)
    """
    baz_bar_text = r"""
      message("2: -${x}-${y}-")
      set(x "X3")
      set(y "Y3" PARENT_SCOPE)
    """
    baz_text = r"""
      add_subdirectory(bar)
    """
    root_text = r"""
      set(x "X")
      set(y "Y")
      add_subdirectory(foo)
      message("3: -${x}-${y}-")
      add_subdirectory(baz)
      message("4: -${x}-${y}-")
      add_subdirectory(qux)
    """
    root_path = pathlib.Path("test-1.cmake")
    def resolve(path: str) -> str:
        match path:
            case "src/foo/CMakeLists.txt":
                return _trim_cmake_text(foo_text)
            case "src/baz/bar/CMakeLists.txt":
                return _trim_cmake_text(baz_bar_text)
            case "src/baz/CMakeLists.txt":
                return _trim_cmake_text(baz_text)
        raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT))
    success, result = _process(_trim_cmake_text(root_text), root_path, context, resolve)
    context.check_not(success)
    expected_messages = [
        ("1: -X-Y-",  _tp.FilePos(pathlib.Path("src/foo/CMakeLists.txt"), 1, 0)),     # 1
        ("3: -X-Y2-", _tp.FilePos(root_path, 4, 0)),                                  # 2
        ("2: -X-Y2-", _tp.FilePos(pathlib.Path("src/baz/bar/CMakeLists.txt"), 1, 0)), # 3
        ("4: -X-Y2-", _tp.FilePos(root_path, 6, 0)),                                  # 4
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected[0])
        subcontext.check_equal(message.file_pos, expected[1])
        subcontext.check_is_none(message.occurrence_uncertainty)
    expected_errors = [
        ('Failed to add subdirectory "qux" ("src/qux/CMakeLists.txt"): No such file or directory',
         _tp.FilePos(root_path, 7, 17)),
    ]
    context.check_equal(len(result.errors), len(expected_errors))
    for i, (error, expected) in enumerate(zip(result.errors, expected_errors)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(error.message, expected[0])
        subcontext.check_equal(error.file_pos, expected[1])


def test_CMakeProcess_DirectoryVariables(context: _t.Context) -> None:
    foo_bar_text = r"""
      message("1: -${CMAKE_CURRENT_SOURCE_DIR}-${CMAKE_CURRENT_BINARY_DIR}-")
    """
    foo_text = r"""
      add_subdirectory(bar)
      message("2: -${CMAKE_CURRENT_SOURCE_DIR}-${CMAKE_CURRENT_BINARY_DIR}-")
    """
    root_text = r"""
      add_subdirectory(foo)
      message("3: -${CMAKE_CURRENT_SOURCE_DIR}-${CMAKE_CURRENT_BINARY_DIR}-")
    """
    root_path = pathlib.Path("test-1.cmake")
    def resolve(path: str) -> str:
        match path:
            case "src/foo/CMakeLists.txt":
                return _trim_cmake_text(foo_text)
            case "src/foo/bar/CMakeLists.txt":
                return _trim_cmake_text(foo_bar_text)
        raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT))
    success, result = _process(_trim_cmake_text(root_text), root_path, context, resolve, set_binary_dir=True)
    context.check(success)
    expected_messages = [
        "1: -/root/src/foo/bar-/root/bin/foo/bar-",
        "2: -/root/src/foo-/root/bin/foo-",
        "3: -/root/src-/root/bin-",
    ]
    context.check_equal(len(result.messages), len(expected_messages))
    for i, (message, expected) in enumerate(zip(result.messages, expected_messages)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(message.message, expected)
        subcontext.check_is_none(message.occurrence_uncertainty)








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


def _process(cmake_text: str, cmake_path: pathlib.Path, context: _t.Context,
             subfile_resolver: _SubfileResolver | None = None, set_binary_dir: bool = False,
             lenient_mode: bool = False) -> tuple[bool, _Result]:
    source_dir = pathlib.Path("src")
    pos_resolver = _cp.PositionResolver()
    result = _Result()
    application = _Application(subfile_resolver, pos_resolver, result, context.logger)
    config = _cp.Config()
    if set_binary_dir:
        config.binary_dir = pathlib.Path("bin")
    if lenient_mode:
        config.lenient_mode = True
    config.define_breakpoint_command = True
    with io.StringIO(cmake_text) as file_:
        cmake_source = _cp.Source(file_, cmake_path)
        success = _cp.process(cmake_source, source_dir, application, pos_resolver, config)
        return success, result


class _Application(_cp.Application):
    def __init__(self, subfile_resolver: _SubfileResolver | None, pos_resolver: _cp.PositionResolver, result: _Result,
                 logger: _l.Logger) -> None:
        self._subfile_resolver = subfile_resolver
        self._abs_base_dir     = pathlib.Path("/root")
        self._pos_resolver     = pos_resolver
        self._result           = result
        self._logger           = logger

    @typing.override
    def open_subfile(self, path: pathlib.Path) -> typing.TextIO:
        assert self._subfile_resolver
        cmake_text = self._subfile_resolver(path.as_posix())
        return io.StringIO(cmake_text)

    @typing.override
    def resolve_path(self, path: pathlib.Path) -> pathlib.Path:
        return self._abs_base_dir / path

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


class _SubfileResolver(typing.Protocol):
    def __call__(self, path: str) -> str:
        ...


class _Result:
    def __init__(self) -> None:
        self.messages = list[_CMakeMessage]()
        self.warnings = list[_LogMessage]()
        self.errors   = list[_LogMessage]()


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
