from __future__ import annotations
from typing import Any

import sys
import pathlib

import archon.log as _l
import archon.command_line_interface as _cli
import archon.parsing.ecfg as _pe


spec = _cli.Spec()
spec.stop_opt(["--"])
spec.short_circuit_opt(["-h", "--help"])
spec.opt_with_arg(["-l", "--log-level"], _l.parse_log_level, _l.LogLevel.INFO)
spec.opt(["-f", "--format-grammar"])
spec.opt(["-a", "--analyze-as-ell1"])

root_logger = _l.RootLogger()
result = _cli.Result()
if not _cli.parse(sys.argv[1:], spec, result, root_logger):
    sys.exit(1)
if result.get_opt("--help"):
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(result.args) != 1:
    root_logger.error("Wrong number of command-line arguments (try --help)")
    sys.exit(1)

path = pathlib.Path(result.args[0])
log_level = result.get_opt("--log-level")
format_grammar = result.get_opt("--format-grammar")
analyze_as_ell1 = result.get_opt("--analyze-as-ell1")

logger = _l.LimitLogger(root_logger, log_level)
def error_handler(pos: _l.FullFilePos, message: str, *args: Any) -> None:
    context = _l.FileContext(path, pos)
    _l.FileContextLogger(logger, context).error(message, *args)
grammar = _pe.parse(path, error_handler)
if not grammar:
    sys.exit(1)
if format_grammar:
    text = _pe.format_grammar(grammar)
    sys.stdout.write(text)
if analyze_as_ell1:
    is_ell1 = _pe.analyze_as_ell1(grammar)
    if not is_ell1:
        logger.warn("Grammar is not ELL(1)")
