from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.parsing.ecfg as _pe


help_           = _b.Wrap(False)
log_level       = _b.Wrap(_l.LogLevel.INFO)
format_grammar  = _b.Wrap(False)
analyze_as_ell1 = _b.Wrap(False)

spec = _cli.Spec()
spec.opt(["--"], _cli.Stop())
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["-l", "--log-level"], _cli.AssignWithArg(log_level, _l.parse_log_level))
spec.opt(["-f", "--format-grammar"], _cli.Raise(format_grammar))
spec.opt(["-a", "--analyze-as-ell1"], _cli.Raise(analyze_as_ell1))

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
try:
    with open(path, "r") as file_:
        def error_handler(pos: _tp.FullTextPos, message: str, *args: typing.Any) -> None:
            context = _tp.FileContext(path, pos)
            _l.FileContextLogger(logger, context).error(message, *args)
        grammar = _pe.parse(file_, error_handler)
except FileNotFoundError as e:
    logger.error("Failed to parse %s: %s", _b.quote(str(path)), e.strerror)
    sys.exit(1)

if not grammar:
    sys.exit(1)

if format_grammar.value:
    text = _pe.format_grammar(grammar)
    sys.stdout.write(text)

if analyze_as_ell1.value:
    is_ell1 = _pe.analyze_as_ell1(grammar)
    if not is_ell1:
        logger.warn("Grammar is not ELL(1)")
