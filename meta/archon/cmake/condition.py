from __future__ import annotations
from typing import Any, assert_never
from collections.abc import Iterable
from dataclasses import dataclass

import enum

import archon.text_pos as _tp
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.argument as _ca


def parse(arguments: Iterable[_ca.Argument], rparen_pos: int) -> Condition:
    return _parse(arguments, rparen_pos)


def evaluate(condition: Condition) -> Result:
    return _evaluate(condition)


class FatalParseError(Exception):
    def __init__(self, pos: int, message: str, *args: Any):
        self.pos     = pos
        self.message = message
        self.args    = args


type Condition = CertainCondition | UncertainCondition

type CertainCondition = FalseCondition | ArgumentCondition | UnopCondition | BinopCondition

@dataclass(slots=True, frozen=True)
class ConditionBase:
    pos: int

@dataclass(slots=True, frozen=True)
class FalseCondition(ConditionBase):
    pass

@dataclass(slots=True, frozen=True)
class ArgumentCondition(ConditionBase):
    string:                  _tp.PosMappedString
    was_quoted_or_bracketed: bool

@dataclass(slots=True, frozen=True)
class UnopCondition(ConditionBase):
    class Operator(enum.Enum):
        COMMAND      =  0
        POLICY       =  1
        TARGET       =  2
        TEST         =  3
        DEFINED      =  4
        EXISTS       =  5
        IS_READABLE  =  6
        IS_WRITABLE  =  7
        IS_DIRECTORY =  8
        IS_ABSOLUTE  =  9
        NOT          = 10
    operator: Operator
    operand:  Condition

@dataclass(slots=True, frozen=True)
class BinopCondition(ConditionBase):
    class Operator(enum.Enum):
        STREQUAL              =  0
        STRLESS               =  1
        STRGREATER            =  2
        STRLESS_EQUAL         =  3
        STRGREATER_EQUAL      =  4
        EQUAL                 =  5
        LESS                  =  6
        GREATER               =  7
        LESS_EQUAL            =  8
        GREATER_EQUAL         =  9
        VERSION_EQUAL         = 10
        VERSION_LESS          = 11
        VERSION_GREATER       = 12
        VERSION_LESS_EQUAL    = 13
        VERSION_GREATER_EQUAL = 14
        MATCHES               = 15
        IN_LIST               = 16
        IS_NEWER_THAN         = 17
        AND                   = 18
        OR                    = 19
    operator: Operator
    left:     Condition
    right:    Condition

@dataclass(slots=True, frozen=True)
class UncertainCondition(ConditionBase):
    reason: _cur.ExpansionUncertaintyReason


type Result = CertainResult | UncertainResult

type CertainResult = FalseResult | TrueResult

@dataclass(slots=True, frozen=True)
class FalseResult:
    def __invert__(self):
        return TrueResult()
    def __and__(self, other):
        return self
    def __or__(self, other):
        return other

@dataclass(slots=True, frozen=True)
class TrueResult:
    def __invert__(self):
        return FalseResult()
    def __and__(self, other):
        return other
    def __or__(self, other):
        return self

@dataclass(slots=True, frozen=True)
class UncertainResult:
    reason: _cur.ExpansionUncertaintyReason
    def __invert__(self):
        return self
    def __and__(self, other):
        if isinstance(other, FalseResult):
            return other
        return self
    def __or__(self, other):
        if isinstance(other, TrueResult):
            return other
        return self








def _parse(arguments: Iterable[_ca.Argument], rparen_pos: int) -> Condition:
    def parse(conditions: list[Condition], rparen_pos: int) -> Condition:
        if len(conditions) < 1:
            return FalseCondition(rparen_pos)

        operator: Any

        # Parse for parentheses
        begin_index: int
        begin_pos: int
        level = 0
        i = 0
        while i < len(conditions):
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                if cond.string.string == "(":
                    if level == 0:
                        begin_index = i
                        begin_pos = cond.pos
                    level += 1
                elif cond.string.string == ")":
                    if level == 0:
                        raise FatalParseError(cond.pos, "Unmatched right parenhesis")
                    level -= 1
                    if level == 0:
                        end_index = i + 1
                        subconditions = conditions[begin_index+1:end_index-1]
                        cond_2 = parse(subconditions, cond.pos)
                        conditions[begin_index:end_index] = [cond_2]
                        i = begin_index + 1
                        continue
            i += 1
        if level != 0:
            raise FatalParseError(begin_pos, "Unmatched left parenhesis")

        # Parse for unary operators
        i = 0
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _NONLOGICAL_UNARY_COND_OPER_MAP.get(cond.string.string)
                if operator is not None:
                    operand = conditions[i+1]
                    cond_2 = UnopCondition(cond.pos, operator, operand)
                    begin_index = i
                    end_index   = i + 2
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index + 1
                    continue
            i += 1


        # Parse for binary operators
        i = 1
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _NONLOGICAL_BINARY_COND_OPER_MAP.get(cond.string.string)
                if operator is not None:
                    left  = conditions[i-1]
                    right = conditions[i+1]
                    cond_2 = BinopCondition(cond.pos, operator, left, right)
                    begin_index = i - 1
                    end_index   = i + 2
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index + 1
                    continue
            i += 1

        # Parse for NOT
        i = len(conditions) - 1
        while i > 0:
            cond = conditions[i-1]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _LOGICAL_UNARY_COND_OPER_MAP.get(cond.string.string)
                if operator is not None:
                    operand = conditions[i]
                    cond_2 = UnopCondition(cond.pos, operator, operand)
                    begin_index = i - 1
                    end_index   = i + 1
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index
                    continue
            i -= 1


        # Parse for AND and OR
        i = 1
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and not cond.was_quoted_or_bracketed:
                operator = _LOGICAL_BINARY_COND_OPER_MAP.get(cond.string.string)
                if operator is not None:
                    left  = conditions[i-1]
                    right = conditions[i+1]
                    cond_2 = BinopCondition(cond.pos, operator, left, right)
                    begin_index = i - 1
                    end_index   = i + 2
                    conditions[begin_index:end_index] = [cond_2]
                    i = begin_index + 1
                    continue
            i += 1

        if len(conditions) > 1:
            # FIXME: Find a way to format the currect list of conditions and insert them
            # into the message        
            raise FatalParseError(conditions[0].pos, "Unreducable condition")

        return conditions[0]

    conditions = list[Condition]()
    for arg in arguments:
        if isinstance(arg, _ca.CertainArgument):
            conditions.append(ArgumentCondition(arg.pos, arg.string, arg.was_quoted_or_bracketed))
            continue
        if isinstance(arg, _ca.UncertainArgument):
            if not arg.was_quoted_or_bracketed:
                return UncertainCondition(arg.pos, arg.reason)
            conditions.append(UncertainCondition(arg.pos, arg.reason))
            continue
        assert_never(arg)
    return parse(conditions, rparen_pos)


_NONLOGICAL_UNARY_COND_OPER_MAP = {
    "COMMAND":      UnopCondition.Operator.COMMAND,
    "POLICY":       UnopCondition.Operator.POLICY,
    "TARGET":       UnopCondition.Operator.TARGET,
    "TEST":         UnopCondition.Operator.TEST,
    "DEFINED":      UnopCondition.Operator.DEFINED,
    "EXISTS":       UnopCondition.Operator.EXISTS,
    "IS_READABLE":  UnopCondition.Operator.IS_READABLE,
    "IS_WRITABLE":  UnopCondition.Operator.IS_WRITABLE,
    "IS_DIRECTORY": UnopCondition.Operator.IS_DIRECTORY,
    "IS_ABSOLUTE":  UnopCondition.Operator.IS_ABSOLUTE,
}

_NONLOGICAL_BINARY_COND_OPER_MAP = {
    "STREQUAL":              BinopCondition.Operator.STREQUAL,
    "STRLESS":               BinopCondition.Operator.STRLESS,
    "STRGREATER":            BinopCondition.Operator.STRGREATER,
    "STRLESS_EQUAL":         BinopCondition.Operator.STRLESS_EQUAL,
    "STRGREATER_EQUAL":      BinopCondition.Operator.STRGREATER_EQUAL,
    "EQUAL":                 BinopCondition.Operator.EQUAL,
    "LESS":                  BinopCondition.Operator.LESS,
    "GREATER":               BinopCondition.Operator.GREATER,
    "LESS_EQUAL":            BinopCondition.Operator.LESS_EQUAL,
    "GREATER_EQUAL":         BinopCondition.Operator.GREATER_EQUAL,
    "VERSION_EQUAL":         BinopCondition.Operator.VERSION_EQUAL,
    "VERSION_LESS":          BinopCondition.Operator.VERSION_LESS,
    "VERSION_GREATER":       BinopCondition.Operator.VERSION_GREATER,
    "VERSION_LESS_EQUAL":    BinopCondition.Operator.VERSION_LESS_EQUAL,
    "VERSION_GREATER_EQUAL": BinopCondition.Operator.VERSION_GREATER_EQUAL,
    "MATCHES":               BinopCondition.Operator.MATCHES,
    "IN_LIST":               BinopCondition.Operator.IN_LIST,
    "IS_NEWER_THAN":         BinopCondition.Operator.IS_NEWER_THAN,
}

_LOGICAL_UNARY_COND_OPER_MAP = {
    "NOT": UnopCondition.Operator.NOT,
}

_LOGICAL_BINARY_COND_OPER_MAP = {
    "AND": BinopCondition.Operator.AND,
    "OR":  BinopCondition.Operator.OR,
}


def _evaluate(condition: Condition) -> Result:
    assert False        
