/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_IMAGE_ACCESSOR_HPP
#define ARCHON_IMAGE_ACCESSOR_HPP





#include <iostream>





#include <algorithm>

#include <archon/core/memory.hpp>
#include <archon/image/image.hpp>
//#include <archon/image/pixel.hpp>


namespace Archon
{
  namespace Imaging
  {
/*

Limitations:
  Maximum image width/height: 2^15-1 = 32767
  Maximum number of channels in a color space: 256




Might be a good idea to make a ReadOp be reinitializable, and store an inatnace of it in the ImageReader. Also, it would be possible to detect that two consecutive operations are asking for the same pixel type, so reinitialization can be skipped.

Is it ok that the single pixel read sets pitch and stride to 0?





Juxtaposition:

Two images are juxtaposed vertically or horizontally.

If the color spaces are the same, the resulting color space is that
color space, and the resulting word type is the narrowest one that is
wide enough for both. The resulting word type will be integer only if
the word type of both images are integer.

If the color spaces are different, then if both has 3 or fewer
primaries, then the resulting color space is RGB. Othewise, if one has
more primaries than the other, then the resulting color space is the
one with the highest number of primaries. Otherwise the resulting
color space is the one associated with the first of the two images. In
all these cases the resulting word type is the narrowest floating
point type that is wide enough for both images.






Thoughts:

ImageReader and ImageWriter are not thread safe, but it is thread safe to attempt to create multiple readers and writers against the same image.

Cvt contains information about conversion - is a composition of conversion functions

There are two readers, one assumes a small block, the other subdivides. Both take Cvt as arg

Cvt always assumes source data is in first buffer, and that the application specifies a final buffer for each call.

Cvt always work with memory consecutive pixels, and therefore does not need to know about pitches and strides.

Cvt also works out the maximum number of pixels per tray based on the maximal required pixel size.

Special Cvt's for methods such as 'unsigned long getPixel()' can be prepared, to minimize the overhead on each call.

A grid holds a 'char *', a pitch, and a stride. Nothing else.

Small block:
  If need conv:
    Make grid for first buffer
    Read from accessor passing the grid
    Call Cvt passing callers buffer as final destination. No pitching or triding is needed here
  Else:
    Make grid for callers buffer
    Read from accessor passing the grid

Subdivision (used by high-level getBlock):
  Make grid for first buffer
  Make grid for callers buffer
  For each sub-block:
    Read from accessor passing the first grid
    Call Cvt passing the buffer that .


void unifiedRead(Cvt *cvt, int left, int bottom, int width, int height)
{
  if(!op)
  {
    Grid grid();
  }
}



An ImageWriter derives two buffers from ImageReader, but needs another
one such that blending can occur. The first two are needed to prep the
source. One buffer now holds the prepped source, which leaves us with
two buffers to read and prep the target. The blending itself, only
needs two buffers, since the result is stored back.

Writing an image (with optional blending) into a target image should,
from the point of view of the target image, work similarly to writing
a block of data passed by the application. The main difference is that
each block operation must start with a read from the source image.



An image accessor encapsulates a position and a clipping region.
All operations are restricted to the clipping region.
Filling conceptually fills the entire image, but is restricted to the clipping region
When pasting an image, the lower left corner is first placed at the accessor position, then the operation is clipped.
When the clipping region is set, it is itself clipped to the image boundary.
The 'position' is always measured relative to the lower left corner of the image (not the clipping region). It can be set arbitrarily, and may indeed fall outside the boundary of the image, for example by setting one or both of the coordinates to a negative value.


Filling the clipping region:

fill(unsigned long trgbColor);
fill(Image::ConstRefArg image);
template<typename T> fill(T const *tray, int w, int h, ColorSpace const &);

Clipping region is initially set to the entire image

When filling with single pixel, first convert to word type of accessor, then call accessor write with pitch and stride = 0
When filling with image, if image size is small enough that if can fit in a tray that can be written without subdivision, then put in in such a tray, then call fill with block.
when filling with block simply iterate over clipping region with a WriteOp where blending is disabled


Pasting data with alpha channel into image with no alpha channel, and blending with original image disabled:

Blending with background color must be done.

This can be done in place rather than from one buffer to another.

Such blending can be performed in any word tpye, integer or float.

For an arbitrary source and target color spaces the sequence must be as follows:

Convert background color to RGBA
Convert callers data to RGBA
Blend RGBA -> RGBA
Convert RGBA -> target format

We can only generally blend in RGB space. For example, a linear blending in HSV space would produce a completely different result, and probably an unwanted one.

 */



    struct ImageReader
    {
      ImageReader(Image::ConstRefArg image);

      ImageReader &setPos(int x = 0, int y = 0) { posX = x; posY = y; return *this; }
      ImageReader &setClip(int left = 0, int bottom = 0, int width = -1, int height = -1);

      unsigned long getPixel() { unsigned long p; getPixel(p); return p; }
      ImageReader &getPixel(unsigned long &);

      template<typename T>
      ImageReader &getBlock(T *tray, int w, int h,
                            ColorSpace const *c = 0);

      template<typename T>
      ImageReader &getBlock(T *tray, size_t pitch, size_t stride, int w, int h,
                            ColorSpace const *c = 0);

    private:
      struct Cvt;

      static int const maxPixelsPerBlock = 1024; // = 32 * 32

      void readSafe(TupleGrid, int w, int h, Cvt const &);
      void read(TupleGrid, int w, int h, int x, int y, Cvt const &);
      template<typename T> bool clipTray(BasicTupleGrid<T> &g, int &x, int &y, int &w, int &h);
      void clearTray(TupleGrid, int w, int h, Cvt const &);
      void fetchColor(bool fg);
      char *getBuf(bool w = false); // false -> first buffer, true -> second buffer

      friend struct ImageWriter;

      Image::ConstRef const image;
      Image::AccessorPtr const accessor;
      WordType const wordType; // The word type of each channel as they appear in the tray buffer when calling the accessor
      WordType const bestFloat; // The best floating point type for representing channel values of the accesses image
      ColorSpace const *const colorSpace; // The color space of the accessed image, or 0 if it is RGBA
      ColorSpace const *const rgb; // The RGB color space
      ColorSpace::Type const colorSpaceType; // The type of the color space of the accessed image
      bool const hasAlpha; // True iff the color space of the accessed image has an alpha channel
      int const numberOfChannels; // Number of channels in the color space of the accessed image
      int const wordSize; // Number of bytes per word of the word type of each channel as they appear in the tray buffer when calling the accessor
      int const pixelSize;  // Number of bytes per pixel as they appear in the tray buffer when calling the accessor
      int const maxPossiblePixelSize; // Number of bytes per pixel for the widest possible pixel
      size_t const bufferSize; // Number of bytes per per buffer
      int const imageWidth, imageHeight; // Actual dimmensions of accessed image
      int posX, posY, clipLeft, clipRight, clipBottom, clipTop;
      Core::MemoryBuffer buffers[2], colorBuffer;
    };


    struct ImageWriter: ImageReader
    {
      ImageWriter(Image::RefArg image): ImageReader(image), blend(false) {}

      ImageWriter &setBlend(bool blend) { this->blend = blend; return *this; }

      ImageWriter &putImage(Image::ConstRefArg image);
      ImageWriter &putImage(ImageReader const &reader);

      template<typename T>
      ImageWriter &putBlock(T const *tray, int w, int h, ColorSpace const *c);

      template<typename T>
      ImageWriter &putBlock(T const *tray, size_t pitch, size_t stride,
                            int w, int h, ColorSpace const *c);

      ImageWriter &putPixel(unsigned long p);

      ImageWriter &setPos(int x = 0, int y = 0) { ImageReader::setPos(x,y); return *this; }
      ImageWriter &setClip(int l = 0, int b = 0, int w = -1, int h = -1) { ImageReader::setClip(l,b,w,h); return *this; }
      ImageWriter &getPixel(unsigned long &p) { ImageReader::getPixel(p); return *this; }
      template<typename T>
      ImageWriter &getBlock(T *t, int w, int h,
                            ColorSpace const *c = Core::CntRefNullTag()) { ImageReader::getBlock(t,w,h,c); return *this; }

    private:
      void putImage(Image const *, Image::Accessor const *);
/*
      void write(void const *tray, size_t pitch, size_t stride, int w, int h, Cvt const &cvt);
*/

      bool blend;
    };




    // Implementation:

    // Buffer management
    // -----------------
    //
    // An ImageReader has two buffers that each are large enough to
    // hold one pixel of the widest possible type, and large enough to
    // hold N pixels of the type that the image codec uses externally,
    // where N is the value of
    // <tt>ImageReader::maxPixelsPerBlock</tt>.
    //
    // 1) Read with optional bgcolor blend (type, color, blend, color, type)
    // 2) Write with optional bgcolor blend and optional blend with original contents.
    //
    //
    // If source and target has same color space
    //
    // If source has alpha, and target does not, then a blending step is required, which means that the source must be converted to RGBA in some floating point word type.
    //
    // 
    //

    // Both source and target has alpha, or neither source not target has alpha:
    //
    //   Noop Same color space, same word type
    //   T    Same color space, different word type
    //   C    Diffrent color space, direct conversion exists, same word type
    //   CC   Diffrent color space, no direct conversion exists, same word type
    //   TC, TCC, CT, CCT, TCT, TCCT
    //
    // Source has no alpha, but target has:
    //
    //   Each of the above with a "A" appended, standing for "add alpha"
    //
    // Source has alpha, but target does not:
    //
    //   B, TB, BT, TBT, CB, BC 
    //
    //   3   Noop, A, B          Same color space, same word type
    //   4   T, TA, TB, BT       Same color space, different word type
    //   4   C, CA, CB, BC       Diffrent color space, direct conversion exists, same word type
    //   3   CC, CCA, CBC        Diffrent color space, no direct conversion exists, same word type
    //   4   TC, TCA, TCB, TBC   Diffrent color space, direct conversion exists, different word type, target matches intermediate word type
    //   3   TCC, TCCA, TCBC     Diffrent color space, no direct conversion exists, same word type
    //   4   CT, CTA, CBT, BCT
    //   3   CCT, CCTA, CBCT
    //   4   TCT, TCTA, TCBT, TBCT
    //   3   TCCT, TCCTA, TCBCT
    //
    // The word type of color conversions and blending will in general
    // be a floating point type, which can be decided based on the
    // accessed image (as best float) and is thus independant of the
    // format of the callers data.


    //
    // Color space and word type conversion
    // ------------------------------------
    //
    // The general task is to convert from one arbitrary color space
    // and word type to another. Assuming that the source and target color spaces are not the same, we first ask the source color space whether it can provide a converter that will convert directly to the target color space. If this is not the case, we are force to first convert to RGBA, then from RGBA to the target color space. That is, we need to call zero, one or two color space conversion functions. , or might not, be able to do the conversion using a single in one At our disposal we have two functions for each color
    // space that will convert to, and from, RGBA. Each of these
    // functions exist in a variant for each available word type. Also
    // available is a function for each unique pair of word types,
    // that will convert from the first type to the other. So the task
    // reduces to determining a sequence of these functions that
    // produces the desired result.
    //
    // In the interest of performace, we want to use the fewest number
    // of conversion functions possible. But there is a catch, for
    // example, if we need two color conversions, because the source
    // color space provides no direct conversion to the target color
    // space, and the source and target word types are both 8-bit
    // bytes, then it would be tempting to choose a conversion from
    // the source color space to RGBA using 8-bit bytes, then a
    // conversion from RGBA to the target color space, again using
    // 8-bit bytes. The problem here is two-fold: First we might loose
    // precision because we store the intermediate result in 8-bit
    // bytes. Second, each of the two integer based color space
    // conversions generally involve internal conversion to and from
    // some floating point format. For these reasons, it would be
    // better, in this case, to choose some floating point type for
    // the intermediate results, and surround the color conversions
    // with two type conversion.
    //
    // Another case is when we need only one color space conversion,
    // becasue the source color space does provide a direct conversion
    // to the target color space, but we also need a type conversion,
    // becasue the source and target word types differ. Say, one is
    // 8-bit bytes and the other is some 16-bit word type. Then it
    // would be tempting to just use one type conversion that convert
    // from 8-bit bytes to 16-bit words, and then a color space
    // conversion based on 16-bit words. The problem again, is that
    // any integer based color conversion involves internal conversion
    // to and from some floating point type. So we end up doing 3 type
    // conversions. So, also in this case, it would be better to
    // choose some intermediate floating point format and surround the
    // color space conversion by two type conversion functions.
    //
    // For the sake of simplicity, we might say that it would never be
    // relevant to choose a color space conversion based on an integer
    // format, since it is almost as efficient to do the type
    // conversions externally. However, there is one case where it is
    // relevant, namely when only a single color space conversion is
    // needed, and the source and target word types are the same
    // integer type, so no external type conversion functions are
    // required. In this case we prefer to just use the one color
    // space conversion, and let it do the conversion to and from
    // floating point internally, because it will allow us to pack a
    // larger number of pixels inside a give buffer space, and this
    // generally means better performance, when a large area operation
    // is subdivied into block.
    //
    // 
    // The intermediate (or color conversion) word type is determined
    // as follows:
    //
    // 1) If either the source or the target word type is 'double',
    // then the color conversion word type is 'double'. We choose this
    // because double is the most efficient type for arithmetic.
    //
    // 2) Otherwise, if both the source and the target word types are
    // floating point, then the color conversion word type is the one
    // of least width among the two.
    //
    // 3) Otherwise, if either the source or the target word type is
    // floating point, then the color conversion word type is that
    // floating point type.
    //
    // 4) Otherwise, if the source and target word type is the same
    // integer type, and the source color space provides a direct
    // conversion to the target color space, then the color conversion
    // word type is that integer type. Conversion to and from floating
    // point type happens internally in the color space conversion
    // function.
    //
    // 5) Otherwise, let S be the number of significant bits of the
    // source word type, and let T be the number of significant bits
    // of the target word type, then the color conversion word type is
    // the floating point type of least width amongn all the available
    // floating point types whose mantissa representation has at least
    // min(S,T) bits.
    //
    //
    // Determination of conversion type
    // --------------------------------
    //
    //                               Source word    Target word
    //                               type is same   type is same   Either source
    //                Same    Same   as color       as color       or target
    //   Conversion   color   word   conversion     conversion     color space
    //   type         space   type   word type      word type      is RGBA
    //   --------------------------------------------------------------------------
    //     Noop        yes    yes
    //     T           yes    no
    //     C           no     yes       yes            yes            yes
    //     CC          no     yes       yes            yes            no
    //     CT          no     no        yes            no             yes
    //     CCT         no     no        yes            no             no
    //     TC          no     no        no             yes            yes
    //     TCC         no     no        no             yes            no
    //     TCT         no     no        no             no             yes
    //     TCCT        no     no        no             no             no
    //
    //
    //
    // Conversion slot usage
    // ---------------------
    //
    //                Uses type    Uses type    Uses color   Uses color
    //   Conversion   conversion   conversion   conversion   conversion
    //   type         slot A       slot B       slot A       slot B
    //   -----------------------------------------------------------------
    //     Noop
    //     T            yes
    //     C                                      yes
    //     CC                                     yes          yes
    //     CT                        yes          yes
    //     CCT                       yes          yes          yes
    //     TC           yes                       yes
    //     TCC          yes                       yes          yes
    //     TCT          yes          yes          yes
    //     TCCT         yes          yes          yes          yes
    //
    // In the case of type conversions, slot A is always associated
    // with the source color space, and slot B is always associated
    // with the target color space.
    //
    struct ImageReader::Cvt
    {
      /**
       * Construct an ininitialized converter. One of the \c init
       * methods must be called before any other method is used.
       */
      Cvt() {}

      /**
       * A shortcut for initializing this converter the same way the
       * \c init method with the same arguments does.
       */
      Cvt(ImageReader &r, ColorSpace const *s, WordType t, int precision = 0);

      /**
       * Initialize this converter for conversion from the native
       * transfer format of the specified image reader, to the
       * specified target color space and and word type. The precision
       * can optioanllly be specified to indicate that only a certain
       * number of significant bits per word in the target format are
       * required.
       */
      void init(ImageReader &r, ColorSpace const *s, WordType t, int precision = 0);

      /**
       * Returns true if no conversion is needed, i.e. when both color
       * space and word type is the same between the source and target
       * formats.
       */
      bool isNoop() const { return type == type_Noop; }

      /**
       * Get the number of bytes per pixel in the target pixel
       * format. This is the number of bytes per target word type,
       * times the number of channels in the target color space.
       */
      int getTargetPixelSize() const { return targetPixelSize; }

      /**
       * Conversion from specified source buffer, to specified target
       * buffer.
       *
       * \param source The source buffer. It may optionally be
       * specified as the internal buffer returned by
       * <tt>getInternalSource()</tt>.
       *
       * \param The target buffer. It may optionally be specified as
       * the internal buffer returned by <tt>getInternalTarget()</tt>.
       *
       * \param n The number of pixels to convert.
       *
       * \note If this method is called while \c isNoop returns true,
       * it reduces to a simple copy, unless the source and target
       * buffers are the same. Such usage is not expected to be
       * necessary.
       */
      void cvt(char const *source, char *target, int n) const;

      /**
       * Get the internal buffer that may be used as source of a
       * conversion.
       */
      char *getInternalSource() const { return buffer2; }

      /**
       * Get the internal buffer that may be used as target of a
       * conversion.
       */
      char *getInternalTarget() const;

    private:
      enum Type
      {
        type_Noop,
        type_T,
        type_C,
        type_CC,
        type_CT,
        type_CCT,
        type_TC,
        type_TCC,
        type_TCT,
        type_TCCT
      };

      void init();

      char *const buffer1, *const buffer2;
      int const numberOfSourceChannels, numberOfTargetChannels;

      Type type;
      int targetPixelSize; // Number of bytes per pixel in the target format
    };

    inline ImageReader::Cvt::Cvt(ImageReader &r, ColorSpace const *s, WordType t, int /*precision*/):
      buffer1(r.getBuf(false)), buffer2(r.getBuf(true)),
      numberOfSourceChannels(r.numberOfChannels),
      numberOfTargetChannels(s ? s->getNumberOfChannels() : 4)
    {
      bool sameWordType = r.wordType == t;
      int targetWordSize = sameWordType ? r.wordSize : getBytesPerWord(t);
      targetPixelSize = numberOfTargetChannels * targetWordSize;
      
      if(r.colorSpace != s)
      {
        // Color space conversion
      }
      else if(!sameWordType)
      {
        // Type conversion only
      }
      else type = type_Noop;
    }

      maxPixelSize = r.pixelSize;
      if(r.colorSpace != s)
      {
        initWithColorConv(r.colorSpace, r.wordType, s, t,
                          std::min(r.bestFloat, bestFloat(t, precision, bytesPerTargetWord)));
        noop = false;
      }
        else if(r.wordType != t)
        {
          initWithTypeConvOnly(r.wordType, t);
          noop = false;
        }
        else
        {
          noop = true;
std::cerr << "NOOP-READOP()" << std::endl;
          return;
        }
      
std::cerr << "typeConv1 = " << (typeConv1 ? "yes" : "no") << std::endl;
std::cerr << "typeConv2 = " << (typeConv2 ? "yes" : "no") << std::endl;
std::cerr << "toRgba    = " << (fromRgba  ? "yes" : "no") << std::endl;
std::cerr << "fromRgba  = " << (toRgba    ? "yes" : "no") << std::endl;
std::cerr << "lastOp    = " << lastOp << std::endl;
std::cerr << "maxPixelSize = " << maxPixelSize << std::endl;

        // This is guaranteed to be at leat one, because maxPixelSize <=
        // maxPossiblePixelSize <= bufferSize.
        maxPixelsPerTray = std::min(int(r.bufferSize / maxPixelSize), int(ImageReader::maxPixelsPerBlock));
std::cerr << "maxPixelsPerTray = " << maxPixelsPerTray << std::endl;
    }

    inline void ImageReader::Cvt::cvt(char const *s, char *t, int n) const
    {
      switch(type)
      {
      case type_Noop:
        if(s != t) std::copy(s, s + n*targetPixelSize, t);  break;
      case type_T:
        (*typeConvA)(s, t, n * numberOfSourceChannels);     break;
      case type_C:
        colorConvA->cvt(s, t, n);                           break;
      case type_CC:
        colorConvA->cvt(s, buffer1, n);
        colorConvB->cvt(buffer1, t, n);                     break;
      case type_CT:
        colorConvA->cvt(s, buffer1, n);
        (*typeConvB)(buffer1, t, n*numberOfTargetChannels); break;
      case type_CCT:
        colorConvA->cvt(s, buffer1, n);
        colorConvB->cvt(buffer1, buffer2, n);
        (*typeConvB)(buffer2, t, n*numberOfTargetChannels); break;
      case type_TC:
        (*typeConvA)(s, buffer1, n*numberOfSourceChannels);
        colorConvA->cvt(buffer1, t, n);                     break;
      case type_TCC:
        (*typeConvA)(s, buffer1, n*numberOfSourceChannels);
        colorConvA->cvt(buffer1, buffer2, n);
        colorConvB->cvt(buffer2, t, n);                     break;
      case type_TCT:
        (*typeConvA)(s, buffer1, n*numberOfSourceChannels);
        colorConvA->cvt(buffer1, buffer2, n);
        (*typeConvB)(buffer2, t, n*numberOfTargetChannels); break;
      case type_TCCT:
        (*typeConvA)(s, buffer1, n*numberOfSourceChannels);
        colorConvA->cvt(buffer1, buffer2, n);
        colorConvB->cvt(buffer2, buffer1,  n);
        (*typeConvB)(buffer1, t, n*numberOfTargetChannels); break;
      }
    }

    inline char *ImageReader::Cvt::getInternalTarget() const
    {
      switch(type)
      {
      case type_Noop:
      case type_CC:
      case type_CT:
      case type_TC:
      case type_TCCT: return buffer2;
      case type_T:
      case type_C:
      case type_CCT:
      case type_TCC:
      case type_TCT:  return buffer1;
      }
    }

    inline void ImageReader::Cvt::init()
    {
    }

/*

*
        int trayHeight = sqrtf(maxPixelsPerTray);
        int trayWidth = maxPixelsPerTray / trayHeight;

std::cerr << "trayWidth = " << trayWidth << std::endl;
std::cerr << "trayHeight = " << trayHeight << std::endl;
*
      }

    private:
      bool noop;           // No operation
      int maxPixelSize;
      int maxPixelsPerBlock;

      WordTypeConverter typeConv1, typeConv2;
      ColorSpace::Converter const *toRgba, *fromRgba;
      int lastOp; // Index of last operation [first type conv, toRgba, fromRgba, last type conv]

      // Get the smallest floating point type with enough bits. Adding 4
      // to allow for a bit of numeric instability in color space
      // conversion
      WordType bestFloat(WordType type, int prec, int bytesPerTargetWord)
      {
        return prec ?
          getBestFloatTypeByMantissaBits(prec + 4) : isFloatingPoint(type) ? type :
          getBestFloatTypeByMantissaBits(bytesPerTargetWord *
                                         std::numeric_limits<unsigned char>::digits + 4);
      }

*
      void init()
      {
        if(targetColorSpace == sourceColorSpace)
        {
          if(targetWordType == sourceWordType)
          {
            noop = true;
            return;
          }
          init_t();
        }
        else
        {
          if(stdColorSpace ? targetColorSpaceType == sourceColorSpaceType | )
        }
        noop = false;
      }
*

      void initWithTypeConvOnly(WordType sourceWordType, WordType targetWordType)
      {
std::cerr << "TYPE-CONV-ONLY" << std::endl;
std::cerr << "source pixel type = " << getWordTypeName(sourceWordType) << std::endl;
std::cerr << "target pixel type = " << getWordTypeName(targetWordType) << std::endl;
        typeConv1 = getWordTypeConverter(sourceWordType, targetWordType);
        typeConv2 = 0;
        toRgba = fromRgba = 0;
        lastOp = 0;
      }

      // Should only be called when conversion is actually needed
      void initWithColorConv(ColorSpace const *sourceColorSpace, WordType sourceWordType,
                             ColorSpace const *targetColorSpace, WordType targetWordType, WordType convWordType)
      {
std::cerr << "COLOR-CONV (word type = "<<getWordTypeName(convWordType)<<")" << std::endl;
std::cerr << "source pixel type = " << (sourceColorSpace ? sourceColorSpace->getMnemonic() : "RGBA(NULL)") << " / " << getWordTypeName(sourceWordType) << std::endl;
std::cerr << "target pixel type = " << (targetColorSpace ? targetColorSpace->getMnemonic() : "RGBA(NULL)") << " / " << getWordTypeName(targetWordType) << std::endl;

        int const bytesPerConvWord = getBytesPerWord(convWordType);

        // We consider each possible conversion in reverse order, such
        // that we can properly keep track of the maximum pixel size,
        // given that the target format of the last conversion is not
        // to be considered.
        bool isLastOp = true;

        if(targetWordType != convWordType)
        {
          typeConv2 = getWordTypeConverter(convWordType, targetWordType);
          { isLastOp = false; lastOp = 3; }
        }
        else typeConv2 = 0;

        if(targetColorSpace) // Null indicates RGBA
        {
          fromRgba = &targetColorSpace->rgbaToNative(convWordType);
          if(isLastOp) { isLastOp = false; lastOp = 2; }
          else
          {
            int s = numberOfTargetChannels * bytesPerConvWord;
            if(maxPixelSize < s) maxPixelSize = s;
          }
        }
        else fromRgba = 0;
        
        if(sourceColorSpace) // Null indicates RGBA
        {
          toRgba = &sourceColorSpace->nativeToRgba(convWordType);
          if(isLastOp) { isLastOp = false; lastOp = 1; }
          else
          {
            int s = 4 * bytesPerConvWord;
            if(maxPixelSize < s) maxPixelSize = s;
          }
        }
        else toRgba = 0;

        if(sourceWordType != convWordType)
        {
          typeConv1 = getWordTypeConverter(sourceWordType, convWordType);
          int s = numberOfSourceChannels * bytesPerConvWord;
          if(maxPixelSize < s) maxPixelSize = s;
        }
        else typeConv1 = 0;
      }
    };
*/


    inline ImageReader::ImageReader(Image::ConstRefArg image):
      image(image),
      accessor(const_cast<Image::Accessor *>(image->acquireAccessor(maxPixelsPerBlock).release())),
      wordType(accessor->getWordType()),
      bestFloat(isFloatingPoint(wordType) ? wordType :
                getBestFloatTypeByMantissaBits(image->getChannelWidth() + 4)),
      colorSpace(image->getColorSpace().isRgba() ? 0 : &image->getColorSpace()),
      rgb(ColorSpace::getRGB()),
      colorSpaceType(colorSpace ? colorSpace->getType() : ColorSpace::type_RGB),
      hasAlpha(colorSpace ? colorSpace->hasAlphaChannel() : true),
      numberOfChannels(colorSpace ? colorSpace->getNumberOfChannels() : 4),
      wordSize(getBytesPerWord(wordType)), pixelSize(numberOfChannels * getBytesPerWord(wordType)),
      maxPossiblePixelSize(ColorSpace::maxNumberOfChannels * getMaxBytesPerWord()),
      bufferSize(std::max(maxPixelsPerBlock * size_t(pixelSize), size_t(maxPossiblePixelSize))),
      imageWidth(image->getWidth()), imageHeight(image->getHeight())
    {
      // Reset position and clipping region
      setClip();
      setPos();
    }

    inline ImageReader &ImageReader::setClip(int l, int b, int w, int h)
    {
      clipLeft   = Core::clamp(l, 0, imageWidth);
      clipBottom = Core::clamp(b, 0, imageHeight);
      clipRight  = w < 0 ? imageWidth  : Core::clamp(l + w, 0, imageWidth);
      clipTop    = h < 0 ? imageHeight : Core::clamp(b + h, 0, imageHeight);
      return *this;
    }

    inline ImageReader &ImageReader::getPixel(unsigned long &p)
    {
      // Request RGB if accessed image uses RGB, otherwise request RGBA.
      prepForRead(colorSpace == rgb ? rgb : 0, wordType_UChar, 8);

      unsigned char b[4];
      readSafe(TupleGrid(reinterpret_cast<char *>(b), 0, 0), 1, 1);

      int const n = std::numeric_limits<unsigned char>::digits;
      unsigned long q =
        static_cast<unsigned long>(Core::adjustFracBitWidth<unsigned>(b[0], n, 8)) << 16 |
        static_cast<unsigned long>(Core::adjustFracBitWidth<unsigned>(b[1], n, 8)) <<  8 |
        static_cast<unsigned long>(Core::adjustFracBitWidth<unsigned>(b[2], n, 8));
      if(hasAlpha)
        q |= static_cast<unsigned long>(255u - 
                                        Core::adjustFracBitWidth<unsigned>(b[3], n, 8)) << 24;
      p = q;
      return *this;
    }

    template<typename T>
    inline ImageReader &ImageReader::getBlock(T *tray, int w, int h, ColorSpace const *c)
    {
      size_t pitch = c->getNumberOfChannels();
      return getBlock(tray, pitch, w * pitch, w, h, c);
    }

    template<typename T> 
    inline ImageReader &ImageReader::getBlock(T *tray, size_t pitch, size_t stride, int w, int h,
                                              ColorSpace const *c)
    {
      prepForRead(c.get(), getWordTypeByType<T>());
      readSafe(TupleGrid(reinterpret_cast<char *>(tray),
                         pitch * sizeof(T), stride * sizeof(T)), w, h);
      return *this;
    }

    inline ImageReader::prepForRead(ColorSpace const *c, WordType t)
    {
      if(c == preppedColorSpace && t == preppedWordType) return;
      readCvt.initForRead(*this, c, t);
      preppedColorSpace = c;
      preppedWordType   = t;
    }

    // 'g' must not refer to any of the internal buffers, since those
    // buffers may get clobbered before data is read from 'g'.
    inline void ImageReader::readSafe(TupleGrid g, int w, int h)
    {
      char *p = g.origin;
      int x = posX, y = posY, w2 = w, h2 = h;
      bool r = clipTray(g, x, y, w2, h2);
      // If anything was clipped, fill the tray with the background
      // color (possibly clobbering both internal buffers.)
      if(!r || w2 != w || h2 != h) clearReadersTray(TupleGrid(p, g.pitch, g.stride), w, h);
      if(r) read(g, w2, h2, x, y);
    }

    // 0 = nothing left (args are undefined), 1 = partial clip, 2 = nothing clipped (args are untouched)
    template<typename T>
    inline bool ImageReader::clipTray(BasicTupleGrid<T> &g, int &x, int &y, int &w, int &h)
    {
      // Horizontal
      int d = clipLeft - x;
      if(0 < d)
      {
        g.origin += d * g.pitch;
        x = clipLeft;
        w -= d;
      }
      d = x + w - clipRight;
      if(0 < d) w -= d;

      // Vertical
      d = clipBottom - y;
      if(0 < d)
      {
        g.origin += d * g.stride;
        y = clipBottom;
        h -= d;
      }
      d = y + h - clipTop;
      if(0 < d) h -= d;

      return 0 < w && 0 < h;
    }

    // 'g' must not refer to any of the internal buffers, sunce they
    // may get clobbered before data is read from 'g'.
    inline void ImageReader::clearReadersTray(TupleGrid g, int w, int h)
    {
std::cerr << "CLEAR READERS TRAY" << std::endl;
      char *b = cvt.getInternalSource();
      fetchColor(false, b); // Backgound color into converters source buffer
      cvt(b, g.origin, 1);  // Convert to tray format
      Core::extendTupleGrid(Core::Grid<char *>(g.origin, 1, 1, g.pitch, g.stride), op.targetPixelSize,
                            0, 0, 0, 0, 0, w, 0, h);
    }

    // Fetch either the backgound or the foreground color into the
    // specified buffer. The pixel format will be identical to that of
    // the accessed image. Both internal buffers may get clobbered.
    inline void ImageReader::fetchColor(bool fg, char *buffer)
    {
std::cerr << "FETCH COLOR("<<fg<<")" << std::endl;
      if(!colorBuffer)
      {
        int const n = 2;
        colorBuffer.reset(n * pixelSize);
        float b[n*4] = { 0, 0.66, 0.66, 0,   // Backgound color
                         1, 1, 1, 1 }; // Foreground color
        ReadOp op(*this, 0, wordType_Float);
        for(int i=0; i<n; ++i)
        {
          char const *c = reinterpret_cast<char const *>(b) + i * n*4;
          std::copy(c, c + n*4, getBuf());
          op(1, colorBuffer.get() + i * n*4);
        }
      }
      char *p = colorBuffer.get() + (fg?1:0) * pixelSize;
      std::copy(p, p+pixelSize, buffer);
    }

    inline char *ImageReader::getBuf(bool w)
    {
      Core::MemoryBuffer &b = buffers[w ? 1 : 0];
      if(!b) b.reset(bufferSize);
      return b.get();
    }


/*
    // Let T be the requested word type, then w * h * numberOfChannels
    // * sizeof(T) must not exceed bufferSize.
    inline bool ImageReader::read(bool b, int w, int h, WordType t)
    {
      accessor->read(Grid<char *>(getBuf(b), w, h, pixelSize, w*pixelSize), x, y);
      if(wordType != t)
      {
        std::cerr << "NEED TYPE CONV" << std::endl;
        bool c = !b;
        getWordTypeConverter(wordType, t)->convert(getBuf(b), getBuf(c), w*h);
        b = c;
      }
      return b;
    }

    inline bool ImageReader::read(bool b, int w, int h, WordType t, ColorSpace::Type c)
    {
      accessor->read(Grid<char *>(getBuf(b), w, h, pixelSize, w*pixelSize), x, y);
      if(wordType != t)
      {
        std::cerr << "NEED TYPE CONV" << std::endl;
        bool c = !b;
        getWordTypeConverter(wordType, t)->convert(getBuf(b), getBuf(c), w*h);
        b = c;
      }
      return b;
    }
*/

    // Specified region must fall within image boundaries
    // targetColorSpace == 0  means RGBA
    inline void ImageReader::read(TupleGrid g, int w, int h, int x, int y, ReadOp const &op)
    {
      if(op.noop)
      {
std::cerr << "READ -> NOOP" << std::endl;
        accessor->read(g, w, h, x, y);
        return;
      }

std::cerr << "caller pitch      = " << g.pitch            << std::endl;
std::cerr << "caller pixel size = " << op.targetPixelSize << std::endl;

      bool denseRows = g.pitch == size_t(op.targetPixelSize);
      bool denseTray = denseRows && g.stride == w * g.pitch;

std::cerr << "denseRows = " << (denseRows ? "yes" : "no") << std::endl;
std::cerr << "denseTray = " << (denseTray ? "yes" : "no") << std::endl;

/*
      if(denseTray && )
      {
      }

      WordTypeConverter typeConv1 = 0, typeConv2 = 0;
      ColorSpace::Converter const *toRgba = 0, *fromRgba = 0;

std::cerr << "sourceColorSpace = " << static_cast<void const *>(colorSpace) << std::endl;
std::cerr << "targetColorSpace = " << static_cast<void const *>(targetColorSpace) << std::endl;


      // If color conversion is needed (null stands for RGBA)
      bool ignorePixelSize = true;
      int maxPixelSize = pixelSize;
      if(targetColorSpace != colorSpace)
      {
        int bytesPerTargetWord = getBytesPerWord(targetWordType);
        // Get the smallest floating point type with enough bits. Adding 4
        // to allow for a bit of numeric instability in color space
        // conversion
        WordType const colorConvWordType =
          std::min(bestFloat, isFloatingPoint(targetWordType) ? targetWordType :
                   getBestFloatTypeByMantissaBits(bytesPerTargetWord *
                                                  std::numeric_limits<unsigned char>::digits + 4));

        int bytesPerColorConvWord = getBytesPerWord(colorConvWordType);
        int numberOfTargetChannels = targetColorSpace ? targetColorSpace->getNumberOfChannels() : 4;

        // We consider each possible conversion in reverse order, such
        // that we can properly keep track of the maximum pixel size,
        // given that the target format of the last conversion need
        // not be considered.
        if(targetWordType != colorConvWordType)
        {
          typeConv2 = getWordTypeConverter(colorConvWordType, targetWordType);
          if(ignorePixelSize) ignorePixelSize = false;
          else
          {
            int s = numberOfTargetChannels * bytesPerTargetWord;
            if(maxPixelSize < s) maxPixelSize = s;
          }
        }

        if(targetColorSpace)
        {
          fromRgba = &targetColorSpace->rgbaToNative(colorConvWordType);
          if(ignorePixelSize) ignorePixelSize = false;
          else
          {
            int s = numberOfTargetChannels * bytesPerColorConvWord;
            if(maxPixelSize < s) maxPixelSize = s;
          }
        }
        
        if(colorSpace) // Null indicates RGBA
        {
          toRgba = &colorSpace->nativeToRgba(colorConvWordType);
          if(ignorePixelSize) ignorePixelSize = false;
          else
          {
            int s = 4 * bytesPerColorConvWord;
            if(maxPixelSize < s) maxPixelSize = s;
          }
        }

        if(wordType != colorConvWordType)
        {
          typeConv1 = getWordTypeConverter(wordType, colorConvWordType);
          if(!ignorePixelSize)
          {
            int s = numberOfChannels * bytesPerColorConvWord;
            if(maxPixelSize < s) maxPixelSize = s;
          }
        }
      }
      else if(targetWordType != wordType)
      {
        typeConv1 = getWordTypeConverter(wordType, targetWordType);
        if(!ignorePixelSize)
        {
          int s = numberOfChannels * getBytesPerWord(targetWordType);
          if(maxPixelSize < s) maxPixelSize = s;
        }
      }

      // This is guaranteed to be at leat one, because maxPixelSize <=
      // maxPossiblePixelSize <= bufferSize.
      int maxPixelsPerTray = std::min(int(bufferSize / maxPixelSize), int(maxPixelsPerBlock));

      int trayHeight = sqrtf(maxPixelsPerTray);
      int trayWidth = maxPixelsPerTray / trayHeight;
      
std::cerr << "typeConv1 = " << (typeConv1 ? "yes" : "no") << std::endl;
std::cerr << "typeConv2 = " << (typeConv2 ? "yes" : "no") << std::endl;
std::cerr << "toRgba   = " << (fromRgba ? "yes" : "no") << std::endl;
std::cerr << "fromRgba = " << (toRgba   ? "yes" : "no") << std::endl;

std::cerr << "maxPixelSize = " << maxPixelSize << std::endl;

std::cerr << "trayWidth = " << trayWidth << std::endl;
std::cerr << "trayHeight = " << trayHeight << std::endl;



      Grid<char *> tray;
      bool r = true;
      int y = 0, h = trayHeight, readWidth;
      for(;;)
      {
        int x = 0, w = trayWidth;
        for(;;)
        {
          if(r)
          {
            accessor->read(*readGrid, w, h, left+x, bottom+y);
            if(typeConv1) (*typeConv1)(*readGridsourceTray.get(), intermediateTray.get(), w*h);
            if(toRgba) toRgba->convert(sourceTray.get(), intermediateTray.get(), w*h);
            if(fromRgba) fromRgba->convert(intermediateTray.get(), targetTray.get(), w*h);
            if(readOnce) r = false;
          }

          writeBlock(Grid<double const *>(effectiveTargetTray, w, h,
                                          targetPitch, targetPitch*readWidth),
                     targetLeft+x, targetBottom+y, targetHorizontalRepeat, targetVerticalRepeat);

          x += w;
          int l = sourceWidth - x;
          if(l <= 0) break;
          if(l < w) w = l; // Adjust width for final column
        }

        y += h;
        int l = sourceHeight - y;
        if(l <= 0) break;
        if(l < h) h = l; // Adjust height for final row
      }
*/
    }





/*
    struct ImageWriter::WriteOp
    {
      // p = max significant bits per target word assuming integer, if zero, it is assumed to be the number of bits per the specified target word type
      WriteOp(ImageWriter const &w, ColorSpace const *c, WordType t, int p = 0)
      {
        if(c != w.colorSpace)
        {
        }
        else if(t != w.wordType)
        {
        }
        else
        {
          noop = true;
          return;
        }
      }

      bool noop;
    };
*/



    inline ImageWriter &ImageWriter::putImage(Image::ConstRefArg image)
    {
      Image::AccessorConstPtr accessor(image->acquireAccessor(maxPixelsPerBlock).release());
      putImage(image.get(), accessor.get());
      return *this;
    }

    inline ImageWriter &ImageWriter::putImage(ImageReader const &reader)
    {
      putImage(reader.image.get(), reader.accessor.get());
      return *this;
    }

    template<typename T>
    inline ImageWriter &ImageWriter::putBlock(T const *tray, int w, int h, ColorSpace const *c)
    {
      return putBlock(tray, 0, 0, w, h, c);
    }

    template<typename T>
    inline ImageWriter &ImageWriter::putBlock(T const */*tray*/, size_t /*pitch*/, size_t /*stride*/,
                                              int /*w*/, int /*h*/, ColorSpace const */*c*/)
    {
//      write(tray, pitch*sizeof(T), stride*sizeof(T), w, h, Cvt(c.get(), getWordTypeByType<T>()));
      return *this;
    }

  inline ImageWriter &ImageWriter::putPixel(unsigned long /*p*/)
    {
/*
      int const n = std::numeric_limits<unsigned char>::digits;
      unsigned t = static_cast<unsigned>(p>>24) & 0xFFu;
      unsigned char b[4] =
        {
          Core::adjustFracBitWidth(static_cast<unsigned>(p>>16) & 0xFFu, 8, n),
          Core::adjustFracBitWidth(static_cast<unsigned>(p>>8)  & 0xFFu, 8, n),
          Core::adjustFracBitWidth(static_cast<unsigned>(p)     & 0xFFu, 8, n),
          Core::adjustFracBitWidth(255u - t,                         8, n)
        };

      // Request RGB if accessed image uses RGB, otherwise request RGBA.
//      writeSafeSingle(b, WriteOp(colorSpace == rgb && !t ? rgb : 0, wordType_UChar, 8));
*/
      return *this;
    }

    inline void ImageWriter::putImage(Image const *, Image::Accessor const *)
    {
/*
      ImageReader::Cvt cvt(this, c->isRgba() ? 0 : c, a->getWordType());
      // Establish transfer region as intersection between target image area and (x,y,w,h)
      write(Grid(tray, pitch, stride));
*/
    }
/*
    // Simplified version of writeSafe for the case of only 1 pixel
    inline void ImageWriter::writeSafeSingle(void const *tray, WriteOp const &op)
    {
      if(0 <= x && x < width && 0 <= y && y < height)
      {
        if(op.blend) accessor->read(x, y, 1, 1, GridL(firstBuffer, 0, 0));
        accessor->write(x, y, 1, 1, GridR(op(1, tray), 0, 0));
      }
    }

    // Simplified version of writeSafe for the case where pixels are memory consecutive and standard ordered
    inline void ImageWriter::writeSafeStraight(void const *tray, int w, int h, WriteOp const &op)
    {
      // If no intersect, do nothing
      // If fully within, writeStraigt
      // Else write
    }

    // Checks and handles target regions that escape the image
    // boundaries. Also checks if the pixel layout in the tray is
    // actually straight (or can be straightened), if so, this fact is
    // used to optimize performance.
    inline void ImageWriter::writeSafe(GridR tray, int w, int h, WriteOp const &op)
    {
      int x = posX, y = posY;
      if(clipTray(tray, x, y, w, h))
      {
        if(straightenTray(tray, x, y, w, h)) writeFastStraight(tray.origin, x, y, w, h, op);
        else writeFast(tray, x, y, w, h, op);
      }
    }

    // Simplified version of writeFast for the case where pixels are memory consecutive and standard ordered
    inline void ImageWriter::writeFastStraight(void const *tray, int x, int y, int w, int h, WriteOp const &op)
    {
      // If no conversion is required we don't need intermediate buffers, so we can write the entire block in one operation
      if(op.noop)
      {
        accessor->write(x, y, w, h, GridR(tray, pixelSize, w*size_t(pixelSize)));
        return;
      }
    }

    // Checks whether conversion is required, and if it is, whether tray subdivision is required.
    // Assumes that target region falls within image boundaries, and non-standard pitch and/or stride
    inline void ImageWriter::writeFast(GridR const &tray, int x, int y, int w, int h, WriteOp const &op)
    {
      // If no conversion is required we don't need intermediate buffers, so we can write the entire block in one operation
      if(op.noop)
      {
        accessor->write(x, y, w, h, tray);
        return;
      }

      // Number of pixels in callers block is small enough, we don't need
      // subdivision, so call writeSmall, else subdivide
    }


    // Assumes that target region falls within image boundaries, and that tray subdivision is required
    inline void ImageWriter::writeBig(GridR const &tray, int x, int y, int w, int h, WriteOp const &op)
    {
      // Blend only if source image has alpha
      // Blending should be a completely separate branch since it requires a floating point type, does it also require RGB?

      // If no conversion is required we don't need intermediate buffers, so we can write the entire block in one operation
      if(!cvt)
      {
        if(!stride) stride = pixelSize;
        if(!pitch)  pitch  = w*stride;
        accessor->write(x, y, w, h, GridR(tray, pitch, stride));
        return;
      }

      // If callers block is small enough that we don't need
      // subdivision, and it has standard pitch and stride, we can
      // skip the conversion step that condenses the pixels from the
      // callers buffer, and we can also skip the subdivision. The
      // test for 1x1 is intended to help the optimizer.
      if(w == 1 && h == 1 || !pitch && !stride && w*h <= cvt.maxPixelsPerTray)
      {
        cvt(w*h, tray);
        accessor->write(x, y, w, h, GridR(tray, pitch, stride));
        return;
      }
    }

    // Assumes that target region falls within image boundaries, and that number of pixels in tray is small enough that subdivision is not required
    inline void ImageWriter::writeSmall(GridR tray, int x, int y, int w, int h, WriteOp const &op)
    {
//      if(op.blend) accessor->read(x, y, 1, 1, GridL(firstBuffer, 0, 0));
      // Make a dense copy of callers pixels
      for(int i = 0; i < h; ++i)
      {
        for(int j = 0; j < w; ++j)
        {
          b[]
        }
      }
      accessor->write(x, y, 1, 1, GridR(op(1, tray), 0, 0));
    }

*/

  }
}


/*

New simpler idea (but maybe bad):

void writeBlock(const void *tray, int w, int h, WriteOp const &op)
{
  int x = posX, y = posY;

  // How many full rows can we fit into a single block operation
  int n = op.maxPixelsPerBlock / w;
  if(h <= n)
  {
    // Good, we can handle all the pixels in one operation
    GridR g(op(h*w, tray), pixelSize, w*size_t(pixelSize));
    clipTray(g, x, y, w, h);
    accessor->write(g, w, h, x, y);
  }
  else if(n)
  {
    // Fair, we can handle a certain number of full rows at a time
    GridR g(tray, pixelSize, w*size_t(pixelSize));
    for(i=0; i<h; ++i)
    {
      GridR g(op(n*w, tray), pixelSize, w*size_t(pixelSize));
      y += n;
      tray += n*w * op.pixelSize;
    }
  }


  if(h*size_t(w) <= size_t(op.maxPixelsPerBlock))
  {
    writeSafe(const void *tray, int w, int h, WriteOp const &op)
  }
}
  if(block is wider than max pixels per block) write each row iteratively with writeStrip

  if(numner of pixels less than or equal to max we can handle) go directly to fast block write;

*/


#endif // ARCHON_IMAGE_ACCESSOR_HPP
