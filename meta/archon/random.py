from __future__ import annotations

import random


# Generates a uniformly random weak composition of M into N parts.
#
# Computes a sequence of N non-negative integers that sum exactly to M. This implementation
# uses the "Stars and Bars" method to guarantee a mathematically unbiased, uniform
# distribution over the space of all possible valid partitions. Every valid sequence has an
# exact 1 / C(M + N - 1, N - 1) probability of being selected.
#
# Time complexity is O(N log N) due to the internal sorting of interval boundaries.
#
def random_weak_composition(m: int, n: int, rng: random.Random) -> list[int]:
    if n == 0:
        return []
    if n == 1:
        return [m]

    # Sample N-1 unique positions without replacement and sort
    bars = rng.sample(range(1, m + n), n - 1)
    bars.sort()

    # Add the absolute boundaries
    bars = [0] + bars + [m + n]

    # The values are the differences between adjacent bars, minus 1
    return [bars[i] - bars[i - 1] - 1 for i in range(1, n + 1)]
