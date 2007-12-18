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

#include <stdexcept>

#include <archon/core/utf16.hpp>
#include <archon/core/char_enc.hpp>


using namespace std;


namespace Archon {
namespace Core {


StringUtf16 case_fold(const StringUtf16&)
{
    throw runtime_error("Not yet implemented"); // FIXME: Implement this using ICU!
}


StringUtf16 to_upper_case(const StringUtf16&)
{
    throw runtime_error("Not yet implemented"); // FIXME: Implement this using ICU!
}


StringUtf16 to_lower_case(const StringUtf16&)
{
    throw runtime_error("Not yet implemented"); // FIXME: Implement this using ICU!
}


StringUtf16 utf16_from_narrow(const string& str, const locale& loc)
{
    // FIXME: Consider introducing ARCHON_CHAR_ENC_IS_UTF8 to speed up
    // this when applicable.
    return utf16_from_wide(WideLocaleCodec(true, loc).decode(str), loc);
}


string utf16_to_narrow(const StringUtf16& str, const locale& loc)
{
    // FIXME: Consider introducing ARCHON_CHAR_ENC_IS_UTF8 to speed up
    // this when applicable.
    return WideLocaleCodec(true, loc).encode(utf16_to_wide(str, loc));
}


} // namespace Core
} // namespace Archon
