/**

\page Concept_Archon_Core_BufferedTextFileImpl Concept: Core_BufferedTextFileImpl

This concept specifies the requirements that a class must meet in order to qualify as a
buffered text file implementation from the point of view of the Archon Core
library. Buffered text file implementation types can be used to customize various text file
related class templates, such as \ref archon::core::GenericBufferedTextFile.


Consider the following identifications:

  - Let `Impl` be a type that satisfies `Concept_Archon_Core_BufferedTextFileImpl`.

  - Let `C` be `Impl::char_type`.

  - Let `impl` be a value of type `Impl`.

  - Let `delim` be a value of type `C`.

  - Let `buffer` be a value of type `core::Span<C>`.

  - Let `dynamic_eof` be a value of type `bool`.

  - Let `offset` be an l-value of type `std::size_t`.

  - Let `found_delim` be an l-value of type `bool`.

  - Let `ec` be an l-value of type `std::error_code`.


Then:

  - `Impl` must meet the requirements of \ref Concept_Archon_Core_TextFileImpl.

  - `impl.read_until(delim, buffer, dynamic_eof, offset, found_delim, ec)` must be a valid
    expression. The returned value must be implicitly convertible to `bool`. The function is
    allowed to carry the `nodiscard` attribute.


### Read until

`impl.read_until(delim, buffer, dynamic_eof, offset, found_delim, ec)` must extract
characters from the file starting at the current position of the logical file pointer and
stopping after the first occurrence of the specified delimiter (\p delim) or at the end of
the file, whichever comes first. Upon return, the logical file pointer must have been
advanced to the position that follows the last extracted character.

Extracted characters must be placed in the specified buffer (\p buffer) starting at the
specified offset (\p offset). Upon return, \p offset must have been advanced to the position
in the buffer that follows the last extracted character.

On success, `read_until()` must return `true` after setting `found_delim` to `true` if the
delimiter was found before the end of file was reached, and to `false` otherwise.

On failure, `read_until()` must return `false` after setting `ec` to an error code that
reflects the cause of the failure.

On success, `read_until()` must leave \p ec "untouched", and on failure, it must leave \p
found_delim "untouched".

On failure, \p offset may, or may not have been advanced.

Upon return, positions in the buffer, beyond the position pointed to by the posterior value
of \p offset, may, or may not have been written to.

*/
