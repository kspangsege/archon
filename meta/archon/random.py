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
