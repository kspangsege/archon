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
 */

#include <archon/platform.hpp> // Never include in other header files
#include <archon/dom/impl/ls.hpp>

#ifdef ARCHON_HAVE_LIBEXPAT
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <vector>
#include <map>
#include <istream>

#include <expat.h>

#include <archon/core/assert.hpp>
#include <archon/core/memory.hpp>
#include <archon/core/text.hpp>
#endif // ARCHON_HAVE_LIBEXPAT


using namespace std;
using namespace archon::core;


// FIXME: PROBLEM: How can an in-context asynchronous parser be supported without completely thread-safe document API? NOT A PROBLEM, because parseWithContext() always operates in synchonous mode. There are still problems!!! The thread that initiates an asynchronious parse will be in conflict will the parsing thread immediately. What are the use cases for asynchronous parsing? Progress reporting? Simultaneous loading of multiple documents? PArtial rendering would require special support of the implementation.

// FIXME: Consider processing instructions and entity references inside internal subset. Handle like comment?

// FIXME: Retainment of CDATA sections must be disabled if DOMConfigurations says so.

// FIXME: Retainment of comments must be disabled if DOMConfigurations says so.

// FIXME: Namespace processing must be disabled if DOMConfiguration says so. What happens when namespace processing is disabled? Will all namespace URIs be set to null?

// FIXME: What about normalization of namespace decl attribs? Is that part of namespace processing when parsing?

// FIXME: Unfortunately, eXpat does not reveal the encoding when it autodetects it. What can be done about it? SOLUTION: Lure out the auto-detection code from eXpat and implement it in the parser. /home/kristian/expat/expat-2.0.1/lib/xmltok.c:1496

// FIXME: An in-context parser must call on_before_children_change() on context node before parsing.

// FIXME: If scripts are supposed to run during the parsing action, or if the DOM could be manipulated in other ways during parsing, or if partial redering happens during parsing, then the parser will need to hold a reference count on each element in the current stack, and on_before_children_change() must be called for all added children.

/*

UPS: An external entity probably does not need to start with an ASCII characters!

Rules: No null chars. First char, if present, must be US-ASCII. A BOM may or may not be present. Encoding is UTF-8, UTF-16LE, UTF-16BE, UTF-32LE, or UTF-32BE.

BOMS

SEE ALSO: http://www.w3.org/TR/2008/REC-xml-20081126/#sec-guessing

1  2  3  4  5  6  7  8
----------------------------------------
no                       Unknown

EF BB BF no              UTF-8 (throw away BOM)
EF BB BF as              UTF-8 (throw away BOM)

as \0 no                 UTF-16LE
\0 as no                 UTF-16BE
FF FE no                 UTF-16LE (throw away BOM)
FE FF no                 UTF-16BE (throw away BOM)
FF FE as \0              UTF-16LE (throw away BOM)
FE FF \0 as              UTF-16BE (throw away BOM)

as \0 \0 \0              UTF-32LE
\0 \0 \0 as              UTF-32BE
FF FE \0 \0 no           UTF-32LE (BOM)
\0 \0 FE FF no           UTF-32BE (BOM)
FF FE \0 \0 as \0 \0 \0  UTF-32LE (BOM)
\0 \0 FE FF \0 \0 \0 as  UTF-32BE (BOM)

Error: Unknown encoding or bad start of XML file.

*/


// FIXME: Must handle the case where no XML parser library is available. Simply throw an LSEXception(PARSE_ERR) in that case.

// FIXME: It is unsafe to throw exceptions through C functions. A different sollution must be found.

// FIXME: eXpat can only handle XML 1.0 documents.



namespace
{
  namespace dom = archon::dom;
  using namespace archon::dom_impl;



  struct LSInput: virtual dom::ls::LSInput
  {
    std::istream *byte_stream;
    dom::DOMString system_id, encoding;

    virtual std::istream *getByteStream() const throw ()
    {
      return byte_stream;
    }

    virtual void setByteStream(std::istream *v) throw ()
    {
      byte_stream = v;
    }

    virtual dom::DOMString getSystemId() const throw ()
    {
      return system_id;
    }

    virtual void setSystemId(dom::DOMString const &v) throw ()
    {
      system_id = v;
    }

    virtual dom::DOMString getEncoding() const throw ()
    {
      return encoding;
    }

    virtual void setEncoding(dom::DOMString const &v) throw ()
    {
      encoding = v;
    }

    virtual ~LSInput() throw () {}
  };



#ifdef ARCHON_HAVE_LIBEXPAT

#ifdef XML_UNICODE // In this case the eXpat API uses UTF-16
  void conv_expat_to_utf16(XML_Char const *src, size_t len, dom::DOMString &dst,
                           bool append = false)
  {
    dom::DOMString::size_type const i = append ? dst.size() : 0;
    dst.resize(i + len);
    transform(src, src+len, dst.begin()+i, &dom::DOMString::traits_type::to_char_type);
  }

  void conv_utf16_to_expat(dom::DOMString const &src, basic_string<XML_Char> &dst)
  {
    dst.resize(src.size());
    transform(src.begin(), src.end(), dst.begin(), &dom::DOMString::traits_type::to_int_type);
  }
#else // XML_UNICODE - In this case the eXpat API uses UTF-8
#error Not implemented
// FIXME: Must convert from UTF-8 to UTF-16
#endif // XML_UNICODE



  struct ParserContext
  {
    void handle_xml_decl(XML_Char const *version, XML_Char const *encoding, bool standalone)
    {
      flush_text();

      ARCHON_ASSERT_1(version, "No version in XML decl - what to do?"); // FIXME: Handle this!

      dom::DOMString version2, encoding2;
      conv_expat_to_utf16(version, str_len(version), version2);
      if (encoding) conv_expat_to_utf16(encoding, str_len(encoding), encoding2);

      xml_version    = impl->parse_xml_ver(version2);
      xml_encoding   = encoding2;
      xml_standalone = standalone;
    }



    void handle_elem_begin(XML_Char const *name, XML_Char const *const *attribs)
    {
      flush_text();

      if (!doc) create_doc(0);

      // Introduce new level
      Level const *parent_level = 0;
      {
        Levels::size_type n = levels.size();
        levels.resize(n+1);
        level = &levels.back();
        level->num_ns_overrides = 0;
        if (n) parent_level = &levels[n-1];
      }

      // Preprocess attributes
      attributes.clear();
      {
        XML_Char const *const *i = attribs;
        for (;;) {
          XML_Char const *const n = *i;
          if (!n) break;

          dom::DOMString name2, value;
          conv_expat_to_utf16(n, str_len(n), name2);

          XML_Char const *const v = *++i;
          if (!v) throw runtime_error("Missing attribute value");
          conv_expat_to_utf16(v, str_len(v), value);

          dom::DOMString prefix, local_name;
          Document::parse_qualified_name(xml_version, name2, prefix, local_name);

          if (prefix.empty()) {
            if (local_name == str_xmlns) register_namespace(prefix, value);
          }
          else if (prefix == str_xmlns) {
            if (local_name == str_xml && value != str_ns_namespace)
              throw dom::ls::LSException(dom::ls::PARSE_ERR, "Prefix 'xml' may not be bound to "
                                         "anything else than "
                                         "'http://www.w3.org/XML/1998/namespace'");
            if (local_name == str_xmlns && value != str_ns_xmlns)
              throw dom::ls::LSException(dom::ls::PARSE_ERR, "Prefix 'xmlns' may not be bound to "
                                         "anything else than 'http://www.w3.org/2000/xmlns/'");
            if (xml_version == Document::xml_ver_1_0 && value.empty())
              throw dom::ls::LSException(dom::ls::PARSE_ERR, "Non-default namespace bindings "
                                         "may not be empty");
            register_namespace(local_name, value);
          }

          attributes.push_back(AttribEntry(name2, prefix, local_name, value));

          ++i;
        }
      }

      // Create a new element
      {
        ParentNode *const parent =
          parent_level ? static_cast<ParentNode *>(parent_level->elem) : doc.get();
        dom::DOMString name2;
        conv_expat_to_utf16(name, str_len(name), name2);
        dom::DOMString ns, prefix, local_name;
        Document::parse_qualified_name(xml_version, name2, prefix, local_name);
        {
          NamespaceMap::const_iterator const i = namespace_map.find(prefix);
          if (i != namespace_map.end() && !i->second.empty()) {
            ns = i->second;
          }
          else if (!prefix.empty()) {
            throw dom::ls::LSException(dom::ls::PARSE_ERR, "Unbound namespace prefix");
          }
        }
        level->elem = doc->create_elem_child_for_parser(parent, ns, name2, prefix, local_name);
        level->is_element_content = level->elem->get_type()->is_element_content();
      }

      // Apply attributes
      {
//         Element *const elem = level->elem;
        Attributes::const_iterator const e = attributes.end();
        for (Attributes::const_iterator i=attributes.begin(); i!=e; ++i) {
//           AttribEntry const &a = *i;
//           dom::DOMString ns2;
//           if (a.prefix.empty()) {
//             if (a.local_name == str_xmlns) ns2 = str_ns_xmlns;
//           }
//           else {
//             NamespaceMap::const_iterator const j = namespace_map.find(a.prefix);
//             if (j == namespace_map.end() || j->second.empty()) {
//               throw dom::ls::LSException(dom::ls::PARSE_ERR, "Unbound namespace prefix");
//             }
//             ns2 = j->second;
//           }
//           elem->set_attrib_for_parser(ns2, a->name, a->prefix, a->local_name, a->value);
        }
      }
    }



    void handle_elem_end()
    {
      flush_text();

      // Undo the namespace overrides of the element that is ending
      if (level->num_ns_overrides) {
        NsOverrides::size_type i = ns_overrides.size();
        NsOverrides::size_type const end = i - level->num_ns_overrides;
        do {
          pair<dom::DOMString, dom::DOMString> const &o = ns_overrides[--i];
          namespace_map[o.first] = o.second;
        }
        while (i != end);
        ns_overrides.resize(end);
      }

      levels.pop_back();
      level = levels.empty() ? 0 : &levels.back();
    }



    void handle_char_data(XML_Char const *data, size_t len)
    {
      conv_expat_to_utf16(data, len, text_accum, true); // Append
    }



    void handle_comment(XML_Char const *data)
    {
      if (in_internal_subset) {
        XML_DefaultCurrent(parser);
        return;
      }

      flush_text();

      dom::DOMString data2;
      conv_expat_to_utf16(data, str_len(data), data2);

      if (doc) {
        level->elem->append_child_for_parser(new Comment(doc.get(), data2));
        return;
      }

      pending_doc_nodes.push_back(dom::Node::COMMENT_NODE+0);
      pending_doc_node_args.push_back(data2);
    }



    void handle_cdata_sect_begin()
    {
      flush_text();
    }



    void handle_cdata_sect_end()
    {
      if (text_accum.empty()) return;

      ARCHON_ASSERT_1(doc, "CDATA section before root element");
      bool const elem_cont_whitespace = level->is_element_content &&
        DOMImplementation::is_whitespace(text_accum);
      level->elem->append_child_for_parser(new CDATASection(doc.get(), text_accum,
                                                            elem_cont_whitespace));
      text_accum.clear();
    }



    void handle_proc_instr(XML_Char const *target, XML_Char const *data)
    {
      flush_text();

      dom::DOMString target2, data2;
      conv_expat_to_utf16(target, str_len(target), target2);
      conv_expat_to_utf16(data,   str_len(data),   data2);

      if (doc) {
        level->elem->append_child_for_parser(new ProcessingInstruction(doc.get(), target2, data2));
        return;
      }

      pending_doc_nodes.push_back(dom::Node::PROCESSING_INSTRUCTION_NODE+0);
      pending_doc_node_args.push_back(target2);
      pending_doc_node_args.push_back(data2);
    }



    void handle_doctype_begin(XML_Char const *name, XML_Char const *public_id,
                              XML_Char const *system_id)
    {
      flush_text();

      ARCHON_ASSERT_1(!doc, "Document already created");

      dom::DOMString name2, public_id2, system_id2;
      conv_expat_to_utf16(name, str_len(name), name2);
      if (public_id) conv_expat_to_utf16(public_id, str_len(public_id), public_id2);
      if (system_id) conv_expat_to_utf16(system_id, str_len(system_id), system_id2);

      doctype.reset(new DocumentType(impl.get(), name2, public_id2, system_id2));
      create_doc(doctype.get());
      doc->appendChild(doctype);

      in_internal_subset = true;
    }



    void handle_doctype_end()
    {
      doctype->set_internal_subset(text_accum);
      text_accum.clear();

      in_internal_subset = false;
    }



    // FIXME: Two versions of this - one must go! This one looks good!
    void handle_entity(XML_Char const *name, bool is_param_entity, XML_Char const *value,
                       int value_len, XML_Char const *public_id, XML_Char const *system_id,
                       XML_Char const *notation_name)
    {
      // FIXME: Unfortunately, and surprisingly, this does not work,
      // and should not work according to the eXpat documentation. A
      // different solution must be found. SOLUTION: Manually format
      // the entity.
      XML_DefaultCurrent(parser);

      if (is_param_entity) return;

      dom::DOMString name2, value2, public_id2, system_id2, notation_name2;
      conv_expat_to_utf16(name, str_len(name), name2);
      if (value) conv_expat_to_utf16(value, value_len, value2);
      if (public_id) conv_expat_to_utf16(public_id, str_len(public_id), public_id2);
      if (system_id) conv_expat_to_utf16(system_id, str_len(system_id), system_id2);
      if (notation_name) conv_expat_to_utf16(notation_name, str_len(notation_name),
                                             notation_name2);

      doctype->add_entity(name2, public_id2, system_id2, notation_name2);

      // FIXME: Load as parse external entity.
    }



    void handle_notation(XML_Char const *name, XML_Char const *public_id,
                         XML_Char const *system_id)
    {
      // FIXME: Unfortunately, and surprisingly, this does not work,
      // and should not work according to the eXpat documentation. A
      // different solution must be found. SOLUTION: Manually format
      // the entity.
      XML_DefaultCurrent(parser);

      dom::DOMString name2, public_id2, system_id2;
      conv_expat_to_utf16(name, str_len(name), name2);
      if (public_id) conv_expat_to_utf16(public_id, str_len(public_id), public_id2);
      if (system_id) conv_expat_to_utf16(system_id, str_len(system_id), system_id2);

      doctype->add_notation(name2, public_id2, system_id2);
    }



    // FIXME: Two versions of this - one must go! This one looks bad!
    void handle_entity(XML_Char const *name, XML_Char const *public_id,
                       XML_Char const *system_id)
    {
      // FIXME: Unfortunately, and surprisingly, this does not work,
      // and should not work according to the eXpat documentation. A
      // different solution must be found.
      XML_DefaultCurrent(parser);

      dom::DOMString name2, public_id2, system_id2;
      conv_expat_to_utf16(name, str_len(name), name2);
      if (public_id) conv_expat_to_utf16(public_id, str_len(public_id), public_id2);
      if (system_id) conv_expat_to_utf16(system_id, str_len(system_id), system_id2);

      doctype->add_notation(name2, public_id2, system_id2);
    }



    void handle_default(XML_Char const *s, int len)
    {
      if (in_internal_subset) {
        conv_expat_to_utf16(s, len, text_accum, true); // Append
        return;
      }
    }



    ParserContext(dom::ref<DOMImplementationLS> const &i, XML_Parser p,
                  dom::DOMString const &uri, dom::DOMString const &enc):
      impl(i), parser(p), str_xml(impl->str_xml), str_xmlns(impl->str_xmlns),
      str_ns_namespace(impl->str_ns_namespace), str_ns_xmlns(impl->str_ns_xmlns),
      document_uri(uri), input_encoding(enc), xml_version(Document::xml_ver_1_0),
      xml_standalone(false), in_internal_subset(false), level(0)
    {
      namespace_map[str_xmlns] = str_ns_xmlns;
    }


    dom::ref<Document> get_doc() { return doc; }

  private:
    dom::ref<DOMImplementationLS> const impl;

    XML_Parser const parser;

    dom::DOMString const str_xml, str_xmlns, str_ns_namespace, str_ns_xmlns;

    dom::DOMString const document_uri;   // Empty if unknown
    dom::DOMString const input_encoding; // Empty if same as xml_encoding

    Document::XmlVersion xml_version;
    dom::DOMString xml_encoding;
    bool xml_standalone;

    dom::ref<DocumentType> doctype;
    bool in_internal_subset;

    dom::ref<Document> doc;

    struct Level
    {
      Element *elem;
      int num_ns_overrides;
      bool is_element_content; // FIXME: Should be a pointer to a schema type information instance acquired from elem->get_type().
    };

    typedef vector<Level> Levels;
    Levels levels;

    Level *level;


    typedef map<dom::DOMString, dom::DOMString> NamespaceMap; // Maps prefix to URL/IRI
    NamespaceMap namespace_map;


    typedef vector<pair<dom::DOMString, dom::DOMString> > NsOverrides;
    NsOverrides ns_overrides;


    struct AttribEntry
    {
      dom::DOMString name, prefix, local_name, value;
      AttribEntry(dom::DOMString const &n, dom::DOMString const &p,
                  dom::DOMString const &l, dom::DOMString const &v):
        name(n), prefix(p), local_name(l), value(v) {}
    };

    typedef vector<AttribEntry> Attributes;
    Attributes attributes;


    dom::DOMString text_accum;

    void flush_text()
    {
      if (text_accum.empty()) return;

      ARCHON_ASSERT_1(doc, "Text before root element");
      bool const elem_cont_whitespace = level->is_element_content &&
        DOMImplementation::is_whitespace(text_accum);
      level->elem->append_child_for_parser(new archon::dom_impl::Text(doc.get(), text_accum,
                                                                      elem_cont_whitespace));
      text_accum.clear();
    }


    vector<dom::uint16> pending_doc_nodes;
    vector<dom::DOMString> pending_doc_node_args;

    void create_doc(DocumentType *doctype)
    {
      doc = impl->create_document(doctype);
      dom::DOMString const input_enc = input_encoding.empty() ? xml_encoding : input_encoding;
      doc->set_doc_info(document_uri, input_enc, xml_version, xml_encoding, xml_standalone);

      // Add pending document nodes
      vector<dom::DOMString>::size_type arg_idx = 0;
      typedef vector<dom::uint16>::const_iterator iter;
      iter const e = pending_doc_nodes.end();
      for (iter i=pending_doc_nodes.begin(); i!=e; ++i) {
        switch (*i) {
        case dom::Node::PROCESSING_INSTRUCTION_NODE:
          {
            dom::DOMString const target = pending_doc_node_args[arg_idx++];
            dom::DOMString const data   = pending_doc_node_args[arg_idx++];
            doc->append_child_for_parser(new ProcessingInstruction(doc.get(), target, data));
          }
          break;
        case dom::Node::COMMENT_NODE:
          {
            dom::DOMString const data = pending_doc_node_args[arg_idx++];
            doc->append_child_for_parser(new Comment(doc.get(), data));
          }
          break;
        default:
          throw runtime_error("Unexpected pending document node type");
        }
      }

      pending_doc_nodes.clear();
      pending_doc_node_args.clear();
    }


    // An empty URL will unregister the prefix
    void register_namespace(dom::DOMString const &prefix, dom::DOMString const &url)
    {
      dom::DOMString &url2 = namespace_map[prefix];
      ns_overrides.push_back(make_pair(prefix, url2));
      ++level->num_ns_overrides;
      url2 = url;
    }


    static size_t str_len(XML_Char const *s)
    {
      XML_Char const *t = s;
      while(*t) ++t;
      return t - s;
    }
  };




  struct LSParser: virtual dom::ls::LSParser
  {
    virtual dom::ref<dom::Document> parse(dom::ref<dom::ls::LSInput> const &input)
      throw (dom::DOMException, dom::ls::LSException)
    {
      std::istream &in               = *input->getByteStream();
      dom::DOMString const system_id =  input->getSystemId();
      dom::DOMString const encoding  =  input->getEncoding();

      basic_string<XML_Char> encoding2;
      conv_utf16_to_expat(encoding, encoding2);

      ParserOwner p(XML_ParserCreate(encoding2.empty() ? 0 : encoding2.c_str()));
      XML_SetXmlDeclHandler(p.ptr, &handle_xml_decl);
      XML_SetElementHandler(p.ptr, &handle_elem_begin, &handle_elem_end);
      XML_SetCharacterDataHandler(p.ptr, &handle_char_data);
      XML_SetCommentHandler(p.ptr, &handle_comment);
      XML_SetCdataSectionHandler(p.ptr, &handle_cdata_sect_begin, &handle_cdata_sect_end);
      XML_SetProcessingInstructionHandler(p.ptr, &handle_proc_instr);
      XML_SetDoctypeDeclHandler(p.ptr, &handle_doctype_begin, &handle_doctype_end);
      XML_SetEntityDeclHandler(p.ptr, handle_entity);
      XML_SetNotationDeclHandler(p.ptr, handle_notation);
      XML_SetDefaultHandler(p.ptr, &handle_default);

      ParserContext ctx(impl, p.ptr, system_id, encoding);
      XML_SetUserData(p.ptr, &ctx);

      streamsize const buffer_size = 2048;
      MemoryBuffer const buffer(buffer_size);

      for (;;) {
        streamsize const n = in.read(buffer.get(), buffer_size).gcount();
        bool const is_last = n < buffer_size;
        if (!XML_Parse(p.ptr, buffer.get(), n, is_last))
          throw dom::ls::LSException(dom::ls::PARSE_ERR, XML_ErrorString(XML_GetErrorCode(p.ptr)));
        if(is_last) break;
      }

      return ctx.get_doc();
    }

    LSParser(DOMImplementationLS *i): impl(i) {}

    virtual ~LSParser() throw () {}

  private:
    dom::ref<DOMImplementationLS> const impl;

    struct ParserOwner
    {
      XML_Parser const ptr;

      ParserOwner(XML_Parser p): ptr(p) {}
      ~ParserOwner() { XML_ParserFree(ptr); }
    };

    static void handle_xml_decl(void *user_data, XML_Char const *version, XML_Char const *encoding,
                                int standalone)
    {
      static_cast<ParserContext *>(user_data)->handle_xml_decl(version, encoding, 0 < standalone);
    }

    static void handle_elem_begin(void *user_data, XML_Char const *name, XML_Char const **attribs)
    {
      XML_Char const *const *const attribs2 = const_cast<XML_Char const *const *>(attribs);
      static_cast<ParserContext *>(user_data)->handle_elem_begin(name, attribs2);
    }

    static void handle_elem_end(void *user_data, XML_Char const *)
    {
      static_cast<ParserContext *>(user_data)->handle_elem_end();
    }

    static void handle_char_data(void *user_data, XML_Char const *data, int len)
    {
      static_cast<ParserContext *>(user_data)->handle_char_data(data, len);
    }

    static void handle_comment(void *user_data, XML_Char const *data)
    {
      static_cast<ParserContext *>(user_data)->handle_comment(data);
    }

    static void handle_cdata_sect_begin(void *user_data)
    {
      static_cast<ParserContext *>(user_data)->handle_cdata_sect_begin();
    }

    static void handle_cdata_sect_end(void *user_data)
    {
      static_cast<ParserContext *>(user_data)->handle_cdata_sect_end();
    }

    static void handle_proc_instr(void *user_data, XML_Char const *target, XML_Char const *data)
    {
      static_cast<ParserContext *>(user_data)->handle_proc_instr(target, data);
    }

    static void handle_doctype_begin(void *user_data, XML_Char const *qualified_name,
                                     XML_Char const *system_id, XML_Char const *public_id, int)
    {
      static_cast<ParserContext *>(user_data)->handle_doctype_begin(qualified_name, public_id,
                                                                    system_id);
    }

    static void handle_doctype_end(void *user_data)
    {
      static_cast<ParserContext *>(user_data)->handle_doctype_end();
    }

    static void handle_entity(void *user_data, XML_Char const *name, int is_param_entity,
                              XML_Char const *value, int value_len, XML_Char const *,
                              XML_Char const *system_id, XML_Char const *public_id,
                              XML_Char const *notation_name)
    {
      static_cast<ParserContext *>(user_data)->handle_entity(name, is_param_entity != 0, value,
                                                             value_len, public_id, system_id,
                                                             notation_name);
    }

    static void handle_notation(void *user_data, XML_Char const *name, XML_Char const *,
                                XML_Char const *system_id, XML_Char const *public_id)
    {
      static_cast<ParserContext *>(user_data)->handle_notation(name, public_id, system_id);
    }

    static void handle_default(void *user_data, XML_Char const *s, int len)
    {
      static_cast<ParserContext *>(user_data)->handle_default(s, len);
    }
  };

#endif // ARCHON_HAVE_LIBEXPAT
}




namespace archon
{
  namespace dom_impl
  {
#ifdef ARCHON_HAVE_LIBEXPAT
    dom::ref<dom::ls::LSParser>
    DOMImplementationLS::createLSParser(dom::uint16 mode, dom::DOMString const &schemaType) const
      throw (dom::DOMException)
    {
      if (mode != dom::ls::DOMImplementationLS::MODE_SYNCHRONOUS)
        throw dom::DOMException(dom::NOT_SUPPORTED_ERR, "Only synchronous mode is supported");
      if (!schemaType.empty())
        throw dom::DOMException(dom::NOT_SUPPORTED_ERR, "Unrecognized schema type");

      return dom::ref<dom::ls::LSParser>(new LSParser(const_cast<DOMImplementationLS *>(this)));
    }
#else // ARCHON_HAVE_LIBEXPAT
    dom::ref<dom::ls::LSParser>
    DOMImplementationLS::createLSParser(dom::uint16, dom::DOMString const &) const
      throw (dom::DOMException)
    {
      throw dom::DOMException(dom::NOT_SUPPORTED_ERR, "XML parsing is unavailable");
    }
#endif // ARCHON_HAVE_LIBEXPAT



    dom::ref<dom::ls::LSInput> DOMImplementationLS::createLSInput() const throw ()
    {
      return dom::ref<dom::ls::LSInput>(new LSInput);
    }



    // Overriding method in DOMImplementation
    bool DOMImplementationLS::has_feature(dom::DOMString const &f, dom::DOMString const &v) const
      throw ()
    {
#ifdef ARCHON_HAVE_LIBEXPAT
      if (f == str_feat_ls) {
        return v.empty() || v == str_ver_3_0;
      }
#endif // ARCHON_HAVE_LIBEXPAT
      return DOMImplementation::has_feature(f,v);
    }



    DOMImplementationLS::DOMImplementationLS(): str_feat_ls(dom::str_from_cloc(L"LS")) {}
  }
}

