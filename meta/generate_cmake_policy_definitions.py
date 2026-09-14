from __future__ import annotations

import typing
import collections.abc
import sys
import pathlib
import io

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_preprocess as _cp


MACRO_NAME = "CM_FOR_EACH_POLICY_TABLE"
MACRO_PARAMS = ["POLICY", "SELECT"]
SUBMACRO_NAME = "SELECT_POLICY_ARGS"
SUBMACRO_PARAMS = ["dummy", "policy_ident", "description", "major", "minor", "patch", "status"]
POLICY_DEFINER_NAME = "DEFINE_POLICY"
POLICY_DEFINER_PARAMS = ["policy_ident", "description", "major", "minor", "patch"]
ROOT_TEXT     = "%s(, %s)" % (MACRO_NAME, SUBMACRO_NAME)
SUBMACRO_TEXT = "%s(%s)" % (POLICY_DEFINER_NAME, ", ".join(POLICY_DEFINER_PARAMS))

assert set(POLICY_DEFINER_PARAMS).issubset(SUBMACRO_PARAMS)


help_     = _b.Wrap(False)
log_level = _b.Wrap(_l.LogLevel.INFO)

spec = _cli.Spec()
spec.opt(["--"], _cli.Stop())
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["-l", "--log-level"], _cli.AssignWithArg(_l.parse_log_level, log_level))


root_logger = _l.RootLogger()
success, args = _cli.parse(sys.argv[1:], spec, root_logger)
if not success:
    sys.exit(1)
if help_.value:
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(args) != 1:
    root_logger.error("Wrong number of command-line arguments (try --help)")
    sys.exit(1)

path = pathlib.Path(args[0])

logger = _l.LimitLogger(root_logger, log_level.value)

class Error(Exception):
    pass

main_tracker = _tp.TextPosTracker()
def error_handler(pos: int, message: str, *args: typing.Any) -> None:
    text_pos = main_tracker.get_text_pos(pos)
    context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
    _l.FileContextLogger(logger, context).error(message, *args)
    raise Error from None

def tokenize_snippet(text: str) -> list[_cp.Token]:
    tracker = _tp.TextPosTracker()
    return list(_cp.tokenize(io.StringIO(text), tracker))

class ScanContext(_cp.ScanContext):
    def __init__(self, tokens: list[_cp.Token], macro_registry: dict[str, _cp.MacroDef]) -> None:
        self._tokens         = iter(tokens)
        self._macro_registry = macro_registry
    @typing.override
    def next_token(self) -> _cp.Token | None:
        return next(self._tokens, None)
    @typing.override
    def lookup_macro(self, name: str) -> _cp.MacroDef | None:
        return self._macro_registry.get(name)
    @typing.override
    def handle_macro_invoc(self, name: str, definition: _cp.MacroDef, arguments: list[list[_cp.Token]] | None,
                           va_args: list[_cp.Token] | None) -> None:
        assert False    

try:
    with open(path) as file_:
        tokens = _cp.tokenize(file_, main_tracker)
        initial = True
        for elem in _cp.parse(tokens, error_handler):
            if not isinstance(elem, _cp.DefineDirective) or elem.name != MACRO_NAME:
                continue
            define = elem
            if define.is_variadic:
                error_handler(define.pos, "Unexpected variadic macro")
                sys.exit(1)
            if define.params != MACRO_PARAMS:
                error_handler(define.pos, "Unexpected macro parameters")
                sys.exit(1)
            submacro_tokens = tokenize_snippet(SUBMACRO_TEXT)
            registry = {
                MACRO_NAME:    _cp.MacroDef(MACRO_PARAMS, False, define.replacement),
                SUBMACRO_NAME: _cp.MacroDef(SUBMACRO_PARAMS, False, submacro_tokens),
            }
            output = list(_cp.preprocess(tokenize_snippet(ROOT_TEXT), registry, error_handler))
            registry = {
                POLICY_DEFINER_NAME: _cp.MacroDef(POLICY_DEFINER_PARAMS, False, []),
            }
            context = ScanContext(output, registry)
            for _ in _cp.scan(context, error_handler):
                pass
            break
except FileNotFoundError as e:
    logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
    sys.exit(1)
except Error:
    sys.exit(1)
