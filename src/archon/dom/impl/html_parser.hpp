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

#ifndef ARCHON_DOM_IMPL_HTML_PARSER_HPP
#define ARCHON_DOM_IMPL_HTML_PARSER_HPP

/// \file
///
/// \author Kristian Spangsege

#include <string>
#include <vector>
#include <locale>

#include <archon/core/logger.hpp>
#include <archon/core/stream.hpp>
#include <archon/core/utf16.hpp>


namespace archon {
namespace dom_impl {
namespace HtmlParser {

class Source;
class Callbacks;
class Attr;
class DocWriter;
class Resolver;



class Config {
public:
    bool treat_warnings_as_errors;
    bool die_on_first_error;
    bool case_insensitive;
    bool accept_xml_1_0_names;
    bool html5_compat;                  ///< Ignore start tag closure
    bool enable_meta_charenc_switching; ///< Ignored when parsing DTD
    bool report_comments;

    Config():
        treat_warnings_as_errors(false),
        die_on_first_error(false),
        case_insensitive(false),
        accept_xml_1_0_names(false),
        html5_compat(false),
        enable_meta_charenc_switching(false),
        report_comments(false)
    {
    }
};

/*
struct Config {
    bool allow_unknown_elems;
    // xxx unknown_elem_content_model;
};
*/



/// Parse the specified HTML file in roughly the same way as a SAX
/// parser would parse an XML file.
///
/// Reporting of scripts within synthetic input will be inhibited if
/// 32 such scripts were already reported without advancing the
/// position within the proper input. As soon as the true proper is
/// advanced, reporting of synthetic scripts will resume.
///
/// Is thread-safe.
///
/// \sa http://www.iana.org/assignments/character-sets
void parse_html(const Source&, Callbacks&, Resolver&,
                core::Logger* = &core::Logger::get_default_logger(),
                const Config& = Config());

void parse_dtd(const Source&, Callbacks&, Resolver&,
               core::Logger* = &core::Logger::get_default_logger(),
               const Config& = Config());

bool parse_xml_proc_instr(const core::StringUtf16& text, core::StringUtf16& xml_target,
                          core::StringUtf16& xml_data);


/*

Pass an optional character encoding.
If unspecified:
  Select a first guess
    Look at first few bytes (copy from eXpat)

Construct transcoder to UTF-16

Look for META. If found and not same as selected, ask Input to change its transcoder. This involves:
  Reverse transcoding any buffered data that is already transcoded.
  

Input:
  get_char:
    if empty utf-16 buf:
      if source buf is empty:
        read more from source:
      (*transcoder)(source_soft_begin, source_soft_end, utf_16_hard_begin)
    return char

If I run out of transcoded data while inside a META start tag, do not clear the preceeding block of "raw" source data.

Raw data for uninterpreted UTF-16 data must always be kept until the application explicitely tells us that we do not have to do it anymore.


Keep one extra buffer to prevent allocation hysteresis.

At '>' of start tag seen:
  in.


*/


class Source {
public:
    Source(core::InputStream& i): in(i) {}

    core::InputStream& in;

    /// The character encoding used in the input. Empty means
    /// 'auto-detect'. For files received over HTTP, the caller should
    /// pass the character encoding information from the
    /// 'Content-Type' HTTP header.
    ///
    /// Character encodings are specified using the names registered
    /// by IANA. Please see the IANA registry for the complete
    /// list. Not all encodings may be available on any specific
    /// platform. The available encodings are precisely those offered
    /// by <archon/util/transcode.hpp>.
    core::StringUtf16 charenc;

    /// A string that identifies the input. Defaults to "<input>",
    /// litteraly. This should in general be the URI reference from
    /// which the input is retreived.
    core::StringUtf16 system_ident;

    /// The base URI to resolve against whan requesting sub components
    /// of the input. Empty means 'the current working directory'. The
    /// base URI should in general be equal to 'system_ident'.
    core::StringUtf16 base_uri;
};



/// All callback functions are no-op's by default.
class Callbacks {
public:
    typedef core::BasicInputStream<core::CharUtf16> InlineStream;

    virtual void doctype_begin(const core::StringUtf16& name,
                               const core::StringUtf16& public_id,
                               const core::StringUtf16& system_id);

    virtual void doctype_end();

    /// All elements will be explicitely closed by a call to
    /// element_end(). Likewise, element_end() can never be called
    /// without a previous corresponding call to
    /// element_begin(). These rules apply independently of whether
    /// the source document contains start and/or end tags.
    ///
    /// Attributes are reported in the order they occur on start tag
    /// in the source document.
    virtual void elem_begin(const core::StringUtf16& name, const std::vector<Attr>&);

    virtual void elem_end(const core::StringUtf16& name);

    /// Called whenever a script tag is encountered.
    virtual void script(const std::vector<Attr>&, InlineStream& inline_script,
                        DocWriter&);

    virtual void style(const std::vector<Attr>&, InlineStream& inline_style);

    /// Continuous text may be broken arbitrarily and reported in
    /// multiple chunks. A reported chunk is never empty.
    virtual void text(const core::StringUtf16& chunk);

    /// Called for each SGML comment declaration. For comment
    /// declarations with multiple comments, only the first comment
    /// within the declaration is included in the text that is passed
    /// as argument. This method will not be called for degenerate
    /// comment declarations containing zero comments.
    virtual void comment(const core::StringUtf16& text);

    virtual void proc_instr(const core::StringUtf16& text);

    virtual ~Callbacks() {}
};



class Attr {
public:
    core::StringUtf16 name, value;
};



class DocWriter {
public:
    /// Inject the specified arbitrary text immediately after the
    /// closing script tag of the currently executing script.
    virtual void write(const core::StringUtf16& text) = 0;

    virtual ~DocWriter() {}
};



class ResolveException: public std::runtime_error {
public:
    ResolveException(const std::string& m): std::runtime_error(m) {}
};


class Resolver {
public:
    /// \throw ResolveException if the specified resource is
    /// unavialble.
    virtual void resolve(const core::StringUtf16& public_ident,
                         const core::StringUtf16& system_ident,
                         const core::StringUtf16& base_uri,
                         core::UniquePtr<core::InputStream>& in,
                         core::StringUtf16& charenc,
                         core::StringUtf16& uri) = 0;

    virtual ~Resolver() {}
};


class DefaultResolver: public Resolver {
public:
    DefaultResolver(const std::locale& l = std::locale()): loc(l) {}

    virtual void resolve(const core::StringUtf16& public_ident,
                         const core::StringUtf16& system_ident,
                         const core::StringUtf16& base_uri,
                         core::UniquePtr<core::InputStream>& in,
                         core::StringUtf16& charenc,
                         core::StringUtf16& uri);

private:
    const std::locale loc;
};


} // namespace HtmlParser
} // namespace dom_impl
} // namespace archon

#endif // ARCHON_DOM_IMPL_HTML_PARSER_HPP
