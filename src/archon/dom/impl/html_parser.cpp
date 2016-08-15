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

/// \file
///
/// \author Kristian Spangsege

#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <map>

#include <archon/core/assert.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/file.hpp>
#include <archon/core/text.hpp>
#include <archon/core/param_expand.hpp>
#include <archon/core/memory.hpp>
#include <archon/util/transcode.hpp>
#include <archon/util/uri.hpp>
#include <archon/dom/impl/util.hpp>
#include <archon/dom/impl/html_parser.hpp>


using namespace archon::core;
using namespace archon::util;


// REFERENCES:
//
// http://www.is-thought.co.uk/book/home.htm  (Web SGML and HTML 4.0 Explained)
//
// http://www.flightlab.com/~joe/sgml/ (Notes on SGML and HTML)
//
// http://crism.maden.org/consulting/pub/sgmldefs.html (Productions from ISO 8879:1986 - SGML)
//
// http://xml.coverpages.org/sgmlsyn/contents.htm (SGML Syntax Summary Table of Contents)
//
// http://www.w3.org/TR/NOTE-sgml-xml-971215 (Comparison of SGML and XML) (good)
//
// http://www.sgmlsource.com/8879/n0029.htm (Web SGML Adaptations Annex to ISO 8879)
//
// http://www.w3.org/MarkUp/SGML/productions.html (SGML Productions)
//
// The SGML FAQ Book




/*

New strategy for input:

Introduce a new class StackedInput with basic fast character retrieval function (get(), unget(), peek(), drop()).

StackedInput maintains a stack of abstract sources StackedInput::Source.
Sources return entire slabs of data to StackedInput through a virtual function Source::get_data().

Completely eradicate the complicated notion of artificial data from old Input.

Handle both internal and external entity references by pushing a new input onto StackedInput.

Handling of document.write():
  If script occurs in entity replacement text (DtdParser::entity_repl_level > 0):
    Do not pass doc-writer input stream, to indicate that document.write() is impossible.
  Else:
    Push a new source onto StackedInput
    Mark it as 'seamless' which means that character extraction continues silently across the end of the inserted chunk.

What does HTML5 say about document.write()? http://www.w3.org/html/wg/drafts/html/master/webappapis.html#document.write%28%29

*/


                                                                         
// Continue developing skip_param_sep().
// Next task is to eliminate all uses of Input::inject()!!!



/// FIXME: Consider BufferEmptyListener
///
/// FIXME: Consider how to deal with InputLoc. Utilize the fact that
/// location can be projected accurately and efficiently by counting
/// newlines within large chunks.
///
template<class Char> class StackedInput {
public:
    /// Returns true if, and only if the end of the stream has been
    /// reached. In that case, it will continue to return false for as
    /// long aas the current source remains in place.
    bool eoi()
    {
        return m_src.m_next == m_src.m_end && !next_chunk();
    }

    /// Returns, but does not extract the next input characters. If
    /// there is no next input character (if eoi() would have returned
    /// true), this function returns the null character. Note that it
    /// also returns null characters when they occur in the input
    /// stream.
    Char peek()
    {
        return eoi() ? Char() : *m_src.m_next;
    }

    /// Drop the next input character.
    ///
    /// DANGEROUS: May be called only in cases where eoi() would
    /// return false. The consequences of violating this rule are
    /// undefined (memory corruption is likely). The intention is that
    /// this function is only called after a call to peek() that did
    /// not return the null character, or after a call to unget().
    void drop()
    {
        ARCHON_ASSERT(!eoi());
        ++m_src.m_next;
    }

    /// Extract the next input character, if any. If there is no next
    /// input character (if eoi() would have returned true), this
    /// function returns false, otherwise it returns true and assigns
    /// the next character to the specified character object.
    bool get(Char& ch)
    {
        if (eoi())
            return false;
        ch = *m_src.m_next++;
        return true;
    }

    // Pretend that the latest extracted character was not extracted.
    //
    // DANGEROUS: Must follow a successfull call to get() or a call to
    // drop(), and there must no in-between calls to unget() or other
    // Input functions. The consequences of violating these rules are
    // undefined (memory corruption is likely).
    void unget()
    {
        ARCHON_ASSERT(m_src.m_next != m_src.m_begin);
        --m_src.m_next;
    }


    /// Read characters from the input stream until a delimiter is
    /// reached.
    ///
    /// The extracted characters are stored in the specified buffer, and
    /// the number of extracted characters is returned. Reading stops
    /// early if the buffer fills up, or if the end of input is
    /// reached.
    std::size_t read_until(Char *b, std::size_t n, Char delim)
    {
        Char* b_0 = b;
        while (n > 0 && !eoi()) {
            std::size_t m = std::min<std::size_t>(n, m_src.m_end - m_src.m_next);
            CharUtf16* p = std::find(m_src.m_next, m_src.m_next + m, delim);
            b = std::copy(m_src.m_next, p, b);
            m_src.m_next = p;
            if (p != m_src.m_end)
                break;
            n -= m;
        }
        return b - b_0;
    }

    struct Chunk {
        Char* m_begin;
        Char* m_end;
        Chunk(Char* begin, Char* end):
            m_begin(begin),
            m_end(end)
        {
        }
    };

    struct Source {
        /// Must return false if the end of input is
        /// reached. Otherwise it must return true after updating the
        /// specified chunk object to refer to a new non-empty chunk
        /// of input.
        ///
        /// Implementations must be able to handle multiple
        /// invocations at end of input. After an instance of Source
        /// has returned false, it must continue to return false for
        /// ever.
        virtual bool next_chunk(Chunk&) = 0;

        virtual ~Source() {}
    };

    void push_source(Source* src, bool take_ownership, bool seamless)
    {
        Chunk empty_chunk(0,0);
        SourceEntry entry(empty_chunk, src, take_ownership, seamless);
        m_stack.push_back(entry);
    }

    void push_source(Chunk chunk, bool take_ownership, bool seamless)
    {
        Source* src = &s_static_source;
        SourceEntry entry(chunk, src, take_ownership, seamless);
        m_stack.push_back(entry);
    }

    /// Pop the current non-root source off the stack. It is an error
    /// to call this function when the current source is the root
    /// source. Doing so results in undefined behavior (memory
    /// corruption is likely).
    void pop_source()
    {
        TIGHTDB_ASSERT(!m_stack.empty());
        if (m_src.m_is_owned) {
            if (m_src.m_source == &s_static_source) {
                delete[] m_src.m_begin;
            }
            else {
                delete m_src.m_source;
            }
        }
        m_src = m_stack.back();
        m_stack.pop();
    }

private:
    struct SourceEntry: Chunk {
        Char* m_next;
        Source* m_source;
        bool m_is_owned;
        bool m_is_seamless;
        SourceEntry(Chunk chunk, Source* source, bool is_owned, bool is_seamless):
            Chunk(chunk),
            m_next(chunk.m_begin),
            m_source(source),
            m_is_owned(is_owned),
            m_is_seamless(is_seamless)
        {
        }
    };

    SourceEntry m_src;
    std::vector<SourceEntry> m_stack;

    bool next_chunk()
    {
        if (m_src.m_source->next_chunk(m_src)) {
            m_src.m_next = m_src.m_begin;
            return true;
        }
        if (!m_src.m_is_seamless || m_stack.empty())
            return false;
        pop_source();
        return !eoi();
    }

    struct StaticSource: Source {
        bool next_chunk(Chunk&) ARCHON_OVERRIDE
        {
            return false;
        }
    };

    static StaticSource s_static_source;
};





class TranscodingSource: public StackedInput<CharUtf16>::Source {
public:
    /// Allows the consumer to process buffered UTF-16 input before
    /// more data is requested from the raw input stream. This may be
    /// appropriate when reading from a network socket, or from a
    /// terminal where a read request may block for a significant
    /// amount of time.
    class BufferEmptyListener {
    public:
        virtual void on_input_buffer_empty() = 0;

        virtual ~BufferEmptyListener() {}
    };

    TranscodingSource(UniquePtr<InputStream>& stream, const StringUtf16& charenc, BufferEmptyListener* = 0);

    bool next_chunk(StackedInput<CharUtf16>::Chunk& chunk) ARCHON_OVERRIDE
    {
        static_cast<void>(chunk);
        throw std::runtime_error("Not yet implemented");
    }
};

//class Input {
//public:






// TODO:
//
// - The parser parses one SGML document entity. An "SGML document",
//   on the other hand, may consist of multiple entities separated in
//   a system dependent way, but that is irrelevant to this parser.
//
// - Impose a maximum on the number of times a script can be executed
//   without advancing the unfaked input position (32). Gecko says
//   20. WebKit say 21. Presto says 128.
//
// - Parse DTD.
//
// - Use DTD to understand where elements end, and to generate
//   elements where the start tag is omitted.
//
// - Inhibit charenc switching unless original encoding is on a
//   stateless white list, and we may also need to require that the
//   new encoding is on that white list.
//
// - Allow for no transcoder, when byte input is UTF-16 already, and
//   with the right byte order. Probably not so important.
//



/*

Open questions
--------------

- Check up on requirements for closing of tags/elements in general entity replacement texts?

- Will marked sections introduce omitted end tags?



Support enabling HTML compatibility quirks (HTML5)
--------------------------------------------------

- Ignore trailing solidus in start tags `<br/>` or `<br />`.

- Other soliduses such as these `<br/ >` and `<br / id="1">` are
  errors, but will be ingnored and not exit the tag context. Other
  non-name characters are errors, but will still form attribute
  names. (CAN WORK FOR SGML TOO)

- Disable support of internal subset. Error if `>` is not found where
  the internal subset could occur in SGML in the DOCTYPE
  declaration. Then skip until next `>` at level 0. Level up on `<`,
  and down on `>`.

- Consider all marked sections as CDATA (but discard the contents
  outside MathML and SVG contexts). Only marked sections on the form
  <![CDATA[...]]> (exactly) inside MathML or SVG are valid.

- Report restricted form of CDATA marked sections in MathML and SVG
  contexts.


Support XML mode
----------------

- Error if document structures (probably elements and CDATA sections)
  do not start and end within the same entity. (SGL / FEATURES / OTHER
  / ENTITIES / INTEGRAL (YES)NO)

- Error if replacement text of general entity referenced from attr
  value has tags, PIs, or declarations.

- Enable reporting of entity declarations and references
  (entity_decl(), entity_begin(), entity_end(), entity_ref())

- Report restricted form of CDATA marked sections.



Start of document
-----------------

<!SGML ... >

Prolog consists of:

<? up to  >
<!>
<!-- ... --  (even number of --)  >
<!X ... >        (where X is a valid first char of name, but the keyword is unknown, or not allowed here. everything is skipped up to first '>').
<!DOCTYPE ... >  (up to first `>` upon any failure)
Any of the above
<!LINKTYPE ... > (up to first `>` upon any failure)
Any of the above except <!DOCTYPE

Any character, that is not white space, and not any of the above, ends
the prolog. If it is not a start tag of the document element, and that
tag is optional, it must be injected at this point. Otherwise there is
a parse error (inject it anyway).

Data following the explicit end tag of the document element is also a
kind of prolog. Here non-white-space characters are not allowed unless
they are part of a comment declaration or a processing instruction.

When attribute names are omitted, the values must 



Parse errors inside DTD
-----------------------

In case of syntax error (except unmatched `]]>`)
1) report the error
2) skip characters until first of: end-of-input, end-of-entity-replacement-text (Ee), `<!>`, `<![`, `<!--`, `<!x`, `<?`, `]]>`

(How does this compare to PCDATA in the base document element (outside prolog)?)    

This does not apply to semantic errors such as:
- References to undefined parameter entities.
- References to undefined character functions as in `&#foo`.
- Use of keywords that are unavailable due to non-syntactic context,
  such as when trying to use CDATA or RCDATA in a parameter entity
  declaration.

Any unclosed marked section in an external parsed DTD entity is
forcefully closed at the end of the entity, and an error is reported.

Any unclosed non-INCLUDE marked section, or unclosed element with CDATA or RCDATA contents in a parsed internal entity is
forcefully closed at the end of the entity, and an error is reported
(entity end in character data, replaceable character data or ignored
marked section).



Parse errors outside DTD
------------------------

A parse error in the keywords part of a marked section causes an error to be reported, and the characters immediately following the point of the error (following an invalid character, or an invalid keyword (complete word)) are interpreted as general top-level characters.



Expansion of parameter entities
-------------------------------

<!ENTITY % x "<!-- &#37;y; -->">  `<!-- %y; -->`
<!ENTITY % y "[&#37;z;]">         `[%z;]`
<!ENTITY % z "(Z)">               `(Z)`
<!ENTITY foo "{%x;}">             `{<!-- [(Z)] -->}`

Shown replacement texts are the same in OpenSP, Gnome XML, and Java DOM.

The `%y;` in the replacement text of `x` will not be recognized as a
reference when `x` is expanded in DTD context (i.e., in a place where
a complete declaration could begin).

When a parameter entity reference is recognized by the parser, the
replacement text for that entity is inserted at the current position
in the input stream, such that the replacement text is parsed in the
context of the reference. This is true regardless of where the
parameter entity reference occurs.

XML requires that any occurance of `%` in a context where a parameter
entity reference can be recognized, is immediately and literally
followed by (without any further entity or character reference
expansion) the rest of a valid parameter entity reference, up to and
including the final `;`.

When parsing the inserted replacement text in the context of a literal
value of an internal entity, no parameter entity reference can
continue beyond the end of the replacement text. Java DOM seems to
allow it, as long as the name itself is not split, but this is
probably an error in that implementation. Neither OpenSP nor Gnome XML
parser allows it.

Expand a parameter entity reference by pushing a begin,end pair onto a
stack of overriding artificial input chunks in the associated Input
object. The Input object must report end-of-input when the end of the
chunk is reached, and the chunk must then be manually popped off the
stack before the rest of the underlying input becomes available again.


Contexts:

- Outside declarations in declaration subset (DTD):

  - Ensure that marked sections are both started and ended at top
    level, or in the same replacement text.

  - Ensure that internal subset is not ended inside replacement text.

  If end-of-input:
    If entity_repl_level > 0:
      If inside a marked section, and the immediately enclosing marked section was started in the current replacement text (marked_section.entity_repl_level == entity_repl_level):
        Report an error
      Pop off the top chunk and continue

  If internal-subset-end token `]`:
    If entity_repl_level > 0:
      Report an error

  If marked-section-end token `]]>`:
    If immediately enclosing marked section was not started in current replacement text (marked_section.entity_repl_level < entity_repl_level):
      Report an error


- In parameter separator:

  - Ensure that declaration (or marked section keywords section) is
    both started and ended at top level, or in the same replacement
    text.

  If end-of-input:
    If entity_repl_level > 0:
      If current decl (or marked section keywords section) was started in the current replacement text (decl_entity_repl_level == entity_repl_level):
        Report an error
      Pop off the top chunk and continue

  If decl-end token `>` (or `[` in case of marked section keywords section):
    If current decl (or marked section keywords section) was not started in the current replacement text (decl_entity_repl_level < entity_repl_level):
      Report an error


- In parameter entity literal:

  - Ensure that a quotation mark that matches the opening mark, closes
    the literal if, and only if it occurs in the same replacement text
    as the opening mark.

  If end-of-input:
    If entity_repl_level > 0:
      If literal was started in the current replacement text (lit_entity_repl_level == entity_repl_level):
        Report an error
      Pop off the top chunk and continue

  If quotation mark that matches opening mark:
    If literal was not started in the current replacement text (lit_entity_repl_level < entity_repl_level):
      Add a literal quotation mark to the replacement text and continue


                                                                                                                     
Need stack of overriding input chunks in Input.



ALL THE FOLLOWING NEEDS TO BE REVISED WITH RESPECT TO THE NEW CLARITY EXPRESSED ABOVE!



XML requires the following:

- In literal values of internal parameter and general entities, the
  first occurance of `%` and `&` must form a valid entity reference,
  even if the entity is unreferenced. (Well-formedness constraint, Fatal error)

- http://www.w3.org/TR/2008/REC-xml-20081126/#vc-entdeclared


- When a `%` occurs literally in an entity value, it must be followed
  immediately by a syntactically valid name and then a `;` (without
  further text replacement).
- The replacement text, if any, is then injected into the input stream
  at the current position (such that the next input character is the
  first one in the replacement value), and parsing resumes.

This disagrees with `onsgmls` which allows loose `%` characters as
well as references without a `;`, and which does not rescan parameter
enetity replacement text. It is unclear to me what the general rule is
for SGML.

HMM, the above conclusions was apparently wrong. The following DTD
snippet produces the same replacement value for `foo` in XML and
`onsgmls`:

                              What I thought onsgmls did
---------------------------------------------------------
<!ENTITY % x "[&#37;y;]">     x="%y;"
<!ENTITY % y "(Y)">           y="(Y)"
<!ENTITY   z "{%x;}">         z="{%y;}" (wrong, resscanning does take place)

onsgmls:
  Ix TEXT [%dud;]
  Idud TEXT (DUD)
  Ifoo TEXT {[(DUD)]}

So, apparently `onsgmls` rescans as much as XML requires. It seems the
only difference is that SGML accepts forms of input that XML rejects.

So the same amount of rescanning takes place, apparently.

Note: XML does not allow for the name in a parameter entity reference
to extend beyond the end of the replacement text where it was started.

<!ENTITY % x1 "%">
<!ENTITY % x2 "%x1;f">
<!ENTITY % x3 "%x2;o">
<!ENTITY % x4 "%x3;o">
<!ENTITY % x5 "%x4;;">
<!ENTITY   x "{%x5;}">

Ix1 TEXT %
Ix2 TEXT %f    (AHA, ONSGMLS RESCANS REPLACEMENT TEXT, BUT DOES NOT ALLOW A TOKEN TO EXTEND BEYOND THE END OF IT)
Ix3 TEXT o     (THIS TIME, `%f` IS RECOGNIZED AS A REFERENCE DURING RESCANNING AND IS SUBSTITUDED WITH THE EMPTY STRING)
Ix4 TEXT oo
Ix5 TEXT oo;
Ix TEXT {oo;}

WHY THEN DOES THE RECURSIVE THING WORK IN ONSGMLS? INVESTIGATE THIS THOROUGHLY!

THE FOLLOWING REASONINGS HAVE TO BE REVISED




Store all parameter entities with an extra space appended (unconditionally).

In literal entity values:
- Just add the text to `text_accum` excluding the extra final space. (not compatible with XML)

In parameter separator context:
- Push a string reference or a begin,end pointer pair onto a stack of
  overriding inputs in the associated Input object. In this case the
  extra final space must be included. This ensures that no token will
  ever cross the boundaries of a replacement text.
- When pushing the overriding input, record the new nesting level. Then if the declaration termination token occurs, check 


- Consider complicated cases such as comment declarations and `]]>`.

- Multi layered parameter expansion can occur, that is, the result of
  parameter expansion may get expanded in a context where the result
  is rescanned for new parameter references, and this can repeat
  itself any number of times. It turns out that it is even possible to
  trigger a recursive parameter expansion, although it is somewhat
  more tricky than with general entity references. [ <!E % p "%"> <!E % a "%p;b"> <!E % b "%p;a"> %a ] (CAN BE SIMPLIFIED)

 it is even possible  In contrast to general entity references,  contain new parameter enetity references, is sometimes rescanned for new parameter entity references (e.g. in parameter separator context).  () in the
  keywords part of a marked section expanded from a general entity
  reference in the context of the base document element. How nasty?
  Hey, that could occur even more easily by expanding a parameter
  entity at top level or in a declaration, with a replacement text
  constructed to form a new parameter entity reference. It even seems that ther eis no limit to the number of layered levels of expansion are possible.


     
Fix flushable_text. Probably rename to text_accum, and have some sort of flag to temporarily disable flushing, such as when parsing tags, PIs, and comment declarations.

     
Use ] as Ee in top-level and MS context, and > in declaration context, and " in string context.

In the prolog, parameter entity references are only recognized and replaced when they occur inside declarations.

Inside the base document element, parameter entity references are recognized and expanded only inside the 'keywords' part of a marked section, and in `<!USEMAP` and `<!USELINK` declarations.

Parameter entity references must be handled by injecting the replacement text into the input stream.

A declaration that is not ended inside the replacement value of parameter entity, referenced from the DTD top level is simply discarded. Other preceeding properly ended declarations in the replacement text will still take effect. This must be detected at the time the first character after the replacement test is requested from the input stream, by detecting that parsing of a declaration is in progress.

When a parameter entity is referenced outside a literal entity value, it is an error if the replacement text ends inside a literal. What happens in lenient mode if this occurs?

Constraints by expansion context:

- In DTD where a declaration can occur (pcdata_dtd):
  - Error if a declaration is started inside the replacement text, but
    extends beyond the end of it. This also applies to complete marked
    section declarations, except when it is an INCLUDE or TEMP
    section. In that case the requirement is only that the opening
    part does not extend beyond the end. The opening part is
    terminated by the `[` token, that follows the keywords. For an
    external entity, all marked sections must be fully contained
    inside the replacement text.
  - XML further requires that INCLUDE sections are fully contained
    inside the replacement text.

- In parameter separator inside declarations (ps_decl):
  - Error if `PI` entity.
  - Error if still in replacement text when `>` is seen.
  - Error if contained comment is not ended in replacement text.
  - Error if contained string literal is not ended in replacement text.

- In parameter separator inside the keywords part of a marked section
  (ps_ms_keywords):
  - Error if `PI` entity.
  - Error if still in replacement text when `[` is seen.
  - Error if contained comment is not ended in replacement text.
  - Error if contained string literal is not ended in replacement text.
  - Error if contained group delimiters are unbalanced.
  - XML allows *only* INCLUDE and IGNORE sections in the DTD, but
    allows none in the internal subset. If further requires that there
    is exactly one keyword after expansion of parameter entity
    references.

- Replaceable parameter data (in literal entity values) (entity_lit):
  - Error if `PI` entity.
  - A quotation character in the replacement text that matches the
    opening quotation character must be handled as any other
    character. This is best handled by simply adding the replacement
    text to `text_accum`. (not compatible with XML)


Expansion of general entity references
--------------------------------------

Just add replacement text to `text_accum` unless 'to-be-parsed' flag
is set.

Must detect and report cyclic reference paths.

Constraints by expansion context:

- In PCDATA (pcdata):
  - Error if a declaration is started inside the replacement text, but
    extends beyond the end of it. This also applies to complete marked
    section declarations, except when it is an INCLUDE or TEMP
    section. In that case the requirement is only that the opening
    part does not extend beyond the end. The opening part is
    terminated by the `[` token, that follows the keywords. For an
    external entity, all marked sections must be fully contained
    inside the replacement text. (can a regenral entity be external
    and parsed?)
  - Error if contained CDATA or RCDATA element is not ended inside
    replacement text.

- In quoted attribute values (rcdata_attr):
  - Error if `PI` entity.
  - Quotation marks in the replacement text that matches the opening
    mark, do not end the quotation.
  - Procedure:
    - When a general entity reference ER is detected, lookup the
      entity definition ED, then call a recursive function expand()
      that scans for new general entity references if 'to-be-parsed'
      flag is set.
    - Add the result to `text_accum`.
    - Cache (NO, NOT ALLOWED, MUST BE ABLE TO TERMINATE TOKEN PARSING,
      IN MOST CASES AT LEAST) the result of the recursive expansion of
      the replacement text in the entity definition. This can be
      reused in other RCDATA contexts, including attribute values.

- In RCDATA contents of an element (rcdata_elem):
  - Error if `PI` entity.
  - Occurances of `</X` in the replacement text, do not end the
    element.
  - The procedure is the same as for rcdata_attr.

- In RCDATA contents of a marked section (rcdata_ms):
  - Error if `PI` entity.
  - Occurances of `]]>` in the replacement text, do not end the section.
  - The procedure is the same as for rcdata_attr.

NO NAME TOKEN (OR ANY OTHER TOKEN) CAN EXTEND BEYOND THE BOUNDARIES OF THE REPLACEMENT TEXT FOR A GENERAL ENTITY REFERENCE.

EVERY NESTED END OF A GENERAL ENTITY REPLACEMENT TEXT MUST END THE CURRENT TOKEN. SO NO CACHING IS ALLOWED. (XML WANTS TO ADD BRACKETING SPACES IN SOME CASES, COULD THIS HELP?)

Two modes for handling general entities:

  - Silently store entity replacement texts, and parse them in context
    for each entity reference (SGML mode).
    - No parsing is needed though, if the value is declared as CDATA.
    - The result of expansion in RCDATA context can be cached and
      reused in all other RCDATA contexts.
    - Another option would be to tokenize the replacment texts on
      demand, and in such a way that in RCDATA context, the tags,
      declarations, and PIs can simply be skipped across in the token
      sequence, and instead remembering the position in an underlying
      buffer of the end of the preceeding entity reference.

  - Immediately parse replacement texts as PCDATA calling
    entity_decl_begin() and entity_decl_end(), then call entity_ref()
    for each reference (XML mode). There are three cases:
    - Parse error, maybe due to unbalanced tags.
    - The replacement text consists of text children and entity
      reference nodes only, in which case it can be expanded in
      attribute values.
    - The replacement text has other nodes, and can therefore never be
      expanded in attribute values, since any `<` character would
      violate XML well-formedness.
    FIXME: According to the XML specification, unreferenced general
    entities do not have to be well-formed.


*/

namespace {

using namespace archon::dom_impl;
using namespace HtmlParser;


typedef std::char_traits<CharUtf16> TraitsU16;


const CharUtf16 char_Greater        = TraitsU16::to_char_type(0x3E); // >
const CharUtf16 char_Less           = TraitsU16::to_char_type(0x3C); // <
const CharUtf16 char_Solidus        = TraitsU16::to_char_type(0x2F); // /
const CharUtf16 char_Exclamation    = TraitsU16::to_char_type(0x21); // !
const CharUtf16 char_Question       = TraitsU16::to_char_type(0x3F); // ?
const CharUtf16 char_Hyphen         = TraitsU16::to_char_type(0x2D); // -
const CharUtf16 char_FullStop       = TraitsU16::to_char_type(0x2E); // .
const CharUtf16 char_Equals         = TraitsU16::to_char_type(0x3D); // =
const CharUtf16 char_DoubleQuote    = TraitsU16::to_char_type(0x22); // " (quotation)
const CharUtf16 char_SingleQuote    = TraitsU16::to_char_type(0x27); // ' (apostrophe)
const CharUtf16 char_Semicolon      = TraitsU16::to_char_type(0x3B); // ;
const CharUtf16 char_Underscore     = TraitsU16::to_char_type(0x5F); // _
const CharUtf16 char_Colon          = TraitsU16::to_char_type(0x3A); // :
const CharUtf16 char_Ampersand      = TraitsU16::to_char_type(0x26); // &
const CharUtf16 char_HashMark       = TraitsU16::to_char_type(0x23); // #
const CharUtf16 char_LeftSqBracket  = TraitsU16::to_char_type(0x5B); // [
const CharUtf16 char_RightSqBracket = TraitsU16::to_char_type(0x5D); // ]
const CharUtf16 char_Percent        = TraitsU16::to_char_type(0x25); // %
const CharUtf16 char_CapitalX       = TraitsU16::to_char_type(0x58); // X
const CharUtf16 char_SmallX         = TraitsU16::to_char_type(0x78); // x
const CharUtf16 char_Replacement    = TraitsU16::to_char_type(0xFFFD);


bool is_space(CharUtf16 c)
{
    // This is the correct check for HTML 4.01. In XML 1.0 and XML 1.1
    // the same set of characters is used, except they exclude form
    // feed. For SGML in general, it is probably configurable.
    TraitsU16::int_type i = TraitsU16::to_int_type(c);
    return i <= 0x20 && (i == 0x20 || // Space
                         i == 0x0A || // Newline
                         i == 0x0D || // Carriage return
                         i == 0x09 || // Horizonatal tab
                         i == 0x0C);  // Form feed (not allowed in XML)
}



/// Input transcoding and buffering. Input is offered as a sequence
/// of UTF-16 elements.
///
/// Beyond transcoding, this class also provides the following two
/// nontrivial features:
///
/// 1) Possibility of injection of arbitrary amounts of artificial
///    UTF-16 data into the input stream. This is needed for the
///    document.write() feature of HTML.
///
/// 2) Possibility of "on the fly" switching of the assumed source
///    character encoding. This is needed for the (bizarre) character
///    encoding info in <META HTTP-EQUIV="Content-Type" ...> feature
///    of HTML.
class Input {
public:
    /// Allows the consumer to process buffered UTF-16 input before
    /// more data is requested from the raw input stream. This may be
    /// appropriate when reading from a network socket, or from a
    /// terminal where a read request may block for a significant
    /// amount of time.
    class BufferEmptyListener {
    public:
        virtual void on_input_buffer_empty() = 0;

        virtual ~BufferEmptyListener() {}
    };


    Input(InputStream& s, const StringUtf16& charenc, BufferEmptyListener* e = 0):
        src(s), got_eoi_from_transcode(false),
        transcoder(get_transcoder_to_utf16(transcode_ISO_8859_1).release()),
        transcoder_needs_input(true), got_eoi_from_source(false), buffer_empty_listener(e)
    {
        buf_u16_soft_begin = buf_u16.soft_end = buf_u16.begin = new CharUtf16[BufferU16::size];
        buf_u16_artificial_end = buf_u16_soft_begin;
        buf_u16_line_loc_pos = buf_u16_soft_begin;
        line_loc.line_num = 1;
        line_loc.column_idx = 0;

        buf_raw_soft_begin = buf_raw.soft_end = buf_raw.begin = new char[BufferRaw::size];

        if (!charenc.empty()) {
            std::string charenc_narrow;
            if (utf16_to_narrow_port(charenc, charenc_narrow)) {
                try {
                    UniquePtr<TranscoderToUtf16> tmp(get_transcoder_to_utf16(charenc_narrow).release());
                    switch_transcoder(tmp);
                }
                catch (TranscoderNotAvailableException &) {}
            }
        }
    }


    ~Input()
    {
        delete[] buf_u16.begin;
        typedef std::vector<BufferU16>::const_iterator iter;
        iter e = extra_bufs_u16.end();
        for (iter i=extra_bufs_u16.begin(); i!=e; ++i)
            delete[] i->begin;
    }


    /// Returns true if, and only if the end of the stream has been
    /// reached.
    bool eoi()
    {
        return empty_u16() && !prepare_u16();
    }

    /// Returns, but does not extract the next input characters. If
    /// there is no next input character (if eoi() would have returned
    /// true), this function returns the null character, but note that
    /// it also returns the null character when they occur in the
    /// input stream.
    CharUtf16 peek()
    {
        return eoi() ? CharUtf16() : *buf_u16_soft_begin;
    }

    /// Extract the next input character, if any. If there is no next
    /// input character (if eoi() would have returned true), this
    /// function returns false, otherwise it returns true and assigns
    /// the next character to \a ch.
    bool get(CharUtf16& ch)
    {
        if (eoi())
            return false;
        ch = *buf_u16_soft_begin++;
        return true;
    }

    /// Drop the next input character.
    ///
    /// DANGEROUS: May be called only in a case where eoi() would
    /// return false. The consequences of violating this rule are
    /// undefined (memory corruption is likely). The intention is that
    /// this function is only called after a call to peek() that did
    /// not return the null character, or after a call to unget().
    void drop()
    {
        ARCHON_ASSERT(!eoi());
        ++buf_u16_soft_begin;
    }

    // Pretend that the latest extracted character was no extracted.
    //
    // DANGEROUS: Must follow a successfull call to get() or a call to
    // drop(), and there must not be any calls to unget() or other
    // Input functions in between. The consequences of violating these
    // rules are undefined (memory corruption is likely).
    void unget()
    {
        ARCHON_ASSERT(buf_u16_soft_begin != buf_u16.begin);
        --buf_u16_soft_begin;
    }


    /// Read characters from the input stream until a delimiter is
    /// reached.
    ///
    /// The extracted characters are stored in the specified buffer, and
    /// the number of extracted characters is returned. Reading stops
    /// early if the buffer fills up, or if the end of input is
    /// reached.
    std::size_t read_until(CharUtf16 *b, std::size_t n, CharUtf16 delim)
    {
        CharUtf16* b0 = b;
        while (0 < n && !eoi()) {
            std::size_t m = std::min<std::size_t>(n, buf_u16.soft_end - buf_u16_soft_begin);
            CharUtf16* p = std::find(buf_u16_soft_begin, buf_u16_soft_begin+m, delim);
            b = std::copy(buf_u16_soft_begin, p, b);
            buf_u16_soft_begin = p;
            if (p != buf_u16.soft_end)
                break;
            n -= m;
        }
        return b - b0;
    }


    // Insert the specified characters as fake data in the input
    // stream at the current read position.
    //
    // Note: Injected data will not be reprocessed if the source
    // character encoding is changed. For this reason, it is important
    // that injection is not used in an attempt to "roll back" the
    // input position after having looked ahead, unless the caller can
    // guarantee that injected data is not supposed to be
    // reinterpreted by a new transcoder. A sufficient requirement,
    // when used to roll back, is that whenever the injected data
    // contains the closing delimiter of a start tag (a META tag in
    // particular) it does not contain anything beyond that closing
    // delimiter.
    template<class Iter> void inject(Iter begin, Iter end)
    {
        // If we have no artificial data already
        if (extra_bufs_u16.empty() && buf_u16_artificial_end <= buf_u16_soft_begin) {

            advance_line_loc();

            // If the buffer is nearly empty, push the contents to the end
            // to maximize the amount of free space before the current
            // read position. This step is idempotent.
            std::size_t used = buf_u16.soft_end - buf_u16_soft_begin;
            if (used <= 16) {
                CharUtf16* e = buf_u16.begin + BufferU16::size;
                if (buf_u16.soft_end != e) {
                    std::copy(buf_u16_soft_begin, buf_u16.soft_end, e - used);
                    buf_u16_soft_begin = e - used;
                    buf_u16.soft_end = e;
                }
            }

            buf_u16_artificial_end = buf_u16_soft_begin;
            buf_u16_line_loc_pos   = buf_u16_soft_begin;
        }

        std::size_t n = end - begin;
        std::size_t free = buf_u16_soft_begin - buf_u16.begin;
        for (;;) {
            if (n <= free) { // The rest of the incoming data fits in this buffer
                buf_u16_soft_begin -= n;
                std::copy(begin, end, buf_u16_soft_begin);
                break;
            }
            if (0 < free) {
                n -= free;
                Iter e = end;
                end -= free;
                std::copy(end, e, buf_u16.begin);
            }
            extra_bufs_u16.push_back(buf_u16); // May throw
            buf_u16.begin = 0; // In case the following memory alloc fails
            buf_u16.begin = new CharUtf16[BufferU16::size];
            buf_u16_soft_begin = buf_u16.soft_end = buf_u16.begin + BufferU16::size;
            free = BufferU16::size;
        }
    }


    void inject(CharUtf16 c) { inject(&c, &c+1); }


    void inject(const StringUtf16& s) { inject(s.begin(), s.end()); }


    // Returns true if not at end-of-input and the next character is
    // not artificial.
    bool next_char_is_real()
    {
        if (eoi())
            return false;
        if (!extra_bufs_u16.empty())
            return false;
        return buf_u16_artificial_end <= buf_u16_soft_begin;
    }


    // Use the specified transcoder for all input data beyond the
    // current position. This does not include data that was
    // artificially injected into the input stream.
    //
    // This scheme is only guaranteed to work if both the original and
    // the new character encodings are stateless, that is, when they
    // do not use shift states. It may be ok to simply state that when
    // switching to a statefull charenc, the new charenc will apply
    // immediately after the closing delimiter of the meta tag, and it
    // will be in an unshifted state at that point. With respect to
    // the input encoding, it may actually be possible to detect
    // whether the transcoder is in a shifted state, both at its
    // current position, and at a previous position (by retranscoding
    // intervending chars with new transcoder, and seeing if that
    // produces a transcoder in a shifted state). However, it is
    // probably better to simply have a white-list of input encodings
    // that are known to be stateless (US-ASCII, ISO-8859-1,
    // ISO-8859-* (probably), UTF-8, UTF-16, UTF-32, WINDOWS_1252),
    // and then simply disabling charenc swithing if the initial
    // encoding is not one of those.
    void switch_transcoder(UniquePtr<TranscoderToUtf16>& t)
    {
        advance_line_loc();

        // For exception safety: Fist calculate the number of consumed
        // UTF-16 elements (num_u16_consumed) and then allocate a
        // /dev/null buffer of that size.

        // Find the UTF-16 buffer that will beconsumed last. This is
        // always the one that holds buffered non-artificial data when
        // buffered non-artificial UTF-16 data exists.
        BufferU16* last_buf_u16;
        CharUtf16* new_soft_end_u16;
        if (extra_bufs_u16.empty()) {
            last_buf_u16 = &buf_u16;
            new_soft_end_u16 = std::max(buf_u16_soft_begin, buf_u16_artificial_end);
        }
        else {
            last_buf_u16 = &extra_bufs_u16.front();
            new_soft_end_u16 = buf_u16_artificial_end;
        }

        // If there is no non-artificial buffered UTF-16 data, then
        // skip all the buffer restoration. This make the buffer
        // restoration an idempotent operation.
        if (last_buf_u16->soft_end != new_soft_end_u16) {

            std::size_t num_u16_consumed = new_soft_end_u16 - last_buf_u16->begin;

            Array<CharUtf16> dev_null(num_u16_consumed); // May throw

            // This reverting of the "raw" buffer read position coupled
            // with the following truncated transcoding operation amounts
            // to a "roll back" of the raw buffer read position to the
            // position that corresponds with the current read position in
            // the UTF-16 buffer. This is possible because each
            // transcode_read() invocation leaves the complete chunk of
            // original data unclobbered until the next transcode
            // invocation.
            buf_raw_soft_begin = prev_buf_raw_soft_begin;
            transcoder_needs_input = false;
            std::size_t n = transcoding_read(dev_null.get(), num_u16_consumed);
            // Previous transcode invocation converted at least this
            // amount, so we must always get to completion (filled output
            // buffer) in one attempt.
            ARCHON_ASSERT_1(n == num_u16_consumed, "Transcoder stopped unexpectedly");

            // Now discard the non-artificial buffered UTF-16 data.
            last_buf_u16->soft_end = new_soft_end_u16;
        }

        transcoder = t;
    }


    struct LineLoc {
        long line_num; // First line is line one.
        long column_idx; // First column is at index zero.
    };


    void get_line_loc(LineLoc& loc)
    {
        advance_line_loc();
        loc = line_loc;
    }


    void push_source(UniquePtr<InputStream>, const StringUtf16& charenc)
    {
        static_cast<void>(charenc);
        // FIXME: Implement this by maintaining a stack of
        // inputs. When an input is popped the application must be
        // able to get a notification.
    }


private:
    struct BufferU16 {
        static const std::size_t size = 1024;

        CharUtf16* begin;
        CharUtf16* soft_end;
    };


    struct BufferRaw {
        static const std::size_t size = 1024;

        char* begin;
        char* soft_end;
    };


    InputStream& src;
    bool got_eoi_from_transcode;
    BufferU16 buf_u16;
    CharUtf16* buf_u16_soft_begin;
    std::vector<BufferU16> extra_bufs_u16; // These may be empty. First buffer is to be consumed last.
    CharUtf16* buf_u16_artificial_end; // If not extra_bufs_u16.empty() or buf_u16_soft_begin < buf_u16_artificial_end, then buf_u16_artificial_end marks the end of artificial data within the last UTF-16 buffer to be consumed.
    CharUtf16* buf_u16_line_loc_pos;
    LineLoc line_loc;
    UniquePtr<TranscoderToUtf16> transcoder;
    bool transcoder_needs_input;
    bool got_eoi_from_source;
    BufferRaw buf_raw;
    char* buf_raw_soft_begin;
    char* prev_buf_raw_soft_begin;
    BufferEmptyListener* const buffer_empty_listener;


    // Is current buffer empty
    bool empty_u16()
    {
        return buf_u16_soft_begin == buf_u16.soft_end;
    }


    // Assumes current buffer is empty
    bool prepare_u16()
    {
        // Check for extra buffers due to injected data
        while (!extra_bufs_u16.empty()) {
            delete[] buf_u16.begin;
            buf_u16 = extra_bufs_u16.back();
            extra_bufs_u16.pop_back();
            buf_u16_soft_begin = buf_u16.begin;
            // Buffer may have become empty due to charenc switching
            if (buf_u16_soft_begin != buf_u16.soft_end)
                return true;
        }

        if (!got_eoi_from_transcode) {
            advance_line_loc();

            // Transcode another chunk
            if (std::size_t n = transcoding_read(buf_u16.begin, BufferU16::size)) {
                buf_u16_soft_begin     = buf_u16.begin;
                buf_u16.soft_end       = buf_u16.begin + n;
                buf_u16_artificial_end = buf_u16_soft_begin; // No artificial data after transcode
                buf_u16_line_loc_pos   = buf_u16_soft_begin;
                return true;
            }

            got_eoi_from_transcode = true;
        }

        return false;
    }


    // 'n' must be at least 64 - a conservative guess on the maximum
    // number of bytes per logical character in any character
    // encoding.
    //
    // At return, the entire raw data chunk that corresponds to the
    // transcoded data is stored in the raw data buffer starting at
    // 'prev_buf_raw_soft_begin'.
    std::size_t transcoding_read(CharUtf16* b, std::size_t n)
    {
        CharUtf16* b0 =  b;
//        bool did_read = false;

        try {
            if (transcoder_needs_input) {
                if (got_eoi_from_source)
                    return 0;

              read:
                // Copy remaining data in input buffer back to start
                std::size_t in_left = buf_raw.soft_end - buf_raw_soft_begin;
                char* end = buf_raw.begin + in_left;
                buf_raw_soft_begin = std::copy_backward(buf_raw_soft_begin, buf_raw.soft_end, end);
                buf_raw.soft_end = end;

                // Notify a 'buffer empty' listener
                if (buffer_empty_listener)
                    buffer_empty_listener->on_input_buffer_empty();

                // Read from source
                std::size_t n2 = buf_raw.begin + BufferRaw::size - buf_raw.soft_end;
                ARCHON_ASSERT_1(0 < n2, "Unexpected lack of free space in input buffer");
                if (std::size_t m = src.read(buf_raw.soft_end, n2)) {
                    buf_raw.soft_end += m;
                }
                else {
                    got_eoi_from_source = true;
                }
//                did_read = true;
            }

            prev_buf_raw_soft_begin = buf_raw_soft_begin; // So charenc switcher can revert
            const char*& in = const_cast<const char*&>(buf_raw_soft_begin);
            transcoder_needs_input =
                transcoder->transcode(in, buf_raw.soft_end, b, b0+n, got_eoi_from_source);

            if (transcoder_needs_input && !got_eoi_from_source) {
                if (b == b0)
                    goto read; // Otherwise we would return a false end-of-input marker.
            }
        }
        catch (TranscodeException &e) {
            throw ReadException(e.what());
        }

        return b - b0;
    }


    // FIXME: Must discount UTF-16 surrogates in the derivation of the
    // column number.
    // FIXME: Should account for tabs assuming a tab stop spacing of
    // 8.
    void advance_line_loc()
    {
        if (!extra_bufs_u16.empty())
            return;
        CharUtf16* p = buf_u16_line_loc_pos;
        CharUtf16* line_begin = 0;
        while (p < buf_u16_soft_begin) {
            TraitsU16::int_type i = TraitsU16::to_int_type(*p);
            if (ARCHON_UNLIKELY(i == 0x0A)) {
                ++line_loc.line_num;
                line_begin = p+1;
            }
            ++p;
        }
        if (line_begin) {
            line_loc.column_idx = buf_u16_soft_begin - line_begin;
        }
        else {
            line_loc.column_idx += buf_u16_soft_begin - buf_u16_line_loc_pos;
        }
        buf_u16_line_loc_pos = buf_u16_soft_begin;
    }
};




// Static information comprising the Document Type Definition
struct DocTypeDef {
    struct Entity {
        virtual StringUtf16 get_replacement_text() const = 0;

        virtual ~Entity() {}
    };

    virtual const Entity* lookup_entity(const StringUtf16& name_cf) const = 0;

    virtual ~DocTypeDef() {}
};




struct NullDtd: DocTypeDef {
    virtual const Entity* lookup_entity(const StringUtf16&) const
    {
        return 0;
    }
};


NullDtd null_dtd;




// Cached info about general entity
struct GenEntity {
    bool valid;
    bool is_simple_text;
    StringUtf16 simple_text;

    GenEntity(): valid(false), is_simple_text(false) {}
};




struct ElementDef {
    bool valid;
    StringUtf16 name;
    bool use_special_cdata_content_handler;
    bool consider_content_as_script;

    ElementDef():
        valid(false), use_special_cdata_content_handler(false), consider_content_as_script(false) {}
};




class DtdParser {
public:
    DtdParser(const Source& src, Callbacks& cb, Resolver& resolv, Logger* log,
              const Config& cfg, Input::BufferEmptyListener* el = 0):
        in(src.in, src.charenc, el),
        system_ident(src.system_ident),
        base_uri(src.base_uri),
        callbacks(cb),
        resolver(resolv),
        logger(log),
        config(cfg),
        case_insens_entity(false),
        warn_on_multiple_entity_decls(true),
        str_PUBLIC_cf(get_case_folded(utf16_from_port("PUBLIC"))),
        str_SYSTEM_cf(get_case_folded(utf16_from_port("SYSTEM"))),
        str_ENTITY_cf(get_case_folded(utf16_from_port("ENTITY"))),
        str_NOTATION_cf(get_case_folded(utf16_from_port("NOTATION"))),
        str_ELEMENT_cf(get_case_folded(utf16_from_port("ELEMENT"))),
        str_ATTLIST_cf(get_case_folded(utf16_from_port("ATTLIST")))
    {
    }

    void parse_dtd(bool internal_subset = false);

protected:
    Input in;
    const StringUtf16 system_ident, base_uri;
    Callbacks& callbacks;
    Resolver& resolver;
    Logger* const logger;
    const Config config;
    const bool case_insens_entity, warn_on_multiple_entity_decls;
    const StringUtf16 str_PUBLIC_cf, str_SYSTEM_cf;
    const StringUtf16 str_ENTITY_cf, str_NOTATION_cf, str_ELEMENT_cf, str_ATTLIST_cf;
    StringUtf16 entity_name, entity_name_cf;

    struct MarkedSection {
        const int m_entity_repl_level;
        MarkedSection(int entity_repl_level):
            m_entity_repl_level(entity_repl_level)
        {
        }
    };


    void case_fold(StringUtf16& s)
    {
        if (config.case_insensitive)
            case_fold_ascii(s); // Optimized for cases where ASCII is dominant
    }


    StringUtf16 get_case_folded(StringUtf16 s)
    {
        case_fold(s);
        return s;
    }


    void case_fold_entity(StringUtf16& s)
    {
        if (case_insens_entity)
            case_fold_ascii(s); // Optimized for cases where ASCII is dominant
    }


    bool valid_name(const StringUtf16& n)
    {
        if (config.accept_xml_1_0_names)
            return validate_xml_1_0_name(n);

        typedef StringUtf16::const_iterator iter;
        iter e = n.end();
        iter i = n.begin();
        if (i == e)
            return false;
        if (!valid_first_name_char_strict(*i))
            return false;
        while (++i != e) {
            if (!valid_second_name_char_strict(*i))
                return false;
        }
        return true;
    }


    bool valid_first_name_char_strict(CharUtf16 c)
    {
        // FIXME: Extra valid characters may be avaialble based on the
        // SGML configuration. The base set, however, is precisely the
        // Latin letters from the ASCII character set. The SGML
        // configuration can only add to this base set. The test below
        // is valid for HTML 4.01.
        TraitsU16::int_type i = TraitsU16::to_int_type(c);
        return 0x61 <= i ? i <= 0x7A :  // a -> z
            0x41 <= i && i <= 0x5A;     // A -> Z
    }


    bool valid_second_name_char_strict(CharUtf16 c)
    {
        // FIXME: Other valid characters may be avaialble based on the
        // SGML configuration. The base set, however, is precisely the
        // Latin letters from the ASCII character set plus the 10
        // digits from the ASCII character set. The SGML configuration
        // can only add to this base set. The test below is valid for
        // HTML 4.01. Note that the reference concrete syntax of SGML
        // does not allow underscore and colon. The reference concrete
        // syntax is to be thought of as the default syntax, not the
        // base syntax. The former is what you get if you do not
        // supply a custom configuration. Tha later is the basis for
        // modifications in a custom configuration.
        if (valid_first_name_char_strict(c))
            return true;
        TraitsU16::int_type i = TraitsU16::to_int_type(c);
        return 0x30 <= i && i <= 0x39 || c == char_Underscore || c == char_Hyphen ||
            c == char_FullStop || c == char_Colon;
    }

    void warn(const char* msg)
    {
        int col_index_adj = 0;
        bool is_warning = true;
        handle_error(col_index_adj, msg, tuple(), is_warning);
    }

    template<class A> void warn(const char* msg, const A& a)
    {
        int col_index_adj = 0;
        bool is_warning = true;
        handle_error(col_index_adj, msg, tuple(a), is_warning);
    }

    template<class A, class B> void warn(const char* msg, const A& a, const B& b)
    {
        int col_index_adj = 0;
        bool is_warning = true;
        handle_error(col_index_adj, msg, tuple(a,b), is_warning);
    }

    void warn_adj(int col_index_adj, const char* msg)
    {
        bool is_warning = true;
        handle_error(col_index_adj, msg, tuple(), is_warning);
    }

    template<class A> void warn_adj(int col_index_adj, const char* msg, const A& a)
    {
        bool is_warning = true;
        handle_error(col_index_adj, msg, tuple(a), is_warning);
    }

    template<class A, class B>
    void warn_adj(int col_index_adj, const char* msg, const A& a, const B& b)
    {
        bool is_warning = true;
        handle_error(col_index_adj, msg, tuple(a,b), is_warning);
    }


    void error(const char* msg)
    {
        int col_index_adj = 0;
        bool is_warning = false;
        handle_error(col_index_adj, msg, tuple(), is_warning);
    }

    template<class A> void error(const char* msg, const A& a)
    {
        int col_index_adj = 0;
        bool is_warning = false;
        handle_error(col_index_adj, msg, tuple(a), is_warning);
    }

    template<class A, class B> void error(const char* msg, const A& a, const B& b)
    {
        int col_index_adj = 0;
        bool is_warning = false;
        handle_error(col_index_adj, msg, tuple(a,b), is_warning);
    }

    void error_adj(int col_index_adj, const char* msg)
    {
        bool is_warning = false;
        handle_error(col_index_adj, msg, tuple(), is_warning);
    }

    template<class A> void error_adj(int col_index_adj, const char* msg, const A& a)
    {
        bool is_warning = false;
        handle_error(col_index_adj, msg, tuple(a), is_warning);
    }

    template<class A, class B>
    void error_adj(int col_index_adj, const char* msg, const A& a, const B& b)
    {
        bool is_warning = false;
        handle_error(col_index_adj, msg, tuple(a,b), is_warning);
    }


    template<class L>
    void handle_error(int col_index_adj, const char* msg, const Tuple<L>& params, bool is_warning)
    {
        if (config.treat_warnings_as_errors)
            is_warning = false;

        if (logger) {
            std::ostringstream out;
            out.imbue(logger->getloc());
            Input::LineLoc line_loc;
            in.get_line_loc(line_loc);
            ARCHON_ASSERT(line_loc.column_idx >= col_index_adj);
            line_loc.column_idx += col_index_adj;
            out << system_ident << ":" << line_loc.line_num << ":" << line_loc.column_idx << ": ";
            const char* severity = is_warning ? "warning: " : "error: ";
            out << severity;
            param_expand(out, msg, params);
            logger->log(out.str());
        }

        if (!is_warning && config.die_on_first_error)
            throw std::runtime_error("Parse error");
    }


    // Skip "parameter separators" until a non-separator character is
    // found, or an error occurs.
    //
    // This function returns false if an error occurs. Otherwise it
    // assigns the non-separator character to the specified character
    // object, and retutns true.
    //
    // In this process, comments will be silently skipped, and
    // parameter entity references will be expanded recursively.
    //
    // The value of `entity_replacement_level` may increase or decrease,
    // but can never decrease below `decl_replacement_level`.
/*
    bool skip_param_sep(CharUtf16& ch)
    {
        CharUtf16 c;

      next:
        if (ARCHON_LIKELY(in.get(c))) {
            if (ARCHON_LIKELY(is_space(c)))
                goto next;
            if (ARCHON_LIKELY(c != char_Percent)) {
                if (ARCHON_LIKELY(c != char_Hyphen)) {
                  done:
                    ch = c;
                    return true;
                }
                if (ARCHON_LIKELY(in.peek() == char_Hyphen)) {
                    in.drop();
                    goto comment;
                }
                goto done;
            }
            c = in.peek();
            if (ARCHON_LIKELY(valid_first_name_char_strict(c)))
                goto entity;
            c = char_Percent;
            goto done;
        }
        // End of input/entity in declaration
        if (ARCHON_LIKELY(entity_replacement_level > 0)) {
            if (ARCHON_LIKELY(decl_entity_replacement_level < entity_replacement_level)) {
                pop_entity_replacement();
                goto next;
            }
        }
        error("Unterminated declaration");
        goto fail;

      entity:
        entity_name = c;
        in.drop();
        {
            bool allow_proc_instr = false;
            eat_rest_of_param_entity_ref(allow_proc_instr);
        }
        goto next;

      comment:
        if (ARCHON_LIKELY(in.get(c))) {
            if (ARCHON_LIKELY(c != char_Hyphen))
                goto comment;
            if (ARCHON_LIKELY(in.peek() == char_Hyphen)) {
                in.drop();
                goto next;
            }
            goto comment;
        }
        // End of input/entity in comment
        error("Unterminated comment");

      fail:
        return false;
    }
*/


    // First name char must already be stored in `entity_name`
    void eat_rest_of_param_entity_ref(bool allow_proc_instr)
    {
        CharUtf16 ch;
        for (;;) {
            ch = in.peek();
            if (!valid_second_name_char_strict(ch))
                break;
            in.drop();
            entity_name += ch;
        }
        if (ch == char_Semicolon)
            in.drop();
        // FIXME: In XML mode, it is an error if the reference is not
        // terminated by a `;`.
        //
        // FIXME: What happens in XML mode if the reference is not
        // terminated by a `;`? How does parsing continue?
        entity_name_cf = entity_name;
        case_fold_entity(entity_name_cf);
        EntityMap::const_iterator i = param_entities.find(entity_name_cf);
        if (i == param_entities.end() || !i->second.is_valid) {
            int adj = -2 - int(entity_name.size());
            if (ch == char_Semicolon)
                --adj;
            warn_adj(adj, "Undefined parameter entity `%1`", entity_name);
            return;
        }
        const Entity& entity = i->second;
        if (ARCHON_UNLIKELY(entity.type == Entity::type_PI)) {
            if (ARCHON_LIKELY(allow_proc_instr)) {
                callbacks.proc_instr(entity.replacement_text);
                return;
            }
            int adj = -2 - int(entity_name.size());
            if (ch == char_Semicolon)
                --adj;
            error_adj(adj, "Processing instruction entity `%1` not allowed here", entity_name);
            return;
        }
        ARCHON_ASSERT(entity.type == Entity::type_Regular);
        if (ARCHON_LIKELY(!entity.is_external)) {
            throw std::runtime_error("Not yet implemented");
/*
            const CharUtf16* data = entity.replacement_text.data();
            Input::Chunk chunk(data, data + entity.replacement_text.size());
            bool take_ownership = false;
            bool seamless = false;
            in.push_source(chunk, take_ownership, seamless);
*/
            return;
        }
        try {
            throw std::runtime_error("Not yet implemented");
/*
            UniquePtr<InputStream> stream;
            StringUtf16 charenc, uri;
            resolver.resolve(entity.public_ident, entity.system_ident, base_uri,
                             stream, charenc, uri);
            UniquePtr<TranscodingSource> source(new TranscodingSource());
            in.push_source(src, charenc);
            // FIXME: Must also stack this->system_ident and this->base_uri and then set them to 'uri'.
*/
        }
        catch (const ResolveException& e) {
            error("Failed to resolve external parameter entity '%1': %2",
                  entity.system_ident, e.what());
        }
    }

/*
                                                        
  If end-of-input:
    If entity_repl_level > 0:
      If current decl (or marked section keywords section) was started in the current replacement text (decl_entity_repl_level == entity_repl_level):
        Report an error
      Pop off the top chunk and continue

  If decl-end token `>` (or `[` in case of marked section keywords section):
    If current decl (or marked section keywords section) was not started in the current replacement text (decl_entity_repl_level < entity_repl_level):
      Report an error
                                                        

                    in.unget();
                    break;
                }
                if (!in.get(c)) {
                  bail_comment:
                    in.inject(char_Hyphen);
                    break;
                }
                if (c != char_Hyphen) {
                    in.unget();
                    goto bail_comment;
                }
                // Skip rest of comment
                for (;;) {
                    if (!in.get(c)) {
                      unterm:
                        if (!inhibit_error_msg)
                            error("Unterminated comment in declaration tag");
                        return true;
                    }
                    if (c == char_Hyphen) {
                        if (!in.get(c))
                            goto unterm;
                        if (c == char_Hyphen)
                            break;
                    }
                }
            }
            space_seen = true;
        }
    }
*/

    // Skip "parameter separator" which may contain inlined comments
    // Returns true if, and only if at least one character was
    // skipped.
    //
    // FIXME: Must also recognize parameter entity references (with
    // some constraints imposed by SGML)
    bool skip_decl_space(bool inhibit_error_msg = false)
    {
        bool space_seen = false;
        CharUtf16 c;
        for (;;) {
            if (!in.get(c))
                break;
            if (!is_space(c)) {
                if (c != char_Hyphen) {
                    in.unget();
                    break;
                }
                if (!in.get(c)) {
                  bail_comment:
                    in.inject(char_Hyphen);
                    break;
                }
                if (c != char_Hyphen) {
                    in.unget();
                    goto bail_comment;
                }
                // Skip rest of comment
                for (;;) {
                    if (!in.get(c)) {
                      unterm:
                        if (!inhibit_error_msg)
                            error("Unterminated comment in declaration tag");
                        return true;
                    }
                    if (c == char_Hyphen) {
                        if (!in.get(c))
                            goto unterm;
                        if (c == char_Hyphen)
                            break;
                    }
                }
            }
            space_seen = true;
        }
        return space_seen;
    }


    bool eat_decl_word(StringUtf16& word)
    {
        StringUtf16 w;
        CharUtf16 c;
        for (;;) {
            if (!in.get(c))
                goto done;
            TraitsU16::int_type i = TraitsU16::to_int_type(c);
            if (0x40 <= i) {
                if (i == 0x5B || i == 0x5D)
                    break;  // Stop on '[', and ']'
            }
            else if (0x30 <= i) {
                if (0x3C <= i)
                    break; // Stop on '<', '=', '>', and '?'
            }
            else if (is_space(c)) {
                goto unget;
            }
            else if (c == char_Hyphen) {
                if (in.peek() == char_Hyphen) { // Stop on comment
                    in.inject(char_Hyphen);
                    goto done;
                }
            }
            else if (c == char_DoubleQuote || c == char_SingleQuote ||
                     c == char_Solidus || c == char_Exclamation || c == char_Question) {
                goto unget;
            }
            w += c;
        }
      unget:
        in.unget();
      done:
        if (w.empty())
            return false;
        word = w;
        return true;
    }


    bool eat_quoted_str(StringUtf16& str, bool inhibit_error_msg = false)
    {
        CharUtf16 d;
        if (!in.get(d))
            return false;
        if (d != char_DoubleQuote && d != char_SingleQuote) {
            in.unget();
            return false;
        }
        str.clear();
        CharUtf16 c;
        for (;;) {
            if (!in.get(c)) {
                if (!inhibit_error_msg)
                    error("Unterminated quoted string literal");
                break;
            }
            if (c == d)
                break;
            str += c;
        }
        return true;
    }


    /// Check whether the specified string is a valid minimum literal,
    /// that is, a string whose characters are restricted to a certain
    /// 'minimum' set specified by SGML.
    bool valid_min_lit(const StringUtf16& n)
    {
        typedef StringUtf16::const_iterator iter;
        iter e = n.end();
        for (iter j = n.begin(); j != e; ++j) {
            TraitsU16::int_type i = TraitsU16::to_int_type(*j);
            if (0x41 <= i) {
                // Accept letters
                if (i <= 0x5A || 0x61 <= i && i <= 0x7A)
                    continue;
                return false;
            }
            if (0x27 <= i) {
                // Accept '\'', '(', ')', '+', ',', '-', '.', '/', digits, ':', '=' and '?'
                if (i <= 0x3A ? i != 0x2A : i == 0x3D || i == 0x3F)
                    continue;
                return false;
            }
            // Accept newline, carriage return, and space
            if (i == 0x20 || i == 0x0A || i == 0x0D)
                continue;
            return false;
        }
        return true;
    }


    bool eat_ext_ident(const StringUtf16& keyword_cf, bool& fail, StringUtf16& public_ident,
                       StringUtf16& system_ident, bool& space_after)
    {
        if (keyword_cf == str_PUBLIC_cf) {
            bool space_before = skip_decl_space();
            if (!eat_quoted_str(public_ident)) {
                error("Missing public identifier in document type declaration");
                fail = true;
                return true;
            }
            if (!valid_min_lit(public_ident))
                error("Public identifier is not a valid minimum literal");
            if (!space_before)
                error("No space before public identifier");
            goto sys;
        }

        if (keyword_cf == str_SYSTEM_cf) {
            public_ident.clear();
          sys:
            bool space_before = skip_decl_space();
            if (eat_quoted_str(system_ident)) {
                if (!space_before)
                    error("No space before system identifier");
                space_after = skip_decl_space();
            }
            else {
                system_ident.clear();
                space_after = space_before;
            }
            fail = false;
            return true;
        }

        return false;
    }


    void end_decl(const char* type)
    {
        bool garbage_seen = false;
        for (;;) {
            skip_decl_space();
            CharUtf16 c;
            if (!in.get(c)) {
                error("Unterminated %1 declaration", type);
                return;
            }
            if (c == char_Greater)
                return;
            if (!garbage_seen) {
                error("Garbage in %1 declaration", type);
                garbage_seen = true;
            }
        }
    }


private:
    struct Entity {
        // XML allows only type_Regular.
        // The type has a saying in how the replacement text is going to be processed
        enum Type {
            type_Regular,  // Parsed character data
            type_CDATA,    // Unparsed character data (only available with general entities)
            type_SDATA,    // Specific character data (only available with general entities) (probably unparsed) (see http://xml.coverpages.org/entCDATA.html)
            type_PI        // Process replacement text as a processing instruction. Expansion only allowed where a processing instruction can validly occur, and never in entity literals.
        };
        Type type;
        bool is_valid;
        bool is_external;
        StringUtf16 replacement_text;
        StringUtf16 public_ident, system_ident;
        Entity(): is_valid(false), is_external(false) {}
    };

    typedef std::map<StringUtf16, Entity> EntityMap;
    EntityMap param_entities; // Indexed by entity name


    void skip_space()
    {
        CharUtf16 c;
        for (;;) {
            if (!in.get(c))
                break;
            if (!is_space(c)) {
                in.unget();
                break;
            }
        }
    }


    void log(const std::string& msg)
    {
        if (logger)
            logger->log(msg);
    }
};



class SgmlParser: public DtdParser, Input::BufferEmptyListener {
public:
    SgmlParser(const DocTypeDef* d, const Source& src, Callbacks& cb, Resolver& resolv,
               Logger* log, const Config& cfg):
        DtdParser(src, cb, resolv, log, cfg, this),
        dtd(d ? *d : null_dtd),
        str_DOCTYPE_cf(get_case_folded(utf16_from_port("DOCTYPE"))),
        str_META_cf(get_case_folded(utf16_from_port("META"))),
        str_HTTP_EQUIV_cf(get_case_folded(utf16_from_port("HTTP-EQUIV"))),
        str_CONTENT_TYPE_cf(get_case_folded(utf16_from_port("Content-Type"))),
        str_CONTENT_cf(get_case_folded(utf16_from_port("CONTENT"))),
        str_CHARSET_uc(get_upcased(utf16_from_port("CHARSET"))),
        str_SCRIPT_cf(get_case_folded(utf16_from_port("SCRIPT"))),
        str_STYLE_cf(get_case_folded(utf16_from_port("STYLE"))),
        text_accum_no_flush(false),
        remain_artificial_scripts(max_consecutive_artificial_scripts)
    {
    }

    void parse_sgml();

private:
    struct ElemCdataInputStream;

    const DocTypeDef& dtd;
    const StringUtf16 str_DOCTYPE_cf;
    const StringUtf16 str_META_cf, str_HTTP_EQUIV_cf, str_CONTENT_TYPE_cf, str_CONTENT_cf,
        str_CHARSET_uc;
    const StringUtf16 str_SCRIPT_cf, str_STYLE_cf;
    StringUtf16 tag_name, tag_name_cf;

    // Accumulated character data. It is used primarily for
    // accumulating text to be reported through Callbacks::text(), but
    // is sometimes used for other purposes. While it is used for
    // other purposes, `text_accum_no_flush` must be set to true to
    // prevent on_input_buffer_empty() from flushing it. Note that
    // on_input_buffer_empty() may get called at any time during
    // extraction of data from the associated Input object.
    StringUtf16 text_accum;
    bool text_accum_no_flush;

    Attr attr;
    std::vector<Attr> attribs;
    std::map<StringUtf16, GenEntity> gen_entities;
    std::map<StringUtf16, ElementDef> elem_defs;
    ElementDef* elem_def;
    int remain_artificial_scripts;
    static const int max_consecutive_artificial_scripts = 32;


    void toupper(StringUtf16& s)
    {
        to_upper_case_ascii(s); // Optimized for cases where ASCII is dominant
    }


    StringUtf16 get_upcased(StringUtf16 s)
    {
        toupper(s);
        return s;
    }


    bool expand_general_entity()
    {
        GenEntity& e = gen_entities[entity_name_cf];
        if (e.is_simple_text) {
          simple_text:
            text_accum += e.simple_text;
            return true;
        }

        if (!e.valid) {
            // FIXME: Should first lookup in internal subset

            const DocTypeDef::Entity* d = dtd.lookup_entity(entity_name_cf);
            if (!d)
                return false;
            e.is_simple_text = true;
            e.simple_text = d->get_replacement_text();
            e.valid = true;
            // In general, an SGML entity replacement value could be
            // either literal text (not for parsing [CDATA]) or it
            // could be text that needs to be parsed (PCDATA). When
            // the replacement text needs to be parsed, it must be
            // parsed in such a way that neither tags nor general
            // entity references can be recognized unless they are
            // entirely contained within the replacement text. This
            // applies recursively, of course.

            if (e.is_simple_text)
                goto simple_text;
        }

        ARCHON_ASSERT_1(false, "Expecting all entities to be simple for now");
        return true;
    }


    void lookup_elem_def()
    {
        ElementDef& e = elem_defs[tag_name_cf];
        if (e.valid) {
            elem_def = &e;
            return;
        }

        validate_elem_name();
        e.name = tag_name;
        e.use_special_cdata_content_handler = tag_name_cf == str_SCRIPT_cf || tag_name_cf == str_STYLE_cf;
        e.consider_content_as_script = e.use_special_cdata_content_handler && tag_name_cf == str_SCRIPT_cf;
        e.valid = true;
        elem_def = &e;

/*
        int i = dtd->lookup_elem(elem_name_cf, case_insensitive);
        if (i < 0) {
            if (!config.allow_unknown_elems)
                error("Unknown element <%1>", elem_name);
        }
        else {
            e.is_cdata = dtd->is_cdata(i);
            e.is_script = dtd->is_script(i);
        }

        e.is_cdata = dtd->get_special_contents(i);
        e.is_script = dtd->is_script(i);
        e.valid = true;

        elem_id = i;

        // Try lookup
        // If fail:
        //   check DTD
        //   If fail:
        //     Validate name
        //     IF good: take it, and ask DTD for the unknown ID.
        //
*/
    }


    void validate_elem_name()
    {
        if (!valid_name(tag_name))
            error("Invalid element name '%1'", tag_name);
    }


    void validate_attr_name()
    {
        if (!valid_name(attr.name))
            error("Invalid attribute name '%1'", attr.name);
    }


    void consume_rest_of_end_tag()
    {
        CharUtf16 c;
        for (;;) {
            if (!in.get(c)) {
                error("Unterminated end tag </%1", tag_name);
                return;
            }
            if (c == char_Greater)
                return;
            if (!is_space(c)) {
                error("Garbage in end tag </%1", tag_name);
                for (;;) {
                    if (!in.get(c))
                        break;
                    if (c == char_Greater)
                        return;
                }
            }
        }
    }


    void handle_script();

    void handle_style();


    void check_flush_text()
    {
        if (!text_accum.empty()) {
            // FIXME: Is text allowed at current position in current
            // contents model? If not, can an element with optional
            // start tag be generated? If not, check lingering exited
            // sub levels on stack, if any one of them that has not be
            // explicitely closed, allows text, add the text to the
            // closest one.
            callbacks.text(text_accum);
            text_accum.clear();
        }
    }


    // Implements method in Input::BufferEmptyListener.
    //
    // Note that this function may get called by the attached Input
    // object during any invocation of a function that extracts data
    // from it.
    void on_input_buffer_empty() ARCHON_OVERRIDE
    {
        if (!text_accum_no_flush)
            check_flush_text();
    }


    void handle_start_tag()
    {
/*
        int const elem = dtd->lookup_element(name);
        int action_args[2];
      again:
        switch (dtd->action(elem, state, action_args)) {
        case action_Push:
            stack.push_back(action_arg[0]);
            state = action_arg[1];
            goto again;
        case action_Pop:

        }
        // If current content model does not allow this element:
        //   can_introduce_new_elem_with_optional_start_tag
        //   can_close_current_elem
        stack.push_back(def);
        if (!def)
            unknown_names.push_back(name);
*/

        if (config.enable_meta_charenc_switching)
            honour_charenc_swithing();

        // FIXME: In fact any element whose content model is CDATA
        // (based on DTD info) should be handled in a special way
        // where the terminating tag is directly tracked down.
        if (tag_name_cf == str_SCRIPT_cf) {
            handle_script();
        }
        else if (tag_name_cf == str_STYLE_cf) {
            handle_style();
        }
        else {
            callbacks.elem_begin(tag_name, attribs);
        }
    }


    void handle_end_tag()
    {
        // FIXME: Unwind stack closing elements
        callbacks.elem_end(tag_name);
    }


    // Special hook for <META HTTP-EQUIV="Content-Type"
    // CONTENT="text/html; charset=...">
    void honour_charenc_swithing()
    {
        if (tag_name_cf != str_META_cf)
            return;

        // Find the relevant attributes
        typedef std::vector<Attr>::const_iterator iter;
        iter e = attribs.end();
        bool found = false;
        StringUtf16 content;
        for (iter i = attribs.begin(); i != e; ++i) {
            StringUtf16 s = i->name;
            case_fold(s);
            if (s == str_HTTP_EQUIV_cf) {
                s = i->value;
                case_fold(s);
                trim(s);
                if (s != str_CONTENT_TYPE_cf)
                    return;
                found = true;
                if (!content.empty())
                    break;
            }
            else if (s == str_CONTENT_cf) {
                content = i->value;
                if (content.empty())
                    return;
                if (found)
                    break;
            }
        }
        if (!found || content.empty())
            return;

        // Check for 'charset=...' in 'content' value
        toupper(content);
        StringUtf16::size_type i = content.find(str_CHARSET_uc);
        if (i == StringUtf16::npos)
            return;
        i += str_CHARSET_uc.size();
        StringUtf16::size_type n = content.size();
        while (i < n && is_space(content[i]))
            ++i;
        if (n <= i || content[i] != char_Equals)
            return;
        ++i;
        while (i < n && is_space(content[i]))
            ++i;
        StringUtf16::size_type b = i;
        while (i < n && !is_space(content[i]) && content[i] != char_Semicolon)
            ++i;
        std::string charenc;
        if (i <= b || !utf16_to_narrow_port(content.substr(b, i-b), charenc))
            return;
        try {
            UniquePtr<TranscoderToUtf16> tmp(get_transcoder_to_utf16(charenc).release());
            in.switch_transcoder(tmp);
        }
        catch (TranscoderNotAvailableException &) {}
    }


    void trim(StringUtf16& s)
    {
        // A heuristic check that assumes that most strings are
        // already trimmed
        typedef StringUtf16::iterator iter;
        iter i = s.begin(), j = s.end();
        if (i == j)
            return;
        --j;
        if (!is_space(*i)) {
            if (i == j || !is_space(*j))
                return;
            goto trim_end;
        }

        // Discard leading spaces
        for (;;) {
            if (i == j) { // Check of "all blank"
                s.clear();
                return;
            }
            if (!is_space(*++i))
                break;
        }

        // Discard trailing spaces
        if (is_space(*j)) {
          trim_end:
            for (;;) {
                if (!is_space(*--j))
                    break;
            }
        }

        s = StringUtf16(i, j+1);
    }


    bool decode_digit(CharUtf16 c, int& d)
    {
        TraitsU16::int_type i = TraitsU16::to_int_type(c);
        if (0x39 < i || i < 0x30)
            return false;
        d = i - 0x30;
        return true;
    }


    bool decode_xdigit(CharUtf16 c, int& d)
    {
        TraitsU16::int_type i = TraitsU16::to_int_type(c);
        if (0x41 <= i) {
            if (0x61 <= i) {
                if (0x66 < i)
                    return false;
                d = 10 + (i - 0x61);
                return true;
            }
            if (0x46 < i)
                return false;
            d = 10 + (i - 0x41);
            return true;
        }
        if (i < 0x30 || 0x39 < i)
            return false;
        d = i - 0x30;
        return true;
    }
};



class SgmlParser::ElemCdataInputStream: public BasicInputStream<CharUtf16> {
public:
    ElemCdataInputStream(SgmlParser& p): parser(p), term_seen(false) {}

    virtual std::size_t read(CharUtf16* b, std::size_t n)
    {
        if (term_seen)
            return 0;
        std::size_t m;
        Input& in = parser.in;
        CharUtf16 c;
        StringUtf16 p, p_cf;
        for (;;) {
            m = in.read_until(b, n, char_Less);
            if (0 < m || n == 0)
                return m;
            if (!in.get(c))
                return 0;
            if (!in.get(c))
                goto bail;
            if (c != char_Solidus) {
                in.unget();
                goto bail;
            }
            extract_tag_name(p);
            p_cf = p;
            parser.case_fold(p_cf);
            if (p_cf != parser.tag_name_cf) {
                parser.error("Invalid closing tag for element <%1> with CDATA content",
                             parser.tag_name);
                in.inject(char_Solidus + p);
                goto bail;
            }
            term_seen = true;
            return 0;

          bail:
            *b++ = char_Less;
            --n;
        }
    }

    void finalize()
    {
        discard_rest();
        if (term_seen)
            parser.consume_rest_of_end_tag();
    }

private:
    SgmlParser& parser;
    bool term_seen;

    // Extract characters from the input until a space, a '>', or the
    // end of input is seen.
    //
    // Be sure to never read beyond the end of the closing delimiter,
    // because in that case we may have extracted data beyond the end
    // tag, and such data may need to be reinterpreted by a new
    // transcoder. For further details, see the notes in
    // Input::inject().
    void extract_tag_name(StringUtf16& n)
    {
        Input& in = parser.in;
        CharUtf16 c;
        for (;;) {
            if (!in.get(c))
                return;
            if (c == char_Greater || is_space(c)) {
                in.unget();
                return;
            }
            n += c;
        }
    }
};



class DocWriterImpl: public DocWriter {
public:
    virtual void write(const StringUtf16& s)
    {
        data += s;
    }

    StringUtf16 data;
};



inline void SgmlParser::handle_script()
{
    bool inhibit = false;
    if (in.next_char_is_real()) {
        remain_artificial_scripts = max_consecutive_artificial_scripts;
    }
    else {
        if (remain_artificial_scripts == 0) {
            error("Too many consecutive scripts in artificial input, execution was suppressed");
            inhibit = true;
        }
        else {
            --remain_artificial_scripts;
        }
    }

    ElemCdataInputStream inline_source(*this);
    DocWriterImpl doc_writer;
    if (!inhibit)
        callbacks.script(attribs, inline_source, doc_writer);
    inline_source.finalize();
    in.inject(doc_writer.data);
}



inline void SgmlParser::handle_style()
{
    ElemCdataInputStream inline_source(*this);
    callbacks.style(attribs, inline_source);
    inline_source.finalize();
}



void SgmlParser::parse_sgml()
{
    CharUtf16 ch;
    StringUtf16 s;
    bool in_prolog = true; // Prolog extends up to the first start or end tag.
    bool doctype_seen = false; // For now we allow only one. With CONCUR option, SGML allows multiple doctypes.
    bool is_start_tag;
    bool ent_ref_in_attr;
    CharUtf16 quote_char;
    bool no_doctype_warned = false;
    std::vector<MarkedSection> marked_sections;

  do_PCDATA:
    if (!in.get(ch)) {
        check_flush_text();
        while (!marked_sections.empty()) {
            error("Unterminated marked section"); // FIXME: Include information about start of marked section
            marked_sections.pop_back();
        }
        return;
    }
    if (ch == char_Ampersand) {
        ent_ref_in_attr = false;
        goto check_GenEntRef;
    }
    if (ch == char_Less)
        goto check_Tag;
    if (ch == char_RightSqBracket)
        goto check_RightSqBracket;
  do_PCDATA_with_char:
    text_accum += ch;
    goto do_PCDATA;

  check_RightSqBracket:
    {
        CharUtf16 ch2;
        if (in.peek() != char_RightSqBracket)
            goto do_PCDATA_with_char;
      ms_again:
        in.drop();
        ch2 = in.peek();
        if (ch2 != char_Greater) {
            text_accum += char_RightSqBracket;
            if (ch2 != char_RightSqBracket)
                goto do_PCDATA_with_char;
            goto ms_again;
        }
        in.drop();
        // Have `]]>`
        if (!marked_sections.empty()) {
            marked_sections.pop_back();
            goto do_PCDATA;
        }
        error_adj(-3, "Marked section end `]]>` not in marked section declaration");
        // FIXME: `onsgmls` does not output the misplaced `]]>`, but
        // HTML browsers do. It is probably because the HTML browsers
        // do not recognize marked sections at all. Check up on HTML5
        // parser specification.
        text_accum += char_RightSqBracket;
        text_accum += char_RightSqBracket;
        ch = char_Greater;
        goto do_PCDATA_with_char;
    }

  check_GenEntRef:
    {
        if (!in.get(ch)) {
          bail:
            text_accum += char_Ampersand;
          ent_exit:
            if (ent_ref_in_attr)
                goto do_QuotedAttrValue;
            goto do_PCDATA;
        }
        if (ch == char_HashMark)
            goto char_ref;
        if (!valid_first_name_char_strict(ch)) {
            in.unget();
            goto bail;
        }
        entity_name = ch;
        for (;;) {
            if (!in.get(ch)) {
                ch = CharUtf16(); // So we can reliably know if a ';' was found
                break;
            }
            if (ch == char_Semicolon)
                break;
            if(!valid_second_name_char_strict(ch)) {
                in.unget();
                break;
            }
            entity_name += ch;
        }
        entity_name_cf = entity_name;
        case_fold_entity(entity_name_cf);
        // FIXME: It is probably not ok if a tag is begun, but not
        // ended inside the replacement text. How about elements, do
        // they need to be closed inside the replacement text? An
        // consider the impact of omitted tags also.
        if (expand_general_entity())
            goto ent_exit;
        error("Undefined entity &%1;;", entity_name);
        text_accum += char_Ampersand;
        text_accum += entity_name;
        if (ch == char_Semicolon)
            text_accum += char_Semicolon;
        goto ent_exit;
      char_ref:
        // FIXME: SGML allows for a character function name to occur
        // here.
        //
        // SGML's reference concrete syntax defines these:
        //
        //       FUNCTION  RE           13
        //                 RS           10
        //                 SPACE        32
        //                 TAB           9
        //
        // However, `onsgmls` changes it to (probably for UNIX
        // compatibility):
        //
        //       FUNCTION  RE           10    \n
        //                 RS           --    (empty, nothing)
        //                 SPACE        32
        //                 TAB           9
        //
        // See also http://xml.coverpages.org//sgmlsyn/sgmlsyn.htm#P62
        // and http://www.is-thought.co.uk/book/sgml-6.htm#char-ref.
        CharUtf16 c;
        if (in.get(c)) {
            int d;
            if (decode_digit(c, d)) {
                UIntFast32 ucs_code_point = d;
                bool overflow = false;
                for (;;) {
                    if (!in.get(c) || c == char_Semicolon)
                        break;
                    if(!decode_digit(c, d)) {
                        in.unget();
                        break;
                    }
                    if (ucs_code_point <= 0xFFFFFFF0/10) {
                        ucs_code_point *= 10;
                        ucs_code_point += d;
                        continue;
                    }
                    if (0xFFFFFFFF/10 < ucs_code_point)
                        overflow = true;
                    ucs_code_point *= 10;
                    if (0xFFFFFFFF-d < ucs_code_point)
                        overflow = true;
                    ucs_code_point += d;
                }
                if (overflow || !utf16_append_ucs_char(text_accum, ucs_code_point)) {
                    error("Character reference with invalid code point");
                    text_accum += char_Replacement;
                }
                goto ent_exit;
            }
            if (c == char_SmallX || config.case_insensitive && c == char_CapitalX) {
                // Hex version
                if (in.get(c)) {
                    if (decode_xdigit(c, d)) {
                        UIntFast32 ucs_code_point = d;
                        int num_digits = 1;
                        for (;;) {
                            if (!in.get(c) || c == char_Semicolon)
                                break;
                            if(!decode_xdigit(c, d)) {
                                in.unget();
                                break;
                            }
                            ucs_code_point <<= 4;
                            ucs_code_point |= d;
                            ++num_digits;
                        }
                        if (8 < num_digits || !utf16_append_ucs_char(text_accum,
                                                                     ucs_code_point)) {
                            error("Hexadecimal character reference with invalid code point");
                            text_accum += char_Replacement;
                        }
                        goto ent_exit;
                    }
                    in.unget();
                }
                error("Invalid character reference");
                text_accum += char_Ampersand;
                text_accum += char_HashMark;
                text_accum += char_SmallX;
                goto ent_exit;
            }
            in.unget();
        }
        error("Invalid character reference");
        text_accum += char_Ampersand;
        text_accum += char_HashMark;
        goto ent_exit;
    }

  check_Tag:
    is_start_tag = true;
    if (!in.get(ch))
        goto bail_Tag;
    if (ch == char_Solidus) {
        if (!in.get(ch)) {
            error("Unterminated end tag");
            goto do_PCDATA;
        }
        if (ch == char_Greater) {
            // FIXME: Allow it if SGML option SHORTTAGS is YES.
            error("Empty end tag");
            goto do_PCDATA;
        }
        is_start_tag = false;
    }
    else {
        // FIXME: Recognize the empty start tag '<>' here if SGML
        // option SHORTTAGS is YES.
        if (ch == char_Exclamation) {
            check_flush_text();
            if (!in.get(ch)) {
                error("Unterminated declaration");
                goto do_PCDATA;
            }
            if (ch != char_Hyphen) {
                if (ch == char_LeftSqBracket)
                    goto do_MarkedSectionDecl;
                if (ch != char_Greater) {
                    // FIXME: SGML allows `<!USEMAP` and `<!USELINK` here
                    in.unget();
                    goto do_Declaration;
                }
                goto do_PCDATA; // An empty declaration is a valid comment declaration
            }
            if (in.peek() != char_Hyphen) {
                in.inject(char_Hyphen);
                goto do_Declaration;
            }
            in.drop();
            goto do_CommentDecl;
        }
        if (ch == char_Question) {
            check_flush_text();
            goto do_ProcInstr;
        }
        if (!valid_first_name_char_strict(ch)) {
            // The first character of an element name must always be one
            // of the Latin letters included the ASCII character
            // set. This is regardless of whether XHTML compatibility
            // mode is enabled.
            in.unget();
          bail_Tag:
            text_accum += char_Less;
            goto do_PCDATA;
        }
    }

    // FIXME: If err_rep is not silent, record the tag position as two (or three - if (!is_start_tag)) minus the current (ask Input for it)
    check_flush_text();
    tag_name = ch;

    // Get the rest of the tag name
    for (;;) {
        if (!in.get(ch))
            break;
        if (is_space(ch))
            break;
        if(ch == char_Greater) {
            in.unget();
            if (config.html5_compat && is_start_tag) {
                std::size_t i = tag_name.size() - 1;
                if (tag_name[i] == char_Solidus)
                    tag_name.resize(i);
            }
            break;
        };
        tag_name += ch;
    }
    tag_name_cf = tag_name;
    case_fold(tag_name_cf);
    if (!doctype_seen) {
        if (!no_doctype_warned) {
            error("No document type declaration"); // FIXME: Should also happen if document is empty
            no_doctype_warned = true;
        }
    }
    in_prolog = false;
    lookup_elem_def();
    if (is_start_tag) {
        attribs.clear();
        goto do_Attribs;
    }
    consume_rest_of_end_tag();
    handle_end_tag();
    goto do_PCDATA;

  do_Attribs:
    // FIXME: Before returning to `do_PCDATA`, check the content type,
    // if it is CDATA or RCDATA, call special tight consumer loops
    // instead. Special handling is also needed for EMPTY content
    // type. `RCDATA` content is terminated only by `</x` where `x` is
    // a valid first char of a name.
    if (!in.get(ch)) {
        error("Unterminated start tag <%1", tag_name);
        ch = char_Greater;
    }
    if (ch == char_Greater) {
        handle_start_tag();
        goto do_PCDATA;
    }
    if (is_space(ch))
        goto do_Attribs;

    attr.name = ch;
    attr.value.clear();

    // Get rest of attribute name
    // FIXME: If err_rep is not silent, record the attribute name position as one minus the current (ask Input for it)
    for (;;) {
        if (!in.get(ch)) {
            ch = CharUtf16();
            goto got_attr_name;
        }
        if (ch == char_Equals)
            goto got_attr_name;
        if (is_space(ch)) {
            // Skip trailing space
            for (;;) {
                if (!in.get(ch)) {
                    ch = CharUtf16();
                    goto got_attr_name;
                }
                if (ch == char_Equals)
                    goto got_attr_name;
                if (!is_space(ch)) {
                    in.unget();
                    goto got_attr_name;
                }
            }
        }
        if(ch == char_Greater) {
            in.unget();
            if (config.html5_compat) { // FIXME: Verify that this makes sense
                std::size_t i = attr.name.size() - 1;
                if (attr.name[i] == char_Solidus) {
                    if (attr.name.size() == 1)
                        goto do_Attribs;
                    attr.name.resize(i);
                }
            }
            goto got_attr_name;
        };
        attr.name += ch;
    }

  got_attr_name:
    validate_attr_name(); // FIXME: Actually a value if not followed by `=`, right?
    if (ch != char_Equals)
        goto save_attr;

    // Get attribute value
    for (;;) {
        if (!in.get(ch))
            goto save_attr;
        if (ch == char_DoubleQuote) {
            quote_char = char_DoubleQuote;
            goto do_QuotedAttrValue;
        }
        if (ch == char_SingleQuote) {
            quote_char = char_SingleQuote;
            goto do_QuotedAttrValue;
        }
        if(ch == char_Greater) {
            in.unget();
            goto save_attr;
        };
        if (!is_space(ch)) {
            for (;;) {
                attr.value += ch;
                if (!in.get(ch))
                    goto save_attr;
                if (is_space(ch))
                    goto save_attr;
                if (ch == char_Greater) {
                    in.unget();
                    if (config.html5_compat) { // FIXME: Verify that this makes sense
                        std::size_t i = attr.value.size() - 1;
                        if (attr.value[i] == char_Solidus)
                            attr.value.resize(i);
                    }
                    goto save_attr;
                }
            }
        }
    }

  do_QuotedAttrValue:
    text_accum_no_flush = true;
    for (;;) {
        if (!in.get(ch)) {
            error("Unterminated quoted attribute value for %1=", attr.name);
            goto quot_exit;
        }
        if (ch == char_Ampersand) {
            ent_ref_in_attr = true;
            goto check_GenEntRef;
        }
        if (ch == quote_char) {
          quot_exit:
            // Force copy. We want to keep using the text_accum buffer
            // for text accumulation, and we want the attribute value
            // allocation to match the string size.
            //
            // FIXME: Introduce a new TextAccum class that stores
            // bytes contiguously, and then report all text as
            // pointers into this buffer. In case of a start tag, all
            // attribute values must be placed in the buffer
            // simultaneously.
            attr.value.assign(text_accum.begin(), text_accum.end());
            text_accum.clear();
            break;
        }
        text_accum += ch;
    }
    text_accum_no_flush = false;

  save_attr:
    attribs.push_back(attr);
    goto do_Attribs;

  do_CommentDecl:
    {
        // FIXME: Consider a faster alternative loop when comments are not reported
        bool garbage_seen = false;
        s.clear();
        for (;;) {
            if (!in.get(ch)) {
                error("Unterminated comment");
                goto report;
            }
            if (ch == char_Hyphen && in.peek() == char_Hyphen) {
                in.drop();
                break;
            }
            s += ch;
        }
        for (;;) {
            skip_decl_space(); // FIXME: Parameter entity references are not allowed here!
            if (!in.get(ch)) {
                error("Unterminated comment declaration");
                goto report;
            }
            if (ch == char_Greater) {
              report:
                if (config.report_comments)
                    callbacks.comment(s);
                goto do_PCDATA;
            }
            if (!garbage_seen) {
                error("Garbage in comment declaration");
                garbage_seen = true;
            }
        }
    }

  do_MarkedSectionDecl:
    //
    // The standard appears to require that there is an initial
    // parameter separator in keywords, but `onsgmls` does no require
    // it, nor is it allowed by XML. I think I should follow
    // `onsgmls`.
    //
    // Contents of IGNORE is parsed only for terminating `]]>` and for
    // matching pairs of `<![` and `]]>`, and the entire contents is
    // thrown away.
    //
    // Contents of CDATA is parsed only for the terminating
    // `]]>`, and the entire contents is added to
    // `text_accum`.
    //
    // Contents of RCDATA is parsed only for the terminating
    // `]]>` and embedded general entity references, and the entire contents is added to
    // `text_accum`.
    //
    // Need to keep track of nesting level of marked INCLUDE/TEMP
    // sections.
    //
    // // Value specifies precedence
    // enum MarkedSectionType {
    //    3   marked_IGNORE  = 3,  // IGNORE
    //    2   marked_CDATA   = 2,  // CDATA
    //    1   marked_RCDATA  = 1,  // RCDATA
    //    0   marked_INCLUDE = 0   // INCLUDE, TEMP
    // };
    //
    // MarkedSectionType type = marked_INCLUDE;
    // for each keyword {
    //     MarkedSectionType t = type_of(keyword);
    //     if (t > type)
    //         type = t;
    // }
    // if (type == type_IGNORE)
    //     goto do_MarkedSectionIGNORE;
    // if (type == type_CDATA)
    //     goto do_MarkedSectionCDATA;
    // if (type == type_RCDATA)
    //     goto do_MarkedSectionRCDATA;
    // marked_sections.push_back(...);
    // goto do_PCDATA;
    //
    // KeepLink: http://www.w3.org/TR/NOTE-sgml-xml-971215
    // KeepLink: http://xml.coverpages.org/sgml.html
    throw std::runtime_error("Marked sections are not yet implemented");

  do_ProcInstr:
    s.clear();
    for (;;) {
        if (!in.get(ch))
            error("Unterminated processing instruction");
        if (ch == char_Greater)
            break;
        s += ch;
    }
    callbacks.proc_instr(s);
    goto do_PCDATA;

  do_Declaration:
    if (eat_decl_word(s)) {
        StringUtf16 s_cf = s;
        case_fold(s_cf);
        if (s_cf == str_DOCTYPE_cf)
            goto do_Doctype;
        error("Illegal declaration <!%1 outside internal DTD subset", s);
    }
    else {
        ARCHON_ASSERT(!in.eoi() && in.peek() != char_Greater);
        error("Garbage at start of declaration");
    }
    goto do_SkipRestOfUnknownDecl;

  do_Doctype:
    {
        if (!in_prolog) {
            error("Document type declaration after end of prolog");
            goto do_SkipRestOfUnknownDecl;
        }
        if (doctype_seen) {
            error("Multiple document type declaration in prolog");
            goto do_SkipRestOfUnknownDecl;
        }
        doctype_seen = true;
        skip_decl_space();
        StringUtf16 doctype_name, public_ident, system_ident;
        if (!eat_decl_word(doctype_name)) {
            error("Missing document type name in document type declaration");
            goto do_SkipRestOfUnknownDecl;
        }
        if (!valid_name(doctype_name))
            error("Invalid document type name '%1'", doctype_name);
        bool has_preceding_space = skip_decl_space();
        if (eat_decl_word(s)) {
            case_fold(s);
            bool fail, space_after;
            if (eat_ext_ident(s, fail, public_ident, system_ident, space_after)) {
                if (fail)
                    goto do_SkipRestOfUnknownDecl;
                has_preceding_space = space_after;
            }
            else {
                error("Unrecognized keyword '%1' in document type declaration", s);
                goto do_SkipRestOfUnknownDecl;
            }
        }

        callbacks.doctype_begin(doctype_name, public_ident, system_ident);

        if (in.peek() == char_LeftSqBracket) {
            if (!has_preceding_space)
                error("No space before 'declaration subset open' delimiter");
            in.drop();
            bool internal_subset = true;
            parse_dtd(internal_subset);
        }

        callbacks.doctype_end();

        if (!in.get(ch)) {
            error("Unterminated document type declaration");
            goto do_PCDATA;
        }
        if (ch != char_Greater) {
            error("Unexpected character in document type declaration");
            goto do_SkipRestOfUnknownDecl;
        }
        goto do_PCDATA;
    }

  do_SkipRestOfUnknownDecl:
    for (;;) {
        skip_decl_space(true);
        if (!in.get(ch) || ch == char_Greater)
            goto do_PCDATA;
        in.unget();
        if (!eat_decl_word(s) && !eat_quoted_str(s, true))
            in.drop();
    }
}



void DtdParser::parse_dtd(bool internal_subset)
{
    CharUtf16 c;
    StringUtf16 s, s_cf;
    std::vector<MarkedSection> marked_sections;

  do_NextDecl:
    if (!in.get(c)) {
        while (!marked_sections.empty()) {
            error("Unterminated marked section"); // FIXME: Include information about start of marked section
            marked_sections.pop_back();
        }
        if (internal_subset)
            error("Unterminated internal DTD subset");
        return;
    }
    if (is_space(c))
        goto do_NextDecl;
    if (c == char_Less)
        goto check_LessThan;
    if (c == char_Percent)
        goto check_Percent;
    if (c == char_RightSqBracket)
        goto check_RightSqBracket;
    goto do_BadChar;

  check_LessThan:
    // Have `<`
    c = in.peek();
    if (c == char_Exclamation) {
        in.drop();
        // Have `<!`
        c = in.peek();
        if (c == char_Hyphen) {
            in.drop();
            // Have `<!-`
            if (in.peek() == char_Hyphen) {
                in.drop();
                // Have `<!--`
                goto do_CommentDecl;
            }
            error("Characters `<!-` not allowed in declaration subset unless they start a comment declaration"); // FIXME: Adjust indicated position (-3)
            goto do_NextDecl;
        }
        else if (valid_first_name_char_strict(c)) {
            // Have '<!' followed by valid first name character
            goto do_Declaration;
        }
        if (c == char_LeftSqBracket) {
            // Have '<!['
            in.drop();
            goto do_MarkedSectionDecl;
        }
        error("Characters `<!` not allowed in declaration subset unless they start a declaration"); // FIXME: Adjust indicated position (-2)
        goto do_NextDecl;
    }
    else if (c == char_Question) {
        in.drop();
        // Have `<?`
        goto do_ProcInstr;
    }
    error("Character `<` not allowed in declaration subset unless it starts a declaration or a processing instruction"); // FIXME: Adjust indicated position (-1)
    goto do_NextDecl;

  check_RightSqBracket:
    // Have `]`
    c = in.peek();
    if (c == char_RightSqBracket) {
      rsb_Again:
        in.drop();
        // Have `]]`
        c = in.peek();
        if (c != char_Greater) {
            error_adj(-2, "Character `]` not allowed here except when forming the marked-section-end token `]]>`");
            if (c == char_RightSqBracket)
                goto rsb_Again;
            c = char_RightSqBracket;
            goto rsb_BadChar;
        }
        in.drop();
        // Have `]]>`
        if (!marked_sections.empty()) {
            marked_sections.pop_back();
        }
        else {
            error_adj(-3, "Marked section end `]]>` not in marked section declaration");
            // Ignore the `]]>` token in lenient mode
        }
        goto do_NextDecl;
    }
    if (internal_subset) {
        // End of internal declaration subset
        while (!marked_sections.empty()) {
            error("Unterminated marked section"); // FIXME: Include information about start of marked section
            marked_sections.pop_back();
        }
        return;
    }
  rsb_BadChar:
    error_adj(-1, "Character `]` not allowed here except when forming the marked-section-end token `]]>`");
    goto do_NextDecl;

  do_Declaration:
    if (eat_decl_word(s)) {
        s_cf = s;
        case_fold(s_cf);
        if (s_cf == str_ENTITY_cf)
            goto do_Entity;
        if (s_cf == str_NOTATION_cf)
            goto do_Notation;
        if (s_cf == str_ELEMENT_cf)
            goto do_Element;
        if (s_cf == str_ATTLIST_cf)
            goto do_AttList;
        error("Unknown declaration type `<!%1`", s);
    }
    else {
        error("Illegal declaration");
    }
    goto do_SkipRestOfUnknownDecl;

  do_Entity:
    {
        bool is_param_ent = false;
        skip_decl_space();
        bool have_space = true;
        if (in.peek() == char_Percent) {
            is_param_ent = true;
            in.drop();
            if (!skip_decl_space())
                have_space = false;
        }
        if (!eat_decl_word(entity_name)) {
            error("Entity name is missing");
            goto do_SkipRestOfUnknownDecl;
        }
        if (!have_space)
            error("Need space before entity name");
        have_space = skip_decl_space();
        if (!valid_name(entity_name))
            error("Invalid entity name '%1'", entity_name);

        entity_name_cf = entity_name;
        case_fold_entity(entity_name_cf);
        // FIXME: May not be a parameter entity!
        Entity* p = &param_entities[entity_name_cf];
        if (p->is_valid) {
            if (warn_on_multiple_entity_decls)
                error("Multiple definitions for parameter entity `%1`", entity_name);
            p = 0;
        }

        if (eat_decl_word(s)) {
            if (!have_space)
                error("Need space after entity name");
            case_fold(s);
            bool fail;
            StringUtf16 public_ident, system_ident;
            if (eat_ext_ident(s, fail, public_ident, system_ident, have_space)) {
                if (fail)
                    goto do_SkipRestOfUnknownDecl;
                // FIXME: May be followed by entity type
                if (p) {
                    p->is_external = true;
                    p->public_ident = public_ident;
                    p->system_ident = system_ident;
                    p->is_valid = true;
                }
                end_decl("entity");
                goto do_NextDecl;
            }
            error("Entity has unknown form");
            goto do_SkipRestOfUnknownDecl;
//            have_space = skip_decl_space();
        }

        // FIXME: Must call a different function (not
        // eat_quoted_str()) that expands parameter entity and
        // character references.
        static_cast<void>(is_param_ent);
        throw std::runtime_error("Entity");
/*
        if (eat_parameter_literal(s)) {
            if (!have_space)
                error("Need space before entity replacement text");
            if (p) {
                // FIXME: Handled bracketed text by adding the
                // bracketing elements before storing the value (all
                // are of type `type_Regular`):
                //
                //   STARTTAG ->  <...>
                //   ENDTAG   ->  </...>
                //   MS       ->  <![...]]>
                //   MD       ->  <!...>
                //
                p->replacement_text = s;
                p->is_valid = true;
            }
            end_decl("entity");
            goto do_NextDecl;
        }
*/

        error("Missing entity replacement text");
        goto do_SkipRestOfUnknownDecl;
    }


    // FIXME:
    //
    // Check for keyword (XML does not allow any)
    //   No keyword -> next token is parameter literal which is to be interpreted as PCDATA when expanded
    //   'CDATA', 'SDATA', 'PI' -> next token is parameter literal
    //   "STARTTAG" | "ENDTAG" | "MS" | "MD" -> next token is parameter literal
    //   'PUBLIC' | 'SYSTEM'
    // `CDATA` and `SDATA` are not allowed in parameter entity declarations.
    error("Entity");
    throw std::runtime_error("Entity");

  do_Notation:
    error("Notation");
    throw std::runtime_error("Notation");

  do_Element:
    error("Element");
    throw std::runtime_error("Element");

  do_AttList:
    error("AttList");
    throw std::runtime_error("AttList");

  do_CommentDecl:
    for (;;) {
        if (!in.get(c)) {
            error("Unterminated comment");
            goto do_NextDecl;
        }
        if (c == char_Hyphen && in.peek() == char_Hyphen) {
            in.drop();
            break;
        }
    }
    end_decl("comment"); // FIXME: Parameter entity references are not allowed here
    goto do_NextDecl;

  check_Percent:
    // FIXME: See http://www.w3.org/TR/2008/REC-xml-20081126/#entproc
    // FIXME: See http://www.w3.org/TR/2008/REC-xml-20081126/#intern-replacement
    if (ARCHON_LIKELY(in.get(c))) {
        if (ARCHON_LIKELY(valid_first_name_char_strict(c))) {
            entity_name = c;
            bool allow_proc_instr = true;
            eat_rest_of_param_entity_ref(allow_proc_instr);
            goto do_NextDecl;
        }
        in.unget();
    }
    error_adj(-1, "Character `%` not allowed here except when forming a parameter entity reference");
    goto do_NextDecl;

  do_MarkedSectionDecl:
    error("Marked section");
    throw std::runtime_error("Marked section");

  do_ProcInstr:
    s.clear();
    for (;;) {
        if (!in.get(c))
            error("Unterminated processing instruction");
        if (c == char_Greater)
            break;
        s += c;
    }
    callbacks.proc_instr(s);
    goto do_NextDecl;

  do_BadChar:
    error_adj(-1, "Character `%1` not allowed here", c);
    goto do_NextDecl;

  do_SkipRestOfUnknownDecl:
    for (;;) {
        skip_decl_space(true);
        if (!in.get(c) || c == char_Greater)
            goto do_NextDecl;
        in.unget();
        if (!eat_decl_word(s) && !eat_quoted_str(s, true))
            in.drop();
    }
}



// Doctype sets initial content model to '(root_name)'


/*
{
    "HTML", false, false, "HEAD, BODY", {
        { "" }
    }
}
*/


/*
class HtmlDtd {
public:
    int action_start_tag(int elem, int state)
    {
        switch(state) {
            case state_Inline:
                if (is_inline(elem))
                    return action_Push:
        }
    }
};
*/



struct CharEnt {
    const char* name;
    int ucs_code_point;
};

CharEnt html_char_entities[] = {
    // ---------------------------------------------
    // Latin-1 characters (file: DTD/xhtml-lat1.ent)
    // ---------------------------------------------
    { "nbsp",      160 }, // no-break space = non-breaking space, U+00A0 ISOnum
    { "iexcl",     161 }, // inverted exclamation mark, U+00A1 ISOnum
    { "cent",      162 }, // cent sign, U+00A2 ISOnum
    { "pound",     163 }, // pound sign, U+00A3 ISOnum
    { "curren",    164 }, // currency sign, U+00A4 ISOnum
    { "yen",       165 }, // yen sign = yuan sign, U+00A5 ISOnum
    { "brvbar",    166 }, // broken bar = broken vertical bar, U+00A6 ISOnum
    { "sect",      167 }, // section sign, U+00A7 ISOnum
    { "uml",       168 }, // diaeresis = spacing diaeresis, U+00A8 ISOdia
    { "copy",      169 }, // copyright sign, U+00A9 ISOnum
    { "ordf",      170 }, // feminine ordinal indicator, U+00AA ISOnum
    { "laquo",     171 }, // left-pointing double angle quotation mark = left pointing guillemet, U+00AB ISOnum
    { "not",       172 }, // not sign = angled dash, U+00AC ISOnum
    { "shy",       173 }, // soft hyphen = discretionary hyphen, U+00AD ISOnum
    { "reg",       174 }, // registered sign = registered trade mark sign, U+00AE ISOnum
    { "macr",      175 }, // macron = spacing macron = overline = APL overbar, U+00AF ISOdia
    { "deg",       176 }, // degree sign, U+00B0 ISOnum
    { "plusmn",    177 }, // plus-minus sign = plus-or-minus sign, U+00B1 ISOnum
    { "sup2",      178 }, // superscript two = superscript digit two = squared, U+00B2 ISOnum
    { "sup3",      179 }, // superscript three = superscript digit three = cubed, U+00B3 ISOnum
    { "acute",     180 }, // acute accent = spacing acute, U+00B4 ISOdia
    { "micro",     181 }, // micro sign, U+00B5 ISOnum
    { "para",      182 }, // pilcrow sign = paragraph sign, U+00B6 ISOnum
    { "middot",    183 }, // middle dot = Georgian comma = Greek middle dot, U+00B7 ISOnum
    { "cedil",     184 }, // cedilla = spacing cedilla, U+00B8 ISOdia
    { "sup1",      185 }, // superscript one = superscript digit one, U+00B9 ISOnum
    { "ordm",      186 }, // masculine ordinal indicator, U+00BA ISOnum
    { "raquo",     187 }, // right-pointing double angle quotation mark = right pointing guillemet, U+00BB ISOnum
    { "frac14",    188 }, // vulgar fraction one quarter = fraction one quarter, U+00BC ISOnum
    { "frac12",    189 }, // vulgar fraction one half = fraction one half, U+00BD ISOnum
    { "frac34",    190 }, // vulgar fraction three quarters = fraction three quarters, U+00BE ISOnum
    { "iquest",    191 }, // inverted question mark = turned question mark, U+00BF ISOnum
    { "Agrave",    192 }, // latin capital letter A with grave = latin capital letter A grave, U+00C0 ISOlat1
    { "Aacute",    193 }, // latin capital letter A with acute, U+00C1 ISOlat1
    { "Acirc",     194 }, // latin capital letter A with circumflex, U+00C2 ISOlat1
    { "Atilde",    195 }, // latin capital letter A with tilde, U+00C3 ISOlat1
    { "Auml",      196 }, // latin capital letter A with diaeresis, U+00C4 ISOlat1
    { "Aring",     197 }, // latin capital letter A with ring above = latin capital letter A ring, U+00C5 ISOlat1
    { "AElig",     198 }, // latin capital letter AE = latin capital ligature AE, U+00C6 ISOlat1
    { "Ccedil",    199 }, // latin capital letter C with cedilla, U+00C7 ISOlat1
    { "Egrave",    200 }, // latin capital letter E with grave, U+00C8 ISOlat1
    { "Eacute",    201 }, // latin capital letter E with acute, U+00C9 ISOlat1
    { "Ecirc",     202 }, // latin capital letter E with circumflex, U+00CA ISOlat1
    { "Euml",      203 }, // latin capital letter E with diaeresis, U+00CB ISOlat1
    { "Igrave",    204 }, // latin capital letter I with grave, U+00CC ISOlat1
    { "Iacute",    205 }, // latin capital letter I with acute, U+00CD ISOlat1
    { "Icirc",     206 }, // latin capital letter I with circumflex, U+00CE ISOlat1
    { "Iuml",      207 }, // latin capital letter I with diaeresis, U+00CF ISOlat1
    { "ETH",       208 }, // latin capital letter ETH, U+00D0 ISOlat1
    { "Ntilde",    209 }, // latin capital letter N with tilde, U+00D1 ISOlat1
    { "Ograve",    210 }, // latin capital letter O with grave, U+00D2 ISOlat1
    { "Oacute",    211 }, // latin capital letter O with acute, U+00D3 ISOlat1
    { "Ocirc",     212 }, // latin capital letter O with circumflex, U+00D4 ISOlat1
    { "Otilde",    213 }, // latin capital letter O with tilde, U+00D5 ISOlat1
    { "Ouml",      214 }, // latin capital letter O with diaeresis, U+00D6 ISOlat1
    { "times",     215 }, // multiplication sign, U+00D7 ISOnum
    { "Oslash",    216 }, // latin capital letter O with stroke = latin capital letter O slash, U+00D8 ISOlat1
    { "Ugrave",    217 }, // latin capital letter U with grave, U+00D9 ISOlat1
    { "Uacute",    218 }, // latin capital letter U with acute, U+00DA ISOlat1
    { "Ucirc",     219 }, // latin capital letter U with circumflex, U+00DB ISOlat1
    { "Uuml",      220 }, // latin capital letter U with diaeresis, U+00DC ISOlat1
    { "Yacute",    221 }, // latin capital letter Y with acute, U+00DD ISOlat1
    { "THORN",     222 }, // latin capital letter THORN, U+00DE ISOlat1
    { "szlig",     223 }, // latin small letter sharp s = ess-zed, U+00DF ISOlat1
    { "agrave",    224 }, // latin small letter a with grave = latin small letter a grave, U+00E0 ISOlat1
    { "aacute",    225 }, // latin small letter a with acute, U+00E1 ISOlat1
    { "acirc",     226 }, // latin small letter a with circumflex, U+00E2 ISOlat1
    { "atilde",    227 }, // latin small letter a with tilde, U+00E3 ISOlat1
    { "auml",      228 }, // latin small letter a with diaeresis, U+00E4 ISOlat1
    { "aring",     229 }, // latin small letter a with ring above = latin small letter a ring, U+00E5 ISOlat1
    { "aelig",     230 }, // latin small letter ae = latin small ligature ae, U+00E6 ISOlat1
    { "ccedil",    231 }, // latin small letter c with cedilla, U+00E7 ISOlat1
    { "egrave",    232 }, // latin small letter e with grave, U+00E8 ISOlat1
    { "eacute",    233 }, // latin small letter e with acute, U+00E9 ISOlat1
    { "ecirc",     234 }, // latin small letter e with circumflex, U+00EA ISOlat1
    { "euml",      235 }, // latin small letter e with diaeresis, U+00EB ISOlat1
    { "igrave",    236 }, // latin small letter i with grave, U+00EC ISOlat1
    { "iacute",    237 }, // latin small letter i with acute, U+00ED ISOlat1
    { "icirc",     238 }, // latin small letter i with circumflex, U+00EE ISOlat1
    { "iuml",      239 }, // latin small letter i with diaeresis, U+00EF ISOlat1
    { "eth",       240 }, // latin small letter eth, U+00F0 ISOlat1
    { "ntilde",    241 }, // latin small letter n with tilde, U+00F1 ISOlat1
    { "ograve",    242 }, // latin small letter o with grave, U+00F2 ISOlat1
    { "oacute",    243 }, // latin small letter o with acute, U+00F3 ISOlat1
    { "ocirc",     244 }, // latin small letter o with circumflex, U+00F4 ISOlat1
    { "otilde",    245 }, // latin small letter o with tilde, U+00F5 ISOlat1
    { "ouml",      246 }, // latin small letter o with diaeresis, U+00F6 ISOlat1
    { "divide",    247 }, // division sign, U+00F7 ISOnum
    { "oslash",    248 }, // latin small letter o with stroke, = latin small letter o slash, U+00F8 ISOlat1
    { "ugrave",    249 }, // latin small letter u with grave, U+00F9 ISOlat1
    { "uacute",    250 }, // latin small letter u with acute, U+00FA ISOlat1
    { "ucirc",     251 }, // latin small letter u with circumflex, U+00FB ISOlat1
    { "uuml",      252 }, // latin small letter u with diaeresis, U+00FC ISOlat1
    { "yacute",    253 }, // latin small letter y with acute, U+00FD ISOlat1
    { "thorn",     254 }, // latin small letter thorn, U+00FE ISOlat1
    { "yuml",      255 }, // latin small letter y with diaeresis, U+00FF ISOlat1

    // ------------------------------------------------
    // Special characters (file: DTD/xhtml-special.ent)
    // ------------------------------------------------
    { "quot",       34 }, // quotation mark, U+0022 ISOnum
    { "amp",        38 }, // ampersand, U+0026 ISOnum
    { "lt",         60 }, // less-than sign, U+003C ISOnum
    { "gt",         62 }, // greater-than sign, U+003E ISOnum
    { "apos",       39 }, // apostrophe = APL quote, U+0027 ISOnum
    { "OElig",     338 }, // latin capital ligature OE, U+0152 ISOlat2
    { "oelig",     339 }, // latin small ligature oe, U+0153 ISOlat2
    { "Scaron",    352 }, // latin capital letter S with caron, U+0160 ISOlat2
    { "scaron",    353 }, // latin small letter s with caron, U+0161 ISOlat2
    { "Yuml",      376 }, // latin capital letter Y with diaeresis, U+0178 ISOlat2
    { "circ",      710 }, // modifier letter circumflex accent, U+02C6 ISOpub
    { "tilde",     732 }, // small tilde, U+02DC ISOdia
    { "ensp",     8194 }, // en space, U+2002 ISOpub
    { "emsp",     8195 }, // em space, U+2003 ISOpub
    { "thinsp",   8201 }, // thin space, U+2009 ISOpub
    { "zwnj",     8204 }, // zero width non-joiner, U+200C NEW RFC 2070
    { "zwj",      8205 }, // zero width joiner, U+200D NEW RFC 2070
    { "lrm",      8206 }, // left-to-right mark, U+200E NEW RFC 2070
    { "rlm",      8207 }, // right-to-left mark, U+200F NEW RFC 2070
    { "ndash",    8211 }, // en dash, U+2013 ISOpub
    { "mdash",    8212 }, // em dash, U+2014 ISOpub
    { "lsquo",    8216 }, // left single quotation mark, U+2018 ISOnum
    { "rsquo",    8217 }, // right single quotation mark, U+2019 ISOnum
    { "sbquo",    8218 }, // single low-9 quotation mark, U+201A NEW
    { "ldquo",    8220 }, // left double quotation mark, U+201C ISOnum
    { "rdquo",    8221 }, // right double quotation mark, U+201D ISOnum
    { "bdquo",    8222 }, // double low-9 quotation mark, U+201E NEW
    { "dagger",   8224 }, // dagger, U+2020 ISOpub
    { "Dagger",   8225 }, // double dagger, U+2021 ISOpub
    { "permil",   8240 }, // per mille sign, U+2030 ISOtech
    { "lsaquo",   8249 }, // single left-pointing angle quotation mark, U+2039 ISO proposed
    { "rsaquo",   8250 }, // single right-pointing angle quotation mark, U+203A ISO proposed
    { "euro",     8364 }, // euro sign, U+20AC NEW

    // ------------------------------------
    // Symbols (file: DTD/xhtml-symbol.ent)
    // ------------------------------------
    { "fnof",      402 }, // latin small letter f with hook = function = florin, U+0192 ISOtech
    { "Alpha",     913 }, // greek capital letter alpha, U+0391
    { "Beta",      914 }, // greek capital letter beta, U+0392
    { "Gamma",     915 }, // greek capital letter gamma, U+0393 ISOgrk3
    { "Delta",     916 }, // greek capital letter delta, U+0394 ISOgrk3
    { "Epsilon",   917 }, // greek capital letter epsilon, U+0395
    { "Zeta",      918 }, // greek capital letter zeta, U+0396
    { "Eta",       919 }, // greek capital letter eta, U+0397
    { "Theta",     920 }, // greek capital letter theta, U+0398 ISOgrk3
    { "Iota",      921 }, // greek capital letter iota, U+0399
    { "Kappa",     922 }, // greek capital letter kappa, U+039A
    { "Lambda",    923 }, // greek capital letter lamda, U+039B ISOgrk3
    { "Mu",        924 }, // greek capital letter mu, U+039C
    { "Nu",        925 }, // greek capital letter nu, U+039D
    { "Xi",        926 }, // greek capital letter xi, U+039E ISOgrk3
    { "Omicron",   927 }, // greek capital letter omicron, U+039F
    { "Pi",        928 }, // greek capital letter pi, U+03A0 ISOgrk3
    { "Rho",       929 }, // greek capital letter rho, U+03A1
    { "Sigma",     931 }, // greek capital letter sigma, U+03A3 ISOgrk3
    { "Tau",       932 }, // greek capital letter tau, U+03A4
    { "Upsilon",   933 }, // greek capital letter upsilon, U+03A5 ISOgrk3
    { "Phi",       934 }, // greek capital letter phi, U+03A6 ISOgrk3
    { "Chi",       935 }, // greek capital letter chi, U+03A7
    { "Psi",       936 }, // greek capital letter psi, U+03A8 ISOgrk3
    { "Omega",     937 }, // greek capital letter omega, U+03A9 ISOgrk3
    { "alpha",     945 }, // greek small letter alpha, U+03B1 ISOgrk3
    { "beta",      946 }, // greek small letter beta, U+03B2 ISOgrk3
    { "gamma",     947 }, // greek small letter gamma, U+03B3 ISOgrk3
    { "delta",     948 }, // greek small letter delta, U+03B4 ISOgrk3
    { "epsilon",   949 }, // greek small letter epsilon, U+03B5 ISOgrk3
    { "zeta",      950 }, // greek small letter zeta, U+03B6 ISOgrk3
    { "eta",       951 }, // greek small letter eta, U+03B7 ISOgrk3
    { "theta",     952 }, // greek small letter theta, U+03B8 ISOgrk3
    { "iota",      953 }, // greek small letter iota, U+03B9 ISOgrk3
    { "kappa",     954 }, // greek small letter kappa, U+03BA ISOgrk3
    { "lambda",    955 }, // greek small letter lamda, U+03BB ISOgrk3
    { "mu",        956 }, // greek small letter mu, U+03BC ISOgrk3
    { "nu",        957 }, // greek small letter nu, U+03BD ISOgrk3
    { "xi",        958 }, // greek small letter xi, U+03BE ISOgrk3
    { "omicron",   959 }, // greek small letter omicron, U+03BF NEW
    { "pi",        960 }, // greek small letter pi, U+03C0 ISOgrk3
    { "rho",       961 }, // greek small letter rho, U+03C1 ISOgrk3
    { "sigmaf",    962 }, // greek small letter final sigma, U+03C2 ISOgrk3
    { "sigma",     963 }, // greek small letter sigma, U+03C3 ISOgrk3
    { "tau",       964 }, // greek small letter tau, U+03C4 ISOgrk3
    { "upsilon",   965 }, // greek small letter upsilon, U+03C5 ISOgrk3
    { "phi",       966 }, // greek small letter phi, U+03C6 ISOgrk3
    { "chi",       967 }, // greek small letter chi, U+03C7 ISOgrk3
    { "psi",       968 }, // greek small letter psi, U+03C8 ISOgrk3
    { "omega",     969 }, // greek small letter omega, U+03C9 ISOgrk3
    { "thetasym",  977 }, // greek theta symbol, U+03D1 NEW
    { "upsih",     978 }, // greek upsilon with hook symbol, U+03D2 NEW
    { "piv",       982 }, // greek pi symbol, U+03D6 ISOgrk3
    { "bull",     8226 }, // bullet = black small circle, U+2022 ISOpub
    { "hellip",   8230 }, // horizontal ellipsis = three dot leader, U+2026 ISOpub
    { "prime",    8242 }, // prime = minutes = feet, U+2032 ISOtech
    { "Prime",    8243 }, // double prime = seconds = inches, U+2033 ISOtech
    { "oline",    8254 }, // overline = spacing overscore, U+203E NEW
    { "frasl",    8260 }, // fraction slash, U+2044 NEW
    { "weierp",   8472 }, // script capital P = power set = Weierstrass p, U+2118 ISOamso
    { "image",    8465 }, // black-letter capital I = imaginary part, U+2111 ISOamso
    { "real",     8476 }, // black-letter capital R = real part symbol, U+211C ISOamso
    { "trade",    8482 }, // trade mark sign, U+2122 ISOnum
    { "alefsym",  8501 }, // alef symbol = first transfinite cardinal, U+2135 NEW
    { "larr",     8592 }, // leftwards arrow, U+2190 ISOnum
    { "uarr",     8593 }, // upwards arrow, U+2191 ISOnum-->
    { "rarr",     8594 }, // rightwards arrow, U+2192 ISOnum
    { "darr",     8595 }, // downwards arrow, U+2193 ISOnum
    { "harr",     8596 }, // left right arrow, U+2194 ISOamsa
    { "crarr",    8629 }, // downwards arrow with corner leftwards = carriage return, U+21B5 NEW
    { "lArr",     8656 }, // leftwards double arrow, U+21D0 ISOtech
    { "uArr",     8657 }, // upwards double arrow, U+21D1 ISOamsa
    { "rArr",     8658 }, // rightwards double arrow, U+21D2 ISOtech
    { "dArr",     8659 }, // downwards double arrow, U+21D3 ISOamsa
    { "hArr",     8660 }, // left right double arrow, U+21D4 ISOamsa
    { "forall",   8704 }, // for all, U+2200 ISOtech
    { "part",     8706 }, // partial differential, U+2202 ISOtech
    { "exist",    8707 }, // there exists, U+2203 ISOtech
    { "empty",    8709 }, // empty set = null set, U+2205 ISOamso
    { "nabla",    8711 }, // nabla = backward difference, U+2207 ISOtech
    { "isin",     8712 }, // element of, U+2208 ISOtech
    { "notin",    8713 }, // not an element of, U+2209 ISOtech
    { "ni",       8715 }, // contains as member, U+220B ISOtech
    { "prod",     8719 }, // n-ary product = product sign, U+220F ISOamsb
    { "sum",      8721 }, // n-ary summation, U+2211 ISOamsb
    { "minus",    8722 }, // minus sign, U+2212 ISOtech
    { "lowast",   8727 }, // asterisk operator, U+2217 ISOtech
    { "radic",    8730 }, // square root = radical sign, U+221A ISOtech
    { "prop",     8733 }, // proportional to, U+221D ISOtech
    { "infin",    8734 }, // infinity, U+221E ISOtech
    { "ang",      8736 }, // angle, U+2220 ISOamso
    { "and",      8743 }, // logical and = wedge, U+2227 ISOtech
    { "or",       8744 }, // logical or = vee, U+2228 ISOtech
    { "cap",      8745 }, // intersection = cap, U+2229 ISOtech
    { "cup",      8746 }, // union = cup, U+222A ISOtech
    { "int",      8747 }, // integral, U+222B ISOtech
    { "there4",   8756 }, // therefore, U+2234 ISOtech
    { "sim",      8764 }, // tilde operator = varies with = similar to, U+223C ISOtech
    { "cong",     8773 }, // approximately equal to, U+2245 ISOtech
    { "asymp",    8776 }, // almost equal to = asymptotic to, U+2248 ISOamsr
    { "ne",       8800 }, // not equal to, U+2260 ISOtech
    { "equiv",    8801 }, // identical to, U+2261 ISOtech
    { "le",       8804 }, // less-than or equal to, U+2264 ISOtech
    { "ge",       8805 }, // greater-than or equal to, U+2265 ISOtech
    { "sub",      8834 }, // subset of, U+2282 ISOtech
    { "sup",      8835 }, // superset of, U+2283 ISOtech
    { "nsub",     8836 }, // not a subset of, U+2284 ISOamsn
    { "sube",     8838 }, // subset of or equal to, U+2286 ISOtech
    { "supe",     8839 }, // superset of or equal to, U+2287 ISOtech
    { "oplus",    8853 }, // circled plus = direct sum, U+2295 ISOamsb
    { "otimes",   8855 }, // circled times = vector product, U+2297 ISOamsb
    { "perp",     8869 }, // up tack = orthogonal to = perpendicular, U+22A5 ISOtech
    { "sdot",     8901 }, // dot operator, U+22C5 ISOamsb
    { "lceil",    8968 }, // left ceiling = APL upstile, U+2308 ISOamsc
    { "rceil",    8969 }, // right ceiling, U+2309 ISOamsc
    { "lfloor",   8970 }, // left floor = APL downstile, U+230A ISOamsc
    { "rfloor",   8971 }, // right floor, U+230B ISOamsc
    { "lang",     9001 }, // left-pointing angle bracket = bra, U+2329 ISOtech
    { "rang",     9002 }, // right-pointing angle bracket = ket, U+232A ISOtech
    { "loz",      9674 }, // lozenge, U+25CA ISOpub
    { "spades",   9824 }, // black spade suit, U+2660 ISOpub
    { "clubs",    9827 }, // black club suit = shamrock, U+2663 ISOpub
    { "hearts",   9829 }, // black heart suit = valentine, U+2665 ISOpub
    { "diams",    9830 }, // black diamond suit, U+2666 ISOpub
};


class HtmlDtd: public DocTypeDef {
public:
    class EntityImpl: public Entity {
    public:
        StringUtf16 get_replacement_text() const
        {
            return replacement_text;
        }

        StringUtf16 replacement_text;
    };

    virtual const Entity* lookup_entity(const StringUtf16& n) const
    {
        Entities::const_iterator i = entities.find(n);
        return i == entities.end() ? 0 : &i->second;
    }

    HtmlDtd()
    {
        CharEnt* b = html_char_entities;
        CharEnt* e = b + sizeof html_char_entities / sizeof (CharEnt);
        for (CharEnt* i = b; i != e; ++i) {
            // FIXME: Consider UTF-16 surrogates
            entities[utf16_from_port(i->name)].replacement_text =
                StringUtf16(1, TraitsU16::to_char_type(i->ucs_code_point));
        }
    }

private:
    typedef std::map<StringUtf16, EntityImpl> Entities;
    Entities entities;

    void def_ent(const std::string& n, const std::string& v)
    {
        entities[utf16_from_port(n)].replacement_text = utf16_from_port(v);
    }

};


} // anonymous namespace




namespace archon {
namespace dom_impl {
namespace HtmlParser {


void parse_html(const Source& src, Callbacks& cb, Resolver& resolv, Logger* logger,
                const Config& config)
{
    HtmlDtd dtd;
    SgmlParser(&dtd, src, cb, resolv, logger, config).parse_sgml();
}


void parse_dtd(const Source& src, Callbacks& cb, Resolver& resolv, Logger* logger,
               const Config& config)
{
    DtdParser(src, cb, resolv, logger, config).parse_dtd();
}


bool parse_xml_proc_instr(const StringUtf16& text, StringUtf16& xml_target,
                          StringUtf16& xml_data)
{
    StringUtf16::size_type n = text.size();
    if (n > 0 & text[n-1] == char_Question) {
        --n;
        StringUtf16::size_type i = 0;
        while (i != n && !is_space(text[i]))
            ++i;
        StringUtf16 target = text.substr(0, i);
        if (i != 0 && validate_xml_1_0_name(target)) {
            if (i != n) {
                ++i;
                while (i != n && is_space(text[i]))
                    ++i;
            }
            xml_target = target;
            xml_data   = text.substr(i);
            return true;
        }
    }
    return false;
}



void DefaultResolver::resolve(const StringUtf16&,
                              const StringUtf16& system_ident,
                              const StringUtf16& base_uri,
                              UniquePtr<InputStream>& in,
                              StringUtf16& charenc,
                              StringUtf16& uri)
{
    Uri::Decomposed uri_decomp(utf16_to_narrow(system_ident, loc));
    uri_decomp.resolve(Uri::Decomposed(utf16_to_narrow(base_uri, loc)));
    std::string scheme = uri_decomp.get_scheme();
    if (scheme.empty() || scheme == "file") {
        try {
            in.reset(make_file_input_stream(uri_decomp.get_path()).release());
        }
        catch (File::AccessException &e) {
            throw ResolveException("Failed to open '"+uri_decomp.get_path()+"': " + e.what());
        }
    }
    else {
        throw ResolveException("Unsupported URI scheme '"+uri_decomp.get_scheme()+"'");
    }
    charenc.clear(); // Unknown
    uri = utf16_from_narrow(uri_decomp.recompose(), loc);
}



void Callbacks::doctype_begin(const core::StringUtf16&, const core::StringUtf16&,
                              const core::StringUtf16&)
{
}

void Callbacks::doctype_end()
{
}

void Callbacks::elem_begin(const core::StringUtf16&, const std::vector<Attr>&)
{
}

void Callbacks::elem_end(const core::StringUtf16&)
{
}

void Callbacks::script(const std::vector<Attr>&, InlineStream&, DocWriter&)
{
}

void Callbacks::style(const std::vector<Attr>&, InlineStream&)
{
}

void Callbacks::text(const core::StringUtf16&)
{
}

void Callbacks::comment(const core::StringUtf16&)
{
}

void Callbacks::proc_instr(const core::StringUtf16&)
{
}


} // namespace HtmlParser
} // namespace dom_impl
} // namespace archon
