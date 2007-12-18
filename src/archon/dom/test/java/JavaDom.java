import java.io.*;
import org.w3c.dom.bootstrap.*;
import org.w3c.dom.*;
import org.w3c.dom.ls.*;

/*
OVERRIDE        YES
        -- Oasis entity catalog for Extensible HTML 1.0 --

PUBLIC  "-//W3C//DTD XHTML 1.0 Strict//EN"        "xhtml1-strict.dtd"
PUBLIC  "-//W3C//DTD XHTML 1.0 Transitional//EN"  "xhtml1-transitional.dtd"
PUBLIC  "-//W3C//DTD XHTML 1.0 Frameset//EN"      "xhtml1-frameset.dtd"

        -- ISO latin 1 entity set for Extensible HTML (XML 1.0 format) --

PUBLIC  "-//W3C//ENTITIES Latin 1 for XHTML//EN"  "xhtml-lat1.ent"
PUBLIC  "-//W3C//ENTITIES Symbols for XHTML//EN" "xhtml-symbol.ent"
PUBLIC  "-//W3C//ENTITIES Special for XHTML//EN" "xhtml-special.ent"

SGMLDECL "xhtml1.dcl"
*/

class JavaDom
{
    public static void main(String[] args) throws ClassNotFoundException, InstantiationException,
        IllegalAccessException, FileNotFoundException
    {
        final DOMImplementationRegistry registry = DOMImplementationRegistry.newInstance();
        final DOMImplementation impl = registry.getDOMImplementation("XML 3.0");
        final DOMImplementationLS ls = (DOMImplementationLS)impl.getFeature("LS", "3.0");

        LSResourceResolver resolver = new LSResourceResolver() {
                public LSInput resolveResource(String type,
                                               String namespaceURI,
                                               String publicId,
                                               String systemId,
                                               String baseURI)
                {
                    String xhtml1_dtd_dir = "/home/kristian/archon/src/archon/dom/test/xhtml-1.0-dtd/";
                    String path = "";
                    System.err.println("Resolving type:         " + type);
                    System.err.println("Resolving namespaceURI: " + namespaceURI);
                    System.err.println("Resolving publicId:     " + publicId);
                    System.err.println("Resolving systemId:     " + systemId);
                    System.err.println("Resolving baseURI:      " + baseURI);
                    if (type.equals("http://www.w3.org/TR/REC-xml")) {

                        // XHTML 1.0

                        if (publicId.equals("-//W3C//DTD XHTML 1.0 Strict//EN")) {
                            path = xhtml1_dtd_dir + "xhtml1-strict.dtd";
                        }
                        else if (publicId.equals("-//W3C//DTD XHTML 1.0 Transitional//EN")) {
                            path = xhtml1_dtd_dir + "xhtml1-transitional.dtd";
                        }
                        else if (publicId.equals("-//W3C//DTD XHTML 1.0 Frameset//EN")) {
                            path = xhtml1_dtd_dir + "xhtml1-frameset.dtd";
                        }
                        else if (publicId.equals("-//W3C//ENTITIES Latin 1 for XHTML//EN")) {
                            path = xhtml1_dtd_dir + "xhtml-lat1.ent";
                        }
                        else if (publicId.equals("-//W3C//ENTITIES Symbols for XHTML//EN")) {
                            path = xhtml1_dtd_dir + "xhtml-symbol.ent";
                        }
                        else if (publicId.equals("-//W3C//ENTITIES Special for XHTML//EN")) {
                            path = xhtml1_dtd_dir + "xhtml-special.ent";
                        }
                        else if (publicId.equals("-//W3C//ENTITIES Special for XHTML//EN")) {
                            path = xhtml1_dtd_dir + "xhtml-special.ent";
                        }

                        // TEST
                        else if (publicId.equals("-//TEST//DTD TEST//EN")) {
                            path = "/home/kristian/archon/src/archon/dom/test/test-xml/test.dtd";
                        }
                    }
                    if (path.isEmpty()) {
                        System.err.println("Unknown resource");
                        System.exit(1);
                    }
                    LSInput i = ls.createLSInput();
                    try {
                        System.err.println("------------------> Resolves to: " + path);
                        i.setByteStream(new FileInputStream(path));
                    }
                    catch (FileNotFoundException e) {
                        System.err.println("File not found");
                        System.exit(1);
                    }
                    return i;
                }
            };

/*
        DocumentType doctype = impl.createDocumentType("html", "-//W3C//DTD XHTML 1.1//EN", "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd");
        Document doc = impl.createDocument("http://www.w3.org/1999/xhtml", "html", doctype);
        Element root = doc.getDocumentElement();
        Element elem = doc.createElement("body");
        root.appendChild(elem);
        Element elem2 = doc.createElementNS("", "body");
        elem.appendChild(elem2);
        dump(doc, "");
        System.out.println("NUM: " + doc.getElementsByTagName("*").getLength());
        System.out.println("NUM: " + doc.getElementsByTagNameNS("*", "").getLength());
*/


//        LSSerializer serializer = ls.createLSSerializer();
//        System.out.println(serializer.writeToString(doc));

        LSParser parser = ls.createLSParser(DOMImplementationLS.MODE_SYNCHRONOUS, null);

//        parser.getDomConfig().setParameter("entities", new Boolean(true));
        parser.getDomConfig().setParameter("validate", new Boolean(true));
        parser.getDomConfig().setParameter("schema-type", "http://www.w3.org/TR/REC-xml");
//        parser.getDomConfig().setParameter("schema-type", "http://www.w3.org/2001/XMLSchema");
//        parser.getDomConfig().setParameter("datatype-normalization", new Boolean(true));
//        parser.getDomConfig().setParameter("infoset", new Boolean(true));
        parser.getDomConfig().setParameter("resource-resolver", resolver);


        LSInput input = ls.createLSInput();
        input.setByteStream(System.in);
        input.setSystemId("/home/kristian/public_html/tests/test.xml");
        Document doc = parser.parse(input);

/*
        Element body = (Element)doc.getElementsByTagName("body").item(0);
        NamedNodeMap a1 = body.getAttributes();
        NamedNodeMap a2 = body.getAttributes();
        System.out.println(a1 == a2);

        Attr style = body.getAttributeNode("style");
        body.setAttribute("style", "viggo");
        Attr style_2 = body.getAttributeNode("style");
        System.out.println(style == style_2);
*/

        dump(doc, "");

/*
        Element e = doc.createElement("test");
        doc.getDocumentElement().appendChild(e);
        e.setAttribute("id", "barnach");
        dump(doc, "");
        doc.getDomConfig().setParameter("validate", new Boolean(true));
//        doc.getDomConfig().setParameter("schema-type", "http://www.w3.org/TR/REC-xml");
        doc.getDomConfig().setParameter("schema-type", "http://www.w3.org/2001/XMLSchema");
        doc.getDomConfig().setParameter("datatype-normalization", new Boolean(true));
        doc.getDomConfig().setParameter("infoset", new Boolean(true));
        doc.normalizeDocument();
        dump(doc, "");
*/
    }


    private static void dump(Node n, String ind)
    {
        if (n instanceof DocumentType) {
            DocumentType doctype = (DocumentType)n;

            System.out.println(ind + "Doctype: " + doctype.getName());
            System.out.println(ind + "    Public ID = " + doctype.getPublicId());
            System.out.println(ind + "    System ID = " + doctype.getSystemId());

            System.out.println(ind + "    Entities:");
            NamedNodeMap entities = doctype.getEntities();
            for (int i=0; i<entities.getLength(); ++i) {
                Entity e = (Entity)entities.item(i);
                dump(e, ind + "        ");
            }

            System.out.println(ind + "    Notations:");
            NamedNodeMap notations = doctype.getNotations();
            for (int i=0; i<notations.getLength(); ++i) {
                Notation e = (Notation)notations.item(i);
                dump(e, ind + "        ");
            }

            if (doctype.getInternalSubset() != null)
                System.out.println(ind + "    Internal subset = " + quote(doctype.getInternalSubset()));
            return;
        }

        if (n instanceof Document) {
            Document doc = (Document)n;
            System.out.println(ind + "Document: " + doc.getDocumentURI());
            System.out.println("    Input encoding = " + doc.getInputEncoding());
            System.out.println("    XML version    = " + doc.getXmlVersion());
            System.out.println("    XML encoding   = " + doc.getXmlEncoding());
            System.out.println("    XML standalone = " + doc.getXmlStandalone());
        }
        else if (n instanceof Element) {
            Element elem = (Element)n;
            System.out.println(ind + "Element: " + elem.getTagName());
        }
        else if (n instanceof Attr) {
            Attr attr = (Attr)n;
            System.out.println(ind + "Attr: " + attr.getName() + (attr.getSpecified()?"  (specified)":"  (not specified)") + (attr.isId()?"  (ID)":"  (not ID)"));
        }
        else if (n instanceof Text) {
            Text text = (Text)n;
            boolean iw = text.isElementContentWhitespace();
            System.out.println(ind + "Text: " + quote(n.getNodeValue()) + (iw ? "  (ignorable whitespace)":""));
        }
        else {
            System.out.println(ind + "Node: " + n.getNodeName());
            if (n.getNodeValue() != null) {
                System.out.println(ind + "    Value: " + quote(n.getNodeValue()));
            }
        }

        if (n.getNamespaceURI() != null) {
            System.out.println(ind + "    NS: " + n.getNamespaceURI());
        }
        NamedNodeMap attribs = n.getAttributes();
        if (attribs != null) {
            for (int i = 0; i < attribs.getLength(); ++i) {
                Node attr = attribs.item(i);
                dump(attr, ind + "    ");
            }
        }
        NodeList children = n.getChildNodes();
        for (int i = 0; i < children.getLength(); ++i) {
            Node child = children.item(i);
            dump(child, ind + "    ");
        }
    }


    private static String quote(String s)
    {
        if (s.matches("^\\p{Graph}+$")) return s;

        String t = "\"";
        for (int i=0; i<s.length(); ++i) {
            char c = s.charAt(i);
            if (c == ' ' || (""+c).matches("\\p{Graph}")) {
                t += c;
                continue;
            }

            switch (c) {
                case '\u0000': t += "\\0";  continue;
                case '\n':     t += "\\n";  continue;
                case '\t':     t += "\\t";  continue;
                case '\b':     t += "\\b";  continue;
                case '\r':     t += "\\r";  continue;
                case '\f':     t += "\\f";  continue;
                case '\\':     t += "\\\\"; continue;
                case '"' :     t += "\\\""; continue;
            }

            if ('\u00FF' < c) t += String.format("\\u%04X", c);
            else t += String.format("\\x%02X", c);
        }

        return t + "\"";
    }
}
