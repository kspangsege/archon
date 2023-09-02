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

#ifndef ARCHON_X_CORE_X_TEXT_FORMATTER_HPP
#define ARCHON_X_CORE_X_TEXT_FORMATTER_HPP

/// \file


#include <cstddef>
#include <cmath>
#include <type_traits>
#include <limits>
#include <utility>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <stack>
#include <locale>
#include <ios>
#include <streambuf>
#include <ostream>

#include <archon/core/features.h>
#include <archon/core/span.hpp>
#include <archon/core/assert.hpp>
#include <archon/core/integer.hpp>
#include <archon/core/math.hpp>
#include <archon/core/buffer.hpp>
#include <archon/core/buffer_contents.hpp>
#include <archon/core/char_mapper.hpp>
#include <archon/core/format.hpp>
#include <archon/core/terminal.hpp>
#include <archon/core/word_wrap.hpp>


namespace archon::core {


/// \brief Format text for display in a monospaced font, such as on a terminal.
///
/// This class provides facilities to format text for display in a monospaced font, such as
/// on a text terminal.
///
/// The main features of the formatter are indentation, word wrapping, justification,
/// alignment, clipping, columnation, and text styling. The formatter features an optional
/// high quality word wrapping method similar to the one used in the TeX formatting system
/// developed by Donald Knuth (\ref Config::high_quality_word_wrapper). Text styling is
/// based on emission of ANSI escape sequences and needs to be enabled in order to take
/// effect (\ref Config::enable_ansi_escape_sequences).
///
/// Input text is submitted to the formatter much like if the formatter was an output
/// stream, in fact, the formatter makes an actual output stream available for submission of
/// input to the formatter (\ref out()). With default settings, no formatting takes place,
/// so any text submitted to the formatter is passed through unmodified to the underlying
/// output stream. The underlying output stream is the one that is passed to the constructor
/// of the formatter. For anything nontrivial to happen, at least one formatting paramter
/// needs to be changed away from its default value.
///
/// Text is formatted one input line at a time. The input is first divided into input
/// sections, then each section is divided into input lines. By default, that is, when not
/// in compilation mode, each newline character encountered in the input produces a separate
/// single-lined input section. See \ref begin_compile() and \ref close_section() for ways
/// to modify this default behavior.
///
/// In general, the formatting of an input line involves breaking it into fragments that fit
/// on separate output lines (word wrapping). By default, each of these fragments are
/// written to the underlying output stream immediately as separate output lines. However,
/// it is possible to hold back the output for a while (\ref begin_hold()), and jump back to
/// earlier output lines in order to amend them (\ref jump()). This is especially useful for
/// producing multi-columned output.
///
/// The formatter maintains an output cursor whose position is advanced as a result of the
/// formatting of each input line. The current position of the output cursor is available
/// through \ref get_line_number() and \ref get_cursor_pos(). The former returns the
/// vertical position, and the latter returns the horizontal position. The horizontain
/// position is zero unless the current output line is open. The cursor can be moved by
/// calling \ref jump(), although movement to earlier lines is only possible while output is
/// held back.
///
/// Many features of the formatting process require that a formatting width is
/// specified. Until a width is specified, those features are effectively disabled. This
/// includes word wrapping, justification, alignment, clipping, and box filling. When box
/// filling is disabled, any specified fill color is ignored.
///
/// When a formatting width is specified (\ref set_width()), it becomes the width of the
/// formatting box, which is what controls many aspects of the formatting process. By
/// default, the left edge of the formatting box is aligned with the left edge of the output
/// medium (terminal), but it can be shifted to the right by setting the offset (\ref
/// set_offset()). This is useful for columnation.
///
/// The amount of horizontal space, that is available for text inside a box, is the width of
/// the box minus left-side padding (\ref set_padding_left()), indentation (\ref
/// set_indent()), and right-side padding (\ref set_padding_right()). If a fragment is too
/// wide to fit in the box, for example because of a long word or because word wrapping is
/// disabled, it will be allowed to extend into the right-side padding area, and possibly
/// even beyond the right-side edge of the box. However, if clipping is enabled (\ref
/// set_clipping()), then any part that would extend beyond the edge of the box will be cut
/// off. Even when clipping is enabled, text may extend into the right-side padding area.
///
/// The fragments that are produced by word wrapping, except the last one that corresponds
/// to the end of the input line, will be subjected to justification, if justification is
/// enabled (\ref set_justify()) and the fragment ended up with at least two words. Here,
/// additional white space will be added between words in order to achieve alignment with
/// both the left and the right side of the area inside the formatting box that is available
/// for text. Leading and trailing space of an input line is never affected by
/// justification.
///
/// The justification mechanism may best be understood as follows: Take the line (line
/// fragment) in its natural unstretched form. Connect the centers of the two outermost
/// words with a tight rubber band. Fix the centers of the remaining in-between words to
/// this rubber band while keeping everything steady. Now, stretch the rubber band until the
/// line length matches the available space between the margins. Detach all the words from
/// the rubber band. Finally, slide each of the in-between words individually to a nearby
/// integer position.
///
/// If whitespace normalization is turned on (set_norm_whitespace()), any leading and
/// trailing space will be removed from each submitted input line, and the number of spaces
/// between words in an input line will be reduced to one.
///
template<class C, class T = std::char_traits<C>> class BasicTextFormatter {
public:
    using char_type   = C;
    using traits_type = T;

    class Format;
    class Cursor;

    struct Config;
    struct SectionInfo;
    struct LineInfo;
    struct WordInfo;
    struct MeasureResult;
    struct SimulateResult;

    using string_view_type = std::basic_string_view<C, T>;
    using ostream_type     = std::basic_ostream<C, T>;

    enum class Align { left, center, right };

    using Weight = core::terminal::Weight;
    using Color  = core::terminal::Color;
    using Style  = core::terminal::TextAttributes;

    /// \{
    ///
    /// \brief Construct text formatter.
    ///
    /// Construct a text formatter.
    ///
    /// \param out The underlying stream onto which the formatter will write the formatted
    /// text.
    ///
    /// \param config A set of parameters through which the formatter can be configured.
    ///
    explicit BasicTextFormatter(ostream_type& out);
    explicit BasicTextFormatter(ostream_type& out, Config config);
    /// \}

    /// \brief Submit serialization of specified value as text formatter input.
    ///
    /// `formatter.write(val)` has the same effect as `formatter.out() << val`.
    ///
    template<class V> void write(V&& val);

    /// \brief Submit value and newline as text formatter input.
    ///
    /// `formatter.writeln(val)` is shorthand for `formatter.write(val)` followed by
    /// `formatter.write("\n")`.
    ///
    template<class V> void writeln(V&& val);

    /// \brief Submit formatted string as text formatter input.
    ///
    /// `formatter.format(pattern, params...)` is shorthand for
    /// `formatter.write(core::formatted(pattern, params...))`.
    ///
    template<class... P> void format(const char* pattern, const P&... params);

    /// \brief Output stream through with text formatter input can be submitted.
    ///
    /// Any text written to the returned output stream will effectively be submitted as text
    /// to the formatter.
    ///
    /// Calling `flush()` on the returned stream, or `pubsync()` on the underlying stream
    /// buffer has the same effect as calling \ref process_input() on the text formatter.
    ///
    auto out() noexcept -> ostream_type&;

    /// \brief Finalize formatting process.
    ///
    /// Process any remaining input under the assumption that no more input will be
    /// received. This operation roughly corresponds to processing any unprocessed intput
    /// (as if by \ref process_input()); then, if there is an open input section, close it
    /// (as if by \ref close_section()); and then, if the current ouput line is open, close
    /// it (as if by \ref close_output_line()).
    ///
    /// The application is advised to always call this function after all text has been
    /// submitted to the formatter in order to avoid loosing output.
    ///
    /// It is an error to call this function when in compilation mode (\ref
    /// begin_compile()). If is also an error to call this function while output is held
    /// back (\ref begin_hold()).
    ///
    void finalize();

    /// \brief Process unprocessed input and flush output.
    ///
    /// If `f` is a text formatter and `out` is the output stream passed to the text
    /// formatter constructor, `f.flush()` has the same effect as `f.process_input()`
    /// followed by `out.flush()`.
    ///
    /// \sa \ref process_input()
    ///
    void flush();

    /// \{
    ///
    /// \brief Change aspects of current text format.
    ///
    /// These funrctions are shorthands for calling \ref get_format(), then calling the
    /// respective "setter" function on the format object (\ref Format), and then passing
    /// the modified format object to \ref set_format().
    ///
    /// It is an error to call any of these functions while an open input section exists
    /// (\ref has_open_section()).
    ///
    void set_width(std::size_t);
    void unset_width();
    void set_indent(std::size_t);
    void set_indent(std::size_t first, std::size_t rest);
    void set_indent(std::size_t first_1, std::size_t first_2, std::size_t rest);
    void set_offset(std::size_t);
    void set_padding(std::size_t);
    void set_padding_left(std::size_t);
    void set_padding_right(std::size_t);
    void set_word_wrap(bool);
    void set_justify(bool);
    void set_clipping(bool);
    void set_align(Align);
    void set_align(double);
    void set_fill_color(Color);
    void unset_fill_color();
    void set_always_fill(bool);
    void set_norm_whitespace(bool);
    void set_min_separation(std::size_t);
    void set_adv_continuation(bool);
    void set_max_displacement(std::size_t);
    /// \}

    /// \brief Reset the text format to its default state.
    ///
    /// This function has same effect as `set_format({})` (see \ref set_format()).
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    void reset_format();

    /// \brief Save the current text format for later.
    ///
    /// This function pushes a copy of the currently selected text format onto the format
    /// stack.
    ///
    /// \sa \ref pop_format()
    ///
    void push_format();

    /// \brief Restore a previously saved text format.
    ///
    /// This function pops the most recently pushed text format off of the format stack, and
    /// instates it as the current text format as if by an invocation of \ref set_format()).
    ///
    /// It is an error to call this function when the format stack is empty. It is also an
    /// error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    /// \sa \ref push_format()
    ///
    void pop_format();

    /// \{
    ///
    /// \brief Change aspects of current text style.
    ///
    /// These functions change various aspects of the current text style.
    ///
    /// ANSI escape sequences need to be enabled for these functions to have any effect. See
    /// \ref Config::enable_ansi_escape_sequences.
    ///
    /// \sa \ref core::terminal::TextAttributes
    /// \sa \ref set_style()
    ///
    void set_weight(Weight);
    void set_underline(bool);
    void set_blink(bool);
    void set_reverse(bool);
    void set_color(Color);
    void set_background_color(Color);
    void unset_color();
    void unset_background_color();
    /// \}

    /// \brief Reset the text style to its default state.
    ///
    /// This function has the same effect as `set_style({})` (see \ref set_style()).
    ///
    void reset_style();

    /// \brief Save the current text style for later.
    ///
    /// This function pushes a copy of the currently selected text style onto the style
    /// stack.
    ///
    /// \sa \ref pop_style()
    ///
    void push_style();

    /// \brief Restore a previously saved text style.
    ///
    /// This function pops the most recently pushed text style off of the style stack, and
    /// instates it as the new current text style (as if by an invocation of \ref
    /// set_style()).
    ///
    /// It is an error to call this function when the style stack is empty.
    ///
    /// \sa \ref push_style()
    ///
    void pop_style();

    /// \brief Process unprocessed input.
    ///
    /// An invocation of this function will cause all unprocessed input to be processed. If
    /// not in compilation mode, all terminated input lines will have been formatted upon
    /// return.
    ///
    /// \sa \ref begin_compile()
    ///
    void process_input();

    /// \brief Get current output line number.
    ///
    /// Get the line number corresponding to the current cursor position. This is the same
    /// as \ref get_last_line_number() unless \ref jump() is invoked.
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    auto get_line_number() -> std::size_t;

    /// \brief Get last output line number.
    ///
    /// Get the line number corresponding to the last generated output line. Initially, this
    /// will be the value of \ref Config::line_number_base.
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    auto get_last_line_number() -> std::size_t;

    /// \brief Get horizontal cursor position.
    ///
    /// FIXME: Talk about nominal vs actual cursor position                                                  
    ///
    /// FIXME: Talk about the fact that this function returns zero unless the current output line is open.             
    ///
    /// This function returns the current horizontal cursor position, which is the same as
    /// the current size of the current output line.
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    auto get_cursor_pos() -> std::size_t;

    /// \brief Skip output lines.
    ///
    /// This function has the same effect on a formatter `f` as `f.jump(f.get_line_number()
    /// + n)` would have.
    ///
    /// The effect differs from that of formatting a newline character only when a fill
    /// color is set (\ref set_fill_color()). Box filling is performed on the origin line
    /// when formatting a newline character, even when the origin line remains otherwise
    /// unopened. This is because the newline character causes the origin output line to be
    /// briefly opened. This is not the case for `skip_line()` and other kinds of jumps
    /// (\ref jump()).
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    void skip_line(std::size_t n = 1);

    /// \brief End the current input section now.
    ///
    /// If an input section is currently open, this function closes it. If an input section
    /// is not currently open, this function generates an empty input section.
    ///
    /// A new input section will be opened automatically if additional text is submitted to
    /// the formatter.
    ///
    /// In compilation mode (see \ref begin_compile()), the section is simply added to the
    /// list of closed sections retained in the input buffer. When not in compilation mode,
    /// the section is immediately formatted, and then evicted from the input buffer.
    ///
    /// Additionally, when not in compilation mode, `close_section()` is automatically
    /// called after each newline character encountered in the input.
    ///
    /// Outstanding input will be processed before the section is closed, as if by an
    /// invocation of \ref process_input().
    ///
    void close_section();

    /// \brief Whether an open input section exists at this time.
    ///
    /// This function returns `true` if, and only if an open input section exists at this
    /// time.
    ///
    /// A new input section is opened whenever a character is written to the formatter and
    /// an open input section does not already exist. The currently open input section is
    /// closed whenever \ref close_section() is called, or, if not in compilation mode,
    /// after a newline character is encoutered in the input.
    ///
    /// When this function is called, any outstanding input is processed, as if by an
    /// invocation of \ref process_input(), before the judgement is made as to whether an
    /// open input section exists.
    ///
    bool has_open_section();

    /// \brief Switch to compilation mode.
    ///
    /// In compilation mode, input sections are not automatically closed after each newline
    /// character, and they are not immediately formatted upon closure.
    ///
    /// In compilation mode, all input sections are retained in the input buffer, therfore,
    /// to avoid excessive memory usage, compilation mode should only be used when
    /// necessary. Further more, since cleanup does not happen until exit from compilation
    /// mode, it is probably a bad idea to remain in compilation mode for longer than
    /// necessary, and it may be necessary to exit compilation mode even when reentering
    /// shortly thereafter, just to allow for clean up to happen.
    ///
    /// If there is an open input section (\ref has_open_section()) it will be closed before
    /// switching to compilation mode, as if by a preceding invocation of \ref
    /// close_section().
    ///
    /// It is an error to call this function when in compilation mode (\ref is_compiling()).
    ///
    void begin_compile();

    /// \brief Switch away from compilation mode.
    ///
    /// If in compilation mode, switch away from compilation mode. Otherwise, do nothing
    /// (idempotency).
    ///
    /// Input sections, that have not been explicitely formatted using \ref
    /// format_section(), will be lost. If there is an open input section, it will be lost
    /// too.
    ///
    void end_compile() noexcept;

    /// \brief Whether formatter is in compilation mode.
    ///
    /// This function returns `true` when, and only when the formatter is in compilation
    /// mode.
    ///
    /// \sa \ref begin_compile() and \ref end_compile()
    ///
    bool is_compiling() const noexcept;

    /// \brief Number of compiled sections.
    ///
    /// This function returns the number of closed sections retained in the input
    /// buffer. When not in compilation mode, this function always returns zero.
    ///
    auto get_num_sections() const noexcept -> std::size_t;

    /// \brief Format the specified input section.
    ///
    /// This function adds a formatted copy of the specified retained input section to the
    /// output.
    ///
    /// \param index The index of the closed input section to be formatted. The index must
    /// be less than the current number of closed sections retained in the input buffer
    /// (\ref get_num_sections()).
    ///
    void format_section(std::size_t section_index);

    /// \brief Get information about closed input section.
    ///
    /// This fucntion returns information about the specified closed input section (\p
    /// section_index).
    ///
    /// The specified section index (\p section_index) must be less than the number of
    /// closed input sections as returned by \ref get_num_sections().
    ///
    auto get_section_info(std::size_t section_index) -> SectionInfo;

    /// \brief Get information about line in closed input section.
    ///
    /// This function returns information about the specified line (\p line_index) in the
    /// specified closed input section (\p section_index).
    ///
    /// The specified section index (\p section_index) must be less than the number of
    /// closed input sections as returned by \ref get_num_sections().
    ///
    /// The specified line index (\p line_index) must be less than the number of lines in
    /// the specified section (\ref SectionInfo::num_lines).
    ///
    auto get_line_info(std::size_t section_index, std::size_t line_index) -> LineInfo;

    /// \brief Get information about word in closed input section.
    ///
    /// This function returns information about the specified word (\p word_index) in the
    /// specified line (\p line_index) in the specified closed input section (\p
    /// section_index).
    ///
    /// The specified section index (\p section_index) must be less than the number of
    /// closed input sections as returned by \ref get_num_sections().
    ///
    /// The specified line index (\p line_index) must be less than the number of lines in
    /// the specified section (\ref SectionInfo::num_lines).
    ///
    /// The specified word index (\p word_index) must be less than the number of words in
    /// the specified line (\ref LineInfo::num_words).
    ///
    auto get_word_info(std::size_t section_index, std::size_t line_index, std::size_t word_index) -> WordInfo;

    /// \brief Measure minimum and maximum width of section.
    ///
    /// This function measures the minimum and maximum width of the specified section.
    ///
    /// The returned minimum width is the smallest value that can be used as the width of
    /// the formatting box (\ref set_width()) without causing text to overflow. Here,
    /// overflow means that text ends up extending into the right side padding area, or even
    /// beyond the right side edge of the formatting box. Note, text is considered to
    /// overflow even when clipping is turned on.
    ///
    /// Thanks to the design of the formatter, it is additionally guaranteed that there can
    /// be no overflow if the width of the formatting box is set to **any** value greater
    /// than, or equal to the returned minimum width.
    ///
    /// The returned maximum width is the smallest value that can be used as the width of
    /// the formatting box without causing any input line to be broken during word wrapping,
    /// had word wrapping been enabled. Whether word wrapping is actually enabled makes no
    /// difference here.
    ///
    /// If word wrapping is disabled, the returned minimum and maximum widths will be equal.
    ///
    /// Formatting parameters, that do not affect the result, include the currently
    /// configured width of the formatting box, whether justification is enabled, the
    /// currently configured alignment disposition, and whether clipping is enabled.
    ///
    /// Formatting parameters, that do affect the result, include padding and indentation
    /// paramters and whether word wrapping is enabled. The offset (position of formatiing
    /// box) has an effect if, and only if `open_line` is true in the specified cursor
    /// state.
    ///
    /// \param section_index The index of the closed input section to be measured. The index
    /// must be less than the current number of closed sections retained in the input buffer
    /// (\ref get_num_sections()).
    ///
    /// \param cursor The initial state that the output cursor should be assumed to be
    /// in. If you pass the object returned by get_cursor_state(), the measurement will be
    /// conducted under exactly the same circumstances as those of the current state of the
    /// formatter. If you pass a default constructed cursor state, the measurement will be
    /// conducted under exactly the same circumstances as those of the initial state of the
    /// formatter. See also SimulateResult::cursor_state.
    ///
    auto measure(std::size_t section_index, const Cursor& cursor) -> MeasureResult;

    /// \brief Compute effect of formatting of section.
    ///
    /// This fucntion computes the effect that the formating of the specified section (\p
    /// section_index) would have.
    ///
    /// \param section_index The index of the closed input section to be analyzed. The index
    /// must be less than the current number of closed sections retained in the input buffer
    /// (\ref get_num_sections()).
    ///
    /// FIXME: State what values are assumed for cursor state                                                                                       
    ///
    /// FIXME: State what values are assumed for the various formatting parameters                                                                                       
    ///
    auto simulate(std::size_t section_index, std::size_t width) -> SimulateResult;

    /// \brief Get state of output cursor.
    ///
    /// FIXME: Move this to somewhere else            
    ///
    auto get_cursor_state() -> Cursor;

    /// \brief Hold back subsequently generated output.
    ///
    /// All subsequently generated output will be held back until \ref end_hold() is
    /// called. This makes it possible to produce multiple output lines, then jump back and
    /// amend some of them using \ref jump(). The current line at the time of calling
    /// `begin_hold()` will be the first line that can be jumped back to. The line number of
    /// that line is returned by \ref get_hold_line_number().
    ///
    /// If `begin_hold()` is called while output is already held back, a new fictitious, and
    /// nested hold is entered. This nested hold is ended by the first subsequent invocation
    /// of \ref end_hold(), but that does not end the actual hold. To end the actual hold,
    /// another invocation of \ref end_hold() is required. In general, any number of nesting
    /// levels can be introduced, and ended this way.
    ///
    /// It is an error to call `begin_hold()` while an open input section exists (\ref
    /// has_open_section()).
    ///
    /// \sa \ref jump()
    /// \sa \ref end_hold()
    ///
    void begin_hold();

    /// \brief Release accumulated output.
    ///
    /// Release the most recently introduced hold on output (see \ref begin_hold()). If this
    /// hold was not a nested one, then write all of the accumulated output to the
    /// underlying output stream, and stop holding back output. The underlying output stream
    /// is the one that was passed to the constructor of the formatter object.
    ///
    /// When a hold is ended, the output cursor will be moved to the last line that was
    /// reached during the hold as if by `jump(get_reached_line_number())`. This is also
    /// true for nested holds.
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()). It is also an error to call this function if output is not
    /// currently held back.
    ///
    /// \sa \ref begin_hold()
    ///
    void end_hold();

    /// \brief Jump back to the first held back output line.
    ///
    /// This function has the same effect on a formatter `f` as
    /// `f.jump(f.get_hold_line_number())` would have.
    ///
    void jump_back();

    /// \brief Jump to other output line.
    ///
    /// Close the current output line if it is open (as if by \ref close_ouput_line()), then
    /// move the cursor to the end of the specified output line (\p line_number).
    ///
    /// If output is currently held back, the first line number that can be jumped to, is
    /// the one that was current when \ref begin_hold() was called (\ref
    /// get_hold_line_number()). Otherwise, it is the current line (\ref get_line_number()).
    ///
    /// A jump to the current line has no other effect than to close the output line if it
    /// is open.
    ///
    /// A jump below the currently last line (\ref get_last_line_number()) is possible, and
    /// has the effect of introducing new output lines as necessary.
    ///
    /// FIXME: Explain that "box filling" does not happen during closure of the origin line if the origin line is not already open.                                     
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    void jump(std::size_t line_number);

    /// \brief Get line number where hold was initiated.
    ///
    /// This function returns the output line number that was current at the time of the
    /// most recent initiation of a hold (\ref begin_hold()). This is the lowest line number
    /// that can be jumped to during this hold (\ref jump()).
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()). It is also an error to call this function if output is not
    /// currently held back.
    ///
    auto get_hold_line_number() -> std::size_t;

    /// \brief Get last line number reached during hold.
    ///
    /// This function returns the last output line number that was reached during the most
    /// recently initiated hold (\ref begin_hold()). This is the line number that will be
    /// jumped to when this hold is ended (\ref end_hold()).
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()). It is also an error to call this function if output is not
    /// currently held back.
    ///
    auto get_reached_line_number() -> std::size_t;

    /// \brief Get current text format.
    ///
    /// This function returns the text format that is currently in effect.
    ///
    /// \sa \ref set_format()
    /// \sa \ref get_style()
    ///
    auto get_format() const noexcept -> Format;

    /// \brief Replace current text format.
    ///
    /// This function instates the specified text format as the new current text format. The
    /// new text format will apply to any subsequently submitted input.
    ///
    /// It is an error to call this function while an open input section exists (\ref
    /// has_open_section()).
    ///
    /// \sa \ref get_format()
    /// \sa \ref set_style()
    ///
    void set_format(const Format&);

    /// \brief Get current text style.
    ///
    /// This function returns the text style that is currently in effect.
    ///
    /// \sa \ref set_style()
    /// \sa \ref get_format()
    ///
    auto get_style() const noexcept -> Style;

    /// \brief Replace current text style.
    ///
    /// This function instates the specified text style as the new current text style. The
    /// new text style will apply to any subsequently submitted input.
    ///
    /// ANSI escape sequences need to be enabled for this function to have any effect. See
    /// \ref Config::enable_ansi_escape_sequences.
    ///
    /// \sa \ref get_style()
    /// \sa \ref set_format()
    ///
    void set_style(Style style);

private:
    class StreambufImpl;
    struct FormatRep;
    struct InputWord;
    struct InputLine;
    struct InputSection;
    struct InputStyle;
    struct OutputLine;
    struct OutputSegment;
    struct HoldEntry;
    struct ExtendedSectionInfo;
    struct ExtendedLineInfo;

    using FormatUnit = word_wrap::Word;

    ostream_type& m_final_out;
    const std::locale m_locale;
    StreambufImpl m_streambuf;
    ostream_type m_out;
    core::BasicCharMapper<C, T> m_char_mapper;
    std::unique_ptr<word_wrap::KnuthWrapper> m_knuth_wrapper;
    Style m_input_style, m_fill_style;
    FormatRep m_format;
    core::Buffer<char_type> m_input_buffer;
    core::Buffer<char_type> m_output_buffer;
    core::Buffer<char_type> m_output_line_buffer;
    std::vector<InputWord> m_input_words;
    std::vector<InputLine> m_input_lines;
    std::vector<InputSection> m_input_sections;
    std::vector<InputStyle> m_input_styles;
    std::vector<FormatUnit> m_format_units;
    std::vector<std::size_t> m_breakpoints;
    std::vector<OutputLine> m_output_lines;
    std::vector<OutputSegment> m_output_segments;
    std::stack<Style> m_input_style_stack;
    std::stack<FormatRep> m_format_stack;
    std::stack<HoldEntry> m_hold_stack;
    const char_type m_newline_char = m_final_out.widen('\n');
    const char_type m_space_char   = m_final_out.widen(' ');
    const bool m_enable_ansi_escape_sequences;
    bool m_word_is_open = false;
    bool m_is_compiling = false;
    bool m_output_line_is_open = false;
    bool m_paragraph_is_open = false;
    bool m_clipping, m_justify, m_fill;
    double m_align;
    double m_round_frac = 0.5;
    std::size_t m_line_number_base;
    std::size_t m_line_number = m_line_number_base;
    std::size_t m_processed_begin = 0;
    std::size_t m_processed_end   = 0;
    std::size_t m_space_begin     = 0;
    std::size_t m_word_begin      = 0; // Has no meaning unless `m_word_is_open` is `true`
    std::size_t m_line_begin      = 0; // Zero unless `m_is_compiling` is `true`
    std::size_t m_section_begin   = 0; // Zero unless `m_is_compiling` is `true`
    std::size_t m_output_end = 0; // One beyond last character in `m_output_buffer`
    std::size_t m_cursor_displacement = 0;
    std::size_t m_inner_left_first_1, m_inner_left_first_2, m_inner_left_rest;
    std::size_t m_inner_right, m_outer_right;

    auto get_input_end() const noexcept -> std::size_t;
    void close_word(std::size_t word_end);
    void close_input_style();
    void do_close_input_style(std::size_t chars_end);
    void clear_input_buffer() noexcept;
    void input_overflow(std::size_t extra_size_needed);
    void format_line(std::size_t line_size);
    void format_line(std::size_t chars_begin, std::size_t chars_end,
                     std::size_t words_begin, std::size_t words_end,
                     const InputStyle*& input_style);
    void fill_and_close();
    void format_newline();
    void do_jump(std::size_t line_number);
    auto prep_format_units(std::size_t chars_begin, std::size_t chars_end,
                           std::size_t words_begin, std::size_t words_end) -> std::size_t;
    void justify_fragment(std::size_t units_begin, std::size_t units_end, std::size_t trailing_space_size,
                          std::size_t old_size, std::size_t new_size) noexcept;
    void do_set_format(const FormatRep&);
    void on_format_changed() noexcept;
    void verify_no_open_section();
    auto do_get_last_line_number() noexcept -> std::size_t;
    auto do_get_cursor_pos() noexcept -> std::size_t;
    void add_output_line();
    void do_open_output_line() noexcept;
    void do_close_output_line();
    void materialize_space();
    void add_space_segment(Style style, std::size_t n);
    void add_output_segment(Style style, std::size_t offset, std::size_t size);
    auto get_output_line() noexcept -> OutputLine&;
    void flush_output();
    void do_flush_output();
    auto get_extended_section_info(std::size_t section_index) -> ExtendedSectionInfo;
    auto get_extended_line_info(std::size_t section_index, std::size_t line_index) -> ExtendedLineInfo;
};


using TextFormatter     = BasicTextFormatter<char>;
using WideTextFormatter = BasicTextFormatter<wchar_t>;



/// \brief Specification of a particular text format.
///
/// An object of this class specifies a particular text format, that is, it specifies a
/// value for each of the formatting parameters of the text formatter. The formatter
/// effectively maintains an instance of this class as its *current format*. The current
/// format of the formatter can be changed, for example, by calling \ref set_format(). Its
/// current format is returned by \ref get_format().
///
/// In general, a default constructed format object has all parameters set to off, zero, or
/// whatever is the default value for the type of the parameter. One exception is word
/// wrapping, which is enabled by default.
///
/// A default constructed format object corresponds to the default format of the text
/// formatter.
///
/// FIXME: Explain box model.                
///
///     --- offset ----><---- width ---->
///                     LLL11xxxxxxxxxRRR
///     1st paragraph   LLL33xxxxxxxxxRRR
///                     LLL33xxxxxxxxxRRR
///                     LLL2222xxxxxxxRRR
///     2nd paragraph   LLL33xxxxxxxxxRRR
///                     LLL33xxxxxxxxxRRR
///                     LLL2222xxxxxxxRRR
///     3rd paragraph   LLL33xxxxxxxxxRRR
///                     LLL33xxxxxxxxxRRR
///
/// Here, `L` and `R` stand for left and right-side padding respectively (\ref
/// set_padding()); `1`, `2`, and `3` stand for first-line-first-paragraph,
/// first-line-nonfirst-paragraph, and nonfirst-line-any-paragraph indentation respectively
/// (\ref set_indent()); and `x` stands for text.
///
/// FIXME: Explain: The role of "offset" is to shift everything to the right. The shape of
/// the formatting box remains unchanged when offset is increased, it just gets shifted to
/// the right.                 
///
/// FIXME: Explain: Fill color is applied as background color to blank and non-blank
/// character positions inside the formatting box (padding + indentation + text).                 
///
/// FIXME: Explain: When "always fill" mode is turned on (\ref set_always_fill()), lines are
/// padded with space characters up to the right-hand side of the formatting box, even when
/// no fill color is specified (\ref set_fill_color()).                 
///
template<class C, class T> class BasicTextFormatter<C, T>::Format {
public:
    Format() noexcept = default;

    bool has_width() const noexcept;
    auto get_width() const noexcept -> std::size_t;
    auto get_indent_first_1() const noexcept -> std::size_t;
    auto get_indent_first_2() const noexcept -> std::size_t;
    auto get_indent_rest() const noexcept -> std::size_t;
    auto get_offset() const noexcept -> std::size_t;
    auto get_padding_left() const noexcept -> std::size_t;
    auto get_padding_right() const noexcept -> std::size_t;
    bool get_word_wrap() const noexcept;
    bool get_justify() const noexcept;
    bool get_clipping() const noexcept;
    double get_align() const noexcept;
    bool has_fill_color() const noexcept;
    auto get_fill_color() const noexcept -> Color;
    bool get_always_fill() const noexcept;
    bool get_norm_whitespace() const noexcept;
    auto get_min_separation() const noexcept -> std::size_t;
    bool get_adv_continuation() const noexcept;
    auto get_max_displacement() const noexcept -> std::size_t;

    void set_width(std::size_t) noexcept;
    void unset_width() noexcept;
    void set_indent(std::size_t) noexcept;
    void set_indent(std::size_t first, std::size_t rest) noexcept;
    void set_indent(std::size_t first_1, std::size_t first_2, std::size_t rest) noexcept;
    void set_offset(std::size_t) noexcept;
    void set_padding(std::size_t) noexcept;
    void set_padding_left(std::size_t) noexcept;
    void set_padding_right(std::size_t) noexcept;
    void set_word_wrap(bool) noexcept;
    void set_justify(bool) noexcept;
    void set_clipping(bool) noexcept;
    void set_align(Align) noexcept;
    void set_align(double) noexcept;
    void set_fill_color(Color) noexcept;
    void unset_fill_color() noexcept;
    void set_always_fill(bool) noexcept;
    void set_norm_whitespace(bool) noexcept;
    void set_min_separation(std::size_t) noexcept;
    void set_adv_continuation(bool) noexcept;
    void set_max_displacement(std::size_t) noexcept;

private:
    FormatRep m_rep;
    Format(const FormatRep&) noexcept;
    friend class BasicTextFormatter;
};



/// \brief State of output cursor.
///
/// This is a specification of the state of the output cursor. That is, a set of parameters,
/// including the position of the cursor, that are used by the formatter to control the
/// target position of the result of formatting of the next input line, as well as some of
/// the aspects of how it is to be formatted (available space).
///
template<class C, class T> class BasicTextFormatter<C, T>::Cursor {
public:
    bool output_line_is_open = false;
    bool paragraph_is_open = false;
    std::size_t line_number = 1;
    std::size_t line_size = 0;
    std::size_t unmaterialized_space = 0;
    std::size_t displacement = 0;

    bool is_valid() const noexcept;
};



/// \brief Text formatter configuration parameters.
///
/// These are the available parameters for configuring the the operation of a text formatter
/// (\ref core::BasicTextFormatter).
///
template<class C, class T> struct BasicTextFormatter<C, T>::Config {
    /// \brief Use high quality, but slow word wrapper.
    ///
    /// If set to `true`, word wrapping will be done using the highest quality method (\ref
    /// word_wrap::KnuthWrapper), but this is also slower than the default method (\ref
    /// word_wrap::greedy()), and has a larger memory footprint.
    ///
    bool high_quality_word_wrapper = false;

    /// \brief Enable emission of ANSI escape sequences.
    ///
    /// Set to `true` to enable emission of ANSI escape sequences to control the text
    /// rendition style.
    ///
    /// Only SGR type escape sequences will be emitted (Select Graphic Rendition).
    ///
    /// Unless enabled, functions such as \ref set_reverse() will have no effect.
    ///
    /// See \ref core::terminal::TextAttributes::change() for more information
    /// about ANSI escape sequences.
    ///
    bool enable_ansi_escape_sequences = false;

    /// \brief Line number to be used for first line.
    ///
    /// The number to be associated with the first generated output line.
    ///
    std::size_t line_number_base = 1;
};



/// \brief Information about closed input section.
///
/// An object of this type carries information about a closed input section. See \ref
/// get_section_info().
///
template<class C, class T> struct BasicTextFormatter<C, T>::SectionInfo {
    /// \brief Size of section.
    ///
    /// This field specifies the size of the closed input section in number of characters
    /// (elements of type \ref char_type).
    ///
    /// FIXME: Can this be zero?                                                                                                                                                                                              
    ///
    std::size_t size;

    /// \brief Number of words in section.
    ///
    /// This field specifies the number of words that occur in the closed input
    /// section. This may be zero.
    ///
    std::size_t num_words;

    /// \brief Number of lines in section.
    ///
    /// This field specifies the number of lines in the input section. It can be zero.
    ///
    /// FIXME: Can this really be zero?                                                                                                                                                                                              
    ///
    /// If the last line of the section is terminated (\ref last_line_is_unterminated is
    /// `false`), then the number of lines in the section, i.e., the value of `num_lines`,
    /// is equal to the number of line terminators (newline characters). Otherwise (when
    /// \ref last_line_is_unterminated is `true`), the number of lines is one more than the
    /// number of line terminators.
    ///
    std::size_t num_lines;

    /// \brief Whether last line is unterminated.
    ///
    /// FIXME: What happens with an empty input section? Can it even be empty, and if it is, does it have one or zero lines?                                                                                             
    ///
    /// This fields is `true` if, and only if the last line in the input section is unterminated. It is terminated when, and only when ???                                                                         
    ///
    bool last_line_is_unterminated;
};



/// \brief Information about line in closed input section.
///
/// An object of this type carries information about a particular line in a closed input
/// section. See \ref get_line_info().
///
template<class C, class T> struct BasicTextFormatter<C, T>::LineInfo {
    /// \brief Position of line within containing input section.
    ///
    /// This field specifies the position of the line within the containing input
    /// section. The position is measured in number of characters (elements of type \ref
    /// char_type), and is relative to the beginning of the containing input section.
    ///
    std::size_t offset;

    /// \brief Size of line.
    ///
    /// This field specifies the size of the line in number of characters (elements of type
    /// \ref char_type). This does not include any surrounding newline characters. The size
    /// of a line may be zero.
    ///
    std::size_t size;

    /// \brief Number of words in line.
    ///
    /// This field specifies the number of words that occur in the input line. This may be
    /// zero.
    ///
    std::size_t num_words;
};



/// \brief Information about word in closed input section.
///
/// An object of this type carries information about a particular word in a closed input
/// section. See \ref get_word_info().
///
template<class C, class T> struct BasicTextFormatter<C, T>::WordInfo {
    /// \brief Position of word within containing input line.
    ///
    /// This field specifies the position of the word within the containing input line. The
    /// position is measured in number of characters (elements of type \ref char_type), and
    /// is relative to the beginning of the containing input line.
    ///
    std::size_t offset;

    /// \brief Size of word.
    ///
    /// This field specifies the size of the word in number of characters (elements of type
    /// \ref char_type). This does not include any surrounding space characters. The size of
    /// a word is always at least one.
    ///
    std::size_t size;
};



template<class C, class T> struct BasicTextFormatter<C, T>::MeasureResult {
    std::size_t min_width_no_oflow;
    std::size_t min_width_no_break;
};



template<class C, class T> struct BasicTextFormatter<C, T>::SimulateResult {
    std::size_t width;
    std::size_t height;
};








// Implementation


template<class C, class T>
inline BasicTextFormatter<C, T>::BasicTextFormatter(ostream_type& out)
    : BasicTextFormatter(out, {}) // Throws
{
}


template<class C, class T>
inline BasicTextFormatter<C, T>::BasicTextFormatter(ostream_type& out, Config config)
    : m_final_out(out)
    , m_locale(m_final_out.getloc()) // Throws
    , m_streambuf(*this)
    , m_out(&m_streambuf) // Throws
    , m_char_mapper(m_locale) // Throws
    , m_enable_ansi_escape_sequences(config.enable_ansi_escape_sequences) // Throws
    , m_line_number_base(config.line_number_base)
{
    m_out.exceptions(std::ios_base::badbit | std::ios_base::failbit); // Throws
    m_out.imbue(m_locale); // Throws
    if (config.high_quality_word_wrapper)
        m_knuth_wrapper = std::make_unique<word_wrap::KnuthWrapper>(); // Throws
    on_format_changed();
    add_output_line(); // Throws
}


template<class C, class T>
template<class V> inline void BasicTextFormatter<C, T>::write(V&& val)
{
    // FIXME: Can be optimized (bypass stream) for case where `val` is of type `string_view_type`.                                  
    // FIXME: Can be optimized (bypass stream) if `val` is of type `char *` by using `m_char_mapper`.                                  
    m_out << std::forward<V>(val); // Throws
}


template<class C, class T>
template<class V> inline void BasicTextFormatter<C, T>::writeln(V&& val)
{
    write(std::forward<V>(val)); // Throws
    write("\n"); // Throws
}


template<class C, class T>
template<class... P> inline void BasicTextFormatter<C, T>::format(const char* pattern, const P&... params)
{
    core::format(m_out, pattern, params...); // Throws
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::out() noexcept -> ostream_type&
{
    return m_out;
}


template<class C, class T>
void BasicTextFormatter<C, T>::finalize()
{
    if (ARCHON_UNLIKELY(m_is_compiling))
        throw std::logic_error("Not allowed while compiling");
    if (ARCHON_UNLIKELY(!m_hold_stack.empty()))
        throw std::logic_error("Not allowed while output is held back");
    close_section(); // Throws
    if (m_output_line_is_open) {
        do_close_output_line(); // Throws
        flush_output(); // Throws
    }
}


template<class C, class T>
void BasicTextFormatter<C, T>::flush()
{
    process_input(); // Throws
    m_final_out.flush(); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_width(std::size_t value)
{
    Format format = { m_format };
    format.set_width(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::unset_width()
{
    Format format = { m_format };
    format.unset_width();
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_indent(std::size_t value)
{
    Format format = { m_format };
    format.set_indent(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_indent(std::size_t first, std::size_t rest)
{
    Format format = { m_format };
    format.set_indent(first, rest);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_indent(std::size_t first_1, std::size_t first_2, std::size_t rest)
{
    Format format = { m_format };
    format.set_indent(first_1, first_2, rest);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_offset(std::size_t value)
{
    Format format = { m_format };
    format.set_offset(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_padding(std::size_t value)
{
    Format format = { m_format };
    format.set_padding(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_padding_left(std::size_t value)
{
    Format format = { m_format };
    format.set_padding_left(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_padding_right(std::size_t value)
{
    Format format = { m_format };
    format.set_padding_right(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_word_wrap(bool value)
{
    Format format = { m_format };
    format.set_word_wrap(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_justify(bool value)
{
    Format format = { m_format };
    format.set_justify(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_clipping(bool value)
{
    Format format = { m_format };
    format.set_clipping(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_align(Align value)
{
    Format format = { m_format };
    format.set_align(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_align(double value)
{
    Format format = { m_format };
    format.set_align(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_fill_color(Color value)
{
    Format format = { m_format };
    format.set_fill_color(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::unset_fill_color()
{
    Format format = { m_format };
    format.unset_fill_color();
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_always_fill(bool value)
{
    Format format = { m_format };
    format.set_always_fill(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_norm_whitespace(bool value)
{
    Format format = { m_format };
    format.set_norm_whitespace(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_min_separation(std::size_t value)
{
    Format format = { m_format };
    format.set_min_separation(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_adv_continuation(bool value)
{
    Format format = { m_format };
    format.set_adv_continuation(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_max_displacement(std::size_t value)
{
    Format format = { m_format };
    format.set_max_displacement(value);
    set_format(format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::reset_format()
{
    do_set_format({}); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::push_format()
{
    m_format_stack.push(m_format); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::pop_format()
{
    if (ARCHON_UNLIKELY(m_format_stack.empty()))
        throw std::logic_error("Empty format stack");
    do_set_format(m_format_stack.top()); // Throws
    m_format_stack.pop();
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_weight(Weight value)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.set_weight(value);
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_underline(bool value)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.set_underline(value);
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_blink(bool value)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.set_blink(value);
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_reverse(bool value)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.set_reverse(value);
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_color(Color value)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.set_color(value);
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_background_color(Color value)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.set_background_color(value);
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::unset_color()
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.unset_color();
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::unset_background_color()
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style.unset_background_color();
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::reset_style()
{
    set_style({}); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::push_style()
{
    m_input_style_stack.push(m_input_style); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::pop_style()
{
    if (ARCHON_UNLIKELY(m_input_style_stack.empty()))
        throw std::logic_error("Empty style stack");
    Style style = m_input_style_stack.top();
    set_style(style); // Throws
    m_input_style_stack.pop();
}


template<class C, class T>
void BasicTextFormatter<C, T>::process_input()
{
    const char_type* base  = m_input_buffer.data();
    const char_type* begin = base + m_processed_begin;
    const char_type* end   = base + get_input_end();
    const char_type* i     = base + m_processed_end;
    ARCHON_ASSERT(i <= end);
    char_type ch;

    if (!m_word_is_open)
        goto space;
    goto word;

  transition_to_space:
    ++i;

  space:
    if (ARCHON_UNLIKELY(i == end))
        goto end;
    ch = *i;
    if (ch == m_space_char) {
        ++i;
        goto space;
    }
    if (ARCHON_LIKELY(ch != m_newline_char))
        goto transition_to_word;
    goto transition_to_new_line;

  transition_to_word:
    m_word_begin = std::size_t(i - begin);
    m_word_is_open = true;
    ++i;

  word:
    if (ARCHON_UNLIKELY(i == end))
        goto end;
    ch = *i;
    if (ARCHON_LIKELY(ch != m_space_char)) {
        if (ARCHON_LIKELY(ch != m_newline_char)) {
            ++i;
            goto word;
        }
        close_word(std::size_t(i - begin)); // Throws
        goto transition_to_new_line;
    }
    close_word(std::size_t(i - begin)); // Throws
    goto transition_to_space;

  transition_to_new_line:
    ARCHON_ASSERT(!m_word_is_open);
    if (!m_is_compiling) {
        std::size_t line_size = std::size_t(i - begin);
        format_line(line_size); // Throws
        fill_and_close(); // Throws
        format_newline(); // Throws
        flush_output(); // Throws
        ++i;
        m_processed_begin = std::size_t(i - base);
        begin = base + m_processed_begin;
        m_space_begin = 0;
    }
    else {
        std::size_t line_end = std::size_t(i - begin);
        ++i;
        std::size_t line_size  = std::size_t(line_end - m_line_begin);
        std::size_t chars_end  = std::size_t(i - begin);
        std::size_t words_end  = m_input_words.size();
        m_input_lines.push_back({ line_size, chars_end, words_end }); // Throws
        m_space_begin  = chars_end;
        m_line_begin   = chars_end;
    }
    goto space;

  end:
    m_processed_end = std::size_t(i - base);
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_line_number() -> std::size_t
{
    verify_no_open_section(); // Throws
    return m_line_number;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_last_line_number() -> std::size_t
{
    verify_no_open_section(); // Throws
    return do_get_last_line_number();
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_cursor_pos() -> std::size_t
{
    verify_no_open_section(); // Throws
    if (m_output_line_is_open)
        return do_get_cursor_pos();
    return 0;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::skip_line(std::size_t n)
{
    verify_no_open_section(); // Throws
    std::size_t line_number = m_line_number;
    core::int_add(line_number, n); // Throws
    do_jump(line_number); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::close_section()
{
    process_input(); // Throws
    std::size_t section_end = std::size_t(m_processed_end - m_processed_begin);
    if (m_word_is_open)
        close_word(section_end);
    if (!m_is_compiling) {
        std::size_t line_size = section_end;
        format_line(line_size); // Throws
        flush_output(); // Throws
        clear_input_buffer();
    }
    else {
        do_close_input_style(section_end); // Throws
        std::size_t section_size = std::size_t(section_end - m_section_begin);
        std::size_t chars_end  = section_end;
        std::size_t words_end  = m_input_words.size();
        std::size_t lines_end  = m_input_lines.size();
        std::size_t styles_end = m_input_styles.size();
        InputSection section = { section_size, chars_end, words_end, lines_end, styles_end };
        m_input_sections.push_back(section); // Throws
        m_space_begin   = section_end;
        m_line_begin    = section_end;
        m_section_begin = section_end;
    }
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::has_open_section()
{
    process_input(); // Throws
    std::size_t section_end = std::size_t(m_processed_end - m_processed_begin);
    std::size_t section_size = std::size_t(section_end - m_section_begin);
    return (section_size > 0);
}


template<class C, class T>
void BasicTextFormatter<C, T>::begin_compile()
{
    if (ARCHON_UNLIKELY(m_is_compiling))
        throw std::logic_error("Not allowed while compiling");
    close_section(); // Throws
    m_is_compiling = true;
}


template<class C, class T>
void BasicTextFormatter<C, T>::end_compile() noexcept
{
    if (m_is_compiling) {
        m_word_is_open = false;
        m_is_compiling = false;
        m_line_begin    = 0;
        m_section_begin = 0;
        m_input_words.clear();
        m_input_lines.clear();
        m_input_sections.clear();
        m_input_styles.clear();
        clear_input_buffer();
    }
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::is_compiling() const noexcept
{
    return m_is_compiling;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_num_sections() const noexcept -> std::size_t
{
    return m_input_sections.size();
}


template<class C, class T>
void BasicTextFormatter<C, T>::format_section(std::size_t section_index)
{
    const InputSection& section = m_input_sections.at(section_index); // Throws
    ARCHON_ASSERT(m_is_compiling);
    std::size_t chars_begin = 0;
    std::size_t words_begin = 0;
    std::size_t lines_begin = 0;
    std::size_t style_index = 0;
    if (section_index > 0) {
        const InputSection& prev_section = m_input_sections[section_index - 1];
        chars_begin = prev_section.chars_end;
        words_begin = prev_section.words_end;
        lines_begin = prev_section.lines_end;
        style_index = prev_section.styles_end;
    }
    const InputStyle* input_style = m_input_styles.data() + style_index;
    std::size_t lines_end = section.lines_end;
    for (std::size_t i = lines_begin; i < lines_end; ++i) {
        const InputLine& line = m_input_lines[i];
        std::size_t chars_end = std::size_t(chars_begin + line.line_size);
        std::size_t words_end = line.words_end;
        format_line(chars_begin, chars_end, words_begin, words_end, input_style); // Throws
        fill_and_close(); // Throws
        format_newline(); // Throws
        flush_output(); // Throws
        chars_begin = line.chars_end;
        words_begin = words_end;
    }
    format_line(chars_begin, section.chars_end, words_begin, section.words_end, input_style); // Throws
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_section_info(std::size_t section_index) -> SectionInfo
{
    ExtendedSectionInfo info = get_extended_section_info(section_index); // Throws
    return info.base;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_line_info(std::size_t section_index, std::size_t line_index) -> LineInfo
{
    ExtendedLineInfo info = get_extended_line_info(section_index, line_index); // Throws
    return info.base;
}


template<class C, class T>
auto BasicTextFormatter<C, T>::get_word_info(std::size_t section_index, std::size_t line_index,
                                             std::size_t word_index) -> WordInfo
{
    ExtendedLineInfo line_info = get_extended_line_info(section_index, line_index); // Throws
    if (ARCHON_UNLIKELY(word_index >= line_info.num_words))
        throw std::out_of_range("Word index");
    std::size_t word_index_2 = std::size_t(line_info.words_begin + word_index);
    const InputWord& word = m_input_words[word_index_2];
    std::size_t offset = std::size_t(word.chars_end - word.word_size - line_info.chars_begin);
    std::size_t size   = word.word_size;
    return { offset, size };
}


template<class C, class T>
auto BasicTextFormatter<C, T>::measure(std::size_t section_index, const Cursor& cursor) -> MeasureResult
{
    if (ARCHON_UNLIKELY(!cursor.is_valid()))
        throw std::logic_error("Invalid cursor state");
    // FIXME: Check that format_line effectively treats m_paragraph_is_open same as below, in particular in the case of the applied indentation on second input line of a section where output line was open at the beginning of the section, and paragraph was not open at the beginning of the section, and the first input line of the section did not produce any output                                                         
    const InputSection& section = m_input_sections.at(section_index); // Throws
    ARCHON_ASSERT(m_is_compiling);
    std::size_t inner_left_first = m_inner_left_first_1;
    if (cursor.paragraph_is_open)
        inner_left_first = m_inner_left_first_2;
    std::size_t cursor_pos = inner_left_first;
    if (cursor.output_line_is_open) {
        cursor_pos = std::size_t(cursor.line_size - cursor.displacement);
        if (inner_left_first >= cursor_pos)
            cursor_pos = inner_left_first;
    }
    std::size_t cursor_pos_after_empty = inner_left_first;
    bool word_wrap = (m_format.word_wrap && m_format.has_width);
    std::size_t max = std::size_t(-1);
    std::size_t max_inner_right_1 = 0;
    std::size_t max_inner_right_2 = 0;
    auto process_line = [&](std::size_t chars_begin, std::size_t chars_end,
                            std::size_t words_begin, std::size_t words_end) {
        std::size_t trailing_space_size = prep_format_units(chars_begin, chars_end, words_begin, words_end); // Throws
        std::size_t num_units = m_format_units.size();
        bool nonempty_output = (num_units > 0 || trailing_space_size > 0);
        if (!nonempty_output) {
            cursor_pos = cursor_pos_after_empty;
            return;
        }
        if (word_wrap) {
            // FIXME: Explain how the scheme below relies on the constraint that for a particular input line, inner size is the same for all possible output lines after the first one. If this was not the case, the computation would probably be a lot more expensive, and the guarantee about no overflow for any width greater than min_width_no_oflow could probably not be maintained.                     

            // First put first word on first line.
            // Then keep adding words to first line while they end up in a position no further to the right than they would be as the first word on a subsequent line.
            // The sum of cursor pos and the size of the first line capped at max becomes the initial value of inner_right_1
            // For each remaining word: If sum of m_inner_left_rest and word size, capped at max, is greater than inner_right_1, set inner_right_1 to that sum.

            // Note: Since whitespace normalization can never increse the amount of
            // whitespace, the computations of `size_1_first`, `size_2` cannot overflow,
            // because neither of them can be greater than the size, in characters, of this
            // input section, and the size of any input section is, by design, representable
            // in std::size_t.
            auto i   = m_format_units.begin();
            auto end = m_format_units.end();
            std::size_t size_1_first = 0;
            if (i != end) {
                size_1_first = std::size_t(i->space_size + i->word_size);
                ++i;
                if (m_inner_left_rest >= cursor_pos) {
                    std::size_t diff = std::size_t(m_inner_left_rest - cursor_pos);
                    while (i != end && std::size_t(size_1_first + i->space_size) <= diff) {
                        size_1_first += i->space_size + i->word_size;
                        ++i;
                    }
                }
            }
            std::size_t size_2 = size_1_first;
            if (i != end) {
                std::size_t size_1_rest = 0;
                for (;;) {
                    std::size_t word_size = i->word_size;
                    size_2 += i->space_size + word_size;
                    ++i;
                    if (ARCHON_UNLIKELY(i == end)) {
                        word_size += trailing_space_size;
                        if (ARCHON_UNLIKELY(word_size > size_1_rest))
                            size_1_rest = word_size;
                        break;
                    }
                    if (ARCHON_UNLIKELY(word_size > size_1_rest))
                        size_1_rest = word_size;
                }
                std::size_t inner_right_1_rest = max;
                if (ARCHON_LIKELY(size_1_rest <= max - m_inner_left_rest))
                    inner_right_1_rest = m_inner_left_rest + size_1_rest;
                if (ARCHON_UNLIKELY(inner_right_1_rest > max_inner_right_1))
                    max_inner_right_1 = inner_right_1_rest;
            }
            else {
                size_1_first += trailing_space_size;
            }
            size_2 += trailing_space_size;
            std::size_t inner_right_1_first = max;
            if (ARCHON_LIKELY(size_1_first <= std::size_t(max - cursor_pos)))
                inner_right_1_first = std::size_t(cursor_pos + size_1_first);
            if (ARCHON_UNLIKELY(inner_right_1_first > max_inner_right_1))
                max_inner_right_1 = inner_right_1_first;
            std::size_t inner_right_2 = max;
            if (ARCHON_LIKELY(size_2 <= std::size_t(max - cursor_pos)))
                inner_right_2 = std::size_t(cursor_pos + size_2);
            if (ARCHON_UNLIKELY(inner_right_2 > max_inner_right_2))
                max_inner_right_2 = inner_right_2;
        }
        else {
            std::size_t size = trailing_space_size;
            for (std::size_t i = 0; i < num_units; ++i) {
                const FormatUnit& unit = m_format_units[i];
                size += std::size_t(unit.space_size + unit.word_size);
            }
            std::size_t inner_right = max;
            if (ARCHON_LIKELY(size <= std::size_t(max - cursor_pos)))
                inner_right = std::size_t(cursor_pos + size);
            if (ARCHON_UNLIKELY(inner_right > max_inner_right_1)) {
                max_inner_right_1 = inner_right;
                max_inner_right_2 = inner_right;
            }
        }
        cursor_pos = m_inner_left_first_2;
    };
    std::size_t chars_begin = 0;
    std::size_t words_begin = 0;
    std::size_t lines_begin = 0;
    if (section_index > 0) {
        const InputSection& prev_section = m_input_sections[section_index - 1];
        chars_begin = prev_section.chars_end;
        words_begin = prev_section.words_end;
        lines_begin = prev_section.lines_end;
    }
    std::size_t lines_end = section.lines_end;
    for (std::size_t i = lines_begin; i < lines_end; ++i) {
        const InputLine& line = m_input_lines[i];
        std::size_t chars_end = std::size_t(chars_begin + line.line_size);
        std::size_t words_end = line.words_end;
        process_line(chars_begin, chars_end, words_begin, words_end); // Throws
        cursor_pos_after_empty = m_inner_left_first_1;
        chars_begin = line.chars_end;
        words_begin = words_end;
    }
    process_line(chars_begin, section.chars_end, words_begin, section.words_end); // Throws
    std::size_t max_outer_right_1 = max;
    if (ARCHON_LIKELY(m_format.padding_right <= std::size_t(max - max_inner_right_1)))
        max_outer_right_1 = std::size_t(max_inner_right_1 + m_format.padding_right);
    std::size_t max_outer_right_2 = max;
    if (ARCHON_LIKELY(m_format.padding_right <= std::size_t(max - max_inner_right_2)))
        max_outer_right_2 = std::size_t(max_inner_right_2 + m_format.padding_right);
    // FIXME: Looks line max_outer_right_1 and max_outer_right_2 are both guaranteed to be greater than, or equal to m_format.offset due to the behaviour of jumping to left margin before formatting an input line, even when output line is already open. However, maybe there should be a flag to determine whether this jump is enabled.                                                      
    std::size_t min_width_no_oflow = 0;
    if (ARCHON_LIKELY(max_outer_right_1 >= m_format.offset))
        min_width_no_oflow = std::size_t(max_outer_right_1 - m_format.offset);
    std::size_t min_width_no_break = 0;
    if (ARCHON_LIKELY(max_outer_right_2 >= m_format.offset))
        min_width_no_break = std::size_t(max_outer_right_2 - m_format.offset);
    return { min_width_no_oflow, min_width_no_break };
}


template<class C, class T>
auto BasicTextFormatter<C, T>::simulate(std::size_t section_index, std::size_t width) -> SimulateResult
{
    const InputSection& section = m_input_sections.at(section_index); // Throws
    ARCHON_ASSERT(m_is_compiling);

    // Note: Since whitespace normalization can never increse the amount of whitespace,                                                                        
    // neither the computation of the width nor the height can overflow, because neither can
    // be greater than the size, in characters, of this input section, and the size of any
    // input section is, by design, representable in std::size_t.

    std::size_t max = std::size_t(-1);
    std::size_t inner_left_first_1 = max;
    std::size_t inner_left_first_2 = max;
    std::size_t inner_left_rest = max;
    std::size_t inner_right = 0;
    const FormatRep& f = m_format;
    if (ARCHON_LIKELY(f.indent_first_1 <= std::size_t(max - f.padding_left)))
        inner_left_first_1 = std::size_t(f.padding_left + f.indent_first_1);
    if (ARCHON_LIKELY(f.indent_first_2 <= std::size_t(max - f.padding_left)))
        inner_left_first_2 = std::size_t(f.padding_left + f.indent_first_2);
    if (ARCHON_LIKELY(f.indent_rest <= std::size_t(max - f.padding_left)))
        inner_left_rest = std::size_t(f.padding_left + f.indent_rest);
    if (ARCHON_LIKELY(f.padding_right <= width))
        inner_right = std::size_t(width - f.padding_right);
    bool output_line_is_open = false;
    bool paragraph_is_open = false;
    std::size_t max_cursor_pos = 0;
    std::size_t height = 0;
    auto process_fragment = [&](std::size_t units_begin, std::size_t units_end,
                                std::size_t trailing_space_size, std::size_t inner_left) {
        std::size_t size = trailing_space_size;
        for (std::size_t i = units_begin; i < units_end; ++i) {
            const FormatUnit& unit = m_format_units[i];
            size += unit.space_size + unit.word_size;
        }
        if (ARCHON_UNLIKELY(size > std::size_t(max - inner_left)))
            throw std::overflow_error("Width");
        std::size_t cursor_pos = std::size_t(inner_left + size);
        if (ARCHON_UNLIKELY(cursor_pos > max_cursor_pos))
            max_cursor_pos = cursor_pos;
    };
    auto process_line = [&](std::size_t chars_begin, std::size_t chars_end,
                            std::size_t words_begin, std::size_t words_end) {
        ARCHON_ASSERT(!output_line_is_open);
        std::size_t trailing_space_size = prep_format_units(chars_begin, chars_end, words_begin, words_end); // Throws
        std::size_t num_units = m_format_units.size();
        bool nonempty_output = (num_units > 0 || trailing_space_size > 0);
        if (!nonempty_output)
            return;
        std::size_t inner_left_first = inner_left_first_2;
        if (!paragraph_is_open) {
            inner_left_first = inner_left_first_1;
            paragraph_is_open = true;
        }
        output_line_is_open = true;
        std::size_t inner_left = inner_left_first;
        std::size_t units_begin = 0;
        if (m_format.word_wrap) {
            std::size_t inner_size_first = 0, inner_size_rest = 0;
            if (ARCHON_LIKELY(inner_left_first <= inner_right))
                inner_size_first = std::size_t(inner_right - inner_left_first);
            if (ARCHON_LIKELY(inner_left_rest <= inner_right))
                inner_size_rest = std::size_t(inner_right - inner_left_rest);
            word_wrap::Geometry geometry[] = {
                { inner_size_first, 1 },
                { inner_size_rest,  1 },
            };
            if (!m_knuth_wrapper) {
                // Fastest
                word_wrap::greedy(m_format_units, trailing_space_size, geometry, m_breakpoints); // Throws
            }
            else {
                // Best quality
                m_knuth_wrapper->wrap(m_format_units, trailing_space_size, geometry, m_breakpoints); // Throws
            }
            for (std::size_t breakpoint : m_breakpoints) {
                std::size_t units_end = breakpoint;
                std::size_t trailing_space_size_2 = 0;
                process_fragment(units_begin, units_end, trailing_space_size_2, inner_left); // Throws
                ++height;
                m_format_units[units_end].space_size = 0;
                inner_left = inner_left_rest;
                units_begin = units_end;
            }
        }
        std::size_t units_end = m_format_units.size();
        process_fragment(units_begin, units_end, trailing_space_size, inner_left); // Throws
    };
    std::size_t chars_begin = 0;
    std::size_t words_begin = 0;
    std::size_t lines_begin = 0;
    if (section_index > 0) {
        const InputSection& prev_section = m_input_sections[section_index - 1];
        chars_begin = prev_section.chars_end;
        words_begin = prev_section.words_end;
        lines_begin = prev_section.lines_end;
    }
    std::size_t lines_end = section.lines_end;
    for (std::size_t i = lines_begin; i < lines_end; ++i) {
        const InputLine& line = m_input_lines[i];
        std::size_t chars_end = std::size_t(chars_begin + line.line_size);
        std::size_t words_end = line.words_end;
        process_line(chars_begin, chars_end, words_begin, words_end); // Throws
        if (output_line_is_open) {
            // Leave paragraph open, but close line
            output_line_is_open = false;
        }
        else {
            // Line was not opened, so close paragraph
            paragraph_is_open = false;
        }
        ++height;
        chars_begin = line.chars_end;
        words_begin = words_end;
    }
    process_line(chars_begin, section.chars_end, words_begin, section.words_end); // Throws
    if (output_line_is_open)
        ++height;
    std::size_t width_2 = 0;
    if (height > 0) {
        std::size_t padding_right = m_format.padding_right;
        if (ARCHON_UNLIKELY(padding_right > std::size_t(max - max_cursor_pos)))
            throw std::overflow_error("Width");
        width_2 = std::size_t(max_cursor_pos + padding_right);
    }
    return { width_2, height };
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_cursor_state() -> Cursor
{
    std::size_t cursor_pos = get_cursor_pos(); // Throws
    return { m_output_line_is_open, m_paragraph_is_open, m_line_number, cursor_pos };                                   
}


template<class C, class T>
void BasicTextFormatter<C, T>::begin_hold()
{
    verify_no_open_section(); // Throws
    std::size_t hold_line_number    = m_line_number;
    std::size_t reached_line_number = m_line_number;
    m_hold_stack.push({ hold_line_number, reached_line_number }); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::end_hold()
{
    verify_no_open_section(); // Throws
    if (ARCHON_UNLIKELY(m_hold_stack.empty()))
        throw std::logic_error("Output is not currently held back");
    const HoldEntry& top = m_hold_stack.top();
    if (top.reached_line_number > m_line_number) {
        if (m_output_line_is_open)
            do_close_output_line(); // Throws
        m_line_number = top.reached_line_number;
    }
    m_hold_stack.pop();
    if (m_hold_stack.empty())
        do_flush_output(); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::jump_back()
{
    std::size_t line_number = get_hold_line_number(); // Throws
    do_jump(line_number); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::jump(std::size_t line_number)
{
    verify_no_open_section(); // Throws
    do_jump(line_number); // Throws
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_hold_line_number() -> std::size_t
{
    verify_no_open_section(); // Throws
    if (ARCHON_UNLIKELY(m_hold_stack.empty()))
        throw std::logic_error("Output is not currently held back");
    const HoldEntry& top = m_hold_stack.top();
    return top.hold_line_number;
}


template<class C, class T>
auto BasicTextFormatter<C, T>::get_reached_line_number() -> std::size_t
{
    verify_no_open_section(); // Throws
    if (ARCHON_UNLIKELY(m_hold_stack.empty()))
        throw std::logic_error("Output is not currently held back");
    const HoldEntry& top = m_hold_stack.top();
    if (m_line_number >= top.reached_line_number)
        return m_line_number;
    return top.reached_line_number;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_format() const noexcept -> Format
{
    return { m_format };
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_format(const Format& f)
{
    do_set_format(f.m_rep); // Throws
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_style() const noexcept -> Style
{
    return m_input_style;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::set_style(Style style)
{
    if (m_enable_ansi_escape_sequences) {
        close_input_style(); // Throws
        m_input_style = style;
    }
}


template<class C, class T>
class BasicTextFormatter<C, T>::StreambufImpl : public std::basic_streambuf<C, T> {
public:
    using int_type = typename std::basic_streambuf<C, T>::int_type;

    explicit StreambufImpl(BasicTextFormatter&) noexcept;
    void put(const char_type* data, std::size_t size);
    void fill(char_type ch, std::size_t size);
    auto get_put_ptr() const noexcept -> char_type*;
    void reset_put_area(char_type* begin, char_type* end) noexcept;

protected:
    auto xsputn(const char_type*, std::streamsize) -> std::streamsize override final;
    auto overflow(int_type) -> int_type override final;
    int sync() override final;

private:
    BasicTextFormatter& m_formatter;
    void bump_put_ptr(std::size_t) noexcept;
};


template<class C, class T>
inline BasicTextFormatter<C, T>::StreambufImpl::StreambufImpl(BasicTextFormatter& formatter) noexcept
    : m_formatter(formatter)
{
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::StreambufImpl::get_put_ptr() const noexcept -> char_type*
{
    return this->pptr();
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::StreambufImpl::reset_put_area(char_type* begin, char_type* end) noexcept
{
    this->setp(begin, end);
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::StreambufImpl::put(const char_type* data, std::size_t size)
{
    std::size_t avail = std::size_t(this->epptr() - this->pptr());
    if (ARCHON_LIKELY(size <= avail))
        goto finish;
    m_formatter.input_overflow(size); // Throws
  finish:
    std::copy_n(data, size, this->pptr());
    bump_put_ptr(size);
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::StreambufImpl::fill(const char_type ch, std::size_t size)
{
    std::size_t avail = std::size_t(this->epptr() - this->pptr());
    if (ARCHON_LIKELY(size <= avail))
        goto finish;
    m_formatter.input_overflow(size); // Throws
  finish:
    std::fill_n(this->pptr(), size, ch);
    bump_put_ptr(size);
}


template<class C, class T>
auto BasicTextFormatter<C, T>::StreambufImpl::xsputn(const char_type* s, std::streamsize count) -> std::streamsize
{
    if (ARCHON_LIKELY(count >= 0)) {
        std::streamsize offset = 0;
        using uint_type = std::make_unsigned_t<std::streamsize>;
        std::size_t max = std::numeric_limits<std::size_t>::max();
        if (ARCHON_LIKELY(uint_type(count) <= max))
            goto finish;
        do {
            put(s + offset, max); // Throws
            offset += std::streamsize(max);
        }
        while (uint_type(count - offset) > max);
      finish:
        put(s + offset, std::size_t(count - offset)); // Throws
    }
    return count;
}


template<class C, class T>
auto BasicTextFormatter<C, T>::StreambufImpl::overflow(int_type ch) -> int_type
{
    m_formatter.input_overflow(1);
    if (!traits_type::eq_int_type(ch, traits_type::eof())) {
        ARCHON_ASSERT(this->pptr() < this->epptr());
        *this->pptr() = traits_type::to_char_type(ch);
        this->pbump(1);
    }
    return 0;
}


template<class C, class T>
int BasicTextFormatter<C, T>::StreambufImpl::sync()
{
    m_formatter.process_input(); // Throws
    return 0;
}


template<class C, class T>
void BasicTextFormatter<C, T>::StreambufImpl::bump_put_ptr(std::size_t size) noexcept
{
    std::size_t size_2 = size;
    int max = std::numeric_limits<int>::max();
    if (ARCHON_LIKELY(size_2 <= unsigned(max)))
        goto finish;
    do {
        this->pbump(max);
        size_2 -= unsigned(max);
    }
    while (size_2 > unsigned(max));
  finish:
    this->pbump(int(size_2));
}


template<class C, class T>
struct BasicTextFormatter<C, T>::FormatRep {
    bool has_width = false;
    bool word_wrap = true;
    bool justify = false;
    bool clipping = false;
    bool has_fill_color = false;
    bool always_fill = false;
    bool norm_whitespace = false;
    bool adv_continuation = false;
    std::size_t offset = 0;
    std::size_t width = 0;
    std::size_t padding_left = 0;
    std::size_t padding_right = 0;
    std::size_t indent_first_1 = 0;
    std::size_t indent_first_2 = 0;
    std::size_t indent_rest = 0;
    std::size_t min_separation = 0;
    std::size_t max_displacement = 0;
    double align = 0;
    Color fill_color = {};
};


// Records the termination of an input word
template<class C, class T>
struct BasicTextFormatter<C, T>::InputWord {
    // The number of characters (elements of type `char_type`) in this word. This does not
    // include any surrounding spaces. This is always at least one.
    std::size_t word_size;

    // Marks the end of the word within the input buffer relative to the beginning of first                  
    // retained section (one beyond index of last non-space character).
    std::size_t chars_end;
};


// Records the termination of an input line
template<class C, class T>
struct BasicTextFormatter<C, T>::InputLine {
    // The number of characters (elements of type `char_type`) in this line. This does not
    // include any surrounding newline characters. Can be zero.
    std::size_t line_size;

    // Marks the end of the line terminator within the input buffer relative to the                  
    // beginning of first retained section (one beyond the position of the newline
    // character).
    std::size_t chars_end;

    // Marks the end of the line within `m_input_words` (one beyond index of last word).
    std::size_t words_end;
};


// Records the termination of an input section
template<class C, class T>
struct BasicTextFormatter<C, T>::InputSection {
    // The number of characters (elements of type `char_type`) in this section. Can be zero.
    std::size_t section_size;

    // Marks the end of the section within the input buffer relative to the beginning of
    // first retained section (one beyond index of last character).
    std::size_t chars_end;

    // Marks the end of the section within `m_input_words` (one beyond index of last word).
    std::size_t words_end;

    // Marks the end of the section within `m_input_lines` (one beyond index of last line
    // termination).
    std::size_t lines_end;

    // Marks the end of the section within `m_input_styles` (one beyond index of last style
    // termination).
    std::size_t styles_end;
};


// Records the termination of an input style
template<class C, class T>
struct BasicTextFormatter<C, T>::InputStyle {
    // Marks the end of an input style (one beyond index of last character) within the input             
    // buffer relative to the beginning of first retained section.
    std::size_t chars_end;

    // The index within `m_input_words` of the first word that ends strictly beyond the
    // ending of this style, or, if there are no such words, one plus the index of the last
    // word in `m_input_words`.
    std::size_t word_index;

    // The attributes of the style that ends here.
    Style style;
};


template<class C, class T>
struct BasicTextFormatter<C, T>::OutputLine {
    // Number of characters (elements of type `char_type`) in this output line, not
    // including a final newline character.
    std::size_t size;

    // The index within `m_output_segments` of the last segment of this line. A value of
    // `std::size_t(-1)` means that there are no segments in this line.
    std::size_t last_segment;

    // The amount of trailing space that has not yet been materialized in the output
    // buffer. This is included in `size`.
    std::size_t unmaterialized_space;
};


// A segment of an output line with uniform style
template<class C, class T>
struct BasicTextFormatter<C, T>::OutputSegment {
    // The style associated with this segment.
    Style style;

    // The position within `m_output_buffer` of the beginning of this segment.
    std::size_t offset;

    // The number of characters (elements of type `char_type`) in this segment.
    std::size_t size;

    // The index within `m_output_segments` of the next segment of this line. Or if this
    // segment is the last one, the index of the first segment.
    std::size_t next;
};


template<class C, class T>
struct BasicTextFormatter<C, T>::HoldEntry {
    // Line number where hold was initiated.
    std::size_t hold_line_number;

    // Greatest line number reached during this hold prior to last jump.
    std::size_t reached_line_number;
};


template<class C, class T>
struct BasicTextFormatter<C, T>::ExtendedSectionInfo {
    SectionInfo base;
    std::size_t chars_begin;
    std::size_t words_begin;
    std::size_t lines_begin;
    std::size_t last_line_size;
};


template<class C, class T>
struct BasicTextFormatter<C, T>::ExtendedLineInfo {
    LineInfo base;
    std::size_t chars_begin;
    std::size_t words_begin;
};


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_input_end() const noexcept -> std::size_t
{
    return std::size_t(m_streambuf.get_put_ptr() - m_input_buffer.data());
}


template<class C, class T>
void BasicTextFormatter<C, T>::close_word(std::size_t word_end)
{
    ARCHON_ASSERT(m_word_is_open);
    std::size_t word_size = std::size_t(word_end - m_word_begin);
    std::size_t chars_end = word_end;
    m_input_words.push_back({ word_size, chars_end }); // Throws
    m_word_is_open = false;
    m_space_begin = word_end;
    if (!m_input_styles.empty()) {
        InputStyle& style = m_input_styles.back();
        if (style.chars_end == word_end) {
            ARCHON_ASSERT(style.word_index == m_input_words.size() - 1);
            style.word_index = m_input_words.size();
        }
    }
}


template<class C, class T>
void BasicTextFormatter<C, T>::close_input_style()
{
    ARCHON_ASSERT(m_enable_ansi_escape_sequences);
    process_input(); // Throws
    std::size_t chars_end = std::size_t(m_processed_end - m_processed_begin);
    do_close_input_style(chars_end); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::do_close_input_style(std::size_t chars_end)
{
    std::size_t chars_begin = 0;
    if (!m_input_styles.empty())
        chars_begin = m_input_styles.back().chars_end;
    ARCHON_ASSERT(chars_end >= chars_begin);
    bool is_empty = (chars_end == chars_begin);
    if (is_empty)
        return;
    std::size_t word_index = m_input_words.size();
    m_input_styles.push_back({ chars_end, word_index, m_input_style }); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::clear_input_buffer() noexcept
{
    ARCHON_ASSERT(!m_word_is_open);
    ARCHON_ASSERT(!m_is_compiling);
    m_processed_begin = 0;
    m_processed_end   = 0;
    char_type* base = m_input_buffer.data();
    std::size_t size  = m_input_buffer.size();
    m_streambuf.reset_put_area(base, base + size);
    m_space_begin = 0;
}


template<class C, class T>
void BasicTextFormatter<C, T>::input_overflow(std::size_t extra_size_needed)
{
    process_input(); // Throws
    char_type* base = m_input_buffer.data();
    std::size_t buffer_size = m_input_buffer.size();
    std::size_t input_end = get_input_end();
    ARCHON_ASSERT(extra_size_needed > std::size_t(buffer_size - input_end));
    std::size_t used_size = std::size_t(input_end - m_processed_begin);
    if (std::size_t(buffer_size - used_size) >= extra_size_needed) {
        ARCHON_ASSERT(m_processed_begin > 0);
        std::copy_n(base + m_processed_begin, used_size, base);
    }
    else {
        auto copy_func = [&](core::Span<char_type> new_mem) noexcept {
            std::copy_n(base + m_processed_begin, used_size, new_mem.data());
            base = new_mem.data();
            buffer_size = new_mem.size();
        };
        m_input_buffer.reserve_extra_a(extra_size_needed, used_size, copy_func); // Throws
    }
    input_end         -= m_processed_begin;
    m_processed_end   -= m_processed_begin;
    m_processed_begin  = 0;
    m_streambuf.reset_put_area(base + input_end, base + buffer_size);
}


template<class C, class T>
void BasicTextFormatter<C, T>::format_line(std::size_t line_size)
{
    ARCHON_ASSERT(!m_word_is_open);
    ARCHON_ASSERT(!m_is_compiling);
    ARCHON_ASSERT(m_line_begin    == 0);
    ARCHON_ASSERT(m_section_begin == 0);
    ARCHON_ASSERT(m_input_lines.empty());
    ARCHON_ASSERT(m_input_sections.empty());
    do_close_input_style(line_size); // Throws
    std::size_t chars_begin = 0;
    std::size_t chars_end   = line_size;
    std::size_t words_begin = 0;
    std::size_t words_end   = m_input_words.size();
    const InputStyle* input_style = m_input_styles.data();
    format_line(chars_begin, chars_end, words_begin, words_end, input_style); // Throws
    m_input_words.clear();
    m_input_styles.clear();
}


template<class C, class T>
void BasicTextFormatter<C, T>::format_line(std::size_t chars_begin, std::size_t chars_end,
                                           std::size_t words_begin, std::size_t words_end,
                                           const InputStyle*& input_style)
{
    std::size_t trailing_space_size = prep_format_units(chars_begin, chars_end, words_begin, words_end); // Throws
    std::size_t num_units = m_format_units.size();
    bool nonempty_output = (num_units > 0 || trailing_space_size > 0);
    if (!nonempty_output)
        return;
    auto get_format_unit = [&](std::size_t word_index) noexcept -> FormatUnit& {
        return m_format_units[word_index - words_begin];
    };
    const char_type* input = m_input_buffer.data() + m_processed_begin;
    // Format a segment of homogeneous text style
    auto process_segment = [&](std::size_t word_index_1, std::size_t offset_1,
                               std::size_t word_index_2, std::size_t offset_2,
                               std::size_t words_end_2) {
        ARCHON_ASSERT(word_index_1 <= word_index_2);
        ARCHON_ASSERT(word_index_2 <= words_end_2);
        ARCHON_ASSERT(word_index_1 < word_index_2 || offset_1 <= offset_2);
        // FIXME: Check and state that it can be proved, that offset relative to format unit
        // is always strictly less than size of unit, and that from this, it is possible to
        // prove that a segment is empty if, and only if (unit_index_1, offset_1) ==
        // (unit_index_2, offset_2).
        bool empty_segment = (word_index_1 == word_index_2 && offset_1 == offset_2);
        if (ARCHON_UNLIKELY(empty_segment))
            return;
        std::size_t output_begin = m_output_end;
        std::size_t word_index = word_index_1;
        const InputWord* word;
        const FormatUnit* unit;
        if (ARCHON_LIKELY(word_index < word_index_2)) {
            word = &m_input_words[word_index];
            unit = &get_format_unit(word_index);
            if (ARCHON_LIKELY(offset_1 >= unit->space_size)) {
                // Last part of first word
                std::size_t pos = std::size_t(word->chars_end - word->word_size);
                std::size_t begin = std::size_t(pos + (offset_1 - unit->space_size));
                std::size_t end   = std::size_t(pos + word->word_size);
                std::size_t size = std::size_t(end - begin);
                m_output_buffer.append({ input + begin, size }, m_output_end); // Throws
                goto transition_to_space;
            }
            // Last part of first space
            std::size_t size = std::size_t(unit->space_size - offset_1);
            m_output_buffer.append_a(m_space_char, m_output_end, size); // Throws
            goto word;
        }
        if (word_index < words_end_2) {
            unit = &get_format_unit(word_index);
            if (offset_2 > unit->space_size) {
                std::size_t offset = offset_1;
                if (offset < unit->space_size) {
                    // Emit last part of sole space
                    std::size_t size = std::size_t(unit->space_size - offset);
                    m_output_buffer.append_a(m_space_char, m_output_end, size); // Throws
                    offset = unit->space_size;
                }
                // Emit part of sole word
                word = &m_input_words[word_index];
                std::size_t pos = std::size_t(word->chars_end - word->word_size);
                std::size_t begin = std::size_t(pos + (offset   - unit->space_size));
                std::size_t end   = std::size_t(pos + (offset_2 - unit->space_size));
                std::size_t size = std::size_t(end - begin);
                m_output_buffer.append({ input + begin, size }, m_output_end); // Throws
                goto done;
            }
        }
        // Emit part of sole space
        {
            std::size_t size = std::size_t(offset_2 - offset_1);
            m_output_buffer.append_a(m_space_char, m_output_end, size); // Throws
        }
        goto done;

      space:
        {
            std::size_t size = unit->space_size;
            m_output_buffer.append_a(m_space_char, m_output_end, size); // Throws
        }

      word:
        {
            const char_type* data = input + (word->chars_end - word->word_size);
            std::size_t size = word->word_size;
            m_output_buffer.append({ data, size }, m_output_end); // Throws
        }

      transition_to_space:
        ++word_index;
        if (ARCHON_LIKELY(word_index < word_index_2)) {
            word = &m_input_words[word_index];
            unit = &get_format_unit(word_index);
            goto space;
        }
        if (word_index < words_end_2) {
            unit = &get_format_unit(word_index);
            if (offset_2 > unit->space_size) {
                // Full space of last word
                {
                    std::size_t size = unit->space_size;
                    m_output_buffer.append_a(m_space_char, m_output_end, size); // Throws
                }
                // First part of last word
                word = &m_input_words[word_index];
                {
                    const char_type* data = input + (word->chars_end - word->word_size);
                    std::size_t size = std::size_t(offset_2 - unit->space_size);
                    m_output_buffer.append({ data, size }, m_output_end); // Throws
                }
                goto done;
            }
        }
        // First part of last space
        {
            std::size_t size = offset_2;
            m_output_buffer.append_a(m_space_char, m_output_end, size); // Throws
        }

      done:
        Style style = input_style->style;
        bool has_fill_color = (m_format.has_fill_color && m_enable_ansi_escape_sequences);
        if (m_fill && has_fill_color && !style.has_background_color())
            style.set_background_color(m_format.fill_color);
        std::size_t offset = output_begin;
        std::size_t size   = std::size_t(m_output_end - output_begin);
        std::size_t cursor_pos = do_get_cursor_pos();
        bool has_overflow = (cursor_pos > m_outer_right || size > std::size_t(m_outer_right - cursor_pos));
        if (has_overflow) {
            if (cursor_pos < m_outer_right) {
                std::size_t size_2 = std::size_t(m_outer_right - cursor_pos);
                add_output_segment(style, offset, size_2); // Throws
                offset += size_2;
                size   -= size_2;
            }
            if (m_clipping)
                return;
            style = input_style->style;
        }
        add_output_segment(style, offset, size); // Throws
    };
    // Format a fragment produced by word-wrapping
    auto process_fragment = [&](std::size_t chars_begin_2, std::size_t chars_end_2,
                                std::size_t units_begin, std::size_t units_end,
                                std::size_t trailing_space_size_2, std::size_t inner_left,
                                std::size_t inner_size, bool justify) {
        std::size_t words_begin_2 = std::size_t(words_begin + units_begin);
        std::size_t words_end_2   = std::size_t(words_begin + units_end);
        // All fragments must have a nonzero footprint in the input buffer
        ARCHON_ASSERT(chars_begin_2 < chars_end_2);
        // All fragments must produce a nonzero amount of output
        std::size_t num_units_2 = std::size_t(units_end - units_begin);
        ARCHON_ASSERT(num_units_2 > 0 || trailing_space_size > 0);
        std::size_t max = std::size_t(-1);
        ARCHON_ASSERT(inner_size <= std::size_t(max - inner_left));
        if (ARCHON_UNLIKELY(m_clipping && inner_left >= m_outer_right))
            return;
        // Skip across input styles that have already ended
        while (input_style->chars_end <= chars_begin_2)
            ++input_style;
        ARCHON_ASSERT(input_style->word_index >= words_begin_2);
        std::size_t pos = inner_left;
        bool align = (m_align > 0);
        if (justify || align) {
            std::size_t fragment_size = trailing_space_size_2;
            for (std::size_t i = units_begin; i < units_end; ++i) {
                const FormatUnit& unit = m_format_units[i];
                fragment_size += unit.space_size + unit.word_size;
            }
            if (inner_size > fragment_size) {
                if (justify) {
                    std::size_t new_fragment_size = inner_size;
                    justify_fragment(units_begin, units_end, trailing_space_size_2,
                                     fragment_size, new_fragment_size);
                    fragment_size = new_fragment_size;
                }
                if (align) {
                    std::size_t excess = std::size_t(inner_size - fragment_size);
                    std::size_t shift = std::size_t(m_align * excess + 0.5);
                    ARCHON_ASSERT(shift <= std::size_t(max - pos));
                    pos += shift;
                }
            }
        }
        std::size_t cursor_pos = do_get_cursor_pos();
        ARCHON_ASSERT(pos >= cursor_pos);
        if (pos > cursor_pos) {
            Style style = m_fill_style;
            bool has_overflow = (pos > m_outer_right);
            if (has_overflow) {
                ARCHON_ASSERT(!m_clipping);
                if (cursor_pos < m_outer_right) {
                    std::size_t size = std::size_t(m_outer_right - cursor_pos);
                    add_space_segment(style, size); // Throws
                    cursor_pos = m_outer_right;
                }
                style = {};
            }
            std::size_t size = std::size_t(pos - cursor_pos);
            add_space_segment(style, size); // Throws
        }
        std::size_t word_index = words_begin_2;
        std::size_t offset = 0;
        while (input_style->chars_end < chars_end_2) {
            std::size_t word_index_2 = input_style->word_index;
            // Map input_style->chars_end to offset_2
            std::size_t space_begin = chars_begin;
            if (word_index_2 > words_begin) {
                const InputWord& prev_word = m_input_words[word_index_2 - 1];
                space_begin = prev_word.chars_end;
            }
            ARCHON_ASSERT(input_style->chars_end >= space_begin);
            std::size_t space_end, new_space_size;
            if (word_index_2 < words_end) {
                const InputWord& word = m_input_words[word_index_2];
                ARCHON_ASSERT(input_style->chars_end <= word.chars_end);
                space_end = std::size_t(word.chars_end - word.word_size);
                const FormatUnit& unit = get_format_unit(word_index_2);
                new_space_size = unit.space_size;
            }
            else {
                space_end = chars_end;
                ARCHON_ASSERT(input_style->chars_end <= space_end);
                new_space_size = trailing_space_size_2;
            }
            std::size_t space_size = std::size_t(space_end - space_begin);
            std::size_t offset_1 = std::size_t(input_style->chars_end - space_begin);
            std::size_t offset_2 = 0;
            if (offset_1 >= space_size) {
                offset_2 = std::size_t(new_space_size + (offset_1 - space_size));
            }
            else {
                offset_2 = std::size_t((double(offset_1) / space_size) * new_space_size + 0.5);
            }
            process_segment(word_index, offset, word_index_2, offset_2, words_end_2); // Throws
            ++input_style;
            word_index = word_index_2;
            offset     = offset_2;
        }
        process_segment(word_index, offset, words_end_2, trailing_space_size_2, words_end_2); // Throws
    };
    std::size_t inner_left_first;
    if (!m_output_line_is_open) {
        // Normal mode
        if (!m_paragraph_is_open) {
            inner_left_first = m_inner_left_first_1;
            m_paragraph_is_open = true;
        }
        else {
            inner_left_first = m_inner_left_first_2;
        }
        do_open_output_line();
        materialize_space(); // Throws
    }
    else {
        // Continuation mode
        inner_left_first = do_get_cursor_pos();
        bool separable = (inner_left_first <= m_inner_right &&
                          m_format.min_separation <= std::size_t(m_inner_right - inner_left_first));
        if (ARCHON_LIKELY(separable)) {
            inner_left_first += m_format.min_separation;
            if (m_format.adv_continuation) {
                // Advanced continuation
                if (inner_left_first <= m_inner_left_rest) {
                    inner_left_first = m_inner_left_rest;
                    goto go_on_1;
                }
                std::size_t displacement = std::size_t(inner_left_first - m_inner_left_rest);
                if (ARCHON_LIKELY(displacement <= m_format.max_displacement))
                    goto go_on_1;
                goto new_line;
            }
          go_on_1:
            std::size_t size = trailing_space_size;
            if (num_units > 0) {
                const FormatUnit& unit = m_format_units.front();
                size = std::size_t(unit.space_size + unit.word_size);
            }
            if (ARCHON_LIKELY(size <= std::size_t(m_inner_right - inner_left_first)))
                goto go_on_2;
        }
      new_line:
        // Break onto a new line
        do_close_output_line(); // Throws
        format_newline(); // Throws
        flush_output(); // Throws
        do_open_output_line();
        materialize_space(); // Throws
        inner_left_first = m_inner_left_rest;
    }
  go_on_2:
    std::size_t inner_size_first = 0, inner_size_rest = 0;
    if (ARCHON_LIKELY(inner_left_first <= m_inner_right))
        inner_size_first = std::size_t(m_inner_right - inner_left_first);
    if (ARCHON_LIKELY(m_inner_left_rest <= m_inner_right))
        inner_size_rest = std::size_t(m_inner_right - m_inner_left_rest);
    std::size_t inner_left = inner_left_first;
    std::size_t inner_size = inner_size_first;
    std::size_t chars_begin_2 = chars_begin;
    std::size_t units_begin = 0;
    bool word_wrap = (m_format.word_wrap && m_format.has_width);
    if (word_wrap) {
        word_wrap::Geometry geometry[] = {
            { inner_size_first, 1 },
            { inner_size_rest,  1 },
        };
        if (!m_knuth_wrapper) {
            // Fastest
            word_wrap::greedy(m_format_units, trailing_space_size, geometry, m_breakpoints); // Throws
        }
        else {
            // Highest quality
            m_knuth_wrapper->wrap(m_format_units, trailing_space_size, geometry, m_breakpoints); // Throws
        }
        for (std::size_t breakpoint : m_breakpoints) {
            std::size_t units_end = breakpoint;
            ARCHON_ASSERT(units_end > units_begin);
            std::size_t chars_end_2 = m_input_words[words_begin + (units_end - 1)].chars_end;
            std::size_t trailing_space_size_2 = 0;
            std::size_t num_units_2 = std::size_t(units_end - units_begin);
            bool justify = (m_justify && num_units_2 >= 2);
            process_fragment(chars_begin_2, chars_end_2, units_begin, units_end,
                             trailing_space_size_2, inner_left, inner_size, justify); // Throws
            do_close_output_line(); // Throws
            format_newline(); // Throws
            flush_output(); // Throws
            do_open_output_line();
            materialize_space(); // Throws
            inner_left = m_inner_left_rest;
            inner_size = inner_size_rest;
            m_format_units[units_end].space_size = 0;
            chars_begin_2 = chars_end_2;
            units_begin = units_end;
        }
    }
    std::size_t chars_end_2 = chars_end;
    std::size_t units_end = m_format_units.size();
    bool justify = false;
    process_fragment(chars_begin_2, chars_end_2, units_begin, units_end,
                     trailing_space_size, inner_left, inner_size, justify); // Throws
    m_paragraph_is_open = true;
}


template<class C, class T>
void BasicTextFormatter<C, T>::fill_and_close()
{
    if (!m_output_line_is_open) {
        m_paragraph_is_open = true;
        do_open_output_line();
        m_paragraph_is_open = false;
    }
    do_close_output_line(); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::format_newline()
{
    ARCHON_ASSERT(!m_output_line_is_open);
    std::size_t line_number = m_line_number;
    core::int_add(line_number, 1); // Throws
    ARCHON_ASSERT(line_number > m_line_number_base);
    std::size_t output_line_index = std::size_t(line_number - m_line_number_base);
    ARCHON_ASSERT(output_line_index <= m_output_lines.size());
    if (output_line_index == m_output_lines.size())
        add_output_line(); // Throws
    m_line_number = line_number;
}


template<class C, class T>
void BasicTextFormatter<C, T>::do_jump(std::size_t line_number)
{
    std::size_t min_line_number = m_line_number;
    if (!m_hold_stack.empty()) {
        HoldEntry& top = m_hold_stack.top();
        min_line_number = m_hold_stack.top().hold_line_number;
        if (m_line_number > top.reached_line_number)
            top.reached_line_number = m_line_number;
    }
    if (ARCHON_UNLIKELY(line_number < min_line_number))
        throw std::invalid_argument("Bad line number");
    m_paragraph_is_open = false;
    if (m_output_line_is_open) {
        do_close_output_line(); // Throws
        flush_output(); // Throws
    }
    std::size_t last_line_number = do_get_last_line_number();
    if (line_number <= last_line_number) {
        m_line_number = line_number;
        return;
    }
    std::size_t n = std::size_t(line_number - last_line_number);
    for (std::size_t i = 0; i < n; ++i) {
        format_newline(); // Throws
        flush_output(); // Throws
    }
}


template<class C, class T>
auto BasicTextFormatter<C, T>::prep_format_units(std::size_t chars_begin, std::size_t chars_end,
                                                 std::size_t words_begin, std::size_t words_end) ->
    std::size_t
{
    std::size_t n = std::size_t(words_end - words_begin);
    m_format_units.resize(n); // Throws
    std::size_t pos = chars_begin;
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t word_index = std::size_t(words_begin + i);
        const InputWord& word = m_input_words[word_index];
        std::size_t space_size = std::size_t(word.chars_end - word.word_size - pos);
        std::size_t word_size  = word.word_size;
        m_format_units[i] = { space_size, word_size };
        pos = word.chars_end;
    }
    std::size_t trailing_space_size = std::size_t(chars_end - pos);
    if (m_format.norm_whitespace) {
        if (n > 0) {
            m_format_units[0].space_size = 0;
            for (std::size_t i = 1; i < n; ++i)
                m_format_units[i].space_size = 1;
        }
        trailing_space_size = 0;
    }
    return trailing_space_size;
}


template<class C, class T>
void BasicTextFormatter<C, T>::justify_fragment(std::size_t units_begin, std::size_t units_end,
                                                std::size_t trailing_space_size,
                                                std::size_t old_size, std::size_t new_size) noexcept
{
    ARCHON_ASSERT(units_begin <= units_end);
    ARCHON_ASSERT(std::size_t(units_end - units_begin) >= 2);
    ARCHON_ASSERT(new_size > old_size);
    std::size_t stretch = std::size_t(new_size - old_size);
    FormatUnit& first_unit = m_format_units[units_begin];
    FormatUnit& last_unit  = m_format_units[units_end - 1];
    std::size_t first_left = first_unit.space_size;
    std::size_t last_left = std::size_t(old_size - trailing_space_size - last_unit.word_size);
    double anchor_1 = first_left + double(first_unit.word_size) / 2;
    double anchor_2 = last_left  + double(last_unit.word_size)  / 2;
    double origin = anchor_1;
    double dist   = anchor_2 - anchor_1;
    ARCHON_ASSERT(dist > 0);
    double factor = (dist + stretch) / dist;
    std::size_t prev_right = std::size_t(first_unit.space_size + first_unit.word_size);
    std::size_t prev_new_right = prev_right;
    for (std::size_t i = units_begin + 1; i < units_end - 1; ++i) {
        FormatUnit& unit = m_format_units[i];
        std::size_t left = std::size_t(prev_right + unit.space_size);
        double offset = origin - double(unit.word_size) / 2;
        double new_left_1 = offset + factor * (left - offset);
        std::size_t new_left_2 = std::size_t(std::floor(new_left_1 + m_round_frac));
        unit.space_size = std::size_t(new_left_2 - prev_new_right);
        ARCHON_ASSERT(unit.space_size >= 1);
        prev_right = std::size_t(left + unit.word_size);
        prev_new_right = std::size_t(new_left_2 + unit.word_size);
    }
    std::size_t last_new_left = std::size_t(last_left + stretch);
    last_unit.space_size = std::size_t(last_new_left - prev_new_right);
    ARCHON_ASSERT(last_unit.space_size >= 1);
    m_round_frac = std::fmod(m_round_frac + core::golden_fraction<double>(), 1.0);
}


template<class C, class T>
void BasicTextFormatter<C, T>::do_set_format(const FormatRep& format)
{
    verify_no_open_section(); // Throws
    m_format = format;
    on_format_changed();
}


template<class C, class T>
void BasicTextFormatter<C, T>::on_format_changed() noexcept
{
    const FormatRep& f = m_format;
    bool has_fill_color = (f.has_fill_color && m_enable_ansi_escape_sequences);
    std::size_t max = std::size_t(-1);
    std::size_t outer_right = max, inner_right = max;
    if (f.has_width) {
        if (ARCHON_LIKELY(f.padding_right <= f.width)) {
            std::size_t diff = std::size_t(f.width - f.padding_right);
            if (ARCHON_LIKELY(diff <= std::size_t(max - f.offset)))
                inner_right = std::size_t(f.offset + diff);
        }
        else {
            std::size_t diff = std::size_t(f.padding_right - f.width);
            if (diff <= f.offset) {
                inner_right = std::size_t(f.offset - diff);
            }
            else {
                inner_right = 0;
            }
        }
        if (ARCHON_LIKELY(f.width <= std::size_t(max - f.offset)))
            outer_right = std::size_t(f.offset + f.width);
    }
    std::size_t inner_left_first_1 = max;
    std::size_t inner_left_first_2 = max;
    std::size_t inner_left_rest    = max;
    if (ARCHON_LIKELY(f.padding_left <= std::size_t(max - f.offset))) {
        std::size_t sum = std::size_t(f.offset + f.padding_left);
        if (ARCHON_LIKELY(f.indent_first_1 <= std::size_t(max - sum)))
            inner_left_first_1 = std::size_t(sum + f.indent_first_1);
        if (ARCHON_LIKELY(f.indent_first_2 <= std::size_t(max - sum)))
            inner_left_first_2 = std::size_t(sum + f.indent_first_2);
        if (ARCHON_LIKELY(f.indent_rest <= std::size_t(max - sum)))
            inner_left_rest = std::size_t(sum + f.indent_rest);
    }
    m_clipping = (f.clipping && f.has_width);
    m_justify = (f.justify && f.has_width);
    m_fill = ((f.always_fill || has_fill_color) && f.has_width);
    m_inner_left_first_1 = inner_left_first_1;
    m_inner_left_first_2 = inner_left_first_2;
    m_inner_left_rest    = inner_left_rest;
    m_inner_right        = inner_right;
    m_outer_right        = outer_right;
    m_align = ((!f.has_width || f.align <= 0) ? 0 : (f.align >= 1 ? 1 : f.align));
    m_fill_style = {};
    if (has_fill_color && f.has_width)
        m_fill_style.set_background_color(f.fill_color);
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::verify_no_open_section()
{
    if (ARCHON_UNLIKELY(has_open_section()))
        throw std::logic_error("Input section is open");
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::do_get_last_line_number() noexcept -> std::size_t
{
    ARCHON_ASSERT(!m_output_lines.empty());
    ARCHON_ASSERT(std::size_t(-1) - m_line_number_base >= m_output_lines.size() - 1);
    return std::size_t(m_line_number_base + (m_output_lines.size() - 1));
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::do_get_cursor_pos() noexcept -> std::size_t
{
    ARCHON_ASSERT(m_output_line_is_open);
    OutputLine& line = get_output_line();
    return std::size_t(line.size - m_cursor_displacement);
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::add_output_line()
{
    std::size_t size = 0;
    std::size_t last_segment = std::size_t(-1);
    std::size_t unmaterialized_space = 0;
    m_output_lines.push_back({ size, last_segment, unmaterialized_space }); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::do_open_output_line() noexcept
{
    ARCHON_ASSERT(m_paragraph_is_open);                                                                                                                                                                                                                                                                                     
    ARCHON_ASSERT(!m_output_line_is_open);
    OutputLine& line = get_output_line();
    if (m_format.offset >= line.size) {
        ARCHON_ASSERT(m_cursor_displacement == 0);
        ARCHON_ASSERT(line.unmaterialized_space <= line.size);
        std::size_t padding = std::size_t(m_format.offset - line.size);
        line.size = m_format.offset;
        line.unmaterialized_space += padding;
    }
    else {
        m_cursor_displacement = std::size_t(line.size - m_format.offset);
    }
    m_output_line_is_open = true;
}


template<class C, class T>
void BasicTextFormatter<C, T>::do_close_output_line()
{
    ARCHON_ASSERT(m_output_line_is_open);
    if (m_format.has_width) {
        std::size_t cursor_pos = do_get_cursor_pos();
        if (cursor_pos < m_outer_right) {
            std::size_t padding = std::size_t(m_outer_right - cursor_pos);
            if (!m_fill) {
                OutputLine& line = get_output_line();
                core::int_add(line.size, padding); // Throws
                line.unmaterialized_space += padding;
            }
            else {
                materialize_space(); // Throws
                add_space_segment(m_fill_style, padding); // Throws
            }
        }
    }
    m_cursor_displacement = 0;
    m_output_line_is_open = false;
}


template<class C, class T>
void BasicTextFormatter<C, T>::materialize_space()
{
    OutputLine& line = get_output_line();
    if (line.unmaterialized_space > 0) {
        std::size_t space_size = line.unmaterialized_space;
        line.size -= space_size;
        line.unmaterialized_space = 0;
        Style style = {};
        add_space_segment(style, space_size); // Throws
    }
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::add_space_segment(Style style, std::size_t n)
{
    std::size_t output_begin = m_output_end;
    m_output_buffer.append_a(m_space_char, m_output_end, n); // Throws
    std::size_t offset = output_begin;
    std::size_t size   = std::size_t(m_output_end - output_begin);
    add_output_segment(style, offset, size); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::add_output_segment(Style style, std::size_t offset, std::size_t size)
{
    ARCHON_ASSERT(size > 0);
    ARCHON_ASSERT(m_output_line_is_open);
    OutputLine& line = get_output_line();
    ARCHON_ASSERT(line.unmaterialized_space == 0);
    std::size_t line_size = line.size;
    core::int_add(line_size, size); // Throws
    std::size_t segment_index = m_output_segments.size();
    std::size_t next = segment_index;
    m_output_segments.push_back({ style, offset, size, next }); // Throws
    line.size = line_size;
    if (line.last_segment != std::size_t(-1)) {
        std::size_t first = m_output_segments[line.last_segment].next;
        m_output_segments[line.last_segment].next = segment_index;
        m_output_segments[segment_index].next = first;
    }
    line.last_segment = segment_index;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::get_output_line() noexcept -> OutputLine&
{
    ARCHON_ASSERT(m_line_number >= m_line_number_base);
    std::size_t output_line_index = std::size_t(m_line_number - m_line_number_base);
    ARCHON_ASSERT(output_line_index < m_output_lines.size());
    return m_output_lines[output_line_index];
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::flush_output()
{
    if (m_hold_stack.empty())
        do_flush_output(); // Throws
}


template<class C, class T>
void BasicTextFormatter<C, T>::do_flush_output()
{
    ARCHON_ASSERT(m_hold_stack.empty());
    const char_type* output = m_output_buffer.data();
    std::size_t n = m_output_lines.size();
    for (std::size_t i = 0; i < n; ++i) {
        const OutputLine& line = m_output_lines[i];
        core::BufferContents output_line(m_output_line_buffer);
        if (line.last_segment != std::size_t(-1)) {
            Style style;
            auto change_style = [&](const Style& style_2) {
                constexpr std::size_t buffer_size = Style::min_change_buffer_size();
                char buffer_1[buffer_size];
                char_type buffer_2[buffer_size];
                std::size_t size = Style::change(style, style_2, buffer_1);
                m_char_mapper.widen(std::string_view(buffer_1, size), buffer_2); // Throws
                output_line.append(core::Span(buffer_2, size)); // Throws
                style = style_2;
            };
            const OutputSegment* last = &m_output_segments[line.last_segment];
            const OutputSegment* j = last;
            do {
                j = &m_output_segments[j->next];
                change_style(j->style); // Throws
                output_line.append(core::Span(output + j->offset, j->size)); // Throws
            }
            while (j != last);
            // Reset style before switching to a new line to avoid strange behaviour in some
            // terminals when the background color is set to a nondefault value.
            change_style({}); // Throws
        }
        bool last_line = (std::size_t(i + 1) == n);
        if (!last_line)
            output_line.append(1, m_newline_char); // Throws
        std::streamsize size = 0;
        core::int_cast(output_line.size(), size); // Throws
        m_final_out.write(output_line.data(), size); // Throws
    }
    ARCHON_ASSERT(m_line_number - m_line_number_base == n - 1);
    m_line_number_base = m_line_number;
    std::size_t last_size = m_output_lines.back().size;
    std::size_t last_unmaterialized_space = m_output_lines.back().unmaterialized_space;
    m_output_end = 0;
    m_output_lines.clear();
    m_output_segments.clear();
    add_output_line();
    m_output_lines.back().size = last_size;
    m_output_lines.back().unmaterialized_space = last_unmaterialized_space;
}


template<class C, class T>
auto BasicTextFormatter<C, T>::get_extended_section_info(std::size_t section_index) -> ExtendedSectionInfo
{
    std::size_t num_sections = m_input_sections.size();
    if (ARCHON_UNLIKELY(section_index >= num_sections))
        throw std::out_of_range("Section index");
    const InputSection& section = m_input_sections[section_index];
    std::size_t chars_begin = 0;
    std::size_t words_begin = 0;
    std::size_t lines_begin = 0;
    if (section_index > 0) {
        const InputSection& prev_section = m_input_sections[section_index - 1];
        chars_begin = prev_section.chars_end;
        words_begin = prev_section.words_end;
        lines_begin = prev_section.lines_end;
    }
    std::size_t size = section.section_size;
    std::size_t num_words = std::size_t(section.words_end - words_begin);
    std::size_t num_lines = std::size_t(section.lines_end - lines_begin);
    std::size_t last_line_size = size;
    if (num_lines > 0) {
        const InputLine& line = m_input_lines[section.lines_end - 1];
        last_line_size = std::size_t(section.chars_end - line.chars_end);
    }
    bool last_line_is_unterminated = (last_line_size > 0);
    if (last_line_is_unterminated)
        ++num_lines;
    SectionInfo base = { size, num_words, num_lines, last_line_is_unterminated };
    return { base, chars_begin, words_begin, lines_begin, last_line_size };
}


template<class C, class T>
auto BasicTextFormatter<C, T>::get_extended_line_info(std::size_t section_index, std::size_t line_index) ->
    ExtendedLineInfo
{
    ExtendedSectionInfo section_info = get_extended_section_info(section_index); // Throws
    if (ARCHON_UNLIKELY(line_index >= section_info.base.num_lines))
        throw std::out_of_range("Lines index");
    std::size_t line_index_2 = std::size_t(section_info.lines_begin + line_index);
    std::size_t chars_begin  = section_info.chars_begin;
    std::size_t words_begin  = section_info.words_begin;
    if (line_index > 0) {
        const InputLine& prev_line = m_input_lines[line_index_2 - 1];
        chars_begin = prev_line.chars_end;
        words_begin = prev_line.words_end;
    }
    std::size_t offset, size, num_words;
    bool is_unterminated_line = (section_info.base.last_line_is_unterminated &&                                                                                                                                              
                                 line_index == std::size_t(section_info.base.num_lines - 1));
    if (!is_unterminated_line) {
        // Terminated line
        const InputLine& line = m_input_lines[line_index_2];
        offset    = std::size_t(chars_begin - section_info.chars_begin);
        size      = line.line_size;
        num_words = std::size_t(line.words_end - words_begin);
    }
    else {
        // Unterminated line
        offset    = std::size_t(chars_begin - section_info.chars_begin);
        size      = section_info.last_line_size;
        num_words = std::size_t(section_info.words_begin + section_info.base.num_words - words_begin);
    }
    LineInfo base = { offset, size, num_words };
    return { base, chars_begin, words_begin };
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::has_width() const noexcept
{
    return m_rep.has_width;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_width() const noexcept -> std::size_t
{
    return m_rep.width;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_indent_first_1() const noexcept -> std::size_t
{
    return m_rep.indent_first_1;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_indent_first_2() const noexcept -> std::size_t
{
    return m_rep.indent_first_2;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_indent_rest() const noexcept -> std::size_t
{
    return m_rep.indent_rest;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_offset() const noexcept -> std::size_t
{
    return m_rep.offset;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_padding_left() const noexcept -> std::size_t
{
    return m_rep.padding_left;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_padding_right() const noexcept -> std::size_t
{
    return m_rep.padding_right;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::get_word_wrap() const noexcept
{
    return m_rep.word_wrap;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::get_justify() const noexcept
{
    return m_rep.justify;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::get_clipping() const noexcept
{
    return m_rep.clipping;
}


template<class C, class T>
inline double BasicTextFormatter<C, T>::Format::get_align() const noexcept
{
    return m_rep.align;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::has_fill_color() const noexcept
{
    return m_rep.has_fill_color;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_fill_color() const noexcept -> Color
{
    return m_rep.fill_color;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::get_always_fill() const noexcept
{
    return m_rep.always_fill;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::get_norm_whitespace() const noexcept
{
    return m_rep.norm_whitespace;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_min_separation() const noexcept -> std::size_t
{
    return m_rep.min_separation;
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Format::get_adv_continuation() const noexcept
{
    return m_rep.adv_continuation;
}


template<class C, class T>
inline auto BasicTextFormatter<C, T>::Format::get_max_displacement() const noexcept -> std::size_t
{
    return m_rep.max_displacement;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_width(std::size_t value) noexcept
{
    m_rep.has_width = true;
    m_rep.width = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::unset_width() noexcept
{
    m_rep.has_width = false;
    m_rep.width = 0;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_indent(std::size_t value) noexcept
{
    set_indent(value, value);
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_indent(std::size_t first, std::size_t rest) noexcept
{
    set_indent(first, first, rest);
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_indent(std::size_t first_1, std::size_t first_2,
                                                         std::size_t rest) noexcept
{
    m_rep.indent_first_1 = first_1;
    m_rep.indent_first_2 = first_2;
    m_rep.indent_rest    = rest;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_offset(std::size_t value) noexcept
{
    m_rep.offset = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_padding(std::size_t value) noexcept
{
    m_rep.padding_left  = value;
    m_rep.padding_right = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_padding_left(std::size_t value) noexcept
{
    m_rep.padding_left = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_padding_right(std::size_t value) noexcept
{
    m_rep.padding_right = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_word_wrap(bool value) noexcept
{
    m_rep.word_wrap = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_justify(bool value) noexcept
{
    m_rep.justify = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_clipping(bool value) noexcept
{
    m_rep.clipping = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_align(Align align) noexcept
{
    double value = 0;
    switch (align) {
        case Align::left:
            break;
        case Align::center:
            value = 0.5;
            break;
        case Align::right:
            value = 1;
            break;
    }
    set_align(value); // Throws
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_align(double value) noexcept
{
    m_rep.align = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_fill_color(Color value) noexcept
{
    m_rep.has_fill_color = true;
    m_rep.fill_color = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::unset_fill_color() noexcept
{
    m_rep.has_fill_color = false;
    m_rep.fill_color = {};
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_always_fill(bool value) noexcept
{
    m_rep.always_fill = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_norm_whitespace(bool value) noexcept
{
    m_rep.norm_whitespace = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_min_separation(std::size_t value) noexcept
{
    m_rep.min_separation = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_adv_continuation(bool value) noexcept
{
    m_rep.adv_continuation = value;
}


template<class C, class T>
inline void BasicTextFormatter<C, T>::Format::set_max_displacement(std::size_t value) noexcept
{
    m_rep.max_displacement = value;
}


template<class C, class T>
inline BasicTextFormatter<C, T>::Format::Format(const FormatRep& rep) noexcept
    : m_rep(rep)
{
}


template<class C, class T>
inline bool BasicTextFormatter<C, T>::Cursor::is_valid() const noexcept
{
    return ((unmaterialized_space <= line_size) &&
            (displacement  <= line_size) &&
            (output_line_is_open ? paragraph_is_open : displacement == 0));
}


} // namespace archon::core

#endif // ARCHON_X_CORE_X_TEXT_FORMATTER_HPP
