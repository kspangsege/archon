from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_tokenizer as _ct


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

try:
    with open(path) as file_:
        tracker = _tp.TextPosTracker()
        for token in _ct.tokenize(file_, tracker):
            text_pos = tracker.get_text_pos(token.pos)
            logger.info("%s:%s: %s: %s", text_pos.line_no, text_pos.pos_on_line, token.type_.name,
                        _b.clamped_quote(token.text, 64))
except FileNotFoundError as e:
    logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
    sys.exit(1)
