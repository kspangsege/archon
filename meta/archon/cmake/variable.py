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
