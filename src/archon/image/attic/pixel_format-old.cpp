/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <map>

#include <archon/core/text.hpp>
#include <archon/util/range_map.hpp>
#include <archon/image/pixel_format.hpp>


using namespace std;
using namespace archon::core;
using namespace archon::Util;
using namespace archon::Imaging;


namespace
{
  struct WordTypeDescriptor
  {
    PixelFormat::WordType type;
    string tag;
    int width; ///< In bits
    bool isFloat;

    WordTypeDescriptor() {}

    WordTypeDescriptor(PixelFormat::WordType type, string tag, int width,
                       bool isFloat):
      type(type), tag(tag), width(width), isFloat(isFloat) {}
  };

  struct WordTypeRegistry
  {
    map<PixelFormat::WordType, WordTypeDescriptor> typeMap;
    map<string, WordTypeDescriptor> tagMap;
    map<int,    WordTypeDescriptor> intWidthMap;   ///< Holds only integer word types
    map<int,    WordTypeDescriptor> floatWidthMap; ///< Holds only floating point word types

    template<typename T> void addDescriptor(PixelFormat::WordType type, string tag)
    {
      int width = sizeof(T) * numeric_limits<unsigned char>::digits;
      bool isFloat = !numeric_limits<T>::is_integer;
      WordTypeDescriptor d(type, tag, width, isFloat);
      typeMap[type] = tagMap[tag] = d;
      if(isFloat) floatWidthMap[width] = d;
      else intWidthMap[width] = d;
    }

    WordTypeRegistry()
    {
      // Add word type in order of decreasing width to get the
      // logically shortest word type when querying by bit width.
      addDescriptor<long double>         (PixelFormat::std_long_double, "long_double");
      addDescriptor<double>              (PixelFormat::std_double,      "double");
      addDescriptor<float>               (PixelFormat::std_float,       "float");
      addDescriptor<PixelFormat::MaxInt> (PixelFormat::std_max_int,     "max_int");
      addDescriptor<unsigned long>       (PixelFormat::std_long,        "long");
      addDescriptor<unsigned int>        (PixelFormat::std_int,         "int");
      addDescriptor<unsigned short>      (PixelFormat::std_short,       "short");
      addDescriptor<unsigned char>       (PixelFormat::std_char,        "char");
    }
  };

  WordTypeRegistry const &getWordTypeRegistry()
  {
    static WordTypeRegistry registry;
    return registry;
  }

  WordTypeDescriptor const &getWordTypeDescriptor(PixelFormat::WordType type)
  {
    return getWordTypeRegistry().typeMap.find(type)->second;
  }

  struct OverlapDetector
  {
    /**
     * Returns true if ther was an overlap with a previously added range
     */
    bool add(int offset, int width)
    {
      Oper o;
      map.update(offset, offset+width, o);
      return o.flag;
    }

  private:
    RangeMap<int, bool> map;

    struct Oper
    {
      bool operator()(bool f)
      {
        if(f) flag = true;
        return false;
      }

      bool flag;

      Oper(): flag(false) {}
    };
  };
}


namespace archon
{
  namespace Imaging
  {
    PixelFormat::Ref PixelFormat::newDefaultFormat()
    {
      return newRgbFormat(bitsPerChar, bitsPerChar, bitsPerChar, bitsPerChar);
    }

    PixelFormat::Ref PixelFormat::newLuminanceFormat(int luminanceWidth,
                                                     int alphaWidth,
                                                     bool reverseChannelOrder,
                                                     int bitsPerWord,
                                                     bool floatingPointWords,
                                                     bool mostSignificantBitsFirst,
                                                     int bitsPerPixel)
    {
      vector<int> channelWidths;
      channelWidths.push_back(luminanceWidth);
      if(alphaWidth) channelWidths.push_back(alphaWidth);
      return PixelFormat::makeFormat(channelWidths, CntRefNullTag(),
                                     reverseChannelOrder,
                                     bitsPerWord, floatingPointWords,
                                     mostSignificantBitsFirst, bitsPerPixel);
    }

    PixelFormat::Ref PixelFormat::newRgbFormat(int redWidth,
                                               int greenWidth,
                                               int blueWidth,
                                               int alphaWidth,
                                               bool reverseChannelOrder,
                                               int bitsPerWord,
                                               bool floatingPointWords,
                                               bool mostSignificantBitsFirst,
                                               int bitsPerPixel)
    {
      vector<int> channelWidths;
      channelWidths.push_back(redWidth);
      channelWidths.push_back(greenWidth);
      channelWidths.push_back(blueWidth);
      if(alphaWidth) channelWidths.push_back(alphaWidth);
      return PixelFormat::makeFormat(channelWidths, CntRefNullTag(),
                                     reverseChannelOrder,
                                     bitsPerWord, floatingPointWords,
                                     mostSignificantBitsFirst, bitsPerPixel);
    }

    PixelFormat::Ref PixelFormat::newHsvFormat(int hueWidth,
                                               int saturationWidth,
                                               int valueWidth,
                                               int alphaWidth,
                                               bool reverseChannelOrder,
                                               int bitsPerWord,
                                               bool floatingPointWords,
                                               bool mostSignificantBitsFirst,
                                               int bitsPerPixel)
    {
      vector<int> channelWidths;
      channelWidths.push_back(hueWidth);
      channelWidths.push_back(saturationWidth);
      channelWidths.push_back(valueWidth);
      if(alphaWidth) channelWidths.push_back(alphaWidth);
      return PixelFormat::makeFormat(channelWidths, alphaWidth ?
                                     ColorSpace::getHsva() :
                                     ColorSpace::getHsv(),
                                     reverseChannelOrder,
                                     bitsPerWord, floatingPointWords,
                                     mostSignificantBitsFirst, bitsPerPixel);
    }

    PixelFormat::Ref PixelFormat::makeFormat(ColorSpace::ConstRefArg colorSpace,
                                             int bitsPerChannel,
                                             bool reverseChannelOrder,
                                             int bitsPerWord,
                                             bool floatingPointWords,
                                             bool mostSignificantBitsFirst,
                                             int bitsPerPixel)
    {
      vector<int> channelWidths;
      for(int i=0; i<colorSpace->getNumberOfChannels(); ++i) channelWidths.push_back(bitsPerChannel);
      return PixelFormat::makeFormat(channelWidths,
                                     colorSpace,
                                     reverseChannelOrder,
                                     bitsPerWord,
                                     floatingPointWords,
                                     mostSignificantBitsFirst,
                                     bitsPerPixel);
    }

    PixelFormat::Ref PixelFormat::makeFormat(vector<int> const &channelWidths,
                                             ColorSpace::ConstRefArg colorSpace,
                                             bool reverseChannelOrder,
                                             int bitsPerWord,
                                             bool floatingPointWords,
                                             bool mostSignificantBitsFirst,
                                             int bitsPerPixel)
    {
      vector<Channel> channelLayout;
      int const n = channelWidths.size();
      for(int i=0; i<n; ++i)
	channelLayout.push_back(Channel(0, channelWidths[i]));
      int offset = 0;
      for(int i=0; i<n; ++i)
      {
	Channel &c = channelLayout[reverseChannelOrder ? n-i-1 : i];
	c.offset = offset;
	offset += c.width;
      }
      return PixelFormat::makeFormat(channelLayout,
                                     colorSpace,
                                     bitsPerWord,
                                     floatingPointWords,
                                     mostSignificantBitsFirst,
                                     bitsPerPixel);
    }

    PixelFormat::Ref PixelFormat::makeFormat(vector<Channel> const &channelLayout,
                                             ColorSpace::ConstRefArg colorSpace,
                                             int bitsPerWord,
                                             bool floatingPointWords,
                                             bool mostSignificantBitsFirst,
                                             int bitsPerPixel)
    {
      return Ref(new PixelFormat(channelLayout, colorSpace,
                                 bitsPerWord, floatingPointWords,
                                 mostSignificantBitsFirst, bitsPerPixel));
    }

    PixelFormat::PixelFormat(vector<Channel> const &channelLayout,
			     ColorSpace::ConstRefArg colorSpace,
                             int bitsPerWord, bool floatingPointWords,
                             bool mostSignificantBitsFirst, int bitsPerPixel)
    {
      // Examine channel layout
      this->channelLayout = channelLayout;
      OverlapDetector det;
      int layoutWidth = 0;
      for(vector<Channel>::const_iterator i=channelLayout.begin(); i!=channelLayout.end(); ++i)
      {
        if(i->offset < 0) throw InvalidFormatException("Negative channel offset");
        if(i->width < 0) throw InvalidFormatException("Negative channel width");
        if(det.add(i->offset, i->width)) throw InvalidFormatException("Overlapping channels");
        int w = i->offset + i->width;
        if(layoutWidth < w) layoutWidth = w;
      }


      // Determine color space based on number of channels
      if(colorSpace)
      {
        this->colorSpace = colorSpace;
        if(colorSpace->getNumberOfChannels() != static_cast<int>(channelLayout.size()))
          throw InvalidFormatException("Number of channels in layout does not match color space");
      }
      else
      {
	switch(channelLayout.size())
	{
	case 1: this->colorSpace = ColorSpace::getLuminance();      break;
	case 2: this->colorSpace = ColorSpace::getLuminanceAlpha(); break;
	case 3: this->colorSpace = ColorSpace::getRgb();            break;
	case 4: this->colorSpace = ColorSpace::getRgba();           break;
        default: throw InvalidFormatException("An explicit color space is needed when the number of channels is greater than 4");
        }
      }


      // Determine word type based on 'bitsPerWord' and 'floatingPointWords'
      WordTypeDescriptor wordTypeDescriptor;
      if(bitsPerWord)
      {
        if(floatingPointWords)
        {
          map<int, WordTypeDescriptor>::const_iterator i =
            getWordTypeRegistry().floatWidthMap.find(bitsPerWord);
          if(i == getWordTypeRegistry().floatWidthMap.end())
            throw InvalidFormatException("This platform does not "
                                         "support "+Text::print(bitsPerWord)+" bits per "
                                         "floating point word");
          wordTypeDescriptor = i->second;
        }
        else
        {
          map<int, WordTypeDescriptor>::const_iterator i =
            getWordTypeRegistry().intWidthMap.find(bitsPerWord);
          if(i == getWordTypeRegistry().intWidthMap.end())
            throw InvalidFormatException("This platform does not "
                                         "support "+Text::print(bitsPerWord)+" bits per "
                                         "integer word");
          wordTypeDescriptor = i->second;
        }
      }
      else wordTypeDescriptor = getWordTypeDescriptor(floatingPointWords ? std_float : std_char);
      wordType = wordTypeDescriptor.type;
      /// \todo FIXME: We might need to verify that floating point types are only used with direct formats.


      this->mostSignificantBitsFirst = mostSignificantBitsFirst;


      // Determine number of bits per pixel based on channel layout
      if(bitsPerPixel)
      {
        if(bitsPerPixel < layoutWidth) throw InvalidFormatException("Channel layout escapes pixel boundary");
      }
      else
      {
        bitsPerPixel = (layoutWidth + wordTypeDescriptor.width - 1) / wordTypeDescriptor.width;
      }
      this->bitsPerPixel = bitsPerPixel;
    }

/*
    string PixelFormat::toString() const
    {
      PixelFormat expanded = getExpandedFormat();
      PixelFormat reduced = expanded.getReducedFormat();
      string s = expanded.formatType == packed ? "packed" :
	expanded.formatType == tight ? "tight" : "direct";

      s += "_";
      s += reduced.wordType == custom_int ? "int" +
	Text::print(expanded.bitsPerWord) :
	reduced.wordType == custom_float ? "float" +
	Text::print(expanded.bitsPerWord) :
	getWordTypeDescriptor(reduced.wordType).tag;

      s += "_";
      if(reduced.channelLayout.empty() && reduced.colorSpace == custom)
      {
	// For straight layouts of custom channels
	s += Text::print(reduced.pixelSize);
	if(expanded.hasAlphaChannel) s += "a";
      }
      else
      {
	vector<int> channelMap(expanded.formatType == packed ?
			       expanded.pixelSize*expanded.bitsPerWord :
			       expanded.pixelSize, -1);
	for(int i=0; i<static_cast<int>(expanded.channelLayout.size()); ++i)
	  channelMap[expanded.channelLayout[i].offset] = i;

	vector<string> colorMap;
	switch(expanded.colorSpace)
	{
	case implied: // Never
	  break;

	case custom:
	  {
	    int n = expanded.channelLayout.size();
	    if(expanded.hasAlphaChannel) --n;
	    for(int i=0; i<n; ++i) colorMap.push_back(Text::print(i+1)+"c");
	  }
	  break;

	case luminance:
	  colorMap.push_back("l");
	  break;

	case rgb:
	  colorMap.push_back("r");
	  colorMap.push_back("g");
	  colorMap.push_back("b");
	  break;

	case hsv:
	  colorMap.push_back("h");
	  colorMap.push_back("s");
	  colorMap.push_back("v");
	  break;
	}
	colorMap.push_back("a");


	// Can we use the condensed channel layout
	bool condensedLayout = expanded.colorSpace != custom;
	if(condensedLayout)
	{
	  // Detect precense of unused fields
	  if(expanded.formatType == direct)
	  {
	    for(int i=0; i<static_cast<int>(channelMap.size()); ++i)
	      if(channelMap[i] < 0)
	    {
	      condensedLayout = false;
	      break;
	    }
	  }
	  else // Packed and tight formats
	  {
	    int i=0;
	    while(i < static_cast<int>(channelMap.size()))
	    {
	      int gapSize = 0;
	      while(i+gapSize < static_cast<int>(channelMap.size()) &&
		    channelMap[i+gapSize] < 0) ++gapSize;
	      if(gapSize)
	      {
		// Do not output a small explicit final gaps for
		// packed formats since pixels word-align anyway
		if(i == static_cast<int>(channelMap.size()) &&
		   gapSize < expanded.bitsPerWord &&
		   expanded.formatType == packed) break;

		condensedLayout = false;
		break;
	      }

	      i += expanded.channelLayout[channelMap[i]].width;;
	    }
	  }
	}


	// Generate channel layout
	if(expanded.formatType == direct)
	{
	  for(int i=0; i<static_cast<int>(channelMap.size()); ++i)
	  {
	    if(i && !condensedLayout) s += "_";
	    int j = channelMap[i];
	    s += j < 0 ? "z" : colorMap[j];
	    if(reduced.channelLayout.size())
	    {
	      int w = reduced.channelLayout[j].width;
	      if(w) s+= Text::print(w);
	    }
	  }
	}
	else // Packed and tight formats
	{
	  int i=0;
	  while(i < static_cast<int>(channelMap.size()))
	  {
	    // Is there a gap of unused bit here
	    int w = 0;
	    while(i+w < static_cast<int>(channelMap.size()) &&
		  channelMap[i+w] < 0) ++w;
	    if(w)
	    {
	      // Do not output a small explicit final gaps for packed
	      // formats since pixels word-align anyway
	      if(i == static_cast<int>(channelMap.size()) &&
		 w < expanded.bitsPerWord &&
		 expanded.formatType == packed) break;

	      if(i && !condensedLayout) s += "_";
	      s += Text::print(w);
	    }
	    else // No gap this time
	    {
	      if(i && !condensedLayout) s += "_";
	      int j = channelMap[i];
	      w = expanded.channelLayout[j].width;
	      s += colorMap[j] + Text::print(w);
	    }
	    i += w;
	  }
	}
      }

      if(mostSignificantBitsFirst) s += "_msb";
      return s;
    }
*/
  }
}
