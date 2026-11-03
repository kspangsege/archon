import collections
import re
import pathlib
import sys

import log
import cmake
import project


def resolve_self_rel_path(argv0, rel_path):
    self_path = pathlib.Path(argv0)
    assert self_path.exists()
    path = self_path.parent / rel_path
    return path.resolve().relative_to(pathlib.Path.cwd())


def find_files(path, suffixes, subdir_filter = None):
    subpaths = []
    def visit(subpath):
        for path_2 in (path / subpath).iterdir():
            subpath_2 = subpath / path_2.name
            if path_2.is_file():
                if subpath_2.suffix in suffixes:
                    subpaths.append(subpath_2)
            elif path_2.is_dir():
                if not subdir_filter or subdir_filter(subpath_2):
                    visit(subpath_2)
    visit(pathlib.Path())
    return subpaths


def lib_dirname_from_subpath(subpath):
    parts = subpath.parts
    assert len(parts) >= 2
    assert parts[0] == "archon"
    if len(parts) > 2:
        return parts[1]
    assert subpath.suffix == ".hpp"
    return subpath.stem


class Library:
    def __init__(self, cmake_name, dir_name):
        self.cmake_name = cmake_name
        self.dir_name = dir_name
        self.cmake_subpath = pathlib.Path("%s_library.cmake" % dir_name)
        self.incl_subpath = pathlib.Path("archon/%s.hpp" % dir_name)
        self.dir_subpath = pathlib.Path("archon/%s" % dir_name)
        self.tool_dir_subpath = self.dir_subpath / "tool"
        self.tool_cmake_subpath = self.tool_dir_subpath / "CMakeLists.txt"
        self.probe_dir_subpath = self.dir_subpath / "probe"
        self.probe_cmake_subpath = self.probe_dir_subpath / "CMakeLists.txt"
        self.demo_dir_subpath = self.dir_subpath / "demo"
        self.demo_cmake_subpath = self.demo_dir_subpath / "CMakeLists.txt"
        self.test_dir_subpath = self.dir_subpath / "test"
        self.test_cmake_subpath = self.test_dir_subpath / "CMakeLists.txt"
        self.test_cmake_name = "%sTest" % cmake_name


class Dependency:
    def __init__(self, cmake_name, type_):
        self.cmake_name = cmake_name
        self.type_ = type_


class ExecutableTarget:
    def __init__(self, cmake_name):
        self.cmake_name = cmake_name
        self.source_files = []
        self.source_files_closing_line_no = None
        self.dependencies = []
        self.dependencies_closing_line_no = None

class LibraryTarget:
    def __init__(self, cmake_name, type_):
        self.cmake_name = cmake_name
        self.type_ = type_
        self.nonheader_source_files = []
        self.nonheader_source_files_closing_line_no = None
        self.dependencies = []
        self.dependencies_closing_line_no = None
        self.installed_header_files = []
        self.installed_header_files_closing_line_no = None


class TargetsCache:
    def __init__(self, src_path):
        self._src_path = src_path
        self._cache = {}

    def discover_libraries(self):
        libraries = []
        for subpath, line_no in self._get(pathlib.Path("CMakeLists.txt")).includes:
            m = re.fullmatch(r"(\w+)_library.cmake", subpath.as_posix())
            if not m:
                continue
            dir_name = m.group(1)
            targets = self.get_targets(subpath)
            assert len(targets) == 1
            target = targets[0]
            assert isinstance(target, LibraryTarget)
            library = Library(target.cmake_name, dir_name)
            libraries.append(library)
        return libraries

    def get_library_nonheader_source_files(self, lib):
        targets = self.get_targets(lib.cmake_subpath)
        assert len(targets) == 1
        target = targets[0]
        assert isinstance(target, LibraryTarget)
        assert target.cmake_name == lib.cmake_name
        return [ subpath for subpath, line_no in target.nonheader_source_files ]

    def get_library_dependencies(self, lib):
        targets = self.get_targets(lib.cmake_subpath)
        assert len(targets) == 1
        target = targets[0]
        assert isinstance(target, LibraryTarget)
        assert target.cmake_name == lib.cmake_name
        return target.dependencies, target.dependencies_closing_line_no

    def get_library_installed_header_files(self, lib):
        targets = self.get_targets(lib.cmake_subpath)
        assert len(targets) == 1
        target = targets[0]
        assert isinstance(target, LibraryTarget)
        assert target.cmake_name == lib.cmake_name
        return [ subpath for subpath, line_no in target.installed_header_files ]

    def get_targets(self, cmake_subpath):
        return self._get(cmake_subpath).targets

    class _Entry:
        def __init__(self, includes, targets):
            self.includes = includes
            self.targets = targets

    def _get(self, cmake_subpath):
        entry = self._cache.get(cmake_subpath)
        if not entry:
            entry = self._parse(cmake_subpath)
            self._cache[cmake_subpath] = entry
        return entry

    def _parse(self, cmake_subpath):
        includes = []
        targets = []
        cmake_path = self._src_path / cmake_subpath
        def fatal(line_no, message, *args):
            prefix = "%s:%s: " % (cmake_path, line_no) if line_no is not None else "%s: " % cmake_path
            print("%sFATAL: %s" % (prefix, (message % args)))
            sys.exit(1)
        target = None
        current_dep_type = None
        def process_add_library_contents(line_no, contents):
            for word in contents.split():
                if re.fullmatch(r"[\w/]+\.cpp", word):
                    subpath = cmake_subpath.parent / word
                    target.nonheader_source_files.append((subpath, line_no))
                    continue
                fatal(line_no, "Unrecognized non-header source file path")
        def process_add_executable_contents(line_no, contents):
            for word in contents.split():
                if re.fullmatch(r"[\w/]+\.cpp", word):
                    subpath = cmake_subpath.parent / word
                    target.source_files.append((subpath, line_no))
                    continue
                fatal(line_no, "Unrecognized source file path")
        def process_target_link_libraries_contents(line_no, contents):
            nonlocal current_dep_type
            for word in contents.split():
                if word in [ "PRIVATE", "PUBLIC", "INTERFACE" ]:
                    current_dep_type = word
                    continue
                if re.fullmatch(r"\w+", word):
                    if current_dep_type:
                        dep = Dependency(word, current_dep_type)
                        target.dependencies.append((dep, line_no))
                        continue
                    fatal(line_no, "Unspecified dependency type")
                fatal(line_no, "Unrecognized dependency name")
        def process_target_sources_contents(line_no, contents):
            for word in contents.split():
                if re.fullmatch(r"[\w/]+\.hpp", word):
                    subpath = cmake_subpath.parent / word
                    target.installed_header_files.append((subpath, line_no))
                    continue
                if re.fullmatch(r'"\$\{CMAKE_CURRENT_BINARY_DIR\}/[\w/.]+"', word):
                    continue
                fatal(line_no, "Unrecognized installed header file path")
        with open(cmake_path, "r") as f:
            in_command = None
            command_line_no = None
            target_map = {}
            target_link_libraries_seen = set()
            target_sources_seen = set()
            line_no = 1
            while True:
                line = f.readline()
                if not line:
                    if in_command:
                        fatal(command_line_no, "Unterminated %s() command", in_command)
                    break
                line = line.rstrip("\n")
                if in_command:
                    if in_command == "add_library":
                        m = re.fullmatch(r"([\s\w/.]*)(\)\s*)?", line)
                        if not m:
                            fatal(line_no, "Unrecognized form of continuation line of add_library() command")
                        contents = m.group(1)
                        closure = bool(m.group(2))
                        process_add_library_contents(line_no, contents)
                        if closure:
                            target.nonheader_source_files_closing_line_no = line_no
                            in_command = None
                    elif in_command == "add_executable":
                        m = re.fullmatch(r"([\s\w/.]*)(\)\s*)?", line)
                        if not m:
                            fatal(line_no, "Unrecognized form of continuation line of add_executable() command")
                        contents = m.group(1)
                        closure = bool(m.group(2))
                        process_add_executable_contents(line_no, contents)
                        if closure:
                            target.source_files_closing_line_no = line_no
                            in_command = None
                    elif in_command == "target_link_libraries":
                        m = re.fullmatch(r"([\s\w.:]*)(\)\s*)?", line)
                        if not m:
                            fatal(line_no, "Unrecognized form of continuation line of target_link_libraries() command")
                        contents = m.group(1)
                        closure = bool(m.group(2))
                        process_target_link_libraries_contents(line_no, contents)
                        if closure:
                            target.dependencies_closing_line_no = line_no
                            in_command = None
                    elif in_command == "target_sources":
                        m = re.fullmatch(r'([\s\w/."${}]*)(\)\s*)?', line)
                        if not m:
                            fatal(line_no, "Unrecognized form of continuation line of target_sources() command")
                        contents = m.group(1)
                        closure = bool(m.group(2))
                        process_target_sources_contents(line_no, contents)
                        if closure:
                            target.installed_header_files_closing_line_no = line_no
                            in_command = None
                    else:
                        assert False
                elif re.fullmatch(r"\s*include\b.*", line):
                    m = re.fullmatch(r"\s*include\s*\(\s*([\w/.]+)\)\s*", line)
                    if not m:
                        fatal(line_no, "Unrecognized form of include() command")
                    subpath = cmake_subpath.parent / m.group(1)
                    includes.append((subpath, line_no))
                elif re.fullmatch(r"\s*add_library\b.*", line):
                    m = re.fullmatch(r"\s*add_library\s*\(\s*(\w+)(\s+(INTERFACE|OBJECT))?([\s\w/.]*)(\)\s*)?", line)
                    if not m:
                        fatal(line_no, "Unrecognized form of opening line of add_library() command")
                    cmake_name = m.group(1)
                    type_ = m.group(3)
                    contents = m.group(4)
                    closure = bool(m.group(5))
                    if cmake_name in target_map:
                        fatal(line_no, "Repeated definition of target %s", cmake_name)
                    target = LibraryTarget(cmake_name, type_)
                    targets.append(target)
                    target_map[cmake_name] = target
                    process_add_library_contents(line_no, contents)
                    if closure:
                        target.source_files_closing_line_no = line_no
                    else:
                        in_command = "add_library"
                        command_line_no = line_no
                elif re.fullmatch(r"\s*add_executable\b.*", line):
                    m = re.fullmatch(r"\s*add_executable\s*\(\s*(\w+)([\s\w/.]*)(\)\s*)?", line)
                    if not m:
                        fatal(line_no, "Unrecognized form of opening line of add_executable() command")
                    cmake_name = m.group(1)
                    contents = m.group(2)
                    closure = bool(m.group(3))
                    if cmake_name in target_map:
                        fatal(line_no, "Repeated definition of target %s", cmake_name)
                    target = ExecutableTarget(cmake_name)
                    targets.append(target)
                    target_map[cmake_name] = target
                    process_add_executable_contents(line_no, contents)
                    if closure:
                        target.source_files_closing_line_no = line_no
                    else:
                        in_command = "add_executable"
                        command_line_no = line_no
                elif re.fullmatch(r"\s*target_link_libraries\b.*", line):
                    m = re.fullmatch(r"\s*target_link_libraries\s*\(\s*(\w+)([\s\w.:]*)(\)\s*)?", line)
                    if not m:
                        fatal(line_no, "Unrecognized form of opening line of target_link_libraries() command")
                    cmake_name = m.group(1)
                    contents = m.group(2)
                    closure = bool(m.group(3))
                    if cmake_name not in target_link_libraries_seen:
                        target_link_libraries_seen.add(cmake_name)
                        target = target_map[cmake_name]
                        current_dep_type = None
                        process_target_link_libraries_contents(line_no, contents)
                        if closure:
                            target.dependencies_closing_line_no = line_no
                        else:
                            in_command = "target_link_libraries"
                            command_line_no = line_no
                elif re.fullmatch(r"\s*target_sources\b.*", line):
                    m = re.fullmatch(r'\s*target_sources\s*\(\s*(\w+)\s+PUBLIC\s+FILE_SET\s+HEADERS(\s+BASE_DIRS\s+'
                                     r'"\$\{ARCHON_BUILD_ROOT\}"\s+"\$\{ARCHON_SOURCE_ROOT\}")?\s+FILES([\s\w/."${}]*)'
                                     r'(\)\s*)?', line)
                    if not m:
                        fatal(line_no, "Unrecognized form of opening line of target_sources() command")
                    cmake_name = m.group(1)
                    contents = m.group(3)
                    closure = bool(m.group(4))
                    if cmake_name in target_sources_seen:
                        fatal(line_no, "Unexpected second target_sources() command for %s", cmake_name)
                    target_sources_seen.add(cmake_name)
                    target = target_map[cmake_name]
                    if not isinstance(target, LibraryTarget):
                        fatal(line_no, "Unexpected target_sources() command for non-library %s", cmake_name)
                    if target.type_ == "OBJECT":
                        fatal(line_no, "Unexpected target_sources() command for OBJECT library %s", cmake_name)
                    process_target_sources_contents(line_no, contents)
                    if closure:
                        target.installed_header_files_closing_line_no = line_no
                    else:
                        in_command = "target_sources"
                        command_line_no = line_no
                line_no += 1
        return self._Entry(includes, targets)


class InclusionCache:
    def __init__(self, src_path, libraries):
        self._src_path = src_path
        self._lib_cmake_names_by_dir_name = { lib.dir_name: lib.cmake_name for lib in libraries }
        self._cache = {}

    def get_inclusions(self, subpath):
        inclusions = self._cache.get(subpath)
        if inclusions is None:
            inclusions = self._get_inclusions(subpath)
            self._cache[subpath] = inclusions
        return inclusions

    def inclusion_closure(self, subpaths, subpath_filter):
        closure = set(subpaths)
        pending = collections.deque(subpaths)
        while pending:
            subpath = pending.popleft()
            inclusions = self.get_inclusions(subpath)
            for incl_subpath in inclusions:
                if not subpath_filter(incl_subpath):
                    continue
                if incl_subpath not in closure:
                    closure.add(incl_subpath)
                    pending.append(incl_subpath)
        return list(closure)

    def find_dependencies(self, subpaths, subpath_filter):
        dependencies = set()
        for subpath in subpaths:
            inclusions = self.get_inclusions(subpath)
            for incl_subpath in inclusions:
                if subpath_filter(incl_subpath):
                    continue
                dir_name = lib_dirname_from_subpath(incl_subpath)
                cmake_name = self._lib_cmake_names_by_dir_name[dir_name]
                if cmake_name not in dependencies:
                    dependencies.add(cmake_name)
        return dependencies

    def _get_inclusions(self, subpath):
        inclusions = []
        with open(self._src_path / subpath, "r") as f:
            while True:
                line = f.readline()
                if not line:
                    break
                line = line.rstrip("\n")
                m = re.fullmatch(r"\s*#\s*include\s*<(archon/.*\.hpp)>\s*", line)
                if m:
                    incl_path = pathlib.Path(m.group(1))
                    inclusions.append(incl_path)
        return inclusions


def discover_project_structure(src_path, logger, dump_group_structure = False):
    root_group = cmake.extract_structure(src_path, logger)
    if not root_group:
        return None
    if dump_group_structure:
        cmake.dump_structure(root_group, logger, log.LogLevel.INFO)
    def include_order_resolver(cmake_path):
        m = re.fullmatch(r"(.*)_library.cmake", cmake_path.name)
        if not m:
            return None
        library_dir_name = m.group(1)
        return cmake_path.parent / ("%s_include_order.txt" % library_dir_name)
    return project.discover_domain_structure(root_group, src_path, logger, include_order_resolver)
