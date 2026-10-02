from __future__ import annotations
from dataclasses import dataclass

import re


def list_split(string: str) -> list[str]:
    if not string:
        return []
    return [s.replace(r"\;", ";") for s in re.split(r"(?<!\\);", string)]


def list_join(list_: list[str]) -> str | None:
    if not list_:
        return None
    return ";".join(list_)


def parse_var_name(string: str) -> VarName:
    m = re.fullmatch(r"CACHE\{(.*)\}", string)
    if m:
        return CacheVarName(m.group(1))
    m = re.fullmatch(r"ENV\{(.*)\}", string)
    if m:
        return EnvVarName(m.group(1))
    return GeneralVarName(string)


type VarName = GeneralVarName | CacheVarName | EnvVarName

@dataclass(slots=True, frozen=True)
class VarNameBase:
    name: str

@dataclass(slots=True, frozen=True)
class GeneralVarName(VarNameBase):
    pass

@dataclass(slots=True, frozen=True)
class CacheVarName(VarNameBase):
    pass

@dataclass(slots=True, frozen=True)
class EnvVarName(VarNameBase):
    pass
