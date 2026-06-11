from __future__ import annotations

import re


def list_split(string: str) -> list[str]:
    if not string:
        return []
    return [s.replace(r"\;", ";") for s in re.split(r"(?<!\\);", string)]


def list_join(list_: list[str]) -> str | None:
    if not list_:
        return None
    return ";".join(list_)
