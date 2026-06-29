from __future__ import annotations

import random
import unittest

import archon.random as _r
import archon.text_pos as _tp
import archon.test as _t


def test_PosMap_ComposeWith(context: _t.Context) -> None:
    rng = context.create_rng()
    num_rounds = 65535
    for _ in range(num_rounds):
        max_lin_segments = 5
        max_outer_segment_size = 10
        max_outer_gap_size = 10
        max_inner_gap_size = 10
        outer = _generate_outer_map(max_lin_segments, max_outer_segment_size, max_outer_gap_size, rng)
        inner = _generate_inner_map(max_lin_segments, outer.size, max_inner_gap_size, rng)
        composed = outer.compose_with(inner)
        size = composed.size
        context.check_equal(size, inner.size)
        for i in range(size):
            context.check_equal(composed.map_(i), outer.map_(inner.map_(i)))


def _generate_outer_map(max_lin_segments: int, max_segment_size: int, max_gap_size: int,
                        rng: random.Random) -> _tp.PosMap:
    num_lin_segments = rng.randint(0, max_lin_segments)
    ref_pos = rng.randint(0, max_gap_size)
    builder = _tp.PosMapBuilder(ref_pos)
    if num_lin_segments == 0:
        size = rng.randint(0, max_gap_size)
        builder.add_nonlinear(size, ref_pos)
        return builder.get()

    for i in range(num_lin_segments):
        domain_gap = rng.randint(0, max_gap_size)
        builder.add_nonlinear(domain_gap, ref_pos)
        seg_size = rng.randint(0, max_segment_size)
        builder.add_linear(seg_size, ref_pos)
        if i + 1 < num_lin_segments:
            ref_gap = rng.randint(0, max_gap_size)
            ref_pos = builder.ref_pos + ref_gap

    size = rng.randint(0, max_gap_size)
    builder.add_nonlinear(size, builder.ref_pos)
    return builder.get()


def _generate_inner_map(max_lin_segments: int, outer_size: int, max_gap_size: int, rng: random.Random) -> _tp.PosMap:
    num_lin_segments = rng.randint(0, max_lin_segments)
    if num_lin_segments == 0:
        ref_pos = rng.randint(0, outer_size)
        builder = _tp.PosMapBuilder(ref_pos)
        size = rng.randint(0, max_gap_size)
        builder.add_nonlinear(size, ref_pos)
        return builder.get()

    end_ref_pos = rng.randint(0, outer_size)
    ref_partitions = _r.random_weak_composition(end_ref_pos, 2 * num_lin_segments, rng)
    ref_gaps = ref_partitions[0::2]
    lin_segment_sizes = ref_partitions[1::2]
    gaps = [rng.randint(0, max_gap_size) for _ in range(num_lin_segments + 1)]

    ref_pos = ref_gaps[0]
    builder = _tp.PosMapBuilder(ref_pos)
    for i in range(num_lin_segments):
        builder.add_nonlinear(gaps[i], ref_pos)
        builder.add_linear(lin_segment_sizes[i], ref_pos)
        if i + 1 < num_lin_segments:
            ref_pos = builder.ref_pos + ref_gaps[i + 1]
    builder.add_nonlinear(gaps[num_lin_segments], builder.ref_pos)
    return builder.get()


# Bridge to Python's native testing framework
def load_tests(loader: unittest.TestLoader, standard_tests: unittest.TestSuite,
               pattern: str | None) -> unittest.TestSuite:
    return _t.generate_native_tests(__name__)


if __name__ == '__main__':
    _t.run_module_tests(__name__)
