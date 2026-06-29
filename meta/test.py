from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.process as _cp


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

cmake_path = pathlib.Path(args[0])

pos_resolver = _cp.PositionResolver()
logger = _l.LimitLogger(root_logger, log_level.value)

class Application(_cp.Application):
    @typing.override
    def message(self, pos: _cur.Position, occurrence_uncertainty: _cp.OccurrenceUncertainty, level: _cp.MessageLevel,
                message: str) -> None:
        certainty = "Uncertain" if occurrence_uncertainty else "Certain"
        context = pos_resolver.resolve_file_context(pos)
        context_logger = _l.FileContextLogger(logger, context)
        context_logger.info("%s: Message(%s): %s", certainty, level.name, message)

application = Application()
if not _cp.process_file(cmake_path, application, pos_resolver, logger):
    sys.exit(1)
