from __future__ import annotations

import typing
import dataclasses
import re

import archon.base as _b
import archon.text_pos as _tp
import archon.cmake.util as _cu


def parse(string: _tp.PosMappedString, error_handler: ErrorHandler) -> Expr:
    return _parse(string, error_handler)


class ErrorHandler(typing.Protocol):
    def __call__(self, pos: int, message: str, *args: typing.Any) -> None:
        ...


type Expr = StringExpr | CompositeExpr | ExpansionExpr

@dataclasses.dataclass(slots=True, frozen=True)
class ExprBase:
    pos: int

@dataclasses.dataclass(slots=True, frozen=True)
class StringExpr(ExprBase):
    string: _tp.PosMappedString

@dataclasses.dataclass(slots=True, frozen=True)
class CompositeExpr(ExprBase):
    parts: list[Expr]

@dataclasses.dataclass(slots=True, frozen=True)
class ExpansionExpr(ExprBase):
    resolution_type: _cu.ResolutionType
    name_expr:       Expr








def _parse(string: _tp.PosMappedString, error_handler: ErrorHandler) -> Expr:
    @dataclasses.dataclass(slots=True)
    class VarExpansion:
        resolution_type: _cu.ResolutionType
        pos:             int
        invalid:         bool

    stack = list[tuple[VarExpansion | None, list[Expr]]]()
    parts = list[Expr]()

    var_expansion: VarExpansion | None = None
    string_builder: _tp.PosMappedStringBuilder | None = None

    def ensure_string_builder(pos: int) -> _tp.PosMappedStringBuilder:
        nonlocal string_builder
        if not string_builder:
            string_builder = _tp.PosMappedStringBuilder(pos)
        return string_builder

    def add_linear(string: str, pos: int) -> None:
        ensure_string_builder(pos).add_linear(string, pos)

    def add_nonlinear(string: str, pos: int) -> None:
        ensure_string_builder(pos).add_nonlinear(string, pos)

    def flush(pos: int, finalize: bool = False) -> None:
        nonlocal string_builder
        if finalize and not parts:
            ensure_string_builder(pos)
        if string_builder:
            assert pos >= string_builder.ref_pos
            string_2 = string_builder.get()
            if string_2.string or (finalize and not parts):
                string_3 = string_2.map_through(string.pos_map)
                parts.append(StringExpr(string_3.pos_map.lead_ref_pos, string_3))
            string_builder = None

    def get_expr() -> Expr:
        assert parts
        if len(parts) == 1:
            return parts[0]
        return CompositeExpr(parts[0].pos, parts)

    invalid_escape_seen = False
    pos = 0
    for m in _REGEX.finditer(string.string):
        if m.start() > pos:
            substring = string.string[pos:m.start()]
            if var_expansion and not var_expansion.invalid:
                # Allowing `$` in order to replicate CMake quirk / bug introduced during
                # implementation of policy CMP0053.
                m_2 = re.search(r"[^$+\-./0-9A-Z_a-z]", substring)
                if m_2:
                    ref_pos = string.pos_map.map_(pos + m_2.start())
                    error_handler(ref_pos, "Invalid literal character (%s) in variable name", _b.quote(m_2.group(0)))
                    var_expansion.invalid = True
            add_linear(substring, pos)
        token_pos = m.start()
        token_text = m.group(0)
        pos = m.end()

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
                            ref_pos = string.pos_map.map_(token_pos)
                            error_handler(ref_pos, "Invalid escape sequence (\"%s\")", token_text)
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
                    ref_pos = string.pos_map.map_(token_pos)
                    error_handler(ref_pos, "Invalid domain (%s) in variable expansion", _b.quote(domain))
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
            name_expr = get_expr()
            orig = var_expansion
            var_expansion, parts = stack.pop()
            if not orig.invalid:
                ref_pos = string.pos_map.map_(orig.pos)
                parts.append(ExpansionExpr(ref_pos, orig.resolution_type, name_expr))
            continue

        assert False

    if len(string.string) > pos:
        substring = string.string[pos:]
        add_linear(substring, pos)
    flush(len(string.string))

    while var_expansion:
        if not var_expansion.invalid:
            ref_pos = string.pos_map.map_(var_expansion.pos)
            error_handler(ref_pos, "Unclosed variable expansion")
        var_expansion, parts = stack.pop()

    flush(len(string.string), finalize=True)
    return get_expr()


_REGEX = re.compile(
    r'(?P<ESCAPE>\\.)|'
    r'(?P<VAREXP>\$[+\-./0-9A-Z_a-z]*\{)|'
    r'(?P<RBRACE>\})',
    re.DOTALL,
)
