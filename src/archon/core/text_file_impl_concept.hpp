/**

\page Concept_Archon_Core_TextFileImpl Concept: Core_TextFileImpl

This concept specifies the requirements that a class must meet in order to qualify as a text
file implementation from the point of view of the Archon Core library. Text file
implementation types can be used to customize various text file related class templates,
such as \ref archon::core::GenericTextFile and \ref archon::core::GenericTextFileStream. See
also \ref Concept_Archon_Core_BufferedTextFileImpl.


Consider the following identifications:

  - Let `Impl` be a type that satisfies `Concept_Archon_Core_TextFileImpl`.

  - Let `C` be the type used to store unencoded characters, usually `char` or `wchar_t`.

  - Let `T` be the character traits type to be used with this text file implementation,
    usually `std::char_traits<C>`.

  - Let `D` be the character codec type to be used with this text file implementation. This
    must be a type that satisfies \ref Concept_Archon_Core_CharCodec.

  - Let `Config` be a struct (or class) carrying configuration parameters.

  - Let `impl` be a value of type `Impl`.

  - Let `file` by an l-value of type `core::File`.

  - Let `locale` be an l-value of type `const std::locale`.

  - Let `config` be a value of type `Config`.

  - Let `state` be a value of type `T::state_type`.

  - Let `buffer` be a value of type `core::Span<C>`.

  - Let `data` be a value of type `core::Span<const C>`.

  - Let `dynamic_eof` be a value of type `bool`.

  - Let `m` be a value of type `std::size_t`.

  - Let `n` be an l-value of type `std::size_t`.

  - Let `p` be a value of type `T::pos_type`.

  - Let `q` be an l-value of type `T::pos_type`.

  - Let `ec` be an l-value of type `std::error_code`.


Then:

  - `Impl::char_type` must be `C`.

  - `Impl::traits_type` must be `T`.

  - `Impl::codec_type` must be `D`.

  - `Impl::pos_type` must be `T::pos_type`.

  - `Impl::state_type` must be `T::state_type`.

  - `Impl::Config` must be `Config`.

  - `Impl::has_degen_unshift` must be a static compile-time constant expression that can be
    evaluated in a boolean context.

  - `Impl(file, &locale, config)` must be a valid expression.

  - `impl.reset(state)` must be a valid expression. The return type of this function is
    allowed to be `void`.

  - `impl.read_ahead(buffer, dynamic_eof, n, ec)` must be a valid expression.  The returned
    value must be implicitly convertible to `bool`. The function is allowed to carry the
    `nodiscard` attribute.

  - `impl.write(data, n, ec)` must be a valid expression. The returned value must be
    implicitly convertible to `bool`. The function is allowed to carry the `nodiscard`
    attribute.

  - `impl.unshift(ec)` must be a valid expression. The returned value must be implicitly
    convertible to `bool`. The function is allowed to carry the `nodiscard` attribute.

  - `impl.advance()` and `Impl::advance(m)` must be a valid expressions. The return type of
    these functions is allowed to be `void`.

  - `impl.discard(ec)` must be a valid expression. The returned value must be implicitly
    convertible to `bool`. The function is allowed to carry the `nodiscard` attribute.

  - `impl.flush(ec)` must be a valid expression. The returned value must be implicitly
    convertible to `bool`. The function is allowed to carry the `nodiscard` attribute.

  - `impl.tell_read(q, ec)` and `Impl::tell_write(q, ec)` must be valid expressions. The
    returned values must be implicitly convertible to `bool`. The functions are allowed to
    carry the `nodiscard` attribute.

  - `impl.seek(p, ec)` must be a valid expression. The returned value must be implicitly
    convertible to `bool`. The function is allowed to carry the `nodiscard` attribute.

  - `impl.imbue(locale, state)` must be a valid expression. The return type of this function
    is allowed to be `void`.


INCOMPLETE!!!


FIXME: Specify behavior relating to exception safety.                                

FIXME: Specify behavior on failure (when function returns false).                                



### Modes

A file implementation is in one of three modes, neutral, reading, or writing. Initially, it
is in neutral mode.

In general, a read operation is allowed only if the implementation is in neutral, or in
reading mode. If it is in neutral mode, the read operation pushes it into reading mode.

In general, a write operation is allowed only if the implementation is in neutral, or in
writing mode. If it is in neutral mode, the write operation pushes it into writing mode.

The intention is to not require that a text file implementation keep explicit track of the
current mode. The application (such as \ref archon::core::GenericTextFile), on the other
hand, will likely have to keep track of the mode in order to not violate the rules governing
the use of the text file implementation.


### File pointers

There are three different file pointers to keep track of; the logical file pointer, the
read-ahead pointer, and the actual file pointer.

The position of the logical file pointer is the start position for the next write operation
(`write()`) and the initial position for the read-ahead pointer after switching to read
mode.

The position of the read-ahead pointer is the start position for the next read-ahead
operation (`read_ahead()`).

The actual file pointer is the file pointer as seen from the point of view of the lowest
level medium. If the lowest level medium is an actual file (\ref core::File), which it
generally is, the actual file pointer is the file pointer provided by, and maintained by the
operating system (sometimes called a file offset).

While in neutral mode, the positions of the logical file pointer, and the read-ahead pointer
both coincide with the position of the actual file pointer.

While in reading mode, the position of the read-ahead pointer is always greater than, or
equal to the position of the logical file pointer, and the position of the actual file
pointer is always greater than, or equal to the position of the read-ahead pointer.

While in writing mode, the position of the read-ahead pointer is undefined, and the position
of the logical file pointer is always greater than, or equal to the position of the actual
file pointer.


### Data members

`Impl::has_degen_unshift` must be a static compile-time constant, and should evaluate to
`true` in a boolean context when the `unshift()` member function has no effect, and
therefore never needs to be called. It must evaluate to `false` if the `unshift()` can have
an effect, and therefore may need to be called in some cases.


### Reset

`Impl::reset()` must reset the file implementation object such that it is in a state that is
appropriate for a newly opened file. It must at least put the file implementation object
into neutral mode. It must perform its duties in a way that does not involve accessing the
associated file object (\ref core::File) in any way. The application must call `reset()`
after construction of the file implementation object, and in connection with each reopening
of the associated file object. The application is allowed to call `reset()` before it calls
`open()` on the associated file object.


### Read and advance

`Impl::read_ahead()` must extract characters starting from the current position of the
read-ahead pointer. Provided that the size of the specified buffer is not zero,
`Impl::read_ahead()` must extract at least one character, unless it is prevented due to end
of file, or by a decoding error. If at least one character can be extracted without blocking
the calling thread, the function must not block the calling thread, but should still extract
as many characters as it can without blocking. If the function returns `true`, and sets \p n
to zero, it indicates to the caller that the end of file has been reached, provided that the
size of the specified buffer is not zero. Upon return, the read-ahead pointer will have been
advanced to the position that follows the last extracted character. While the position of
the logical file pointer remains unchanged, the actual file pointer may, or may not have
been advanced. This function must be called only while in neutral mode, or while in reading
mode. Upon return, the file implementation object will be in reading mode if any characters
were extracted, or if the the file implementation object was already in reading
mode. Otherwise, the file implementation object may, or may not have been pushed from
neutral mode into reading mode, even if the read operation fails. Behavior is undefined if
this function is called while the file implementation is in writing mode.

`Impl::read_ahead()` reports a decoding error by returning `false` and setting `ec` to \ref
archon::core::TextFileError::invalid_byte_seq. In this case, all preceding characters will
have been extracted, and the read-ahead pointer will have been advanced to the position that
follows the last extracted character.

FIXME: Integrate this information for  `Impl::read_ahead()`:                     
> On success, if \p n is zero and the size of the specified buffer is greater
> than zero, it means that the end of input has been reached.
>
> On success, \p ec is left untouched. On failure \p n is left untouched, and no
> characters will have been read from the stream.

FIXME: State that behavior of `Impl::read_ahead()` is similar for other read errors.               

`Impl::advance()` may be called while the file implementation is in reading mode, or in
neutral mode. It must move the logical file pointer forward. When no argument is provided,
the logical file pointer must be moved forward to the position of the read-ahead
pointer. When an argument is provided, it must move the logical file pointer forward by the
specified number of characters. Here, one character corresponds to one slot in the buffer
passed to `Impl::read_ahead()`. The specified number of character must be less than, or
equal to the actual number of characters between the logical file pointer and the read-ahead
pointer. Behavior is undefined if this rule is broken. Upon return, the mode of the file
implementation (reading or neutral) is unchanged. Behavior is undefined if either of these
functions are called while the file implementation is in writing mode.

FIXME: Integrate this information for  `Impl::advance()`:                               
> `noexcept` is not guaranteed on any of these (all text file impl functions
> must be considered throwing from a generic point of view)


### Write and unshift

`Impl::write()` must inject the specified characters into the file, starting from the
current position of the logical file pointer. Upon return, the logical file pointer will
have been advanced to coincide with the position that follows the last injected
character. The actual file pointer may, or may not have been advanced. This function must be
called only while in neutral mode, or while in writing mode. Upon return, the file
implementation object will be in writing mode if any characters were injected, or if the
file implementation object was already in writing mode. Otherwise, the file implementation
object may, or may not have been pushed from neutral mode into writing mode, even if the
write operation fails. Behavior is undefined if this function is called while the file
implementation is in reading mode.

FIXME: Integrate this information for `Impl::write()`:                     
> On success, \p n will always be equal to `data.size()`. On failure, \p n will
> always be less than `data.size()` (provided that `data.size()` is greater than
> zero), and it will indicate how many characters were written before the
> failure occurred.
>
> On success, \p ec is left untouched.

`Impl::unshift()` may be called while the file implementation is in writing mode, or in
neutral mode. It has no effect if `Impl::has_degen_unshift` is `true`. Otherwise, if the
shift state at the current position of the logical file pointer is not the initial shift
state, this function must produce a byte sequence (and write it to the underlying medium)
that brings the shift state back to the initial shift state. Upon return, the position of
the logical file pointer, as measured in number characters (units of type `C`), is
unchanged. When measured in number of units of the underlying medium (bytes), however, it
may have changed. Upon return, the file implementation object will be in writing mode if any
bytes were generated, or if the file implementation object was already in writing
mode. Otherwise, the file implementation object may, or may not have been pushed from
neutral mode into writing mode, even if the unshift operation fails. Behavior is undefined
if this function is called while the file implementation is in reading mode.


### Discard and flush

`Impl::discard()` causes buffered data to be discarded. It may be called only while the file
implementation is in reading, or in neutral mode. If the file implementation is in neutral
mode, this function has no effect. Behavior is undefined if this function is called while
the file implementation is in writing mode. Beyond discarding buffered data, when in reading
mode, this function switches to neutral mode after moving the read-ahead pointer and the
actual file pointer backwards to the position of the logical file pointer. After a failed
invocation of discard(), the mode is unchanged.

FIXME: Integrate this information for `Impl::discard()`:                               
> Revert read-ahead position, and actual read/write position to coincide with
> the current logical read/write position.
>
> If this function throws, nothing, that is observable by the caller, will have
> changed.

`Impl::flush()` causes un-written buffered data to be written to the underlying medium. It
may be called only while the file implementation is in writing, or in neutral mode. If the
file implementation is in neutral mode, this function has no effect. Behavior is undefined
if this function is called while the file implementation is in reading mode. The flushing
operation causes the actual file pointer to be advanced to the position of the logical file
pointer. Upon return, on success, the file implementation will have been switched to neutral
mode. After a failed invocation of flush(), the mode is unchanged. If encoding of a
character fails, `Impl::flush()` must still flush everything up to the point of the failure.


### Tell and seek

`Impl::tell_read()` determines the position of the logical file pointer when the file
implementation is in reading, or in neutral mode. Behavior is undefined if this function is
called while the file implementation is in writing mode. The position is returned as an
offset from the beginning of the lowest level medium (usually an actual file), and measured
in number of units of the lowest level medium (usually bytes). The positions of the logical,
and the actual file pointer remains unchanged.

`Impl::tell_write()` determines the position of the logical file pointer when the file
implementation is in writing, or in neutral mode. Behavior is undefined if this function is
called while the file implementation is in reading mode. The position is returned as an
offset from the beginning of the lowest level medium (usually an actual file), and measured
in number of units of the lowest level medium (usually bytes). This function may, or may not
change the position of the actual file pointer. The position of the logical file pointer,
however, remains unchanged.

`Impl::seek()` moves the logical file pointer to the specified position. It may be called
only while the file implementation is in reading, or in neutral mode. Behavior is undefined
if this function is called while the file implementation is in writing mode. After a
successful invocation of seek(), the file is in neutral mode. After a failed invocation of
seek(), the mode is unchanged.

FIXME: Integrate this information for `Impl::seek()`:                               
> The "state" part of pos arg is ignored by dome implementations.


### Imbue

`Impl::imbue()` may be called only while the file implementation is in neutral mode. It must
then take any necessary action in in order to adopt the specified locale and shift
state. Upon return, the file implementation will still be in neutral mode.  Behavior is
undefined if this function is called while the file implementation is in reading mode, or in
writing mode.

*/
