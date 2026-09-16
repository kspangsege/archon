from __future__ import annotations

import re


def nonescaping_join(elements: list[str]) -> str | None:
    if not elements:
        return None
    return ";".join(elements)


def unescaping_split(string: str | None) -> list[str]:
    if not string:
        return []

    parts = list[str]()
    elements = list[str]()
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
