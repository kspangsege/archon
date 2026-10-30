# This file is part of the Archon project, a suite of C++ libraries.
#
# Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

from __future__ import annotations

import typing
import dataclasses
import collections.abc as _cabc
import bisect
import re

import archon.base as _b


# Format a Python function invocation by translating it into an element sequence and passing
# it to `format_elements()`.
#
# Each argument in `args` is either a positional value (`Value`) or a keyword argument
# (`KeyValue(key, val)` formatted as `key=val`). Values of type `None`, `bool`, and `int`
# are formatted as standard Python literals, `str` values are formatted as splittable string
# literals (`String`), and `Atom` values are inserted verbatim as unbreakable code
# fragments.
#
def format_call(func_name: str, args: _cabc.Sequence[Argument], max_width: int = 79) -> str:
    return _format_call(func_name, args, max_width=max_width)


# Format a line of code by breaking it as necessary at available break points. Requires
# balanced parentheses.
#
# If a line exceeds `max_width` and has at least one available break point, it is broken at
# the last break point that keeps the line within `max_width` (or at the first available
# break point if none keeps it within `max_width`). If the break point is inside
# parentheses, the next line is indented to the horizontal position immediately following
# the innermost unclosed opening parenthesis.
#
# A line has an available break point after a comma provided that something follows the
# comma on the line. A line also has a break point after a whitespace character, S, inside a
# splittable string (`String`) provided that there is a non-whitespace character that
# precedes S and provided that S is followed immediately by a non-whitespace character.
#
def format_elements(elements: _cabc.Sequence[Element], max_width: int = 79) -> str:
    return _format_elements(elements, max_width)


type Argument = Value | KeyValue

type Value = None | bool | int | str | Atom

type Element = Atom | String | LParen | RParen | Comma


@dataclasses.dataclass(slots=True, frozen=True)
class KeyValue:
    key: str
    val: Value

# Atomic code fragment: identifiers, numbers, or short expressions
@dataclasses.dataclass(slots=True, frozen=True)
class Atom:
    text: str

# Splittable string literal, specified in unquoted and unescaped form
@dataclasses.dataclass(slots=True, frozen=True)
class String:
    value: str

@dataclasses.dataclass(slots=True, frozen=True)
class LParen:
    pass

@dataclasses.dataclass(slots=True, frozen=True)
class RParen:
    pass

@dataclasses.dataclass(slots=True, frozen=True)
class Comma:
    pass








def _format_call(func_name: str, args: _cabc.Sequence[Argument], max_width: int) -> str:
    elements: list[Element] = [Atom(func_name), LParen()]
    def add(arg: Argument) -> None:
        match arg:
            case None:
                elements.append(Atom("None"))
            case int():
                elements.append(Atom(str(arg)))
            case str():
                elements.append(String(arg))
            case Atom():
                elements.append(arg)
            case KeyValue(key, val):
                elements.append(Atom("%s=" % key))
                add(val)
            case _:
                typing.assert_never(arg)

    first = True
    for arg in args:
        if not first:
            elements.append(Comma())
        first = False
        add(arg)
    elements.append(RParen())
    return _format_elements(elements, max_width=max_width)


def _format_elements(elements: _cabc.Sequence[Element], max_width: int) -> str:
    if not elements:
        return ""

    lines: list[str] = []
    indent_stack = [0]
    line_elems: list[_CookedElement] = []
    line_width = 0
    breakable = False

    def append(elem: _CookedElement) -> None:
        nonlocal line_width, breakable
        if line_elems and isinstance(line_elems[-1], Comma):
            line_width += 1
            breakable = True
        line_elems.append(elem)
        line_width += _elem_width(elem)
        if isinstance(elem, _CookedString):
            breakable = True

    for raw_elem in elements:
        append(_cook_element(raw_elem))
        while line_width > max_width and breakable:
            emitted, remaining = _split_line(line_elems, indent_stack[-1], max_width)
            lines.append(_render_and_update_indent(emitted, indent_stack))
            line_elems.clear()
            line_width = indent_stack[-1]
            breakable = False
            for elem in remaining:
                append(elem)

    if line_elems:
        lines.append(_render_and_update_indent(line_elems, indent_stack))
    return "\n".join(lines)


def _cook_element(elem: Element) -> _CookedElement:
    match elem:
        case String():
            return _cook_string(elem.value)
        case _:
            return elem


def _cook_string(string: str) -> _CookedString | Atom:
    fragments = _STRING_FRAGMENT_REGEX.findall(string)
    if len(fragments) <= 1:
        return Atom(_b.quote(string))
    parts = ['"']
    breakpoints: list[int] = []
    pos = 1
    for i, frag in enumerate(fragments):
        if i > 0:
            breakpoints.append(pos)
        part = _b.quote(frag)[1:-1]
        parts.append(part)
        pos += len(part)
    parts.append('"')
    return _CookedString("".join(parts), tuple(breakpoints))


_STRING_FRAGMENT_REGEX = re.compile(r"\s*\S+\s*")


def _split_line(line_elems: list[_CookedElement], indent: int,
                max_width: int) -> tuple[list[_CookedElement], list[_CookedElement]]:
    prefix_sizes: list[int] = []
    pos = indent
    for i, elem in enumerate(line_elems):
        if i > 0 and isinstance(line_elems[i - 1], Comma):
            pos += 1
        prefix_sizes.append(pos)
        pos += _elem_width(elem)

    # Search backward for the rightmost break point that keeps the line within `max_width`
    for i in reversed(range(len(line_elems))):
        elem = line_elems[i]
        size = prefix_sizes[i]
        match elem:
            case Comma():
                j = i + 1
                if j < len(line_elems) and size + 1 <= max_width:
                    return line_elems[:j], line_elems[j:]
            case _CookedString() as cs:
                if size < max_width:
                    k = bisect.bisect_right(cs.breakpoints, max_width - size - 1) - 1
                    if k >= 0:
                        prefix, rest = _split_cooked_string(cs, k)
                        j = i + 1
                        return [*line_elems[:i], prefix], [rest, *line_elems[j:]]

    # Fallback: break at the earliest available break point
    for i, elem in enumerate(line_elems):
        match elem:
            case Comma():
                j = i + 1
                if j < len(line_elems):
                    return line_elems[:j], line_elems[j:]
            case _CookedString() as cs:
                prefix, rest = _split_cooked_string(cs, 0)
                j = i + 1
                return [*line_elems[:i], prefix], [rest, *line_elems[j:]]

    assert False  # Function must be called only when the line can be broken


def _elem_width(elem: _CookedElement) -> int:
    match elem:
        case LParen() | RParen() | Comma():
            return 1
        case Atom(text) | _CookedString(text, _):
            return len(text)
        case _:
            typing.assert_never(elem)


def _split_cooked_string(string: _CookedString, i: int) -> tuple[Atom, _CookedString | Atom]:
    b = string.breakpoints[i]
    prefix = Atom(string.string[:b] + '"')
    rest = '"' + string.string[b:]
    if i + 1 == len(string.breakpoints):
        return prefix, Atom(rest)
    breakpoints = tuple(c - b + 1 for c in string.breakpoints[i+1:])
    return prefix, _CookedString(rest, breakpoints)


def _render_and_update_indent(line_elems: list[_CookedElement], indent_stack: list[int]) -> str:
    pos = indent_stack[-1]
    parts = [" " * pos]
    for i, elem in enumerate(line_elems):
        if i > 0 and isinstance(line_elems[i - 1], Comma):
            parts.append(" ")
            pos += 1
        match elem:
            case LParen():
                parts.append("(")
                pos += 1
                indent_stack.append(pos)
            case RParen():
                parts.append(")")
                pos += 1
                assert len(indent_stack) > 1
                indent_stack.pop()
            case Comma():
                parts.append(",")
                pos += 1
            case Atom(text) | _CookedString(text, _):
                parts.append(text)
                pos += len(text)
            case _:
                typing.assert_never(elem)
    return "".join(parts)


type _CookedElement = Atom | _CookedString | LParen | RParen | Comma

@dataclasses.dataclass(slots=True, frozen=True)
class _CookedString:
    string:      str  # Escaped and quoted
    breakpoints: tuple[int, ...]  # Never empty
