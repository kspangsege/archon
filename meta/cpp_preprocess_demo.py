from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_preprocess as _cp


no_serialize = _b.Wrap(False)
log_level    = _b.Wrap(_l.LogLevel.INFO)
help_        = _b.Wrap(False)

spec = _cli.Spec()
spec.opt(["-s", "--no-serialize"], _cli.Raise(no_serialize))
spec.opt(["-l", "--log-level"], _cli.AssignWithArg(_l.parse_log_level, log_level))
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["--"], _cli.Stop())


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
        macro_registry = dict[str, _cp.MacroDef]()
        def error_handler(pos: _cp.Position, message: str, *args: typing.Any) -> None:
            assert pos.file_index == 0
            text_pos = tracker.get_text_pos(pos.pos_in_file)
            context = _l.FileContext(path, _l.FullTextPos(text_pos.line_no, text_pos.pos_on_line))
            _l.FileContextLogger(logger, context).error(message, *args)
        tokens_iter = _cp.preprocess(input_ = file_, file_index = 0, tracker = tracker,
                                     macro_registry = macro_registry, error_handler = error_handler)
        if no_serialize.value:
            for token in tokens_iter:
                pos = token.pos
                assert pos.file_index == 0
                text_pos = tracker.get_text_pos(pos.pos_in_file)
                string = _b.clamped_quote(token.text, 64)
                if token.synthetic:
                    string += " (synthetic)"
                logger.info("%s:%s: %s: %s", text_pos.line_no, text_pos.pos_on_line, token.type_.name, string)
        else:
            parts = list[str]()
            def flush() -> None:
                string = "".join([p for p in parts])
                print(string)
                parts.clear()
            for token in tokens_iter:
                if token.type_ is _cp.TokenType.END_OF_INPUT:
                    if parts:
                        flush()
                    break
                if token.type_ is _cp.TokenType.NEWLINE:
                    flush()
                    continue
                parts.append(token.text)
except FileNotFoundError as e:
    logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
    sys.exit(1)
