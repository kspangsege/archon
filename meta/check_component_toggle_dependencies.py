import re
import pathlib
import sys

import utils


cmake_path = utils.resolve_self_rel_path(sys.argv[0], "../cmake")
src_path = utils.resolve_self_rel_path(sys.argv[0], "../src")

targets_cache = utils.TargetsCache(src_path)
libraries = targets_cache.discover_libraries()


expected_lib_dependencies = {}
for lib in libraries:
    expected = expected_lib_dependencies.setdefault(lib.cmake_name, set())
    dependencies, closing_line_no = targets_cache.get_library_dependencies(lib)
    expected.update(dep.cmake_name for dep, line_no in dependencies)
    if (src_path / lib.tool_dir_subpath).exists():
        for target in targets_cache.get_targets(lib.tool_cmake_subpath):
            assert isinstance(target, utils.ExecutableTarget)
            for dep, line_no in target.dependencies:
                if dep.cmake_name != lib.cmake_name:
                    expected.add(dep.cmake_name)

expected_demo_dependencies = {}
for lib in libraries:
    expected = expected_demo_dependencies.setdefault(lib.cmake_name, set())
    if (src_path / lib.probe_dir_subpath).exists():
        for target in targets_cache.get_targets(lib.probe_cmake_subpath):
            assert isinstance(target, utils.ExecutableTarget)
            for dep, line_no in target.dependencies:
                if dep.cmake_name != lib.cmake_name:
                    expected.add(dep.cmake_name)
    if (src_path / lib.demo_dir_subpath).exists():
        for target in targets_cache.get_targets(lib.demo_cmake_subpath):
            assert isinstance(target, utils.ExecutableTarget)
            for dep, line_no in target.dependencies:
                if dep.cmake_name != lib.cmake_name:
                    expected.add(dep.cmake_name)

expected_test_subsuite_dependencies = {}
for lib in libraries:
    expected = expected_test_subsuite_dependencies.setdefault(lib.cmake_name, set())
    if (src_path / lib.test_dir_subpath).exists():
        targets = targets_cache.get_targets(lib.test_cmake_subpath)
        assert len(targets) == 1
        target = targets[0]
        assert isinstance(target, utils.LibraryTarget)
        for dep, line_no in target.dependencies:
            if dep.cmake_name != lib.cmake_name:
                expected.add(dep.cmake_name)


expected_test_executor_dependencies = set()
targets = targets_cache.get_targets(pathlib.Path("CMakeLists.txt"))
assert len(targets) == 1
target = targets[0]
assert isinstance(target, utils.ExecutableTarget)
for dep, line_no in target.dependencies:
    if dep.cmake_name != lib.cmake_name:
        expected_test_executor_dependencies.add(dep.cmake_name)


dependencies_path = cmake_path / "dependencies.cmake"
def error(line_no, message, *args):
    prefix = "%s:%s: " % (dependencies_path, line_no) if line_no is not None else "%s: " % dependencies_path
    print("%sERROR: %s" % (prefix, (message % args)))

with open(dependencies_path, "r") as f:
    line_no = 1
    while True:
        line = f.readline()
        if not line:
            break
        line = line.rstrip("\n")
        m = re.fullmatch(r'set\(ARCHON_(LIB|DEMO|TEST)_DEPS(_(\w+))?\s+(""|([\s\w]+))\)\s*', line)
        if m:
            category = m.group(1)
            lib_name = m.group(3)
            value    = m.group(5)
            assert category == "TEST" or lib_name is not None

            if category == "LIB":
                expected = expected_lib_dependencies[lib_name]
                prefix = lib_name
            elif category == "DEMO":
                expected = expected_demo_dependencies[lib_name]
                prefix = "Demo apps of %s" % lib_name
            elif lib_name is not None:
                expected = expected_test_subsuite_dependencies[lib_name]
                prefix = "Test sub-suite of %s" % lib_name
            else:
                expected = expected_test_executor_dependencies
                prefix = "Test executor"

            actual = value.split() if value is not None else []
            actual_as_set = set(actual)
            for lib in libraries:
                is_expected = lib.cmake_name in expected
                is_actual   = lib.cmake_name in actual_as_set
                if is_expected and not is_actual:
                    error(line_no, "%s: Missing dependency %s", prefix, lib.cmake_name)
                if not is_expected and is_actual:
                    error(line_no, "%s: Superfluous dependency %s", prefix, lib.cmake_name)

        line_no += 1
