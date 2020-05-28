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


#include <archon/core/typed_object_registry.hpp>
#include <archon/check.hpp>


using namespace archon;


ARCHON_TEST(Core_TypedObjectRegistry_Void)
{
    int a_1 = 0;
    int a_2 = 0;
    long b = 0;

    core::TypedObjectRegistry<void, 3> reg;
    reg.register_(a_1);
    reg.register_(a_2);
    reg.register_(b);

    ARCHON_CHECK_EQUAL(reg.get<int>(), &a_2);
    ARCHON_CHECK_EQUAL(reg.get<long>(), &b);
    ARCHON_CHECK_NOT(reg.get<long long>());
}


ARCHON_TEST(Core_TypedObjectRegistry_ConstVoid)
{
    const int a_1 = 0;
    const int a_2 = 0;
    const long b = 0;

    core::TypedObjectRegistry<const void, 3> reg;
    reg.register_(a_1);
    reg.register_(a_2);
    reg.register_(b);

    ARCHON_CHECK_EQUAL(reg.get<const int>(), &a_2);
    ARCHON_CHECK_EQUAL(reg.get<const long>(), &b);
    ARCHON_CHECK_NOT(reg.get<const long long>());
}


ARCHON_TEST(Core_TypedObjectRegistry_Nonvoid)
{
    struct Base {};
    struct A : Base {};
    struct B : Base {};
    struct C : Base {};
    A a_1, a_2;
    B b;

    core::TypedObjectRegistry<Base, 3> reg;
    reg.register_(a_1);
    reg.register_(a_2);
    reg.register_(b);

    ARCHON_CHECK_EQUAL(reg.get<A>(), &a_2);
    ARCHON_CHECK_EQUAL(reg.get<B>(), &b);
    ARCHON_CHECK_NOT(reg.get<C>());
}
