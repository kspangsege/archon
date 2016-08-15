/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <stdexcept>
#include <limits>
#include <algorithm>
//#include <string>

#include <archon/core/memory.hpp>
#include <archon/image/file_format.hpp>

/*
#include <archon/core/text.hpp>
#include <archon/thread/condition.hpp>
#include <archon/util/transcode.hpp>
#include <archon/image/integer_pixel_format.hpp>
*/


#include <iostream>



using namespace std;
using namespace archon::Core;
using namespace archon::Imaging;


namespace
{
  /**
   * Assemble two bytes into a 16-bit unsigned integer with the least
   * significant byte first, but require that the value can be stored
   * in an <tt>int</tt>.
   */
  inline int makeWord(char *b)
  {
    unsigned v = static_cast<unsigned>(static_cast<unsigned char>(b[1])) << 8 |
      static_cast<unsigned>(static_cast<unsigned char>(b[0]));
    if(static_cast<unsigned>(numeric_limits<int>::max()) < v)
      throw FileFormat::InvalidFormatException("Numeric field out of "
                                               "range in GIF stream");
    return v;
  }

  inline void read(InputStream::RefArg in, char *buffer, int n)
  {
    if(in->readAll(buffer, n) != static_cast<unsigned>(n))
      throw FileFormat::InvalidFormatException("Premature end of GIF stream");
  }

  /**
   * Since <tt>giflib</tt> is not thread safe, this is a reimplementation of it.
   *
   * \sa http://www.w3.org/Graphics/GIF/spec-gif89a.txt
   * \sa http://sourceforge.net/projects/giflib
   */
  struct FormatGif: FileFormat
  {
    string getName() const
    {
      return "gif";
    }

    bool checkSignature(InputStream::RefArg in) const
    {
      char header[6];
      return in->readAll(header, 6)==6 &&
        (equal(header, header+6, "GIF87a") ||
         equal(header, header+6, "GIF89a"));
    }

    bool checkSuffix(string s) const
    {
      return s == "gif";
    }

    BufferedImage::Ref load(InputStream::RefArg in, Logger *logger,
                            ProgressTracker *tracker) const
    {
      if(!checkSignature(in))
        throw InvalidFormatException("Not a GIF header");

      // Read the Logical Screen Descriptor and the Global Color Table
      // and create the "canvas" image.
      int numberOfGlobalColors;
      Array<char> globalColorMap;
      BufferedImage::Ref canvas;
      {
        char buffer[7];
        read(in, buffer, sizeof(buffer));

        int width  = makeWord(buffer+0);
        int height = makeWord(buffer+2);
        if(width < 1 || height < 1)
          throw InvalidFormatException("Bad screen size in GIF stream");
        cerr << "width  = " << width  << endl;
        cerr << "height = " << height << endl;

        cerr << "colorResolution = " << ((buffer[4] & 0x70) >> 4) + 1 << endl;

        numberOfGlobalColors = 1 << ((buffer[4] & 0x07) + 1);
        cerr << "numberOfGlobalColors = " << numberOfGlobalColors << endl;

        // Read the Global Color Table if present
        if(buffer[4] & 0x80)
        {
          cerr << "Has global color table" << endl;
          globalColorMap.reset(3*numberOfGlobalColors);
          read(in, globalColorMap.get(), 3*numberOfGlobalColors);
        }

        canvas = BufferedImage::newImage(width, height);
      }

      // Loop over remaining blocks in stream
      for(;;)
      {
        char blockType;
              try{
        read(in, &blockType, 1);
              }catch(...){cerr<<"CATCH-1"<<endl;throw;}
        switch(blockType)
        {
        case '\x2C': // Image descriptor
          {
            char buffer[9];
            read(in, buffer, sizeof(buffer));

            int left   = makeWord(buffer+0);
            int top    = makeWord(buffer+2);
            int width  = makeWord(buffer+4);
            int height = makeWord(buffer+6);
            cerr << "left   = " << left   << endl;
            cerr << "top    = " << top    << endl;
            cerr << "width  = " << width  << endl;
            cerr << "height = " << height << endl;

            bool interlace = buffer[8] & 0x80;
            cerr << "interlace = " << interlace << endl;

            int numberOfColors = 1 << ((buffer[8] & 0x07) + 1);
            cerr << "numberOfColors = " << numberOfColors << endl;

            // Read the Local Color Table if present
            Array<char> colorMap;
            if(buffer[8] & 0x80)
            {
              cerr << "Has local color table" << endl;
              colorMap.reset(3*numberOfColors);
              try{
              read(in, globalColorMap.get(), 3*numberOfColors);
              }catch(...){cerr<<"CATCH-2"<<endl;throw;}
            }
          }
        }
      }
      return canvas;
    }


    void save(ImageSource::ConstRefArg, OutputStream::RefArg,
              Logger *, ProgressTracker *) const
    {
      throw runtime_error("Not yet implemented");
    }
  };
}


namespace archon
{
  namespace Imaging
  {
    FileFormat::ConstRef getDefaultGifFileFormat()
    {
      static FileFormat::ConstRef f(new FormatGif());
      return f;
    }
  }
}
