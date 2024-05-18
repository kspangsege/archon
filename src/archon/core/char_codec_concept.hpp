/**

\page Concept_Archon_Core_CharCodec Concept: Core_CharCodec

This concept specifies the requirements that a class must meet in order to qualify as a
character codec from the point of view of the Archon Core library. Objects of such a type
can be used to decode and encode streams of characters. Character codec types can be used to
customize various character codec related class templates, such as \ref
archon::core::TextCodecImpl and \ref archon::core::TextFileImpl.


Consider the following identifications:

  - Let `Codec` be a type that satisfies `Concept_Archon_Core_CharCodec`.

  - Let `C` be the type used to store unencoded characters, usually `char` or `wchar_t`.

  - Let `T` be the character traits type to be used with this character codec, usually
    `std::char_traits<C>`.

  - Let `Config` be a struct (or class) carrying configuration parameters.

  - Let `codec` be a value of type `Codec`.

  - Let `const_codec` be a value of type `const Codec`.

  - Let `locale` be a l-value of type `const std::locale`.

  - Let `config` be a value of type `Config`.

  - Let `state` be an l-value of type `T::state_type`.

  - Let `byte_data` be a value of type `core::Span<const char>`.

  - Let `char_data` be a value of type `core::Span<const C>`.

  - Let `byte_buffer` be a value of type `core::Span<char>`.

  - Let `char_buffer` be a value of type `core::Span<C>`.

  - Let `data_offset` be an l-value of type `std::size_t`.

  - Let `buffer_offset` be an l-value of type `std::size_t`.

  - Let `buffer_size` be a value of type `std::size_t`.

  - Let `error` be an l-value of type `bool`.


Then:

  - `Codec::char_type` must be `C`.

  - `Codec::traits_type` must be `T`.

  - `Codec::Config` must be `Config`.

  - `Codec::is_degen` must be a static compile-time constant expression that can be
    evaluated in a boolean context.

  - `Codec(locale)`, `Codec(locale, config)`, and `Codec(&locale, config)` must all be a
    valid expressions.

  - `codec.imbue(locale)` must be a valid expression.

  - `const_codec.is_stateless()` must be a valid expression that can be evaluated in a
    boolean context, and it must never throw.

  - `const_codec.decode(state, byte_data, data_offset, end_of_data, char_buffer,
    buffer_offset, error)` must be a valid expression that can be evaluated in a boolean
    context.

  - `const_codec.encode(state, char_data, data_offset, byte_buffer, buffer_offset, error)`
    must be a valid expression that can be evaluated in a boolean context.

  - `const_codec.unshift(state, byte_buffer, buffer_offset)` must be a valid
    expression that can be evaluated in a boolean context.

  - `const_codec.simul_decode(state, byte_data, data_offset, buffer_size)` must be a valid
    expression.

  - `Codec::max_simul_decode_size()` must be a valid compile-time constant expression that
    can be evaluated in a boolean context, and it must never throw.


`is_degen` should be `true` when the character codec is degenerate, and must be `false` when
it is non-degenerate. A degenerate codec is one where the character type (`C`) is `char`,
where the encoded form is identical to the unencoded form, and where the decode and encode
operations pass data through unmodified. A degenerate codec cannot be stateful, and the
decode and encode operations cannot fail.

`imbue()` must adopt the specified locale but retain the remaining configuration as passed
to the constructor.

`is_stateless()` should return `true` when the character codec is stateless, and must return
`false` when it is stateful. A stateful character codec is one where the result of decoding
a byte sequence depends on a shift state which generally depends on previously decoded byte
sequences. For such codecs, the current shift state is maintained by the state object
(`T::state_type`).

Exception guarantee for `decode()`, `encode()`, and `unshift()`: If they throw, none of
their reference arguments will have been changed, but buffer contents beyond the specified
buffer offset may have been clobbered.

The representation of shift state in `state` is locale specific, so behavior is undefined if
a state, that was produced by a codec using one locale, is later passed to a codec that uses
another locale.


INCOMPLETE!!!


Decode
------

Valid input to the decoding operation (`decode()`) consists of a series of valid input
sequences. For a stateless codec (`is_stateless()`), each valid input sequence codes for
precisely one code point (logical character). For a stateful codec, each valid input
sequence codes for zero or one code points, and may also modify the shift state (the \p
state argument). What constitutes a valid input sequence will generally depend on the
current shift state. No valid input sequence is empty, and no valid input sequence is a
prefix of another valid input sequence unless they are the same sequence.

The decoding process advances one full valid input sequence at a time, and when the consumed
input sequence codes for a character, the production of that character happens synchronously
with the consumption of the input sequence. The decoding process can only advance when the
input contains another full valid input sequence. Also, when the next valid input sequence
codes for a character, the decoding process can only advance if there is space for another
character in the output buffer, i.e., when \p buffer_offset is less than
`char_buffer.size()`.

When there is no more space in the output buffer (\p buffer_offset is equal to
`char_buffer.size()`) and the next input sequence does not code for a character (only
changes the shift state), it is unspecified whether the decoding process will proceed or
stop before consuming that input sequence. Both behaviors are allowed.

When the decoding operation stops, \p data_offset will point one beyond the last byte of the
last consumed full valid input sequence, or be unchanged if no input sequences were
consumed. Likewise, \p buffer_offset will point one beyond the last produced character, or
be unchanged if no characters were produced.

Setting \p end_of_input to `true` means that the specified input is not just a prefix of the
remaining input, but covers everything up until and including the last bytes of the
input. In this case, `decode()` returns `true` if, and only if all input was consumed. All
input was consumed if, and only if \p data_offset is equal to `byte_data.size()` upon
return. The point here is that when the end of input is present, any final incomplete input
sequence is an error.

When \p end_of_input is set to `false`, `decode()` returns `true` if, and only if one of the
following are true:

* All input was consumed, so \p data_offset is equal to `byte_data.size()` upon return.

* The remaining input does not contain, as a prefix, another valid input sequence, but is
  instead a prefix of a valid input sequence.

When `decode()` returns `false`, `error` is set to `true` if decoding stopped due to the
presence of and invalid input sequence, and to `false` if decoding stopped due to lack of
space in the output buffer (\p char_buffer, \p buffer_offset). If both conditions become
true at the same time, it is unspecified whether `error` is set to `true` or `false`. Both
behaviors are allowed. When decoding stops due to the presence of an invalid input sequence,
\p data_offset will point to the first byte of that invalid input sequence. When \p
end_of_input is `true`, an incomplete input sequence at the end of input counts as an
invalid input sequence.

When `decode()` returns `true`, `error` is left unchanged.

When `decode()` returns `false` and sets `error` to `false`, it follows that \p
buffer_offset is equal to `char_buffer.size()` upon return.

Upon return from `decode()`, the part of the buffer (\p char_buffer) that succeeds the final
value of \p buffer_offset may have been clobbered.

Because invalid input sequences are never consumed, when an error is reported, it follows
that \p data_offset will be strictly less than `byte_data.size()` upon return.

Decoding is guaranteed to work one output character at a time, which means that with enough
steps it is possible to decode any input stream using an output buffer of size one.


Encode
------

Never produces partial byte sequences.

Upon return, the part of the buffer after the final value of `buffer_offset` may have been modified by this function.

Questions:
- What is the meaning of the returned boolean value?
- Is `error` set to false on completion, or left untouched?


Simulate decode
---------------

Behavior is undefined if the difference between `data.size()` and \p data_offset
(prior to invocation) is more than `max_simul_decode_size()`.

Behavior is unspecified if \p buffer_size is greater than the increase in
`buffer_offset` that would be caused by an invocation of `decode()` given the
same shift state and same span of data.



`T::state_type` must be such that statement `T::state_type state = {}` creates a properly initialized state that represents the initial shift state. `T::state_type` must be copyable, and copying must be efficient.                                       

*/
