/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 *
 * This file prevides a few utility iterators.
 */

#ifndef ARCHON_CORE_GENERATE_HPP
#define ARCHON_CORE_GENERATE_HPP

#include <functional>

namespace archon {
namespace core {

template<class F> struct FuncGenerator {
    using value_type = typename F::result_type;
    FuncGenerator(const F& func, value_type init_val):
        func(func),
        val(init_val)
    {
    }
    value_type operator()()
    {
        value_type v = val;
        val = func(val);
        return v;
    }
private:
    F func;
    value_type val;
};

template<class F> inline auto make_func_generator(const F& func, typename F::argument_type init_val)
{
    return FuncGenerator<F>(func, init_val);
}

template<class T> inline auto make_inc_generator(T init_val = 0)
{
    return make_func_generator(std::bind2nd(std::plus<T>(), 1), init_val);
}

} // namespace core
} // namespace archon

#endif // ARCHON_CORE_GENERATE_HPP
