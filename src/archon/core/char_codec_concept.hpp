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

  - `T::state_type` must be such that statement `T::state_type state = {}` creates a
    properly initialized state that represents the initial shift state. `T::state_type` must
    be copyable, and copying should be efficient.

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

Valid input to the decoding operation (`decode()`) and output from the encoding process
(`encode()`) consists of a series of valid byte sequences. For a stateless codec
(`is_stateless()`), each valid byte sequence codes for precisely one code point (logical
character). For a stateful codec, each valid byte sequence codes for zero or one code
points, and may also correspond to a change in shift state (the \p state argument). What
constitutes a valid byte sequence will generally depend on the current shift state. No valid
byte sequence is empty, and no valid byte sequence is a proper prefix of another valid byte
sequence.


Decode
------

\code{.cpp}

  decode(state, byte_data, data_offset, end_of_data, char_buffer, buffer_offset, error)

\endcode

A decoding process uses one or more incremental steps to decode a stream of bytes
corresponding to the encoding of a sequence of characters. In each step, a section of the
byte stream must be passed as input to `decode()`. `decode()` will then consume a prefix of
those bytes while placing the corresponding decoded characters in the supplied output
space. If the specified section of the byte stream was not fully consumed, or if the byte
stream continues after the consumed section, `decode()` should be called again. In general,
the application must arrange for the input to a particular step of the decoding process to
consist of unconsumed bytes from the previous step followed by zero or more additional bytes
from the stream.

With a stateful codec, the initial shift state for one step of the decoding process (\p
state) must be the final state from the previous step, or, if there is no previous step, it
must be the initial shift state as constructed through default initialization
(`T::state_type()`).

Input to a particular invocation of `decode()` is the part of \p byte_data that succeeds the
initial value of \p data_offset. Behavior is undefined if the initial value of \p
data_offset is greater than the size of \p byte_data.

The output space that is available during a particular invocation of `decode()` is the part
of \p char_buffer that succeeds the initial value of \p buffer_offset. Behavior is undefined
if the initial value of \p buffer_offset is greater than the size of \p char_buffer.

The decoding process advances one or more full valid input byte sequences at a time, and
when a consumed byte sequence codes for a character, the production of that character
happens synchronously with the consumption of the byte sequence. The decoding process can
only advance when the input contains a full valid byte sequence. Also, when the next valid
byte sequence codes for a character, the decoding process can only advance if there is
enough output space for another character, i.e., when \p buffer_offset is less than
`char_buffer.size()`.

The decoding process is guaranteed to advance if the input contains a valid byte sequence
and there is output space for at least one character. This is true for both stateless and
stateful codecs (`is_stateless()`), and even when the next byte sequence does not code for a
character.

When there is no more space in the output buffer (\p buffer_offset is equal to
`char_buffer.size()`) and the next input byte sequence does not code for a character (only
changes the shift state), it is unspecified whether the decoding process will proceed or
stop before consuming that byte sequence. Both behaviors are allowed.

When the decoding operation stops, \p data_offset will have been updated to point one beyond
the last byte of the last consumed full valid byte sequence, or it will be unchanged if no
byte sequences were consumed. Likewise, \p buffer_offset will have been updated to point one
beyond the last produced character, or it will be unchanged if no characters were produced.

Setting \p end_of_input to `true` means that the specified input is not just a prefix of the
remaining input, but covers everything up to and including the last bytes of the input. In
this case, `decode()` returns `true` if, and only if all input was consumed. All input was
consumed if, and only if \p data_offset is equal to `byte_data.size()` upon return. The
point here is that when the end of input is present, any final incomplete input sequence is
taken to be an error.

When \p end_of_input is set to `false`, `decode()` returns `true` if, and only if one of the
following are true:

  - All input was consumed, so \p data_offset is equal to `byte_data.size()` upon return.

  - The remaining input does not contain, as a prefix, another valid byte sequence, but is
    instead a prefix of a valid byte sequence.

When `decode()` returns `false`, `error` is set to `true` if decoding stopped due to the
presence of and invalid byte sequence, and to `false` if decoding stopped due to lack of
output space (\p char_buffer, \p buffer_offset). If both conditions become true at the same
time, it is unspecified whether `error` is set to `true` or `false`. Both behaviors are
allowed. When decoding stops due to the presence of an invalid byte sequence, \p data_offset
will point to the first byte of that invalid byte sequence. When \p end_of_input is `true`,
an incomplete byte sequence at the end of input counts as an invalid byte sequence.

When `decode()` returns `true`, `error` is left unchanged.

When `decode()` returns `false` and sets `error` to `false`, it follows that \p
buffer_offset is equal to `char_buffer.size()` upon return.

Upon return from `decode()`, the output buffer memory (\p char_buffer) may have been
reallocated, and the part of the buffer contents that succeeds the final value of \p
buffer_offset may have been clobbered.

Because invalid byte sequences are never consumed, when an error is reported, it follows
that \p data_offset will be strictly less than `byte_data.size()` upon return.


Encode
------

\code{.cpp}

  encode(state, char_data, data_offset, byte_buffer, buffer_offset, error)

\endcode

An encoding process uses one or more incremental steps to encode a stream of characters. In
each step, a section of the character stream must be passed as input to
`encode()`. `encode()` will then consume a prefix of those characters while producing the
corresponding encoding in the supplied output space. If the specified section of the
character stream was not fully consumed, or if the character stream continues after the
consumed section, `encode()` should be called again. In general, the application must
arrange for the input to a particular step of the encoding process to consist of unconsumed
characters from the previous step followed by zero or more additional characters from the
stream.

With a stateful codec, the initial shift state for one step of the encoding process (\p
state) must be the final state from the previous step, or, if there is no previous step, it
must be the initial shift state as constructed through default initialization
(`T::state_type()`).

Input to a particular invocation of `encode()` is the part of \p char_data that succeeds the
initial value of \p data_offset. Behavior is undefined if the initial value of \p
data_offset is greater than the size of \p char_data.

The output space that is available during a particular invocation of `encode()` is the part
of \p byte_buffer that succeeds the initial value of \p buffer_offset. Behavior is undefined
if the initial value of \p buffer_offset is greater than the size of \p byte_buffer.

Each encoding step produces zero or more full valid byte sequences. When a produced byte
sequence codes for a character, that character is the next input character, and the
consumption of that character happens synchronously with the production of the byte
sequence. Likewise, when a produced byte sequence codes for a change in shift state, the
change in shift state happens synchronously with the production of the byte sequence. Note
that a particular byte sequence may code for both a character and a change in shift
state. The encoding operation can only advance if the input is non-empty and there is enough
output space for the next byte sequence to be produced.

The encoding process is guaranteed to advance, i.e., consume input, if the input is
non-empty and sufficient output space is provided. There is no upper limit on how much
output space might be needed in the worst case, but in general it will be twice the maximum
length of a byte sequence, one to change the shift state and one for the encoding of the
next character. The application must be prepared to expand its output buffer as requested by
`encode()`.

The encoding operation is guaranteed to not produce any output if the specified input is
empty. This is true even in cases where a state-changing output sequence could otherwise
have been generated (but see `unshift()`). From this, it follows that the encoding operation
is guaranteed to not change the shift state if the specified input is empty.

When the encoding operation stops, \p data_offset will have been updated to point one beyond
the last consumed character, or it will be unchanged if no characters were
consumed. Likewise, \p buffer_offset will have been updated to point one beyond the last
byte of the last produced byte sequence, or it will be unchanged if no byte sequences were
produced.

When `encode()` returns `false`, `error` is set to `true` if encoding stopped due to the
presence of and invalid character, and to `false` if encoding stopped due to lack of output
space (\p byte_buffer, \p buffer_offset). If both conditions become true at the same time,
it is unspecified whether `error` is set to `true` or `false`. Both behaviors are
allowed. When encoding stops due to the presence of an invalid character, \p data_offset
will point to that character upon return.

When `encode()` returns `true`, `error` is left unchanged.

When `encode()` returns `true`, it follows that \p data_offset is equal to
`char_data.size()` upon return.

Upon return from `encode()`, the output buffer memory (\p byte_buffer) may have been
reallocated, and the part of the buffer contents that succeeds the final value of \p
buffer_offset may have been clobbered.

When an error is reported, it follows that \p data_offset will be strictly less than
`char_data.size()` upon return.


Unshift
-------

INCOMPLETE!!!


Simulate decode
---------------

INCOMPLETE!!!

Behavior is undefined if the difference between `data.size()` and \p data_offset
(prior to invocation) is more than `max_simul_decode_size()`.

Behavior is unspecified if \p buffer_size is greater than the increase in
`buffer_offset` that would be caused by an invocation of `decode()` given the
same shift state and same span of data.

*/
