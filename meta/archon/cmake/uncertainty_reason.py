from __future__ import annotations

import typing
import dataclasses
import enum

import archon.base as _b


type ValueUncertaintyReason = ExpansionUncertaintyReason | AssignmentOccurrenceUncertaintyReason

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
