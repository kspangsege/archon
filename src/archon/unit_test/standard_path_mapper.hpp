// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/// \file

#ifndef ARCHON__UNIT_TEST__STANDARD_PATH_MAPPER_HPP
#define ARCHON__UNIT_TEST__STANDARD_PATH_MAPPER_HPP

#include <archon/base/build_environment.hpp>
#include <archon/unit_test/test_config.hpp>


namespace archon::unit_test {


/// \brief Express source file paths relative to current working directory.
///
/// This class implements a source file path mapper that uses information about
/// the build environment (\ref base::BuildEnvironment) to transform the paths
/// of the source files such that they are expressed relative to the current
/// working directory.
///
class StandardPathMapper :
        public SourcePathMapper {
public:
    explicit StandardPathMapper(base::BuildEnvironment&) noexcept;

    bool map(std::filesystem::path&) const override;

private:
    base::BuildEnvironment& m_build_env;
};








// Implementation


inline StandardPathMapper::StandardPathMapper(base::BuildEnvironment& build_env) noexcept :
    m_build_env(build_env)
{
}


} // namespace archon::unit_test

#endif // ARCHON__UNIT_TEST__STANDARD_PATH_MAPPER_HPP
