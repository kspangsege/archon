from __future__ import annotations
from typing import Protocol
from abc import ABC, abstractmethod
from dataclasses import dataclass

import archon.cmake.util as _cu
import archon.cmake.uncertainty_reason as _cur


class VariableResolver(Protocol):
    def __call__(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> Value:
        ...

class VariableState(ABC):
    @abstractmethod
    def get(self, resolution_type: _cu.ResolutionType, variable_name: str, pos: int) -> Value:
        ...

    @abstractmethod
    def set_(self, variable_name: str, value: str | None) -> None:
        ...

    @abstractmethod
    def taint(self, variable_name: str, reason: _cur.ValueUncertaintyReason) -> None:
        ...


type Value = CertainValue | UncertainValue

@dataclass(slots=True, frozen=True)
class CertainValue:
    string: str | None

@dataclass(slots=True, frozen=True)
class UncertainValue:
    reason: _cur.ValueUncertaintyReason | None
