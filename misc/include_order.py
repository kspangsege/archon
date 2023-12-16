import re
import sys
import os
import tempfile
import enum

self_path = sys.argv[0]
assert os.path.basename(self_path) == "include_order.py"


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
def is_ansi_term(output_stream):
    if output_stream.isatty():
        return sys.platform != "win32" or "ANSICON" in os.environ
    return False

#         
#
class AnsiTermColor(enum.Enum):
    BLACK   = 0
    RED     = 1
    GREEN   = 2
    YELLOW  = 3
    BLUE    = 4
    MAGENTA = 5
    CYAN    = 6
    WHITE   = 7

#         
#
def ansi_term_set_color(color):
    if color == AnsiTermColor.BLACK:
        return "\033[30m"
    if color == AnsiTermColor.RED:
        return "\033[31m"
    if color == AnsiTermColor.GREEN:
        return "\033[32m"
    if color == AnsiTermColor.YELLOW:
        return "\033[33m"
    if color == AnsiTermColor.BLUE:
        return "\033[34m"
    if color == AnsiTermColor.MAGENTA:
        return "\033[35m"
    if color == AnsiTermColor.CYAN:
        return "\033[36m"
    if color == AnsiTermColor.WHITE:
        return "\033[37m"
    assert False

#         
#
def ansi_term_reset_color():
    return "\033[39m"



#         
#
class LogLevel(enum.Enum):
    FATAL  = 1
    ERROR  = 2
    WARN   = 3
    INFO   = 4
    DETAIL = 5
    DEBUG  = 6
    TRACE  = 7


#         
#
class Logger:
    def fatal(self, pattern, *args):
        self.log(LogLevel.FATAL, pattern, *args)

    def error(self, pattern, *args):
        self.log(LogLevel.ERROR, pattern, *args)

    def warn(self, pattern, *args):
        self.log(LogLevel.WARN, pattern, *args)

    def info(self, pattern, *args):
        self.log(LogLevel.INFO, pattern, *args)

    def detail(self, pattern, *args):
        self.log(LogLevel.DETAIL, pattern, *args)

    def debug(self, pattern, *args):
        self.log(LogLevel.DEBUG, pattern, *args)

    def trace(self, pattern, *args):
        self.log(LogLevel.TRACE, pattern, *args)

    def log(self, level, pattern, *args):
        prefix = ""
        message = pattern % args
        self._log(level, prefix, message)


#         
#
class RootLogger(Logger):
    def __init__(self, output_stream = sys.stdout):
        self._output_stream = output_stream
        self._is_ansi_term = is_ansi_term(self._output_stream)

    def _log(self, level, prefix, message):
        label = None
        color = None
        if level == LogLevel.FATAL:
            label = "FATAL"
            color = AnsiTermColor.RED
        elif level == LogLevel.ERROR:
            label = "ERROR"
            color = AnsiTermColor.RED
        elif level == LogLevel.WARN:
            label = "WARNING"
            color = AnsiTermColor.YELLOW
        if label and color is not None and self._is_ansi_term:
            label = ansi_term_set_color(color) + label + ansi_term_reset_color()
        message_2 = message
        if label:
            message_2 = "%s: %s" % (label, message)
        self._output_stream.write("%s%s\n" % (prefix, message_2))


#         
#
class LimitLogger(Logger):
    def __init__(self, base_logger, limit_level):
        self._base_logger = base_logger
        self._limit_level = limit_level

    def _log(self, level, prefix, message):
        if level.value <= self._limit_level.value:
            self._base_logger._log(level, prefix, message)


#         
#
class FileContextLogger(Logger):
    def __init__(self, base_logger, path, line_no = 0):
        self._base_logger = base_logger
        self._path = path
        self._line_no = line_no

    def set_line_no(self, line_no):
        self._line_no = line_no

    def for_alt_line(self, line_no):
        return self.for_alt_context(self._path, line_no)

    def for_alt_context(self, path, line_no):
        return FileContextLogger(self._base_logger, path, line_no)

    def _log(self, level, prefix, message):
        prefix_2 = "%s:%s: %s" % (self._path, self._line_no, prefix)
        self._base_logger._log(level, prefix_2, message)



#        
#
# FIXME: Clarify that a path to the source directory or to any file or directory inside the source directory can be mapped to its corresponding include path. The result of mapping the source directory path to an include path is the empty string. Correspondingly, if the empty string is taken as an include path and mapped back to a regular path, the result is the path to the source diretory.
#
class PathMapper:
    def __init__(self, source_root_path):
        self._src_root_path = source_root_path
        self._abs_src_root_path = os.path.abspath(self._src_root_path)

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
        return os.path.join(self._src_root_path, incl_path)

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

    # This function returns true if, and only if the two paths refer to the same file or
    # subdirectory inside the source root. This includes the case where they both refer to
    # the source root. It is an error if either path refers to somting outside the source
    # root.
    #
    def are_same_paths(self, path_1, path_2):
        return self.to_include_path(path_1) == self.to_include_path(path_2)

    # This function returns true if, and only if the two paths refer to files or
    # subdirectories in the same subdirectory under the source root. This includes the case
    # where they refer to the same file or subdirectory, and also the case where they both
    # refer to the source root. It is an error if either path refers to somting outside the
    # source root.
    #
    def are_sibling_paths(self, path_1, path_2):
        incl_path_1 = self.to_include_path(path_1)
        incl_path_2 = self.to_include_path(path_2)
        if incl_path_1 == incl_path_2:
            return True
        if not incl_path_1 or not incl_path_2:
            return False
        return os.path.dirname(incl_path_1) == os.path.dirname(incl_path_2)

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

        if len(segments) < 1:
            return None

        is_full = False
        if segments[-1] == "everything.hpp":
            is_full = True
            pass
        elif segments[-1].startswith("everything_") and segments[-1].endswith(".hpp"):
            pass
        else:
            return None

        cutoff = -1
        if len(segments) >= 2 and segments[-2] == "noinst":
            cutoff = -2

        assoc_dir_path = self.from_include_path(join_rel_path(segments[:cutoff]))
        return PathMapper.ReferenceHeader(header_path, assoc_dir_path, is_full)

    # This function returns true when the specified path is in the specified domain, that
    # is, when the path is in the root of the domain but not in any of the exluded
    # subdirectories of the domain.
    #
    def domain_contains(self, domain, path):
        if not self.is_prefix_path(domain.root_path, path):
            return False
        for excluded_path in domain.subdir_exclusions:
            if self.is_prefix_path(excluded_path, path):
                return False
        return True

    # This function returns a string that describes the domain. It has the form `<root
    # path>` or `<root path> with subdir exclusions [<relative subdir path>]`. Here,
    # `<relative subdir path>` is the path to an excluded subdirectory expressed relative to
    # the root path.
    #
    def describe_domain(self, domain):
        subdir_exclusions = []
        for path in domain.subdir_exclusions:
            subdir_exclusions.append(os.path.relpath(path, domain.root_path))
        if not subdir_exclusions:
            return "%r" % domain.root_path
        return "%r with subdir exclusions %s" % (domain.root_path, subdir_exclusions)

    #        
    #
    class ReferenceHeader:
        def __init__(self, header_path, assoc_dir_path, is_full):
            self.header_path = header_path       # Path to reference header file
            self.assoc_dir_path = assoc_dir_path # Path to associated source (sub)directory
            self.is_full = is_full               # Not a partial reference header

    #        
    #
    class Domain:
        def __init__(self, root_path, subdir_exclusions):
            self.root_path = root_path                 # Path to root directory of domain
            self.subdir_exclusions = subdir_exclusions # List of paths of excluded subdirectories



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
    def find_header_files(self, dir_path):
        return self.find_files(dir_path, [ ".h", ".hpp" ])

    #        
    #
    def find_files_in_domain(self, domain, suffixes):
        subdir_exclusions = set()
        for path in domain.subdir_exclusions:
            subdir_exclusions.add(self._path_mapper.to_include_path(path))
        subdir_filter = lambda name, path: self._path_mapper.to_include_path(path) not in subdir_exclusions
        return self.find_files(domain.root_path, suffixes, subdir_filter)

    #        
    #
    def find_header_files_in_domain(self, domain):
        return self.find_files_in_domain(domain, [ ".h", ".hpp" ])

    #        
    #
    def find_source_files_in_domain(self, domain):
        return self.find_files_in_domain(domain, [ ".h", ".c", ".hpp", ".cpp" ])

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



# FIXME: Tend to robust cache invalidation                                                 

#        
#
class DomainCache:
    def __init__(self, path_mapper, file_structure_cache):
        self._path_mapper = path_mapper
        self._file_structure_cache = file_structure_cache
        self._domains = {}

    def get(self, header_path):
        incl_path = self._path_mapper.to_include_path(header_path)
        domain = self._domains.get(incl_path)
        if domain:
            return domain
        reference_header = self._path_mapper.get_as_reference_header(header_path)
        domain = PathMapper.Domain(reference_header.assoc_dir_path, self._get_subdir_exclusions(reference_header))
        self._domains[incl_path] = domain
        return domain

    def _get_subdir_exclusions(self, reference_header):
        exclusions = [] # Include paths
        for header_path in self._file_structure_cache.find_header_files(reference_header.assoc_dir_path):
            reference_header_2 = self._path_mapper.try_get_as_reference_header(header_path)
            if not reference_header_2 or not reference_header_2.is_full:
                continue
            if self._path_mapper.are_sibling_paths(header_path, reference_header.header_path):
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



#        
#
class InclusionCache:
    def __init__(self, path_mapper, file_structure_cache):
        self._path_mapper = path_mapper
        self._file_structure_cache = file_structure_cache
        self._inclusions = {}

    # This function returns a list of inclusion objects (`Inclusion`) corresponding to
    # include statements in the specified source file (`source_path`). Only those include
    # statements that match a certian pattern (see below) will be included. They are
    # returned in the order that they occur in the source file.
    #
    # Include statements will be include if, and only if they are on the form `#include
    # <archon/**/*.h>` or `#include <archon/**/*.hpp>` with arbitrary space allowed before
    # and after `#` and before `<`.
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
                    match = re.fullmatch("(\s*#\s*include\s*<(archon/.*)>\s*)", line) # FIXME: Find a way to make this configurable                                                                      
                    if match:
                        line         = match.group(1)
                        incl_subpath = match.group(2)
                        inclusions.append(InclusionCache.Inclusion(line_no, incl_subpath, line))
                    line_no += 1

        self._inclusions[incl_path] = inclusions
        return inclusions

    # This class represent an include statement as found in some source file.
    #
    class Inclusion:
        def __init__(self, line_no, incl_path, line):
            self.line_no = line_no     # Line number in includer
            self.incl_path = incl_path # Include path of included header
            self.line = line           # Full line of include statement without termination

    # This function returns a list of reference header objects
    # (`PathMapper.ReferenceHeader`) corresponding to full reference headers reachable from
    # the specified full reference header. If `recurse` is false, this list contains only
    # the specified reference header. Otherwise it contains all the full reference headers
    # in its reference closure. When multiple reference headers are returned, they are
    # returned in depth-first post-order (child before parent).
    #
    def get_full_reference_headers(self, reference_header, recurse):
        assert reference_header.is_full
        reference_headers = []
        if recurse:
            marked = set() # Cycle and duplicate detection
            def traverse(reference_header):
                for inclusion in self.get_inclusions(reference_header.header_path):
                    header_path = self._path_mapper.from_include_path(inclusion.incl_path)
                    reference_header_2 = self._path_mapper.try_get_as_reference_header(header_path)
                    if reference_header_2:
                        if inclusion.incl_path in marked:
                            return
                        marked.add(inclusion.incl_path)
                        traverse(reference_header_2)
                        if reference_header_2.is_full:
                            reference_headers.append(reference_header_2)
            marked.add(self._path_mapper.to_include_path(reference_header.header_path))
            traverse(reference_header)
        reference_headers.append(reference_header)
        return reference_headers

    # This function returns a list of include paths corresponding to the reference order
    # defined by the specified reference header (`header_path`), which must be a full
    # reference header. This list does not contain any reference headers.
    #
    # Let R be the specified full reference header. Then, if `local_only` is true, the
    # returned list only covers the part of the reference order that is directly specified
    # by R and R's associated partial reference headers, if any. If `local_only` is false,
    # the list covers the full order as specified by all reference headers in the reference
    # closure of R.
    #
    def get_reference_order(self, header_path, local_only = False):
        reference_header = self._path_mapper.get_as_reference_header(header_path)
        assert reference_header.is_full
        incl_paths = []
        marked = set() # Cycle and duplicate detection
        marked.add(self._path_mapper.to_include_path(header_path))
        def traverse(reference_header):
            for inclusion in self.get_inclusions(reference_header.header_path):
                if inclusion.incl_path in marked:
                    continue
                marked.add(inclusion.incl_path)
                header_path_2 = self._path_mapper.from_include_path(inclusion.incl_path)
                reference_header_2 = self._path_mapper.try_get_as_reference_header(header_path_2)
                if not reference_header_2:
                    incl_paths.append(inclusion.incl_path)
                    continue
                if local_only:
                    if reference_header_2.is_full:
                        continue
                    if not self._path_mapper.are_sibling_paths(header_path_2, header_path):
                        continue
                traverse(reference_header_2)
        traverse(reference_header)
        return incl_paths

    # This function discards any part of the cache that is affected by the contents of the
    # file at the specified path.
    #
    def invalidate(self, header_path):
        incl_path = self._path_mapper.to_include_path(header_path)
        self._inclusions.pop(incl_path, None)

    # This function sorts the specified list of include paths topologically. The include
    # paths are assumed to refer to header files.
    #
    # A list, L, of header files is understood as being in topological order when a header
    # file, H, in L does not occur more than once in L, and also is not directly or                                                                        
    # indirectly included by a header file that occurs before H in L.
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



# This class acts as shared context for the various check functions offered by this script.
#
class Checker:
    def __init__(self, source_root_path, logger):
        self._logger = logger
        self._path_mapper = PathMapper(source_root_path)
        self._file_structure_cache = FileStructureCache(self._path_mapper)
        self._domain_cache = DomainCache(self._path_mapper, self._file_structure_cache)
        self._inclusion_cache = InclusionCache(self._path_mapper, self._file_structure_cache)

    # This function checks that the specified filesystem path refers to a full reference
    # header inside the source root, If it does, this function returns the corresponding
    # reference header object (`PathMapper.ReferenceHeader`). If it does not, this function
    # generates ab appropriate error message and returns `None`.
    #
    def map_path_arg_to_reference_header(self, path):
        if not self._path_mapper.can_be_mapped_to_include_path(path):
            root_path = path_mapper.from_include_path("")
            self._logger.error("Header path (%r) must be inside source root (%r)", path, root_path)
            return None
        reference_header = self._path_mapper.try_get_as_reference_header(path)
        if not reference_header or not reference_header.is_full:
            self._error("Header path (%r) must refer to full reference header", header_path)
            return None
        return reference_header

    # This function runs all available checks for the specified reference header, which must
    # be a full reference header.
    #
    def check(self, reference_header, recurse, fixup):
        if not self.check_reference_consistency(reference_header):
            return False
        insert = False
        if not self.check_reference_coverage(reference_header, recurse, insert):
            return False
        if not self.check_reference_order(reference_header, recurse):
            return False
        if not self.check_include_order(reference_header, recurse, fixup):
            return False
        return True

    #        
    #
    def check_reference_reachability(self, reference_header):
        # FIXME: Check that there is a full reference header for every partial reference header   <------------ or should this actually be required? YES, at least for now                        
        # FIXME: Check that there are no reference headers in different directories but with same associated directory              

        # Gather total set of reference headers reachable through inclusions from the specified one.

        # Keep track of every distinct associated directory

        # Travserse all headers in associated directory of specified header

        # Fail if a reference header is not reachable

        # Fail if an associated directory has more than one full reference header (e.g. if both `foo/everything.hpp` and `foo/noinst/everything.hpp` exists)

        # After end of traversal of all headers in associated directory of specified header, fail if an associated directory has no full reference header (only partial)

        return False

    # This function performs a number of related checks. First and foremost, it checks that
    # the reference order specification, rooted at the specified reference header, is
    # self-consistent (more in this below). Additionally, it checks that there are no
    # inclusion cycles among reference headers, that no reference header includes a regular
    # header outside its domain, and that a regular header is included at most once.
    #
    # If R is a reference header, a necessary condition for self-consistency of the
    # reference order specified by R is that the reference orders specified by any two
    # reference headers in the reference closure of R are compatible. Two reference orders
    # are compatible if they agree on the order of the headers in their intersection.
    #
    # The exact conditions for self-consistency of of the reference order specified by R are
    # as follows:                                                                                                                                                                                                      
    #
    def check_reference_consistency(self, reference_header):
        assert reference_header.is_full
        self._logger.info("Checking consistency of referenece order rooted at <%s>",
                          self._path_mapper.to_include_path(reference_header.header_path))

        class Includer:
            def __init__(self, header_path, line_no):
                self.header_path = header_path # Path to includer
                self.line_no = line_no         # Line number in includer

        marked = set() # Cycle detection
        includers = {}
        errors_occurred = False

        def traverse(reference_header):
            nonlocal errors_occurred
            logger = FileContextLogger(self._logger, reference_header.header_path)
            domain = self._domain_cache.get(reference_header.header_path)
            prev_reference_header_inclusion = None

            def check_regular_inclusion(inclusion, header_path):
                nonlocal errors_occurred
                logger.detail("Checking inclusion of regular header <%s>", inclusion.incl_path)
                if not self._path_mapper.domain_contains(domain, header_path):
                    logger.error("Inclusion of regular header <%s> outside domain (%s)" %
                                 (inclusion.incl_path, self._path_mapper.describe_domain(domain)))
                    errors_occurred = True
                    return True
                includer = includers.get(inclusion.incl_path)
                if includer:
                    logger.error("Multiple inclusions of <%s> (see context below)", inclusion.incl_path)
                    logger_2 = logger.for_alt_context(includer.header_path, includer.line_no)
                    logger_2.error("Context: Prior inclusion of <%s>", inclusion.incl_path)
                    errors_occurred = True
                    return True
                includers[inclusion.incl_path] = Includer(reference_header.header_path, inclusion.line_no)
                return True

            def check_reference_inclusion(inclusion, reference_header_2):
                nonlocal errors_occurred
                nonlocal prev_reference_header_inclusion
                logger.detail("Checking inclusion of reference header <%s>", inclusion.incl_path)
                if inclusion.incl_path in marked:
                    rev_inclusion_path = []
                    includer = Includer(reference_header.header_path, inclusion.line_no)
                    incl_path = inclusion.incl_path
                    class Inclusion:
                        def __init__(self, incl_path, line_no):
                            self.incl_path = incl_path
                            self.line_no = line_no
                    while True:
                        rev_inclusion_path.append(Inclusion(incl_path, includer.line_no))
                        incl_path = self._path_mapper.to_include_path(includer.header_path)
                        if incl_path == inclusion.incl_path:
                            break
                        includer = includers[incl_path]
                    n = len(rev_inclusion_path)
                    logger.fatal("Reference header inclusion cycle of length %s detected (see context below)", n)
                    incl_path = inclusion.incl_path
                    for i in range(n):
                        inclusion_2 = rev_inclusion_path[-1 - i]
                        logger_2 = logger.for_alt_context(self._path_mapper.from_include_path(incl_path),
                                                          inclusion_2.line_no)
                        logger_2.fatal("Context: Step %s of %s of include cycle: Inclusion of <%s>", i + 1, n,
                                       inclusion_2.incl_path)
                        incl_path = inclusion_2.incl_path
                    errors_occurred = True
                    return False
                includer = includers.get(inclusion.incl_path)
                if not includer:
                    includer = Includer(reference_header.header_path, inclusion.line_no)
                    includers[inclusion.incl_path] = includer
                    if not traverse(reference_header_2):
                        return False
                elif prev_reference_header_inclusion:
                    prev_inclusion, prev_includer = prev_reference_header_inclusion
                    def get_includer_path(includer):
                        rev_includer_path = []
                        includer_2 = includer
                        while includer_2:
                            rev_includer_path.append(includer_2)
                            incl_path = self._path_mapper.to_include_path(includer_2.header_path)
                            includer_2 = includers.get(incl_path)
                        return list(reversed(rev_includer_path))
                    includer_path_1 = get_includer_path(prev_includer)
                    includer_path_2 = get_includer_path(includer)
                    i = 0
                    while True:
                        assert len(includer_path_1) > i
                        assert len(includer_path_2) > i
                        assert self._path_mapper.are_same_paths(includer_path_1[i].header_path,
                                                                includer_path_2[i].header_path)
                        if includer_path_1[i].line_no != includer_path_2[i].line_no:
                            break
                        i += 1
                        if not i < len(includer_path_1) or not i < len(includer_path_2):
                            break
                    includes_1_2 = i == len(includer_path_1) # Previous inclusion includes current inclusion
                    includes_2_1 = i == len(includer_path_2) # Current inclusion includes previous inclusion
                    assert not (includes_1_2 and includes_2_1)
                    if includes_1_2 or includes_2_1:
                        logger.error("Inclusion of <%s> after <%s> conflicts with unordered relationship established "
                                     "by direct or indirect inclusion of one in the other (see context below)",
                                     inclusion.incl_path, prev_inclusion.incl_path)
                        if includes_1_2:
                            logger_2 = logger.for_alt_context(includer_path_2[i].header_path,
                                                              includer_path_2[i].line_no)
                            logger_2.error("Context: Unordered relationship established here by direct or indirect "
                                           "inclusion of <%s>", inclusion.incl_path)
                        else:
                            logger_2 = logger.for_alt_context(includer_path_1[i].header_path,
                                                              includer_path_1[i].line_no)
                            logger_2.error("Context: Unordered relationship established here by direct or indirect "
                                           "inclusion of <%s>", prev_inclusion.incl_path)
                        errors_occurred = True
                    elif includer_path_1[i].line_no > includer_path_2[i].line_no:
                        nearest_common_includer_path = includer_path_1[i].header_path
                        logger.error("Inclusion of <%s> after <%s> conflicts with order established in <%s> "
                                     "(see context below)", inclusion.incl_path, prev_inclusion.incl_path,
                                     self._path_mapper.to_include_path(nearest_common_includer_path))
                        logger_2 = logger.for_alt_line(prev_inclusion.line_no)
                        logger_2.error("Context: Preceding inclusion of <%s>", prev_inclusion.incl_path)
                        logger_3 = logger.for_alt_context(nearest_common_includer_path, includer_path_2[i].line_no)
                        logger_3.error("Context: Order-establishing direct or indirect include of <%s>",
                                       inclusion.incl_path)
                        logger_3.set_line_no(includer_path_1[i].line_no)
                        logger_3.error("Context: Order-establishing direct or indirect include of <%s>",
                                       prev_inclusion.incl_path)
                        errors_occurred = True
                prev_reference_header_inclusion = (inclusion, includer)
                return True

            incl_path = self._path_mapper.to_include_path(reference_header.header_path)
            marked.add(incl_path)
            for inclusion in self._inclusion_cache.get_inclusions(reference_header.header_path):
                logger.set_line_no(inclusion.line_no)
                header_path = self._path_mapper.from_include_path(inclusion.incl_path)
                reference_header_2 = self._path_mapper.try_get_as_reference_header(header_path)
                if reference_header_2 is None:
                    if not check_regular_inclusion(inclusion, header_path):
                        return False
                else:
                    if not check_reference_inclusion(inclusion, reference_header_2):
                        return False
            marked.remove(incl_path)
            return True

        traverse(reference_header)

        if errors_occurred:
            return False
        return True

    # This function checks that the specified reference header, which must be a full
    # reference header, covers all the regular header files in its domain.
    #
    # More precisely, if R is the specified full reference header and D is its domain, this
    # function checks that all header files in D are directly included by R, or by one of
    # the partial reference headers associated with R.
    #
    # If `recurse` is true, this check is performed for all full reference headers in the
    # reference closure of the specified reference header. Otherwise, it is performed only
    # for the specified full reference header.
    #
    # If `insert` is true, this function automatically inserts the missing include
    # statements into the specified full reference header. Such a change will need to be
    # reviewed and adjusted by a human operator.
    #
    def check_reference_coverage(self, reference_header, recurse, insert):
        assert reference_header.is_full
        errors_occurred = False
        for reference_header in self._inclusion_cache.get_full_reference_headers(reference_header, recurse):
            domain = self._domain_cache.get(reference_header.header_path)
            self._logger.info("Checking coverage by reference header <%s> of %s",
                              self._path_mapper.to_include_path(reference_header.header_path),
                              self._path_mapper.describe_domain(domain))

            coverage = set()
            local_only = True
            for incl_path in self._inclusion_cache.get_reference_order(reference_header.header_path, local_only):
                coverage.add(incl_path)

            missing_inclusions = []
            for header_path in self._file_structure_cache.find_header_files_in_domain(domain):
                if self._path_mapper.is_reference_header(header_path):
                    continue
                incl_path = self._path_mapper.to_include_path(header_path)
                if incl_path in coverage:
                    continue
                missing_inclusions.append(incl_path)
                if not insert:
                    self._logger.error("Uncovered header file <%s>", incl_path)
                    errors_occurred = True

            if insert and missing_inclusions and not errors_occurred:
                sorted_missing_inclusions = self._inclusion_cache.sort_topologically(missing_inclusions)
                with open(reference_header.header_path, "a") as file:
                    file.write("\n// Inserted\n")
                    for incl_path in sorted_missing_inclusions:
                        self._logger.info("Inserting missing inclusion <%s>", incl_path)
                        file.write("#include <%s>\n" % incl_path)
                    self._inclusion_cache.invalidate(reference_header.header_path)

        if errors_occurred:
            return False
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
    def check_reference_order(self, reference_header, recurse):
        assert reference_header.is_full

        class Includer:
            def __init__(self, incl_path, line_no):
                self.incl_path = incl_path # Include path of includer
                self.line_no = line_no     # Line number in includer

        errors_occurred = False

        for reference_header in self._inclusion_cache.get_full_reference_headers(reference_header, recurse):
            self._logger.info("Checking topological order in reference header <%s>",
                              self._path_mapper.to_include_path(reference_header.header_path))

            marked = {} # Maps includers of current traversal path to inclusion depth (cycle detection)
            curr_inclusions = [] # Inclusions corresponding to current traversal path
            first_includers = {} # Maps included headers to their first includer

            def traverse(header_path, at_root):
                nonlocal errors_occurred
                logger = FileContextLogger(self._logger, header_path)
                incl_path = self._path_mapper.to_include_path(header_path)

                def check_inclusion(inclusion):
                    nonlocal errors_occurred
                    offset = marked.get(inclusion.incl_path)
                    if offset is not None:
                        n = len(curr_inclusions) - offset
                        logger.fatal("Inclusion cycle of length %s detected (see context below)", n)
                        incl_path_2 = inclusion.incl_path
                        for i in range(n):
                            inclusion_2 = curr_inclusions[offset + i]
                            logger_2 = logger.for_alt_context(self._path_mapper.from_include_path(incl_path_2),
                                                              inclusion_2.line_no)
                            logger_2.fatal("Context: Step %s of %s of include cycle: Inclusion of <%s>", i + 1, n,
                                           inclusion_2.incl_path)
                            incl_path_2 = inclusion_2.incl_path
                        errors_occurred = True
                        return False
                    if at_root:
                        logger.detail("Checking inclusion of <%s>", inclusion.incl_path)
                    includer = first_includers.get(inclusion.incl_path)
                    if includer:
                        if at_root:
                            logger.error("Improper dependency ordering: <%s> was already directly or indirectly "
                                         "included (see context below)", inclusion.incl_path)
                            rev_inclusions = []
                            incl_path_2 = inclusion.incl_path
                            while True:
                                rev_inclusions.append(InclusionCache.Inclusion(includer.line_no, incl_path_2, ""))
                                incl_path_2 = includer.incl_path
                                if incl_path_2 == incl_path:
                                    break
                                includer = first_includers.get(incl_path_2)
                            n = len(rev_inclusions)
                            for i in range(n):
                                inclusion_2 = rev_inclusions[n - 1 - i]
                                logger_2 = logger.for_alt_context(self._path_mapper.from_include_path(incl_path_2),
                                                                  inclusion_2.line_no)
                                logger_2.error("Context: Step %s of %s: Inclusion of <%s>", i + 1, n,
                                               inclusion_2.incl_path)
                                incl_path_2 = inclusion_2.incl_path
                            errors_occurred = True
                    else:
                        first_includers[inclusion.incl_path] = Includer(incl_path, inclusion.line_no)
                        if not traverse(self._path_mapper.from_include_path(inclusion.incl_path), False):
                            return False
                    return True

                marked[incl_path] = len(curr_inclusions)
                for inclusion in self._inclusion_cache.get_inclusions(header_path):
                    logger.set_line_no(inclusion.line_no)
                    curr_inclusions.append(inclusion)
                    if not check_inclusion(inclusion):
                        return False
                    curr_inclusions.pop()
                del marked[incl_path]
                return True

            if not traverse(reference_header.header_path, True):
                return False

        if errors_occurred:
            return False
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

    errors_occurred = False
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
                errors_occurred = True
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
                        errors_occurred = True
                prev_index = index
        if need_rewrite:
            if noncontiguity_line_no is None:
                rewrites.append(Rewrite(source_path, inclusions))
            else:
                print("%s:%s: ERROR: Unable to rewrite because inclusions are non-contiguous" %
                      (source_path, noncontiguity_line_no))
                errors_occurred = True
    if errors_occurred:
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



def parse_commandline(argv, offset, options, min_args, max_args, logger):
    option_values = len(options) * [ False ]
    for i in range(len(options)):
        if len(argv) > offset and argv[offset] == options[i]:
            option_values[i] = True
            offset += 1
    num_remain = len(argv) - offset
    if num_remain < min_args:
        logger.error("Too few commandline arguments")
        sys.exit(1)
    if num_remain > max_args:
        logger.error("Too many commandline arguments")
        sys.exit(1)
    return option_values + argv[offset:] + (max_args - num_remain) * [ None ]



if len(sys.argv) < 2:
    prefix = "%s %s" % (sys.orig_argv[0], self_path)
    sys.stdout.write(f"""\
Usage:  {prefix}  check                        [-n] [-v] [-f]  <source root path>  <header path>
   or:  {prefix}  check-reference-consistency       [-v]       <source root path>  <header path>
   or:  {prefix}  check-reference-coverage     [-r]      [-i]  <source root path>  <header path>
   or:  {prefix}  check-reference-order        [-r] [-v]       <source root path>  <header path>
   or:  {prefix}  check-include-order          [-r] [-v] [-f]  <source root path>  <header path>
""")
    sys.exit(0)

root_logger = RootLogger()
func = sys.argv[1]
if func == "check":
    no_recurse, verbose, fixup, source_root_path, header_path = \
        parse_commandline(sys.argv, 2, [ "-n", "-v", "-f" ], 2, 2, root_logger)
    logger = LimitLogger(root_logger, LogLevel.DETAIL if verbose else LogLevel.INFO)
    checker = Checker(source_root_path, logger)
    reference_header = checker.map_path_arg_to_reference_header(header_path)
    if not reference_header:
        sys.exit(1)
    if not checker.check(reference_header, not no_recurse, fixup):
        logger.info("FAILED")
        sys.exit(1)
    logger.info("Passed")
elif func == "check-reference-consistency":
    verbose, source_root_path, header_path = \
        parse_commandline(sys.argv, 2, [ "-v" ], 2, 2, root_logger)
    logger = LimitLogger(root_logger, LogLevel.DETAIL if verbose else LogLevel.INFO)
    checker = Checker(source_root_path, logger)
    reference_header = checker.map_path_arg_to_reference_header(header_path)
    if not reference_header:
        sys.exit(1)
    if not checker.check_reference_consistency(reference_header):
        logger.info("FAILED")
        sys.exit(1)
    logger.info("Passed")
elif func == "check-reference-coverage":
    recurse, insert, source_root_path, header_path = \
        parse_commandline(sys.argv, 2, [ "-r", "-i" ], 2, 2, root_logger)
    logger = root_logger
    checker = Checker(source_root_path, logger)
    reference_header = checker.map_path_arg_to_reference_header(header_path)
    if not reference_header:
        sys.exit(1)
    if not checker.check_reference_coverage(reference_header, recurse, insert):
        logger.info("FAILED")
        sys.exit(1)
    logger.info("Passed")
elif func == "check-reference-order":
    recurse, verbose, source_root_path, header_path = \
        parse_commandline(sys.argv, 2, [ "-r", "-v" ], 2, 2, root_logger)
    logger = LimitLogger(root_logger, LogLevel.DETAIL if verbose else LogLevel.INFO)
    checker = Checker(source_root_path, logger)
    reference_header = checker.map_path_arg_to_reference_header(header_path)
    if not reference_header:
        sys.exit(1)
    if not checker.check_reference_order(reference_header, recurse):
        logger.info("FAILED")
        sys.exit(1)
    logger.info("Passed")
elif func == "check-include-order":
    recurse, verbose, fixup, source_root_path, header_path = \
        parse_commandline(sys.argv, 2, [ "-r", "-v", "-f" ], 2, 2, root_logger)
    logger = LimitLogger(root_logger, LogLevel.DETAIL if verbose else LogLevel.INFO)
    checker = Checker(source_root_path, logger)
    reference_header = checker.map_path_arg_to_reference_header(header_path)
    if not reference_header:
        sys.exit(1)
    if not checker.check_include_order(reference_header, recurse, fixup):
        logger.info("FAILED")
        sys.exit(1)
    logger.info("Passed")
else:
    root_logger.error("Unknown function '%s'", func)
    sys.exit(1)
