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

#ifndef ARCHON_X_CORE_X_WORD_WRAP_HPP
#define ARCHON_X_CORE_X_WORD_WRAP_HPP

/// \file


#include <cstddef>
#include <vector>

#include <archon/core/span.hpp>


namespace archon::core::word_wrap {


struct Word;
struct Geometry;


/// \brief Perform word-wrapping using fast algorithm.
///
/// This function performs word-wrapping using a fast algorithm.
///
/// The fast algorithm puts as many words as possible on the first line, then as many words
/// as possible on the next line, and so on. This method has linear time complexity in terms
/// of the number of words.
///
/// Use this method for word-wrapping if speed is important, otherwise consider using \ref
/// word_wrap::KnuthWrapper.
///
/// \param geometry See \ref word_wrap::Geometry for how this array of geometry objects
/// specifies an infinte sequence of input line sizes.
///
/// \param breakpoints On success, is set to the list of produced breakpoints. On failure,
/// the contents is unspecified.
///
/// \param first_geometry_index This index within \p geometry of the geometry object that
/// describes the size of the first line.
///
void greedy(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
            core::Span<const word_wrap::Geometry> geometry, std::vector<std::size_t>& breakpoints,
            std::size_t first_geometry_index = 0);



/// \brief Perform word-wrapping using Knut's algorithm.
///
/// This class offers high quality word wrapping using an algorithm similar to the one used
/// in the TeX formatting system developed by Donald Knuth.
///
/// This algorithm works by minimizing the sum of the squares of the gaps at the end of each
/// line (assuming left alignment). The last line is never included in this sum.
///
/// This algorithm has quadratic time complexity in terms of the number of words.
///
/// Use this method for word wrapping if quality is important, otherwise consider using \ref
/// word_wrap::greedy(). The latter is both faster and has a lower memory footprint.
///
class KnuthWrapper {
public:
    /// \brief Perform word-wrapping using Knuth's algorithm.
    ///
    /// This function is like \ref word_wrap::greedy(), but uses Knuth's algorithm instead
    /// of the greedy one.
    ///
    void wrap(core::Span<const word_wrap::Word> words, std::size_t trailing_space_size,
              core::Span<const word_wrap::Geometry> geometry, std::vector<std::size_t>& breakpoints,
              std::size_t first_geometry_index = 0);

    using badness_type = std::uint_fast64_t;

    /// \brief Get resulting badness from last wrapping operation.
    ///
    /// This function returns the "badness" value resulting from the last invocation of \ref
    /// wrap().
    ///
    /// Intended for testing purposes only.
    ///
    auto get_badness() const noexcept -> badness_type;

private:
    struct WordSlot {
        // Adjusted word size.
        std::size_t word_size;

        // Before moving right, the current cursor position (position relative to the
        // beginning of the line) is stored here, and it is restored from here after moving
        // back.
        std::size_t cursor_pos;
    };

    struct LineSlot {
        // The maximum number of characters that can fit on this line.
        std::size_t size;

        // The index of the geometry object of which this is the first line.
        std::size_t geom_index;

        // The index of the first word placed on this line.
        std::size_t word_index;

        // Before moving down, the badness of the line break is stored here. After moving
        // up, it is taken from here and integrated into the accumulated badness of the
        // result from below.
        badness_type badness;

        // Before moving down, the index, within `m_results`, of the current result is
        // stored here. After moving up, it is restored from here.
        std::size_t result_index;
    };

    struct CacheSlot {
        // One plus the index, within `m_results`, of the result stored in this cache slot,
        // or zero if this cache slot is empty.
        std::size_t result_ident;
    };

    // If `breakpoint_index` is std::size_t(-2), then this is an indefinite result, and
    // `badness` is a lower bound on the actual badness. Otherwise, this is a definite
    // result and `badness` specifies its badness.
    struct Result {
        // The accumulated badness for the line breaks of this result, or a lower bound on
        // that.
        badness_type badness;

        // A value of std::size_t(-2) indicates that this result is indefinite. Otherwise it
        // is the index, within `m_breakpoints`, of the first breakpoint, or std::size_t(-1)
        // if there are no breakpoints.
        std::size_t breakpoint_index;

        static auto definite(badness_type badness,
                             std::size_t breakpoint_index = std::size_t(-1)) noexcept -> Result;
        static auto indefinite(badness_type badness) noexcept -> Result;

        bool is_indefinite() const noexcept;
    };

    struct Breakpoint {
        // The index of the first word that follows this breakpoint. Can never be zero.
        std::size_t word_index;

        // The index, within `m_breakpoints`, of the next breakpoint, or std::size_t(-1) if
        // this is the last one.
        std::size_t next_breakpoint_index;
    };

    std::vector<WordSlot> m_word_slots;
    std::vector<LineSlot> m_line_slots;
    std::vector<CacheSlot> m_cache;
    std::vector<Result> m_results;
    std::vector<Breakpoint> m_breakpoints;
    badness_type m_badness = 0;
};



/// \brief Relevant per-word metrics.
///
/// A word object specifies those metrics of a word that are relevant to word wrapping.
///
/// The sizes are specified in number of characters, which makes sense because those
/// characters are assumed to be displayed using a monospace font.
///
struct Word {
    /// \brief Size of preceding space.
    ///
    /// The size, measured in number of characters, of the space that precedes this word. It
    /// is allowed to be zero.
    ///
    std::size_t space_size;

    /// \brief Size of word.
    ///
    /// The size, measured in number of characters, of this word. It is allowed to be zero.
    ///
    std::size_t word_size;
};



/// \brief Part of specification of infinite sequence of line sizes.
///
/// An array of geometry objects, as passed to \ref word_wrap::greedy() and \ref
/// word_wrap::KnuthWrapper::wrap(), is used to specify an infinite sequence of input line
/// sizes.
///
/// The first geometry object describes the first line, as well as the index within the
/// array of the geometry object that describes the second line. Ordinarily, the first
/// geometry object is the first entry in the array, however, functions such as \ref
/// word_wrap::greedy() allow you to specify any entry as the one that describes the first
/// line.
///
/// In general, for N > 0, the (N+1)'th line is described by the geometry object that is
/// specified as next geometry object of the geometry object that describes the N'th line.
///
struct Geometry {
    /// \brief Size of described line.
    ///
    /// The size, measured in number of characters, of the line described by this geometry
    /// object (characters are assumed to be displayed using a monospace font).
    ///
    std::size_t line_size;

    /// \brief Index of next geometry object.
    ///
    /// The index of the geometry object that describes the next line within a list such as
    /// the one passed to \ref word_wrap::greedy() and \ref word_wrap::KnuthWrapper::wrap().
    ///
    std::size_t next_geometry_index;
};








// Implementation


inline auto KnuthWrapper::get_badness() const noexcept -> badness_type
{
    return m_badness;
}


inline auto KnuthWrapper::Result::definite(badness_type badness, std::size_t breakpoint_index) noexcept -> Result
{
    return { badness, breakpoint_index };
}


inline auto KnuthWrapper::Result::indefinite(badness_type badness) noexcept -> Result
{
    return { badness, std::size_t(-2) };
}


inline bool KnuthWrapper::Result::is_indefinite() const noexcept
{
    return (breakpoint_index == std::size_t(-2));
}


} // namespace archon::core::word_wrap

#endif // ARCHON_X_CORE_X_WORD_WRAP_HPP
