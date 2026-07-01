from __future__ import annotations

import dataclasses
import enum
import re


def nonescaping_list_join(elements: list[str]) -> str | None:
    if not elements:
        return None
    return ";".join(elements)


def unescaping_list_split(string: str | None) -> list[str]:
    if not string:
        return []
    return [s.replace(r"\;", ";") for s in re.split(r"(?<!\\);", string)]


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
