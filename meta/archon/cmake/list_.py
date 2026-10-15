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

import re


def nonescaping_join(elements: list[str]) -> str | None:
    if not elements:
        return None
    return ";".join(elements)


def unescaping_split(string: str | None) -> list[str]:
    if not string:
        return []

    parts: list[str] = []
    elements: list[str] = []
    square_level = 0
    prev_end = 0

    for m in _SPLIT_REGEX.finditer(string):
        token = m.group(0)
        begin, end = m.span()

        if begin > prev_end:
            parts.append(string[prev_end:begin])
        prev_end = end

        match token:
            case "\\;":
                parts.append(";")
                continue
            case "[":
                square_level += 1
                parts.append("[")
                continue
            case "]":
                square_level -= 1
                parts.append("]")
                continue
            case ";":
                if square_level == 0:
                    element = "".join(parts)
                    elements.append(element)
                    parts.clear()
                else:
                    parts.append(";")
                continue
        assert False

    if prev_end < len(string):
        parts.append(string[prev_end:])

    element = "".join(parts)
    elements.append(element)
    return elements








_SPLIT_REGEX = re.compile(r"\\;|\[|\]|;")
