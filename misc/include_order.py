import re
import sys
import os
import tempfile

self_path = sys.argv[0]
assert os.path.basename(self_path) == "include_order.py"
abs_src_path = os.path.abspath(os.path.join(os.path.dirname(self_path), "../src"))

def transform_file(path, transfer, destin_path = None):
    temp_path = None
    try:
        with tempfile.NamedTemporaryFile(mode = "w", dir = os.path.dirname(path), delete = False) as temp_file:
            temp_path = temp_file.name
            with open(path, "r") as origin_file:
                transfer(origin_file, temp_file)
        destin_path_2 = path
        if destin_path is not None:
            destin_path_2 = destin_path
        os.replace(temp_path, destin_path_2)
        temp_path = None
    finally:
        if temp_path is not None:
            os.remove(temp_path)

def copy_file(origin_path, destin_path):
    def transfer(origin_file, temp_file):
        while True:
            line = origin_file.readline()
            if not line:
                break
            temp_file.write(line)
    transform_file(origin_path, transfer, destin_path)

# Returns true iff `path_1` is a equal to, or a prefix of `path_2`. The paths can be the
# empty string, which mean that it refers to the implied base directory.
#
def is_prefix_path(path_1, path_2):
    return os.path.commonpath([path_1, path_2]) == os.path.commonpath([path_1, path_1])

# Split relative path into its segments. An empty string is understood as a zero-segment
# path rather than a path with one empty segment.
#
def split_rel_path(path):
    path_2 = path
    rev_segments = []
    while path_2:
        dir_path, segment = os.path.split(path_2)
        rev_segments.append(segment)
        path_2 = dir_path
    return list(reversed(rev_segments))

# Reconstruct relative path from its segments. A zero-segment path becomes an empty string.
#
def join_rel_path(segments):
    if segments:
        return os.path.join(segments[0], *segments[1:])
    return ""



#        
#
# FIXME: Clarify that a path to the source directory or to any file or directory inside the source directory can be mapped to its corresponding include path. The result of mapping the source directory path to an include path is the empty string. Correspondingly, if the empty string is taken as an include path and mapped back to a regular path, the result is the path to the source diretory.
#
class PathMapper:
    def __init__(self, abs_src_root_path):
        self._abs_src_root_path = abs_src_root_path

    # This function returns the include path that corrresponds to the specified path. This                   
    # function can also be used for directory paths. If the specified path refers to the
    # source root, this function returns the empty string.
    #
    def to_include_path(self, path):
        incl_path = self.try_map_to_include_path(path)
        assert incl_path is not None
        return incl_path

    # This function returns the resolved filesystem path corresponding to the specified                     
    # include path. This function can also be used for directory paths. If the specified
    # include path is the empty string, the returned path will refer to the source root.
    #
    def from_include_path(self, incl_path):
        return os.path.relpath(os.path.join(self._abs_src_root_path, incl_path))

    #        
    #
    def can_be_mapped_to_include_path(self, path):
        return bool(self.try_map_to_include_path(path))

    # If the specified path refers to the source root or to something directly or indirectly                    
    # inside the source root, this function returns the corresponding include
    # path. Othwerwise, this function returns `None`. This function can also be used for
    # directory paths. If the specified path refers to the source root, this function
    # returns the empty string.
    #
    def try_map_to_include_path(self, path):
        path_2 = os.path.abspath(path)
        if is_prefix_path(self._abs_src_root_path, path_2):
            incl_path = os.path.relpath(path_2, self._abs_src_root_path)
            if incl_path != ".":
                return incl_path
            return ""
        return None

    #        
    #
    def are_same_paths(self, path_1, path_2):
        return self.to_include_path(path_1) == self.to_include_path(path_2)

    #        
    #
    def is_prefix_path(self, path_1, path_2):
        return is_prefix_path(self.to_include_path(path_1), self.to_include_path(path_2))

    #        
    #
    def is_reference_header(self, header_path):
        return bool(self.try_get_as_reference_header(header_path))

    #        
    #
    def get_as_reference_header(self, header_path):
        reference_header = self.try_get_as_reference_header(header_path)
        assert reference_header
        return reference_header

    #        
    #
    def try_get_as_reference_header(self, header_path):
        segments = split_rel_path(self.to_include_path(header_path))

        if len(segments) < 1 or segments[-1] != "everything.hpp":
            return None

        cutoff = -1
        if len(segments) >= 2 and segments[-2] == "noinst":
            cutoff = -2

        assoc_dir_path = self.from_include_path(join_rel_path(segments[:cutoff]))
        return PathMapper.ReferenceHeader(header_path, assoc_dir_path)

    #        
    #
    class ReferenceHeader:
        def __init__(self, header_path, assoc_dir_path):
            self.header_path = header_path       # Path to reference header file
            self.assoc_dir_path = assoc_dir_path # Path to associated source (sub)directory



#        
#
class FileStructureCache:
    def __init__(self, path_mapper):
        self._path_mapper = path_mapper
        self._dir_entries = {}

    # This function returns a list of include paths, one for each of the files located in the specified directory. the set of files                                    
    #
    def find_files(self, dir_path, suffixes, subdir_filter = None):
        paths = []
        def traverse(dir_path):
            for entry in self._get_dir_entries(dir_path):
                if entry.is_dir:
                    if entry.name != "attic" and (not subdir_filter or subdir_filter(entry.name, entry.path)):
                        traverse(entry.path)
                else:
                    ext = os.path.splitext(entry.path)[1]
                    if ext in suffixes:
                        paths.append(entry.path)
        traverse(dir_path)
        return paths

    #        
    #
    def find_header_files(self, dir_path, subdir_filter = None):
        return self.find_files(dir_path, [ ".h", ".hpp" ], subdir_filter)

    #        
    #
    def find_source_files(self, dir_path, subdir_filter = None):
        return self.find_files(dir_path, [ ".h", ".c", ".hpp", ".cpp" ], subdir_filter)

    class _DirEntry:
        def __init__(self, name, path, is_dir):
            self.name = name
            self.path = path
            self.is_dir = is_dir

    def _get_dir_entries(self, dir_path):
        incl_path = self._path_mapper.to_include_path(dir_path)
        entries = self._dir_entries.get(incl_path)
        if entries is not None:
            return entries

        entries = []
        for name in os.listdir(dir_path):
            path = os.path.join(dir_path, name)
            is_dir = False
            if os.path.isfile(path):
                pass
            elif os.path.isdir(path):
                is_dir = True
            else:
                continue
            entries.append(FileStructureCache._DirEntry(name, path, is_dir))

        self._dir_entries[incl_path] = entries
        return entries



#        
#
class InclusionCache:
    def __init__(self, path_mapper, file_structure_cache):
        self._path_mapper = path_mapper
        self._file_structure_cache = file_structure_cache
        self._inclusions = {}

    #        
    #
    def get_inclusions(self, source_path):
        incl_path = self._path_mapper.to_include_path(source_path)
        inclusions = self._inclusions.get(incl_path)
        if inclusions is not None:
            return inclusions

        inclusions = []
        if os.path.basename(source_path) != "config.h":
            with open(source_path, "r") as file:
                line_no = 1
                while True:
                    line = file.readline()
                    if not line:
                        break
                    line = line.rstrip("\n")
                    match = re.fullmatch("(\s*#\s*include\s*<(archon/.*)>\s*)", line)
                    if match:
                        line         = match.group(1)
                        incl_subpath = match.group(2)
                        inclusions.append(InclusionCache.Inclusion(line_no, incl_subpath, line))
                    line_no += 1

        self._inclusions[incl_path] = inclusions
        return inclusions

    #        
    #
    class Inclusion:
        def __init__(self, line_no, incl_path, line):
            self.line_no = line_no     # Line number in includer
            self.incl_path = incl_path # Include path of included header
            self.line = line

    # This function returns a list of reference header objects
    # (`PathMapper.ReferenceHeader`). If `recurse` is false, this list contains only the
    # specified reference header (`header_path`). Otherwise it contains all the reference
    # headers in the reference closure of the specified reference header. When multiple
    # reference headers are returned, they are returned in depth-first post-order (child
    # before parent).
    #
    def get_reference_headers(self, header_path, recurse):
        reference_headers = []
        if recurse:
            marked = set() # Cycle and duplicate detection
            def visit(header_path):
                for inclusion in self.get_inclusions(header_path):
                    header_path_2 = self._path_mapper.from_include_path(inclusion.incl_path)
                    reference_header = self._path_mapper.try_get_as_reference_header(header_path_2)
                    if reference_header:
                        if inclusion.incl_path in marked:
                            return
                        marked.add(inclusion.incl_path)
                        visit(header_path_2)
                        reference_headers.append(reference_header)
            marked.add(self._path_mapper.to_include_path(header_path))
            visit(header_path)
        reference_headers.append(self._path_mapper.get_as_reference_header(header_path))
        return reference_headers

    # If `header_path` refers to a reference header, then this function returns the
    # corresponding recursivly expanded reference order. The order is returned in the form
    # of a list of include paths. The specified header and any directly or indirectly
    # included reference headers are not themselves included in this list.
    #
    def get_reference_order(self, header_path):
        incl_paths = []
        marked = set() # Cycle and duplicate detection
        marked.add(self._path_mapper.to_include_path(header_path))
        def expand(header_path):
            for inclusion in inclusion_cache.get_inclusions(header_path):
                if inclusion.incl_path in marked:
                    continue
                marked.add(inclusion.incl_path)
                header_path_2 = path_mapper.from_include_path(inclusion.incl_path)
                if self._path_mapper.is_reference_header(header_path_2):
                    expand(header_path_2)
                    continue
                incl_paths.append(inclusion.incl_path)
        expand(header_path)
        return incl_paths

    #        
    #
    def invalidate(self, header_path):
        incl_path = self._path_mapper.to_include_path(header_path)
        self._inclusions.pop(incl_path, None)

    #        
    #
    def sort_topologically(self, incl_paths):
        root_set = set()
        for incl_path in incl_paths:
            root_set.add(incl_path)
        incl_paths_2 = []
        done = set()
        marked = set() # Cycle detection
        def traverse(incl_path):
            if incl_path in done:
                return
            marked.add(incl_path)
            for inclusion in self.get_inclusions(self._path_mapper.from_include_path(incl_path)):
                assert inclusion.incl_path not in marked
                traverse(inclusion.incl_path)
            if incl_path in root_set:
                incl_paths_2.append(incl_path)
            marked.remove(incl_path)
            done.add(incl_path)
        for incl_path in incl_paths:
            traverse(incl_path)
        return incl_paths_2

    #        
    #
    def get_subdir_exclusions(self, header_path):
        reference_header = self._path_mapper.get_as_reference_header(header_path)
        exclusions = [] # Include paths
        for header_path_2 in self._file_structure_cache.find_header_files(reference_header.assoc_dir_path):
            reference_header_2 = self._path_mapper.try_get_as_reference_header(header_path_2)
            if not reference_header_2:
                continue
            if self._path_mapper.are_same_paths(header_path_2, header_path):
                continue
            dir_path = reference_header_2.assoc_dir_path
            skip = False
            remove = set() # Include paths
            for incl_path in exclusions:
                dir_path_2 = self._path_mapper.from_include_path(incl_path)
                assert not self._path_mapper.are_same_paths(dir_path_2, dir_path)
                if self._path_mapper.is_prefix_path(dir_path_2, dir_path):
                    skip = True
                    break
                if self._path_mapper.is_prefix_path(dir_path, dir_path_2):
                    remove.add(incl_path)
            assert not (skip and remove)
            if not skip:
                exclusions = [ incl_path for incl_path in exclusions if incl_path not in remove ]
                exclusions.append(self._path_mapper.to_include_path(dir_path))

        return [ self._path_mapper.from_include_path(incl_path) for incl_path in exclusions ]



# This function performs the following check: If R is the specified reference header
# (`header_path`) and D is the domain of R, check that all header files in D are directly
# included in R.
#
# If `recurse` is true, this check is performed for all reference headers in the reference
# closure of the specified reference header.
#
# If `insert` is true, this function automatically inserts the missing include statements
# into the reference header. Such a change needs to be reviewed and adjusted by a human
# operator.
#
# FIXME: Should this function also check that everything directly included by the reference header is in its domain or is another reference header                            
#
def check_reference_cover(header_path, recurse, insert, path_mapper, file_structure_cache, inclusion_cache):
    error_occurred = False
    for reference_header in inclusion_cache.get_reference_headers(header_path, recurse):
        subdir_exclusions = set()
        subdir_exclusions_alt = []
        for path in inclusion_cache.get_subdir_exclusions(reference_header.header_path):
            subdir_exclusions.add(path_mapper.to_include_path(path))
            subdir_exclusions_alt.append(os.path.relpath(path, reference_header.assoc_dir_path))

        if not subdir_exclusions_alt:
            print("Checking coverage of %r by reference header <%s>" %
                  (reference_header.assoc_dir_path, path_mapper.to_include_path(reference_header.header_path)))
        else:
            print("Checking coverage of %r by reference header <%s> with subdir exclusions %s" %
                  (reference_header.assoc_dir_path, path_mapper.to_include_path(reference_header.header_path),
                   subdir_exclusions_alt))

        coverage = set()
        for inclusion in inclusion_cache.get_inclusions(reference_header.header_path):
            header_path_2 = path_mapper.from_include_path(inclusion.incl_path)
            if not path_mapper.is_reference_header(header_path_2):
                coverage.add(inclusion.incl_path)

        missing_inclusions = []
        subdir_filter = lambda name, path: path_mapper.to_include_path(path) not in subdir_exclusions
        for header_path_2 in file_structure_cache.find_header_files(reference_header.assoc_dir_path, subdir_filter):
            if path_mapper.is_reference_header(header_path_2):
                continue
            incl_path = path_mapper.to_include_path(header_path_2)
            if incl_path in coverage:
                continue
            missing_inclusions.append(incl_path)
            if not insert:
                print("ERROR: Uncovered header file <%s>" % incl_path)
                error_occurred = True

        if insert and missing_inclusions and not error_occurred:
            sorted_missing_inclusions = inclusion_cache.sort_topologically(missing_inclusions)
            with open(reference_header.header_path, "a") as file:
                file.write("\n// Inserted\n")
                for incl_path in sorted_missing_inclusions:
                    print("Inserting missing inclusion <%s>" % incl_path)
                    file.write("#include <%s>\n" % incl_path)
                inclusion_cache.invalidate(reference_header.header_path)

    if error_occurred:
        print("FAILED")
        return False

    print("Passed")
    return True



# This function checks that the order of header inclusions in the specified reference header
# (`header_path`), i.e., the reference order, is a topological order. This is not a
# requirement for the check of inclusion order to work (`check_include_order()`), but it is
# deemed to be a desirable property of the reference order from the perspective of a human
# reader.
#
# Here, *topological order* means that when a header file, H, is included in another header
# file, R, then H was not already directly or indirectly included by a preceding include
# statement in R. See also `sort_topologically()` of `InclusionCache`.
#
# If `recurse` is true, this check is performed for all reference headers in the reference
# closure of the specified reference header.
#
# If `verbose` is true, a log statement is generated for every inclusion in every checked
# reference header.
#
# FIXME: Would it be possible to have a "auto fix" option for this function?                           
#
def check_reference_order(header_path, recurse, verbose, path_mapper, inclusion_cache):
    class Includer:
        def __init__(self, incl_path, line_no):
            self.incl_path = incl_path # Include path of includer
            self.line_no = line_no     # Line number in includer

    error_occurred = False
    for reference_header in inclusion_cache.get_reference_headers(header_path, recurse):
        print("Checking topological order in reference header <%s>" %
              path_mapper.to_include_path(reference_header.header_path))
        marked = {} # Maps includers of current traversal path to inclusion depth (cycle detection)
        curr_inclusions = [] # Inclusions corresponding to current traversal path
        first_includers = {} # Maps included headers to their first includer
        def traverse(header_path, at_root):
            nonlocal error_occurred
            incl_path = path_mapper.to_include_path(header_path)
            marked[incl_path] = len(curr_inclusions)
            for inclusion in inclusion_cache.get_inclusions(header_path):
                curr_inclusions.append(inclusion)
                offset = marked.get(inclusion.incl_path)
                if offset is not None:
                    n = len(curr_inclusions) - offset
                    print("%s:%s: FATAL: Include cycle of length %s detected:" % (header_path, inclusion.line_no, n))
                    incl_path_2 = inclusion.incl_path
                    for i in range(n):
                        inclusion_2 = curr_inclusions[offset + i]
                        print("%s:%s: Cycle (%s/%s): Inclusion of <%s>" %
                              (path_mapper.from_include_path(incl_path_2), inclusion_2.line_no, i + 1, n,
                               inclusion_2.incl_path))
                        incl_path_2 = inclusion_2.incl_path
                    error_occurred = True
                    return False
                if verbose and at_root:
                    print("%s:%s: Checking inclusion of <%s>" % (header_path, inclusion.line_no, inclusion.incl_path))
                includer = first_includers.get(inclusion.incl_path)
                if includer:
                    if at_root:
                        print("%s:%s: ERROR: Improper dependency ordering: <%s> was already directly or indirectly "
                              "included as follows:" % (header_path, inclusion.line_no, inclusion.incl_path))
                        rev_inclusions = []
                        incl_path_2 = inclusion.incl_path
                        while True:
                            rev_inclusions.append(InclusionCache.Inclusion(includer.line_no, incl_path_2, ""))
                            incl_path_2 = includer.incl_path
                            if incl_path_2 == incl_path:
                                break
                            includer = first_includers.get(incl_path_2)
                        incl_path_2 = incl_path
                        n = len(rev_inclusions)
                        for i in range(n):
                            inclusion_2 = rev_inclusions[n - 1 - i]
                            print("%s:%s: Step (%s/%s): Inclusion of <%s>" %
                                  (path_mapper.from_include_path(incl_path_2), inclusion_2.line_no, i + 1, n,
                                   inclusion_2.incl_path))
                            incl_path_2 = inclusion_2.incl_path
                        error_occurred = True
                else:
                    first_includers[inclusion.incl_path] = Includer(incl_path, inclusion.line_no)
                    if not traverse(path_mapper.from_include_path(inclusion.incl_path), False):
                        return False
                curr_inclusions.pop()
            del marked[incl_path]
            return True
        if not traverse(reference_header.header_path, True):
            break
    if error_occurred:
        print("FAILED")
        return False
    print("Passed")
    return True



# This function checks that the order of inclusion of local header files is in agreement
# with the order in the specified reference header (`header_path`). It does this for all
# source files in the domain of that reference header. When the specified reference header
# includes other reference headers, the total reference order corresponds to a depth-first
# traversal of the tree of reference headers.
#
# If `verbose` is true, a log statement is generated for every checked source file.
#
# If `fixup` is true, this function automatically fixes the inclusion order in those source
# files where it is found to be wrong.
#
# FIXME: Explain how the "fixup" feature requires extra regularity of inclusions                                             
#
def check_include_order(header_path, verbose, fixup, path_mapper, file_structure_cache, inclusion_cache):
    reference_header = path_mapper.get_as_reference_header(header_path)

    subdir_exclusions = set()
    subdir_exclusions_alt = []
    for path in inclusion_cache.get_subdir_exclusions(header_path):
        subdir_exclusions.add(path_mapper.to_include_path(path))
        subdir_exclusions_alt.append(os.path.relpath(path, reference_header.assoc_dir_path))

    if not subdir_exclusions_alt:
        print("Checking include order in %r against reference order <%s>" %
              (reference_header.assoc_dir_path, path_mapper.to_include_path(header_path)))
    else:
        print("Checking include order in %r against reference order <%s> with subdir exclusions %s" %
              (reference_header.assoc_dir_path, path_mapper.to_include_path(header_path), subdir_exclusions_alt))

    headers = {}
    index = 0
    for incl_path in inclusion_cache.get_reference_order(header_path):
        headers[incl_path] = index
        index += 1

    class Rewrite:
        def __init__(self, source_path, inclusions):
            self.source_path = source_path
            self.inclusions = inclusions

    error_occurred = False
    rewrites = []
    subdir_filter = lambda name, path: path_mapper.to_include_path(path) not in subdir_exclusions
    for source_path in file_structure_cache.find_source_files(reference_header.assoc_dir_path, subdir_filter):
        if path_mapper.is_reference_header(source_path):
            continue
        if verbose:
            print("Checking %r" % source_path)
        inclusions = inclusion_cache.get_inclusions(source_path)
        noncontiguity_line_no = None
        if inclusions:
            prev_line_no = inclusions[0].line_no
            for inclusion in inclusions[1:]:
                if inclusion.line_no != prev_line_no + 1:
                    noncontiguity_line_no = inclusion.line_no
                    break
                prev_line_no = inclusion.line_no
        need_rewrite = False
        prev_index = None
        for inclusion in inclusions:
            index = headers.get(inclusion.incl_path)
            if index is None:
                print("%s:%s: ERROR: Inclusion of <%s> not covered by reference order" %
                      (source_path, inclusion.line_no, inclusion.incl_path))
                error_occurred = True
            else:
                if prev_index is not None and index <= prev_index:
                    if fixup:
                        need_rewrite = True
                    else:
                        for inclusion_2 in inclusions:
                            index_2 = headers.get(inclusion_2.incl_path)
                            if index_2 > index:
                                print("%s:%s: ERROR: Out of order inclusion of <%s> (must be included before <%s>)" %
                                      (source_path, inclusion.line_no, inclusion.incl_path, inclusion_2.incl_path))
                        error_occurred = True
                prev_index = index
        if need_rewrite:
            if noncontiguity_line_no is None:
                rewrites.append(Rewrite(source_path, inclusions))
            else:
                print("%s:%s: ERROR: Unable to rewrite because inclusions are non-contiguous" %
                      (source_path, noncontiguity_line_no))
                error_occurred = True
    if error_occurred:
        print("FAILED")
        return False
    for rewrite in rewrites:
        print("Rewriting %s" % rewrite.source_path)
        copy_file(rewrite.source_path, rewrite.source_path + "~")
        def transfer(origin_file, temp_file):
            begin = rewrite.inclusions[+0].line_no
            end   = rewrite.inclusions[-1].line_no + 1
            for i in range(1, begin):
                line = origin_file.readline()
                assert line
                temp_file.write(line)
            for i in range(begin, end):
                line = origin_file.readline()
                assert line
            for inclusion in sorted(rewrite.inclusions, key = lambda i: headers[i.incl_path]):
                temp_file.write("%s\n" % (inclusion.line))
            while True:
                line = origin_file.readline()
                if not line:
                    break
                temp_file.write(line)
        transform_file(rewrite.source_path, transfer)
        inclusion_cache.invalidate(rewrite.source_path)
    print("Passed")
    return True


def check(reference_header_path, source_root_path):
    pass
    # for path in find_files(source_root_path, [ ".hpp" ]):
    #     incl_path = to_include_path(path)
    #     if not ("/" + incl_path).endswith("/noinst/everything.hpp"):
    #         continue
    #     print("----------------->>> %s" % incl_path)  
    
    # # Find all `noinst/everything.hpp` anywhere under include root
    # # Verify that `<archon/noinst/everything.hpp>` is there
    # # For each of them:
    # #   Check reference coverage
    # # For each of them:
    # #   Check reference order
    # # Check include order
    # return true



if len(sys.argv) < 2:
    prefix = "%s %s" % (sys.orig_argv[0], self_path)
    sys.stdout.write(f"""\
Usage:  {prefix}  check [-v] [-f]  <header path>
   or:  {prefix}  check-reference-cover  [-r] [-i]  <header path>
   or:  {prefix}  check-reference-order  [-r] [-v]  <header path>
   or:  {prefix}  check-include-order  [-v] [-f]  <header path>
""")
    sys.exit(0)

def parse_commandline(argv, offset, options, min_args, max_args):
    option_values = len(options) * [ False ]
    for i in range(len(options)):
        if len(argv) > offset and argv[offset] == options[i]:
            option_values[i] = True
            offset += 1
    num_remain = len(argv) - offset
    if num_remain < min_args:
        print("ERROR: Too few commandline arguments")
        sys.exit(1)
    if num_remain > max_args:
        print("ERROR: Too many commandline arguments")
        sys.exit(1)
    return option_values + argv[offset:] + (max_args - num_remain) * [ None ]

func = sys.argv[1]
if func == "check":
    reference_header_path = from_include_path("archon/noinst/everything.hpp")
    source_root_path = abs_src_path
    if not check(reference_header_path, source_root_path):
        sys.exit(1)
elif func == "check-reference-cover":
    recurse, insert, header_path = parse_commandline(sys.argv, 2, [ "-r", "-i" ], 1, 1)
    path_mapper = PathMapper(abs_src_path)
    if not path_mapper.can_be_mapped_to_include_path(header_path):
        root_path = path_mapper.from_include_path("")
        print("ERROR: Header path (%r) must be inside source root (%r)" % (header_path, root_path))
        sys.exit(1)
    if not path_mapper.is_reference_header(header_path):
        print("ERROR: Header path (%r) must refer to reference header" % header_path)
        sys.exit(1)
    file_structure_cache = FileStructureCache(path_mapper)
    inclusion_cache = InclusionCache(path_mapper, file_structure_cache)
    if not check_reference_cover(header_path, recurse, insert, path_mapper, file_structure_cache, inclusion_cache):
        sys.exit(1)
elif func == "check-reference-order":
    recurse, verbose, header_path, = parse_commandline(sys.argv, 2, [ "-r", "-v" ], 1, 1)
    path_mapper = PathMapper(abs_src_path)
    if not path_mapper.can_be_mapped_to_include_path(header_path):
        root_path = path_mapper.from_include_path("")
        print("ERROR: Header path (%r) must be inside source root (%r)" % (header_path, root_path))
        sys.exit(1)
    if not path_mapper.is_reference_header(header_path):
        print("ERROR: Header path (%r) must refer to reference header" % header_path)
        sys.exit(1)
    file_structure_cache = FileStructureCache(path_mapper)
    inclusion_cache = InclusionCache(path_mapper, file_structure_cache)
    if not check_reference_order(header_path, recurse, verbose, path_mapper, inclusion_cache):
        sys.exit(1)
elif func == "check-include-order":
    verbose, fixup, header_path = parse_commandline(sys.argv, 2, [ "-v", "-f" ], 1, 1)
    path_mapper = PathMapper(abs_src_path)
    if not path_mapper.can_be_mapped_to_include_path(header_path):
        root_path = path_mapper.from_include_path("")
        print("ERROR: Header path (%r) must be inside source root (%r)" % (header_path, root_path))
        sys.exit(1)
    if not path_mapper.is_reference_header(header_path):
        print("ERROR: Header path (%r) must refer to reference header" % header_path)
        sys.exit(1)
    file_structure_cache = FileStructureCache(path_mapper)
    inclusion_cache = InclusionCache(path_mapper, file_structure_cache)
    if not check_include_order(header_path, verbose, fixup, path_mapper, file_structure_cache, inclusion_cache):
        sys.exit(1)
else:
    print("ERROR: Unknown function '%s'" % func)
    sys.exit(1)
