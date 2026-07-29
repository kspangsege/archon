from __future__ import annotations

import dataclasses
import collections
import enum

import archon.cmake.version as _cve


# Toggleable policies
class Policy(enum.Enum):
    CMP0180 = enum.auto()  # project() normal variable shadowing


def get_definitions() -> collections.abc.Iterator[Definition]:
    return iter(_definitions)


def get_definition(policy: Policy) -> Definition:
    return _definitions_by_policy[policy]


# A policy is togglable if `toggleable` is not `None`. If `toggleable` is `None`, toggling
# is not yet supported for that policy. If the negotiated policy version is greater than or
# equal to `force_version`, the policy must be considered "forced" to ON even if it is
# otherwise togglable.
@dataclasses.dataclass(slots=True, frozen=True)
class Definition:
    name:             str
    description:      str
    intro_version:    _cve.Version
    force_version:    _cve.Version | None
    suppress_warning: bool
    toggleable:       Policy | None








_definitions = list[Definition]()
_definitions_by_name = dict[str, Definition]()
_definitions_by_policy = dict[Policy, Definition]()


def _define_policy(name: str, description: str, intro_version: _cve.Version, force_version: _cve.Version | None = None,
                   suppress_warning: bool = False, toggleable: Policy | None = None) -> None:
    assert name not in _definitions_by_name
    assert not toggleable or toggleable not in _definitions_by_policy
    definition = Definition(name, description, intro_version, force_version, suppress_warning, toggleable)
    _definitions.append(definition)
    _definitions_by_name[name] = definition
    if toggleable:
        _definitions_by_policy[toggleable] = definition


_define_policy("CMP0180", "project() normal variable shadowing", _cve.Version(3, 31, 0), suppress_warning=True,
               toggleable=Policy.CMP0180)


assert _definitions_by_policy.keys() == set(Policy)
