from __future__ import annotations
from typing import override

import sys
import pathlib

import archon.log as _l
import archon.command_line_interface as _cli
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.process as _cp


spec = _cli.Spec()
spec.stop_opt(["--"])
spec.short_circuit_opt(["-h", "--help"])
spec.opt_with_arg(["-l", "--log-level"], _l.parse_log_level, _l.LogLevel.INFO)

root_logger = _l.RootLogger()
result = _cli.Result()
if not _cli.parse(sys.argv[1:], spec, result, root_logger):
    sys.exit(1)
if result.get_opt("--help"):
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(result.args) < 1:
    root_logger.error("Too few command-line arguments (try --help)")
    sys.exit(1)

cmake_path = pathlib.Path(result.args[0])
log_level = result.get_opt("--log-level")

pos_resolver = _cp.PositionResolver()
logger = _l.LimitLogger(root_logger, log_level)

class Application(_cp.Application):
    @override
    def message(self, pos: _cur.Position, occurrence_uncertainty: _cp.OccurrenceUncertainty, level: _cp.MessageLevel,
                message: str) -> None:
        certainty = "Uncertain" if occurrence_uncertainty else "Certain"
        context = pos_resolver.resolve_file_context(pos)
        context_logger = _l.FileContextLogger(logger, context)
        context_logger.info("%s: Message(%s): %s", certainty, level.name, message)

application = Application()
_cp.process(cmake_path, application, pos_resolver, logger)
