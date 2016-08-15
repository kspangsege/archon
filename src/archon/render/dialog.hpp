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

#ifndef ARCHON_RENDER_DIALOG_HPP
#define ARCHON_RENDER_DIALOG_HPP

#include <archon/core/shared_ptr.hpp>
#include <archon/dom/html.hpp>


namespace archon {
namespace Render {

class Dialog {
public:
    typedef core::SharedPtr<Dialog> Ptr;
    typedef Ptr const &Arg;

    virtual void show() = 0;
    virtual void hide() = 0;

    virtual dom::ref<dom::html::HTMLDocument> get_dom() = 0;

    virtual ~Dialog() {}
};

} // namespace Render
} // namespace archon

#endif // ARCHON_RENDER_DIALOG_HPP
