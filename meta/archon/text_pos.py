from __future__ import annotations
from dataclasses import dataclass

import bisect
import pathlib


type TextPos = NoTextPos | LineTextPos | FullTextPos

@dataclass(slots=True, frozen=True)
class NoTextPos:
    pass

@dataclass(slots=True, frozen=True)
class LineTextPos:
    line_no: int = 1

@dataclass(slots=True, frozen=True)
class FullTextPos(LineTextPos):
    offset: int = 0


@dataclass(slots=True, frozen=True)
class FileContext:
    path: pathlib.Path
    pos:  TextPos = NoTextPos()


class TextPosTracker:
    def __init__(self) -> None:
        self._offset       = 0
        self._line_offsets = [0]

    def track(self, chunk: str) -> int:
        orig_offset = self._offset
        self._offset += len(chunk)
        i = 0
        while True:
            j = chunk.find("\n", i) + 1
            if j == 0:
                break
            self._line_offsets.append(orig_offset + j)
            i = j
        return orig_offset

    def get_text_pos(self, offset: int) -> FullTextPos:
        assert offset <= self._offset
        i = bisect.bisect_right(self._line_offsets, offset) - 1
        line_no = 1 + i
        offset_2 = offset - self._line_offsets[i]
        return FullTextPos(line_no, offset_2)


class FilePosTracker(TextPosTracker):
    def __init__(self, path: pathlib.Path) -> None:
        TextPosTracker.__init__(self)
        self._path = path

    @property
    def path(self) -> pathlib.Path:
        return self._path

    def get_file_context(self, offset: int) -> FileContext:
        text_pos = self.get_text_pos(offset)
        return FileContext(self._path, text_pos)


@dataclass(slots=True, frozen=True)
class PosMappedString:
    string:  str
    pos_map: PosMap

    @classmethod
    def from_linear_string(cls, string: str, ref_pos: int = 0) -> PosMappedString:
        builder = PosMappedStringBuilder(ref_pos)
        builder.add_linear(string, ref_pos)
        return builder.get()

    @classmethod
    def from_nonlinear_string(cls, string: str, ref_pos: int = 0) -> PosMappedString:
        builder = PosMappedStringBuilder(ref_pos)
        builder.add_nonlinear(string, ref_pos)
        return builder.get()


class PosMappedStringBuilder:
    def __init__(self, lead_ref_pos: int = 0) -> None:
        self._string = ""
        self._pos_map_builder = PosMapBuilder(lead_ref_pos)

    @property
    def ref_pos(self) -> int:
        return self._pos_map_builder.ref_pos

    def add_linear(self, string: str, ref_pos: int) -> None:
        self._string += string
        self._pos_map_builder.add_linear(len(string), ref_pos)

    def add_nonlinear(self, string: str, ref_pos: int) -> None:
        self._string += string
        self._pos_map_builder.add_nonlinear(len(string), ref_pos)

    def add_pos_mapped_string(self, string: PosMappedString) -> None:
        self._pos_map_builder.add_pos_map(string.pos_map) # Throws
        self._string += string.string

    def get(self) -> PosMappedString:
        pos_map = self._pos_map_builder.get()
        assert len(self._string) == pos_map.size
        return PosMappedString(self._string, pos_map)


# Maps character positions in a *derivative string* to positions in the associated
# *reference string* in a way that does not need to be linear (can be fractured).
#
# The map covers `size` character positions in the derivative string from zero to `size -
# 1`. Each entry in `lin_segments` specifies a linear segment of the map. The segments occur
# according to the order of covered character positions, and they cannot overlap.
#
# A derived character position, `pos`, that falls inside a linear segment, `seg`, maps to
# `seg.ref_pos + (pos - seg.begin)`. The derived position is inside the segment when
# `seg.begin <= pos < seg.end`.
#
# A derived character position, `pos`, that occurs inside the map, `map`, but before the
# first linear segment if there are any linear segments, maps to `map.lead_ref_pos`. The
# derived position is inside the map when `0 <= pos < map.size`.
#
# A derived character position, `pos`, that occurs after a linear segment, `seg`, but inside
# the map and before the subsequent linear segment if there are any subsequent linear
# segments, maps to `seg.ref_pos + seg.size`.
#
# A map, `map`, is valid if and only if all of the following are true:
#
#   * `map.size >= 0`.
#
#   * If `seg` is a linear segment in `map`, then `seg.begin <= seg.end`.
#
#   * If `first` is the first linear segment in `map`, then `first.begin >= 0` and
#     `first.ref_pos >= map.lead_ref_pos`.
#
#   * If `last` is the last linear segment in `map`, then `last.end <= map.size`.
#
#   * If `a` and `b` are linear segments, and `a` occurs immediately before `b` in
#     `map.lin_segments`, then `a.end <= b.begin` and `a.ref_pos + a.size <= b.ref_pos`.
#
@dataclass(slots=True, frozen=True)
class PosMap:
    @dataclass(slots=True, frozen=True)
    class LinSegment:
        begin:   int
        end:     int
        ref_pos: int

        @property
        def size(self) -> int:
            return self.end - self.begin

        @property
        def end_ref_pos(self) -> int:
            return self.ref_pos + self.size

        def _map(self, pos: int) -> int:
            assert pos >= self.begin
            return self.ref_pos + (pos - self.begin) if pos < self.end else self.end_ref_pos

    lead_ref_pos: int
    lin_segments: tuple[LinSegment, ...]
    size:         int

    def map_(self, pos: int) -> int:
        if pos < 0 or pos > self.size:
            raise KeyError("Position is out of bounds")
        i = self._find_segment(pos)
        return self._map(i, pos)

    # Produce map that corresponds to first mapping through `other` then through this map.
    def compose_with(self, other: PosMap) -> PosMap:
        if other.map_(other.size) > self.size:
            raise KeyError("Range of other map is out of bounds")
        i = self._find_segment(other.lead_ref_pos)
        ref_pos = self._map(i, other.lead_ref_pos)
        builder = PosMapBuilder(ref_pos)
        for seg in other.lin_segments:
            builder.add_nonlinear(seg.begin - builder.pos, ref_pos)
            i = self._find_segment(seg.ref_pos, i + 1)
            seg_2 = self.lin_segments[i] if i >= 0 else None
            assert not seg_2 or seg_2.begin <= seg.ref_pos
            if seg_2 and seg.end_ref_pos <= seg_2.end:
                builder.add_linear(seg.size, seg_2._map(seg.ref_pos))
            else:
                if seg_2 and seg.ref_pos < seg_2.end:
                    size = seg_2.end - seg.ref_pos
                    builder.add_linear(size, seg_2._map(seg.ref_pos))
                while True:
                    assert i < len(self.lin_segments)
                    if i == len(self.lin_segments) - 1:
                        seg_2 = None
                    else:
                        i += 1
                        seg_2 = self.lin_segments[i]
                    if not seg_2 or seg_2.begin >= seg.end_ref_pos:
                        builder.add_nonlinear(seg.end - builder.pos, builder.ref_pos)
                        break
                    pos = seg.begin + (seg_2.begin - seg.ref_pos)
                    builder.add_nonlinear(pos - builder.pos, builder.ref_pos)
                    if seg_2.end >= seg.end_ref_pos:
                        size = seg.end_ref_pos - seg_2.begin
                        builder.add_linear(size, seg_2.ref_pos)
                        break
                    builder.add_linear(seg_2.size, seg_2.ref_pos)
            i = self._find_segment(seg.end_ref_pos, i + 1)
            ref_pos = self._map(i, seg.end_ref_pos)
        builder.add_nonlinear(other.size - builder.pos, ref_pos)
        return builder.get()

    def _find_segment(self, pos: int, offset: int = 0) -> int:
        assert 0 <= pos <= self.size
        assert 0 <= offset <= len(self.lin_segments)
        return bisect.bisect_right(self.lin_segments, pos, lo=offset, key=lambda seg: seg.begin) - 1

    def _map(self, i: int, pos: int) -> int:
        if i >= 0:
            seg = self.lin_segments[i]
            return seg._map(pos)
        return self.lead_ref_pos


class PosMapBuilder:
    def __init__(self, lead_ref_pos: int = 0):
        self._lead_ref_pos = lead_ref_pos
        self._lin_segments = list[PosMap.LinSegment]()
        self._pos          = 0
        self._ref_pos      = lead_ref_pos

    @property
    def pos(self) -> int:
        return self._pos

    @property
    def ref_pos(self) -> int:
        return self._ref_pos

    def add_linear(self, size: int, ref_pos: int) -> None:
        if size < 0:
            raise ValueError("Negative size")
        if ref_pos < self._ref_pos:
            raise ValueError("Reference position overlap")
        if self._lin_segments:
            last = self._lin_segments[-1]
            end_ref_pos = last.end_ref_pos
            assert end_ref_pos <= self._ref_pos
            no_gap = self._pos == last.end and ref_pos == end_ref_pos
            if no_gap:
                end = last.end + size
                self._lin_segments[-1] = PosMap.LinSegment(last.begin, end, last.ref_pos)
                self._pos = end
                self._ref_pos = ref_pos + size
                return
        if size:
            begin = self._pos
            end   = begin + size
            self._lin_segments.append(PosMap.LinSegment(begin, end, ref_pos))
            self._pos = end
        self._ref_pos = ref_pos + size

    def add_nonlinear(self, size: int, ref_pos: int) -> None:
        if size < 0:
            raise ValueError("Negative size")
        if ref_pos < self._ref_pos:
            raise ValueError("Reference position overlap")
        if ref_pos > self._ref_pos and size:
            begin = self._pos
            end = begin
            self._lin_segments.append(PosMap.LinSegment(begin, end, ref_pos))
        self._pos += size
        self._ref_pos = ref_pos

    def add_pos_map(self, pos_map: PosMap) -> None:
        if pos_map.lead_ref_pos < self._ref_pos:
            raise ValueError("Reference position overlap")
        self._ref_pos = pos_map.lead_ref_pos
        pos = 0
        for seg in pos_map.lin_segments:
            self.add_nonlinear(seg.begin - pos, self._ref_pos)
            self.add_linear(seg.size, seg.ref_pos)
            pos = seg.end
        self.add_nonlinear(pos_map.size - pos, self._ref_pos)

    def get(self) -> PosMap:
        size = self._pos
        return PosMap(self._lead_ref_pos, tuple(self._lin_segments), size)
