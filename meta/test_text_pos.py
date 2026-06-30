from __future__ import annotations

import random
import unittest

import archon.random as _r
import archon.text_pos as _tp
import archon.test as _t


def test_PosMap_ComposeWith(context: _t.Context) -> None:
    rng = context.create_rng()
    num_rounds = 8192
    for _ in range(num_rounds):
        max_lin_segments = 5
        max_outer_lin_segment_size = 10
        max_outer_gap_size = 10
        max_inner_gap_size = 10
        outer = _generate_outer_map(max_lin_segments, max_outer_lin_segment_size, max_outer_gap_size, rng)
        inner = _generate_inner_map(max_lin_segments, outer.size, max_inner_gap_size, rng)
        composed = outer.compose_with(inner)
        size = composed.size
        context.check_equal(size, inner.size)
        for i in range(size + 1):
            context.check_equal(composed.map_(i), outer.map_(inner.map_(i)))


def _generate_outer_map(max_lin_segments: int, max_lin_segment_size: int, max_gap_size: int,
                        rng: random.Random) -> _tp.PosMap:
    num_lin_segments = rng.randint(0, max_lin_segments)
    lin_segments = list[_tp.PosMap.LinSegment]()
    pos = rng.randint(0, max_gap_size)
    ref_pos = rng.randint(0, max_gap_size)
    for i in range(num_lin_segments):
        size = rng.randint(0, max_lin_segment_size)
        begin = pos
        end = begin + size
        lin_segments.append(_tp.PosMap.LinSegment(begin, end, ref_pos))
        pos = end + rng.randint(0, max_gap_size)
        ref_pos += size
        if i < num_lin_segments - 1:
            ref_pos += rng.randint(0, max_gap_size)
    size = pos
    return _tp.PosMap(tuple(lin_segments), size)


def _generate_inner_map(max_lin_segments: int, outer_size: int, max_gap_size: int, rng: random.Random) -> _tp.PosMap:
    num_lin_segments = rng.randint(0, max_lin_segments)
    lin_segments = list[_tp.PosMap.LinSegment]()
    pos = rng.randint(0, max_gap_size)
    if num_lin_segments > 0:
        ref_size = rng.randint(0, outer_size)
        ref_pos_offset = rng.randint(0, outer_size - ref_size)
        ref_partitions = _r.random_weak_composition(ref_size, 2 * num_lin_segments - 1, rng)
        lin_segment_sizes = ref_partitions[0::2]
        ref_gaps = ref_partitions[1::2]
        ref_pos = ref_pos_offset
        for i in range(num_lin_segments):
            size = lin_segment_sizes[i]
            begin = pos
            end = begin + size
            lin_segments.append(_tp.PosMap.LinSegment(begin, end, ref_pos))
            pos = end + rng.randint(0, max_gap_size)
            ref_pos += size
            if i < num_lin_segments - 1:
                ref_pos += ref_gaps[i]
        assert ref_pos == ref_pos_offset + ref_size
    size = pos
    return _tp.PosMap(tuple(lin_segments), size)


# Bridge to Python's native testing framework
def load_tests(loader: unittest.TestLoader, standard_tests: unittest.TestSuite,
               pattern: str | None) -> unittest.TestSuite:
    return _t.generate_native_tests(__name__)


if __name__ == '__main__':
    _t.run_module_tests(__name__)
