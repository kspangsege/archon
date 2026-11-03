import re

import log


class InclusionCache:
    def __init__(self, src_path, logger):
        self._src_path = src_path
        self._logger = logger
        self._cache = {}

    def get_inclusions(self, path):
        inclusions = self._cache.get(path)
        if inclusions is None:
            inclusions = self._get_inclusions(path)
            self._cache[path] = inclusions
        return inclusions

    def _get_inclusions(self, path):
        path_2 = self._src_path / path
        context = log.FileContext(path_2)
        context_logger = log.FileContextLogger(self._logger, context)
        inclusions = []
        with open(path_2, "r") as f:
            while True:
                context.line_no += 1
                line = f.readline()
                if not line:
                    break
                line = line.rstrip("\n")
                if re.fullmatch(r"\s*#\s*include\s*<archon/[^>]*\.hpp>\s*", line):
                    m = re.fullmatch(r"\s*#\s*include\s*<(archon/(\w+/)*\w+\.hpp)>\s*", line)
                    if not m:
                        context_logger.fatal("Unrecognized include syntax")
                        raise InclusionException
                    path_3 = m.group(1)
                    inclusions.append(Inclusion(path_3, context.line_no))
        return inclusions


class Inclusion:
    def __init__(self, path, line_no):
        self.path = path
        self.line_no = line_no


class InclusionException(Exception):
    pass
