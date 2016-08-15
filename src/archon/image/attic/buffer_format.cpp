/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <archon/image/buffer_format.hpp>


using namespace std;
using namespace archon::image;


namespace
{
  struct BufferFormatImpl: BufferFormat
  {
    UniquePtr<Codec> newCodec() const
    {
    }
  };

  struct CodecImpl
  {
  }

    private:
      PixelFormat::Ref pixelFormat;

      /**
       * The right edge is at a lower address in memory than the
       * left edge. The opposite is the default.
       */
      bool rightToLeft;

      /**
       * The top edge is at a lower address in memory than the
       * bottom edge. The opposite is the default.
       */
      bool topToBottom;

      /**
       * Memory address changes faster when moving horizonally than
       * when moving vertically. The opposite is the default.
       */
      bool verticalStrips;

      /**
       * Align each strip (row or column) on a word boundary. The
       * word size is that of the associated pixel format. This
       * setting has no impact if the width of the pixel format is
       * an integer multiple of the word size.
       *
       * \sa PixelFormat
       */
      bool wordAlignStrip;

      BufferFormat(PixelFormat::RefArg pixelFormat,
                   bool rightToLeft, bool topToBottom,
                   bool verticalStrips, bool wordAlignStrip):
        pixelFormat(pixelFormat),
        rightToLeft(rightToLeft), topToBottom(topToBottom),
        verticalStrips(verticalStrips), wordAlignStrip(wordAlignStrip) {}
  }
