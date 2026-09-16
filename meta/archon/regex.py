from __future__ import annotations

import typing
import dataclasses
import enum


def format_as_python_regex(expression: Expression) -> str:
    return _format_as_python_regex(expression)


def is_nullable(expression: Expression) -> bool:
    return _is_nullable(expression)


type Expression = Alternation | Sequence | Repetition | Group | Wildcard | CharClass | Literal | Anchor

@dataclasses.dataclass(slots=True, frozen=True)
class Alternation:
    alternatives: list[Expression]  # Must be non-empty

@dataclasses.dataclass(slots=True, frozen=True)
class Sequence:
    elements: list[Expression]

@dataclasses.dataclass(slots=True, frozen=True)
class Repetition:
    expression: Expression
    min_: int         # Must be greater than or equal to zero
    max_: int | None  # When specified, must be greater than or equal to min_

@dataclasses.dataclass(slots=True, frozen=True)
class Group:
    expr: Expression

@dataclasses.dataclass(slots=True, frozen=True)
class Wildcard:
    pass

@dataclasses.dataclass(slots=True, frozen=True)
class CharClass:
    inverted: bool
    items:    list[Item]  # Must be non-empty

@dataclasses.dataclass(slots=True, frozen=True)
class Literal:
    string: str

@dataclasses.dataclass(slots=True, frozen=True)
class Anchor:
    class Type(enum.Enum):
        BEGIN = 0
        END   = 1
    type_: Type


type Item = Range | Char

@dataclasses.dataclass(slots=True, frozen=True)
class Range:
    # `last` must be greater than or equal to `first` from the point of view Unicode code
    # points
    first: str  # Must be a single character
    last:  str  # Must be a single character

@dataclasses.dataclass(slots=True, frozen=True)
class Char:
    char: str  # Must be a single character








def _format_as_python_regex(expression: Expression) -> str:
    PREC_ALT  = 0
    PREC_SEQ  = 1
    PREC_REP  = 2
    PREC_ATOM = 3

    def format_(expression: Expression, parent_prec: int) -> str:
        match expression:
            case Alternation(alternatives):
                assert alternatives
                if len(alternatives) == 1:
                    return format_(alternatives[0], parent_prec)
                string = "|".join(format_(a, PREC_ALT) for a in alternatives)
                if parent_prec > PREC_ALT:
                    return "(?:%s)" % string
                return string

            case Sequence(elements):
                if len(elements) == 1:
                    return format_(elements[0], parent_prec)
                string = "".join(format_(e, PREC_SEQ) for e in elements)
                if parent_prec > PREC_SEQ:
                    return "(?:%s)" % string
                return string

            case Repetition(subexpression, min_, max_):
                assert min_ >= 0
                assert max_ is None or max_ >= min_
                if min_ == 0 and max_ == 1:
                    quant = "?"
                elif min_ == 0 and max_ is None:
                    quant = "*"
                elif min_ == 1 and max_ is None:
                    quant = "+"
                elif max_ is None:
                    quant = "{%s,}" % min_
                elif min_ == max_:
                    quant = "{%s}" % min_
                else:
                    quant = "{%s,%s}" % (min_, max_)
                string = format_(subexpression, PREC_REP) + quant
                if parent_prec >= PREC_REP:
                    return "(?:%s)" % string
                return string

            case Group(expr):
                string = format_(expr, PREC_ALT)
                return "(%s)" % string

            case Wildcard():
                return "."

            case CharClass(inverted, items):
                assert items
                parts = []
                if inverted:
                    parts.append("^")
                for item in items:
                    match item:
                        case Range(first, last):
                            assert ord(first) <= ord(last)
                            parts.append("%s-%s" % (format_class_char(first), format_class_char(last)))
                            continue
                        case Char(char):
                            assert len(char) == 1
                            parts.append(format_class_char(char))
                            continue
                    typing.assert_never(item)
                return "[%s]" % "".join(parts)

            case Literal(string):
                string_2 = "".join("\\" + c if c in r"\.^$*+?{}[]|()" else c for c in string)
                if len(string) != 1 and parent_prec > PREC_SEQ:
                    return "(?:%s)" % string_2
                return string_2

            case Anchor(type_):
                match type_:
                    case Anchor.Type.BEGIN:
                        string_2 = "^"
                    case Anchor.Type.END:
                        string_2 = "$"
                    case _:
                        typing.assert_never(type_)
                if parent_prec > PREC_SEQ:
                    return "(?:%s)" % string_2
                return string_2

        typing.assert_never(expression)

    def format_class_char(char: str) -> str:
        return "\\" + char if char in r"]\-^" else char

    return format_(expression, PREC_ALT)


def _is_nullable(expression: Expression) -> bool:
    match expression:
        case Alternation(alternatives):
            return any(_is_nullable(a) for a in alternatives)
        case Sequence(elements):
            return all(_is_nullable(e) for e in elements)
        case Repetition(subexpression, min_, max_):
            return min_ == 0 or _is_nullable(subexpression)
        case Group(subexpression):
            return _is_nullable(subexpression)
        case Wildcard() | CharClass():
            return False
        case Literal(string):
            return len(string) == 0
        case Anchor():
            return True
    typing.assert_never(expression)
