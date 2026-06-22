from __future__ import annotations
from typing import Any, assert_never
from collections.abc import Iterable
from dataclasses import dataclass

import enum
import re

import archon.base as _b
import archon.text_pos as _tp
import archon.cmake.util as _cu
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.variable as _cv
import archon.cmake.argument as _ca
import archon.cmake.regex as _cr


def parse(arguments: Iterable[_ca.Argument], rparen_pos: int) -> Condition:
    return _parse(arguments, rparen_pos)


def evaluate(condition: Condition, command_name: str, file_index: int, variable_state: _cv.VariableState) -> Result:
    return _evaluate(condition, command_name, file_index, variable_state)


class FatalParseError(Exception):
    def __init__(self, pos: int, message: str, *args: Any):
        self.pos     = pos
        self.message = message
        self.args    = args


class FatalEvalError(Exception):
    def __init__(self, pos: int, message: str, *args: Any):
        self.pos     = pos
        self.message = message
        self.args    = args


type Condition = FalseCondition | UnopCondition | BinopCondition | ArgumentCondition | UncertainCondition

@dataclass(slots=True, frozen=True)
class ConditionBase:
    pos: int

@dataclass(slots=True, frozen=True)
class FalseCondition(ConditionBase):
    pass

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
class ArgumentCondition(ConditionBase):
    string:     _tp.PosMappedString
    was_bare:   bool  # Neither quoted nor bracketed
    is_derived: bool

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
            if isinstance(cond, ArgumentCondition) and cond.was_bare:
                if cond.string.string == "(":
                    if level == 0:
                        begin_index = i
                        begin_pos = cond.pos
                    level += 1
                elif cond.string.string == ")":
                    if level == 0:
                        raise FatalParseError(cond.pos, "Unmatched right parenthesis")
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
            raise FatalParseError(begin_pos, "Unmatched left parenthesis")

        # Parse for unary operators
        i = 0
        while i < len(conditions) - 1:
            cond = conditions[i]
            if isinstance(cond, ArgumentCondition) and cond.was_bare:
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
            if isinstance(cond, ArgumentCondition) and cond.was_bare:
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
            if isinstance(cond, ArgumentCondition) and cond.was_bare:
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
            if isinstance(cond, ArgumentCondition) and cond.was_bare:
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
            def format_(cond: Condition):
                match cond:
                    case FalseCondition():
                        return "()"
                    case UnopCondition():
                        return "(%s operation)" % cond.operator.name
                    case BinopCondition():
                        return "(%s operation)" % cond.operator.name
                    case ArgumentCondition():
                        # Bare parentheses are not possible here
                        if cond.was_bare and re.fullmatch(r"[0-9A-Z_a-z]+", cond.string.string):
                            return cond.string.string
                        return _b.quote(cond.string.string)
                    case UncertainCondition():
                        return "(uncertain argument)"
                assert_never(cond)
            prefix = conditions[:3]
            string = " ".join(format_(c) for c in prefix)
            if len(conditions) > len(prefix):
                string += " ..."
            raise FatalParseError(conditions[0].pos, "Irreducible argument sequence: %s", string)

        return conditions[0]

    conditions = list[Condition]()
    for arg in arguments:
        if isinstance(arg, _ca.CertainArgument):
            conditions.append(ArgumentCondition(arg.pos, arg.string, arg.was_bare, arg.is_derived))
            continue
        if isinstance(arg, _ca.UncertainArgument):
            if arg.was_bare:
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




def _evaluate(cond: Condition, command_name: str, file_index: int, variable_state: _cv.VariableState) -> Result:
    def eval_as_bool(cond: Condition) -> Result:
        if isinstance(cond, FalseCondition):
            return FalseResult()
        if isinstance(cond, UnopCondition):
            match cond.operator:
                case UnopCondition.Operator.COMMAND:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator COMMAND")
                case UnopCondition.Operator.POLICY:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator POLICY")
                case UnopCondition.Operator.TARGET:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator TARGET")
                case UnopCondition.Operator.TEST:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator TEST")
                case UnopCondition.Operator.DEFINED:
                    return eval_defined(cond.operand)
                case UnopCondition.Operator.EXISTS:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator EXISTS")
                case UnopCondition.Operator.IS_READABLE:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator IS_READABLE")
                case UnopCondition.Operator.IS_WRITABLE:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator IS_WRITABLE")
                case UnopCondition.Operator.IS_DIRECTORY:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator IS_DIRECTORY")
                case UnopCondition.Operator.IS_ABSOLUTE:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator IS_ABSOLUTE")
                case UnopCondition.Operator.NOT:
                    return eval_not(cond.operand)
            assert_never(cond.operator)
        if isinstance(cond, BinopCondition):
            match cond.operator:
                case BinopCondition.Operator.STREQUAL:
                    return eval_strequal(cond.left, cond.right)
                case BinopCondition.Operator.STRLESS:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator STRLESS")
                case BinopCondition.Operator.STRGREATER:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator STRGREATER")
                case BinopCondition.Operator.STRLESS_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator STRLESS_EQUAL")
                case BinopCondition.Operator.STRGREATER_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator STRGREATER_EQUAL")
                case BinopCondition.Operator.EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator EQUAL")
                case BinopCondition.Operator.LESS:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator LESS")
                case BinopCondition.Operator.GREATER:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator GREATER")
                case BinopCondition.Operator.LESS_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator LESS_EQUAL")
                case BinopCondition.Operator.GREATER_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator GREATER_EQUAL")
                case BinopCondition.Operator.VERSION_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator VERSION_EQUAL")
                case BinopCondition.Operator.VERSION_LESS:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator VERSION_LESS")
                case BinopCondition.Operator.VERSION_GREATER:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator VERSION_GREATER")
                case BinopCondition.Operator.VERSION_LESS_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator VERSION_LESS_EQUAL")
                case BinopCondition.Operator.VERSION_GREATER_EQUAL:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator VERSION_GREATER_EQUAL")
                case BinopCondition.Operator.MATCHES:
                    return eval_matches(cond.left, cond.right)
                case BinopCondition.Operator.IN_LIST:
                    return eval_in_list(cond.left, cond.right)
                case BinopCondition.Operator.IS_NEWER_THAN:
                    raise FatalEvalError(cond.pos, "Unsupported condition operator IS_NEWER_THAN")
                case BinopCondition.Operator.AND:
                    return eval_and(cond.left, cond.right)
                case BinopCondition.Operator.OR:
                    return eval_or(cond.left, cond.right)
            assert_never(cond.operator)
        if isinstance(cond, ArgumentCondition):
            if not cond.was_bare:
                if is_true_constant(cond.string.string, cond.pos):
                    return TrueResult()
                return FalseResult()
            if is_true_constant(cond.string.string, cond.pos):
                return TrueResult()
            if is_false_constant(cond.string.string, cond.pos):
                return FalseResult()
            variable_name = cond.string.string
            value = variable_state.get(_cu.ResolutionType.GENERAL, variable_name, cond.pos)
            if isinstance(value, _cv.CertainValue):
                if value.string is None or is_false_constant(value.string, cond.pos):
                    return FalseResult()
                return TrueResult()
            if isinstance(value, _cv.UncertainValue):
                position = _cur.Position(file_index, cond.pos)
                reason = _cur.ExpansionUncertaintyReason(command_name, variable_name, position, value.reason)
                return UncertainResult(reason)
            assert_never(value)
        if isinstance(cond, UncertainCondition):
            return UncertainResult(cond.reason)
        assert_never(cond)

    def eval_defined(operand: Condition) -> Result:
        result = eval_as_str(operand)
        match result:
            case _CertainStringResult(string):
                pass
            case UncertainResult():
                return result
        var_ref = _cu.parse_variable_reference(string.string)
        value = variable_state.get(var_ref.resolution_type, var_ref.variable_name, operand.pos)
        if isinstance(value, _cv.CertainValue):
            return TrueResult() if value.string is not None else FalseResult()
        if isinstance(value, _cv.UncertainValue):
            position = _cur.Position(file_index, cond.pos)
            reason = _cur.ExpansionUncertaintyReason(command_name, var_ref.variable_name, position, value.reason)
            return UncertainResult(reason)
        assert_never(value)

    def eval_strequal(left: Condition, right: Condition) -> Result:
        result_1 = eval_as_str_from_var_or_str(left)
        result_2 = eval_as_str_from_var_or_str(right)
        if isinstance(result_1, _CertainStringResult):
            pass
        elif isinstance(result_1, UncertainResult):
            return result_1
        else:
            assert_never(result_1)
        if isinstance(result_2, _CertainStringResult):
            pass
        elif isinstance(result_2, UncertainResult):
            return result_2
        else:
            assert_never(result_2)
        if result_1.string.string == result_2.string.string:
            return TrueResult()
        return FalseResult()

    def eval_matches(left: Condition, right: Condition) -> Result:
        result_1 = eval_as_str_from_var_or_str(left)
        result_2 = eval_as_str(right)
        regex:  _cr.Regex | None = None
        reason: _cur.ExpansionUncertaintyReason | None = None
        match result_2:
            case _CertainStringResult():
                try:
                    regex = _cr.compile_(result_2.string.string)
                except _cr.SyntaxError as e:
                    pos = result_2.string.pos_map.map_(e.pos)
                    raise FatalEvalError(pos, "Regular expression syntax error: %s", e)
            case UncertainResult():
                reason = result_2.reason
            case _:
                assert_never(result_2)
        match result_1:
            case _CertainStringResult():
                string = result_1.string.string
            case UncertainResult():
                if not reason:
                    reason = result_1.reason
            case _:
                assert_never(result_1)
        # CMake exposes up to 9 capture groups excluding the full match
        max_groups = 9
        if reason:
            variable_state.taint("CMAKE_MATCH_COUNT", reason)
            n = max_groups
            if regex:
                # If the regex is known, the number of capture groups is known, which is the
                # maximum possible number of capture variables that might be affected.
                n = min(n, regex.num_capture_groups())
            for i in range(n + 1):
                variable_state.taint("CMAKE_MATCH_%s" % i, reason)
            return UncertainResult(reason)
        assert regex
        m = regex.matches(string)
        if m:
            groups = [m.group(0)] + list(m.groups(""))[:max_groups]
            if groups[0]:
                # CMAKE_MATCH_COUNT is the highest N with a nonempty capture
                n = max(i for i in range(len(groups)) if groups[i])
                variable_state.set_("CMAKE_MATCH_COUNT", str(n))
                # Replicating CMake quirk / bug by only setting the capture variable
                # if the captured string is nonempty.
                for i, group in enumerate(groups):
                    if group:
                        variable_state.set_("CMAKE_MATCH_%s" % i, group)
            else:
                # Weirdly, CMake sets CMAKE_MATCH_COUNT to the empty string when the full
                # match is the empty string.
                variable_state.set_("CMAKE_MATCH_COUNT", "")
            return TrueResult()
        else:
            variable_state.set_("CMAKE_MATCH_COUNT", "0")
        return FalseResult()

    def eval_in_list(left: Condition, right: Condition) -> Result:
        result_1 = eval_as_str_from_var_or_str(left)
        result_2 = eval_as_str_from_var(right)
        if isinstance(result_1, _CertainStringResult):
            pass
        elif isinstance(result_1, UncertainResult):
            return result_1
        else:
            assert_never(result_1)
        if isinstance(result_2, _CertainStringResult):
            pass
        elif isinstance(result_2, UncertainResult):
            return result_2
        else:
            assert_never(result_2)
        if result_1.string.string in _cu.list_split(result_2.string.string):
            return TrueResult()
        return FalseResult()

    def eval_not(operand: Condition) -> Result:
        return ~eval_as_bool(operand)

    def eval_and(left: Condition, right: Condition) -> Result:
        # In CMake, AND is not short-circuiting
        return eval_as_bool(left) & eval_as_bool(right)

    def eval_or(left: Condition, right: Condition) -> Result:
        # In CMake, OR is not short-circuiting
        return eval_as_bool(left) | eval_as_bool(right)

    def eval_as_str_from_var(cond: Condition) -> _StringResult:
        result = eval_as_str(cond)
        match result:
            case _CertainStringResult(string):
                pass
            case UncertainResult():
                return result
            case _:
                assert_never(result)
        variable_name = string.string
        value = variable_state.get(_cu.ResolutionType.GENERAL, variable_name, cond.pos)
        if isinstance(value, _cv.CertainValue):
            string = _tp.PosMappedString.from_nonlinear_string(value.string or "", cond.pos)
            is_derived = True
            return _CertainStringResult(string, is_derived)
        if isinstance(value, _cv.UncertainValue):
            position = _cur.Position(file_index, cond.pos)
            reason = _cur.ExpansionUncertaintyReason(command_name, variable_name, position, value.reason)
            return UncertainResult(reason)
        assert_never(value)

    def eval_as_str_from_var_or_str(cond: Condition) -> _StringResult:
        if isinstance(cond, ArgumentCondition) and cond.was_bare:
            variable_name = cond.string.string
            value = variable_state.get(_cu.ResolutionType.GENERAL, variable_name, cond.pos)
            if isinstance(value, _cv.CertainValue):
                if value.string is None:
                    return _CertainStringResult(cond.string, cond.is_derived)
                string = _tp.PosMappedString.from_nonlinear_string(value.string, cond.pos)
                is_derived = True
                return _CertainStringResult(string, is_derived)
            if isinstance(value, _cv.UncertainValue):
                position = _cur.Position(file_index, cond.pos)
                reason = _cur.ExpansionUncertaintyReason(command_name, variable_name, position, value.reason)
                return UncertainResult(reason)
            assert_never(value)
        return eval_as_str(cond)

    def eval_as_str(cond: Condition) -> _StringResult:
        if isinstance(cond, ArgumentCondition):
            return _CertainStringResult(cond.string, cond.is_derived)
        result = eval_as_bool(cond)
        match result:
            case FalseResult():
                string = "0"
            case TrueResult():
                string = "1"
            case UncertainResult():
                return result
            case _:
                assert_never(result)
        is_derived = True
        return _CertainStringResult(_tp.PosMappedString.from_nonlinear_string(string, cond.pos), is_derived)

    def is_false_constant(string: str, pos: int) -> bool:
        string_cf = string.casefold()
        if string_cf in {"off", "no", "false", "n", "ignore", "notfound", ""} or string_cf.endswith("-notfound"):
            return True
        value = _b.Wrap(0)
        if as_number(string, value, pos):
            return value == 0
        return False

    def is_true_constant(string: str, pos: int) -> bool:
        string_cf = string.casefold()
        if string_cf in {"on", "yes", "true", "y"}:
            return True
        value = _b.Wrap(0)
        if as_number(string, value, pos):
            return value != 0
        return False

    def as_number(string: str, value: _b.Wrap[int], pos: int) -> bool:
        m = _FLOAT_REGEX.fullmatch(string)
        if not m:
            return False
        if _INT_REGEX.fullmatch(string):
            value.value = int(string)
            return True
        raise FatalEvalError(pos, "Unsupported floating-point syntax (%s)", _b.quote(string))

    return eval_as_bool(cond)


type _StringResult = _CertainStringResult | UncertainResult

@dataclass(slots=True, frozen=True)
class _CertainStringResult:
    string:     _tp.PosMappedString
    is_derived: bool


# Attuned to C function `strtod()`
_FLOAT_REGEX = re.compile(r"""
    \s*   # Allow leading whitespace
    [+-]? # Optional sign
    (?:
        # 1. Hexadecimal Floats
        0x
        (?:
            [0-9a-f]+\.?[0-9a-f]* |
            \.[0-9a-f]+
        )
        (?:p[+-]?[0-9]+)?

        |

        # 2. Decimal Floats
        (?:
            [0-9]+\.?[0-9]* |
            \.[0-9]+
        )
        (?:e[+-]?[0-9]+)?

        |

        # 3. Infinity and NaN
        INF(?:INITY)?
        |
        NAN(?:\([a-z0-9_]*\))?
    )
""", re.VERBOSE | re.IGNORECASE)


_INT_REGEX = re.compile(r"\s*[+-]?[0-9]+")
