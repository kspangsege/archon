from __future__ import annotations

import typing
import sys
import pathlib

import archon.base as _b
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cmake.uncertainty_reason as _cur
import archon.cmake.version as _cve
import archon.cmake.process as _cp


help_             = _b.Wrap(False)
source_dir        = _b.Wrap(".")
binary_dir        = _b.Wrap(typing.cast(str | None, None))
cmake_path        = _b.Wrap(typing.cast(str | None, None))
lenient_mode      = _b.Wrap(False)
suppress_messages = _b.Wrap(False)
cmake_version     = _b.Wrap(_cp.CMAKE_VERSION)
log_level         = _b.Wrap(_l.LogLevel.INFO)
breakpoint_       = _b.Wrap(False)

initial_variables = dict[str, str | None]()

def parse_assignment(string: str) -> tuple[str, str]:
    i = string.find("=")
    if i >= 0:
        return string[:i], string[i+1:]
    raise ValueError

def set_var(arg: tuple[str, str]) -> None:
    name, value = arg
    initial_variables[name] = value

def unset_var(name: str) -> None:
    initial_variables[name] = None

def parse_version(string: str) -> _cve.Version:
    version = _cve.parse(string)
    if _cp.LOWEST_SUPPORTED_CMAKE_VERSION <= version <= _cp.CMAKE_VERSION:
        return version
    raise ValueError

spec = _cli.Spec()
spec.opt(["--"], _cli.Stop())
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["-s", "--source-dir"], _cli.AssignWithArg(str, source_dir))
spec.opt(["-b", "--binary-dir"], _cli.AssignWithArg(str, binary_dir))
spec.opt(["-p", "--cmake-path"], _cli.AssignWithArg(str, cmake_path))
spec.opt(["-S", "--set-var"], _cli.CallWithArg(parse_assignment, set_var))
spec.opt(["-U", "--unset-var"], _cli.CallWithArg(str, unset_var))
spec.opt(["-L", "--lenient-mode"], _cli.Raise(lenient_mode))
spec.opt(["-M", "--suppress-messages"], _cli.Raise(suppress_messages))
spec.opt(["-v", "--cmake-version"], _cli.AssignWithArg(parse_version, cmake_version))
spec.opt(["-l", "--log-level"], _cli.AssignWithArg(_l.parse_log_level, log_level))
spec.opt(["-B", "--breakpoint"], _cli.Raise(breakpoint_))


root_logger = _l.RootLogger()
success, args = _cli.parse(sys.argv[1:], spec, root_logger)
if not success:
    sys.exit(1)
if help_.value:
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(args) != 0:
    root_logger.error("Wrong number of command-line arguments (try --help)")
    sys.exit(1)

abs_base_path = pathlib.Path.cwd()
pos_resolver = _cp.PositionResolver()
logger = _l.LimitLogger(root_logger, log_level.value)
application = _cp.SimpleApplication(abs_base_path, pos_resolver, logger)
config = _cp.Config()
config.source_dir = pathlib.Path(source_dir.value)
config.lenient_mode = lenient_mode.value
config.suppress_messages = suppress_messages.value
config.cmake_version = cmake_version.value
config.initial_variables = initial_variables
config.define_breakpoint_command = breakpoint_.value

if binary_dir.value is not None:
    config.binary_dir = pathlib.Path(binary_dir.value)

if cmake_path.value is not None:
    cmake_path_2 = pathlib.Path(cmake_path.value)
else:
    cmake_path_2 = config.source_dir / "CMakeLists.txt"

try:
    with application.open_subfile(cmake_path_2) as file_:
        source = _cp.Source(file_, cmake_path_2)
        if not _cp.process(source, application, pos_resolver, config):
            sys.exit(1)
except FileNotFoundError as e:
    logger.error("Failed to process %s: %s", _b.quote(str(cmake_path_2)), e.strerror)
    sys.exit(1)
