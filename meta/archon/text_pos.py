from __future__ import annotations

import dataclasses
import bisect
import pathlib


type TextPos = NoTextPos | LineTextPos | FullTextPos

@dataclasses.dataclass(slots=True, frozen=True)
class NoTextPos:
    pass

@dataclasses.dataclass(slots=True, frozen=True)
class LineTextPos:
    line_no: int = 1

@dataclasses.dataclass(slots=True, frozen=True)
class FullTextPos(LineTextPos):
    offset: int = 0


@dataclasses.dataclass(slots=True, frozen=True)
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


@dataclasses.dataclass(slots=True, frozen=True)
class PosMappedString:
    string:  str
    pos_map: PosMap

    @property
    def begin_ref_pos(self) -> int:
        return self.pos_map.begin_ref_pos

    @property
    def end_ref_pos(self) -> int:
        return self.pos_map.end_ref_pos

    def map_through(self, pos_map: PosMap) -> PosMappedString:
        return PosMappedString(self.string, pos_map.compose_with(self.pos_map))

    def is_valid(self) -> bool:
        if not self.pos_map.is_valid():
            return False
        if len(self.string) != self.pos_map.size:
            return False
        return True

    @classmethod
    def from_linear_string(cls, string: str, ref_pos: int = 0) -> PosMappedString:
        builder = PosMappedStringBuilder()
        builder.add_linear(string, ref_pos)
        return builder.finalize_and_get()

    @classmethod
    def from_nonlinear_string(cls, string: str, ref_pos: int = 0) -> PosMappedString:
        builder = PosMappedStringBuilder()
        builder.add_nonlinear(string, ref_pos)
        return builder.finalize_and_get()


class PosMappedStringBuilder:
    def __init__(self) -> None:
        self._string = ""
        self._pos_map_builder = PosMapBuilder()

    @property
    def ref_pos(self) -> int:
        return self._pos_map_builder.ref_pos

    def add_linear(self, string: str, ref_pos: int | None = None) -> None:
        self._string += string
        self._pos_map_builder.add_linear(len(string), ref_pos)

    def add_nonlinear(self, string: str, ref_pos: int | None = None) -> None:
        self._string += string
        self._pos_map_builder.add_nonlinear(len(string), ref_pos)

    def add_pos_mapped_string(self, string: PosMappedString) -> None:
        self._pos_map_builder.add_pos_map(string.pos_map)
        self._string += string.string

    def bump_ref_pos_to(self, ref_pos: int) -> None:
        self._pos_map_builder.bump_ref_pos_to(ref_pos)

    def finalize_and_get(self) -> PosMappedString:
        pos_map = self._pos_map_builder.finalize_and_get()
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
# first linear segment if there are any linear segments, maps to zero. The derived position
# is inside the map when `0 <= pos < map.size`.
#
# A derived character position, `pos`, that occurs after a linear segment, `seg`, but inside
# the map and before the subsequent linear segment if there are any subsequent linear
# segments, maps to `seg.end_ref_pos`, which is `seg.ref_pos + seg.size`.
#
# A map, `map`, is valid if and only if all of the following are true:
#
#   * `map.size >= 0`.
#
#   * If `seg` is a linear segment in `map`, then `seg.begin <= seg.end`.
#
#   * If `first` is the first linear segment in `map`, then `first.begin >= 0` and
#     `first.ref_pos >= 0`.
#
#   * If `last` is the last linear segment in `map`, then `last.end <= map.size`.
#
#   * If `a` and `b` are linear segments, and `a` occurs immediately before `b` in
#     `map.lin_segments`, then `a.end <= b.begin` and `a.end_ref_pos <= b.ref_pos`.
#
@dataclasses.dataclass(slots=True, frozen=True)
class PosMap:
    @dataclasses.dataclass(slots=True, frozen=True)
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

    lin_segments: tuple[LinSegment, ...]
    size:         int

    @property
    def begin_ref_pos(self) -> int:
        if self.lin_segments:
            first = self.lin_segments[0]
            if first.begin == 0:
                return first.ref_pos
            assert first.begin > 0
        return 0

    @property
    def end_ref_pos(self) -> int:
        if self.lin_segments:
            return self.lin_segments[-1].end_ref_pos
        return 0

    def map_(self, pos: int) -> int:
        if pos < 0 or pos > self.size:
            raise KeyError("Position is out of bounds")
        i = self._find_segment(pos)
        return self._map(i, pos)

    # Produce map that corresponds to first mapping through `other` then through this map.
    def compose_with(self, other: PosMap) -> PosMap:
        if other.end_ref_pos > self.size:
            raise KeyError("Range of other map is out of bounds")
        builder = PosMapBuilder()
        i = self._find_segment(0)
        builder.bump_ref_pos_to(self._map(i, 0))
        for inner_seg in other.lin_segments:
            builder.add_nonlinear(inner_seg.begin - builder.pos)
            i = self._find_segment(inner_seg.ref_pos, i + 1)
            outer_seg = self.lin_segments[i] if i >= 0 else None
            assert not outer_seg or outer_seg.begin <= inner_seg.ref_pos
            if outer_seg and inner_seg.end_ref_pos <= outer_seg.end:
                # First outer segment completely covers current inner segment
                builder.add_linear(inner_seg.size, outer_seg._map(inner_seg.ref_pos))
            else:
                size = 0
                if outer_seg and inner_seg.ref_pos < outer_seg.end:
                    # First outer segment has overlap with current inner segment
                    size = outer_seg.end - inner_seg.ref_pos
                builder.add_linear(size, self._map(i, inner_seg.ref_pos))
                while True:
                    assert i < len(self.lin_segments)
                    if i == len(self.lin_segments) - 1:
                        # There is no next outer segment
                        builder.add_nonlinear(inner_seg.end - builder.pos)
                        break
                    next_outer_seg = self.lin_segments[i+1]
                    if next_outer_seg.begin >= inner_seg.end_ref_pos:
                        # Next outer segment has no overlap with current inner segment
                        builder.add_nonlinear(inner_seg.end - builder.pos)
                        break
                    i += 1
                    outer_seg = next_outer_seg
                    # Next outer segment has overlap with current inner segment
                    pos = inner_seg.begin + (outer_seg.begin - inner_seg.ref_pos)
                    builder.add_nonlinear(pos - builder.pos)
                    if outer_seg.end >= inner_seg.end_ref_pos:
                        # Next outer segment covers end of current inner segment
                        size = inner_seg.end_ref_pos - outer_seg.begin
                        builder.add_linear(size, outer_seg.ref_pos)
                        break
                    # Next outer segment is completely contained in current inner segment
                    builder.add_linear(outer_seg.size, outer_seg.ref_pos)
            i = self._find_segment(inner_seg.end_ref_pos, i + 1)
            builder.bump_ref_pos_to(self._map(i, inner_seg.end_ref_pos))
        builder.add_nonlinear(other.size - builder.pos)
        return builder.finalize_and_get()

    def is_valid(self) -> bool:
        if self.size < 0:
            return False
        prev: PosMap.LinSegment | None = None
        for seg in self.lin_segments:
            if seg.begin > seg.end:
                return False
            if prev:
                if prev.end > seg.begin:
                    return False
                if prev.ref_pos + prev.size > seg.ref_pos:
                    return False
            prev = seg
        if self.lin_segments:
            first = self.lin_segments[0]
            if first.begin < 0:
                return False
            if first.ref_pos < 0:
                return False
            last = self.lin_segments[-1]
            if last.end > self.size:
                return False
        return True

    # Find last linear segment that begins at or before the specified position. If no linear
    # segment begins at or before the specified position, this function returns -1.
    def _find_segment(self, pos: int, offset: int = 0) -> int:
        assert 0 <= pos <= self.size
        assert 0 <= offset <= len(self.lin_segments)
        return bisect.bisect_right(self.lin_segments, pos, lo=offset, key=lambda seg: seg.begin) - 1

    def _map(self, i: int, pos: int) -> int:
        if i >= 0:
            seg = self.lin_segments[i]
            return seg._map(pos)
        return 0


class PosMapBuilder:
    def __init__(self) -> None:
        self._lin_segments = list[PosMap.LinSegment]()
        self._pos          = 0
        self._ref_pos      = 0

    @property
    def pos(self) -> int:
        return self._pos

    @property
    def ref_pos(self) -> int:
        return self._ref_pos

    def add_linear(self, size: int, ref_pos: int | None = None) -> None:
        if size < 0:
            raise ValueError("Negative size")
        if ref_pos is None:
            ref_pos = self._ref_pos
        elif ref_pos < self._ref_pos:
            raise ValueError("Reference position overlap")
        if size:
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
            begin = self._pos
            end   = begin + size
            self._lin_segments.append(PosMap.LinSegment(begin, end, ref_pos))
            self._pos = end
        self._ref_pos = ref_pos + size

    def add_nonlinear(self, size: int, ref_pos: int | None = None) -> None:
        if size < 0:
            raise ValueError("Negative size")
        if ref_pos is None:
            ref_pos = self._ref_pos
        elif ref_pos < self._ref_pos:
            raise ValueError("Reference position overlap")
        if size:
            self._flush_lin_segment(ref_pos)
            self._pos += size
        self._ref_pos = ref_pos

    def add_pos_map(self, pos_map: PosMap) -> None:
        self.bump_ref_pos_to(pos_map.begin_ref_pos)
        pos = 0
        for seg in pos_map.lin_segments:
            self.add_nonlinear(seg.begin - pos)
            self.add_linear(seg.size, seg.ref_pos)
            pos = seg.end
        self.add_nonlinear(pos_map.size - pos)

    def bump_ref_pos_to(self, ref_pos: int) -> None:
        if ref_pos < self._ref_pos:
            raise ValueError("Reference position overlap")
        self._ref_pos = ref_pos

    def finalize_and_get(self) -> PosMap:
        self._flush_lin_segment(self._ref_pos)
        size = self._pos
        return PosMap(tuple(self._lin_segments), size)

    def _flush_lin_segment(self, ref_pos: int) -> None:
        end_ref_pos = self._lin_segments[-1].end_ref_pos if self._lin_segments else 0
        if ref_pos > end_ref_pos:
            begin = self._pos
            end = begin
            self._lin_segments.append(PosMap.LinSegment(begin, end, ref_pos))
