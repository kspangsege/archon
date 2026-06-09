from __future__ import annotations
from typing import Any

import pathlib

import archon.log as _l
import archon.cmake.lowlevel_parser as _clp


def process(cmake_path: pathlib.Path, logger: _l.Logger) -> None:
    processor = _Processor(logger)
    processor.process(cmake_path)


class _Processor:
    def __init__(self, logger: _l.Logger) -> None:
        self._logger = logger

    def process(self, cmake_path: pathlib.Path) -> None:
        directory = _Directory()
        self._process_file(cmake_path, directory)

    def _process_file(self, cmake_path: pathlib.Path, directory: _Directory) -> None:
        def error_handler(pos: _l.FullFilePos, message: str, *args: Any) -> None:
            context = _l.FileContext(cmake_path, pos)
            _l.FileContextLogger(self._logger, context).error(message, *args)
            # FIXME: Record that an error occurred    
        for invoc in _clp.parse(cmake_path, error_handler):
            self._logger.info("-----> %s", invoc.command_name)
            # Expand arguments
            # If if(), process_if(invoc, context)


class _Directory:
    pass
