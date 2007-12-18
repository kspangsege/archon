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

#include <cstdlib>
#include <exception>
#include <memory>
#include <locale>
#include <vector>
#include <iostream>

#include <GL/gl.h>

#include <archon/features.h>
#include <archon/core/build_config.hpp>
#include <archon/core/options.hpp>
#include <archon/core/logger.hpp>
#include <archon/core/sys.hpp>
#include <archon/dom/impl/html.hpp>
#include <archon/dom/impl/html_parser.hpp>
#include <archon/display/implementation.hpp>
#include <archon/display/connection.hpp>
#include <archon/display/window.hpp>
#include <archon/display/event.hpp>
#include <archon/render/texture_cache.hpp>
#include <archon/render/dom_renderer.hpp>


using namespace Archon::Core;
using namespace Archon::DomImpl;
using namespace Archon::Display;
using namespace Archon::Render;

namespace dom = Archon::dom;


namespace {

class DomBuilder: public HtmlParser::Callbacks {
public:
    DomBuilder(const dom::ref<HTMLImplementation>& i):
        impl(i),
        level(0)
    {
        tag_script = utf16_from_port("script");
        tag_style  = utf16_from_port("style");
    }

    dom::ref<HTMLDocument> get_doc()
    {
        return doc;
    }

    void doctype_begin(const StringUtf16& name,
                       const StringUtf16& public_id,
                       const StringUtf16& system_id) override
    {
        flush_text();

        std::cout << "<!DOCTYPE " << encode(decode(name)) << " "
            "PUBLIC \"" << encode(decode(public_id)) << "\""
            " \"" << encode(decode(system_id)) << "\" " << "[\n";
    }

    void doctype_end() override
    {
        flush_text();

        std::cout << "]>\n";
    }

    void elem_begin(const StringUtf16& name,
                    const std::vector<HtmlParser::Attr>& attribs) override
    {
        flush_text();

        std::cout << format_start_tag(name, attribs) << "\n";

        if (!doc)
            create_doc(0);

        // Introduce new level
        const Level* parent_level = 0;
        {
            auto n = levels.size();
            levels.resize(n+1);
            level = &levels.back();
            if (n)
                parent_level = &levels[n-1];
        }

        // Create a new element
        {
            ParentNode* parent =
                parent_level ? static_cast<ParentNode*>(parent_level->elem.get()) : doc.get();
            dom::DOMString ns, prefix, local_name; // All are empty in DOM Level 1.
            level->elem.reset(doc->create_elem_child_for_parser(parent, ns, name, prefix, local_name));
            level->is_element_content = level->elem->get_type()->is_element_content();
        }

        // Apply attributes
        {
            Element* elem = level->elem.get();
            for (const HtmlParser::Attr& attr: attribs) {
                elem->setAttribute(attr.name, attr.value);
/*
                dom::DOMString ns2;
                if (attr.prefix.empty()) {
                    if (attr.local_name == str_xmlns)
                        ns2 = str_ns_xmlns;
                }
                else {
                    NamespaceMap::const_iterator j = namespace_map.find(attr.prefix);
                    if (j == namespace_map.end() || j->second.empty())
                        throw dom::ls::LSException(dom::ls::PARSE_ERR, "Unbound namespace prefix");
                    ns2 = j->second;
                }
                elem->set_attrib_for_parser(ns2, attr.name, attr.prefix, attr.local_name, attr.value);
*/
            }
        }
    }

    void elem_end(const StringUtf16& name) override
    {
        flush_text();

        std::cout << "</" << encode(decode(name)) << ">\n";

        levels.pop_back();
        level = levels.empty() ? 0 : &levels.back();
    }

    void script(const std::vector<HtmlParser::Attr>& attribs, InlineStream& inline_script,
                HtmlParser::DocWriter& doc) override
    {
        elem_begin(tag_script, attribs);
        std::cout << quote(decode(inline_script.read_all(70))) << "\n";
        doc.write(utf16_from_port(" Odif\nRalf "));
        elem_end(tag_script);
    }

    void style(const std::vector<HtmlParser::Attr>& attribs, InlineStream& inline_style) override
    {
        elem_begin(tag_style, attribs);
        std::cout << quote(decode(inline_style.read_all(70))) << "\n";
        elem_end(tag_style);
    }

    void text(const StringUtf16& chunk) override
    {
        std::wstring s = trimmer.trim(decode(chunk)).substr(0, 70);
        if (!s.empty())
            std::cout << quote(s) << "\n";

        text_accum += chunk;
    }

    void comment(const StringUtf16& text) override
    {
        flush_text();

        std::wstring s = decode(text);
        s = trimmer.trim(s);
        s = s.substr(0, 70);
        if (!s.empty())
            std::cout << "<!--"<<encode(s)<<"-->\n";

        if (doc) {
            level->elem->append_child_for_parser(new Comment(doc.get(), text));
            return;
        }

        pending_doc_nodes.push_back(dom::Node::COMMENT_NODE+0);
        pending_doc_node_args.push_back(text);
    }

    void proc_instr(const StringUtf16& text) override
    {
        flush_text();

        std::wstring s = trimmer.trim(decode(text)).substr(0, 70);
        if (!s.empty())
            std::cout << "<?"<<encode(s)<<">\n";

        dom::DOMString target, data;
        if (!HtmlParser::parse_xml_proc_instr(text, target, data))
            data = text;

        if (doc) {
            level->elem->append_child_for_parser(new ProcessingInstruction(doc.get(), target, data));
            return;
        }

        pending_doc_nodes.push_back(dom::Node::PROCESSING_INSTRUCTION_NODE+0);
        pending_doc_node_args.push_back(target);
        pending_doc_node_args.push_back(data);
    }

private:
    const dom::ref<HTMLImplementation> impl;
    const dom::DOMString document_uri;   // Empty if unknown
    const dom::DOMString input_encoding; // Empty if same as xml_encoding

    dom::ref<HTMLDocument> doc;


    struct Level {
        dom::ref<Element> elem;
        bool is_element_content; // FIXME: Should be a pointer to a schema type information instance acquired from elem->get_type().
    };

    std::vector<Level> levels;

    Level* level;


    dom::DOMString text_accum;

    void flush_text()
    {
        if (text_accum.empty())
            return;

        if (level) {
            bool elem_cont_whitespace = level->is_element_content &&
                DOMImplementation::is_whitespace(text_accum);
            level->elem->append_child_for_parser(new Archon::DomImpl::Text(doc.get(), text_accum,
                                                                           elem_cont_whitespace));
        }
        text_accum.clear();
    }


    std::vector<dom::uint16> pending_doc_nodes;
    std::vector<dom::DOMString> pending_doc_node_args;

    void create_doc(DocumentType*)
    {
        doc.reset(new HTMLDocument(&*impl, HTMLDocument::mode_HTML_Strict));
//        doc = impl->create_document(doctype);
        HTMLDocument::XmlVersion xml_version = HTMLDocument::xml_ver_1_0; // FIXME: Supposed to be unspecified (null) for an HTML document, since it has no XML declaration
        dom::DOMString xml_encoding; // Empty since an HTML document has no XML declaration
        bool xml_standalone = false; // False since an HTML document has no XML declaration
        doc->set_doc_info(document_uri, input_encoding, xml_version, xml_encoding, xml_standalone);

        // Add pending document nodes
        std::vector<dom::DOMString>::size_type arg_idx = 0;
        auto end = pending_doc_nodes.end();
        for (auto i = pending_doc_nodes.begin(); i != end; ++i) {
            switch (*i) {
                case dom::Node::PROCESSING_INSTRUCTION_NODE: {
                    dom::DOMString target = pending_doc_node_args[arg_idx++];
                    dom::DOMString data   = pending_doc_node_args[arg_idx++];
                    doc->append_child_for_parser(new ProcessingInstruction(doc.get(), target, data));
                    break;
                }
                case dom::Node::COMMENT_NODE: {
                    dom::DOMString data = pending_doc_node_args[arg_idx++];
                    doc->append_child_for_parser(new Comment(doc.get(), data));
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected pending document node type");
            }
        }

        pending_doc_nodes.clear();
        pending_doc_node_args.clear();
    }



    Text::WideTrimmer trimmer;
    Text::WideOptionalWordQuoter quoter;
    WideLocaleCodec codec;
    StringUtf16 tag_script, tag_style;

    std::string format_start_tag(const StringUtf16& name, const std::vector<HtmlParser::Attr>& a)
    {
        std::string s = '<' + encode(decode(name));
        auto e = a.end();
        for (auto i = a.begin(); i != e; ++i) {
            s += ' ';
            s += quote(decode(i->name), true);
            s += '=';
            s += quote(decode(i->value));
        }
        return s + '>';
    }

    std::wstring decode(const StringUtf16& s)
    {
        return utf16_to_wide(s, codec.getloc());
    }

    std::string encode(const std::wstring& s)
    {
        return codec.encode(s);
    }

    std::string quote(const std::wstring& s, bool optional = false)
    {
        return codec.encode(quoter.print(s, !optional));
    }
};



class CloseException: public std::exception
{
};

class EventHandlerImpl: public EventHandler {
public:
    void on_damage(const AreaEvent& e) override
    {
        int left = e.x, top = e.y;
        int width = e.width, height = e.height;
        int right = left + width, bottom = top + height;

        int gl_left   = left;
        int gl_right  = right;
        int gl_top    = win_height - top;
        int gl_bottom = win_height - bottom;
        glViewport(gl_left, gl_bottom, width, height);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(gl_left, gl_right, gl_bottom, gl_top, -1, 1);
        glMatrixMode(GL_MODELVIEW);

        glColor3f(1,1,1);
        glBegin(GL_QUADS);
        glVertex2i(left, top);
        glVertex2i(left, bottom);
        glVertex2i(right, bottom);
        glVertex2i(right, top);
        glEnd();

        document.render(&dom_renderer);

        glFlush();
    }

    void on_resize(const SizeEvent& e) override
    {
        win_width  = e.width;
        win_height = e.height;
    }

    void on_keydown(const KeyEvent& e) override
    {
        switch (e.key_sym) {
            case KeySym_Control_L: // Modifier
                ctrl_left_down = true;
                break;
            case KeySym_Control_R: // Modifier
                ctrl_right_down = true;
                break;
            case KeySym_q:
                if (ctrl_left_down || ctrl_right_down)
                    throw CloseException();
                break;
            default:
                break;
        }
    }

    void on_keyup(const KeyEvent& e) override
    {
        switch (e.key_sym) {
            case KeySym_Control_L: // Modifier
                ctrl_left_down = false;
                break;
            case KeySym_Control_R: // Modifier
                ctrl_right_down = false;
                break;
            default:
                break;
        }
    }

    void on_close(const Event&) override
    {
        throw CloseException();
    }

    EventHandlerImpl(Window& win, int w, int h, DomRenderer& r, HTMLDocument& doc):
        window(win),
        win_width(w),
        win_height(h),
        dom_renderer(r),
        document(doc)
    {
    }

private:
    Window& window;
    int win_width, win_height;
    DomRenderer& dom_renderer;
    HTMLDocument& document;
    bool ctrl_left_down = false, ctrl_right_down = false;
};

} // anonymous namespace


int main(int argc, const char* argv[])
{
    std::locale::global(std::locale(""));

    const char* title = "Archon HTML browser";

    try_fix_preinstall_datadir(argv[0], "render/test/");
    std::string resource_dir = get_value_of(build_config_param_DataDir);

    bool opt_TreatWarningsAsErrors = false;
    bool opt_AbortOnError          = false;
    bool opt_CaseInsens            = false;
    bool opt_XhtmlCompat           = false;
    bool opt_CharencSwitch         = false;
    bool opt_ReportComments        = false;
    std::string opt_Charenc        = "";

    CommandlineOptions opts;
    opts.add_help(title, "URL");
    opts.check_num_args(0,1);
    opts.add_switch("e", "treat-warnings-as-errors", opt_TreatWarningsAsErrors, true,
                    "Treat warnings as errors");
    opts.add_switch("E", "abort-or-error", opt_AbortOnError, true, "Abort on error");
    opts.add_switch("i", "case-insens", opt_CaseInsens, true, "Turn off case sensitivity");
    opts.add_switch("x", "xhtml-compat", opt_XhtmlCompat, true, "Enable XHTML compatibility mode");
    opts.add_switch("s", "charenc-switch", opt_CharencSwitch, true, "Enable switching of character "
                    "encoding based on META tags with HTTP-EQUIV=\"Content-Type\"");
    opts.add_switch("C", "report-comments", opt_ReportComments, true, "Enable reporting of "
                    "comments");
    opts.add_switch("c", "charenc", opt_Charenc, opt_Charenc, "The character encoding "
                    "of the input, if it is known", true);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? EXIT_SUCCESS : EXIT_FAILURE;

    dom::ref<HTMLDocument> doc;
    {
        UniquePtr<InputStream> in;
        HtmlParser::DefaultResolver resolv;
        StringUtf16 uri;
        StringUtf16 charenc;
        if (argc < 2) {
            in.reset(make_stdin_stream().release());
            charenc = utf16_from_narrow(Sys::get_env_locale_charenc(), std::locale());
        }
        else {
            std::string path = argv[1];
            StringUtf16 public_ident;
            StringUtf16 system_ident = utf16_from_narrow(path, std::locale());
            StringUtf16 base_uri; // Empty means 'current working directory'
            resolv.resolve(public_ident, system_ident, base_uri, in, charenc, uri);
        }
        if (!opt_Charenc.empty())
            charenc = utf16_from_narrow(opt_Charenc, std::locale());
        HtmlParser::Source src(*in);
        src.system_ident = uri;
        src.charenc = charenc;
        dom::ref<HTMLImplementation> html_impl(new HTMLImplementation);
        DomBuilder dom_builder(html_impl);
        Logger* logger = &Logger::get_default_logger();
        HtmlParser::Config config;
        config.treat_warnings_as_errors      = opt_TreatWarningsAsErrors;
        config.die_on_first_error            = opt_AbortOnError;
        config.case_insensitive              = opt_CaseInsens;
        config.accept_xml_1_0_names          = opt_XhtmlCompat;
        config.enable_meta_charenc_switching = opt_CharencSwitch;
        config.report_comments               = opt_ReportComments;
        HtmlParser::parse_html(src, dom_builder, resolv, logger, config);
        doc = dom_builder.get_doc();
    }

    Implementation::Ptr impl = Archon::Display::get_default_implementation();
    Connection::Ptr conn = impl->new_connection();
    int screen = -1; // Default screen
    bool double_buffer = false; // Only front buffer is needed
    int visual = conn->choose_gl_visual(screen, double_buffer); // OpenGL rendering
    Context::Ptr context = conn->new_gl_context(screen, visual);
    Window::Ptr window = conn->new_window(512, 512, screen, visual);
    window->set_title(title);

    std::unique_ptr<TextureCache> texture_cache = make_texture_cache();
    DomRenderer dom_renderer(*texture_cache, resource_dir);

    EventHandlerImpl event_handler(*window, 512, 512, dom_renderer, *doc);
    EventProcessor::Ptr event_proc = conn->new_event_processor(&event_handler);
    event_proc->register_window(window);

    window->show();

    Bind b(context, window);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawBuffer(GL_FRONT);

    try {
        event_proc->process();
    }
    catch (CloseException&) {}
}
