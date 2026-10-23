import re
import pathlib
import sys

import util


src_path = util.resolve_self_rel_path(sys.argv[0], "../src")


header_file_exclusions = {
    "core": [
        "integer_concept.hpp",
        "float_concept.hpp",
        "char_codec_concept.hpp",
        "text_codec_impl_concept.hpp",
        "text_file_impl_concept.hpp",
        "buffered_text_file_impl_concept.hpp",
    ],
    "image": [
        "channel_spec_concept.hpp",
        "channel_packing_concept.hpp",
        "pixel_format_concept.hpp",
    ],
}


foreign_dir_names = set([ "attic", "tool", "probe", "demo", "test" ])


def library_source_file_subpath_filter(lib):
    def filter_(subpath):
        parts = subpath.parts
        assert len(parts) >= 1
        assert parts[0] == "archon"
        if len(parts) >= 3 and parts[1] == lib.dir_name:
            return parts[2] not in foreign_dir_names
        return len(parts) == 2 and subpath.stem == lib.dir_name and subpath.suffix == ".hpp"
    return filter_


def subdir_source_file_subpath_filter(subdir_subpath):
    def filter_(subpath):
        return subpath.is_relative_to(subdir_subpath)
    return filter_


def single_source_file_subpath_filter(source_file_subpath):
    def filter_(subpath):
        return subpath == source_file_subpath
    return filter_



def find_library_installed_header_files(lib):
    def subdir_filter(infrapath):
        parts = infrapath.parts
        assert len(parts) >= 1
        return parts[0] != "noinst" and parts[0] not in foreign_dir_names
    subpaths = util.find_files(src_path / lib.dir_subpath, [ ".hpp" ], subdir_filter)
    exclusions = { pathlib.Path(p) for p in header_file_exclusions.get(lib.dir_name, []) }
    exclusions.add(pathlib.Path("%s_namespace.hpp" % lib.dir_name))
    subpaths_2 = [ lib.dir_subpath / p for p in subpaths if p not in exclusions ]
    if (src_path / lib.incl_subpath).is_file():
        subpaths_2.append(lib.incl_subpath)
    return subpaths_2


def find_library_nonheader_source_files(lib):
    def subdir_filter(infrapath):
        parts = infrapath.parts
        assert len(parts) >= 1
        return parts[0] not in foreign_dir_names
    subpaths = util.find_files(src_path / lib.dir_subpath, [ ".cpp" ], subdir_filter)
    return [ lib.dir_subpath / p for p in subpaths ]


def find_subdir_nonheader_source_files(subdir_subpath):
    return [ subdir_subpath / p for p in util.find_files(src_path / subdir_subpath, [ ".cpp" ]) ]


targets_cache = util.TargetsCache(src_path)
libraries = targets_cache.discover_libraries()
inclusions_cache = util.InclusionCache(src_path, libraries)
dependency_order = [ lib.cmake_name for lib in libraries ] + [ lib.test_cmake_name for lib in libraries ]
errors_occurred = False


def derive_expected_dependencies(installed_header_dependencies, nonheader_source_dependencies):
    dependencies = []
    for cmake_name in dependency_order:
        of_installed_header_files = cmake_name in installed_header_dependencies
        of_nonheader_source_files = cmake_name in nonheader_source_dependencies
        if of_installed_header_files and not of_nonheader_source_files:
            type_ = "INTERFACE"
        elif of_installed_header_files and of_nonheader_source_files:
            type_ = "PUBLIC"
        elif not of_installed_header_files and of_nonheader_source_files:
            type_ = "PRIVATE"
        else:
            continue
        dependencies.append(util.Dependency(cmake_name, type_))
    return dependencies


def compare_dependencies(cmake_subpath, what, actual_dependencies, closing_line_no, expected_dependencies):
    cmake_path = src_path / cmake_subpath
    def error(line_no, message, *args):
        global errors_occurred
        prefix = "%s:%s" % (cmake_path, line_no) if line_no is not None else cmake_path
        print("%s: %s: ERROR: %s" % (prefix, what, (message % args)))
        errors_occurred = True

    dep_order_by_cmake_name = { cmake_name: i for i, cmake_name in enumerate(dependency_order) }
    actual_dependencies_by_cmake_name   = { dep.cmake_name for dep, line_no in actual_dependencies }
    expected_dependencies_by_cmake_name = { dep.cmake_name: dep for dep in expected_dependencies }

    seen = set()
    highest_order = 0
    for actual, line_no in actual_dependencies:
        if actual.cmake_name not in dep_order_by_cmake_name:
            error(line_no, "Unrecognized dependency: %s %s", actual.type_, actual.cmake_name)
            continue
        if actual.cmake_name in seen:
            error(line_no, "Repeated dependency: %s %s", actual.type_, actual.cmake_name)
            continue
        seen.add(actual.cmake_name)
        expected = expected_dependencies_by_cmake_name.get(actual.cmake_name)
        if not expected:
            error(line_no, "Superfluous dependency: %s %s", actual.type_, actual.cmake_name)
            continue
        if expected.type_ != actual.type_:
            error(line_no, "Wrong dependency type: %s %s (should have been %s)", actual.type_, actual.cmake_name,
                  expected.type_)
        order = dep_order_by_cmake_name[actual.cmake_name]
        if order < highest_order:
            error(line_no, "Out-of-order dependency: %s %s", actual.type_, actual.cmake_name)
        else:
            highest_order = order

    for expected in expected_dependencies:
        if expected.cmake_name in actual_dependencies_by_cmake_name:
            continue
        order = dep_order_by_cmake_name[expected.cmake_name]
        line_no = None
        for actual, line_no_2 in actual_dependencies:
            if dep_order_by_cmake_name[actual.cmake_name] > order:
                line_no = line_no_2
                break
        if line_no is None:
            line_no = closing_line_no
        error(line_no, "Missing dependency: %s %s", expected.type_, expected.cmake_name)


def check_lib(lib):
    what = "%s library" % lib.cmake_name
    print("Checking: %s" % what)

    installed_header_files = targets_cache.get_library_installed_header_files(lib)
    nonheader_source_files = targets_cache.get_library_nonheader_source_files(lib)

    installed_header_files_2 = find_library_installed_header_files(lib)
    nonheader_source_files_2 = find_library_nonheader_source_files(lib)
    assert set(installed_header_files_2) == set(installed_header_files)
    assert set(nonheader_source_files_2) == set(nonheader_source_files)

    subpath_filter = library_source_file_subpath_filter(lib)
    installed_header_closure = inclusions_cache.inclusion_closure(installed_header_files, subpath_filter)
    nonheader_source_closure = inclusions_cache.inclusion_closure(nonheader_source_files, subpath_filter)
    assert set(installed_header_closure) == set(installed_header_files)

    installed_header_dependencies = inclusions_cache.find_dependencies(installed_header_closure, subpath_filter)
    nonheader_source_dependencies = inclusions_cache.find_dependencies(nonheader_source_closure, subpath_filter)

    actual_dependencies, closing_line_no = targets_cache.get_library_dependencies(lib)
    expected_dependencies = derive_expected_dependencies(installed_header_dependencies, nonheader_source_dependencies)
    compare_dependencies(lib.cmake_subpath, what, actual_dependencies, closing_line_no, expected_dependencies)


def check_tools(lib):
    if not (src_path / lib.tool_dir_subpath).is_dir():
        return
    combined_source_files = set()
    targets = targets_cache.get_targets(lib.tool_cmake_subpath)
    for target in targets:
        assert isinstance(target, util.ExecutableTarget)
        what = "%s/%s tool" % (lib.cmake_name, target.cmake_name)
        print("Checking: %s" % what)
        actual_dependencies = target.dependencies
        closing_line_no = target.dependencies_closing_line_no
        source_files = [ subpath for subpath, line_no in target.source_files ]
        combined_source_files = combined_source_files | set(source_files)
        subpath_filter = subdir_source_file_subpath_filter(lib.tool_dir_subpath)
        source_files_closure = inclusions_cache.inclusion_closure(source_files, subpath_filter)
        dependencies = inclusions_cache.find_dependencies(source_files_closure, subpath_filter)
        expected_dependencies = derive_expected_dependencies(set(), dependencies)
        compare_dependencies(lib.tool_cmake_subpath, what, actual_dependencies, closing_line_no,
                             expected_dependencies)
    combined_source_files_2 = set(find_subdir_nonheader_source_files(lib.tool_dir_subpath))
    assert combined_source_files_2 == combined_source_files


def check_probes(lib):
    if not (src_path / lib.probe_dir_subpath).is_dir():
        return
    combined_source_files = set()
    targets = targets_cache.get_targets(lib.probe_cmake_subpath)
    for target in targets:
        assert isinstance(target, util.ExecutableTarget)
        what = "%s/%s probe" % (lib.cmake_name, target.cmake_name)
        print("Checking: %s" % what)
        actual_dependencies = target.dependencies
        closing_line_no = target.dependencies_closing_line_no
        source_files = [ subpath for subpath, line_no in target.source_files ]
        combined_source_files = combined_source_files | set(source_files)
        subpath_filter = subdir_source_file_subpath_filter(lib.probe_dir_subpath)
        source_files_closure = inclusions_cache.inclusion_closure(source_files, subpath_filter)
        dependencies = inclusions_cache.find_dependencies(source_files_closure, subpath_filter)
        expected_dependencies = derive_expected_dependencies(set(), dependencies)
        compare_dependencies(lib.probe_cmake_subpath, what, actual_dependencies, closing_line_no,
                             expected_dependencies)
    combined_source_files_2 = set(find_subdir_nonheader_source_files(lib.probe_dir_subpath))
    assert combined_source_files_2 == combined_source_files


def check_demos(lib):
    if not (src_path / lib.demo_dir_subpath).is_dir():
        return
    combined_source_files = set()
    targets = targets_cache.get_targets(lib.demo_cmake_subpath)
    for target in targets:
        assert isinstance(target, util.ExecutableTarget)
        what = "%s/%s demo" % (lib.cmake_name, target.cmake_name)
        print("Checking: %s" % what)
        actual_dependencies = target.dependencies
        closing_line_no = target.dependencies_closing_line_no
        source_files = [ subpath for subpath, line_no in target.source_files ]
        combined_source_files = combined_source_files | set(source_files)
        subpath_filter = subdir_source_file_subpath_filter(lib.demo_dir_subpath)
        source_files_closure = inclusions_cache.inclusion_closure(source_files, subpath_filter)
        dependencies = inclusions_cache.find_dependencies(source_files_closure, subpath_filter)
        expected_dependencies = derive_expected_dependencies(set(), dependencies)
        compare_dependencies(lib.demo_cmake_subpath, what, actual_dependencies, closing_line_no,
                             expected_dependencies)
    combined_source_files_2 = set(find_subdir_nonheader_source_files(lib.demo_dir_subpath))
    assert combined_source_files_2 == combined_source_files


def check_test_subsuite(lib):
    if not (src_path / lib.test_dir_subpath).is_dir():
        return False
    targets = targets_cache.get_targets(lib.test_cmake_subpath)
    assert len(targets) == 1
    target = targets[0]
    assert isinstance(target, util.LibraryTarget)
    assert target.type_ == "OBJECT"
    assert target.cmake_name == lib.test_cmake_name
    what = "%s test sub-suite" % lib.cmake_name
    print("Checking: %s" % what)
    actual_dependencies = target.dependencies
    closing_line_no = target.dependencies_closing_line_no
    source_files = [ subpath for subpath, line_no in target.nonheader_source_files ]
    assert set(source_files) == set(find_subdir_nonheader_source_files(lib.test_dir_subpath))
    subpath_filter = subdir_source_file_subpath_filter(lib.test_dir_subpath)
    source_files_closure = inclusions_cache.inclusion_closure(source_files, subpath_filter)
    dependencies = inclusions_cache.find_dependencies(source_files_closure, subpath_filter)
    expected_dependencies = derive_expected_dependencies(set(), dependencies)
    compare_dependencies(lib.test_cmake_subpath, what, actual_dependencies, closing_line_no,
                         expected_dependencies)
    return True


def check_test_executor(test_subsuite_targets):
    cmake_subpath = pathlib.Path("CMakeLists.txt")
    targets = targets_cache.get_targets(cmake_subpath)
    assert len(targets) == 1
    target = targets[0]
    assert isinstance(target, util.ExecutableTarget)
    assert target.cmake_name == "Test"
    what = "Test executor"
    print("Checking: %s" % what)
    actual_dependencies = target.dependencies
    closing_line_no = target.dependencies_closing_line_no
    source_files = [ subpath for subpath, line_no in target.source_files ]
    assert len(source_files) == 1
    source_file = source_files[0]
    subpath_filter = single_source_file_subpath_filter(source_file)
    source_files_closure = inclusions_cache.inclusion_closure(source_files, subpath_filter)
    dependencies = inclusions_cache.find_dependencies(source_files_closure, subpath_filter)
    dependencies.update(test_subsuite_targets)
    expected_dependencies = derive_expected_dependencies(set(), dependencies)
    compare_dependencies(cmake_subpath, what, actual_dependencies, closing_line_no,
                         expected_dependencies)


test_subsuite_targets = set()
for lib in libraries:
    check_lib(lib)
    check_tools(lib)
    check_probes(lib)
    check_demos(lib)
    if check_test_subsuite(lib):
        test_subsuite_targets.add(lib.test_cmake_name)

check_test_executor(test_subsuite_targets)

if errors_occurred:
    sys.exit(1)
