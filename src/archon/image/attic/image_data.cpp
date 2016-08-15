/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <archon/core/text.hpp>
#include <archon/image/color.hpp>
#include <archon/image/image_data.hpp>


using namespace std;


namespace archon
{
  namespace Imaging
  {
    template<typename Float, typename Int>
    inline Int ImageData::normFloatToInt(Float v, int intBits)
    {
      Int maxInt;
      // Be carefull not to attempt shifts by the total number of
      // bits in the integer, because that does not work.
      if(intBits == sizeof(Int)*numeric_limits<unsigned char>::digits)
	maxInt = numeric_limits<Int>::max();
      else
      {
	maxInt = 1;
	maxInt <<= intBits;
	maxInt -= 1;
      }
      v *= maxInt;
      v += 0.5;
      return v < static_cast<Float>(0) ? static_cast<Int>(0) :
	v >= static_cast<Float>(maxInt) ? maxInt : static_cast<Int>(v);
    }

    template<typename Int, typename Float, bool maskInput>
    inline Float ImageData::intToNormFloat(Int v, int intBits)
    {
      Int maxInt;
      // Be carefull not to attempt shifts by the total number of
      // bits in the integer, because that does not work.
      if(intBits == sizeof(Int)*numeric_limits<unsigned char>::digits)
	maxInt = numeric_limits<Int>::max();
      else
      {
	maxInt = 1;
	maxInt <<= intBits;
	maxInt -= 1;
      }
      if(maskInput) v &= maxInt;
      return static_cast<Float>(v)/static_cast<Float>(maxInt);
    }

    /**
     * This is neccessary because C++ does not support partial
     * specialization of function templates.
     */
    template<typename Word, bool isInteger> struct ImageData::DirectWordAccess {};

    /**
     * Specialization for integer words
     */
    template<typename Word> struct ImageData::DirectWordAccess<Word, true>
    {
      inline static
      long double decode(ImageData const *image, Word v, MemoryField const &f)
      {
	if(image->pixelFormat.mostSignificantBitsFirst)
	  v >>= image->pixelFormat.bitsPerWord-f.bitWidth;
	return intToNormFloat<Word, long double, true>(v, f.bitWidth);
      }

      inline static
      Word encode(ImageData const *image, long double v, MemoryField const &f)
      {
	Word w = normFloatToInt<long double, Word>(v, f.bitWidth);
	if(image->pixelFormat.mostSignificantBitsFirst)
	  w <<= image->pixelFormat.bitsPerWord-f.bitWidth;
	return w;
      }
    };

    /**
     * Specialization for floating point words
     */
    template<typename Word> struct ImageData::DirectWordAccess<Word, false>
    {
      inline static
      long double decode(ImageData const *image, Word v, MemoryField const &f)
      {
	return (static_cast<long double>(v)-f.min)/(f.max-f.min);
      }

      inline static
      Word encode(ImageData const *image, long double v, MemoryField const &f)
      {
	return static_cast<Word>(f.min + (f.max-f.min)*v);
      }
    };

    template<typename Word, bool nativeEndianness>
    inline Word ImageData::readWord(Word const *p) const
    {
      return nativeEndianness ? *p :
	readWithBytePermutation<Word>(p, bytePermutation);
    }

    template<typename Word, bool nativeEndianness>
    inline void ImageData::writeWord(Word v, Word *p) const
    {
      if(nativeEndianness) *p = v;
      else writeWithBytePermutation<Word>(v, p, bytePermutation);
    }

    /*
     * \todo FIXME: Make faster access modes by adding template
     * parameters for int-with-full-bit-width and
     * mostSignificantBitsFirst.
     */
    template<typename Word, bool nativeEndianness>
    void ImageData::decodePixelSequenceDirect(void const *data, int,
					      long double *tray,
					      int pitch, long n) const
    {
      int m = memoryFields.size();
      Word const *p = reinterpret_cast<Word const *>(data);
      for(int i=0; i<n; ++i) // For each pixel
      {
	for(int j=0; j<m; ++j)
	{
	  ImageData::MemoryField const &f = memoryFields[j];
	  if(f.channelIndex < 0) continue;
	  tray[f.channelIndex] =
	    DirectWordAccess<Word, numeric_limits<Word>::is_integer>::
	    decode(this, readWord<Word, nativeEndianness>(p), f);
	  ++p;
	}
	tray += pitch;
      }
    }

    /**
     * \todo FIXME: Make faster access modes by adding template
     * parameters for int-with-full-bit-width and
     * mostSignificantBitsFirst.
     */
    template<typename Word, bool nativeEndianness>
    void ImageData::encodePixelSequenceDirect(long double const *tray,
					      int pitch, long n,
					      void *data, int) const
    {
      int m = memoryFields.size();
      Word *p = reinterpret_cast<Word *>(data);
      for(int i=0; i<n; ++i) // For each pixel
      {
	for(int j=0; j<m; ++j)
	{
	  ImageData::MemoryField const &f = memoryFields[j];
	  if(f.channelIndex < 0) continue;
	  writeWord<Word, nativeEndianness>
	    (DirectWordAccess<Word, numeric_limits<Word>::is_integer>::
	     encode(this, tray[f.channelIndex], f), p);
	  ++p;
	}
	tray += pitch;
      }
    }


    template<typename Word, typename WordAssemble,
	     typename ChannelAssemble, bool nativeEndianness>
    void ImageData::decodePixelSequencePackedTight(void const *data,
						   int wordBitOffset,
						   long double *tray,
						   int pitch, long n) const
    {
      Word const *p = reinterpret_cast<Word const *>(data);
      vector<MemoryField>::const_iterator field = memoryFields.begin();

      // Skip initial unused bit fields
      int bitAdvance = 0;
      while(field->channelIndex < 0)
      {
	bitAdvance += field->bitWidth;
	if(++field != memoryFields.end()) continue;
	if(!--n) return;
	field = memoryFields.begin();
      }
      long wordAdvance;
      if(bitAdvance)
      {
	wordBitOffset = Math::modulo(wordBitOffset + bitAdvance,
				     pixelFormat.bitsPerWord, wordAdvance);
	if(wordAdvance) p += wordAdvance;
      }

      // Load the first word from memory
      WordAssemble word = readWord<Word, nativeEndianness>(p);

      // Prepare for assembly of first channel
      int channelBitWidth = field->bitWidth;
      int channelBitOffset = 0;
      ChannelAssemble channel = 0;

      // Iterate over bit chunk transfers
      for(;;)
      {
	// The number of unparsed bits remaining in the current input word
	int remainingWordBits = pixelFormat.bitsPerWord - wordBitOffset;
	int remainingChannelBits = channelBitWidth - channelBitOffset;

	// The number of bits we can transfer as one chunk
	bitAdvance = min(remainingWordBits, remainingChannelBits);

	// Special trick to support shifts by N bits where N is the
	// number of bits in WordAssemble
	WordAssemble mask = (static_cast<WordAssemble>(1)<<bitAdvance-1<<1) - 1;
	// Transfer bitAdvance bits from word to channel
	if(pixelFormat.mostSignificantBitsFirst)
	  channel |= static_cast<ChannelAssemble>(word>>remainingWordBits-bitAdvance & mask) << remainingChannelBits-bitAdvance;
	else channel |= static_cast<ChannelAssemble>(word>>wordBitOffset & mask) << channelBitOffset;

	// If the channel is not yet complete the the word mus be complete
	if(bitAdvance < remainingChannelBits)
	{
	  // Advance to next word of the image data
	  ++p;
	  word = readWord<Word, nativeEndianness>(p);
	  wordBitOffset = 0;
	  channelBitOffset += bitAdvance;
	  continue;
	}

	// Channel is complete (and maybe word is complete too)

	// Store completed channel
        tray[field->channelIndex] = intToNormFloat<ChannelAssemble, long double, false>(channel, field->bitWidth);


	// Skip unused bit fields
	for(;;)
	{
	  if(++field == memoryFields.end())
	  {
	    if(!--n) return;

	    // Start next pixel
	    field = memoryFields.begin();
	    tray += pitch;
	  }
	  if(-1 < field->channelIndex) break;
	  bitAdvance += field->bitWidth;
	}
	if(bitAdvance)
	{
	  wordBitOffset = Math::modulo<int>(wordBitOffset + bitAdvance,
					    pixelFormat.bitsPerWord,
					    wordAdvance);
	  if(wordAdvance)
	  {
	    p += wordAdvance;

	    // Load the next word from memory
	    word = readWord<Word, nativeEndianness>(p);
	  }
	}

	// Prepare for assembly of next channel
	channelBitWidth = field->bitWidth;
	channelBitOffset = 0;
	channel = 0;
      }
    }

    /**
     * \todo FIXME: Working with word masks like here is not always
     * what you want. Currently any unused bit-fields will be
     * untouched in the target buffer. Sometimes it will be important
     * that the skipped bits are actually cleared to zero. If we want
     * to clear thos bits we can improve performance since we would
     * only have to load original words at the first and at the last
     * word in tight formats.
     *
     * Word Is the integral integer type into which pixels data is
     * encoded.This type determines how many bytes are read from or
     * written to memory at a time and the order in which those bytes
     * are combined to produced wider integers. The order is also
     * affected by the chosen endianness however.
     *
     * WordAssemble Is the integral integer type used to hold a single
     * word while while manipulating bits. It must be at least as wide
     * as Word and at least as wide as an 'int', and it should be no
     * wider than neccessary.
     *
     * ChannelAssemble Is the integral integer type used to hold a
     * single channel value while manipulating bits. It must be at
     * least as wide as the widest channel and at least as wide as an
     * 'int', and it should be no wider than neccessary.
     */
    template<typename Word, typename WordAssemble,
	     typename ChannelAssemble, bool nativeEndianness>
    void ImageData::encodePixelSequencePackedTight(long double const *tray,
						   int pitch, long n,
						   void *data,
						   int wordBitOffset) const
    {
      vector<MemoryField>::const_iterator field = memoryFields.begin();
      Word *p = reinterpret_cast<Word *>(data);
      WordAssemble fullWordMask = (static_cast<WordAssemble>(1)<<pixelFormat.bitsPerWord-1<<1) - 1;

      // Skip initial unused bit fields
      int bitAdvance = 0;
      while(field->channelIndex < 0)
      {
	bitAdvance += field->bitWidth;
	if(++field != memoryFields.end()) continue;
	if(!--n) return;
	field = memoryFields.begin();
      }
      long wordAdvance;
      if(bitAdvance)
      {
	wordBitOffset = Math::modulo(wordBitOffset + bitAdvance,
				     pixelFormat.bitsPerWord, wordAdvance);
	if(wordAdvance) p += wordAdvance;
      }

      // Prepare for assembly of first word
      WordAssemble word = 0;
      WordAssemble wordMask = 0;

      // Prepare for assembly of first channel
      int channelBitWidth = field->bitWidth;
      int channelBitOffset = 0;
      ChannelAssemble channel =
	normFloatToInt<long double, ChannelAssemble>(tray[field->channelIndex], field->bitWidth);

      // Iterate over bit chunk transfers
      for(;;)
      {
	// The number of unparsed bits remaining in the current input channel
	int remainingChannelBits = channelBitWidth - channelBitOffset;
	int remainingWordBits = pixelFormat.bitsPerWord - wordBitOffset;

	// The number of bits we can transfer as one chunk
	bitAdvance = min(remainingChannelBits, remainingWordBits);

	// Special trick to support shifts by N bits where N is the
	// number of bits in WordAssemble
	WordAssemble mask = (static_cast<WordAssemble>(1)<<bitAdvance-1<<1) - 1;
	// Transfer bitAdvance bits from channel to word
	if(pixelFormat.mostSignificantBitsFirst)
	{
	  word |= (static_cast<WordAssemble>(channel>>remainingChannelBits-bitAdvance) & mask) << remainingWordBits-bitAdvance;
	  wordMask |= mask << remainingWordBits-bitAdvance;
	}
	else
	{
	  word |= (static_cast<WordAssemble>(channel>>channelBitOffset) & mask) << wordBitOffset;
	  wordMask |= mask << wordBitOffset;
	}

	// If the channel is not yet complete the the word mus be complete
	if(bitAdvance < remainingChannelBits)
	{
	  // Store completed word
	  if(wordMask != fullWordMask)
	    word |= readWord<Word, nativeEndianness>(p) & ~wordMask;
	  writeWord<Word, nativeEndianness>(word, p);

	  // Advance to next word of the image data
	  ++p;
	  wordBitOffset = 0;
	  word = wordMask = 0;
	  channelBitOffset += bitAdvance;
	  continue;
	}

	// Channel is complete (and maybe word is complete too)

	// 'word' always contains unwritten bits at this point

	// Skip unused bit fields
	for(;;)
	{
	  if(++field == memoryFields.end())
	  {
	    if(!--n)
	    {
	      if(wordMask != fullWordMask)
	        word |= readWord<Word, nativeEndianness>(p) & ~wordMask;
	      writeWord<Word, nativeEndianness>(word, p);
	      return;
	    }

	    // Start next pixel
	    field = memoryFields.begin();
	    tray += pitch;
	  }
	  if(-1 < field->channelIndex) break;
	  bitAdvance += field->bitWidth;
	}
	if(bitAdvance)
	{
	  wordBitOffset = Math::modulo<int>(wordBitOffset + bitAdvance,
					    pixelFormat.bitsPerWord, wordAdvance);
	  if(wordAdvance)
	  {
	    // Store completed word
	    if(wordMask != fullWordMask)
	      word |= readWord<Word, nativeEndianness>(p) & ~wordMask;
	    writeWord<Word, nativeEndianness>(word, p);

	    p += wordAdvance;
	    word = wordMask = 0;
	  }
	}

	// Load next channel
	channelBitWidth = field->bitWidth;
	channelBitOffset = 0;
	channel = normFloatToInt<long double, ChannelAssemble>(tray[field->channelIndex], field->bitWidth);
      }
    }



    inline void ImageData::blendWithBackground(long double const *source,
					       long double *target) const
    {
      long double const opacity = source[3];
      long double const transparency = 1 - opacity;
      target[0] = opacity * source[0] + transparency * 0 /* backgroundColor[0] */;
      target[1] = opacity * source[1] + transparency * 0 /* backgroundColor[1] */;
      target[2] = opacity * source[2] + transparency * 0 /* backgroundColor[2] */;
    }



    template<bool alpha, bool custom>
    inline void ImageData::decodeFromRgb(long double const *source,
					 long double *target, size_t n) const
    {
      size_t const m = custom ? numberOfChannels : alpha ? 4 : 3;
      for(size_t i=0; i<n; ++i)
      {
	target[0] = source[0];
	target[1] = source[1];
	target[2] = source[2];
	target[3] = alpha ? source[m-1] : 1;
	source += m;
	target += 4;
      }
    }

    template<bool alpha, bool custom>
    inline void ImageData::encodeToRgb(long double const *source,
				       long double *target, size_t n) const
    {
      size_t const m = custom ? numberOfChannels : alpha ? 4 : 3;
      for(size_t i=0; i<n; ++i)
      {
	if(alpha)
	{
	  target[0] = source[0];
	  target[1] = source[1];
	  target[2] = source[2];
	  if(custom) fill(target+3, target+m-1, 0);
	  target[m-1] = source[3];
	}
	else
	{
	  blendWithBackground(source, target);
	  if(custom) fill(target+3, target+m, 0);
	}
	source += 4;
	target += m;
      }
    }



    template<bool alpha, bool custom>
    inline void ImageData::decodeFromLuminance(long double const *source,
					       long double *target, size_t n) const
    {
      size_t const m = custom ? numberOfChannels : alpha ? 2 : 1;
      for(size_t i=0; i<n; ++i)
      {
	Color::convertLuminanceToRgb(source[0], target);
	target[3] = alpha ? source[m-1] : 1;
	source += m;
	target += 4;
      }
    }

    template<bool alpha, bool custom>
    inline void ImageData::encodeToLuminance(long double const *source,
					     long double *target, size_t n) const
    {
      size_t const m = custom ? numberOfChannels : alpha ? 2 : 1;
      for(size_t i=0; i<n; ++i)
      {
	if(alpha)
	{
	  target[0] = Color::convertRgbToLuminance(source);
	  if(custom) fill(target+1, target+m-1, 0);
	  target[m-1] = source[3];
	}
	else
	{
	  BasicVector<long double, 3> rgb;
	  blendWithBackground(source, rgb.get());
	  target[0] = Color::convertRgbToLuminance(rgb);
	  if(custom) fill(target+1, target+m, 0);
	}
	source += 4;
	target += m;
      }
    }



    template<bool alpha>
    inline void ImageData::decodeFromHsv(long double const *source,
					 long double *target, size_t n) const
    {
      size_t const m = alpha ? 4 : 3;
      for(size_t i=0; i<n; ++i)
      {
	Color::convertHsvToRgb(source, target);
	target[3] = alpha ? source[m-1] : 1;
	source += m;
	target += 4;
      }
    }

    template<bool alpha>
    inline void ImageData::encodeToHsv(long double const *source,
				       long double *target, size_t n) const
    {
      size_t const m = alpha ? 4 : 3;
      for(size_t i=0; i<n; ++i)
      {
	if(alpha)
	{
	  Color::convertRgbToHsv(source, target);
	  target[m-1] = source[3];
	}
	else
	{
	  BasicVector<long double, 3> rgb;
	  blendWithBackground(source, rgb.get());
	  Color::convertRgbToHsv(rgb.get(), target);
	}
	source += 4;
	target += m;
      }
    }



    template<typename Word> void ImageData::setupCodecDirect()
    {
      if(bytePermutation.empty())
      {
	decoder = &ImageData::decodePixelSequenceDirect<Word, true>;
	encoder = &ImageData::encodePixelSequenceDirect<Word, true>;
      }
      else
      {
	decoder = &ImageData::decodePixelSequenceDirect<Word, false>;
	encoder = &ImageData::encodePixelSequenceDirect<Word, false>;
      }
    }

    template<typename Word, typename WordAssemble, typename ChannelAssemble>
    void ImageData::setupCodecPackedTight()
    {
      if(bytePermutation.empty())
      {
	decoder = &ImageData::decodePixelSequencePackedTight<Word,
	  WordAssemble, ChannelAssemble, true>;
	encoder = &ImageData::encodePixelSequencePackedTight<Word,
	  WordAssemble, ChannelAssemble, true>;
      }
      else
      {
	decoder = &ImageData::decodePixelSequencePackedTight<Word,
	  WordAssemble, ChannelAssemble, false>;
	encoder = &ImageData::encodePixelSequencePackedTight<Word,
	  WordAssemble, ChannelAssemble, false>;
      }
    }

    template<typename Word, typename WordAssemble>
    void ImageData::setupCodecPackedTight()
    {
      // Determine the bit-width of the widest channel
      int w = 0;
      for(vector<MemoryField>::iterator i = memoryFields.begin();
	  i != memoryFields.end(); ++i)
	if(-1 < i->channelIndex && w < i->bitWidth) w = i->bitWidth;
      int const m = numeric_limits<unsigned char>::digits;

      // Determine the number of bytes needed to hold the widest channel
      size_t n = (w + m - 1) / m;

      if(n <= sizeof(unsigned char))       setupCodecPackedTight<Word,
	WordAssemble, unsigned int>();
      else if(n <= sizeof(unsigned short)) setupCodecPackedTight<Word,
	WordAssemble, unsigned int>();
      else if(n <= sizeof(unsigned int))   setupCodecPackedTight<Word,
	WordAssemble, unsigned int>();
      else if(n <= sizeof(unsigned long))  setupCodecPackedTight<Word,
	WordAssemble, unsigned long>();
      else                                 setupCodecPackedTight<Word,
	WordAssemble, PixelFormat::MaxInt>();
    }


    template<typename Word, typename WordAssemble>
    void ImageData::setupCodec()
    {
      if(pixelFormat.formatType == PixelFormat::direct)
	setupCodecDirect<Word>();
      else setupCodecPackedTight<Word, WordAssemble>();
   }



    /*
     * The objective for this constructor is to transform the
     * specified set of input parameters into a new set of parameters
     * that will minimize the number of calculations required by the
     * pixel data accessor methods of this class.
     */
    ImageData::ImageData(void *buffer, int pixelsPerStrip, int numberOfStrips,
			 BufferFormat::ConstRefArg bufferFormat,
			 int left, int bottom, int width, int height,
			 vector<bool> const &endianness):
      buffer(buffer),
      pixelsPerStrip(pixelsPerStrip), numberOfStrips(numberOfStrips),
      bufferFormat(bufferFormat), endianness(endianness)
    {
      // Determine number of bytes per word
      bytesPerWord = pixelFormat.bitsPerWord / numeric_limits<unsigned char>::digits;

      // Determine byte permutation unless the requested endianness is
      // compatible with the native endianess up through the relevant
      // levels.
      {
	int levels = findMostSignificantBit(bytesPerWord);
	if(endianness.size())
	{
	  // Only word sizes that are a power of two times the size of
	  // a byte are supported with custom endianness
	  if(bytesPerWord != 1<<levels)
	    ARCHON_THROW1(PixelFormat::InconsistencyException, "Cannot handle "
			  "word types with number of bytes ("+
			  Text::toString(bytesPerWord)+") not being a power "
			  "of two when using custom endianness");
	  vector<bool> nativeEndianness = detectNativeEndianness();
	  if(static_cast<int>(nativeEndianness.size()) < levels)
	    ARCHON_THROW1(PixelFormat::InconsistencyException, "Cannot handle "
			  "word types larger than the maximum integer type "
			  "when using custom endianness");
	  if(!compareEndianness(endianness, nativeEndianness, levels))
	    bytePermutation = computeBytePermutation(endianness, levels);
	}
      }

      // Determine number of channels
      numberOfChannels = pixelFormat.channelLayout.size();

      // Determine bits per pixel
      bitsPerPixel = pixelFormat.formatType == PixelFormat::tight ?
	pixelFormat.pixelSize : pixelFormat.pixelSize * pixelFormat.bitsPerWord;

      // Compute memory field list
      vector<int> memoryMap(pixelFormat.formatType == PixelFormat::direct ?
			    pixelFormat.pixelSize : bitsPerPixel, -1);
      for(int i=0; i<numberOfChannels; ++i) memoryMap[pixelFormat.channelLayout[i].offset] = i;
      if(pixelFormat.formatType == PixelFormat::direct)
      {
	for(int i=0; i<static_cast<int>(memoryMap.size()); ++i)
	{
	  int j = memoryMap[i];
	  if(j < 0)
	  {
	    memoryFields.push_back(MemoryField());
	    continue;
	  }
	  memoryFields.push_back(MemoryField(j, pixelFormat.channelLayout[j].width));
	}
      }
      else // packed and tight formats
      {
	int i=0;
	while(i < static_cast<int>(memoryMap.size()))
	{
	  int j = -1;
	  int w = 0;
	  // Check for a gap of unused bit
	  while(i+w < static_cast<int>(memoryMap.size()) && memoryMap[i+w] < 0) ++w;
	  if(!w) // No gap this time
	  {
	    j = memoryMap[i];
	    w = pixelFormat.channelLayout[j].width;
	  }
	  memoryFields.push_back(MemoryField(j, w));
	  i += w;
	}
      }

      // Determine number of bits between strips (the stride)
      bitsPerStrip = bitsPerPixel * pixelsPerStrip;
      if(bufferFormat.wordAlignStrip)
      {
	long r = bitsPerStrip % pixelFormat.bitsPerWord;
	if(r) bitsPerStrip += pixelFormat.bitsPerWord-r;
      }


      // Determine the width and height of the frame of interest
      int fullWidth  = bufferFormat.verticalStrips ? numberOfStrips : pixelsPerStrip;
      int fullHeight = bufferFormat.verticalStrips ? pixelsPerStrip : numberOfStrips;
      interestLeft   = left;
      interestBottom = bottom;
      interestWidth  = width  ? width  : fullWidth  - left;
      interestHeight = height ? height : fullHeight - bottom;


      // Determine the offset of the pricipal bit. The principal bit
      // is the bit with the lowest bit-level index of all the bits
      // that are part of pixels that fall withing the selected
      // sub-section of the underlaying pixel buffer. Bit-level
      // indexes increases with significance of pit position.
      {
	int x = bufferFormat.rightToLeft ? fullWidth  - interestWidth  - left   : left;
	int y = bufferFormat.topToBottom ? fullHeight - interestHeight - bottom : bottom;
	if(bufferFormat.verticalStrips) swap(x, y);
	principalBitOffset = x * bitsPerPixel + y * bitsPerStrip;
      }


      switch(pixelFormat.wordType)
      {
      case PixelFormat::std_char:
	setupCodec<unsigned char, unsigned int>();
	break;
	
      case PixelFormat::std_short:
	setupCodec<unsigned short, unsigned int>();
	break;

      case PixelFormat::std_int:
	setupCodec<unsigned int, unsigned int>();
	break;

      case PixelFormat::std_long:
	setupCodec<unsigned long, unsigned long>();
	break;

      case PixelFormat::std_max_int:
	setupCodec<PixelFormat::MaxInt, PixelFormat::MaxInt>();
	break;

      case PixelFormat::std_float:
	setupCodecDirect<float>();
	break;

      case PixelFormat::std_double:
	setupCodecDirect<double>();
	break;

      case PixelFormat::std_long_double:
	setupCodecDirect<long double>();
	break;

      case PixelFormat::custom_int:
      case PixelFormat::custom_float:
	break; // Never
      }
    }

/*
    ImageData ImageData::getSubImage(int left, int bottom,
				     int width, int height,
				     vector<int> const &channelMap,
				     PixelFormat::ColorScheme colorScheme,
				     bool hasAlphaChannel) const
    {
      PixelFormat pixelFormat = this->pixelFormat;
      if(channelMap.size()) pixelFormat = pixelFormat.getDerivedFormat(channelMap, colorScheme, hasAlphaChannel);

      return ImageData(buffer, pixelsPerStrip, numberOfStrips,
		       pixelFormat, bufferFormat,
		       interestLeft + left, interestBottom + bottom,
		       width  ? width  : interestWidth  - left,
		       height ? height : interestHeight - bottom,
		       endianness);
    }
*/

    size_t ImageData::getMinimumBufferSizeInWords() const
    {
      return (bitsPerStrip*numberOfStrips + pixelFormat.bitsPerWord-1)/pixelFormat.bitsPerWord;
    }

    void *ImageData::allocateBuffer() const
    {
      size_t const n = getMinimumBufferSizeInWords();
      switch(pixelFormat.wordType)
      {
      case PixelFormat::std_char:        return new unsigned char[n];
      case PixelFormat::std_short:       return new unsigned short[n];
      case PixelFormat::std_int:         return new unsigned int[n];
      case PixelFormat::std_long:        return new unsigned long[n];
      case PixelFormat::std_max_int:     return new PixelFormat::MaxInt[n];
      case PixelFormat::std_float:       return new float[n];
      case PixelFormat::std_double:      return new double[n];
      case PixelFormat::std_long_double: return new long double[n];
      case PixelFormat::custom_int:
      case PixelFormat::custom_float: break;
      }
      return 0; // Never
    }
  }
}
