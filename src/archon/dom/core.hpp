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

#ifndef ARCHON_DOM_CORE_HPP
#define ARCHON_DOM_CORE_HPP

/// \file
///
/// \author Kristian Spangsege

#include <stdexcept>

#include <archon/dom/util/types.hpp>
#include <archon/dom/util/ref.hpp>
#include <archon/dom/util/object.hpp>
#include <archon/dom/util/string.hpp>


namespace archon {
namespace dom {

class NodeList;
class Element;
class DocumentType;
class Document;
class DOMImplementation;



class DOMException: public std::runtime_error {
public:
    DOMException(int16 c, std::string msg): std::runtime_error(msg), code(c) {}

    const int16 code;
};

constexpr int16 INDEX_SIZE_ERR              =  1;
constexpr int16 DOMSTRING_SIZE_ERR          =  2;
constexpr int16 HIERARCHY_REQUEST_ERR       =  3;
constexpr int16 WRONG_DOCUMENT_ERR          =  4;
constexpr int16 INVALID_CHARACTER_ERR       =  5;
constexpr int16 NO_DATA_ALLOWED_ERR         =  6;
constexpr int16 NO_MODIFICATION_ALLOWED_ERR =  7;
constexpr int16 NOT_FOUND_ERR               =  8;
constexpr int16 NOT_SUPPORTED_ERR           =  9;
constexpr int16 INUSE_ATTRIBUTE_ERR         = 10;
constexpr int16 INVALID_STATE_ERR           = 11;
constexpr int16 SYNTAX_ERR                  = 12;
constexpr int16 INVALID_MODIFICATION_ERR    = 13;
constexpr int16 NAMESPACE_ERR               = 14;
constexpr int16 INVALID_ACCESS_ERR          = 15;
constexpr int16 VALIDATION_ERR              = 16;
constexpr int16 TYPE_MISMATCH_ERR           = 17;



class DOMImplementationList: public virtual DOMObject {
public:
    virtual ref<DOMImplementation> item(uint32 index) const throw () = 0;

    virtual uint32 getLength() const throw () = 0;
};



class DOMImplementationSource: public virtual DOMObject {
public:
    virtual ref<DOMImplementation> getDOMImplementation(const DOMString& features) const
        throw () = 0;

    virtual ref<DOMImplementationList> getDOMImplementationList(const DOMString& features) const
        throw () = 0;
};



class Node: public virtual DOMObject {
public:
    static constexpr uint16 ELEMENT_NODE                =  1;
    static constexpr uint16 ATTRIBUTE_NODE              =  2;
    static constexpr uint16 TEXT_NODE                   =  3;
    static constexpr uint16 CDATA_SECTION_NODE          =  4;
    static constexpr uint16 ENTITY_REFERENCE_NODE       =  5;
    static constexpr uint16 ENTITY_NODE                 =  6;
    static constexpr uint16 PROCESSING_INSTRUCTION_NODE =  7;
    static constexpr uint16 COMMENT_NODE                =  8;
    static constexpr uint16 DOCUMENT_NODE               =  9;
    static constexpr uint16 DOCUMENT_TYPE_NODE          = 10;
    static constexpr uint16 DOCUMENT_FRAGMENT_NODE      = 11;
    static constexpr uint16 NOTATION_NODE               = 12;

    virtual DOMString getNodeName() const throw () = 0;

    virtual DOMString getNodeValue() const throw (DOMException) = 0;

    virtual void setNodeValue(const DOMString& nodeValue) throw (DOMException) = 0;

    virtual uint16 getNodeType() const throw () = 0;

    virtual ref<Node> getParentNode() const throw () = 0;

    virtual ref<NodeList> getChildNodes() const throw () = 0;

    virtual ref<Node> getFirstChild() const throw () = 0;

    virtual ref<Node> getLastChild() const throw () = 0;

    virtual ref<Node> getPreviousSibling() const throw () = 0;

    virtual ref<Node> getNextSibling() const throw () = 0;

    virtual ref<Document> getOwnerDocument() const throw () = 0;

    virtual ref<Node> insertBefore(const ref<Node>& newChild, const ref<Node>& refChild)
        throw (DOMException) = 0;

    virtual ref<Node> replaceChild(const ref<Node>& newChild, const ref<Node>& oldChild)
        throw (DOMException) = 0;

    virtual ref<Node> removeChild(const ref<Node>& oldChild) throw (DOMException) = 0;

    virtual ref<Node> appendChild(const ref<Node>& newChild) throw (DOMException) = 0;

    virtual bool hasChildNodes() const throw () = 0;

    virtual bool isSupported(const DOMString& feature, const DOMString& version) const
        throw () = 0;

    virtual DOMString getNamespaceURI() const throw () = 0;

    virtual DOMString getPrefix() const throw () = 0;

    virtual void setPrefix(const DOMString& prefix) throw (DOMException) = 0;

    virtual DOMString getLocalName() const throw () = 0;

    virtual DOMString getTextContent() const throw (DOMException) = 0;

    virtual void setTextContent(const DOMString& textContent) throw (DOMException) = 0;

    virtual bool isSameNode(const ref<const Node>& other) const throw () = 0;
};



class NodeList: public virtual DOMObject {
public:
    virtual ref<Node> item(uint32 index) const throw () = 0;

    virtual uint32 getLength() const throw () = 0;
};



class NamedNodeMap: public virtual DOMObject {
public:
    virtual ref<Node> getNamedItem(const DOMString& name) const throw () = 0;

    virtual ref<Node> setNamedItem(const ref<Node>& arg) throw (DOMException) = 0;

    virtual ref<Node> removeNamedItem(const DOMString& name) throw (DOMException) = 0;

    virtual ref<Node> item(uint32 index) const throw () = 0;

    virtual uint32 getLength() const throw () = 0;

    virtual ref<Node> getNamedItemNS(const DOMString& namespaceURI,
                                     const DOMString& localName) const throw (DOMException) = 0;

    virtual ref<Node> setNamedItemNS(const ref<Node>& arg) throw (DOMException) = 0;

    virtual ref<Node> removeNamedItemNS(const DOMString& namespaceURI,
                                        const DOMString& localName) throw (DOMException) = 0;
};



class Attr: public virtual Node {
public:
    virtual DOMString getName() const throw () = 0;

    virtual bool getSpecified() const throw () = 0;

    virtual DOMString getValue() const throw () = 0;

    virtual void setValue(const DOMString& value) throw (DOMException) = 0;

    virtual ref<Element> getOwnerElement() const throw () = 0;

//      TypeInfo getSchemaTypeInfo();

    virtual bool isId() const throw () = 0;
};



class Element: public virtual Node {
public:
    virtual DOMString getTagName() const throw () = 0;

    virtual DOMString getAttribute(const DOMString& name) const throw () = 0;

    virtual void setAttribute(const DOMString& name, const DOMString& value)
        throw (DOMException) = 0;

    virtual ref<NodeList> getElementsByTagName(const DOMString& name) const throw () = 0;

    virtual ref<NodeList> getElementsByTagNameNS(const DOMString& namespaceURI,
                                                 const DOMString& localName) const
        throw (DOMException) = 0;
};



class DocumentFragment: public virtual Node {};



class CharacterData: public virtual Node {
public:
    virtual DOMString getData() const throw (DOMException) = 0;

    virtual void setData(const DOMString& data) throw (DOMException) = 0;
};



class Text: public virtual CharacterData {
public:
//    virtual ref<Text> splitText(uint32 offset) throw (DOMException) = 0;

    virtual bool isElementContentWhitespace() const throw () = 0;

//    virtual DOMString getWholeText() const throw () = 0;

//    virtual ref<Text> replaceWholeText(const DOMString& content) throw (DOMException) = 0;
};



class Comment: public virtual CharacterData {};



class CDATASection: public virtual Text {};



class ProcessingInstruction: public virtual Node {
public:
    virtual DOMString getTarget() const throw () = 0;

    virtual DOMString getData() const throw () = 0;

    virtual void setData(const DOMString& data) throw (DOMException) = 0;
};



class DocumentType: public virtual Node {
public:
    virtual DOMString getName() const throw () = 0;

    virtual ref<NamedNodeMap> getEntities() const throw () = 0;

    virtual ref<NamedNodeMap> getNotations() const throw () = 0;

    virtual DOMString getPublicId() const throw () = 0;

    virtual DOMString getSystemId() const throw () = 0;

    virtual DOMString getInternalSubset() const throw () = 0;
};



class Notation: public virtual Node {
public:
    virtual DOMString getPublicId() const throw () = 0;

    virtual DOMString getSystemId() const throw () = 0;
};



class Entity: public virtual Node {
public:
    virtual DOMString getPublicId() const throw () = 0;

    virtual DOMString getSystemId() const throw () = 0;

    virtual DOMString getNotationName() const throw () = 0;

    virtual DOMString getInputEncoding() const throw () = 0;

    virtual DOMString getXmlEncoding() const throw () = 0;

    virtual DOMString getXmlVersion() const throw () = 0;
};



class Document: public virtual Node {
public:
    virtual ref<DocumentType> getDoctype() const throw () = 0;

    virtual ref<DOMImplementation> getImplementation() const throw () = 0;

    virtual ref<Element> getDocumentElement() const throw () = 0;

    virtual ref<Element> createElement(const DOMString& tagName) const throw (DOMException) = 0;

    virtual ref<DocumentFragment> createDocumentFragment() const throw () = 0;

    virtual ref<Text> createTextNode(const DOMString& data) const throw () = 0;

    virtual ref<Comment> createComment(const DOMString& data) const throw () = 0;

    virtual ref<CDATASection> createCDATASection(const DOMString& data) const
        throw(DOMException) = 0;

    virtual ref<ProcessingInstruction> createProcessingInstruction(const DOMString& target,
                                                                   const DOMString& data) const
        throw (DOMException) = 0;

    virtual ref<NodeList> getElementsByTagName(const DOMString& name) const throw () = 0;

    virtual ref<Element> createElementNS(const DOMString& namespaceURI,
                                         const DOMString& qualifiedName) const
        throw (DOMException) = 0;

    virtual ref<NodeList> getElementsByTagNameNS(const DOMString& namespaceURI,
                                                 const DOMString& localName) const throw () = 0;

    virtual DOMString getInputEncoding() const throw () = 0;

    virtual DOMString getXmlEncoding() const throw () = 0;

    virtual bool getXmlStandalone() const throw () = 0;

    virtual void setXmlStandalone(bool xmlStandalone) throw (DOMException) = 0;

    virtual DOMString getXmlVersion() const throw () = 0;

    virtual void setXmlVersion(const DOMString& xmlVersion) throw (DOMException) = 0;

    virtual DOMString getDocumentURI() const throw () = 0;

    virtual void setDocumentURI(const DOMString& documentURI) throw () = 0;

    virtual ref<Node> adoptNode(const ref<Node>& source) const throw (DOMException) = 0;
};



class DOMImplementation: public virtual DOMObject {
public:
    virtual bool hasFeature(const DOMString& feature,
                            const DOMString& version) const throw () = 0;

    virtual ref<DocumentType> createDocumentType(const DOMString& qualifiedName,
                                                 const DOMString& publicId,
                                                 const DOMString& systemId) const
        throw (DOMException) = 0;

    virtual ref<Document>
    createDocument(const DOMString& namespaceURI, const DOMString& qualifiedName,
                   const ref<DocumentType>& doctype) const throw (DOMException) = 0;

//    virtual ref<DOMObject> getFeature(const DOMString& feature,
//                                      const DOMString& version) const throw () = 0;
};


} // namespace dom
} // namespace archon

#endif // ARCHON_DOM_CORE_HPP
