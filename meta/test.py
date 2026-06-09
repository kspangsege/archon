import sys
import pathlib

import archon.log as _l
import archon.command_line_interface as _cli
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

logger = _l.LimitLogger(root_logger, log_level)
_cp.process(cmake_path, logger)
