from __future__ import annotations

import typing
import dataclasses
import collections.abc
import enum
import re

import archon.text_pos as _tp


# Tokenize C++ source code in accordance with a C++26 preprocessor.
#
# The input must be "newline normalized" (text mode).
#
# The C++ source code as assumed to use ASCII only.
#
def tokenize(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    return _tokenize(input_, tracker)


@dataclasses.dataclass(slots=True, frozen=True)
class Token:
    type_: TokenType
    text:  str
    pos:   int


class TokenType(enum.Enum):
    NEWLINE       = enum.auto()
    WHITESPACE    = enum.auto()
    LINE_COMMENT  = enum.auto()
    BLOCK_COMMENT = enum.auto()
    HASH_HASH     = enum.auto()
    HASH          = enum.auto()
    RAW_STRING    = enum.auto()
    STRING_LIT    = enum.auto()
    CHAR_LIT      = enum.auto()
    NUMBER        = enum.auto()
    IDENTIFIER    = enum.auto()
    PUNCT         = enum.auto()
    BAD_CHAR      = enum.auto()
    HEADER_NAME   = enum.auto()








def _basic_tokenize(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    line_iter = _logical_lines(input_, tracker)
    def consume_block_comment():
        closing_marker = "*/"
        while True:
            j = line.text.find(closing_marker, i)
            if j != -1:
                j += len(closing_marker)
                lines.append(line[i:j])
                i = j
                break
            lines.append(line[i:])
            line_obj = next(line_iter, None)
            i = 0
            if not line_obj:
                line = ""
                break
            line = line_obj.line
        yield Token(TokenType.BLOCK_COMMENT, "".join(lines), pos)
    def consume_raw_string_literal():
        ...     
    for line_obj in line_iter:
        line = line_obj.text
        nontrivial_token_seen = False
        i = 0
        while i < len(line):
            m = _TOKEN_REGEX.match(line.text, i)
            text = m.group()
            pos = i
            i += len(text)
            assert m
            group_name = m.lastgroup
            match group name:
                case "BLOCK_COMMENT":
                    consume_blok_comment()
                    continue
                case ""
            # If start of block comment:
            #   ...
            # Elif start of raw string:
            #   ...
            # If not trivial:
            #   nontrivial_token_seen = True
            # yield


# FIXME: Also handle generation of `header-name` tokens here (`<foo.h>`)    
# FIXME: When looking for the end of a raw string literal, also consume the optional UDL suffix (user defined literals)    
#
def _tokenize(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[Token]:
    line_iter = _logical_lines(input_, tracker)

    # State tracking for Preprocessor `header-name` detection
    is_first_token = True
    in_directive = False
    directive_name = None
    expect_header = False

    i = 0
    n = 0
    while True:
        if i >= n:
            line = next(line_iter, None)
            if not line:
                break
            text = line.text
            pos = line.pos
            i = 0
            n = len(text)
            continue

        if expect_header and text[i] == '<':
            end_pos = text.find('>', i)
            if end_pos != -1:
                val = text[i:end_pos+1]
                yield Token(TokenType.HEADER_NAME, val, pos + i)
                i = end_pos + 1
                expect_header = False
                is_first_token = False
                continue

        # 3. Primary Token Match
        m = _TOKEN_REGEX.match(text, i)
        assert m

        group_name = m.lastgroup
        assert group_name is not None
        token_type = TokenType[group_name]
        val = m.group()
        tok_pos = pos + i

        if token_type == TokenType.BLOCK_COMMENT:
            end_idx = text.find("*/", i + 2)
            if end_idx != -1:
                val = text[i:end_idx+2]
                yield Token(TokenType.BLOCK_COMMENT, val, tok_pos)
                i = end_idx + 2
            else:
                acc = [val]
                for next_line in line_iter:
                    end_idx = next_line.text.find("*/")
                    if end_idx != -1:
                        acc.append(next_line.text[:end_idx+2])
                        yield Token(TokenType.BLOCK_COMMENT, "".join(acc), tok_pos)
                        text = next_line.text
                        pos = next_line.pos
                        i = end_idx + 2
                        n = len(text)
                        break
                    else:
                        acc.append(next_line.text)
                else:
                    yield Token(TokenType.BLOCK_COMMENT, "".join(acc), tok_pos)
                    return
            continue

        if token_type == TokenType.RAW_STRING:
            delim_start = val.find('"') + 1
            delim = val[delim_start:-1]
            closing_marker = f"){delim}\""

            end_idx = text.find(closing_marker, i + len(val))
            if end_idx != -1:
                close_end = end_idx + len(closing_marker)

                udl_match = _IDENT_REGEX.match(text, close_end)
                if udl_match:
                    close_end = udl_match.end()

                val = text[i:close_end]
                yield Token(TokenType.RAW_STRING, val, tok_pos)
                i = close_end
            else:
                acc = [val]
                for next_line in line_iter:
                    end_idx = next_line.text.find(closing_marker)
                    if end_idx != -1:
                        close_end = end_idx + len(closing_marker)

                        udl_match = _IDENT_REGEX.match(next_line.text, close_end)
                        if udl_match:
                            close_end = udl_match.end()

                        acc.append(next_line.text[:close_end])
                        yield Token(TokenType.RAW_STRING, "".join(acc), tok_pos)
                        text = next_line.text
                        pos = next_line.pos
                        i = close_end
                        n = len(text)
                        break
                    else:
                        acc.append(next_line.text)
                else:
                    yield Token(TokenType.RAW_STRING, "".join(acc), tok_pos)
                    return
            continue

        if token_type not in (TokenType.WHITESPACE, TokenType.BLOCK_COMMENT, TokenType.LINE_COMMENT,
                              TokenType.NEWLINE):
            if is_first_token and token_type == TokenType.HASH:
                in_directive = True
            elif token_type == TokenType.IDENTIFIER:
                if in_directive and directive_name is None:
                    directive_name = val
                    if val in ("include", "import"):
                        expect_header = True
                elif in_directive and val == "__has_include":
                    expect_header = True
                else:
                    # e.g., `#include MACRO`. We hit MACRO, meaning a `<...>` should not be captured here.
                    expect_header = False
            elif token_type == TokenType.PUNCT and val == "(":
                # `__has_include ( <...> )` allows a parenthesis before the header name.
                pass
            else:
                # Any other token (numbers, stray punctuators, strings) breaks the header-name sequence
                expect_header = False

            is_first_token = False

        # 7. Reset state at new logical line
        if token_type == TokenType.NEWLINE:
            is_first_token = True
            in_directive = False
            directive_name = None
            expect_header = False

        if token_type == TokenType.STRING_LIT and expect_header:
            token_type = TokenType.HEADER_NAME
            expect_header = False

        yield Token(token_type, val, tok_pos)
        i += len(val)


_UCN_REGEX_STRING = r"(?:\\u[0-9A-Fa-f]{4}|\\u\{[0-9A-Fa-f]+\}|\\U[0-9A-Fa-f]{8}|\\N\{[^}\n]+\})"

_IDENT_REGEX_STRING = r"(?:(?:[A-Za-z_]|%s)(?:\w+|%s)*)" % (_UCN_REGEX_STRING, _UCN_REGEX_STRING)

_TOKEN_REGEX = re.compile("|".join("(?P<%s>%s)" % (name, expr) for name, expr in [
    ("NEWLINE",       r"\n"),
    ("WHITESPACE",    r"[ \t\f\v]+"),
    ("LINE_COMMENT",  r"//[^\n]*"),
    ("BLOCK_COMMENT", r"/\*"),
    ("HASH_HASH",     r"##|%:%:"),
    ("HASH",          r"#|%:"),
    ("RAW_STRING",    r'(?:u8|u|U|L)?R"[^()\\\s]*\('),
    ("STRING_LIT",    r'(?:u8|u|U|L)?"(?:\\.|[^"\\\n])*("(?:%s)?|$)' % _IDENT_REGEX_STRING),
    ("CHAR_LIT",      r"(?:u8|u|U|L)?'(?:\\.|[^'\\\n])*('(?:%s)?|$)" % _IDENT_REGEX_STRING),
    ("NUMBER",        r"(?:\d|\.\d)(?:\.|[eEpP][+-]|%s|\'?\w)*" % _UCN_REGEX_STRING),
    ("IDENTIFIER",    _IDENT_REGEX_STRING),
    ("PUNCT",         (r"::|\.\.\.|->\*|->|\+\+|--|<<=|>>=|<<|>>|<=>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|\/=|%=|&=|\^=|\|=|"
                       r"\.\*|<%|%>|<:(?:(?!:)|(?=::|:>))|:>|[{}()\[\];,.?:+\-*%^&|~!=<>]")),
    ("BAD_CHAR",      r"."),
]), re.ASCII | re.DOTALL)

_IDENT_REGEX = re.compile(_IDENT_REGEX_STRING, re.ASCII)


def _logical_lines(input_: typing.TextIO, tracker: _tp.TextPosTracker) -> collections.abc.Iterator[_Line]:
    pos = 0
    lines = list[str]()
    for line in input_:
        continuation = "\\\n"
        if line.endswith(continuation):
            i = len(line) - len(continuation)
            lines.append(line[:i])
            tracker.advance(i)
            tracker.new_line()
        else:
            lines.append(line)
            tracker.advance(len(line))
            tracker.new_line()
            yield _Line(pos, "".join(lines))
            lines.clear()
            pos = tracker.current()
    if lines:
        yield _Line(pos, "".join(lines))


@dataclasses.dataclass(slots=True, frozen=True)
class _Line:
    pos:  int
    text: str
