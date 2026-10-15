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
import enum
import sys
import os


def is_ansi_term(output_stream: typing.TextIO) -> bool:
    if output_stream.isatty():
        return sys.platform != "win32" or "ANSICON" in os.environ
    return False


class AnsiTermColor(enum.Enum):
    BLACK   = 0
    RED     = 1
    GREEN   = 2
    YELLOW  = 3
    BLUE    = 4
    MAGENTA = 5
    CYAN    = 6
    WHITE   = 7


def ansi_term_set_color(color: AnsiTermColor) -> str:
    if color == AnsiTermColor.BLACK:
        return "\033[30m"
    if color == AnsiTermColor.RED:
        return "\033[31m"
    if color == AnsiTermColor.GREEN:
        return "\033[32m"
    if color == AnsiTermColor.YELLOW:
        return "\033[33m"
    if color == AnsiTermColor.BLUE:
        return "\033[34m"
    if color == AnsiTermColor.MAGENTA:
        return "\033[35m"
    if color == AnsiTermColor.CYAN:
        return "\033[36m"
    if color == AnsiTermColor.WHITE:
        return "\033[37m"
    assert False


def ansi_term_reset_color() -> str:
    return "\033[39m"
