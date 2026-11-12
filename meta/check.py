import re
import sys
import pathlib

import base
import log
import command_line_interface as _cli
import inclusion_cache as _ic
import cmake
import project
import check_target_dependencies as _ctd


root_path = base.resolve_self_rel_path(sys.argv[0], "..")
src_path = root_path / "src"


spec = _cli.Spec()
spec.stop_opt(["--"])
spec.short_circuit_opt(["-h", "--help"])
spec.opt_with_arg(["-l", "--log-level"], log.parse_log_level, log.LogLevel.INFO)
spec.opt(["--dump-domain-structure"])
spec.opt(["--dump-group-structure"])
spec.opt(["--emit-cmake-messages"])

root_logger = log.RootLogger()
result = _cli.Result()
if not _cli.parse(sys.argv[1:], spec, result, root_logger):
    sys.exit(1)
if result.get_opt("--help"):
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(result.args) < 1:
    root_logger.error("Too few command-line arguments (try --help)")
    sys.exit(1)

function = result.args[0]
log_level = result.get_opt("--log-level")
dump_domain_structure = result.get_opt("--dump-domain-structure")
dump_group_structure = result.get_opt("--dump-group-structure")
emit_cmake_messages = result.get_opt("--emit-cmake-messages")


logger = log.LimitLogger(root_logger, log_level)
def extract_domain_structure() -> project.Structure | None:
    root_group = cmake.extract_structure(root_path, logger, emit_cmake_messages)
    if not root_group:
        return None
    if dump_group_structure:
        cmake.dump_structure(root_group, logger, log.LogLevel.INFO)
    def include_order_resolver(cmake_path: pathlib.Path) -> pathlib.Path | None:
        m = re.fullmatch(r"(.*)_library.cmake", cmake_path.name)
        if not m:
            return None
        library_dir_name = m.group(1)
        return cmake_path.parent / ("%s_include_order.txt" % library_dir_name)
    structure = project.discover_domain_structure(root_group, src_path, logger, include_order_resolver)
    if not structure:
        return None
    if dump_domain_structure:
        project.dump_domain_structure(structure, logger, log.LogLevel.INFO)
    return structure


if function == "target-dependencies":
    inclusion_cache = _ic.InclusionCache(src_path, logger)
    structure = extract_domain_structure()
    if not structure:
        sys.exit(1)
    extra_dependencies = [
        ("Test", "CoreTest"),
        ("Test", "LogTest"),
        ("Test", "CliTest"),
        ("Test", "CheckTest"),
        ("Test", "MathTest"),
        ("Test", "UtilTest"),
        ("Test", "ImageTest"),
        ("Test", "FontTest"),
        ("Test", "DisplayTest"),
    ]
    try:
        success = _ctd.check(inclusion_cache, structure, extra_dependencies, logger)
        if not success:
            sys.exit(1)
    except _ic.InclusionException:
        sys.exit(1)
    sys.exit(0)


root_logger.error("Unknown function %s (try --help)", base.quote(function))
sys.exit(1)
