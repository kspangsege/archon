// General rules:
//
// - Lines should not be longer than 100 characters (for readability).
//
// - Source files should never contain characters ouside the portable character set as
//   defined by POSIX (for compatibility).
//
// - Source files should never include TAB characters (for consistency of presentation).
//
// - All macro names must consist entirely of upplercase letters, digits and underscores,
//   and they must contain at least two upper case letters.
//
// - A name that is valid for a macro (see above), is not valid for anything other than a
//   macro.


// Public macro names (those defined in public headers) must have `ARCHON_` as a prefix.

#define ARCHON_MY_MACRO 1


// A function name uses lowercase and its parts are separated by underscores.

my_type my_func()
{
    // Put the opening brace of a function body on the next line below the function
    // prototype. This also applies to class member functions.

    // Put all other opening braces on the same line as the syntax element to which the
    // brace is subordinate.

    // Use 4 spaces per indentation level (no tabs).

    if (...) {
        // ...
    }
    else {
        // ...
    }

    // Always put subordinate statements on a new line (for ease in debugging).

    if (...)
        return ...;

    // No space between type and '*' or '&'
    int* foo1 = ...;
    int& foo2 = ...;

    // `const` goes before the type (west const)
    const int foo3 = ...;

    // ... but not when 'const' operates on a pointer type
    const int* foo3 = ...; // 'const' operates on 'int' not 'int*'
    int* const foo4 = ...; // 'const' operates on 'int*'
    int* const* const foo5 = ...;
}


void my_func_2()
{
    // This indentation and brace placement style agrees with K&R style except for the
    // 'extra' indentation of 'cases' in a switch statement.

    switch (...) {
        case type::foo: {
            // ...
            break;
        }
        case type::bar: {
            // ...
            break;
        }
    }

    try {
        // ...
    }
    catch (...) {
        // ...
    }
}



// A name space name uses lowercase and its parts are separated by underscores.

namespace my_utils {

// No indentation inside name spaces.


// A Class name uses CamelCase with uppercase initial.

template<class T> class MyClass
    : public Base1
    , private Base2 {
public:
    // Public member variables do not have a `s_` or `m_` prefixes.
    int baz;

    MyClass(...)
        : Base2(...)
        , m_bar(7)
    {
        // ...
    }

private:
    // Nonpublic static member variables have prefix `s_`.
    static int s_foo;

    // Nonpublic regular member variables have prefix `m_`.
    int m_bar;
};


} // namespace my_utils

// Note that `:` and `,` break onto a new line when they occur in the "base clause" of a
// class declaration or in the "member initializer list" of a constructor definition.



// Order of class members (roughly):

class MyClass2 {
public:
    // Types

    // Variables

    // Functions

protected:
    // Same as 'public'

private:
    // Same as 'public'

    // Friends
};




// About FIXMEs:
//
// A FIXME conveys information about a known issue or shortcoming. It may also include
// information on how to fix the problem, and on possible conflicts with anticipated future
// features.
//
// A FIXME is often added in the following situations:
//
// - While working on, or studying a particular part of the code you uncover an issue or a
//   shortcoming. Additionally, you may have gained an understanding of how to fix it.
//
// - While implementing a new feature, you are forced to cut a corner, but you have some
//   good ideas about how to continue later, and/or you may have knowledge about a certain
//   planned feature that would require a more complete solution.
//
// A FIXME is generally not about a bug or an error, and it should generally not be
// considered a task either. Is is simply a memo to oneself or to some other developer who
// is going to work on the code at some later point in time.
//
// A FIXME should never be deleted unless by somebody who understands the meaning of it and
// knows that the problem is fixed, or has otherwise disappeared.
