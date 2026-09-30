from __future__ import annotations

import textwrap
import io
import unittest

import archon.text_pos as _tp
import archon.test as _t
import archon.cpp_preprocess as _cp


def test_CppPreprocess_Tokenize_Basics(context: _t.Context) -> None:
    _check_valid_tokenize(context, "", [])
    _check_valid_tokenize(context, "\n", [
        (_cp.TokenType.NEWLINE, "\n"),
    ])
    _check_valid_tokenize(context, " ", [
        (_cp.TokenType.WHITESPACE, " "),
    ])
    _check_valid_tokenize(context, "// foo", [
        (_cp.TokenType.LINE_COMMENT, "// foo"),
    ])
    _check_valid_tokenize(context, _trim_cpp_text(r"""
      /*
      foo
      */
    """), [
        (_cp.TokenType.BLOCK_COMMENT, "/*\nfoo\n*/"),
        (_cp.TokenType.NEWLINE, "\n"),
    ])
    _check_valid_tokenize(context, "#", [
        (_cp.TokenType.HASH, "#"),
    ])
    _check_valid_tokenize(context, "##", [
        (_cp.TokenType.HASH_HASH, "##"),
    ])
    _check_valid_tokenize(context, _trim_cpp_text(r"""
      R"(
      foo
      )"
    """), [
        (_cp.TokenType.RAW_STRING_LIT, 'R"(\nfoo\n)"'),
        (_cp.TokenType.NEWLINE, "\n"),
    ])
    _check_valid_tokenize(context, '"foo"', [
        (_cp.TokenType.STRING_LIT, '"foo"'),
    ])
    _check_valid_tokenize(context, "'f'", [
        (_cp.TokenType.CHAR_LIT, "'f'"),
    ])
    _check_valid_tokenize(context, "7", [
        (_cp.TokenType.NUMBER, "7"),
    ])
    _check_valid_tokenize(context, "foo", [
        (_cp.TokenType.IDENTIFIER, "foo"),
    ])
    _check_valid_tokenize(context, "+=", [
        (_cp.TokenType.PUNCT, "+="),
    ])
    _check_valid_tokenize(context, "$", [
        (_cp.TokenType.STRAY_CHAR, "$"),
    ])
    _check_valid_tokenize(context, _trim_cpp_text(r"""
      x /* foo */ 7
      ("bar")
    """), [
        (_cp.TokenType.IDENTIFIER, "x"),
        (_cp.TokenType.WHITESPACE, " "),
        (_cp.TokenType.BLOCK_COMMENT, "/* foo */"),
        (_cp.TokenType.WHITESPACE, " "),
        (_cp.TokenType.NUMBER, "7"),
        (_cp.TokenType.NEWLINE, "\n"),
        (_cp.TokenType.PUNCT, "("),
        (_cp.TokenType.STRING_LIT, '"bar"'),
        (_cp.TokenType.PUNCT, ")"),
        (_cp.TokenType.NEWLINE, "\n"),
    ])








def _trim_cpp_text(text: str) -> str:
    return textwrap.dedent(text.removeprefix("\n"))


type ExpectedToken = tuple[_cp.TokenType, str]


def _check_valid_tokenize(context: _t.Context, cpp_text: str, expected_tokens: list[ExpectedToken]) -> None:
    with io.StringIO(cpp_text) as input_:
        file_index = 0
        tracker = _tp.TextPosTracker()
        tokens = list(_cp.tokenize(input_, file_index, tracker))
    _check_tokens(context, tokens, expected_tokens)


def _check_tokens(context: _t.Context, tokens: list[_cp.Token], expected_tokens: list[ExpectedToken]) -> None:
    context.check(bool(tokens))
    context.check_equal(tokens[-1].type_, _cp.TokenType.END_OF_INPUT)
    tokens_2 = tokens[:-1]
    context.check_equal(len(tokens_2), len(expected_tokens))
    for i, (token, expected) in enumerate(zip(tokens_2, expected_tokens)):
        subcontext = context.subcontext(1 + i)
        subcontext.check_equal(token.type_, expected[0])
        subcontext.check_equal(token.text, expected[1])


# Bridge to Python's native testing framework
def load_tests(loader: unittest.TestLoader, standard_tests: unittest.TestSuite,
               pattern: str | None) -> unittest.TestSuite:
    return _t.generate_native_tests(__name__)


if __name__ == '__main__':
    _t.run_module_tests(__name__)
