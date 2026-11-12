# This file is part of the Archon project, a suite of C++ libraries.
#
# Copyright (C) 2026 Kristian Spangsege <kristian.spangsege@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

from __future__ import annotations

import typing
import dataclasses
import collections.abc as _cabc
import sys
import pathlib
import stat

import archon.base as _b
import archon.text_pos as _tp
import archon.log as _l
import archon.command_line_interface as _cli
import archon.cpp_preprocess as _cp


no_serialize = _b.Wrap(False)
log_level    = _b.Wrap(_l.LogLevel.INFO)
help_        = _b.Wrap(False)

include_path: list[pathlib.Path] = []

def append_include_path_entry(path: pathlib.Path) -> None:
    include_path.append(path)

spec = _cli.Spec()
spec.opt(["-n", "--no-serialize"], _cli.Raise(no_serialize))
spec.opt(["-I", "--search"], _cli.CallWithArg(pathlib.Path, append_include_path_entry))
spec.opt(["-l", "--log-level"], _cli.AssignWithArg(_l.parse_log_level, log_level))
spec.opt(["-h", "--help"], _cli.ShortCircuit(help_))
spec.opt(["--"], _cli.Stop())


root_logger = _l.RootLogger()
success, args = _cli.parse(sys.argv[1:], spec, root_logger)
if not success:
    sys.exit(1)
if help_.value:
    _cli.show_help(spec, root_logger)
    sys.exit(0)
if len(args) != 1:
    root_logger.error("Wrong number of command-line arguments (try --help)")
    sys.exit(1)

path = pathlib.Path(args[0])

logger = _l.LimitLogger(root_logger, log_level.value)


class Application(_cp.Application):
    def __init__(self, include_path: list[pathlib.Path], file_registry: FileRegistry) -> None:
        self.error = False
        self._include_path = include_path
        self._file_registry = file_registry

    @typing.override
    def resolve_include(self, path: str, resolution_type: _cp.ResolutionType, base_file_index: int) -> int | None:
        match resolution_type:
            case _cp.ResolutionType.BRACKETED:
                pass
            case _cp.ResolutionType.QUOTED:
                base_entry = self._file_registry.get(base_file_index)
                file_index = self._find_file(path, base_entry.path.parent)
                if file_index is not None:
                    return file_index
            case _:
                typing.assert_never(resolution_type)
        for dir_path in self._include_path:
            file_index = self._find_file(path, dir_path)
            if file_index is not None:
                return file_index
        return None

    @typing.override
    def open_file(self, file_index: int) -> _cp.Source:
        entry = self._file_registry.get(file_index)
        input_ = open(entry.path)
        return _cp.Source(input_, entry.position_tracker)

    @typing.override
    def handle_pragma(self, tokens: _cabc.Sequence[_cp.Token], pos: _cp.Position, end_pos: _cp.Position) -> None:
        text = "".join(t.text for t in tokens)
        entry = self._file_registry.get(pos.file_index)
        rpos = entry.position_tracker(pos.pos_in_file)
        context = _l.FileContext(rpos.path, _l.FullTextPos(rpos.line_no, rpos.pos_on_line))
        _l.FileContextLogger(logger, context).info("Pragma: %s", text)

    @typing.override
    def handle_message(self, type_: _cp.MessageType, pos: _cp.Position, message: str, *args: typing.Any) -> None:
        entry = self._file_registry.get(pos.file_index)
        rpos = entry.position_tracker(pos.pos_in_file)
        context = _l.FileContext(rpos.path, _l.FullTextPos(rpos.line_no, rpos.pos_on_line))
        context_logger = _l.FileContextLogger(logger, context)
        match type_:
            case _cp.MessageType.ERROR:
                context_logger.error(message, *args)
                self.error = True
                return
            case _cp.MessageType.WARNING:
                context_logger.warn(message, *args)
                return
        typing.assert_never(type_)

    def _find_file(self, path: str, dir_path: pathlib.Path) -> int | None:
        resolved_path = dir_path / path
        file_index = self._file_registry.ensure(resolved_path)
        if file_index is None:
            return None
        return file_index


class FileRegistry:
    def __init__(self) -> None:
        self._entries: list[FileEntry] = []
        # Maps (st_dev, st_ino) to file_index
        self._identities: dict[tuple[int, int], int] = {}

    def ensure(self, path: pathlib.Path) -> int | None:
        try:
            stat_result = path.stat()
        except (FileNotFoundError, NotADirectoryError):
            return None
        if not stat.S_ISREG(stat_result.st_mode):
            return None

        identity = (stat_result.st_dev, stat_result.st_ino)
        if identity in self._identities:
            return self._identities[identity]

        position_tracker = _cp.PositionTracker(str(path))
        entry = FileEntry(path, position_tracker)
        file_index = len(self._entries)
        self._entries.append(entry)
        self._identities[identity] = file_index
        return file_index

    def get(self, file_index: int) -> FileEntry:
        return self._entries[file_index]


@dataclasses.dataclass(slots=True, frozen=True)
class FileEntry:
    path:             pathlib.Path
    position_tracker: _cp.PositionTracker


try:
    with open(path) as input_:
        file_registry = FileRegistry()
        file_index = file_registry.ensure(path)
        if file_index is None:
            raise RuntimeError("Unexpected concurrent modification")
        root_entry = file_registry.get(file_index)
        source = _cp.Source(input_, root_entry.position_tracker)
        app = Application(include_path, file_registry)
        tokens_iter = _cp.preprocess(file_index, source, app)
        if no_serialize.value:
            for token in tokens_iter:
                pos = token.pos
                entry = file_registry.get(pos.file_index)
                rpos = entry.position_tracker(pos.pos_in_file)
                string = _b.clamped_quote(token.text, 64)
                if token.synthetic:
                    string += " (synthetic)"
                logger.info("%s:%s:%s: %s: %s", rpos.path, rpos.line_no, rpos.pos_on_line, token.type_.name, string)
        else:
            parts: list[str] = []
            def flush() -> None:
                string = "".join([p for p in parts])
                sys.stdout.write(string)
                parts.clear()
            for token in tokens_iter:
                if token.type_ is _cp.TokenType.END_OF_INPUT:
                    if parts:
                        flush()
                    break
                parts.append(token.reconstruct())
                if token.type_ is _cp.TokenType.NEWLINE:
                    flush()
        if app.error:
            sys.exit(1)
except OSError as e:
    logger.error("Failed to open %s: %s", _b.quote(str(path)), e.strerror)
    sys.exit(1)
