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

#include <iostream>

#include <archon/features.h>
#include <archon/core/logger.hpp>
#include <archon/core/text.hpp>
#include <archon/core/options.hpp>
#include <archon/dom/impl/html_parser.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::DomImpl;

namespace {

class Callbacks: public HtmlParser::Callbacks {
public:
    void doctype_begin(const StringUtf16& name,
                       const StringUtf16& public_id,
                       const StringUtf16& system_id) ARCHON_OVERRIDE
    {
        cout << "<!DOCTYPE " << encode(decode(name)) << " "
            "PUBLIC \"" << encode(decode(public_id)) << "\""
            " \"" << encode(decode(system_id)) << "\" " << "[\n";
    }

    void doctype_end() ARCHON_OVERRIDE
    {
        cout << "]>\n";
    }

    void elem_begin(const StringUtf16& name,
                    const vector<HtmlParser::Attr>& a) ARCHON_OVERRIDE
    {
        cout << format_start_tag(name, a) << "\n";
    }

    void elem_end(const StringUtf16& name) ARCHON_OVERRIDE
    {
        cout << "</" << encode(decode(name)) << ">\n";
    }

    void script(const vector<HtmlParser::Attr>& a, InlineStream& inline_script,
                HtmlParser::DocWriter& doc) ARCHON_OVERRIDE
    {
        wstring s = decode(inline_script.read_all(70));
        cout << format_start_tag(utf16_from_port("SCRIPT"), a) << encode(s) << "</SCRIPT>\n";
        doc.write(utf16_from_port(" Odif\nRalf "));
    }

    void style(const vector<HtmlParser::Attr>& a, InlineStream& inline_style) ARCHON_OVERRIDE
    {
        wstring s = decode(inline_style.read_all(70));
        cout << format_start_tag(utf16_from_port("STYLE"), a) << encode(s) << "</STYLE>\n";
    }

    void text(const StringUtf16& chunk) ARCHON_OVERRIDE
    {
      wstring s = trimmer.trim(decode(chunk)).substr(0, 70);
      if (!s.empty())
          cout << quote(s) << "\n";
    }

    void comment(const StringUtf16& text) ARCHON_OVERRIDE
    {
      wstring s = trimmer.trim(decode(text)).substr(0, 70);
      if (!s.empty())
          cout << "<!--"<<encode(s)<<"-->\n";
    }

    void proc_instr(const StringUtf16& text) ARCHON_OVERRIDE
    {
      wstring s = trimmer.trim(decode(text)).substr(0, 70);
      if (!s.empty())
          cout << "<?"<<encode(s)<<">\n";
    }

private:
    string format_start_tag(const StringUtf16& name, const vector<HtmlParser::Attr>& a)
    {
        string s = '<' + encode(decode(name));
        typedef vector<HtmlParser::Attr>::const_iterator iter;
        iter e = a.end();
        for (iter i = a.begin(); i != e; ++i) {
            s += ' ';
            s += quote(decode(i->name), true);
            s += '=';
            s += quote(decode(i->value));
        }
        return s + '>';
    }

    wstring decode(const StringUtf16& s)
    {
        return utf16_to_wide(s, codec.getloc());
    }

    string encode(const wstring& s)
    {
        return codec.encode(s);
    }

    string quote(const wstring& s, bool optional = false)
    {
        return codec.encode(quoter.print(s, !optional));
    }


    Text::WideTrimmer trimmer;
    Text::WideOptionalWordQuoter quoter;
    WideLocaleCodec codec;
};

} // anonymous namespace


int main(int argc, const char* argv[]) throw ()
{
    locale::global(locale(""));

    bool   opt_TreatWarningsAsErrors = false;
    bool   opt_AbortOnError          = false;
    bool   opt_CaseInsens            = false;
    bool   opt_XhtmlCompat           = false;
    bool   opt_CharencSwitch         = false;
    bool   opt_ReportComments        = false;
    string opt_Charenc               = "";

    CommandlineOptions opts;
    opts.add_help("Testing the HTML parser", "URL");
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
        return stop == 2 ? 0 : 1;

    UniquePtr<InputStream> in(make_stdin_stream().release());
    HtmlParser::Source src(*in);
    src.charenc = utf16_from_port(opt_Charenc);
    Callbacks cb;
    HtmlParser::DefaultResolver resolv;
    Logger* logger = &Logger::get_default_logger();
    HtmlParser::Config config;
    config.treat_warnings_as_errors      = opt_TreatWarningsAsErrors;
    config.die_on_first_error            = opt_AbortOnError;
    config.case_insensitive              = opt_CaseInsens;
    config.accept_xml_1_0_names          = opt_XhtmlCompat;
    config.enable_meta_charenc_switching = opt_CharencSwitch;
    config.report_comments               = opt_ReportComments;
    HtmlParser::parse_html(src, cb, resolv, logger, config);
}
