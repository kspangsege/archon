from __future__ import annotations

import typing
import dataclasses
import collections

import archon.text_pos as _tp
import archon.cmake.util as _cu
import archon.cmake.lowlevel_parser as _clp
import archon.cmake.uncertainty_reason as _cur


type Argument = CertainArgument | UncertainArgument

@dataclasses.dataclass(slots=True, frozen=True)
class ArgumentBase:
    pos:      int
    was_bare: bool  # Neither quoted nor bracketed

@dataclasses.dataclass(slots=True, frozen=True)
class CertainArgument(ArgumentBase):
    string:     _tp.PosMappedString
    is_derived: bool

@dataclasses.dataclass(slots=True, frozen=True)
class UncertainArgument(ArgumentBase):
    reason: _cur.ExpansionUncertaintyReason


# FIXME: Consider expanding protoarguments just in time using a yielding scheme    
class ArgumentServer:
    def __init__(self, invoc: _clp.GeneralizedInvoc, arguments: list[Argument], file_index: int):
        self._invoc      = invoc
        self._arguments  = arguments
        self._file_index = file_index
        self._begin         = 0
        self._end           = len(self._arguments)

    def consume(self) -> CertainArgument | None:
        return self.consume_if(lambda _: True)

    def consume_keyword(self, keywords: collections.abc.Container[str]) -> CertainArgument | None:
        return self.consume_if(lambda s: s in keywords)

    def consume_not_keyword(self, keywords: collections.abc.Container[str]) -> CertainArgument | None:
        return self.consume_if(lambda s: s not in keywords)

    def consume_if(self, pred: collections.abc.Callable[[str], bool]) -> CertainArgument | None:
        if self._begin < self._end:
            arg = self._arguments[self._begin]
            if isinstance(arg, CertainArgument):
                if pred(arg.string.string):
                    self._begin += 1
                    return arg
                return None
            if isinstance(arg, UncertainArgument):
                self._begin += 1
                raise UncertainArgumentException(arg.reason, arg.was_bare) from None
            typing.assert_never(arg)
        return None

    def consume_last(self) -> CertainArgument | None:
        return self.consume_last_if(lambda _: True)

    def consume_last_if(self, pred: collections.abc.Callable[[str], bool]) -> CertainArgument | None:
        if self._begin < self._end:
            arg = self._arguments[self._end - 1]
            if isinstance(arg, CertainArgument):
                if pred(arg.string.string):
                    self._end -= 1
                    return arg
                return None
            if isinstance(arg, UncertainArgument):
                self._end -= 1
                raise UncertainArgumentException(arg.reason, arg.was_bare) from None
            typing.assert_never(arg)
        return None

    def find_keyword(self, keywords: collections.abc.Container[str]) -> int:
        return self.find(lambda s: s in keywords)

    def find(self, pred: collections.abc.Callable[[str], bool]) -> int:
        i = self._begin
        while i < self._end:
            arg = self._arguments[i]
            if isinstance(arg, CertainArgument):
                if pred(arg.string.string):
                    return i - self._begin
                i += 1
                continue
            if isinstance(arg, UncertainArgument):
                raise UncertainArgumentException(arg.reason, arg.was_bare) from None
            typing.assert_never(arg)
        return -1

    @property
    def at_end(self) -> bool:
        assert self._begin <= self._end
        return self._begin == self._end

    @property
    def next_pos(self) -> int:
        if self._begin < len(self._arguments):
            arg = self._arguments[self._begin]
            return arg.pos
        return self._invoc.rparen_pos


class UncertainArgumentException(Exception):
    def __init__(self, reason: _cur.ExpansionUncertaintyReason, was_bare: bool) -> None:
        Exception.__init__(self)
        self.reason = reason
        self.was_bare = was_bare
