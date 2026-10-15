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
import dataclasses
import enum

import archon.base as _b
import archon.cmake.policy as _cpo


type ValueUncertaintyReason = ExpansionUncertaintyReason | AssignmentOccurrenceUncertaintyReason | \
    AssignmentPolicyUncertaintyReason

@dataclasses.dataclass(slots=True, frozen=True)
class ExpansionUncertaintyReason:
    command_name:             str
    param_type:               ParamType
    param_name:               str
    expansion_position:       Position
    value_uncertainty_reason: ValueUncertaintyReason | None

    def get_qual_param_ref(self) -> str:
        return get_qual_param_ref(self.param_type, self.param_name)


@dataclasses.dataclass(slots=True, frozen=True)
class AssignmentOccurrenceUncertaintyReason:
    command_name:                  str
    assignment_position:           Position
    occurrence_uncertainty_reason: ExpansionUncertaintyReason


@dataclasses.dataclass(slots=True, frozen=True)
class AssignmentPolicyUncertaintyReason:
    command_name:              str
    command_position:          Position
    policy:                    _cpo.Policy
    policy_uncertainty_reason: ValueUncertaintyReason


class ParamType(enum.Enum):
    REGULAR_VAR = 0
    CACHE_VAR   = 1
    ENV_VAR     = 2
    MACRO_PARAM = 3


@dataclasses.dataclass(slots=True, frozen=True)
class Position:
    file_index: int
    pos:        int


def get_qual_param_ref(param_type: ParamType, param_name: str) -> str:
    match param_type:
        case ParamType.REGULAR_VAR:
            return "regular variable %s" % _b.quote(param_name)
        case ParamType.CACHE_VAR:
            return "cache variable %s" % _b.quote(param_name)
        case ParamType.ENV_VAR:
            return "environment variable %s" % _b.quote(param_name)
        case ParamType.MACRO_PARAM:
            return "macro parameter %s" % _b.quote(param_name)
    typing.assert_never(param_type)
