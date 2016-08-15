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

#include <stdexcept>
#include <iostream>

#include <archon/util/nullable_string.hpp>


#define TEST(assertion)              if(!(assertion)) throw runtime_error("Test failed")
#define TEST_MSG(assertion, message) if(!(assertion)) throw runtime_error(message)


using namespace std;
using namespace archon::Core;
using namespace archon::Util;


int main() throw()
{
  NullableString<char> s1;
  TEST_MSG(!s1, "s1 should be null");
  TEST_MSG(s1 == s1, "s1 should be equal to itself");
  TEST_MSG(!(s1 != s1), "s1 should not be different from itself");
  NullableString<char> s2("Alpha");
  TEST_MSG(s2, "s2 should not be null");
  TEST_MSG(s2.size() == 5, "s2 has bad length");
  TEST_MSG(s2.size() != 5 || equal(s2.data(), s2.data()+5, "Alpha"), "s2 has bad data");
  TEST_MSG(!(s2 == s1), "s2 and s1 should not be equal");
  TEST_MSG(s2 != s1, "s2 and s1 should be different");
  TEST_MSG(s2 == s2, "s2 should be equal to itself");
  TEST_MSG(!(s2 != s2), "s2 should not be different from itself");
  NullableString<char> s3(s1);
  TEST_MSG(!s3, "s3 should be null");
  TEST_MSG(s3 == s1, "s3 and s1 should be equal");
  TEST_MSG(!(s3 != s1), "s3 and s1 should not be different");
  TEST_MSG(!(s3 == s2), "s3 and s2 should not be equal");
  TEST_MSG(s3 != s2, "s3 and s2 should be different");
  TEST_MSG(s3 == s3, "s3 should be equal to itself");
  TEST_MSG(!(s3 != s3), "s3 should not be different from itself");
  NullableString<char> s4(s2);
  TEST_MSG(s4, "s4 should not be null");
  TEST_MSG(s4.size() == 5, "s4 has bad length");
  TEST_MSG(s4.size() != 5 || equal(s4.data(), s4.data()+5, "Alpha"), "s4 has bad data");
  TEST_MSG(!(s4 == s1), "s4 and s1 should not be equal");
  TEST_MSG(s4 != s1, "s4 and s1 should be different");
  TEST_MSG(s4 == s2, "s4 and s2 should be equal");
  TEST_MSG(!(s4 != s2), "s4 and s2 should not be different");
  TEST_MSG(!(s4 == s3), "s4 and s3 should not be equal");
  TEST_MSG(s4 != s3, "s4 and s3 should be different");
  TEST_MSG(s4 == s4, "s4 should be equal to itself");
  TEST_MSG(!(s4 != s4), "s4 should not be different from itself");

  NullableString<char> s5("Beta");
  TEST_MSG(s5, "s5 should not be null");
  TEST_MSG(s5.size() == 4, "s5 has bad length");
  TEST_MSG(s5.size() != 4 || equal(s5.data(), s5.data()+4, "Beta"), "s5 has bad data");
  TEST_MSG(!(s5 == s1), "s5 and s1 should not be equal");
  TEST_MSG(s5 != s1, "s5 and s1 should be different");
  TEST_MSG(!(s5 == s2), "s5 and s2 should not be equal");
  TEST_MSG(s5 != s2, "s5 and s2 should be different");

  NullableString<char> s6("Beta");
  TEST_MSG(s6 == s5, "s6 and s5 should be equal");
  TEST_MSG(!(s6 != s5), "s6 and s6 should not be different");

  NullableString<char> s7("");
  TEST_MSG(s7, "s7 should not be null");
  TEST_MSG(s7.size() == 0, "s7 has bad length");
  TEST_MSG(!(s7 == s1), "s7 and s1 should not be equal");
  TEST_MSG(s7 != s1, "s7 and s1 should be different");
  TEST_MSG(!(s7 == s2), "s7 and s2 should not be equal");
  TEST_MSG(s7 != s2, "s7 and s2 should be different");

  NullableString<char> s8("");
  TEST_MSG(s8 == s7, "s8 and s7 should be equal");
  TEST_MSG(!(s8 != s7), "s8 and s7 should not be different");

  s1 = s1;
  TEST_MSG(!s1, "s1 should be null after self assignment");
  TEST_MSG(s1 == s1, "s1 should be equal to itself after self assignment");
  TEST_MSG(s1 == s3, "s1 should be equal to s3 after self assignment");

  s2 = s2;
  TEST_MSG(s2, "s2 should not be null after self assignment");
  TEST_MSG(s2.size() == 5, "s2 has bad length after self assignment");
  TEST_MSG(s2.size() != 5 || equal(s2.data(), s2.data()+5, "Alpha"),
                     "s2 has bad data after self assignment");
  TEST_MSG(s2 == s2, "s2 should be equal to itself after self assignment");
  TEST_MSG(s2 == s4, "s2 should be equal to s4 after self assignment");

  s1 = s2;
  TEST_MSG(s1, "s1 should not be null after after assigning s2 to s1");
  TEST_MSG(s1.size() == 5, "s1 has bad length after assigning s2 to s1");
  TEST_MSG(s1.size() != 5 || equal(s1.data(), s1.data()+5, "Alpha"),
                     "s1 has bad data after assigning s2 to s1");
  TEST_MSG(s1 == s2, "s1 and s2 should be equal after assigning s2 to s1");

  s4 = s3;
  TEST_MSG(!s4, "s4 should be null after assigning s3 to s4");
  TEST_MSG(s4 == s3, "s4 and s3 should be equal after assigning s3 to s4");

  cerr << "OK" << endl;

  return 0;
}
