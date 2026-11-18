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

import dataclasses
import enum
import re


def parse_index_arg(arg: str) -> int | None:
    # CMake fails an index argument if the integer value does not fit inside a native `long`
    # of the platform. This behavior is difficult to emulate in Python, so we ignore it.
    if _INDEX_ARG_REGEX.fullmatch(arg):
        return int(arg)
    return None


def parse_variable_reference(string: str) -> VariableReference:
    m = re.fullmatch(r"CACHE\{(.*)\}", string)
    if m:
        return VariableReference(ResolutionType.CACHE, m.group(1))
    m = re.fullmatch(r"ENV\{(.*)\}", string)
    if m:
        return VariableReference(ResolutionType.ENV, m.group(1))
    return VariableReference(ResolutionType.GENERAL, string)


@dataclasses.dataclass(slots=True, frozen=True)
class VariableReference:
    resolution_type: ResolutionType
    variable_name:   str


class ResolutionType(enum.Enum):
    GENERAL = 0
    CACHE   = 1
    ENV     = 2








# CMake uses native strtol() which silently skips leading whitespace. This quirky behavior
# is replicated here.
_INDEX_ARG_REGEX = re.compile(r"\s*[-+]?\d+", re.ASCII)
