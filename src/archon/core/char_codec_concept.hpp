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
sequences. For such codecs, the current shift state is maintain by the state object
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

Never consumes partial byte sequences.

Upon return, the part of the buffer after the final value of `buffer_offset` may have been modified by this function.

BELOW IS NOT NECESSARILY RIGHT: SEE BEGINNING OF FILE                

If this function returns true, the decoding operation completed successfully. If it returns false, it did not.     

When false is returned, \p error is set to true if the reason for the incompleteness is invalid input, and to false otherwise.    But what does data_offset point to?               

When false is returned, and \p error is set to false, and buffer_offset is equal to the size of the specified buffer upon return, additional buffer space is required in order for decoding to proceed.

When false is returned, and \p error is set to false, and buffer_offset is less than the size of the specified buffer upon return, additional input (\p data) is required in order for decoding to proceed.

In any case, data_offset and buffer_offset are updated to reflect how far decoding has progressed.    But how far can input consumption be ahead of output production?              


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
