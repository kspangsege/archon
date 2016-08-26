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

#include <iterator>
#include <vector>

#include <archon/core/text.hpp>
#include <archon/dom/impl/core.hpp>
#include <archon/dom/impl/ls.hpp>
#include <archon/dom/impl/html.hpp>
#include <archon/dom/impl/impl.hpp>


namespace {

using namespace archon::dom;


typedef ref<DOMImplementation> Impl;
typedef std::vector<Impl> Impls;

typedef std::pair<DOMString, DOMString> Feature;
typedef std::vector<Feature> Features;



struct List: DOMImplementationList {
    virtual ref<DOMImplementation> item(uint32 index) const throw ()
    {
        return impls.at(index);
    }

    virtual uint32 getLength() const throw ()
    {
        return impls.size();
    }

    Impls impls;

    virtual ~List() throw () {}
};



struct Source: DOMImplementationSource {
    virtual ref<DOMImplementation> getDOMImplementation(const DOMString& f) const throw ()
    {
        Features features;
        parse_features(f, features);

        Impls::const_iterator impls_end = impls.end();
        for (Impls::const_iterator i = impls.begin(); i != impls_end; ++i) {
            const Impl impl = *i;
            bool good = true;
            Features::const_iterator features_end = features.end();
            for (Features::const_iterator j = features.begin(); j != features_end; ++j) {
                const Feature feat = *j;
                if (!impl->hasFeature(feat.first, feat.second)) {
                    good = false;
                    break;
                }
            }
            if (good)
                return impl;
        }

        return nullptr;
    }


    virtual ref<DOMImplementationList> getDOMImplementationList(const DOMString& f) const throw ()
    {
        Features features;
        parse_features(f, features);

        ref<List> list(new List);

        Impls::const_iterator impls_end = impls.end();
        for (Impls::const_iterator i = impls.begin(); i != impls_end; ++i) {
            const Impl impl = *i;
            bool good = true;
            Features::const_iterator features_end = features.end();
            for (Features::const_iterator j = features.begin(); j != features_end; ++j) {
                const Feature feat = *j;
                if (!impl->hasFeature(feat.first, feat.second)) {
                    good = false;
                    break;
                }
            }
            if (good)
                list->impls.push_back(impl);
        }

        return list;
    }


    Source(const Impls &i):
        impls(i) {}

    virtual ~Source() throw () {}


private:
    const Impls impls;


    static void parse_features(const DOMString& f, Features& features)
    {
        typedef DOMString::traits_type traits;
        DOMString space(1, traits::to_char_type(0x20));
        typedef std::vector<DOMString> Tokens;
        Tokens tokens;
        archon::core::Text::split(f, space, back_inserter(tokens), true);

        DOMString feature, version;
        Tokens::const_iterator tokens_end = tokens.end();
        for (Tokens::const_iterator i = tokens.begin(); i != tokens_end; ++i) {
            const DOMString tok = *i;
            if (!feature.empty() && version.empty()) {
                traits::int_type i = traits::to_int_type(tok[0]);
                if (i < 0x3A && 0x30 <= i) {
                    version = tok;
                    continue;
                }
            }
            if (!feature.empty())
                features.push_back(make_pair(feature, version));
            feature = tok;
            version.clear();
        }
        if (!feature.empty())
            features.push_back(make_pair(feature, version));
    }
};



ref<Source> new_source()
{
    Impls impls;

    impls.push_back(Impl(new archon::dom_impl::DOMImplementationLS));
    impls.push_back(Impl(new archon::dom_impl::HTMLImplementation));

    return ref<Source>(new Source(impls));
}

} // unnamed namespace



namespace archon {
namespace dom_impl {

dom::ref<dom::DOMImplementationSource> get_default_impl_src()
{
    static dom::ref<Source> source = new_source();
    return source;
}

} // namespace dom_impl
} // namespace archon
