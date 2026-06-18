from __future__ import annotations
from typing import Protocol, Any
from dataclasses import dataclass

import re

import archon.base as _b
import archon.text_pos as _tp
import archon.cmake.util as _cu


def parse(string: str, pos: int, error_handler: ErrorHandler) -> Expr:
    return _parse(string, pos, error_handler)


class ErrorHandler(Protocol):
    def __call__(self, pos: int, message: str, *args: Any) -> None:
        ...


type Expr = StringExpr | CompositeExpr | ExpansionExpr

@dataclass(slots=True, frozen=True)
class ExprBase:
    pos: int

@dataclass(slots=True, frozen=True)
class StringExpr(ExprBase):
    string: _tp.PosMappedString

@dataclass(slots=True, frozen=True)
class CompositeExpr(ExprBase):
    parts: list[Expr]

@dataclass(slots=True, frozen=True)
class ExpansionExpr(ExprBase):
    resolution_type: _cu.ResolutionType
    name_expr:       Expr


def _parse(string: str, pos: int, error_handler: ErrorHandler) -> Expr:
    @dataclass(slots=True)
    class VarExpansion:
        resolution_type: _cu.ResolutionType
        pos:             int
        invalid:         bool

    stack = list[tuple[VarExpansion | None, list[Expr]]]()
    parts = list[Expr]()

    var_expansion: VarExpansion | None = None
    string_builder: _tp.PosMappedStringBuilder | None = None

    def ensure_string_builder(ref_pos: int) -> _tp.PosMappedStringBuilder:
        nonlocal string_builder
        if not string_builder:
            string_builder = _tp.PosMappedStringBuilder(ref_pos)
        return string_builder

    def add_linear(string: str, ref_pos: int) -> None:
        ensure_string_builder(ref_pos).add_linear(string, ref_pos)

    def add_nonlinear(string: str, ref_pos: int) -> None:
        ensure_string_builder(ref_pos).add_nonlinear(string, ref_pos)

    def flush(ref_pos: int, finalize: bool = False) -> None:
        nonlocal string_builder
        if finalize and not parts:
            ensure_string_builder(ref_pos)
        if string_builder:
            assert ref_pos >= string_builder.ref_pos
            string = string_builder.get()
            if string.string or (finalize and not parts):
                parts.append(StringExpr(string.pos_map.lead_ref_pos, string))
            string_builder = None

    def get_expr() -> Expr:
        assert parts
        if len(parts) == 1:
            return parts[0]
        return CompositeExpr(parts[0].pos, parts)

    invalid_escape_seen = False
    subpos = 0
    for m in _REGEX.finditer(string):
        if m.start() > subpos:
            substring = string[subpos:m.start()]
            if var_expansion and not var_expansion.invalid:
                # Allowing `$` in order to replicate CMake quirk / bug introduced during
                # implementation of policy CMP0053.
                m_2 = re.search(r"[^$+\-./0-9A-Z_a-z]", substring)
                if m_2:
                    error_pos = pos + subpos + m_2.start()
                    error_handler(error_pos, "Invalid literal character (%s) in variable name", _b.quote(m_2.group(0)))
                    var_expansion.invalid = True
            add_linear(substring, pos + subpos)
        token_pos = pos + m.start()
        token_text = m.group(0)
        subpos = m.end()

        if m.group("ESCAPE"):
            char = token_text[-1]
            replacement = char
            if char.isalnum():
                match char:
                    case "n":
                        replacement = "\n"
                    case "t":
                        replacement = "\t"
                    case "r":
                        replacement = "\r"
                    case _:
                        if not var_expansion.invalid if var_expansion else not invalid_escape_seen:
                            error_handler(token_pos, "Invalid escape sequence (\"%s\")", token_text)
                        if var_expansion:
                            var_expansion.invalid = True
                        else:
                            invalid_escape_seen = True
                        replacement = ""
            elif char == ";" and not var_expansion:
                # CMake retains the escape sequence `\;` if it occurs outside a variable
                # expansion, i.e., when it does not occur in a literal part of a variable
                # name. All other escape sequences are replaced by the characters they
                # represent. This prevents the semicolon from being interpreted as a list
                # item separator if the contents of the string is subjected to list-based
                # splitting. If it is, the splitting operation will cause `\;` to be
                # replaced with `;`. If the contents of the string is never subjected to
                # list-based splitting, `\;` will never be replaced with `;`. A quirky
                # side-effect of this scheme, is that `\;` and `\\;` has the exact same
                # meaning / effect.
                replacement = "\\;"
            elif char == "\n":
                # Inside a quoted argument, a backslash followed by a newline is replaced
                # with nothing by CMake. Inside a bare argument (not escaped and not
                # bracketed), such an escape sequence can never occur.
                replacement = ""
            add_nonlinear(replacement, token_pos)
            continue

        if m.group("VAREXP"):
            flush(token_pos)
            assert len(token_text) >= 2 and token_text[0] == "$" and token_text[-1] == "{"
            domain = token_text[1:-1]
            resolution_type = _cu.ResolutionType.GENERAL
            invalid = False
            match domain:
                case "":
                    pass
                case "CACHE":
                    resolution_type = _cu.ResolutionType.CACHE
                case "ENV":
                    resolution_type = _cu.ResolutionType.ENV
                case _:
                    error_handler(token_pos, "Invalid domain (%s) in variable expansion", _b.quote(domain))
                    invalid = True
            stack.append((var_expansion, parts))
            parts = []
            var_expansion = VarExpansion(resolution_type, token_pos, invalid)
            continue

        if m.group("RBRACE"):
            if not var_expansion:
                add_linear(token_text, token_pos)
                continue
            flush(token_pos, finalize=True)
            orig = var_expansion
            var_expansion, parts = stack.pop()
            if not orig.invalid:
                name_expr = get_expr()
                parts.append(ExpansionExpr(orig.pos, orig.resolution_type, name_expr))
            continue

        assert False

    if len(string) > subpos:
        substring = string[subpos:]
        add_linear(substring, pos + subpos)
    flush(pos + len(string))

    while var_expansion:
        if not var_expansion.invalid:
            error_handler(var_expansion.pos, "Unclosed variable expansion")
        var_expansion, parts = stack.pop()

    flush(pos + len(string), finalize=True)
    return get_expr()


_REGEX = re.compile(
    r'(?P<ESCAPE>\\.)|'
    r'(?P<VAREXP>\$[+\-./0-9A-Z_a-z]*\{)|'
    r'(?P<RBRACE>\})',
    re.DOTALL,
)
