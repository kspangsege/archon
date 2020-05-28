/**

\page Concept_Archon_Core_Integer Concept: Core_Integer

This concept specifies the requirements that a type must meet in order to qualify as an
integer type from the point of view of the Archon Core library.

An integer type is understood as having a certain number of value bits, and optionally a
sign bit. An integer type is signed if it has a sign bit. Otherwise it is unsigned. All
integer types have at least 1 value bit.

For a particular object of an integer type, the represented value is entirely determined by
the value bits and the sign bit.

For a signed type, a represented value is negative when, and only when the sign bit is
one. Unsigned types cannot represent negative values.

Value bits have a definite order, and they are identified by their position in this
order. The position of the first value bit is zero.

For unsigned types, the represented value is the sum of `bit(i) * 2^i` for all bit positions
`i`. Here, `bit(i)` is the value of the bit at position `i`. The same is true for signed
types, provided that the value of the sign bit is zero.

The range of representable values for an unsigned integer type is from zero to `2^n - 1`,
where `n` is the number of value bits of the type. For an signed type, the maximal
representable value is still `2^n - 1`, but the minimal representable value is either
`-(2^n)` or `-(2^n - 1)` depending on which representation is used for negative
values. Possible representations include two's complement, ones' complement, and
sign-magnitude.

An integer type, `V`, has an associated *parts representation*, which is an array of parts
where each part carries some of the bits of `V`. The type of the parts must be one of the
fundamental unsigned integer types, and it must not rank below `unsigned`. The number of
parts must be such that the total number of bits in the parts representation is greater
than, or equal to the total number of bits in `V` (number of value bits plus one if
signed). The first part contains the first bits of `V`, and so forth. The last part may, or
may not have unused bits. If it has both used and unused bits, the used bits come before the
unused bits. The number of parts must be low enough that the last part has at least one used
bit. If `V` is signed, the parts representation uses two's complement representation for
negative values. The two's complement representation uses all bits in the parts
representation, including any unused bits in the last part.



Assume the following identifications:

  - Let `V` be a type.

  - Let `T` be `core::IntegerTraits<V>`.

  - Let `width` be the number of value bits in `V` plus one if `V` is signed.

  - Let `v` and `w` be objects of type `V`.

  - Let `P` be the type of the promotion of `v` (see `+v` below).

  - Let `i` be an object of type `int`.


Then `V` conforms to the `Core_Integer` concept if, and only if all of the following
requirements are met:

  - `T::is_specialized` must be a static compile-time constant expression of type
    `bool`. Its value must be `true`.

  - `T::num_value_bits` must be a static compile-time constant expression of type `int`. Its
    value must be the number of value bits in `V`.

  - `T::is_signed` must be a static compile-time constant expression of boolean type whose
    value is `true` when, and only when `V` is a signed.

  - `T::unsigned_type` must be an unsigned integer type that conforms to `Core_Integer`. If
    `V` is unsigned, `T::unsigned_type` and `V` must be the same type. If `V` is signed,
    `T::unsigned_type` must be able to represent all non-negative values that are
    representable in `V`. The number of value bits in `T::unsigned_type` must be less than,
    or equal to the number of value bits in `core::IntegerTraits<P>::unsigned_type`. For
    fundamental integer types other than `bool`, `T::unsigned_type` and
    `std::make_unsigned_t<V>` must be the same type. For `bool`, `T::unsigned_type` must be
    `bool`.

  - `V()` must be a valid non-throwing expression that can be evaluated at compile time. It
    must construct an object that represents zero.

  - `V(i)` must be a valid non-throwing type conversion expression that can be evaluated at
    compile time if `i` is a compile-time constant. The operation must be value preserving
    if `i` represents a value that is also representable in `V`.

  - `int(v)` must be a valid non-throwing type conversion expression that can be evaluated
    at compile time if `v` is a compile-time constant. The operation must be value
    preserving if `v` represents a value that is also representable in `int`.

  - `V(v)` must be a valid non-throwing copy-construction that can be evaluated at compile
    time if `v` is a compile-time constant.

  - `v = w` must be a valid non-throwing assignment expression that can be evaluated at
    compile time if `w` is a compile-time constant.

  - `+v`, `-v`, `v + w`, `v - w`, `v * w`, `v / w`, `v % w` must be valid non-throwing
    expressions that can be evaluated at compile time if `v` and `w` are compile-time
    constants. The results must have type `P`. For each operation except `%`, the results
    must agree with the corresponding operation on a hypothetical integer type of infinite
    precision (Z) provided that the infinite-precision result is representable in `P`; and
    in the case of `/`, provided that `w` is not zero. For `%` specifically, the result must
    agree with the infinite-precision result provided that the infinite-precision result of
    `v / w` is representable in `P`, and provided that `w` is not zero. If `P` is unsigned;
    then the results of `-v`, `v + w`, and `v - w` must be congruent to `-z(v) mod 2^n`,
    `z(v) + z(w) mod 2^n`, and `z(v) - z(w) mod 2^n` respectively, even when the infinite
    precision results are not representable in `P`. Here, `n` is the number of value bits in
    `P`, and `z(v)` should be understood as a conversion to a signed integer type of
    infinite precision. For the fundamental integer types, and when `P` is unsigned, this
    guarantee is provided in paragraph 4 of section 6.9.1 "Fundamental types" of the C++17
    specification.

  - `~v`, `v & w`, `v | w`, and `v ^ w` must be valid non-throwing expressions that can be
    evaluated at compile time if `v` and `w` are compile-time constants. The results must
    have type P. If `v` and `w` are non-negative, the results of `v & w`, `v | w`, and `v ^
    w` must accord with expectation (bitwise "and", bitwise "or", and bitwise "exclusive
    or"). If `P` is unsigned, the result of `~v` must accord with expectation (bitwise
    "not").

  - `v << i` must be a valid non-throwing expression that can be evaluated at compile time
    if `v` and `i` are compile-time constants. The result must have type `P`. If `P` is
    unsigned, and `i` is non-negative and less than `width`; the result of `v << i` must be
    congruent to `2^i * v` modulo `2 ^ width`. If `P` is signed, `v` is non-negative, and
    `i` is non-negative and less than `width`; the result of `v << i` must be equal to `2^i
    * v` if that value is representable in `P`.

  - `v >> i` must be a valid non-throwing expression that can be evaluated at compile time
    if `v` and `i` are compile-time constants. The result must have type `P`. If `P` is
    unsigned, and `i` is non-negative and less than `width`; or if `P` is signed, `v` is
    non-negative, and `i` is non-negative and less than `width`; the result of `v >> i` must
    be equal to the integer part of `v / 2^i`.

  - `v == w`, `v != w`, `v < w`, `v > w`, `v <= w`, and `v >= w` must be valid non-throwing
    expressions that can be evaluated at compile time if `v` and `w` are compile-time
    constants. The results must have boolean type. The results must accord with the usual
    meaning of the involved operators.

  - `+v` must be a promotion operation. This operation must be value preserving and
    idempotent. The type of the promoted value, `P`, must conform to the `Core_Integer`
    concept. It must also be such that if `u` is of type
    `core::IntegerTraits<P>::unsigned_type`, then `+u` is unsigned.

  - `T::min()` must be a valid function invocation that can be evaluated at compile
    time. The return type must be `V`. It must return the lowest value that can be
    represented in `V`. This is zero if `V` is unsigned, and the most negative representable
    value if `V` is signed. The most negative representable value is either `-(2^n)` or
    `-(2^n) + 1` where `n` is the number of value bits in `V`, depending on which
    representation is used in `V` for negative values.

  - `T::max()` must be a valid function invocation that can be evaluated at compile
    time. The return type must be `V`. It must return the highest value that can be
    represented in `V`. This is always `2^n - 1` where `n` is the number of value bits in
    `V`.

  - `T::part_type` must be the type of the individual parts in the parts representation (see
    above). It must be one of the fundamental unsigned integer types, and it must not rank
    below `unsigned`.

  - `T::num_parts` must be a static compile-time constant expression of type `int`. Its
    value must be the number of parts in the parts representation (see above).

  - `T::get_parts(v)` must be a valid function invocation that can be evaluated at compile
    time if `v` is a compile-time constant. It must return the parts representation. The
    return type must be `std::array<U, N>` where `U` is `T::part_type` and `N` is
    `T::num_parts`.

  - `T::from_parts(parts)` must be a valid function invocation that can be evaluated at
    compile time if `parts` is a compile-time constant. It must return an object of type `V`
    constructed from the specified parts representation. The type of `parts` must be
    `std::array<U, N>` where `U` is `T::part_type` and `N` is `T::num_parts`. The operation
    must be value preserving if the value represented by `parts` is also representable in
    `V`.

  - `T::has_divmod` must be a static compile-time constant expression of boolean type. It
    must be `false` unless `T::DivMod` and `T::divmod()` are properly defined (see
    below). Otherwise it may be `true`.

  - If `T::has_divmod` is `true`, `T::DivMod` must be a type equivalent to one defined as
    `struct DivMod { V quot; V rem; };` Here, `quot` is the quotient resulting from an
    integer division (see `T::divmod(v, w)`), and `rem` is the corresponding remainder.

  - If `T::has_divmod` is `true`, `T::divmod(v, w)` must be a valid non-throwing expression
    that can be evaluated at compile time if `v` and `w` are compile-time constants. The
    result must have type `T::DivMod`. If `res` is the result, and the value of `v / w` is
    representable in `V`, `res.quot` must be equal to `v / w`. Likewise, if the value of `v
    % w` is representable in `V`, `res.rem` must be equal to `v % w`.

  - `T::has_find_msb_pos` must be a static compile-time constant expression of boolean
    type. It must be `false` unless `T::find_msb_pos()` is properly defined (see
    below). Otherwise it may be `true`.

  - If `T::has_find_msb_pos` is `true`, `T::find_msb_pos(v)` must be a valid function
    invocation that can be evaluated at compile time if `v` is a compile-time constant. The
    type of the result must be `int`. The result must be the position of the most
    significant bit set in `v`, or -1 if `v` is zero. If `V` is signed and `n` is the number
    of value bits in `V`, the first `n` bits are taken to be the value bits, and the
    position of the sign bit is taken to be one plus the position of the last value
    bit. Therefore, if `v` is negative, the result will always be `n`.

*/
