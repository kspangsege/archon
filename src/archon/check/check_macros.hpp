// This file is part of the Archon project, a suite of C++ libraries.
//
// Copyright (C) 2020 Kristian Spangsege <kristian.spangsege@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef ARCHON_X_CHECK_X_CHECK_MARCOS_HPP
#define ARCHON_X_CHECK_X_CHECK_MARCOS_HPP

/// \file


#include <exception>


/// \{
///
/// \brief Condition checks.
///
/// These macros check that the specified condition evaluates to `true` and `false`
/// respectively.
///
/// The expansion of these macros assume that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// These macros are implemented in terms of \ref
/// archon::check::TestContext::check_general_cond().
///
/// \sa \ref ARCHON_CHECK_EQUAL()
///
#define ARCHON_CHECK(cond)     X_ARCHON_CHECK(cond)
#define ARCHON_CHECK_NOT(cond) X_ARCHON_CHECK_NOT(cond)
/// \}



/// \{
///
/// \brief Checks involving simple comparisons.
///
/// These macros compare the two specified arguments.
///
/// Unlike in the case or regular comparisons, these macros perform reliable comparisons
/// when arguments are of integer or floating-point type.
///
/// The expansion of these macros assume that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// These macros are implemented in terms of \ref archon::check::TestContext::check_equal(),
/// \ref archon::check::TestContext::check_not_equal(), \ref
/// archon::check::TestContext::check_less(), \ref
/// archon::check::TestContext::check_less_equal(), \ref
/// archon::check::TestContext::check_not_less(), \ref
/// archon::check::TestContext::check_not_less_equal(), \ref
/// archon::check::TestContext::check_greater(), \ref
/// archon::check::TestContext::check_greater_equal(), \ref
/// archon::check::TestContext::check_not_greater(), and \ref
/// archon::check::TestContext::check_not_greater_equal().
///
/// \sa \ref ARCHON_CHECK_DIST_LESS()
/// \sa \ref ARCHON_CHECK_BETWEEN()
/// \sa \ref ARCHON_CHECK_APPROXIMATELY_EQUAL()
/// \sa \ref ARCHON_CHECK_COMPARE()
///
#define ARCHON_CHECK_EQUAL(a, b)             X_ARCHON_CHECK_EQUAL(a, b)
#define ARCHON_CHECK_NOT_EQUAL(a, b)         X_ARCHON_CHECK_NOT_EQUAL(a, b)
#define ARCHON_CHECK_LESS(a, b)              X_ARCHON_CHECK_LESS(a, b)
#define ARCHON_CHECK_LESS_EQUAL(a, b)        X_ARCHON_CHECK_LESS_EQUAL(a, b)
#define ARCHON_CHECK_NOT_LESS(a, b)          X_ARCHON_CHECK_NOT_LESS(a, b)
#define ARCHON_CHECK_NOT_LESS_EQUAL(a, b)    X_ARCHON_CHECK_NOT_LESS_EQUAL(a, b)
#define ARCHON_CHECK_GREATER(a, b)           X_ARCHON_CHECK_GREATER(a, b)
#define ARCHON_CHECK_GREATER_EQUAL(a, b)     X_ARCHON_CHECK_GREATER_EQUAL(a, b)
#define ARCHON_CHECK_NOT_GREATER(a, b)       X_ARCHON_CHECK_NOT_GREATER(a, b)
#define ARCHON_CHECK_NOT_GREATER_EQUAL(a, b) X_ARCHON_CHECK_NOT_GREATER_EQUAL(a, b)
/// \}



/// \{
///
/// \brief Checks involving distance between two values.
///
/// These macros determine the distance between the first two arguments, \p a and \p b, and
/// then check that it relates correctly to the specified distance, \p dist. When all
/// arguments (\p a, \p b, and \p dist) have integer type, the check is exact.
///
/// `ARCHON_CHECK_DIST_LESS(a, b, dist)` checks that the distance between \p a and \p b is
/// less than \p dist with distance determination and comparison performed by \ref
/// archon::check::TestContext::dist_less().
///
/// `ARCHON_CHECK_DIST_LESS_EQUAL(a, b, dist)` checks that the distance between \p a and \p
/// b is less than, or equal to \p dist with distance determination and comparison performed
/// by \ref archon::check::TestContext::dist_less_equal().
///
/// `ARCHON_CHECK_DIST_GREATER(a, b, dist)` checks that the distance between \p a and \p b
/// is greater than \p dist with distance determination and comparison performed by \ref
/// archon::check::TestContext::dist_greater().
///
/// `ARCHON_CHECK_DIST_GREATER_EQUAL(a, b, dist)` checks that the distance between \p a and
/// \p b is greater than, or equal to \p dist with distance determination and comparison
/// performed by \ref archon::check::TestContext::dist_greater_equal().
///
/// The macros that have `NOT` in their names check a condition that is the logical negation
/// of the condition checked in the respective macros without the `NOT`.
///
/// The expansion of these macros assume that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// These macros are implemented in terms of \ref
/// archon::check::TestContext::check_dist_less(), \ref
/// archon::check::TestContext::check_dist_less_equal(), \ref
/// archon::check::TestContext::check_dist_not_less(), \ref
/// archon::check::TestContext::check_dist_not_less_equal(), \ref
/// archon::check::TestContext::check_dist_greater(), \ref
/// archon::check::TestContext::check_dist_greater_equal(), \ref
/// archon::check::TestContext::check_dist_not_greater(), and \ref
/// archon::check::TestContext::check_dist_not_greater_equal().
///
/// \sa \ref ARCHON_CHECK_EQUAL()
///
#define ARCHON_CHECK_DIST_LESS(a, b, dist)              X_ARCHON_CHECK_DIST_LESS(a, b, dist)
#define ARCHON_CHECK_DIST_LESS_EQUAL(a, b, dist)        X_ARCHON_CHECK_DIST_LESS_EQUAL(a, b, dist)
#define ARCHON_CHECK_DIST_NOT_LESS(a, b, dist)          X_ARCHON_CHECK_DIST_NOT_LESS(a, b, dist)
#define ARCHON_CHECK_DIST_NOT_LESS_EQUAL(a, b, dist)    X_ARCHON_CHECK_DIST_NOT_LESS_EQUAL(a, b, dist)
#define ARCHON_CHECK_DIST_GREATER(a, b, dist)           X_ARCHON_CHECK_DIST_GREATER(a, b, dist)
#define ARCHON_CHECK_DIST_GREATER_EQUAL(a, b, dist)     X_ARCHON_CHECK_DIST_GREATER_EQUAL(a, b, dist)
#define ARCHON_CHECK_DIST_NOT_GREATER(a, b, dist)       X_ARCHON_CHECK_DIST_NOT_GREATER(a, b, dist)
#define ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(a, b, dist) X_ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(a, b, dist)
/// \}



/// \{
///
/// \brief Checks involving presence of value in interval.
///
/// These macros check whether \p x is, or is not between the specified minimum (\p min) and
/// maximum (\p max) values. \p x is between those values if \p min is less than, or equal
/// to \p x; and \p x is less than, or equal to \p max.
///
/// The expansion of these macros assume that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// These macros are implemented in terms of \ref
/// archon::check::TestContext::check_between() and \ref
/// archon::check::TestContext::check_not_between().
///
/// \sa \ref ARCHON_CHECK_EQUAL()
///
#define ARCHON_CHECK_BETWEEN(x, min, max)     X_ARCHON_CHECK_BETWEEN(x, min, max)
#define ARCHON_CHECK_NOT_BETWEEN(x, min, max) X_ARCHON_CHECK_NOT_BETWEEN(x, min, max)
/// \}



/// \{
///
/// \brief Inexact relational checks.
///
/// These are the inexact floating point comparisons defined by Donald. E. Knuth. in volume
/// II of his "The Art of Computer Programming" 3rd edition, section 4.2.2 "Accuracy of
/// Floating Point Arithmetic", definitions (21)-(24):
///
///   | Comparison                  | Meaning
///   |-----------------------------|-------------------------------------------
///   | approximately equal         | abs(a-b) <= max(abs(a), abs(b)) * epsilon
///   | essentially equal           | abs(a-b) <= min(abs(a), abs(b)) * epsilon
///   | not approximately equal     | abs(a-b) >  max(abs(a), abs(b)) * epsilon
///   | not essentially equal       | abs(a-b) >  min(abs(a), abs(b)) * epsilon
///   | definitely less than        | b - a    >  max(abs(a), abs(b)) * epsilon
///   | definitely greater than     | a - b    >  max(abs(a), abs(b)) * epsilon
///   | not definitely less than    | b - a    <= max(abs(a), abs(b)) * epsilon
///   | not definitely greater than | a - b    <= max(abs(a), abs(b)) * epsilon
///
/// In general you should set \p epsilon to some small multiple of the machine epsilon for
/// the floating point type used in your computations
/// (e.g. `std::numeric_limits<double>::epsilon()`). As a general rule, a longer and more
/// complex computation needs a higher multiple of the machine epsilon.
///
/// The expansion of these macros assume that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// These macros are implemented in terms of \ref
/// archon::check::TestContext::check_approximately_equal(), \ref
/// archon::check::TestContext::check_essentially_equal(), \ref
/// archon::check::TestContext::check_not_approximately_equal(), \ref
/// archon::check::TestContext::check_not_essentially_equal(), \ref
/// archon::check::TestContext::check_definitelt_less(), \ref
/// archon::check::TestContext::check_definitelt_greater(), \ref
/// archon::check::TestContext::check_not_definitelt_less(), and \ref
/// archon::check::TestContext::check_not_definitelt_greater().
///
#define ARCHON_CHECK_APPROXIMATELY_EQUAL(a, b, epsilon)     X_ARCHON_CHECK_APPROXIMATELY_EQUAL(a, b, epsilon)
#define ARCHON_CHECK_ESSENTIALLY_EQUAL(a, b, epsilon)       X_ARCHON_CHECK_ESSENTIALLY_EQUAL(a, b, epsilon)
#define ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(a, b, epsilon) X_ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(a, b, epsilon)
#define ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(a, b, epsilon)   X_ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(a, b, epsilon)
#define ARCHON_CHECK_DEFINITELY_LESS(a, b, epsilon)         X_ARCHON_CHECK_DEFINITELY_LESS(a, b, epsilon)
#define ARCHON_CHECK_DEFINITELY_GREATER(a, b, epsilon)      X_ARCHON_CHECK_DEFINITELY_GREATER(a, b, epsilon)
#define ARCHON_CHECK_NOT_DEFINITELY_LESS(a, b, epsilon)     X_ARCHON_CHECK_NOT_DEFINITELY_LESS(a, b, epsilon)
#define ARCHON_CHECK_NOT_DEFINITELY_GREATER(a, b, epsilon)  X_ARCHON_CHECK_NOT_DEFINITELY_GREATER(a, b, epsilon)
/// \}



/// \brief Custom comparison check.
///
/// This macro compares the two specified arguments using the specified comparison
/// operation.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro is implemented in terms of \ref archon::check::TestContext::check_compare().
///
#define ARCHON_CHECK_COMPARE(a, b, comp) X_ARCHON_CHECK_COMPARE(a, b, comp)



/// \brief Check that expression throws right kind of exception.
///
/// This macro checks that the evaluation of the specified expression causes an exception of
/// the specified class to be thrown.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro expands to code that calls \ref archon::check::TestContext::check_succeeded()
/// if an exception of the specified type was thrown, and that calls \ref
/// archon::check::TestContext::check_throw_failed() with appropriate arguments if no
/// exception was thrown. If an exception of a different type is thrown, the that exception
/// will terminate the test, and the test will be registered as failed in the same was as if
/// some other code has thrown that exception.
///
#define ARCHON_CHECK_THROW(expr, exception_class) X_ARCHON_CHECK_THROW(expr, exception_class)


/// \brief Check that expression throws right kind of exception.
///
/// This macro is like \ref ARCHON_CHECK_THROW() except that it also checks that the
/// specified exception condition (\p exception_cond) evaluates to `true`. In the exception
/// condition, use the name \p e to refer to the caught exception.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro expands to code that calls \ref archon::check::TestContext::check_succeeded()
/// if an exception of the specified type was thrown and the exception condition evaluates
/// to `true`, that calls \ref archon::check::TestContext::check_throw_ex_failed() with
/// appropriate arguments if no exception was thrown, and that calls \ref
/// archon::check::TestContext::check_throw_ex_cond_failed() with appropriate arguments if
/// an exception of the specified type was thrown, but the exception condition evaluated to
/// `false`.
///
#define ARCHON_CHECK_THROW_EX(expr, exception_class, exception_cond)    \
    X_ARCHON_CHECK_THROW_EX(expr, exception_class, exception_cond)


/// \brief Check that expression throws exception of any type.
///
/// This macro checks that the evaluation of the specified expression causes an exception of
/// any type to be thrown.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro expands to code that calls \ref archon::check::TestContext::check_succeeded()
/// if an exception of any type was thrown, and that calls \ref
/// archon::check::TestContext::check_throw_any_failed() with appropriate arguments if no
/// exception was thrown.
///
#define ARCHON_CHECK_THROW_ANY(expr) X_ARCHON_CHECK_THROW_ANY(expr)


/// \brief Check that expression does not throw.
///
/// This macro checks that the evaluation of the specified expression does not throw an
/// exception. This is useful when it is appropriate for the execution of the test case to
/// continue even if the expression throws.
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro expands to code that calls \ref archon::check::TestContext::check_succeeded()
/// if no exception was thrown, and that calls \ref
/// archon::check::TestContext::check_nothrow_failed() with appropriate arguments if an
/// exception was thrown.
///
#define ARCHON_CHECK_NOTHROW(expr) X_ARCHON_CHECK_NOTHROW(expr)


/// \brief Check equality of two sequences.
///
/// This macro checks that the two specified sequences are equal, i.e., that they have the
/// same length, and that their elements are stepwise equal. The types of the specified
/// sequences must be such that `std::begin()` and `std::end()` return sensible
/// values. Element equality is checked as if by \ref ARCHON_CHECK_EQUAL().
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro is implemented in terms of \ref archon::check::TestContext::check_equal_seq().
///
#define ARCHON_CHECK_EQUAL_SEQ(a, b) X_ARCHON_CHECK_EQUAL_SEQ(a, b)


/// \brief Custom sequence comparison.
///
/// This macro is like \ref ARCHON_CHECK_EQUAL_SEQ, but allows for a custom comparison
/// operation to be specified (\p comp).
///
/// The expansion of this macro assumes that a variable named `test_context` of type
/// `TestContext&` is available (see \ref ARCHON_TEST()).
///
/// This macro is implemented in terms of \ref
/// archon::check::TestContext::check_compare_seq().
///
#define ARCHON_CHECK_COMPARE_SEQ(a, b, comp) X_ARCHON_CHECK_COMPARE_SEQ(a, b, comp)








// Implementation


#define X_ARCHON_CHECK(cond)                                            \
    test_context.check_general_cond(bool(cond), __FILE__, __LINE__, "ARCHON_CHECK", #cond)


#define X_ARCHON_CHECK_NOT(cond)                                        \
    test_context.check_general_cond(!bool(cond), __FILE__, __LINE__, "ARCHON_CHECK_NOT", #cond)



// ============================ Exact comparisons ============================


#define X_ARCHON_CHECK_EQUAL(a, b)                                      \
    test_context.check_equal(a, b, "ARCHON_CHECK_EQUAL", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_EQUAL(a, b)                                  \
    test_context.check_not_equal(a, b, "ARCHON_CHECK_NOT_EQUAL", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_LESS(a, b)                                       \
    test_context.check_less(a, b, "ARCHON_CHECK_LESS", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_LESS_EQUAL(a, b)                                 \
    test_context.check_less_equal(a, b, "ARCHON_CHECK_LESS_EQUAL", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_LESS(a, b)                                   \
    test_context.check_not_less(a, b, "ARCHON_CHECK_NOT_LESS", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_LESS_EQUAL(a, b)                             \
    test_context.check_not_less_equal(a, b, "ARCHON_CHECK_NOT_LESS_EQUAL", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_GREATER(a, b)                                    \
    test_context.check_greater(a, b, "ARCHON_CHECK_GREATER", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_GREATER_EQUAL(a, b)                              \
    test_context.check_greater_equal(a, b, "ARCHON_CHECK_GREATER_EQUAL", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_GREATER(a, b)                                \
    test_context.check_not_greater(a, b, "ARCHON_CHECK_NOT_GREATER", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_GREATER_EQUAL(a, b)                          \
    test_context.check_not_greater_equal(a, b, "ARCHON_CHECK_NOT_GREATER_EQUAL", #a, #b, __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_LESS(a, b, dist)                            \
    test_context.check_dist_less(a, b, dist, "ARCHON_CHECK_DIST_LESS", #a, #b, #dist, __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_LESS_EQUAL(a, b, dist)                      \
    test_context.check_dist_less_equal(a, b, dist, "ARCHON_CHECK_DIST_LESS_EQUAL", #a, #b, #dist, __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_NOT_LESS(a, b, dist)                        \
    test_context.check_dist_not_less(a, b, dist, "ARCHON_CHECK_DIST_NOT_LESS", #a, #b, #dist, __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_NOT_LESS_EQUAL(a, b, dist)                  \
    test_context.check_dist_not_less_equal(a, b, dist, "ARCHON_CHECK_DIST_NOT_LESS_EQUAL", #a, #b, #dist, \
                                           __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_GREATER(a, b, dist)                         \
    test_context.check_dist_greater(a, b, dist, "ARCHON_CHECK_DIST_GREATER", #a, #b, #dist, __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_GREATER_EQUAL(a, b, dist)                   \
    test_context.check_dist_greater_equal(a, b, dist, "ARCHON_CHECK_DIST_GREATER_EQUAL", #a, #b, #dist, \
                                          __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_NOT_GREATER(a, b, dist)                     \
    test_context.check_dist_not_greater(a, b, dist, "ARCHON_CHECK_DIST_NOT_GREATER", #a, #b, #dist, __FILE__, __LINE__)


#define X_ARCHON_CHECK_DIST_NOT_GREATER_EQUAL(a, b, dist)               \
    test_context.check_dist_not_greater_equal(a, b, dist, "ARCHON_CHECK_DIST_NOT_GREATER_EQUAL", #a, #b, #dist, \
                                              __FILE__, __LINE__)


#define X_ARCHON_CHECK_BETWEEN(x, min, max)                             \
    test_context.check_between(x, min, max, "ARCHON_CHECK_BETWEEN", #x, #min, #max, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_BETWEEN(x, min, max)                         \
    test_context.check_not_between(x, min, max, "ARCHON_CHECK_NOT_BETWEEN", #x, #min, #max, __FILE__, __LINE__)



// ============================ Inexact comparisons ============================


#define X_ARCHON_CHECK_APPROXIMATELY_EQUAL(a, b, epsilon)               \
    test_context.check_approximately_equal(a, b, epsilon, "ARCHON_CHECK_APPROXIMATELY_EQUAL", #a, #b, #epsilon, \
                                           __FILE__, __LINE__)


#define X_ARCHON_CHECK_ESSENTIALLY_EQUAL(a, b, epsilon)                 \
    test_context.check_essentially_equal(a, b, epsilon, "ARCHON_CHECK_ESSENTIALLY_EQUAL", #a, #b, #epsilon, \
                                         __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL(a, b, epsilon)           \
    test_context.check_not_approximately_equal(a, b, epsilon, "ARCHON_CHECK_NOT_APPROXIMATELY_EQUAL", #a, #b, \
                                               #epsilon, __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL(a, b, epsilon)             \
    test_context.check_not_essentially_equal(a, b, epsilon, "ARCHON_CHECK_NOT_ESSENTIALLY_EQUAL", #a, #b, #epsilon, \
                                             __FILE__, __LINE__)


#define X_ARCHON_CHECK_DEFINITELY_LESS(a, b, epsilon)                   \
    test_context.check_definitely_less(a, b, epsilon, "ARCHON_CHECK_DEFINITELY_LESS", #a, #b, #epsilon, \
                                       __FILE__, __LINE__)


#define X_ARCHON_CHECK_DEFINITELY_GREATER(a, b, epsilon)                \
    test_context.check_definitely_greater(a, b, epsilon, "ARCHON_CHECK_DEFINITELY_GREATER", #a, #b, #epsilon, \
                                          __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_DEFINITELY_LESS(a, b, epsilon)               \
    test_context.check_not_definitely_less(a, b, epsilon, "ARCHON_CHECK_NOT_DEFINITELY_LESS", #a, #b, #epsilon, \
                                           __FILE__, __LINE__)


#define X_ARCHON_CHECK_NOT_DEFINITELY_GREATER(a, b, epsilon)            \
    test_context.check_not_definitely_greater(a, b, epsilon, "ARCHON_CHECK_NOT_DEFINITELY_GREATER", #a, #b, #epsilon, \
                                              __FILE__, __LINE__)



// ============================ Custom comparisons ============================


#define X_ARCHON_CHECK_COMPARE(a, b, comp)                              \
    test_context.check_compare(a, b, comp, __FILE__, __LINE__, #a, #b, #comp)



// ============================ Exceptions ============================


#define X_ARCHON_CHECK_THROW(expr, exception_class)                     \
    ([&] {                                                              \
        try {                                                           \
            expr;                                                       \
            test_context.check_throw_failed(__FILE__, __LINE__, #expr, #exception_class); \
        }                                                               \
        catch (exception_class&) {                                      \
            test_context.check_succeeded();                             \
            return true;                                                \
        }                                                               \
        return false;                                                   \
    }())


#define X_ARCHON_CHECK_THROW_EX(expr, exception_class, exception_cond)  \
    ([&] {                                                              \
        try {                                                           \
            expr;                                                       \
            test_context.check_throw_ex_failed(__FILE__, __LINE__, #expr, #exception_class, #exception_cond); \
        }                                                               \
        catch (exception_class& e) {                                    \
            static_cast<void>(e);                                       \
            if (exception_cond) {                                       \
                test_context.check_succeeded();                         \
                return true;                                            \
            }                                                           \
            test_context.check_throw_ex_cond_failed(__FILE__, __LINE__, #expr, #exception_class, #exception_cond); \
        }                                                               \
        return false;                                                   \
    }())


#define X_ARCHON_CHECK_THROW_ANY(expr)                                  \
    ([&] {                                                              \
        try {                                                           \
            expr;                                                       \
            test_context.check_throw_any_failed(__FILE__, __LINE__, #expr);   \
        }                                                               \
        catch (...) {                                                   \
            test_context.check_succeeded();                             \
            return true;                                                \
        }                                                               \
        return false;                                                   \
    }())


#define X_ARCHON_CHECK_NOTHROW(expr)                                    \
    ([&] {                                                              \
        try {                                                           \
            expr;                                                       \
            test_context.check_succeeded();                             \
            return true;                                                \
        }                                                               \
        catch (std::exception& e) {                                     \
            test_context.check_nothrow_failed(__FILE__, __LINE__, #expr, &e); \
        }                                                               \
        catch (...) {                                                   \
            test_context.check_nothrow_failed(__FILE__, __LINE__, #expr, nullptr); \
        }                                                               \
        return false;                                                   \
    }())



// ============================ Sequence comparison ============================


#define X_ARCHON_CHECK_EQUAL_SEQ(a, b)                                  \
    test_context.check_equal_seq(a, b, __FILE__, __LINE__, #a, #b)


#define X_ARCHON_CHECK_COMPARE_SEQ(a, b, comp)                          \
    test_context.check_compare_seq(a, b, comp, __FILE__, __LINE__, #a, #b, #comp)


#endif // ARCHON_X_CHECK_X_CHECK_MARCOS_HPP
