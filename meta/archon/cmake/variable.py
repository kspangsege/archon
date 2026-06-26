from __future__ import annotations

import typing
import abc
import dataclasses

import archon.cmake.util as _cu
import archon.cmake.uncertainty_reason as _cur


class VariableResolver(typing.Protocol):
    def __call__(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> Value:
        ...

class VariableState(abc.ABC):
    @abc.abstractmethod
    def get(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> Value:
        ...

    @abc.abstractmethod
    def set_(self, variable_name: str, value: str | None) -> None:
        ...

    @abc.abstractmethod
    def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
        ...


type Value = CertainValue | UncertainValue

@dataclasses.dataclass(slots=True, frozen=True)
class CertainValue:
    string: str | None

@dataclasses.dataclass(slots=True, frozen=True)
class UncertainValue:
    reason: _cur.ValueUncertaintyReason | None
