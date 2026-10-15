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

import typing
import abc
import dataclasses
import enum

import archon.cmake.util as _cu
import archon.cmake.uncertainty_reason as _cur


class VariableResolver(typing.Protocol):
    def __call__(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> Value:
        ...

class VariableState(abc.ABC):
    @abc.abstractmethod
    def get(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> tuple[Value, VariableType]:
        ...

    @abc.abstractmethod
    def set_(self, variable_name: str, value: str | None) -> None:
        ...

    @abc.abstractmethod
    def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
        ...


class VariableType(enum.Enum):
    REGULAR = 0
    CACHE   = 1
    ENV     = 2


def variable_to_param_type(variable_type: VariableType) -> _cur.ParamType:
    match variable_type:
        case VariableType.REGULAR:
            return _cur.ParamType.REGULAR_VAR
        case VariableType.CACHE:
            return _cur.ParamType.CACHE_VAR
        case VariableType.ENV:
            return _cur.ParamType.ENV_VAR
    typing.assert_never(variable_type)


type Value = CertainValue | UncertainValue

@dataclasses.dataclass(slots=True, frozen=True)
class CertainValue:
    string: str | None

@dataclasses.dataclass(slots=True, frozen=True)
class UncertainValue:
    reason: _cur.ValueUncertaintyReason | None
