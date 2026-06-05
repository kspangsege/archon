from __future__ import annotations
from typing import Protocol, Any, TextIO, assert_never
from collections.abc import Iterator
from dataclasses import dataclass

import enum
import re
import sys
import pathlib

import archon.base as _b
import archon.graph as _g
import archon.ansi as _a
import archon.log as _l


def parse(path: pathlib.Path, error_handler: ErrorHandler) -> Grammar | None:
    return _parse(path, error_handler)

def format_grammar(grammar: Grammar) -> str:
    return _format_grammar(grammar)

def format_expression(expression: Expression) -> str:
    return _format_expression(expression)

def analyze_as_ell1(grammar: Grammar, output_stream: TextIO = sys.stdout) -> bool:
    return _analyze_as_ell1(grammar, output_stream)


class ErrorHandler(Protocol):
    def __call__(self, pos: _l.FullFilePos, message: str, *args: Any) -> None:
        ...


class FormatError(Exception):
    pass


# A grammar must have at least one nonterminal definition, and no two definitions must be
# for the same nonterminal.
#
# All named terminals referenced in the grammar must be specified exactly once in
# `named_terminals`.
#
# No defined nonterminal must match a named terminal.
#
@dataclass(slots=True, frozen=True)
class Grammar:
    named_terminals:   list[str]
    start_nonterminal: str  # Must match one of the defined nonterminals
    definitions:       list[Definition]

@dataclass(slots=True, frozen=True)
class Definition:
    nonterminal: str  # Must match matches r"\w+" but not r"\d.*"
    expression:  Expression

type Expression = Alternation | Sequence | Repetition | Nonterminal | Terminal

@dataclass(slots=True, frozen=True)
class Alternation:
    alternatives: list[Expression]  # Must be nonempty

@dataclass(slots=True, frozen=True)
class Sequence:
    elements: list[Expression]

@dataclass(slots=True, frozen=True)
class Repetition:
    expression: Expression
    min_: int         # Must be greater than or equal to zero
    max_: int | None  # When specified, must be greater than or equal to min_

@dataclass(slots=True, frozen=True)
class Nonterminal:
    name: str  # Must match one of the defined nonterminals

type Terminal = NamedTerminal | LiteralTerminal

@dataclass(slots=True, frozen=True)
class NamedTerminal:
    name: str  # Must match one of the named terminals

@dataclass(slots=True, frozen=True)
class LiteralTerminal:
    string: str



class ParseError(Exception):
    pass


# Self-grammar for Extended Context-Free Grammar (ECFG) plus directive syntax
#
# %token DIR_LEAD  # Generated at start of line that contains "%token" or "%start"
# %token DEF_LEAD  # Generated at start of line that contains "=", but not "%token" or "%start"
# %token NAME
# %token LITERAL
#
# grammar     = directive* definition+
# directive   = DIR_LEAD (token_decl | start_dir)
# token_decl  = "%token" NAME  # Introduces a named terminal
# start_dir   = "%start" NAME  # Specifies the start nonterminal (defaults to first defined nonterminal)
# definition  = DEF_LEAD nonterminal "=" alternation
# alternation = sequence ("|" sequence)*
# sequence    = repetition*
# repetition  = primary ("*" | "+" | "?")?
# primary     = nonterminal | terminal | "(" alternation ")"
# nonterminal = NAME
# terminal    = LITERAL | NAME  # Can be NAME only if the name is one of the named terminals


def _parse(path: pathlib.Path, error_handler: ErrorHandler) -> Grammar | None:
    named_terminals      = list[str]()
    named_terminal_map   = dict[str, int]()
    defined_nonterminals = set[str]()
    referenced_names     = list[tuple[str, _l.FullFilePos]]()
    referenced_name_map  = dict[str, int]()

    start_nonterminal: str | None = None
    start_dir_pos:     _l.FullFilePos

    token: _Token

    def parse_grammar() -> Grammar | None:
        nonlocal start_nonterminal
        while isinstance(token, _DirLeadToken):
            parse_directive()
        definitions = []
        while not isinstance(token, _EndOfInputToken):
            definition = parse_definition()
            if definition:
                definitions.append(definition)
        if not definitions:
            if not errors_seen:
                error(token.pos, "No definitions")
            return None
        if start_nonterminal is not None:
            if start_nonterminal not in defined_nonterminals:
                error(start_dir_pos, "Specified start nonterminal `%s` is not defined", start_nonterminal)
                return None
        else:
            start_nonterminal = definitions[0].nonterminal
        for name, pos in referenced_names:
            if name not in defined_nonterminals:
                error(pos, "Undefined name `%s`", name)
        return Grammar(named_terminals, start_nonterminal, definitions)

    def parse_directive() -> None:
        assert isinstance(token, _DirLeadToken)
        advance()
        if is_symbol(_SymbolToken.Symbol.TOKEN):
            parse_token_decl()
            return
        if is_symbol(_SymbolToken.Symbol.START):
            parse_start_dir()
            return
        error(token.pos, "Unexpected start of directive")
        skip_directive()

    def parse_token_decl() -> None:
        assert is_symbol(_SymbolToken.Symbol.TOKEN)
        pos = token.pos
        advance()
        if not isinstance(token, _NameToken):
            error(token.pos, "Expected token name after `%token`")
            skip_directive()
            return
        name = token
        advance()
        if not isinstance(token, (_DirLeadToken, _DefLeadToken)):
            error(token.pos, "Unexpected text after token declaration")
            skip_directive()
            return
        if name.value in named_terminal_map:
            error(name.pos, "Redeclaration of token `%s`", name)
            return
        named_terminal_map[name.value] = len(named_terminals)
        named_terminals.append(name.value)

    def parse_start_dir() -> None:
        nonlocal start_nonterminal, start_dir_pos
        assert is_symbol(_SymbolToken.Symbol.START)
        pos = token.pos
        advance()
        if not isinstance(token, _NameToken):
            error(token.pos, "Expected nonterminal name after `%start`")
            skip_directive()
            return
        name = token
        advance()
        if not isinstance(token, (_DirLeadToken, _DefLeadToken)):
            error(token.pos, "Unexpected text after start directive")
            skip_directive()
            return
        if start_nonterminal is not None:
            error(pos, "Respecification of start nonterminal")
            return
        start_nonterminal = name.value
        start_dir_pos     = pos

    def parse_definition() -> Definition | None:
        if isinstance(token, _DefLeadToken):
            advance()
        if not isinstance(token, _NameToken):
            error(token.pos, "Expected nonterminal name for definition")
            skip_definition()
            return None
        nonterminal = token.value
        advance()
        if nonterminal in defined_nonterminals:
            error(token.pos, "Redefinition of nonterminal `%s`", nonterminal)
        defined_nonterminals.add(nonterminal)
        if not is_symbol(_SymbolToken.Symbol.EQUAL):
            error(token.pos, "Expected equals sign after nonterminal being defined")
            skip_definition()
            return None
        advance()
        expression = parse_alternation()
        if is_symbol(_SymbolToken.Symbol.EQUAL):
            error(token.pos, "Stray equals sign")
            skip_definition()
            return None
        assert isinstance(token, (_DefLeadToken, _EndOfInputToken))
        return Definition(nonterminal, expression)

    def parse_alternation() -> Expression:
        alternatives = []
        while True:
            expression = parse_sequence()
            if expression:
                alternatives.append(expression)
            if not is_symbol(_SymbolToken.Symbol.BAR):
                break
            advance()
        assert len(alternatives) >= 1
        if len(alternatives) == 1:
            return alternatives[0]
        return Alternation(alternatives)

    def parse_sequence() -> Expression:
        elements = []
        while True:
            if not isinstance(token, (_NameToken, _LiteralToken)) and not is_symbol(_SymbolToken.Symbol.LPAREN):
                break
            expression = parse_repetition()
            if expression:
                elements.append(expression)
        if len(elements) == 1:
            return elements[0]
        return Sequence(elements)

    def parse_repetition() -> Expression | None:
        expression = parse_primary()
        max_: int | None
        if is_symbol(_SymbolToken.Symbol.STAR):
            min_, max_ = 0, None
        elif is_symbol(_SymbolToken.Symbol.PLUS):
            min_, max_ = 1, None
        elif is_symbol(_SymbolToken.Symbol.QMARK):
            min_, max_ = 0, 1
        else:
            return expression
        advance()
        if not expression:
            return None
        return Repetition(expression, min_, max_)

    def parse_primary() -> Expression | None:
        name: Any
        if isinstance(token, _NameToken):
            name = token
            advance()
            if name.value in named_terminal_map:
                return NamedTerminal(name.value)
            if name.value not in referenced_name_map:
                referenced_name_map[name.value] = len(referenced_names)
                referenced_names.append((name.value, name.pos))
            return Nonterminal(name.value)
        if isinstance(token, _LiteralToken):
            literal = token
            advance()
            return LiteralTerminal(literal.value)
        if is_symbol(_SymbolToken.Symbol.LPAREN):
            advance()
            expression = parse_alternation()
            if not is_symbol(_SymbolToken.Symbol.RPAREN):
                error(token.pos, "Expected closing parenthesis")
                return None
            advance()
            return expression
        assert False

    def skip_directive() -> None:
        while not isinstance(token, (_EndOfInputToken, _DirLeadToken, _DefLeadToken)):
            advance()

    def skip_definition() -> None:
        while not isinstance(token, (_EndOfInputToken, _DefLeadToken)):
            advance()

    def is_symbol(symbol: _SymbolToken.Symbol) -> bool:
        return isinstance(token, _SymbolToken) and token.symbol is symbol

    errors_seen = False
    def error(pos: _l.FullFilePos, message: str, *args: Any) -> None:
        nonlocal errors_seen
        errors_seen = True
        error_handler(pos, message, *args)

    def tokenize_error_handler(pos: _l.FullFilePos, message: str, *args: Any) -> None:
        error(pos, message, *args)

    tokens = _tokenize(path, tokenize_error_handler)
    token = next(tokens)
    def advance() -> None:
        nonlocal token
        assert not isinstance(token, _EndOfInputToken)
        token = next(tokens)

    grammar = parse_grammar()
    if errors_seen:
        return None
    return grammar


def _tokenize(path: pathlib.Path, error_handler: ErrorHandler) -> Iterator[_Token]:
    with open(path, "r", encoding="utf-8") as file_:
        line_no = 0

        while True:
            line_no += 1
            line = file_.readline()
            if not line:
                break
            line = line.rstrip("\n")

            tokens: list[_Token | _ErrorToken] = []
            has_directive = False
            has_equal     = False

            for m in _TOKEN_REGEX.finditer(line):
                kind = m.lastgroup
                text = m.group()
                pos = _l.FullFilePos(line_no, m.start())

                if kind in ("WS", "COMMENT"):
                    continue

                if kind == "NAME":
                    value = text
                    tokens.append(_NameToken(pos, text, value))
                    continue

                if kind == "LITERAL":
                    value = value = re.sub(r"\\(.)", r"\1", text[1:-1])
                    tokens.append(_LiteralToken(pos, text, value))
                    continue

                if kind == "SYMBOL":
                    symbol = _SYMBOL_MAP[text]
                    if symbol in [_SymbolToken.Symbol.TOKEN, _SymbolToken.Symbol.START]:
                        has_directive = True
                    elif symbol == _SymbolToken.Symbol.EQUAL:
                        has_equal = True
                    tokens.append(_SymbolToken(pos, text, symbol))
                    continue

                if kind == "UNCLOSED_LIT":
                    tokens.append(_ErrorToken(pos, text, _ErrorToken.Error.UNCLOSED_LITERAL))
                    continue

                if kind == "ILL_CHAR":
                    tokens.append(_ErrorToken(pos, text, _ErrorToken.Error.ILLEGAL_CHARACTER))
                    continue

                assert False

            if has_directive:
                yield _DirLeadToken(_l.FullFilePos(line_no))
            elif has_equal:
                yield _DefLeadToken(_l.FullFilePos(line_no))

            for token in tokens:
                if not isinstance(token, _ErrorToken):
                    yield token
                    continue
                match token.error:
                    case _ErrorToken.Error.ILLEGAL_CHARACTER:
                        error_handler(token.pos, "Illegal character (%s)", _b.quote(token.text))
                        continue
                    case _ErrorToken.Error.UNCLOSED_LITERAL:
                        error_handler(token.pos, "Unclosed literal")
                        continue
                assert_never(token.error)

        yield _EndOfInputToken(_l.FullFilePos(line_no))


type _Token = _DirLeadToken | _DefLeadToken | _NameToken | _LiteralToken | _SymbolToken | _EndOfInputToken

@dataclass(slots=True, frozen=True)
class _TokenBase:
    pos: _l.FullFilePos

@dataclass(slots=True, frozen=True)
class _DirLeadToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _DefLeadToken(_TokenBase):
    pass

@dataclass(slots=True, frozen=True)
class _NameToken(_TokenBase):
    text:  str
    value: str

@dataclass(slots=True, frozen=True)
class _LiteralToken(_TokenBase):
    text:  str
    value: str

@dataclass(slots=True, frozen=True)
class _SymbolToken(_TokenBase):
    class Symbol(enum.Enum):
        TOKEN  = 0
        START  = 1
        EQUAL  = 2
        BAR    = 3
        STAR   = 4
        PLUS   = 5
        QMARK  = 6
        LPAREN = 7
        RPAREN = 8
    text:   str
    symbol: Symbol

@dataclass(slots=True, frozen=True)
class _ErrorToken(_TokenBase):
    class Error(enum.Enum):
        ILLEGAL_CHARACTER = 0
        UNCLOSED_LITERAL  = 1
    text:  str
    error: Error

@dataclass(slots=True, frozen=True)
class _EndOfInputToken(_TokenBase):
    pass


_TOKEN_REGEX = re.compile(
    r"(?P<WS>[ \t]+)|"
    r"(?P<COMMENT>#.*)|"
    r"(?P<NAME>[a-zA-Z_]\w*)|"
    r"(?P<LITERAL>\"(?:[^\"\\]|\\.)*\")|"
    r"(?P<SYMBOL>%token\b|%start\b|=|\||\*|\+|\?|\(|\))|"
    r"(?P<UNCLOSED_LIT>\"(?:[^\"\\]|\\.)*\\?)|"
    r"(?P<ILL_CHAR>.)"
)


_SYMBOL_MAP = {
    "%token": _SymbolToken.Symbol.TOKEN,
    "%start": _SymbolToken.Symbol.START,
    "=":      _SymbolToken.Symbol.EQUAL,
    "|":      _SymbolToken.Symbol.BAR,
    "*":      _SymbolToken.Symbol.STAR,
    "+":      _SymbolToken.Symbol.PLUS,
    "?":      _SymbolToken.Symbol.QMARK,
    "(":      _SymbolToken.Symbol.LPAREN,
    ")":      _SymbolToken.Symbol.RPAREN,
}


def _format_grammar(grammar: Grammar) -> str:
    max_lhs_size = 0
    for definition in grammar.definitions:
        size = len(definition.nonterminal)
        if size > max_lhs_size:
            max_lhs_size = size
    def format_token_decl(name: str) -> str:
        return "%%token %s\n" % name
    def format_start_dir(nonterminal: str) -> str:
        return "%%start %s\n" % nonterminal
    def format_definition(definition: Definition) -> str:
        lhs = definition.nonterminal
        indent_1 = " " * len(lhs)
        indent_2 = " " * (max_lhs_size - len(lhs))
        lines = list[str]()
        def add(expression):
            rhs = _format_expression(expression)
            if not lines:
                line = "%s%s = %s\n" % (lhs, indent_2, rhs)
            else:
                line = "%s%s | %s\n" % (indent_1, indent_2, rhs)
            lines.append(line)
        if not isinstance(definition.expression, Alternation):
            add(definition.expression)
        else:
            for alternative in definition.expression.alternatives:
                add(alternative)
        return "".join(lines)
    parts = []
    for name in grammar.named_terminals:
        parts.append(format_token_decl(name))
    parts.append("\n")
    if grammar.start_nonterminal != grammar.definitions[0].nonterminal:
        parts.append(format_start_dir(grammar.start_nonterminal))
        parts.append("\n")
    for definition in grammar.definitions:
        parts.append(format_definition(definition))
    return "%s" % "".join(parts)


def _format_expression(expression: Expression) -> str:
    PREC_MIN         = 0
    PREC_ALTERNATION = 1
    PREC_SEQUENCE    = 2
    PREC_REPETITION  = 3
    def format_expr(expression: Expression, parent_prec: int) -> str:
        match expression:
            case Alternation(alternatives):
                string = " | ".join(format_expr(a, PREC_ALTERNATION) for a in alternatives)
                return "(%s)" % string if PREC_ALTERNATION < parent_prec else string
            case Sequence(elements):
                if not elements:
                    return ""
                string = " ".join(format_expr(e, PREC_SEQUENCE) for e in elements)
                return "(%s)" % string if PREC_SEQUENCE < parent_prec else string
            case Repetition(subexpression, min_, max_):
                string = format_expr(subexpression, PREC_REPETITION)
                if min_ == 0 and max_ is None:
                    string += "*"
                elif min_ == 1 and max_ is None:
                    string += "+"
                elif min_ == 0 and max_ == 1:
                    string += "?"
                else:
                    raise FormatError("Unsupported quantifier")
                return "(%s)" % string if PREC_REPETITION < parent_prec else string
            case Nonterminal(name):
                return name
            case NamedTerminal(name):
                return name
            case LiteralTerminal(string):
                return '"%s"' % re.sub(r'([\\"])', r'\\\1', string)
        assert_never(expression)
    return format_expr(expression, PREC_MIN)


def _analyze_as_ell1(grammar: Grammar, output_stream: TextIO = sys.stdout) -> bool:
    is_ell1 = True

    definition_map = dict[str, Definition]()
    for definition in grammar.definitions:
        definition_map[definition.nonterminal] = definition

    def find_reachable_nonterminals() -> set[str]:
        seen = set[str]()
        def visit_definition(nonterminal: str) -> None:
            if nonterminal in seen:
                return
            seen.add(nonterminal)
            definition = definition_map[nonterminal]
            visit_expression(definition.expression)
        def visit_expression(expression: Expression) -> None:
            match expression:
                case Alternation(alternatives):
                    for a in alternatives:
                        visit_expression(a)
                    return
                case Sequence(elements):
                    for e in elements:
                        visit_expression(e)
                    return
                case Repetition(subexpression):
                    visit_expression(subexpression)
                    return
                case Nonterminal(name):
                    visit_definition(name)
                    return
                case NamedTerminal() | LiteralTerminal():
                    return
            assert_never(expression)
        visit_definition(grammar.start_nonterminal)
        return seen

    reachable_nonterminals = find_reachable_nonterminals()

    def detect_left_recursion() -> list[str] | None:
        nonlocal is_ell1
        nonterminals = list(reachable_nonterminals)
        index_map = {name: i for i, name in enumerate(nonterminals)}
        def add_left_dependencies(expression: Expression, dependencies: set[int]) -> None:
            match expression:
                case Alternation(alternatives):
                    for a in alternatives:
                        add_left_dependencies(a, dependencies)
                    return
                case Sequence(elements):
                    for e in elements:
                        add_left_dependencies(e, dependencies)
                        if not is_nullable(e):
                            break
                    return
                case Repetition(subexpression, _, max_):
                    if max_ is None or max_ > 0:
                        add_left_dependencies(subexpression, dependencies)
                    return
                case Nonterminal(name):
                    dependencies.add(index_map[name])
                    return
                case LiteralTerminal() | NamedTerminal():
                    return
            assert_never(expression)
        left_dependencies: list[set[int]] = [set() for _ in nonterminals]
        for definition in grammar.definitions:
            index = index_map.get(definition.nonterminal)
            if index is not None:
                add_left_dependencies(definition.expression, left_dependencies[index])
        cycle = _g.find_dependency_cycle(left_dependencies)
        if cycle:
            return [nonterminals[i] for i in cycle]
        return None

    nullable_map:  dict[str, bool]            = {n: False for n in reachable_nonterminals}
    first_set_map: dict[str, set[_Lookahead]] = {n: set() for n in reachable_nonterminals}

    def is_nullable(expression: Expression) -> bool:
        match expression:
            case Alternation(alternatives):
                return any(is_nullable(a) for a in alternatives)
            case Sequence(elements):
                return all(is_nullable(e) for e in elements)
            case Repetition(subexpression, min_, _):
                return min_ == 0 or is_nullable(subexpression)
            case Nonterminal(name):
                return nullable_map[name]
            case LiteralTerminal() | NamedTerminal():
                return False
        assert_never(expression)

    def compute_first_set(expression: Expression) -> set[_Lookahead]:
        match expression:
            case Alternation(alternatives):
                return set().union(*(compute_first_set(a) for a in alternatives))
            case Sequence(elements):
                first_set = set()
                for e in elements:
                    first_set |= compute_first_set(e)
                    if not is_nullable(e):
                        break
                return first_set
            case Repetition(subexpression, _, max_):
                if max_ is None or max_ > 0:
                    return compute_first_set(subexpression)
                return set()
            case Nonterminal(name):
                return first_set_map[name]
            case LiteralTerminal() | NamedTerminal():
                return {expression}
        assert_never(expression)

    while True:
        changed = False
        for definition in grammar.definitions:
            if not definition.nonterminal in reachable_nonterminals:
                continue
            nullable  = is_nullable(definition.expression)
            first_set = compute_first_set(definition.expression)
            if nullable != nullable_map[definition.nonterminal]:
                nullable_map[definition.nonterminal] = nullable
                changed = True
            if not first_set <= first_set_map[definition.nonterminal]:
                first_set_map[definition.nonterminal] |= first_set
                changed = True
        if not changed:
            break

    follow_set_map: dict[str, set[_Lookahead]] = {n: set() for n in reachable_nonterminals}
    follow_set_map[grammar.start_nonterminal].add(_EndOfInput())

    def update_follow_set(expression: Expression, follow_set: set[_Lookahead]) -> bool:
        match expression:
            case Alternation(alternatives):
                changed = False
                for a in alternatives:
                    changed |= update_follow_set(a, follow_set)
                return changed
            case Sequence(elements):
                changed = False
                follow_set_2 = follow_set
                for e in reversed(elements):
                    changed |= update_follow_set(e, follow_set_2)
                    if is_nullable(e):
                        follow_set_2 = follow_set_2 | compute_first_set(e)
                    else:
                        follow_set_2 = compute_first_set(e)
                return changed
            case Repetition(subexpression, _, max_):
                if max_ is None or max_ > 0:
                    follow_set_2 = follow_set
                    if max_ is None or max_ > 1:
                        follow_set_2 = follow_set_2 | compute_first_set(subexpression)
                    return update_follow_set(subexpression, follow_set_2)
                return False
            case Nonterminal(name):
                target_follow_set = follow_set_map[name]
                orig_len = len(target_follow_set)
                target_follow_set |= follow_set
                return len(target_follow_set) > orig_len
            case LiteralTerminal() | NamedTerminal():
                return False
        assert_never(expression)

    while True:
        changed = False
        for definition in grammar.definitions:
            if not definition.nonterminal in reachable_nonterminals:
                continue
            changed |= update_follow_set(definition.expression, follow_set_map[definition.nonterminal])
        if not changed:
            break

    def analyze_definition(level: int, definition: Definition) -> None:
        nontrivial = isinstance(definition.expression, (Sequence, Alternation, Repetition))
        add_line(level, nontrivial, "Definition of [%s]: Parse [%s]", definition.nonterminal,
                 _format_expression(definition.expression))
        if nontrivial:
            analyze_expression(level + 1, definition.expression, follow_set_map[definition.nonterminal])

    def analyze_expression(level: int, expression: Expression, follow_set: set[_Lookahead]) -> None:
        nonlocal is_ell1
        match expression:
            case Alternation(alternatives):
                alternatives_2 = []
                for i, a in enumerate(alternatives):
                    nullable    = is_nullable(a)
                    first_set   = compute_first_set(a)
                    predict_set = first_set | follow_set if nullable else first_set
                    alternatives_2.append((i, a, predict_set))
                for i, alternative, predict_set in alternatives_2:
                    warnings = []
                    for i_2, alternative_2, predict_set_2 in alternatives_2[i+1:]:
                        overlap = predict_set & predict_set_2
                        if overlap:
                            is_ell1 = False
                            warnings.append("Ambiguity on %s with %s alternative" %
                                            (format_lookahead_set(overlap), _b.as_ord(1 + i_2)))
                    nontrivial = isinstance(alternative, (Sequence, Alternation, Repetition))
                    if_form = "If" if i == 0 else "Else if"
                    add_line(level, nontrivial or warnings, "%s lookahead is in %s: Parse [%s]", if_form,
                             format_lookahead_set(predict_set), _format_expression(alternative))
                    for w in warnings:
                        add_warning(level + 1, "%s", w)
                    if nontrivial:
                        analyze_expression(level + 1, alternative, follow_set)
                return

            case Sequence(elements):
                follow_sets = []
                follow_set_2 = follow_set
                for element in reversed(elements):
                    follow_sets.append(follow_set_2)
                    if is_nullable(element):
                        follow_set_2 = follow_set_2 | compute_first_set(element)
                    else:
                        follow_set_2 = compute_first_set(element)
                follow_sets.reverse()
                for element, follow_set in zip(elements, follow_sets):
                    nontrivial = isinstance(element, (Sequence, Alternation, Repetition))
                    add_line(level, nontrivial, "Parse [%s]", _format_expression(element))
                    if nontrivial:
                        analyze_expression(level + 1, element, follow_set)
                return

            case Repetition(subexpression, min_, max_):
                first_set = compute_first_set(subexpression)
                predict_set_continue = first_set | follow_set if is_nullable(subexpression) else first_set
                predict_set_break    = follow_set
                warnings = []
                if is_nullable(subexpression) and max_ is None:
                    warnings.append("Unbounded repetition of nullable subexpression (infinite loop)")
                if max_ is None or max_ > min_:
                    overlap = predict_set_continue & predict_set_break
                    if overlap:
                        is_ell1 = False
                        warnings.append("Continue/break ambiguity on %s" % format_lookahead_set(overlap))
                quantifier = generate_repetition_quantifier(min_, max_, predict_set_continue)
                nontrivial = isinstance(subexpression, (Sequence, Alternation, Repetition))
                add_line(level, nontrivial or warnings, "Parse [%s] %s", _format_expression(subexpression), quantifier)
                for w in warnings:
                    add_warning(level + 1, "%s", w)
                if nontrivial:
                    if max_ is None or max_ > 0:
                        follow_set_2 = follow_set
                        if max_ is None or max_ > 1:
                            follow_set_2 = follow_set_2 | first_set
                    else:
                        follow_set_2 = set()
                    analyze_expression(level + 1, subexpression, follow_set_2)
                return

            case LiteralTerminal() | NamedTerminal() | Nonterminal():
                assert False

        assert_never(expression)

    def compute_lookahead_order() -> dict[_Lookahead, int]:
        order = dict[_Lookahead, int]()
        def visit(expression: Expression):
            match expression:
                case Alternation(alternatives):
                    for a in alternatives:
                        visit(a)
                    return
                case Sequence(elements):
                    for e in elements:
                        visit(e)
                    return
                case Repetition(subexpression, _, _):
                    visit(subexpression)
                    return
                case Nonterminal() | NamedTerminal():
                    return
                case LiteralTerminal():
                    append(expression)
                    return
        def append(lookahead: _Lookahead) -> None:
            order.setdefault(lookahead, len(order))
        for definition in grammar.definitions:
            visit(definition.expression)
        for name in grammar.named_terminals:
            append(NamedTerminal(name))
        append(_EndOfInput())
        return order

    lookahead_order = compute_lookahead_order()

    def format_lookahead_set(lookahead_set: set[_Lookahead]) -> str:
        def key(lookahead: _Lookahead) -> int:
            return lookahead_order[lookahead]
        def format_(lookahead: _Lookahead) -> str:
            if isinstance(lookahead, _EndOfInput):
                return "<EOI>"
            return _format_expression(lookahead)
        return "{%s}" % ", ".join(format_(l) for l in sorted(lookahead_set, key=key))

    def generate_repetition_quantifier(min_: int, max_: int | None, repeat_set: set[_Lookahead]) -> str:
        if min_ == 0 and max_ == 0:
            return "zero times"

        mandatory = ""
        if min_ == 1:
            mandatory = "once"
        elif min_ > 1:
            mandatory = "%d times" % min_

        optional = ""
        if max_ is None:
            optional = "while lookahead is in %s" % format_lookahead_set(repeat_set)
        elif max_ > min_:
            diff = max_ - min_
            more = " more" if min_ > 0 else ""
            if diff == 1:
                optional = "once%s if lookahead is in %s" % (more, format_lookahead_set(repeat_set))
            else:
                optional = "up to %d times%s while lookahead is in %s" % (diff, more, format_lookahead_set(repeat_set))

        if mandatory and optional:
            return "%s, then %s" % (mandatory, optional)
        if mandatory:
            return mandatory
        return optional

    lines = []
    def add_line(level, want_colon, message: str, *args: Any) -> None:
        indentation = "    " * level
        lines.append("%s%s%s\n" % (indentation, message % args, ":" if want_colon else ""))

    is_ansi_term = _a.is_ansi_term(output_stream)
    def add_warning(level, message: str, *args: Any) -> None:
        want_colon = False
        prefix = "WARNING: "
        if is_ansi_term:
            prefix = _a.ansi_term_set_color(_a.AnsiTermColor.YELLOW) + prefix + _a.ansi_term_reset_color()
        add_line(level, want_colon, "%s%s", prefix, message % args)

    for definition in grammar.definitions:
        if definition.nonterminal not in reachable_nonterminals:
            add_warning(0, "Unreachable nonterminal `%s`", definition.nonterminal)

    cycle = detect_left_recursion()
    if cycle:
        is_ell1 = False
        add_warning(0, "Left recursion detected: %s", " -> ".join(cycle + [cycle[0]]))

    for definition in grammar.definitions:
        if definition.nonterminal in reachable_nonterminals:
            analyze_definition(0, definition)

    output_stream.write("".join(lines))
    return is_ell1


@dataclass(slots=True, frozen=True)
class _EndOfInput:
    pass

type _Lookahead = Terminal | _EndOfInput
