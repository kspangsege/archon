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

# Returns true iff `path_1` is a equal to, or a prefix of `path_2`
def is_prefix_path(path_1, path_2):
    return os.path.commonpath([path_1, path_2]) == os.path.commonpath([path_1, path_1])

def to_include_path(path):
    path_2 = os.path.abspath(path)
    assert is_prefix_path(abs_src_path, path_2)
    return os.path.relpath(path_2, abs_src_path)

def from_include_path(path):
    return os.path.relpath(os.path.join(abs_src_path, path))

class Inclusion:
    def __init__(self, line_no, incl_path, line):
        self.line_no   = line_no   # Line number in includer
        self.incl_path = incl_path # Include path of included header
        self.line      = line

class Includer:
    def __init__(self, incl_path, line_no):
        self.incl_path = incl_path # Include path of includer
        self.line_no   = line_no   # Line number in includer

def get_inclusions(header_path):
    inclusions = []
    if os.path.basename(header_path) != "config.h":
        with open(header_path, "r") as file:
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
                    inclusions.append(Inclusion(line_no, incl_subpath, line))
                line_no += 1
    return inclusions

def get_include_order(header_path):
    incl_paths = []
    def expand(header_path):
        dir_path = os.path.dirname(header_path)
        for inclusion in get_inclusions(header_path):
            header_path_2 = from_include_path(inclusion.incl_path)
            if os.path.basename(header_path_2) == "everything.hpp":
                expand(header_path_2)
            else:
                incl_paths.append(inclusion.incl_path)
    expand(header_path)
    return incl_paths

def find_files(dir_path, suffixes, subdir_filter = None):
    paths = []
    def traverse(path):
        names = os.listdir(dir_path)
        for name in os.listdir(path):
            subpath = os.path.join(path, name)
            if os.path.isfile(subpath):
                ext = os.path.splitext(subpath)[1]
                if ext in suffixes:
                    paths.append(subpath)
            elif os.path.isdir(subpath):
                if name != "attic" and (subdir_filter is None or subdir_filter(name)):
                    traverse(subpath)
    traverse(dir_path)
    return paths

def find_header_files(dir_path, subdir_filter = None):
    return find_files(dir_path, [ ".h", ".hpp" ], subdir_filter)

def find_source_files(dir_path, subdir_filter = None):
    return find_files(dir_path, [ ".h", ".c", ".hpp", ".cpp" ], subdir_filter)

def sort_topologically(incl_paths):
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
        for inclusion in get_inclusions(from_include_path(incl_path)):
            assert inclusion.incl_path not in marked
            traverse(inclusion.incl_path)
        if incl_path in root_set:
            incl_paths_2.append(incl_path)
        marked.remove(incl_path)
        done.add(incl_path)
    for incl_path in incl_paths:
        traverse(incl_path)
    return incl_paths_2



def check_dependency_ordering(header_path):
    root_incl_path = to_include_path(header_path)
    marked = {} # Maps includers of current traversal path to inclusion depth (cycle detection)
    curr_inclusions = [] # List of current inclusions
    first_includers = {} # Maps included headers to their first includer
    error_occurred = False
    def traverse(incl_path):
        nonlocal error_occurred
        at_root = incl_path == root_incl_path
        marked[incl_path] = len(curr_inclusions)
        for inclusion in get_inclusions(from_include_path(incl_path)):
            curr_inclusions.append(inclusion)
            offset = marked.get(inclusion.incl_path)
            if offset is not None:
                n = len(curr_inclusions) - offset
                print("%s:%s: FATAL: Include cycle of length %s detected:" %
                      (from_include_path(incl_path), inclusion.line_no, n))
                incl_path_2 = inclusion.incl_path
                for i in range(n):
                    inclusion_2 = curr_inclusions[offset + i]
                    print("%s:%s: Cycle (%s/%s): Inclusion of <%s>" %
                          (from_include_path(incl_path_2), inclusion_2.line_no, i + 1, n, inclusion_2.incl_path))
                    incl_path_2 = inclusion_2.incl_path
                error_occurred = True
                return False
            if at_root:
                print("%s:%s: Checking inclusion of <%s>" %
                      (from_include_path(incl_path), inclusion.line_no, inclusion.incl_path))
            includer = first_includers.get(inclusion.incl_path)
            if includer:
                if at_root:
                    print("%s:%s: ERROR: Improper dependency ordering: <%s> was already directly or indirectly "
                          "included as follows:" %
                          (from_include_path(incl_path), inclusion.line_no, inclusion.incl_path))
                    rev_inclusions = []
                    incl_path_2 = inclusion.incl_path
                    while True:
                        rev_inclusions.append(Inclusion(includer.line_no, incl_path_2, ""))
                        incl_path_2 = includer.incl_path
                        if incl_path_2 == incl_path:
                            break
                        includer = first_includers.get(incl_path_2)
                    incl_path_2 = incl_path
                    n = len(rev_inclusions)
                    for i in range(n):
                        inclusion_2 = rev_inclusions[n - 1 - i]
                        print("%s:%s: Step (%s/%s): Inclusion of <%s>" %
                              (from_include_path(incl_path_2), inclusion_2.line_no, i + 1, n, inclusion_2.incl_path))
                        incl_path_2 = inclusion_2.incl_path
                    error_occurred = True
            else:
                first_includers[inclusion.incl_path] = Includer(incl_path, inclusion.line_no)
                if not traverse(inclusion.incl_path):
                    return False
            curr_inclusions.pop()
        del marked[incl_path]
        return True
    traverse(root_incl_path)
    if error_occurred:
        print("FAILED")
        return False
    print("Passed")
    return True


def check_coverage(header_path, dir_path, exclude_test, insert):
    coverage = set()
    for incl_path in get_include_order(header_path):
        coverage.add(incl_path)
    incl_path = to_include_path(header_path)
    error_occurred = False
    inserts = []
    subdir_filter = None
    if exclude_test:
        subdir_filter = lambda name: name != "test"
    for header_path_2 in find_header_files(dir_path, subdir_filter):
        if os.path.basename(header_path_2) != "everything.hpp":
            incl_path_2 = to_include_path(header_path_2)
            if incl_path_2 == incl_path:
                continue
            if incl_path_2 not in coverage:
                if insert:
                    inserts.append(incl_path_2)
                else:
                    print("ERROR: Uncovered header file <%s>" % incl_path_2)
                    error_occurred = True
    if error_occurred:
        print("FAILED")
        return False
    if inserts:
        sorted_inserts = sort_topologically(inserts)
        with open(header_path, "a") as file:
            file.write("\n// Inserted\n")
            for incl_path_2 in sorted_inserts:
                print("Inserting missing inclusion <%s>" % incl_path_2)
                file.write("#include <%s>\n" % incl_path_2)
    print("Passed")
    return True


def check_include_order(header_path, dir_path, rewrite):
    headers = {}
    index = 0
    for incl_path in get_include_order(header_path):
        headers[incl_path] = index
        index += 1
    class Rewrite:
        def __init__(self, header_path, inclusions):
            self.header_path = header_path
            self.inclusions  = inclusions
    error_occurred = False
    rewrites = []
    for source_path in find_source_files(dir_path):
        if os.path.basename(source_path) != "everything.hpp":
            print("Checking %s" % source_path)
            inclusions = get_inclusions(source_path)
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
                    print("%s:%s: ERROR: Uncovered inclusion <%s>" %
                          (source_path, inclusion.line_no, inclusion.incl_path))
                    error_occurred = True
                else:
                    if prev_index is not None and index <= prev_index:
                        if rewrite:
                            need_rewrite = True
                        else:
                            print("%s:%s: ERROR: Out of order inclusion <%s>" %
                                  (source_path, inclusion.line_no, inclusion.incl_path))
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
        print("Rewriting %s" % rewrite.header_path)
        copy_file(rewrite.header_path, rewrite.header_path + "~")
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
        transform_file(rewrite.header_path, transfer)
    print("Passed")
    return True


if len(sys.argv) < 2:
    prefix = "%s %s" % (sys.orig_argv[0], self_path)
    sys.stdout.write(f"""\
Usage:  {prefix}  order  <header path>
   or:  {prefix}  cover  [-e]  [-i]  <header path>  <dir path>
   or:  {prefix}  check  [-r]  <header path>  <dir path>
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
if func == "order":
    header_path, = parse_commandline(sys.argv, 2, [], 1, 1)
    if not check_dependency_ordering(header_path):
        sys.exit(1)
elif func == "cover":
    exclude_test, insert, header_path, dir_path = parse_commandline(sys.argv, 2, [ "-e", "-i" ], 2, 2)
    if not check_coverage(header_path, dir_path, exclude_test, insert):
        sys.exit(1)
elif func == "check":
    rewrite, header_path, dir_path = parse_commandline(sys.argv, 2, [ "-r" ], 2, 2)
    if not check_include_order(header_path, dir_path, rewrite):
        sys.exit(1)
else:
    print("ERROR: Unknown function '%s'" % func)
    sys.exit(1)
