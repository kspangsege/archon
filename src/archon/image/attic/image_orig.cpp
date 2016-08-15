/**
 * \file
 *
 * \author Kristian Spangsege
 */

#include <errno.h>
#include <string.h>
#include <setjmp.h>
#include <algorithm>

#include <archon/core/stream.hpp>
#include <archon/thread/thread.hpp>
#include <archon/image/image.hpp>
#include <archon/image/image_png.hpp>
#include <archon/image/image_tiff.hpp>
#include <archon/image/image_pnm.hpp>
#include <archon/image/image_jpeg.hpp>
#include <archon/image/image_gif.hpp>


using namespace std;


namespace archon
{
  namespace image
  {
    namespace
    {
      struct DefaultContext: Image::Context
      {
	size_t getNumberOfFormats() const
	{
	  return formats.size();
	}

	const Image::Format *getFormat(size_t i) const
	{
	  return formats[i];
	}

	void add(const Image::Format *f)
	{
	  if(f) formats.push_back(f);
	}

	DefaultContext()
	{
	  add(getDefaultPngFormat());
	  add(getDefaultTiffFormat());
	  add(getDefaultPnmFormat());
	  add(getDefaultJpegFormat());
	  add(getDefaultGifFormat());
	}

	vector<const Image::Format *> formats;
      };
    }

    const string Image::PNG  = "png";
    const string Image::TIFF = "tiff";
    const string Image::PNM  = "pnm";
    const string Image::JPEG = "jpeg";
    const string Image::GIF  = "gif";

    Image::Image(unsigned width, unsigned height,
		 ComponentSpecifier components,
		 unsigned bitsPerComponent,
		 string comment)
    {
      if(width<1 || height<1)
	ARCHON_THROW1(ArgumentException,
		      "Invalid zero size image");
      if(bitsPerComponent != 1 &&
	 bitsPerComponent != 2 &&
	 bitsPerComponent != 4 &&
	 bitsPerComponent != 8 &&
	 bitsPerComponent != 16)
	ARCHON_THROW1(ArgumentException, "Unsupported component width: " +
		      Text::toString(bitsPerComponent));

      unsigned charsPerPixel = (bitsPerComponent*components+7)/8;
      unsigned charsPerRow   = charsPerPixel*width;

      MemoryBuffer buffer(sizeof(Rep) + charsPerRow*height);
      r.reset(new(buffer.get()) Rep(width, height, bitsPerComponent,
				    charsPerPixel, charsPerRow,
				    (1<<bitsPerComponent)-1, components,
				    comment));
      buffer.out();
    }

    const Image::Context *Image::Context::get()
    {
      static DefaultContext c;
      return &c;
    }

    Image::Rep *Image::Rep::refClone() const
    {
      size_t pixelBufferSize = charsPerRow*height;
      MemoryBuffer buffer(sizeof(Rep) + pixelBufferSize);
      Rep *r = new(buffer.get()) Rep(width, height, bitsPerComponent,
				     charsPerPixel, charsPerRow,
				     maxComponentValue, components,
				     comment);
      const char *source = getPixelBuffer();
      copy(source, source+pixelBufferSize, r->getPixelBuffer());
      buffer.out();
      return r;
    }

    void Image::Rep::refDelete() throw()
    {
      // Explicit call to destructor
      this->~Rep();
      // A MemoryBuffer allocated this memory, so let the
      // MemoryBuffer deallocate it too.
      MemoryBuffer buffer(reinterpret_cast<char *>(this));
    }

    /*
     * Assumes that a 'char' is 8 bit wide
     */
    unsigned Image::Rep::getComponent(char *pixel, unsigned component)
    {
      switch(bitsPerComponent)
      {
      case 8:
	return *(reinterpret_cast<unsigned char *>(pixel)+component);
      case 16:
	{
	  unsigned char *const c = reinterpret_cast<unsigned char *>(pixel)+component*2;
	  return *c + (static_cast<unsigned>(*(c+1))<<8);
	}
      case 1:
	return *reinterpret_cast<unsigned char *>(pixel)>>component & 1;
      case 2:
	return *reinterpret_cast<unsigned char *>(pixel)>>component*2 & 3;
      case 4:
	return component<2 ?
	  component==0 ? *reinterpret_cast<unsigned char *>(pixel) & 15 : *reinterpret_cast<unsigned char *>(pixel)>>4 & 15 :
	  component==2 ? *(reinterpret_cast<unsigned char *>(pixel)+1) & 15 : *(reinterpret_cast<unsigned char *>(pixel)+1)>>4 & 15;
      }
      ARCHON_THROW1(InternalException,
		    "Illegal component bit width");
    }

    /*
     * Assumes that a 'char' is 8 bit wide
     */
    void Image::Rep::setComponent(char *pixel, unsigned component, unsigned v)
    {
      unsigned char mask;
      switch(bitsPerComponent)
      {
      case 8:
	*(reinterpret_cast<unsigned char *>(pixel)+component) = static_cast<unsigned char>(v);
	break;
      case 16:
	{
	  unsigned char *const c = reinterpret_cast<unsigned char *>(pixel)+component*2;
	  *c = static_cast<unsigned char>(v);
	  *(c+1) = static_cast<unsigned char>(v>>8);
	  break;
	}
      case 1:
	mask = static_cast<unsigned char>(1)<<component;
	*reinterpret_cast<unsigned char *>(pixel) = *reinterpret_cast<unsigned char *>(pixel) & ~mask |
	  static_cast<unsigned char>(v)<<component & mask;
	break;
      case 2:
	mask = static_cast<unsigned char>(3)<<component*2;
	*reinterpret_cast<unsigned char *>(pixel) = *reinterpret_cast<unsigned char *>(pixel) & ~mask |
	  static_cast<unsigned char>(v)<<component*2 & mask;
	break;
      case 4:
	if(component<2)
	  *reinterpret_cast<unsigned char *>(pixel) = component==0 ?
	    *reinterpret_cast<unsigned char *>(pixel) & 240 | static_cast<unsigned char>(v)    & 15 :
	    *reinterpret_cast<unsigned char *>(pixel) & 15  | static_cast<unsigned char>(v)<<4 & 240;
	else
	  *(reinterpret_cast<unsigned char *>(pixel)+1) = component==2 ?
	    *(reinterpret_cast<unsigned char *>(pixel)+1) & 240 | static_cast<unsigned char>(v)    & 15 :
	    *(reinterpret_cast<unsigned char *>(pixel)+1) & 15  | static_cast<unsigned char>(v)<<4 & 240;
	break;
      default:
	ARCHON_THROW1(InternalException,
		      "Illegal component bit width");
      }
    }


    void Image::Rep::getPixel(int x, int y, unsigned &l)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_l:
	l = getComponent(p, 0);
	break;
      case components_la:
	l = static_cast<unsigned>(getComponent(p, 0)*toFloat(getComponent(p, 1)));
	break;
      case components_rgb:
	l = static_cast<unsigned>((static_cast<unsigned long>(getComponent(p, 0))+getComponent(p, 1)+getComponent(p, 2))/3);
	break;
      case components_rgba:
	l = static_cast<unsigned>((static_cast<double>(getComponent(p, 0))+getComponent(p, 1)+getComponent(p, 2))/3*toFloat(getComponent(p, 3)));
	break;
      }
    }

    void Image::Rep::setPixel(int x, int y, unsigned l)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_la:
	setComponent(p, 1, maxComponentValue);
      case components_l:
	setComponent(p, 0, l);
	break;
      case components_rgba:
	setComponent(p, 3, maxComponentValue);
      case components_rgb:
	setComponent(p, 0, l);
	setComponent(p, 1, l);
	setComponent(p, 2, l);
	break;
      }
    }

    void Image::Rep::getPixel(int x, int y, unsigned &l, unsigned &a)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_l:
	l = getComponent(p, 0);
	a = maxComponentValue;
	break;
      case components_la:
	l = getComponent(p, 0);
	a = getComponent(p, 1);
	break;
      case components_rgb:
	l = static_cast<unsigned>((static_cast<unsigned long>(getComponent(p, 0))+getComponent(p, 1)+getComponent(p, 2))/3);
	a = maxComponentValue;
	break;
      case components_rgba:
	l = static_cast<unsigned>((static_cast<unsigned long>(getComponent(p, 0))+getComponent(p, 1)+getComponent(p, 2))/3);
	a = getComponent(p, 3);
	break;
      }
    }

    void Image::Rep::setPixel(int x, int y, unsigned l, unsigned a)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_l:
	setComponent(p, 0, static_cast<unsigned>(l*toFloat(a&maxComponentValue)));
	break;
      case components_la:
	setComponent(p, 0, l);
	setComponent(p, 1, a);
	break;
      case components_rgb:
	{
	  const unsigned _l = static_cast<unsigned>(l*toFloat(a&maxComponentValue));
	  setComponent(p, 0, _l);
	  setComponent(p, 1, _l);
	  setComponent(p, 2, _l);
	  break;
	}
      case components_rgba:
	setComponent(p, 0, l);
	setComponent(p, 1, l);
	setComponent(p, 2, l);
	setComponent(p, 3, a);
	break;
      }
    }

    void Image::Rep::getPixel(int x, int y, unsigned &r, unsigned &g, unsigned &b)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_l:
	r = g = b = getComponent(p, 0);
	break;
      case components_la:
	r = g = b = static_cast<unsigned>(getComponent(p, 0)*toFloat(getComponent(p, 1)));
	break;
      case components_rgb:
	r = getComponent(p, 0);
	g = getComponent(p, 1);
	b = getComponent(p, 2);
	break;
      case components_rgba:
	{
	  const double a = toFloat(getComponent(p, 3));
	  r = static_cast<unsigned>(getComponent(p, 0)*a);
	  g = static_cast<unsigned>(getComponent(p, 1)*a);
	  b = static_cast<unsigned>(getComponent(p, 2)*a);
	  break;
	}
      }
    }

    void Image::Rep::setPixel(int x, int y, unsigned r, unsigned g, unsigned b)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_la:
	setComponent(p, 1, maxComponentValue);
      case components_l:
	setComponent(p, 0, static_cast<unsigned>((static_cast<unsigned long>(r)+b+g)/3));
	break;
      case components_rgba:
	setComponent(p, 3, maxComponentValue);
      case components_rgb:
	setComponent(p, 0, r);
	setComponent(p, 1, g);
	setComponent(p, 2, b);
	break;
      }
    }

    void Image::Rep::getPixel(int x, int y, unsigned &r, unsigned &g, unsigned &b, unsigned &a)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_l:
	r = g = b = getComponent(p, 0);
	a = maxComponentValue;
	break;
      case components_la:
	r = g = b = getComponent(p, 0);
	a = getComponent(p, 1);
	break;
      case components_rgb:
	r = getComponent(p, 0);
	g = getComponent(p, 1);
	b = getComponent(p, 2);
	a = maxComponentValue;
	break;
      case components_rgba:
	r = getComponent(p, 0);
	g = getComponent(p, 1);
	b = getComponent(p, 2);
	a = getComponent(p, 3);
	break;
      }
    }

    void Image::Rep::setPixel(int x, int y, unsigned r, unsigned g, unsigned b, unsigned a)
    {
      char *p = getPixelPtr(x, y);
      switch(components)
      {
      case components_l:
	setComponent(p, 0, static_cast<unsigned>((static_cast<double>(r)+b+g)/3*toFloat(a&maxComponentValue)));
	break;
      case components_la:
	setComponent(p, 0, static_cast<unsigned>((static_cast<unsigned long>(r)+b+g)/3));
	setComponent(p, 1, a);
	break;
      case components_rgb:
	{
	  const double _a = toFloat(a&maxComponentValue);
	  setComponent(p, 0, static_cast<unsigned>(r*_a));
	  setComponent(p, 1, static_cast<unsigned>(g*_a));
	  setComponent(p, 2, static_cast<unsigned>(b*_a));
	  break;
	}
      case components_rgba:
	setComponent(p, 0, r);
	setComponent(p, 1, g);
	setComponent(p, 2, b);
	setComponent(p, 3, a);
	break;
      }
    }


    void Image::_load(Ref<Stream::Reader> reader,
		      string sourceName, string formatSpecifier,
		      ProgressTracker *tracker, Logger *logger,
		      const Context *context)
      throw(UnknownFormatException, InvalidFormatException,
	    IOException, UnexpectedException)
    {
      if(!context) context = Context::get();

      Ref<Stream::RewindReader> rewindReader(Stream::RewindReader::get(reader));

      // Primary auto-detection
      if(formatSpecifier.empty())
	for(unsigned i=0; i<context->getNumberOfFormats(); ++i)
      {
	const Format *f = context->getFormat(i);
	bool s = f->checkSignature(rewindReader);
	rewindReader->rewind();
	if(!s) continue;
	formatSpecifier = f->getSpecifier();
	break;
      }
      rewindReader->release();

      // Secondary auto-detection
      if(formatSpecifier.empty())
      {
	string suffix = Text::toLowerCase(File::suffixOf(sourceName));
	if(!suffix.empty())
	  for(unsigned i=0; i<context->getNumberOfFormats(); ++i)
	{
	  const Format *f = context->getFormat(i);
	  if(!f->checkSuffix(suffix)) continue;
	  formatSpecifier = f->getSpecifier();
	  break;
	}
      }

      if(formatSpecifier.empty())
	ARCHON_THROW1(UnknownFormatException,
		      "Image format could not be detected from the initial "
		      "data nor from the file name: \"" + sourceName + "\"");

      for(unsigned i=0; i<context->getNumberOfFormats(); ++i)
      {
	const Format *f = context->getFormat(i);
	if(formatSpecifier != f->getSpecifier()) continue;
	*this = f->load(rewindReader, tracker, logger);
	return;
      }

      ARCHON_THROW1(ArgumentException, "Unrecognized format "
		    "specifier: \"" + formatSpecifier + "\"");
    }

    void Image::_save(Ref<Stream::Writer> writer,
		      string targetName, string formatSpecifier,
		      ProgressTracker *tracker, Logger *logger,
		      const Context *context) const
      throw(UnknownFormatException, IOException, UnexpectedException)
    {
      if(!context) context = Context::get();

      if(formatSpecifier.empty())
      {
	// Determine format by suffix
	string suffix = Text::toLowerCase(File::suffixOf(targetName));
	if(!suffix.empty()) for(unsigned i=0; i<context->getNumberOfFormats(); ++i)
	{
	  const Format *f = context->getFormat(i);
	  if(!f->checkSuffix(suffix)) continue;
	  formatSpecifier = f->getSpecifier();
	  break;
	}
      }

      if(formatSpecifier.empty())
	ARCHON_THROW1(UnknownFormatException, "Image format could not be "
		      "detected from the file name: \"" + targetName + "\"");

      for(unsigned i=0; i<context->getNumberOfFormats(); ++i)
      {
	const Format *f = context->getFormat(i);
	if(formatSpecifier != f->getSpecifier()) continue;
	f->save(*this, writer, tracker, logger);
	return;
      }

      ARCHON_THROW1(ArgumentException, "Unrecognized format "
		    "specifier: \"" + formatSpecifier + "\"");
    }
  }
}
