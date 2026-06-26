from __future__ import annotations

import dataclasses
import enum
import pathlib
import json


def as_ord(val: int) -> str:
    match val:
        case 1:
            return "1st"
        case 2:
            return "2nd"
        case 3:
            return "3rd"
    return "%sth" % val


def quote(string: str) -> str:
    return json.dumps(string)


def resolve_self_rel_path(argv0: str, rel_path: str) -> pathlib.Path:
    self_path = pathlib.Path(argv0)
    assert self_path.exists()
    path = self_path.parent / rel_path
    return path.resolve().relative_to(pathlib.Path.cwd())


@dataclasses.dataclass(slots=True)
class Wrap[T]:
    value: T
