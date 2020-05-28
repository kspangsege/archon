// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.


#include <memory>

#include <archon/core/features.h>
#include <archon/core/flat_map.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_FlatMap_NoncopiableValue)
{
    core::FlatMap<int, std::unique_ptr<int>> map;
    map.emplace(7, std::make_unique<int>(17));
    map.emplace(2, std::make_unique<int>(12));
    if (ARCHON_LIKELY(ARCHON_CHECK_EQUAL(map.size(), 2))) {
        auto i = map.begin();
        ARCHON_CHECK_EQUAL(i->first, 2);
        if (ARCHON_LIKELY(ARCHON_CHECK(i->second)))
            ARCHON_CHECK_EQUAL(*i->second, 12);
        ++i;
        ARCHON_CHECK_EQUAL(i->first, 7);
        if (ARCHON_LIKELY(ARCHON_CHECK(i->second)))
            ARCHON_CHECK_EQUAL(*i->second, 17);
    }
}
