from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_preprocess as _cp


MACRO_NAME = "CM_FOR_EACH_POLICY_TABLE"
POLICY_PARAM_NAME = "POLICY"
SELECT_PARAM_NAME = "SELECT"


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

tracker = _tp.TextPosTracker()
def error_handler(pos: int, message: str, *args: typing.Any) -> None:
    text_pos = tracker.get_text_pos(pos)
    context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
    _l.FileContextLogger(logger, context).error(message, *args)
    raise Error from None

def process(tokens: list[_cp.Token]) -> None:
    assert False    

try:
    with open(path) as file_:
        initial = True
        for elem in _cp.parse(file_, tracker, error_handler):
            if not isinstance(elem, _cp.DefineDirective) or elem.name != MACRO_NAME:
                continue
            define = elem
            if define.is_variadic:
                error_handler(define.pos, "Unexpected variadic macro")
                sys.exit(1)
            if define.params != [POLICY_PARAM_NAME, SELECT_PARAM_NAME]:
                error_handler(define.pos, "Unexpected macro parameters")
                sys.exit(1)
            process(define.replacement)
            break
except FileNotFoundError as e:
    logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
    sys.exit(1)
except Error:
    sys.exit(1)
