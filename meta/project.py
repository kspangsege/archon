from __future__ import annotations
from typing import Protocol, Never, Any, Iterable

import re
import pathlib

import graph
import log
import cmake


# Re-export
LibraryType = cmake.LibraryType
Scope       = cmake.Scope


# The domain structure differs from the CMake group structure in the following ways:
#
#   - The domain structure is flat, not hierarchical.
#
#   - Source and header file paths are expressed relative to the source root directory
#     specified as `src_path`.
#
#   - Dependencies are resolved.
#
#   - Each domain is associated with an "include order", which is a list of all the header
#     files in the domain in the order that they should be included.
#
# Each CMake group with at least one target gives rise to a domain.
#
# The source files of a domain is the union of the source files of the targets in that
# domain.
#
# The header files of a domain is the union of the installed header files of the library
# targets in that domain and the header files in the include order of the domain.
#
def discover_domain_structure(root_group: cmake.Group, src_path: pathlib.Path, logger: log.Logger,
                              include_order_resolver: IncludeOrderResolver | None = None) -> Structure | None:
    try:
        return _discover_domain_structure(root_group, src_path, logger, include_order_resolver)
    except _DiscoverError:
        return None


def dump_domain_structure(structure: Structure, logger: log.Logger, log_level: log.LogLevel) -> None:
    if logger.will_log(log_level):
        _dump_domain_structure(structure, logger, log_level)


class IncludeOrderResolver(Protocol):
    def __call__(self, cmake_path: pathlib.Path) -> pathlib.Path | None:
        ...


class Structure:
    def __init__(self, domains: Iterable[Domain], targets: Iterable[Target], dependency_order: Iterable[int]) -> None:
        self.domains          = list(domains) # in CMake configuration order
        self.targets          = list(targets) # in CMake configuration order
        self.dependency_order = list(dependency_order) # Target indexes


class Domain:
    def __init__(self, cmake_path: pathlib.Path, include_order: Iterable[SourcePath]) -> None:
        self.cmake_path    = cmake_path
        self.include_order = list(include_order)


class Target:
    def __init__(self, cmake_name: str, domain: int, source_files: Iterable[SourcePath],
                 source_file_sets: Iterable[SourceFileSet], dependency_sets: Iterable[DependencySet], line_no: int,
                 closing_line_no: int) -> None:
        self.cmake_name       = cmake_name
        self.domain           = domain # Index of associated domain
        self.source_files     = list(source_files)
        self.source_file_sets = list(source_file_sets)
        self.dependency_sets  = list(dependency_sets)
        self.line_no          = line_no
        self.closing_line_no  = closing_line_no

class LibraryTarget(Target):
    def __init__(self, cmake_name: str, library_type: LibraryType, domain: int, source_files: Iterable[SourcePath],
                 source_file_sets: Iterable[SourceFileSet], dependency_sets: Iterable[DependencySet],
                 header_file_sets: Iterable[HeaderFileSet], line_no: int, closing_line_no: int) -> None:
        Target.__init__(self, cmake_name, domain, source_files, source_file_sets, dependency_sets, line_no,
                        closing_line_no)
        self.library_type     = library_type
        self.header_file_sets = list(header_file_sets)

class ExecutableTarget(Target):
    def __init__(self, cmake_name: str, domain: int, source_files: Iterable[SourcePath],
                 source_file_sets: Iterable[SourceFileSet], dependency_sets: Iterable[DependencySet], line_no: int,
                 closing_line_no: int) -> None:
        Target.__init__(self, cmake_name, domain, source_files, source_file_sets, dependency_sets, line_no,
                        closing_line_no)


class SourceFileSet:
    def __init__(self, files: Iterable[SourcePath], cmake_path: pathlib.Path, line_no: int,
                 closing_line_no: int) -> None:
        self.files           = list(files)
        self.cmake_path      = cmake_path
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


class HeaderFileSet:
    def __init__(self, files: Iterable[SourcePath], cmake_path: pathlib.Path, line_no: int,
                 closing_line_no: int) -> None:
        self.files           = list(files)
        self.cmake_path      = cmake_path
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


class DependencySet:
    def __init__(self, dependencies: Iterable[Dependency], cmake_path: pathlib.Path, line_no: int,
                 closing_line_no: int) -> None:
        self.dependencies    = list(dependencies)
        self.cmake_path      = cmake_path
        self.line_no         = line_no
        self.closing_line_no = closing_line_no


class SourcePath:
    def __init__(self, path: str, line_no: int) -> None:
        self.path    = path # relative to source root
        self.line_no = line_no

class IncludeOrderEntry(SourcePath):
    def __init__(self, path: str, force: bool, line_no: int) -> None:
        SourcePath.__init__(self, path, line_no)
        self.force = force


class Dependency:
    def __init__(self, scope: Scope, target_index: int, line_no: int) -> None:
        self.scope        = scope
        self.target_index = target_index
        self.line_no      = line_no


def _discover_domain_structure(root_group: cmake.Group, src_path: pathlib.Path, logger: log.Logger,
                               include_order_resolver: IncludeOrderResolver | None = None) -> Structure:
    def discover() -> Structure:
        domains: list[Domain] = []
        original_targets: list[tuple[int, cmake.Target]] = []
        target_map: dict[str, int] = {}
        def visit(group: cmake.Group) -> None:
            if group.targets:
                include_order_path: pathlib.Path | None
                if isinstance(group, cmake.Subgroup) and group.type_ == cmake.Subgroup.Type.SUBDIR:
                    include_order_path = group.cmake_path.parent / "include_order.txt"
                else:
                    include_order_path = include_order_resolver(group.cmake_path) if include_order_resolver else None
                include_order = fetch_include_order(include_order_path)
                domain = Domain(group.cmake_path, include_order)
                domain_index = len(domains)
                domains.append(domain)
                for target in group.targets:
                    if target.cmake_name in target_map:
                        context = log.FileContext(group.cmake_path, target.line_no)
                        fatal(context, "Multiple targets with same name")
                    target_index = len(original_targets)
                    original_targets.append((domain_index, target))
                    target_map[target.cmake_name] = target_index
            for subgroup in group.subgroups:
                visit(subgroup)
        visit(root_group)
        targets: list[Target] = []
        dependency_graph: list[set[int]] = [set() for _ in original_targets]
        for i, (domain_index, target) in enumerate(original_targets):
            domain = domains[domain_index]
            source_files = []
            for path in target.source_files:
                context = log.FileContext(domain.cmake_path, path.line_no)
                path_2 = resolve_source_path(path.path, context)
                source_files.append(SourcePath(path_2, path.line_no))
            file_set: Any
            source_file_sets = []
            for file_set in target.source_file_sets:
                cmake_path = get_cmake_path(file_set.subgroup_path)
                files = []
                for path in file_set.files:
                    context = log.FileContext(cmake_path, path.line_no)
                    if path.scope != cmake.Scope.PRIVATE:
                        fatal(context, "Unexpected source file scope (%s)", path.scope.name)
                    path_2 = resolve_source_path(path.path, context)
                    files.append(SourcePath(path_2, path.line_no))
                if files:
                    source_file_set = SourceFileSet(files, cmake_path, file_set.line_no, file_set.closing_line_no)
                    source_file_sets.append(source_file_set)
            dependency_sets = []
            for link_item_set in target.link_item_sets:
                dependencies = []
                for item in link_item_set.items:
                    if not re.fullmatch(r"\w*", item.value):      
                        continue
                    if item.config != cmake.LinkItem.Config.GENERAL:
                        context = get_file_context(link_item_set.subgroup_path, item.line_no)
                        fatal(context, "Unexpected link item configuration selector (%s)", item.config.name)
                    dependency_index = target_map.get(item.value)
                    if dependency_index is None:
                        context = get_file_context(link_item_set.subgroup_path, item.line_no)
                        fatal(context, "Undefined dependency (%s)", item.value)
                    _, dependency_target = original_targets[dependency_index]
                    if not isinstance(dependency_target, cmake.LibraryTarget):
                        context = get_file_context(link_item_set.subgroup_path, item.line_no)
                        fatal(context, "Invalid non-library dependency (%s)", item.value)
                    dependencies.append(Dependency(item.scope, dependency_index, item.line_no))
                    dependency_graph[i].add(dependency_index)
                if dependencies:
                    cmake_path = get_cmake_path(link_item_set.subgroup_path)
                    dependency_sets.append(DependencySet(dependencies, cmake_path, link_item_set.line_no,
                                                         link_item_set.closing_line_no))
            target_2: Target
            if isinstance(target, cmake.LibraryTarget):
                header_file_sets = []
                for file_set in target.header_file_sets:
                    # FIXME: Pay attention to which header files sets are actually
                    # installed, rather than assuming it is the one called "HEADERS"            
                    # FIXME: Pay attention to base directories                     
                    if file_set.scope != cmake.Scope.PUBLIC:
                        context = get_file_context(file_set.subgroup_path, file_set.line_no)
                        fatal(context, "Unexpected header file set scope (%s)", file_set.scope.name)
                    if file_set.name != "HEADERS":
                        context = get_file_context(file_set.subgroup_path, file_set.line_no)
                        fatal(context, "Unexpected header file set name (%s)", file_set.name)                       
                    cmake_path = get_cmake_path(file_set.subgroup_path)
                    files = []
                    for path in file_set.files:
                        context = log.FileContext(cmake_path, path.line_no)
                        path_2 = resolve_source_path(path.path, context)
                        files.append(SourcePath(path_2, path.line_no))
                    # FIXME: Should empty header file sets be included (base directories)?      
                    header_file_sets.append(HeaderFileSet(files, cmake_path, file_set.line_no,
                                                          file_set.closing_line_no))
                target_2 = LibraryTarget(target.cmake_name, target.library_type, domain_index, source_files,
                                         source_file_sets, dependency_sets, header_file_sets, target.line_no,
                                         target.closing_line_no)
                targets.append(target_2)
                continue
            if isinstance(target, cmake.ExecutableTarget):
                if target.header_file_sets:
                    file_set = target.header_file_sets[0]
                    context = get_file_context(file_set.subgroup_path, file_set.line_no)
                    fatal(context, "Unexpected header file set for executable target")
                target_2 = ExecutableTarget(target.cmake_name, domain_index, source_files, source_file_sets,
                                            dependency_sets, target.line_no, target.closing_line_no)
                targets.append(target_2)
                continue
            assert False
        dependency_order = graph.stable_topological_sort(dependency_graph)
        if dependency_order is None:
            cycle = graph.find_dependency_cycle(dependency_graph)
            assert cycle
            target_2 = targets[cycle[0]]
            domain = domains[target_2.domain]
            context = log.FileContext(domain.cmake_path, target_2.line_no)
            fatal(context, "Dependency cycle detected")
        return Structure(domains, targets, dependency_order)

    def get_file_context(subgroup_path: list[int], line_no: int) -> log.FileContext:
        path = get_cmake_path(subgroup_path)
        return log.FileContext(path, line_no)

    def get_cmake_path(subgroup_path: list[int]) -> pathlib.Path:
        group = root_group
        for index in subgroup_path:
            group = group.subgroups[index]
        return group.cmake_path

    def fetch_include_order(path: pathlib.Path | None) -> list[IncludeOrderEntry]:
        if path is None or not path.exists():
            return []
        include_order = []
        context = log.FileContext(path)
        context_logger = log.FileContextLogger(logger, context)
        with open(path, "r") as f:
            while True:
                context.line_no += 1
                line = f.readline()
                if not line:
                    break
                line = line.rstrip("\n")
                m = re.fullmatch(r"\s*(((\.\./)*(\w+/)*\w+\.(h|hpp))(\s+FORCE)?)?\s*(#.*)?", line)
                if not m:
                    context_logger.fatal("Invalid syntax")
                    raise _DiscoverError
                if not m.group(1):
                    continue
                path_2 = m.group(2)
                force = bool(m.group(3))
                path_3 = resolve_source_path(path.parent / path_2, context)
                include_order.append(IncludeOrderEntry(path_3, force, context.line_no))
        return include_order

    def resolve_source_path(path: pathlib.Path, file_context: log.FileContext) -> str:
        if not path.is_relative_to(src_path):
            fatal(file_context, "Source file path outside source root")
        return path.relative_to(src_path).as_posix()

    def fatal(file_context: log.FileContext, message: str , *args: Any) -> Never:
        log.FileContextLogger(logger, file_context).fatal(message, *args)
        raise _DiscoverError

    return discover()


class _DiscoverError(Exception):
    pass


def _dump_domain_structure(structure: Structure, logger: log.Logger, log_level: log.LogLevel) -> None:
    def dump_structure() -> None:
        for domain in structure.domains:
            dump(0, "Domain(%s)", domain.cmake_path)
            if domain.include_order:
                dump(1, "Include order:")
                for path in domain.include_order:
                    dump(2, "%s (line %s)", path.path, path.line_no)
        for target in structure.targets:
            domain = structure.domains[target.domain]
            if isinstance(target, LibraryTarget):
                dump(0, "LibraryTarget(%s, %s [line %s] [closing line %s])", target.cmake_name,
                     target.library_type.name, target.line_no, target.closing_line_no)
                dump(1, "Domain: %s [%s]", 1 + target.domain, domain.cmake_path)
                if target.library_type != LibraryType.INTERFACE:
                    dump_source_files(target)
                dump_source_file_sets(target)
                dump_header_file_sets(target)
                dump_dependencies_sets(target)
                continue
            if isinstance(target, ExecutableTarget):
                dump(0, "ExecutableTarget(%s [line %s] [closing line %s])", target.cmake_name, target.line_no,
                     target.closing_line_no)
                dump(1, "Domain: %s [%s]", 1 + target.domain, domain.cmake_path)
                dump_source_files(target)
                dump_source_file_sets(target)
                dump_dependencies_sets(target)
                continue
            assert False
        dump(0, "Dependency order:")
        for index in structure.dependency_order:
            target = structure.targets[index]
            dump(1, "%s [%s]", 1 + index, target.cmake_name)

    def dump_source_files(target: Target) -> None:
        dump(1, "Source files:")
        for path in target.source_files:
            dump(2, "%s [line %s]", path.path, path.line_no)

    def dump_source_file_sets(target: Target) -> None:
        for file_set in target.source_file_sets:
            dump(1, "SourceFileSet(%s [line %s] [closing line %s]):", file_set.cmake_path, file_set.line_no,
                 file_set.closing_line_no)
            for path in file_set.files:
                dump(2, "%s [line %s]", path.path, path.line_no)

    def dump_header_file_sets(target: LibraryTarget) -> None:
        for file_set in target.header_file_sets:
            dump(1, "HeaderFileSet(%s [line %s] [closing line %s]):", file_set.cmake_path, file_set.line_no,
                 file_set.closing_line_no)
            for path in file_set.files:
                dump(2, "%s [line %s]", path.path, path.line_no)

    def dump_dependencies_sets(target: Target) -> None:
        for dependency_set in target.dependency_sets:
            dump(1, "DependencySet(%s [line %s] [closing line %s]):", dependency_set.cmake_path,
                 dependency_set.line_no, dependency_set.closing_line_no)
            for dep in dependency_set.dependencies:
                dependency_target = structure.targets[dep.target_index]
                dump(2, "%s %s [%s] [line %s]", dep.scope.name, dep.target_index, dependency_target.cmake_name,
                     dep.line_no)

    lines = []
    def dump(level: int, message: str, *args: Any) -> None:
        indent = level * "    "
        lines.append("%s%s" % (indent, message % args))
    dump_structure()
    logger.log(log_level, "%s", "\n".join(lines))
