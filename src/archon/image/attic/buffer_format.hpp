/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_IMAGE_BUFFER_FORMAT_HPP
#define ARCHON_IMAGE_BUFFER_FORMAT_HPP

#include <archon/image/pixel_format.hpp>

namespace Archon
{
  namespace Imaging
  {
    struct BufferFormat: virtual Core::CntRefObjectBase, Core::CntRefDefs<BufferFormat>
    {
      static BufferFormat::Ref newDefaultFormat()
      {
        return newBufferFormat();
      }

      static BufferFormat::Ref newBufferFormat(PixelFormat::RefArg pixelFormat = PixelFormat::newDefaultFormat(),
                                               bool rightToLeft = false, bool topToBottom = false,
                                               bool verticalStrips = false, bool wordAlignStrip = false)
      {
        return Ref(new BufferFormat(pixelFormat, rightToLeft, topToBottom,
                                    verticalStrips, wordAlignStrip));
      }

      virtual Core::UniquePtr<BufferCodec> newCodec() const = 0;
    };
  }
}

#endif // ARCHON_IMAGE_BUFFER_FORMAT_HPP
