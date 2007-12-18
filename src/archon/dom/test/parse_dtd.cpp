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
#include <archon/core/sys.hpp>
#include <archon/dom/impl/html_parser.hpp>


using namespace std;
using namespace Archon::Core;
using namespace Archon::DomImpl;

namespace {

class Callbacks: public HtmlParser::Callbacks {
public:
    void proc_instr(const StringUtf16& text) ARCHON_OVERRIDE
    {
        wstring s = trimmer.trim(decode(text)).substr(0, 70);
        if (!s.empty())
            cout << "<?"<<encode(s)<<">\n";
    }


    wstring decode(const StringUtf16& s)
    {
        return utf16_to_wide(s, codec.getloc());
    }

    string encode(const wstring& s)
    {
        return codec.encode(s);
    }

    string qoute(const wstring& s, bool optional = false)
    {
        return codec.encode(qouter.print(s, !optional));
    }


    Text::WideTrimmer trimmer;
    Text::WideOptionalWordQuoter qouter;
    WideLocaleCodec codec;
};

} // anonymous namespace


int main(int argc, const char* argv[]) throw ()
{
    locale::global(locale(""));

    bool   opt_TreatWarningsAsErrors = false;
    bool   opt_AbortOnError          = false;
    bool   opt_CaseInsens            = false;
    bool   opt_AllowXml10Names       = false;
    string opt_Charenc               = "";

    CommandlineOptions opts;
    opts.add_help("Testing the DTD parser", "URL");
    opts.check_num_args(0,1);
    opts.add_switch("e", "treat-warnings-as-errors", opt_TreatWarningsAsErrors, true,
                    "Treat warnings as errors");
    opts.add_switch("E", "abort-or-error", opt_AbortOnError, true, "Abort on error");
    opts.add_switch("i", "case-insens", opt_CaseInsens, true, "Turn off case sensitivity");
    opts.add_switch("a", "allow-xml10-names", opt_AllowXml10Names, true, "Allow XML 1.0 names");
    opts.add_switch("c", "charenc", opt_Charenc, opt_Charenc, "The character encoding "
                    "of the input, if it is known", true);
    if (int stop = opts.process(argc, argv))
        return stop == 2 ? 0 : 1;

    UniquePtr<InputStream> in;
    HtmlParser::DefaultResolver resolv;
    StringUtf16 uri;
    StringUtf16 charenc;
    if (argc < 2) {
        in.reset(make_stdin_stream().release());
        charenc = utf16_from_narrow(Sys::get_env_locale_charenc(), locale());
    }
    else {
        string path = argv[1];
        StringUtf16 public_ident;
        StringUtf16 system_ident = utf16_from_narrow(path, locale());
        StringUtf16 base_uri; // Empty means 'current working directory'
        resolv.resolve(public_ident, system_ident, base_uri, in, charenc, uri);
    }
    if (!opt_Charenc.empty())
        charenc = utf16_from_narrow(opt_Charenc, locale());
    HtmlParser::Source src(*in);
    src.system_ident = uri;
    src.charenc = charenc;
    src.base_uri = uri;
    Callbacks cb;
    Logger* logger = &Logger::get_default_logger();
    HtmlParser::Config config;
    config.treat_warnings_as_errors = opt_TreatWarningsAsErrors;
    config.die_on_first_error       = opt_AbortOnError;
    config.case_insensitive         = opt_CaseInsens;
    config.accept_xml_1_0_names     = opt_AllowXml10Names;
    HtmlParser::parse_dtd(src, cb, resolv, logger, config);
}
