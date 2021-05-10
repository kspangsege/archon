

Hmm, why does `CC=gcc-10 CXX=g++-10 sh ./do.sh check -f "Base_TextCodec_EncodePosix" -l off -n 10000000 -t 1` run in half the time of `CC=gcc-10 CXX=g++-10 sh ./do.sh check -f "Base_TextCodec_EncodePosix" -l off -n 10000000 -t 24`? Can something be done about it (maybe using atomic test index)?




TODO:
- Issue with multiple test file/dir path guards using same name in nested scopes / function invocations: Consider adding line number to name. Maybe impossible to robustly prevent. Consider just adding a warning in the documentation.
- Fix newline transformations on Windows
- Finish Base64




Consider: Add `base::SeedMemoryBuffer::SeedMemoryBuffer(std::unique_ptr<T[]> memory, std::size_t size) noexcept` and `std::unique_ptr<T[]> base::SeedMemoryBuffer::release() noexcept` in order to make `base::SeedMemoryBuffer` further render `base::Buffer` obsolete. `release()` will return null until the buffer no longer uses the specified (nonowned) seed memory.
Consider: Rename `base::Buffer` -> `base::SimpleBuffer` and `base::SeedMemoryBuffer` -> `base::Buffer`.




```
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # require at least gcc 4.8
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
        message(FATAL_ERROR "GCC version must be at least 4.8!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # require at least clang 3.2
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.2)
        message(FATAL_ERROR "Clang version must be at least 3.2!")
    endif()
else()
    message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()
```


Careful with use of memory order relaxed in base::BasicLimitLogger: https://www.youtube.com/watch?v=cWkUqK71DZ0


Need version of base::int_cast() that throws on overflow.
Need version of base::twos_compl_cast() that throws on overflow.


https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/


Windows needs newline translation:
- Looks like newline output translation can be efficiently combined with character encoding, at least in BasicFileLogger
- Introduce
  - template<class C, class T = std::char_traits<C>, class A> std::basic_string<C, T, A> File::load_text(path, locale) will internally construct IncrementalDecoder
  - void File::save_text(path, base::Span<const C> text, locale = {}) will internally construct IncrementalEncoder.


Gold linker: https://github.com/frobware/c-hacks/blob/master/cmake/use-gold-linker.cmake


Test harness: See if the function of `ARCHON_CHECK_EQUAL_SEQ()` could instead be merged into `ARCHON_CHECK_EQUAL()` by detecting the applicability of `std::begin()` and `std::end()`. NO, NOT A GOOD IDEA.

Test harness: Breadcrumb trail: Consider adding `ARCHON_TEST_TRAIL(parent_test_context, trail_segment)` defines a new `text_context` variable. Consider using it in unit test `Base_Integer_Mask`.

Base64 unit test <-- cases from RFC


Seems like there are lots of checks in test_integer.cpp with bad assumptions about sizes of types.


Unit test: Compare lists of comparable element types, and formatting of lists in truncated form. Seems I need size limiting version of base::as_list() similarly to base::quote().


test_utf8.cpp ???
test_util_uri.cpp ???


Continue porting `unit_test.hpp` / `unit_test.cpp`
- Use base::FileStream for output from XML reporter.
- Add commandline option for XML reporter output.
- Move `ensure_subdir()` from `unit_test/run.cpp` to `base/filesystem.hpp`.



Unit testing:
- Maybe reuse unit tests from circular buffer for array seeded vector.


Extra unit testing features:
- Easy exec from main() front end with env vars
- Install signal handlers for unit testing to be able to get current set of running tests.
- Report relative progress during unit testing.
- Report top-5 time users using specialized reporter.


Unit testing future improvements:
- Show sequence difference when using ARCHON_CHECK_EQUAL_SEQ().
- Configurable timestamp format
- Configurable maximum size of quoted forms of strings
- Allow all threads to log to same log file. This should be enabled when none of  `@i`, `@n`, `@I` or `@N` are used in the template. In this case, threads must still have log message prefixes.


Consider package structure:
 - base
 - format
 - log
 - string_template : log
 - cli
 - unit_test : base, log, cli


ColorOutputStream

Consider: Introduce base::EncodeLogger derived from base::WideLogger, and then introduce unit_test::TestContext::wlogger of type base::WideLogger. But how does this interact with ColorOutputStream?

Consider resetting output stream formatting flags in logger as in string template.

Installable shared library + headers -> https://stackoverflow.com/a/45843676/1698548

Use platform_support.hpp in platform_info.cpp



Explain logger thread safety:
- A particular instance of BasicLogger is said to be thread safe if all its
  public functions are individually and mutually thread-safe. A necessary
  condition for thread-safety is that the logging functions can be called by any
  thread, and concurrently by multiple threads without causing data races. In
  addition, it shall be considered a necessary condition that concurrent access
  never leads to adverse effects on the presentation of the log messages (such
  as "tearing").
- Also explain what thread safety means in terms of associated objects (limit, prefix, sink, map).
- A RootLogger is thread safe if the implementation of root_log() is thread-safe...                   .                      
- FileLogger is thread-safe provided that the associated file-like object is not written to in other ways concurrently.
- On Linux (and more generally, on POSIX), FileLogger can be considered thread-safe if the associated file-like object is a file in the file system (no partial writes), even if it is written to in other ways concurrently.
- StreamLogger is thread-safe provided that the associated stream is not accessed in other ways concurrently.
- PrefixLogger is thread safe provided that the base logger is thread safe.
- TimestampLogger is thread safe provided that the base logger is thread safe. But talk about monotonicity of generated timestamps                                               
- LimitLogger is thread safe provided that the base logger is thread safe.
- ChannelLogger is thread safe provided that the base logger, and all other loggers passed to the constructor are thread safe.
- DuplicatingLogger is thread safe provided that the base logger, and all other loggers passed to the constructor are thread safe.
It is generally safe for two threads to concurrently construct two loggers on top of a single shared base logger.


Logging over pipe howto:
- Application must ensure locale match
- Individual channels need to be explicitely made available.
- Only fixed log level limit is possible.
- On-wire format: (channel_index, level, prefix, message)
- On the sending side:
  - Instantiate a special Sink implementation for each channel that is to be made available.
  - In on-wide format, combine channel and message prefix into one.
- On receiving side:
  - Application specifies a base logger and an ordered list of channel names to work with.
  - For each channel name, look up channel, then add object (combo(base.message_prefix, base.channel_prefix) as channel_prefix, sink) to per-channel object list.
  - When a message arrives, find per-channel object (immediate subscription), then call sink.sink_log(incoming_level, channel_prefix, incoming_prefix, incoming_message).



Stacktraces in terminate


CMake: Enable link-time optimization


format_rfc_1123() -> time.hpp (see legacy archon)

Check timer (monotonic, process CPU time, thread CPU time) availability on Windows and macOS.

Continue porting file.hpp


- system_process.hpp
- all file addendums
- shunting_yard_parser.hpp
- alg_expr_serializer.hpp
- utf-8<->utf-16
- Check old list of fixes


Port int_codec

Consider adding support for seeking (seekp() in BufferOutputStreambuf and BasicSeedMemoryStreambuf.

What about system error codes?  Looks like error condition equivalence is only correct starting with GCC 7.4, 8.3, 9.1.  What about Clang?    What about MSVS?
