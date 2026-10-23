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

import heapq


# Determine the Lexicographically First Topological Sort of the specified dependency graph
# (dependency before dependant).
#
# The nodes of the dependency graph are identified by their index in their initial
# order. `dependencies` has an entry for each node in the graph, and each entry is a set of
# node indexes. `i` must be in `dependencies[j]` when and only when the node at index `i` is
# a dependency of the node at index `j`.
#
# If the graph has no cycles, this function returns a list of node indexes in the determined
# order. Otherwise this function returns None.
#
def stable_topological_sort(dependencies: list[set[int]]) -> list[int] | None:
    n = len(dependencies)
    adj_list: list[list[int]] = [[] for _ in range(n)]
    in_degree = [len(deps) for deps in dependencies]

    # Build the adjacency list (dependency -> dependent)
    for i, deps in enumerate(dependencies):
        for dep in deps:
            adj_list[dep].append(i)

    # Initialize the min-heap with items having 0 dependencies.
    # (A sorted list is naturally a valid min-heap).
    heap = [i for i, deg in enumerate(in_degree) if deg == 0]

    result = []
    while heap:
        curr = heapq.heappop(heap)
        result.append(curr)

        for neighbor in adj_list[curr]:
            in_degree[neighbor] -= 1
            if in_degree[neighbor] == 0:
                heapq.heappush(heap, neighbor)

    if len(result) != n:
        return None

    return result


def find_dependency_cycle(dependencies: list[set[int]]) -> list[int] | None:
    n = len(dependencies)
    states = [0] * n
    stack: list[tuple[int, list[int], int]] = []
    for index in range(n):
        deps = None
        pos = 0
        while True:
            if pos == 0:
                assert index < n
                state = states[index]
                if state == 1:
                    cycle = []
                    while True:
                        i, _, _ = stack.pop()
                        cycle.append(i)
                        if i == index:
                            break
                    return list(reversed(cycle))
                if state == 0:
                    deps = list(dependencies[index])
                    states[index] = 1 # Marked
            if deps and pos < len(deps):
                stack.append((index, deps, pos + 1))
                index = deps[pos]
                deps = None
                pos = 0
                continue
            states[index] = 2 # Done
            if stack:
                index, deps, pos = stack.pop()
                continue
            break
    return None
