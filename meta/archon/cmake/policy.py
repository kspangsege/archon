# This file is part of the Archon project, a suite of C++ libraries.
#
# Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

from __future__ import annotations

import dataclasses
import collections
import enum

import archon.cmake.version as _cve


# Toggleable policies
class Policy(enum.Enum):
    CMP0180 = enum.auto()  # project() normal variable shadowing


def get_name(policy: Policy) -> str:
    definition = get_definition(policy)
    return definition.name


def get_definition(policy: Policy) -> Definition:
    return _definitions_by_policy[policy]


def get_definitions() -> collections.abc.Iterator[Definition]:
    return iter(_definitions)


# A policy is togglable if `toggleable` is not `None`. If `toggleable` is `None`, toggling
# is not yet supported for that policy. If the negotiated policy version is greater than or
# equal to `force_version`, the policy must be considered "forced" to ON even if it is
# otherwise togglable.
@dataclasses.dataclass(slots=True, frozen=True)
class Definition:
    name:          str
    description:   str
    intro_version: _cve.Version
    force_version: _cve.Version | None
    toggleable:    Policy | None








_definitions: list[Definition] = []
_definitions_by_name: dict[str, Definition] = {}
_definitions_by_policy: dict[Policy, Definition] = {}


def _define_policy(name: str, description: str, intro_version: _cve.Version, force_version: _cve.Version | None = None,
                   toggleable: Policy | None = None) -> None:
    assert name not in _definitions_by_name
    assert not toggleable or toggleable not in _definitions_by_policy
    definition = Definition(name, description, intro_version, force_version, toggleable)
    _definitions.append(definition)
    _definitions_by_name[name] = definition
    if toggleable:
        _definitions_by_policy[toggleable] = definition


_define_policy("CMP0180", "project() normal variable shadowing", _cve.Version(3, 31, 0), toggleable=Policy.CMP0180)


assert _definitions_by_policy.keys() == set(Policy)
