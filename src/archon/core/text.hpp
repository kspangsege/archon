/*
 * This file is part of the Archon library framework.
 *
 * Copyright (C) 2012  Kristian Spangsege <kristian.spangsege@gmail.com>
 *
 * The Archon library framework is free software: You can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * The Archon library framework is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the Archon library framework.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_CORE_TEXT_HPP
#define ARCHON_CORE_TEXT_HPP

#include <stdexcept>
#include <limits>
#include <algorithm>
#include <utility>
#include <locale>
#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <sstream>
#include <iomanip>

#include <archon/core/assert.hpp>
#include <archon/core/functions.hpp>
#include <archon/core/char_enc.hpp>
#include <archon/core/generator.hpp>
#include <archon/core/memory.hpp>


namespace archon
{
  namespace core
  {
    /**
     * Functions and objects for working with text strings.
     */
    namespace Text
    {
      /**
       * Check if the first argument is a prefix of the second
       * argument.
       *
       * \note This will generally not work with multi-byte encodings
       * such as UTF-8.
       */
      template<class Ch>
      bool is_prefix(std::basic_string<Ch> const &prefix, std::basic_string<Ch> const &s);

      template<class Ch>
      bool is_prefix(Ch const *prefix, std::basic_string<Ch> const &s);


      /**
       * Check if the first argument is a suffix of the second
       * argument.
       *
       * \note This will generally not work with multi-byte encodings
       * such as UTF-8.
       */
      template<class Ch>
      bool is_suffix(std::basic_string<Ch> const &suffix, std::basic_string<Ch> const &s);

      template<class Ch>
      bool is_suffix(Ch const *suffix, std::basic_string<Ch> const &s);


      /**
       * Check if the first argument is a substring of the second
       * argument.
       *
       * \note This will generally not work with multi-byte encodings
       * such as UTF-8.
       */
      template<class Ch>
      bool is_substring(std::basic_string<Ch> const &substring, std::basic_string<Ch> const &s);

      template<class Ch>
      bool is_substring(Ch const *substring, std::basic_string<Ch> const &s);




      /**
       * Get the part of the second argument that is before the first
       * (or last) occurance of the substring specified as the first
       * argument. If the specified substring is not found, then the
       * empty string is returned.
       *
       * \param last Pass true if you need the prefix before the last
       * occurance of the substring, rather than before the first
       * occurance.
       *
       * \note This will generally not work with multi-byte encodings
       * such as UTF-8.
       */
      template<class Ch>
      std::basic_string<Ch> get_prefix(std::basic_string<Ch> const &substring,
                                       std::basic_string<Ch> const &s, bool last = false);

      template<class Ch>
      std::basic_string<Ch> get_prefix(Ch const *substring,
                                       std::basic_string<Ch> const &s, bool last = false);


      /**
       * Get the part of the second argument that is after the last
       * (or first) occurance of the substring specified as the first
       * argument. If the specified substring is not found, then the
       * empty string is returned.
       *
       * \param first Pass true if you need the suffix after the first
       * occurance of the substring, rather than after the last
       * occurance.
       *
       * \note This will generally not work with multi-byte encodings
       * such as UTF-8.
       */
      template<class Ch>
      std::basic_string<Ch> get_suffix(std::basic_string<Ch> const &substring,
                                       std::basic_string<Ch> const &s, bool first = false);

      template<class Ch>
      std::basic_string<Ch> get_suffix(Ch const *substring,
                                       std::basic_string<Ch> const &s, bool first = false);




      /**
       * <pre>
       *
       *   split("1;2;3", ";", std::back_inserter(vec));
       *
       * </pre>
       */
      template<class Ch, class Out>
      void split(std::basic_string<Ch> const &s, std::basic_string<Ch> const &delim, Out out,
                 bool discard_empty = false);

      template<class Ch, class Out>
      void split(std::basic_string<Ch> const &s, Ch const *delim, Out out,
                 bool discard_empty = false);


      /**
       * \tparam In An iterator whose element type is convertible to
       * std::basic_string<Ch>.
       */
      template<class In, class Ch>
      std::basic_string<Ch> join(In begin, In end, std::basic_string<Ch> const &delim);

      template<class In, class Ch>
      std::basic_string<Ch> join(In begin, In end, Ch const *delim);




      /**
       * Transform the specified wide character string into a
       * multi-byte encoded string of characters from the portable
       * character set. This is done according to the rules prescribed
       * by the specified locale.
       *
       * The function is guaranteed to successfully convert any wide
       * character string that contains characters from the portable
       * character set only.
       *
       * This function is guaranteed to fail if the wide character
       * string contains a character that uses more than one byte in
       * the multi-byte encoding.
       *
       * It is unspecified whether this function fails if the wide
       * character string contains a character that uses only one byte
       * in the multi-byte encoding, but is not part of the portable
       * character set.
       *
       * This function uses std::ctype<Ch>::narrow() to perform the
       * transcoding.
       *
       * \sa CharEnc
       */
      template<typename Ch>
      std::string narrow_port(std::basic_string<Ch> const &str,
                              std::locale const &loc) throw (NarrowException);


      /**
       * Same as the two argument version, except in this case, no
       * exception is thrown. Instead, the specified replacement
       * character is used whenever a wide character cannot be
       * converted to a narrow one.
       */
      template<typename Ch>
      std::string narrow_port(std::basic_string<Ch> const &str, char replacement,
                              std::locale const &loc) throw ();


      /**
       * Same as the two argument version, except in this case, no
       * exception is thrown. Instead, true is returned if, and only
       * if the conversion was successful.
       */
      template<typename Ch>
      bool narrow_port(std::basic_string<Ch> const &str, std::string &result,
                       std::locale const &loc) throw ();


      /**
       * Transform the specified multi-byte encoded string of
       * characters from the portable character set into a string of
       * wide characters. This is done according to the rules
       * prescribed by the specified locale.
       *
       * \sa CharEnc
       */
      template<typename Ch>
      std::basic_string<Ch> widen_port(std::string const &str,
                                       std::locale const &loc) throw ();




      template<typename Ch>
      std::basic_string<Ch> toupper(std::basic_string<Ch> const &str,
                                    std::locale const &loc) throw ();

      template<typename Ch>
      std::basic_string<Ch> tolower(std::basic_string<Ch> const &str,
                                    std::locale const &loc) throw ();




      /**
       * Scan through the specified region of the specified string and
       * convet any capital letter from the ASCII character set to the
       * corresponding lower case character. That is, this function
       * assumes that the input bytes are 7-bit ASCII characters. It
       * works according to the "C" or "POSIX" locale, but
       * independently of locale settings.
       *
       * It is not an error if the character encoding of the passed
       * string is ISO-8859-1 or UTF-8 or any other characters set
       * that includes ASCII as a sub set, but of course it will still
       * only target the plain old 26 letters of the english alphabet.
       *
       * \param s The target string to be manipulated.
       *
       * \param i The index of the first character in the string to
       * consider for lower casing.
       *
       * \param n The total number of characters in the string to
       * consider for lower casing.
       *
       * \throw std::invalid_argument If 'i' is out of range.
       */
/*
      void tolower_ascii_in(std::string &s, std::string::size_type i=0,
                            std::string::size_type n=std::string::npos);
*/

      /**
       * Just like 'tolower_ascii_in', but converts to upper case
       * instead of lower case.
       */
/*
      void toupper_ascii_in(std::string &s, std::string::size_type i=0,
                            std::string::size_type n=std::string::npos);
*/

/*

NOTE: THESE ARE ALMOST ALL RIGTH UNDER THE ASSUMPTION THAT THE STRINGS CONTAIN ASCII ENCODED CHARACTERS. THE ONLY THING THAT IS NEEDED, IS TO REPLACE THE CHARACTER CONSTANTS WITH FIXED NUMBERS. SEE ALSO <archon/dom/impl/util.hpp>

      void tolower_ascii_in(string &s, string::size_type i, string::size_type n)
      {
        if(s.size() < i) throw invalid_argument("Index out of range");
        n = min(s.size() - i, n);
        string::iterator a = s.begin()+i, b = a+n;
        while(a < b)
        {
          char c = *a;
          if('A' <= c && c <= 'Z') *a |= ' ';
          a++;
        }
      }

      void toupper_ascii_in(string &s, string::size_type i, string::size_type n)
      {
        if(s.size() < i) throw invalid_argument("Index out of range");
        n = min(s.size() - i, n);
        string::iterator a = s.begin()+i, b = a+n;
        while(a < b)
        {
          char c = *a;
          if('a' <= c && c <= 'z') *a &= ~' ';
          *a++;
        }
      }
*/




      /**
       * Provides various forms of removal of specific classes of
       * characters. The character classes are determined by the
       * specified locale.
       */
      template<class C> struct BasicTrimmer
      {
        typedef C                           CharType;
        typedef std::basic_string<CharType> StringType;
        typedef std::ctype<CharType>        CtypeType;

        BasicTrimmer(std::locale const &loc = std::locale::classic());

        /**
         * Strip leading and trailing white space. Or if \c m is
         * specified, strip leading and trailing characters of that
         * class instead.
         */
        StringType trim(StringType s, std::ctype_base::mask m = std::ctype_base::space) const;

        /**
         * Strip leading and trailing characters that do not belong to
         * the specified class.
         */
        StringType trim_not(StringType s, std::ctype_base::mask m) const;

        /**
         * Strip leading and trailing blank lines.
         *
         * While \c trim with white space removal will eliminate any
         * indent on the first line of the input, this method will
         * retain that indent. Otherwise the two methods are
         * identical.
         */
        StringType line_trim(StringType s) const;

      private:
        std::locale const loc;
        CtypeType const &ctype_facet;
        CharType const nl, cr;
      };

      typedef BasicTrimmer<char>    Trimmer;
      typedef BasicTrimmer<wchar_t> WideTrimmer;



      /**
       * Strip leading and trailing white space assuming the character
       * representation is ASCII.
       *
       * \note This function constructs a new Trimmer object every
       * time it is called, so if you need to trim many strings, you
       * will gain performace by constructing that object yourself,
       * and only construct it once.
       */
      std::string trim_ascii(std::string s);

      /**
       * Strip leading and trailing blank lines assuming the character
       * representation is ASCII.
       *
       * While \c trim_ascii will eliminate any indent on the first
       * line of the input this function retains that
       * indent. Otherwise they are identical.
       *
       * \note This function constructs a new Trimmer object every
       * time it is called, so if you need to trim many strings, you
       * will gain performace by constructing that object yourself,
       * and only construct it once.
       */
      std::string line_trim_ascii(std::string s);



      template<class Ch, class Obj> struct PrinterBase
      {
        virtual std::basic_string<Ch> print(Obj const &) const = 0;
        virtual ~PrinterBase() {}
      };



      struct ParseException: std::runtime_error
      {
        ParseException(std::string m): std::runtime_error(m) {}
      };

      /**
       * Print values to strings according to the specified locale, or
       * the "C" locale if nothing is specified.
       */
      template<class C> struct BasicValuePrinter
      {
        /**
         * \note This method is not thread safe.
         */
        template<class T> std::basic_string<C> print(T) const;

        BasicValuePrinter(std::locale const &loc = std::locale::classic()) { o.imbue(loc); }
        BasicValuePrinter(BasicValuePrinter const &p) { o.imbue(p.o.getloc()); }
        BasicValuePrinter &operator=(BasicValuePrinter const &p);

      private:
        mutable std::basic_ostringstream<C> o;
      };

      typedef BasicValuePrinter<char> ValuePrinter;
      typedef BasicValuePrinter<wchar_t> WideValuePrinter;


      /**
       * Parse values from strings according to the specified locale,
       * or the "C" locale if nothing is specified.
       */
      template<class C> struct BasicValueParser
      {
        /**
         * \note This method is not thread safe.
         */
        template<class T> T parse(std::basic_string<C>) const throw(ParseException);

        BasicValueParser(std::locale const &loc = std::locale::classic());
        BasicValueParser(BasicValueParser const &p);
        BasicValueParser &operator=(BasicValueParser const &p);

      private:
        mutable std::basic_istringstream<C> i;
      };

      typedef BasicValueParser<char>    ValueParser;
      typedef BasicValueParser<wchar_t> WideValueParser;


      /**
       * The combination of a value printer and parser.
       */
      template<class C> struct BasicValueCodec: BasicValuePrinter<C>, BasicValueParser<C>
      {
        BasicValueCodec(std::locale const &loc = std::locale::classic());
      };

      typedef BasicValueCodec<char>    ValueCodec;
      typedef BasicValueCodec<wchar_t> WideValueCodec;



      /**
       * Print values to strings according to the "C" locale.
       *
       * This function is thread safe but slow due to the fact that it
       * needs to constructs a new parser object every time it is
       * called. If you care about performance, construct the object
       * yourself and reuse it. There is about a factor of 4 in
       * performance difference.
       *
       * \sa ValuePrinter
       */
      template<class T> std::string print(T v);

      /**
       * Parse values from strings according to the "C" locale.
       *
       * This function is thread safe but slow due to the fact that it
       * needs to constructs a new parser object every time it is
       * called. If you care about performance, construct the object
       * yourself and reuse it. There is about a factor of 4 in
       * performance difference.
       *
       * \sa ValueParser
       */
      template<class T> T parse(std::string s) throw(ParseException);



      /**
       * Write the specified signed or unsigned integer, formatted
       * using base-2 (binary). A sign will be emitted if the value is
       * negative and \c twos_complement is not 'true'.
       *
       * Formatting of negative values will only work on platforms
       * that use two's complement representation. This is not
       * guaranteed by C++.
       */
      template<class Ch, class Tr, class T>
      std::basic_ostream<Ch, Tr> &format_binary(std::basic_ostream<Ch, Tr> &out, T v,
                                                bool leading_zeroes = false,
                                                bool twos_complement = false);



      template<class Ch, class Tok> struct InputTokenizer: Generator<Tok>
      {
        typedef Ch                           CharType;
        typedef std::basic_string<CharType>  StringType;
        typedef std::basic_istream<CharType> StreamType;
        typedef Tok                          Token;
        typedef CharType const              *CharPtr;
        typedef std::pair<CharPtr, CharPtr>  CharPtrPair;

        /**
         * Construct a tokeized whose input is an STL stream.
         *
         * \param in The stream to be tokenized.
         *
         * \param return_delims Also produce tokens from non-empty
         * delimiting strings.
         *
         * \param return_empty Also produce empty tokens before a
         * delimitor that is immediately preceeded by another
         * delimiter or the start of input.
         */
        InputTokenizer(StreamType &in, bool return_delims = false, bool return_empty = false);

        /**
         * Generate the next token.
         *
         * \sa Generator<>::generate
         */
        bool generate(Token &);

      protected:
        /**
         * Search for the next token delimiter. A token delimiter may
         * be an empty string.
         *
         * \param begin Start searching from this character.
         *
         * \param end The character immediately following the range of
         * characters to search. It is guaranteed that begin is
         * strictly less than end. That is, there will always be at
         * least one character to search.
         *
         * \return If a delimitor is found then the first element in
         * the pair points to the first character of the delimiter and
         * the the last element points to the character immediately
         * following the delimitor. If the delimitor is empty the two
         * elements will be equal. If a delimitor is not found then
         * the second element shall be null and the first element
         * shall point to the first character of a final section of
         * the character range that must be preserved as the start of
         * data during the next delimitor search.
         *
         * \sa delim_search_eoi()
         */
        virtual CharPtrPair delim_search(CharPtr begin, CharPtr end) = 0;


        /**
         * Similar to delim_search(), but called when your
         * delim_search() requests more input, and no more input is
         * available due to end-of-input having been reached.
         *
         * This method may be called also when \c begin and \c end are
         * equal.
         *
         * The default implementation of this method simply reports
         * that the end of input is the end of the final token. If
         * this is not appropriate for your application, override this
         * method.
         *
         * If this method returns with a request for more input, the
         * remaining untokenized input is simply discarded, and the
         * tokenizer reports successfull completion.
         *
         * If this method returns with a dilimiter, and that delimiter
         * ends before the end-of-input, then delim_search() will be
         * called next, and the fact, that the end-of-input condition
         * was detected, is forgotten.
         */
        virtual CharPtrPair delim_search_eoi(CharPtr begin, CharPtr end);


        /**
         * Produce a token for the current prefix of the input
         * stream. If you specified at construction time that you also
         * want delimitind trings to produce tokens, then this method
         * will be called for each non-empty delimiting string too and
         * the \c is_delim will indicate wether this passed string is
         * a delimiter or not.
         *
         * \param s The current prefix of the input stream as defined
         * by <tt>delim_search</tt>.
         *
         * \param is_delim True iff the passed string is a delimiter
         * rather than a regular token.
         *
         * \return The token corresponding with the passed string.
         *
         * \note If \c is_delim is false, then it may be assumed that
         * this token is the one whose end was found by the last call
         * to delim_search() or delim_search_eoi().
         */
        virtual Token token(StringType s, bool is_delim) = 0;

      private:
        void read();

        StreamType &in;
        bool return_delims, return_empty;
        CharType buffer[4096];
        CharPtr begin, end;
        StringType delim;
      };



      /**
       * A derivative of InputTokenizer where the tokens are the
       * delimited strings themselves.
       */
      template<class Ch> struct InputSplitter: InputTokenizer<Ch, std::basic_string<Ch> >
      {
        typedef InputTokenizer<Ch, std::basic_string<Ch> > _Super;
        typedef typename _Super::StringType StringType;
        typedef typename _Super::StreamType StreamType;

        InputSplitter(StreamType &, bool return_delims = false, bool return_empty = false);

      private:
        StringType token(StringType, bool);
      };



      /**
       * Split the input into tokens delimited as specified.
       */
      template<class Ch> struct SimpleTokenizer: InputSplitter<Ch>
      {
        typedef InputSplitter<Ch> _Super;
        typedef typename _Super::CharType    CharType;
        typedef typename _Super::StringType  StringType;
        typedef typename _Super::StreamType  StreamType;
        typedef typename _Super::CharPtr     CharPtr;
        typedef typename _Super::CharPtrPair CharPtrPair;

        /**
         * <pre>
         *
         *                          Return
         *    Delimiting    Return  empty   Combine
         *    mode          delims  tokens  delims    Description
         *   -----------------------------------------------------------------------
         *    regular       no       no      yes (1)  Non-empty regular tokens only
         *    incl_empty    no       yes     no  (2)  Include empty tokens
         *    incl_delims   yes      no      no       Include single delim tokens
         *    delims_empty  yes      yes     no  (2)  Include delim and empty tokens
         *    comb_delims   yes      no (2)  yes      Include combined delimiters
         *
         *   (1) Whether delimitors are combined makes no difference to the application.
         *   (2) "Return empty tokens" and "Combine delims" mutually exclude each other.
         *
         * </pre>
         */
        enum DelimMode
        {
          regular,
          incl_empty,
          incl_delims,
          delims_empty,
          comb_delims
        };

        /**
         * \param delims The set of characters that act as
         * delimiters. Passing the empty string is interpreted as
         * meaning "all white space characters".
         *
         * \param return_delims If true then each delimiting character
         * in the input will be returned as an individual token.
         *
         * \param mode The delimiting mode. See DelimMode.
         *
         * \param The locale to use when determining which characters
         * are white space. Used only when an empty string is
         * specified for 'delims'.
         */
        SimpleTokenizer(StreamType &, StringType const &delims = StringType(),
                        DelimMode mode = regular, std::locale const &loc = std::locale());

      private:
        CharPtrPair delim_search(CharPtr, CharPtr);

        StringType const delims;
        bool const combine_delims;
        CharPtr const delims_begin, delims_end;
        std::locale const loc;
        std::ctype<CharType> const *const ctype;
      };



      /**
       * Split an input stream into its constituent lines.
       */
      template<class Ch> struct LineReader: SimpleTokenizer<Ch>
      {
        typedef SimpleTokenizer<Ch> _Super;
        typedef typename _Super::StreamType  StreamType;
        LineReader(StreamType &, std::locale const &loc = std::locale());
      };



      /**
       * Formats text to a certain width, or flatterns text to one
       * line if \c width < 0.
       *
       * Below a text is viewed as consisting of words delimited by
       * sequences of white-space characters.
       *
       * In this context white-space characters are considered to be:
       *
       * <pre>
       *
       *   - " ",  SP,    space
       *   - "\t", HT,    horizontal tab
       *   - "\r", CR,    carrage return
       *   - "\n", LF/NL, line-feed/new-line
       *
       * </pre>
       *
       *
       * All sequences of white-space characters are reduced to one
       * of:
       *
       * - A single space
       *
       * - One or more newline characters
       *
       * - Nothing
       *
       * depending on where it occurs in the text and the value of \c
       * width (especially whether it is negative or not.)
       *
       *
       * <h3>Existing line breaks</h3>
       *
       * If \c width >= 0 then newline characters occuring in the
       * source text are preserved, except when they occur at the end
       * of the text, that is, when no words follow. If \c width < 0
       * then newline characters are discarded.
       *
       *
       * <h3>Line splitting<h3>
       *
       * If \c width > 0 then lines are split into pieces, so that
       * each resulting line has a maximum length of <tt>width</tt>.
       * Whenever possible the splitting is done right after the last
       * word that fits on the line.  This only fails if we encounter
       * a word that is wider than <tt>width</tt>. In this case the
       * word is split into as many lines as needed in such a way that
       * all except the last line have length exactly equal to
       * <tt>width</tt>.
       */
      template<class Ch>
      std::basic_string<Ch> format(std::basic_string<Ch> const &, int width = 0,
                                   std::locale const &loc = std::locale());




      template<class C> struct HexDecoder
      {
        typedef C CharType;
        typedef std::basic_string<C> StringType;

        CharType decode(StringType const &s) const
        {
          in.str(s);
          in.clear();
          unsigned long v;
          in >> v;
          if(in.fail() || in.bad()|| in.get() != std::char_traits<CharType>::eof() ||
             int_less_than(std::numeric_limits<CharType>::max(), v))
            throw ParseException("Bad hex digit");
          return std::char_traits<CharType>::to_char_type(v);
        }

        /**
         * For wide strings (std::wstring), the locale matters. For
         * narrow strings (std::string), you can always use
         * std::locale::classic().
         *
         * \sa CharEnc.
         */
        HexDecoder(std::locale const &l)
        {
          in.imbue(std::locale(std::locale::classic(), l, std::locale::ctype));
          in.setf(std::ios_base::hex, std::ios_base::basefield);
        }

      private:
        mutable std::basic_istringstream<CharType> in;
      };





      template<class C> struct BasicOptionalWordQuoter
      {
        typedef C CharType;
        typedef std::basic_string<C> StringType;

        /**
         * Assumes the specified string is the next word to be
         * considered for quoting. The word is quoted if it has to be
         * based on the characters in it. It is quoted in any case if
         * \a always_quote was set to true.
         */
        StringType print(StringType const &s, bool always_quote = false) const;

        /**
         * Isolate the next word from the specified string using white-space
         * as delimiter and taking double-quotation and escaping ala
         * C-strings into account.
         *
         * \param s The string of which the first word must be
         * isolated. If, and only if true is returned, this argument
         * will be updated to reflect the removal of the first word.
         *
         * \param w The extracted word is stored in this variable, but
         * only if true is returned.
         *
         * \return True unless \c s contains no more words. If false
         * is returned, neither argument is modified.
         *
         * \throw ParseException If am error is detected in the
         * quoting or the escaping syntax.
         */
        bool parse(StringType &s, StringType &w) const;

        /**
         * \param special A set of grachical characters
         * (std::ctype_base::graph) beyond ( ), ("), and (\) that may
         * not appear in an unquoted string.
         */
        BasicOptionalWordQuoter(StringType special = StringType(),
                                std::locale const &l = std::locale());

      private:
        StringType const extra, quot_extra;
        std::locale const loc; // Classic but with ctype facet from constructor arg
        std::ctype<CharType> const &ctype_facet;
      };


      typedef BasicOptionalWordQuoter<char>    OptionalWordQuoter;
      typedef BasicOptionalWordQuoter<wchar_t> WideOptionalWordQuoter;









      // Implementation:


      template<class Ch>
      inline bool is_prefix(std::basic_string<Ch> const &prefix, std::basic_string<Ch> const &s)
      {
        return s.compare(0, prefix.size(), prefix) == 0;
      }

      template<class Ch>
      inline bool is_prefix(Ch const *prefix, std::basic_string<Ch> const &s)
      {
        return is_prefix(std::basic_string<Ch>(prefix), s);
      }


      template<class Ch>
      inline bool is_suffix(std::basic_string<Ch> const &suffix, std::basic_string<Ch> const &s)
      {
        typename std::basic_string<Ch>::size_type n = suffix.size(), m = s.size();
        return n <= m && s.compare(m-n, n, suffix) == 0;
      }

      template<class Ch>
      inline bool is_suffix(Ch const *suffix, std::basic_string<Ch> const &s)
      {
        return is_suffix(std::basic_string<Ch>(suffix), s);
      }


      template<class Ch>
      inline bool is_substring(std::basic_string<Ch> const &substring,
                               std::basic_string<Ch> const &s)
      {
        return s.find(substring) != std::basic_string<Ch>::npos;
      }

      template<class Ch>
      inline bool is_substring(Ch const *substring, std::basic_string<Ch> s)
      {
        return is_substring(std::basic_string<Ch>(substring), s);
      }


      template<class Ch>
      inline std::basic_string<Ch> get_prefix(std::basic_string<Ch> const &substring,
                                              std::basic_string<Ch> const &s, bool last)
      {
        typedef typename std::basic_string<Ch>::size_type size_type;
        size_type const i = last ? s.rfind(substring) : s.find(substring);
        if(i == std::basic_string<Ch>::npos) return std::basic_string<Ch>();
        return std::basic_string<Ch>(s,0,i);
      }

      template<class Ch>
      inline std::basic_string<Ch> get_prefix(Ch const *substring,
                                              std::basic_string<Ch> const &s, bool last)
      {
        return get_prefix(std::basic_string<Ch>(substring), s, last);
      }


      template<class Ch>
      inline std::basic_string<Ch> get_suffix(std::basic_string<Ch> const &substring,
                                              std::basic_string<Ch> const &s, bool first)
      {
        typedef typename std::basic_string<Ch>::size_type size_type;
        size_type const i = first ? s.find(substring) : s.rfind(substring);
        if (i == std::basic_string<Ch>::npos) return std::basic_string<Ch>();
        return std::basic_string<Ch>(s, i+substring.size());
      }

      template<class Ch>
      inline std::basic_string<Ch> get_suffix(Ch const *substring,
                                              std::basic_string<Ch> const &s, bool first)
      {
        return get_suffix(std::basic_string<Ch>(substring), s, first);
      }


      template<class Ch, class Out>
      inline void split(std::basic_string<Ch> const &s, std::basic_string<Ch> const &delim,
                        Out out, bool discard_empty)
      {
        if (delim.empty()) throw std::runtime_error("Empty delimiter");
        typedef typename std::basic_string<Ch>::const_iterator iter;
        iter i = s.begin();
        iter const e = s.end();
        for (;;) {
          using std::search;
          iter const j = search(i, e, delim.begin(), delim.end());
          if (!discard_empty || i != j) *out++ = std::basic_string<Ch>(i,j);
          if (j == e) break;
          i = j + delim.size();
        }
      }

      template<class Ch, class Out>
      inline void split(std::basic_string<Ch> const &s, Ch const *delim,
                        Out out, bool discard_empty)
      {
        split(s, std::basic_string<Ch>(delim), out, discard_empty);
      }


      template<class In, class Ch>
      inline std::basic_string<Ch> join(In begin, In end, std::basic_string<Ch> const &delim)
      {
        std::basic_ostringstream<Ch> out;
        if (begin != end) {
          out << *begin;
          while(++begin != end) out << delim << *begin;
        }
        return out.str();
      }

      template<class In, class Ch>
      inline std::basic_string<Ch> join(In begin, In end, Ch const *delim)
      {
        return join(begin, end, std::basic_string<Ch>(delim));
      }




      template<typename Ch>
      inline std::string narrow_port(std::basic_string<Ch> const &str,
                                     std::locale const &loc) throw (NarrowException)
      {
        std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(loc);
        std::string str2;
        str2.reserve(str.size());
        typedef typename std::basic_string<Ch>::const_iterator iter;
        iter const end = str.end();
        for (iter i=str.begin(); i!=end; ++i) {
          char const ch = ctype.narrow(*i, '\0');
          if (ch == '\0' && ctype.narrow(*i, 'c') == 'c')
            throw NarrowException("Unrepresentable character");
          str2 += ch;
        }
        return str2;
      }


      template<typename Ch>
      inline std::string narrow_port(std::basic_string<Ch> const &str, char replacement,
                                     std::locale const &loc) throw ()
      {
        std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(loc);
        std::string str2;
        str2.reserve(str.size());
        typedef typename std::basic_string<Ch>::const_iterator iter;
        iter const end = str.end();
        for (iter i=str.begin(); i!=end; ++i) str2 += ctype.narrow(*i, replacement);
        return str2;
      }


      template<typename Ch>
      inline bool narrow_port(std::basic_string<Ch> const &str, std::string &result,
                              std::locale const &loc) throw ()
      {
        std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(loc);
        std::string str2;
        str2.reserve(str.size());
        typedef typename std::basic_string<Ch>::const_iterator iter;
        iter const end = str.end();
        for (iter i=str.begin(); i!=end; ++i) {
          char const ch = ctype.narrow(*i, '\0');
          if (ch == '\0' && ctype.narrow(*i, 'c') == 'c') return false;
          str2 += ch;
        }
        result = str2;
        return true;
      }


      template<typename Ch>
      inline std::basic_string<Ch> widen_port(std::string const &str, std::locale const &loc) throw ()
      {
        std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(loc);
        std::basic_string<Ch> str2;
        str2.reserve(str.size());
        typedef typename std::string::const_iterator iter;
        iter const end = str.end();
        for (iter i=str.begin(); i!=end; ++i) str2 += ctype.widen(*i);
        return str2;
      }




      template<typename Ch>
      inline std::basic_string<Ch> toupper(std::basic_string<Ch> const &str,
                                           std::locale const &loc) throw ()
      {
        std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(loc);
        std::basic_string<Ch> str2;
        str2.reserve(str.size());
        typedef typename std::basic_string<Ch>::const_iterator iter;
        iter const end = str.end();
        for (iter i=str.begin(); i!=end; ++i) str2 += ctype.toupper(*i);
        return str2;
      }

      template<typename Ch>
      inline std::basic_string<Ch> tolower(std::basic_string<Ch> const &str,
                                           std::locale const &loc) throw ()
      {
        std::ctype<Ch> const &ctype = std::use_facet<std::ctype<Ch> >(loc);
        std::basic_string<Ch> str2;
        str2.reserve(str.size());
        typedef typename std::basic_string<Ch>::const_iterator iter;
        iter const end = str.end();
        for (iter i=str.begin(); i!=end; ++i) str2 += ctype.tolower(*i);
        return str2;
      }





      template<class C> inline
      BasicTrimmer<C>::BasicTrimmer(std::locale const &l):
        loc(l), ctype_facet(std::use_facet<CtypeType>(loc)),
        nl(ctype_facet.widen('\n')), cr(ctype_facet.widen('\r')) {}

      template<class C> inline typename BasicTrimmer<C>::StringType
      BasicTrimmer<C>::trim(StringType s, std::ctype_base::mask m) const
      {
        typename StringType::const_iterator i = s.begin(), j = s.end();
        while(i != j && ctype_facet.is(m,     *i)) ++i;
        while(i != j && ctype_facet.is(m, *(j-1))) --j;
        return StringType(i,j);
      }

      template<class C> inline typename BasicTrimmer<C>::StringType
      BasicTrimmer<C>::trim_not(StringType s, std::ctype_base::mask m) const
      {
        typename StringType::const_iterator i = s.begin(), j = s.end();
        while(i != j && !ctype_facet.is(m,     *i)) ++i;
        while(i != j && !ctype_facet.is(m, *(j-1))) --j;
        return StringType(i,j);
      }

      template<class C> inline typename BasicTrimmer<C>::StringType
      BasicTrimmer<C>::line_trim(StringType s) const
      {
        typedef typename StringType::const_iterator iter;
        iter i = s.begin(), j = s.end();
        iter f = i;
        while(i != j && ctype_facet.is(std::ctype_base::space, *i))
        {
          CharType c = *i++;
          if(c == nl || c == cr) f = i;
        }
        while(i != j && ctype_facet.is(std::ctype_base::space, *(j-1))) --j;
        return i == j ? StringType() : StringType(f,j);
      }


      inline std::string trim_ascii(std::string s)
      {
        return Trimmer(std::locale::classic()).trim(s);
      }

      inline std::string line_trim_ascii(std::string s)
      {
        return Trimmer(std::locale::classic()).line_trim(s);
      }




      template<class C> template<class T>
      std::basic_string<C> BasicValuePrinter<C>::print(T v) const
      {
        o.str(std::basic_string<C>());
        o.clear();
        o << v;
        return o.str();
      }

      template<class C>
      BasicValuePrinter<C> &BasicValuePrinter<C>::operator=(BasicValuePrinter const &p)
      {
        o.imbue(p.o.getloc());
        return *this;
      }


      template<class C> template<class T>
      T BasicValueParser<C>::parse(std::basic_string<C> s) const
        throw(ParseException)
      {
        i.str(s);
        i.clear();
        T v;
        i >> v;
        if (!i || i.get() != std::char_traits<C>::eof())
          throw ParseException("Unrepresentable value '"+env_encode(s)+"'");
        return v;
      }

      template<class C> BasicValueParser<C>::BasicValueParser(std::locale const &loc)
      {
        i.imbue(loc);
        i.flags(i.flags() & ~std::ios_base::skipws);
      }

      template<class C> BasicValueParser<C>::BasicValueParser(BasicValueParser const &p)
      {
        i.imbue(p.i.getloc());
        i.flags(i.flags() & ~std::ios_base::skipws);
      }

      template<class C>
      BasicValueParser<C> &BasicValueParser<C>::operator=(BasicValueParser const &p)
      {
        i.imbue(p.i.getloc());
        return *this;
      }


      template<class C>
      BasicValueCodec<C>::BasicValueCodec(std::locale const &loc):
        BasicValuePrinter<C>(loc), BasicValueParser<C>(loc) {}


      template<class T> std::string print(T v)
      {
        return ValuePrinter().print(v);
      }

      template<class T> T parse(std::string s) throw(ParseException)
      {
        return ValueParser().parse<T>(s);
      }


      template<class Ch, class Tr, class T>
      std::basic_ostream<Ch, Tr> &format_binary(std::basic_ostream<Ch, Tr> &out,
                                                T v, bool leading_zeroes, bool twos_compl)
      {
        if(int_less_than(v,0) && !twos_compl)
        {
          out << '-';
          if(v == std::numeric_limits<T>::min())
          {
            out << "1";
            for(int i=0; i<std::numeric_limits<T>::digits; ++i) out << '0';
            return out;
          }
          v = -v;
        }
        int pos = sizeof(T)*std::numeric_limits<unsigned char>::digits - 1;
        for(;;)
        {
          bool digit = v & T(1) << pos;
          if(digit || leading_zeroes || !pos) out << (digit ? '1' : '0');
          if(!pos) break;
          if(digit) leading_zeroes = true;
          --pos;
        }
        return out;
      }



      template<class C, class T>
      InputTokenizer<C, T>::InputTokenizer(StreamType &in, bool return_delims,
                                           bool return_empty):
        in(in), return_delims(return_delims), return_empty(return_empty),
        begin(buffer), end(buffer)
      {
        read();
      }


      template<class C, class T> inline typename InputTokenizer<C, T>::CharPtrPair
      InputTokenizer<C, T>::delim_search_eoi(CharPtr, CharPtr end)
      {
        return CharPtrPair(end, end);
      }


      template<class C, class T> bool InputTokenizer<C, T>::generate(Token &t)
      {
        if(!delim.empty())
        {
          t = token(delim, true);
          delim.clear();
          return true;
        }

        StringType s;

        for(;;)
        {
          CharPtrPair d(end, CharPtr(0));
          if(begin < end) d = delim_search(begin, end);
          if(!d.second)
          {
            if(!in.eof())
            {
              if(buffer < d.first)
              {
                s.append(begin, d.first);
                begin = buffer;
                end = std::copy(d.first, end, buffer);
              }
              read();
              continue;
            }

            if(begin == end && s.empty()) return false;

            d = delim_search_eoi(begin, end);
            if(!d.second)
            {
              // Discard untokenized input
              s.clear();
              begin = end;
              return false;
            }
          }

          s.append(begin, d.first);
          begin = d.second;
          if(return_delims) delim = StringType(d.first, d.second);
          if(return_empty || !s.empty())
          {
            t = token(s, false);
            return true;
          }
          if(!delim.empty())
          {
            t = token(delim, true);
            delim.clear();
            return true;
          }
        }
      }


      template<class C, class T> inline void InputTokenizer<C, T>::read()
      {
        std::streamsize p = end-buffer, n = sizeof(buffer)/sizeof(*buffer)-p;
        if(!n) throw std::runtime_error("Failed to split input: "
                                        "Required context exceeds buffer capasity");
        in.read(buffer+p, n);
        if(in.bad()) throw std::runtime_error("Error while reading");
        end += in.gcount();
      }


      template<class C>
      inline InputSplitter<C>::InputSplitter(StreamType &in,
                                             bool return_delims, bool return_empty):
        InputTokenizer<C, std::basic_string<C> >(in, return_delims, return_empty) {}

      template<class C>
      inline typename InputSplitter<C>::StringType InputSplitter<C>::token(StringType s, bool)
      {
        return s;
      }


      template<class Ch>
      inline SimpleTokenizer<Ch>::SimpleTokenizer(StreamType &in, StringType const &delims,
                                                  DelimMode mode, std::locale const &l):
        InputSplitter<Ch>(in, mode == incl_delims || mode == delims_empty || mode == comb_delims,
                          mode == incl_empty || mode == delims_empty),
        delims(delims), combine_delims(mode == regular || mode == comb_delims),
        delims_begin(delims.data()), delims_end(delims_begin+delims.size()),
        loc(l), ctype(delims.empty() ? &std::use_facet<std::ctype<CharType> >(loc): 0) {}

      template<class Ch> inline typename SimpleTokenizer<Ch>::CharPtrPair
      SimpleTokenizer<Ch>::delim_search(CharPtr begin, CharPtr end)
      {
        CharPtr const d = ctype ? ctype->scan_is(std::ctype_base::space, begin, end) :
          std::find_first_of(begin, end, delims_begin, delims_end);
        if(d == end) return CharPtrPair(end, CharPtr(0));
        CharPtr e = d+1;
        if(combine_delims)
        {
          if(ctype) e = ctype->scan_not(std::ctype_base::space, e, end);
          else while(e < end)
          {
            if(delims.find(*e) == StringType::npos) break;
            ++e;
          }
        }
        return CharPtrPair(d,e);
      }


      template<class Ch> inline LineReader<Ch>::LineReader(StreamType &in, std::locale const &l):
        SimpleTokenizer<Ch>(in, env_widen<Ch>("\n"), SimpleTokenizer<Ch>::incl_empty, l) {}




      template<class Ch> std::basic_string<Ch> format(std::basic_string<Ch> const &value,
                                                      int width, std::locale const &loc)
      {
        typedef Ch CharType;
        typedef std::basic_string<CharType> StringType;
        typedef typename StringType::size_type SizeType;

        // Widen some fixed strings
        BasicLocaleCharMapper<CharType> mapper(loc);
        StringType const nl = mapper.widen("\n");
        StringType const sp = mapper.widen(" ");

        SizeType position = 0; // Number of characters already put into the current line
        int pending_newlines = 0; // Number of consecutive empty lines
                                  // needing to be appended if
                                  // something follows them
        StringType buffer, word;
        std::ctype<CharType> const &ctype = std::use_facet<std::ctype<CharType> >(loc);

        std::basic_istringstream<Ch> in(value);
        SimpleTokenizer<CharType> tokenizer(in, StringType(),
                                            SimpleTokenizer<CharType>::incl_delims, loc);
        while(tokenizer.generate(word))
        {
          if(word == nl)
          {
            if(width < 0) continue;
            ++pending_newlines;
            position = 0;
            continue;
          }

          if(ctype.is(std::ctype_base::space, word[0])) continue;

          if(0 < pending_newlines)
          {
            for(int i=0; i<pending_newlines; ++i) buffer += nl;
            pending_newlines = 0;
          }

          if(0 < width)
          {
            // Break line if next word exceeds max. width (never applies to the first word on a line)
            if(0 < position && static_cast<SizeType>(width) <= word.size() + position)
            {
              buffer += nl;
              position = 0;
            }

            // Break word if it is wider than max. width (only applies to the first word on a line)
            while(position == 0 && static_cast<SizeType>(width) < word.size())
            {
              buffer += word.substr(0, static_cast<SizeType>(width));
              buffer += nl;
              word.erase(0, static_cast<SizeType>(width));
            }
          }

          if(0 < position)
          {
            buffer += sp;
            ++position;
          }
          buffer += word;
          position += word.size();
        }

        return buffer;
      }




      template<class C> typename BasicOptionalWordQuoter<C>::StringType
      BasicOptionalWordQuoter<C>::print(StringType const &s, bool always_quote) const
      {
        typename StringType::size_type n = s.size();
        if (n == 0) {
          std::basic_ostringstream<CharType> out;
          out.imbue(loc);
          out << '"' << '"';
          return out.str();
        }
        CharType c = s[0];
        typename StringType::size_type i = 0;
        for (;;) {
          if (extra.find(c) != StringType::npos || !ctype_facet.is(std::ctype_base::graph, c)) break;
          if (++i == n) { // Clean
            if (always_quote) {
              std::basic_ostringstream<CharType> out;
              out.imbue(loc);
              out << '"' << s << '"';
              return out.str();
            }
            return s;
          }
          c = s[i];
        }

        CharType const *p = s.data();
        std::basic_ostringstream<CharType> out;
        out.imbue(loc);
        out << std::hex << std::uppercase;
        out.fill(ctype_facet.widen('0'));
        out << '"';

        CharType const space = ctype_facet.widen(' ');
        typename StringType::size_type j = 0;
        if (c == space || ctype_facet.is(std::ctype_base::graph, c)) goto search;
        for (;;) {
          {
            typename StringType::size_type const m = i - j;
            if (0 < m) out.write(p+j, m);
          }

          {
            switch(ctype_facet.narrow(c, ' ')) {
            case '\0': out << "\\0";  break;
            case '\n': out << "\\n";  break;
            case '\t': out << "\\t";  break;
            case '\v': out << "\\v";  break;
            case '\b': out << "\\b";  break;
            case '\r': out << "\\r";  break;
            case '\f': out << "\\f";  break;
            case '\a': out << "\\a";  break;
            case '\\': out << "\\\\"; break;
            case '"' : out << "\\\""; break;
            default:
              typename std::char_traits<CharType>::int_type const d =
                std::char_traits<CharType>::to_int_type(c);
              ARCHON_ASSERT_1(int_less_than_equal(d, 0xFFFFFFFF),
                              "print_optionally_quoted_word: "
                              "Character value too large to be handled");
              if (0xFFFF < d) out << "\\U" << std::setw(8) << d;
              else if (0xFF < d) out << "\\u" << std::setw(4) << d;
              else out << "\\x" << std::setw(2) << d;
            }
          }
          j = ++i;

        search:
          for (;;) {
            if (i == n) {
              typename StringType::size_type const m = i - j;
              if (0 < m) out.write(p+j, m);
              out << '"';
              return out.str();
            }
            c = p[i];
            if (quot_extra.find(c) != StringType::npos ||
                c != space && !ctype_facet.is(std::ctype_base::graph, c)) break;
            ++i;
          }
        }
      }



      template<class C> bool BasicOptionalWordQuoter<C>::parse(StringType &s, StringType &w) const
      {
        typename StringType::size_type i = 0;
        while(i<s.size() && ctype_facet.is(std::ctype_base::space, s[i])) ++i;
        if(i == s.size()) return false;
        std::basic_ostringstream<CharType> out;
        HexDecoder<CharType> hex_decoder(loc);
        CharType const quot = ctype_facet.widen('"'), slosh = ctype_facet.widen('\\');
        do
        {
          if(ctype_facet.is(std::ctype_base::space, s[i])) break;
          if(s[i] == quot)
          {
            ++i;
            for(;;)
            {
              if(i == s.size())
                throw ParseException("Unterminated double-quoted part");

              if(s[i] == quot)
              {
                ++i;
                break;
              }

              if(s[i] == slosh)
              {
                ++i;
                if(i == s.size())
                  throw ParseException("Unterminated escape sequence '\\'");
                switch(ctype_facet.narrow(s[i], ' '))
                {
                case '0':  out << "\0"; break;
                case 'n':  out << "\n"; break;
                case 't':  out << "\t"; break;
                case 'v':  out << "\v"; break;
                case 'b':  out << "\b"; break;
                case 'r':  out << "\r"; break;
                case 'f':  out << "\f"; break;
                case 'a':  out << "\a"; break;
                case '\\': out << "\\"; break;
                case '"':  out << "\""; break;

                case 'x':
                  if(s.size() <= i+2) throw ParseException("Unterminated escape sequence '\\x'");
                  try
                  {
                    out << hex_decoder.decode(s,i,2);
                  }
                  catch(ParseException &)
                  {
                    throw ParseException("Bad escape sequence '\\x"+
                                         env_encode(s.substr(i,2))+"'");
                  }
                  break;

                case 'u':
                  if(s.size() <= i+4) throw ParseException("Unterminated escape sequence '\\u'");
                  try
                  {
                    out << hex_decoder.decode(s,i,4);
                  }
                  catch(ParseException &)
                  {
                    throw ParseException("Bad escape sequence '\\u"+
                                         env_encode(s.substr(i,4))+"'");
                  }
                  break;

                case 'U':
                  if(s.size() <= i+8) throw ParseException("Unterminated escape sequence '\\U'");
                  try
                  {
                    out << hex_decoder.decode(s,i,8);
                  }
                  catch(ParseException &)
                  {
                    throw ParseException("Bad escape sequence '\\U"+
                                         env_encode(s.substr(i,8))+"'");
                  }
                  break;

                default:
                  throw ParseException("Unrecognized escape sequence '\\" +
                                       env_encode(s.substr(i)) + "'");
                }

                ++i;
              }
              else
              {
                out << s[i];
                ++i;
              }
            }
          }
          else
          {
            out << s[i];
            ++i;
          }
        }
        while(i < s.size());

        s.erase(0,i);
        w = out.str();
        return true;
      }



      template<class C>
      BasicOptionalWordQuoter<C>::BasicOptionalWordQuoter(StringType special, std::locale const &l):
        extra(widen_port<CharType>("\"", l)+special),
        quot_extra(widen_port<CharType>("\"\\", l)),
        loc(std::locale::classic(), l, std::locale::ctype),
        ctype_facet(std::use_facet<std::ctype<CharType> >(loc)) {}
    }
  }
}

#endif // ARCHON_CORE_TEXT_HPP
