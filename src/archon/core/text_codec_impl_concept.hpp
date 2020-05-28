/**

\page Concept_Archon_Core_TextCodecImpl Concept: Core_TextCodecImpl

This concept specifies the requirements that a class must meet in order to qualify as a text
codec implementation from the point of view of the Archon Core library. Text codec
implementation types can be used to customize various text codec related class templates,
such as \ref archon::core::GenericTextCodec.


Consider the following identifications:

  - Let `Impl` be a type that satisfies `Concept_Archon_Core_TextCodecImpl`.

  - Let `C` be the type used to store unencoded characters, usually `char` or `wchar_t`.

  - Let `T` be the character traits type to be used with this text codec implementation,
    usually `std::char_traits<C>`.

  - Let `Config` be a struct (or class) carrying configuration parameters.

  - Let `Decoder` be the type of the decoder used by this text codec implementation.

  - Let `Encoder` be the type of the encoder used by this text codec implementation.

  - Let `locale` be a l-value of type `const std::locale`.

  - Let `config` be a value of type `Config`.


Then:

  - `Impl::char_type` must be `C`.

  - `Impl::traits_type` must be `T`.

  - `Impl::Config` must be `Config`.

  - `Impl::decoder_type` must be `Decoder`.

  - `Impl::encoder_type` must be `Encoder`.

  - `Impl::is_degen` must be a static compile-time constant expression that can be evaluated
    in a boolean context.

  - `Impl(&locale, config)` must all be a valid expressions.    


`is_degen` should be `true` when the implementation type produces a degenerate text codec,
and must be `false` when it produces a non-degenerate text codec. A degenerate text codec is
one where the character type (`C`) is `char`, where the encoded form is identical to the
unencoded form, and where the decode and encode operations pass data through
unmodified. With a degenerate text codec, the decode and encode operations cannot fail.


INCOMPLETE!!!

*/
