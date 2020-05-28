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

#ifndef ARCHON_X_CHECK_X_SEED_SEQ_HPP
#define ARCHON_X_CHECK_X_SEED_SEQ_HPP

/// \file


#include <utility>

#include <archon/core/random.hpp>


namespace archon::check {

/// \brief PRNG seeding customized for check testing.
///
/// This class offers functionality similar to \ref core::SeedSeq, but it disables all forms
/// of mutation including assignment. This allows it to be used safely with \ref
/// check::TestContext::seed_seq().
///
class SeedSeq {
public:
    using result_type = core::SeedSeq::result_type;

    SeedSeq(core::SeedSeq) noexcept;

    SeedSeq() noexcept;
    template<class T> SeedSeq(std::initializer_list<T> init_seed_seq);
    template<class I> SeedSeq(I begin, I end);

    template<class I> void generate(I begin, I end) const;

    auto size() const noexcept -> std::size_t;
    template<class I> void param(I dest) const;

private:
    const core::SeedSeq m_seed_seq;
};








// Implementation


inline SeedSeq::SeedSeq(core::SeedSeq seed_seq) noexcept
    : m_seed_seq(std::move(seed_seq))
{
}


inline SeedSeq::SeedSeq() noexcept
{
}


template<class T> inline SeedSeq::SeedSeq(std::initializer_list<T> init_seed_seq)
    : m_seed_seq(init_seed_seq) // Throws
{
}


template<class I> inline SeedSeq::SeedSeq(I begin, I end)
    : m_seed_seq(begin, end) // Throws
{
}


template<class I> inline void SeedSeq::generate(I begin, I end) const
{
    m_seed_seq.generate(begin, end); // Throws
}


inline auto SeedSeq::size() const noexcept -> std::size_t
{
    return m_seed_seq.size();
}


template<class I> inline void SeedSeq::param(I dest) const
{
    return m_seed_seq.param(dest); // Throws
}


} // namespace archon::check

#endif // ARCHON_X_CHECK_X_SEED_SEQ_HPP
