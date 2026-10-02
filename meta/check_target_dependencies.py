import collections

import log
import project


def check(inclusion_cache, structure, extra_dependencies, logger):
    target_map = {}
    for i, target in enumerate(structure.targets):
        assert target.cmake_name not in target_map
        target_map[target.cmake_name] = i
    extra_dependencies_2 = {}
    for dependant, dependency in extra_dependencies:
        target_index = target_map[dependency]
        target = structure.targets[target_index]
        assert isinstance(target, project.LibraryTarget)
        extra_dependencies_2.setdefault(target_map[dependant], set()).add(target_index)

    def get_domain_header_files(target):
        domain = structure.domains[target.domain]
        return {path.path for path in domain.include_order}

    def get_installed_header_files(target):
        paths = []
        if isinstance(target, project.LibraryTarget):
            for file_set in target.header_file_sets:
                for path in file_set.files:
                    paths.append(path.path)
        return paths

    def get_nonheader_source_files(target):
        paths = []
        for path in target.source_files:
            paths.append(path.path)
        return paths

    def inclusion_closure(file_paths, domain_filter):
        closure = set(file_paths)
        pending = collections.deque(file_paths)
        while pending:
            path = pending.popleft()
            for path_2 in inclusion_cache.get_inclusions(path):
                if not domain_filter(path_2.path):
                    continue
                if path_2.path not in closure:
                    closure.add(path_2.path)
                    pending.append(path_2.path)
        return closure

    header_file_map = {}
    warned_overlaspping_installed_header_files = False
    for i, target in enumerate(structure.targets):
        for path in get_installed_header_files(target):
            if path in header_file_map:
                if not warned_overlaspping_installed_header_files:
                    logger.warn("Targets with overlapping installed header files")
                    warned_overlaspping_installed_header_files = True
            header_file_map[path] = i

    warned_uncovered_inclusion = False
    warned_local_and_foreign_inclusion = False
    def find_dependencies(file_paths, domain_filter, target_index):
        nonlocal warned_uncovered_inclusion, warned_local_and_foreign_inclusion
        dependencies = set()
        for path in file_paths:
            for inclusion in inclusion_cache.get_inclusions(path):
                is_local = domain_filter(inclusion.path)
                target_index_2 = header_file_map.get(inclusion.path)
                target_index_3 = target_index_2 if target_index_2 != target_index else None
                if not is_local and target_index_3 is None:
                    if not warned_uncovered_inclusion:
                        logger.warn("Inclusion that is neither local nor foreign")
                        warned_uncovered_inclusion = True
                if is_local and target_index_3 is not None:
                    if not warned_local_and_foreign_inclusion:
                        logger.warn("Inclusion that is both local and foreign")
                        warned_local_and_foreign_inclusion = True
                if target_index_3 is not None:
                    dependencies.add(target_index_3)
        return dependencies

    def derive_expected_dependencies(installed_header_dependencies, nonheader_source_dependencies):
        dependencies = {}
        for i in range(len(structure.targets)):
            of_installed_header_files = i in installed_header_dependencies
            of_nonheader_source_files = i in nonheader_source_dependencies
            if of_installed_header_files and not of_nonheader_source_files:
                type_ = project.Scope.INTERFACE
            elif of_installed_header_files and of_nonheader_source_files:
                type_ = project.Scope.PUBLIC
            elif not of_installed_header_files and of_nonheader_source_files:
                type_ = project.Scope.PRIVATE
            else:
                continue
            dependencies[i] = type_
        return dependencies

    errors_occurred = False
    warned_unclosed_installed_header_files = False
    for i, target in enumerate(structure.targets):
        logger.detail("Checking: %s" % target.cmake_name)

        domain_header_files = get_domain_header_files(target)
        def domain_filter(path):
            return path in domain_header_files

        installed_header_files = get_installed_header_files(target)
        nonheader_source_files = get_nonheader_source_files(target)

        installed_header_closure = inclusion_closure(installed_header_files, domain_filter)
        nonheader_source_closure = inclusion_closure(nonheader_source_files, domain_filter)

        if set(nonheader_source_files) != nonheader_source_closure:
            assert set(nonheader_source_files) < nonheader_source_closure
            if not warned_unclosed_installed_header_files:
                logger.warn("Installed header file includes non-installed header file")
                warned_unclosed_installed_header_files = True

        installed_header_dependencies = find_dependencies(installed_header_closure, domain_filter, i)
        nonheader_source_dependencies = find_dependencies(nonheader_source_closure, domain_filter, i)

        for target_index in extra_dependencies_2.get(i, []):
            nonheader_source_dependencies.add(target_index)

        expected_dependencies = derive_expected_dependencies(installed_header_dependencies,
                                                             nonheader_source_dependencies)

        def error(context, message, *args):
            nonlocal errors_occurred
            log.FileContextLogger(logger, context).error(message, *args)
            errors_occurred = True

        seen = set()
        for dependency_set in target.dependency_sets:
            highest_target_index = 0
            for dep in dependency_set.dependencies:
                context = log.FileContext(dependency_set.cmake_path, dep.line_no)
                target_2 = structure.targets[dep.target_index]
                if dep.target_index in seen:
                    error(context, "Repeated dependency: %s %s", dep.scope.name, target_2.cmake_name)
                    continue
                seen.add(dep.target_index)
                expected_type = expected_dependencies.get(dep.target_index, None)
                if expected_type is None:
                    error(context, "Superfluous dependency: %s %s", dep.scope.name, target_2.cmake_name)
                    continue
                if expected_type != dep.scope:
                    error(context, "Wrong dependency type: %s %s (should have been %s)", dep.scope.name,
                          target_2.cmake_name, expected_type.name)
                if dep.target_index < highest_target_index:
                    error(context, "Out-of-order dependency: %s %s", dep.scope.name, target_2.cmake_name)
                else:
                    highest_target_index = dep.target_index

        domain = structure.domains[target.domain]
        context = log.FileContext(domain.cmake_path, target.line_no)
        first_dependency_set = target.dependency_sets[0] if target.dependency_sets else None
        for j, target_2 in enumerate(structure.targets):
            expected_type = expected_dependencies.get(j, None)
            if expected_type is None or j in seen:
                continue
            if first_dependency_set:
                line_no = first_dependency_set.closing_line_no
                for dep in first_dependency_set.dependencies:
                    if dep.target_index > j:
                        line_no = dep.line_no
                        break
                context = log.FileContext(first_dependency_set.cmake_path, line_no)
            error(context, "Missing dependency: %s %s", expected_type.name, target_2.cmake_name)

    return not errors_occurred
