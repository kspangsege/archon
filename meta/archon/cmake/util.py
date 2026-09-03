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
