from __future__ import annotations

import typing


class Version(typing.NamedTuple):
    major: int
    minor: int = 0
    patch: int = 0

    def __str__(self) -> str:
        return "%s.%s.%s" % (self.major, self.minor, self.patch)


def parse(string: str) -> Version:
    parts = string.split(".")
    if not (0 < len(parts) <= 3):
        raise ValueError
    components = []
    for part in parts:
        if not part.isdigit():
            raise ValueError
        components.append(int(part))
    return Version(*components)
