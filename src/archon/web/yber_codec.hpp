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

#ifndef ARCHON_WEB_YBER_CODEC_HPP
#define ARCHON_WEB_YBER_CODEC_HPP

#include <memory>

#include <archon/core/codec.hpp>


namespace archon {
namespace web {

/// Get the codec object for the Yber encoding. Internal encoding is UTF-8, that
/// is, when you decode an Yber encoded string, you get a UTF-8 encoded string.
///
/// \note The 'Y' is pronounced like the german U with an umlaut.
std::unique_ptr<const core::Codec> get_yber_codec();

} // namespace web
} // namespace archon

#endif // ARCHON_WEB_YBER_CODEC_HPP
