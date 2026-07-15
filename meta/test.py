from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.process as _cp


help_      = _b.Wrap(False)
binary_dir = _b.Wrap(typing.cast(str | None, None))
cmake_path = _b.Wrap(typing.cast(str | None, None))
log_level  = _b.Wrap(_l.LogLevel.INFO)

spec = _cli.Spec()
spec.opt(["--"], _cli.Stop())
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["-b", "--binary-dir"], _cli.AssignWithArg(str, binary_dir))
spec.opt(["-p", "--cmake-path"], _cli.AssignWithArg(str, cmake_path))
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

source_dir = pathlib.Path(args[0])

abs_base_path = pathlib.Path.cwd()
pos_resolver = _cp.PositionResolver()
logger = _l.LimitLogger(root_logger, log_level.value)
application = _cp.SimpleApplication(abs_base_path, pos_resolver, logger)
config = _cp.Config()

if binary_dir.value is not None:
    config.binary_dir = pathlib.Path(binary_dir.value)

if cmake_path.value is not None:
    cmake_path_2 = pathlib.Path(cmake_path.value)
else:
    cmake_path_2 = source_dir / "CMakeLists.txt"

try:
    with application.open_subfile(cmake_path_2) as file_:
        source = _cp.Source(file_, cmake_path_2)
        if not _cp.process(source_dir, source, application, pos_resolver, config):
            sys.exit(1)
except FileNotFoundError as e:
    logger.error("Failed to process %s: %s", _b.quote(str(cmake_path)), e.strerror)
    sys.exit(1)
