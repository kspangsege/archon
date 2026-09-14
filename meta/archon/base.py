from __future__ import annotations

import dataclasses
import enum
import collections
import pathlib


def as_ord(val: int) -> str:
    match val:
        case 1:
            return "1st"
        case 2:
            return "2nd"
        case 3:
            return "3rd"
    return "%sth" % val


def as_num_of(val: int, spec: NumOfSpec) -> str:
    return "%s %s" % (val, spec.singular_form if val == 1 else spec.plural_form)


@dataclasses.dataclass(slots=True)
class NumOfSpec:
    singular_form: str
    plural_form:   str


def chomp(string: str) -> str:
    return string.removesuffix("\n")


def quote(string: str) -> str:
    return "\"%s\"" % "".join(_quote_replace(ch) for ch in string)


def clamped_quote(string: str, max_size: int) -> str:
    ellipsis = "..."
    min_max_size = 2 + len(ellipsis)
    if max_size < min_max_size:
        max_size = min_max_size
    limit_2 = max_size - 2
    limit_1 = limit_2 - len(ellipsis)
    string_2 = ""
    breach_pos = None
    for ch in string:
        repl = _quote_replace(ch)
        if breach_pos is None:
            if len(repl) <= limit_1 - len(string_2):
                string_2 += repl
                continue
            breach_pos = len(string_2)
        if len(repl) <= limit_2 - len(string_2):
            string_2 += repl
            continue
        return "\"%s...\"" % string_2[:breach_pos]
    return "\"%s\"" % string_2


def resolve_self_rel_path(argv0: str, rel_path: str) -> pathlib.Path:
    self_path = pathlib.Path(argv0)
    assert self_path.exists()
    path = self_path.parent / rel_path
    return path.resolve().relative_to(pathlib.Path.cwd())


@dataclasses.dataclass(slots=True)
class Wrap[T]:
    value: T


type Predicate[T] = collections.abc.Callable[[T], bool]








def _quote_replace(ch: str) -> str:
    repl = _SIMPLE_ESCAPES.get(ch)
    if repl is not None:
        return repl
    if ch.isprintable():
        return ch
    val = ord(ch)
    if val <= 0xFFFF:
        return "\\u%04X" % val
    assert val <= 0xFFFFFFFF
    return "\\U%08X" % val


_SIMPLE_ESCAPES = {
    "\a": "\\a",
    "\b": "\\b",
    "\t": "\\t",
    "\n": "\\n",
    "\v": "\\v",
    "\f": "\\f",
    "\r": "\\r",
    "\"": "\\\"",
    "\\": "\\\\",
}
