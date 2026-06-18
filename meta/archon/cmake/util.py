from __future__ import annotations
from dataclasses import dataclass

import enum
import re


def parse_variable_reference(string: str) -> VariableReference:
    m = re.fullmatch(r"CACHE\{(.*)\}", string)
    if m:
        return VariableReference(ResolutionType.CACHE, m.group(1))
    m = re.fullmatch(r"ENV\{(.*)\}", string)
    if m:
        return VariableReference(ResolutionType.ENV, m.group(1))
    return VariableReference(ResolutionType.GENERAL, string)


@dataclass(slots=True, frozen=True)
class VariableReference:
    resolution_type: ResolutionType
    variable_name:   str


class ResolutionType(enum.Enum):
    GENERAL = 0
    CACHE   = 1
    ENV     = 2
