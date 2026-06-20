from __future__ import annotations
from dataclasses import dataclass


type ValueUncertaintyReason = ExpansionUncertaintyReason | AssignmentOccurrenceUncertaintyReason

@dataclass(slots=True, frozen=True)
class ExpansionUncertaintyReason:
    command_name:             str
    variable_name:            str
    expansion_position:       Position
    value_uncertainty_reason: ValueUncertaintyReason | None

@dataclass(slots=True, frozen=True)
class AssignmentOccurrenceUncertaintyReason:
    command_name:                  str
    assignment_position:           Position
    occurrence_uncertainty_reason: ExpansionUncertaintyReason


@dataclass(slots=True, frozen=True)
class Position:
    file_index: int
    pos:        int
