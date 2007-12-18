/**
 * \file
 *
 * \author Kristian Spangsege
 */

#ifndef ARCHON_IMAGE_IMAGE_HPP
#define ARCHON_IMAGE_IMAGE_HPP

#include <stdio.h>
#include <string.h>
#include <string>

#include <archon/core/logger.hpp>
#include <archon/core/stream.hpp>
#include <archon/core/text.hpp>
#include <archon/core/muta_ref.hpp>
#include <archon/core/file.hpp>

namespace Archon
{
  namespace ImageOrig
  {
    /**
     * A general purpose in-memory bitmap image (rectangular pixel
     * array) with value semantics as well as copy-on-write semantics
     * to improve performance.
     *
     * The class comes with a comprehensive set of manipulation
     * functions and is able to read and write image files in various
     * well-known image file formats such as PNG, JPEG and GIF.
     *
     * This class offers direct access to the memory buffer holding
     * the pixels. Pixels are stored in row major order starting with
     * the left most pixel of the bottom most row. Pixel coordinates
     * (x,y) should likewise be understood such that (0,0) is at the
     * lower left corner of the image.
     *
     * Each pixel consists of 1 or more chars. Each new pixel always
     * starts on a char boundary (i.e. pixels are char aligned.)
     *
     * There are two parameters of the Image that determine just how
     * many chars are used per pixel: The number of components per
     * pixel and the number of bits per component.
     *
     * A pixel consistst of 1, 2, 3 or 4 components (or channels). The
     * specific interpretation of the components is a function of the
     * number as follows:
     *
     * <pre>
     *
     *   Number of
     *   components   Interpretation (in given order)
     *  -----------------------------------------------
     *   1            L      Luminance
     *   2            LA     Luminance, alpha
     *   3            RGB    Red, green, blue
     *   4            RGBA   Red, green, blue, alpha
     *
     * </pre>
     *
     * Each component (or channel) consists of 1, 2, 4, 8, or 16 bits.
     *
     * Assuming that chars are 8 bit wide we could have up to 8 chars
     * per pixel. Assuming further the the target system is a
     * little-endian system such as the Intel x86 family, we get the
     * following pixel layouts:
     *
     * <pre>
     *                  Chars per               Address offset
     *  Configuration   pixel            3        2        1        0
     * ------------------------------------------------------------------
     *  L(1)              1                                     0000000l
     *  L(2)              1                                     000000ll
     *  L(4)              1                                     0000llll
     *  L(8)              1                                     llllllll
     *  L(16)             2                            LLLLLLLL llllllll
     *
     *  LA(1)             1                                     000000al
     *  LA(2)             1                                     0000aall
     *  LA(4)             1                                     aaaallll
     *  LA(8)             2                            aaaaaaaa llllllll
     *  LA(16)            4          AAAAAAAA aaaaaaaa LLLLLLLL llllllll
     *
     *  RGB(1)            1                                     00000bgr
     *  RGB(2)            1                                     00bbggrr
     *  RGB(4)            2                            0000bbbb ggggrrrr
     *  RGB(8)            3                   bbbbbbbb gggggggg rrrrrrrr
     *  RGB(16)           6      ... GGGGGGGG gggggggg RRRRRRRR rrrrrrrr
     *
     *  RGBA(1)           1                                     0000abgr
     *  RGBA(2)           1                                     aabbggrr
     *  RGBA(4)           2                            aaaabbbb ggggrrrr
     *  RGBA(8)           4          aaaaaaaa bbbbbbbb gggggggg rrrrrrrr
     *  RGBA(16)          8      ... GGGGGGGG gggggggg RRRRRRRR rrrrrrrr
     *
     * </pre>
     *
     * Where capital letters indicate MBS (most significant byte.) On
     * a big-endian system such as the PPC family seen in Macs we
     * would for instance get this instead:
     *
     * <pre>
     *                  Chars per               Address offset
     *  Configuration   pixel            3        2        1        0
     * ------------------------------------------------------------------
     *  LA(16)            4          aaaaaaaa AAAAAAAA llllllll LLLLLLLL
     *
     * </pre>
     *
     * So please note that the exact byte ordering of the pixel buffer
     * used in this class is not entirely fixed. It depends on the
     * endianness of the platform.
     *
     * \todo Revisit all file format I/O for supporting bits-per-char
     * greater than 8.
     *
     * \todo Considder adding a "Pixel operator(x, y)" where Pixel
     * holds a pointer to the pixel byte(s). Then protect the copy
     * constructor and the copy assignmen operator so the application
     * cannot store the returned object longer than the image. On the
     * Pixel class should be a number of conversion operators and
     * assignment operators.
     *
     * \todo Add fill and clear member functions.
     */
    struct Image
    {
      static std::string const PNG;  ///< Specifier for PNG format
      static std::string const TIFF; ///< Specifier for TIFF format
      static std::string const PNM;  ///< Specifier for PNM format
      static std::string const JPEG; ///< Specifier for JPEG format
      static std::string const GIF;  ///< Specifier for GIF format

      /**
       * Returns true unless the image is a null image.
       */
      operator bool() const { return r; }

      enum ComponentSpecifier
      {
	components_l    = 1, ///< Luminance (one channel)
	components_la   = 2, ///< Luminance and alpha (two channels)
	components_rgb  = 3, ///< Red, green and blue (three channels)
	components_rgba = 4  ///< Red, green, blue and alpha (four channels)
      };

      /**
       * Create a special null image. A null image has no pixel buffer
       * nor any attributes and thus, it requires a minimum of
       * storage.
       *
       * \note Most operations are illegal on a null image.
       */
      Image() {}

      /**
       * Create an image with an uninitialized pixel buffer.
       */
      Image(unsigned width, unsigned height,
	    ComponentSpecifier components = components_rgba,
	    unsigned bitsPerComponent = 8,
	    std::string comment = "");

      unsigned getWidth()             const { return r->width;             }
      unsigned getHeight()            const { return r->height;            }
      unsigned getBitsPerComponent()  const { return r->bitsPerComponent;  }
      unsigned getCharsPerPixel()     const { return r->charsPerPixel;     }
      unsigned getCharsPerRow()       const { return r->charsPerRow;       }
      unsigned getMaxComponentValue() const { return r->maxComponentValue; }

      ComponentSpecifier getComponentSpecifier() const { return r->components; }

      /**
       * Fetch the image comment.
       *
       * \return The comment in UTF-8 encoding.
       */
      std::string getComment() const { return r->comment; }

      /**
       * Set a new image comment
       *
       * \param c The comment in UTF-8 encoding.
       */
      void setComment(std::string c)
      {
	r.mutate(); r->comment = c;
      }

      char const *getPixelBuffer() const
      {
	return r->getPixelBuffer();
      }

      char *getPixelBuffer()
      {
	r.leak();
	return r->getPixelBuffer();
      }

      char const *getPixelPtr(int x, int y) const
      {
	return r->getPixelPtr(x, y);
      }

      char *getPixelPtr(int x, int y)
      {
	r.leak();
	return r->getPixelPtr(x, y);
      }

      void getPixel(int x, int y, unsigned &l) const
      {
	r->getPixel(x, y, l);
      }

      void setPixel(int x, int y, unsigned l)
      {
	r.mutate();
	r->setPixel(x, y, l);
      }

      void getPixel(int x, int y, unsigned &l, unsigned &a) const
      {
	r->getPixel(x, y, l, a);
      }

      void setPixel(int x, int y, unsigned l, unsigned a)
      {
	r.mutate();
	r->setPixel(x, y, l, a);
      }

      void getPixel(int x, int y, unsigned &r, unsigned &g, unsigned &b) const
      {
	this->r->getPixel(x, y, r, g, b);
      }

      void setPixel(int x, int y, unsigned r, unsigned g, unsigned b)
      {
	this->r.mutate();
	this->r->setPixel(x, y, r, g, b);
      }

      void getPixel(int x, int y, unsigned &r, unsigned &g,
		    unsigned &b, unsigned &a) const
      {
	this->r->getPixel(x, y, r, g, b, a);
      }

      void setPixel(int x, int y, unsigned r, unsigned g,
		    unsigned b, unsigned a)
      {
	this->r.mutate();
	this->r->setPixel(x, y, r, g, b, a);
      }

      void getPixel(int x, int y, double &l) const
      {
	r->getPixel(x, y, l);
      }

      void setPixel(int x, int y, double l)
      {
	r.mutate();
	r->setPixel(x, y, l);
      }

      void getPixel(int x, int y, double &l, double &a) const
      {
	r->getPixel(x, y, l, a);
      }

      void setPixel(int x, int y, double l, double a)
      {
	r.mutate();
	r->setPixel(x, y, l, a);
      }

      void getPixel(int x, int y, double &r, double &g, double &b) const
      {
	this->r->getPixel(x, y, r, g, b);
      }

      void setPixel(int x, int y, double r, double g, double b)
      {
	this->r.mutate();
	this->r->setPixel(x, y, r, g, b);
      }

      void getPixel(int x, int y, double &r, double &g,
		    double &b, double &a) const
      {
	this->r->getPixel(x, y, r, g, b, a);
      }

      void setPixel(int x, int y, double r, double g, double b, double a)
      {
	this->r.mutate();
	this->r->setPixel(x, y, r, g, b, a);
      }

      /**
       * Efficient swapping that avoids access to the referenced
       * object, in particular, its reference count.
       */
      void swap(Image &i) { std::swap(r, i.r); }

      struct FormatException: std::runtime_error
      {
	FormatException(std::string m): std::runtime_error(m) {}
      };

      /**
       * Image format could not be detected.
       */
      struct UnknownFormatException: formatException
      {
	UnknownFormatException(std::string m): FormatException(m) {}
      };

      /**
       * The image file contents is corrupt or of a different format
       * than the one selected.
       */
      struct InvalidFormatException: FormatException
      {
	InvalidFormatException(std::string m): FormatException(m) {}
      };

      struct ProgressTracker
      {
	virtual void progress(double fraction) throw() = 0;
        virtual ~ProgressTracker() {}
      };

      /**
       * This class represents one single image format. It could be
       * PNG or it could be JPEG or it could be anything else.
       *
       * It is an abstract base class or interface listing the methods
       * required for interaction with the image loader and image
       * saver. Objects of this type are offered for service through
       * the Context object.
       */
      struct Format
      {
	/**
	 * Must return the unique specifier for this image format. The
	 * specifier is the sub-field of the MIME type. That is, the
	 * part of the MIME type after "image/". This will normally
	 * correspond to the file name suffix, but not always.
	 */
	virtual std::string getSpecifier() const = 0;

	/**
	 * Check if the initial characters from the passed stream
	 * identifies the stream contents as being of this format.
	 */
	virtual bool checkSignature(Ref<Stream::Reader>) const = 0;

	/**
	 * Check if the passed suffix is a proper file name suffix for
	 * this format.
	 *
	 * \param suffix Is always converted to lowercase before
	 * passed to this method.
	 */
	virtual bool checkSuffix(std::string suffix) const = 0;

	virtual Image load(Ref<Stream::Reader>, ProgressTracker *, Logger *) const
	  throw(InvalidFormatException, IOException, UnexpectedException) = 0;

	virtual void save(Image, Ref<Stream::Writer>, ProgressTracker *, Logger *) const
	  throw(IOException, UnexpectedException) = 0;

	/**
	 * Ensures proper destruction of derived classes.
	 */
	virtual ~Format() {}

      protected:
	/**
	 * A backdoor allowing loaders to get hold of a non-const
	 * pointer to the pixel buffer without putting the Rep object
	 * into the leaked state which would result in an otherwise
	 * redundant copying of the image data.
	 *
	 * Please do not use this member function for any other
	 * purpose.
	 */
	char *getPixelBufferNoLeak(Image const &i) const
	{
	  return const_cast<char *>(i.getPixelBuffer());
	}
      };

      /**
       * Holds the list of known image formats.
       */
      struct Context
      {
	virtual size_t getNumberOfFormats() const = 0;
	virtual Format const *getFormat(size_t index) const = 0;

	/**
	 * Get the default image context
	 */
	static Context const *get();

	/**
	 * Ensures proper destruction of derived classes.
	 */
	virtual ~Context() {}
      };

      /**
       * Load an image from the specified reader. If the format
       * specifier is empty, an attempt to auto-detect the format will
       * be done. First the initial part of the stream is examined
       * against the various format signatures. If this yields no
       * unique result then the suffix of the source name (if any) is
       * used to try to dertermine the format.
       *
       * \param l Pass 0 to disable logging. Otherwise this logger
       * will be used to report warnings and non-fatal errors during
       * parsing of the stream contents.
       *
       * \param c Pass 0 if you want to use the default context.
       *
       * \throw UnknownFormatException If an empty formatSpecifier is
       * passed and the format could not be detected from the initial
       * image contents nor from the suffix of the source name.
       *
       * \throw InvalidFormatException When a fatal error occurs
       * during parsing of the stream contents.
       */
      Image(Ref<Stream::Reader> r,
	    std::string sourceName, std::string formatSpecifier = "",
	    Logger *l = Logger::get(), Context const *c = 0)
	throw(UnknownFormatException, InvalidFormatException,
	      IOException, UnexpectedException)
      {
	_load(r, sourceName, formatSpecifier, 0, l, c);
      }

      /**
       * Load an image from the specified file. If the format
       * specifier is empty, an attempt to auto-detect the format will
       * be done. First the initial part of the stream is examined
       * against the various format signatures. If this yields no
       * unique result then the suffix of the source name (if any) is
       * used to try to dertermine the format.
       *
       * \param l Pass 0 to disable logging. Otherwise this logger
       * will be used to report warnings and non-fatal errors during
       * parsing of the file contents.
       *
       * \param c Pass 0 if you want to use the default context.
       *
       * \throw UnknownFormatException If an empty formatSpecifier is
       * passed and the format could not be detected from the initial
       * image contents nor from the suffix of the file name.
       *
       * \throw InvalidFormatException When a fatal error occurs
       * during parsing of the file contents.
       */
      explicit Image(std::string filePath, std::string formatSpecifier = "",
		     Logger *l = Logger::get(), Context const *c = 0)
	throw(UnknownFormatException, InvalidFormatException,
	      IOException, UnexpectedException)
      {
	_load(Stream::makeFileReader(filePath),
	      File::nameOf(filePath), formatSpecifier, 0, l, c);
      }

      /**
       * Load an image from the specified reader. The new image
       * contents replaces the old contents.
       *
       * \param r The stream that will provide the image data.
       *
       * \param sourceName A nmemonic which will be used to identify
       * the stream in log and exception messages.
       *
       * \param t Pass an instance of ProgressTracker if you need
       * progress indications. This is needed if you want to display a
       * progress bar or if you need to display partially loaded
       * images.
       *
       * \param l Pass 0 to disable logging. Otherwise this logger
       * will be used to report warnings and non-fatal errors during
       * parsing of the stream contents.
       *
       * \param c Pass 0 if you want to use the default context.
       *
       * \throw UnknownFormatException If an empty formatSpecifier is
       * passed and the format could not be detected from the initial
       * image contents nor from the suffix of the source name.
       *
       * \throw InvalidFormatException When a fatal error occurs
       * during parsing of the stream contents.
       */
      void load(Ref<Stream::Reader> r,
		std::string sourceName, std::string formatSpecifier = "",
		ProgressTracker *t = 0, Logger *l = Logger::get(),
		Context const *c = 0)
	throw(UnknownFormatException, InvalidFormatException,
	      IOException, UnexpectedException)
      {
	_load(r, sourceName, formatSpecifier, t, l, c);
      }

      /**
       * Load in image from the specified file. The new image
       * contents replaces the old contents.
       *
       * \param filePath The path name of a file containg an image in
       * one of the supported formats.
       *
       * \param t Pass an instance of ProgressTracker if you need
       * progress indications. This is needed if you want to display a
       * progress bar or if you need to display partially loaded
       * images.
       *
       * \param l Pass 0 to disable logging. Otherwise this logger
       * will be used to report warnings and non-fatal errors during
       * parsing of the file contents.
       *
       * \param c Pass 0 if you want to use the default context.
       *
       * \throw UnknownFormatException If an empty formatSpecifier is
       * passed and the format could not be detected from the initial
       * image contents nor from the suffix of the file name.
       *
       * \throw InvalidFormatException When a fatal error occurs
       * during parsing of the file contents.
       */
      void load(std::string filePath, std::string formatSpecifier = "",
		ProgressTracker *t = 0, Logger *l = Logger::get(),
		Context const *c = 0)
	throw(UnknownFormatException, InvalidFormatException,
	      IOException, UnexpectedException)
      {
	_load(Stream::makeFileReader(filePath),
	      File::nameOf(filePath), formatSpecifier, t, l, c);
      }

      /**
       * If an empty format specifier is passed the format is
       * determined by the suffix of the target name.
       *
       * \param t Pass an instance of ProgressTracker if you need
       * progress indications. This is needed if eg. you want to
       * display a progress bar.
       *
       * \param l Pass 0 to disable logging. Otherwise this logger
       * will be used to report warnings and non-fatal errors during
       * serialization of the image.
       *
       * \param c Pass 0 if you want to use the default context.
       *
       * \throw UnknownFormatException If an empty formatSpecifier is
       * passed and the format could not be detected from the suffix
       * of the target name.
       */
      void save(Ref<Stream::Writer> w,
		std::string targetName, std::string formatSpecifier = "",
		ProgressTracker *t = 0, Logger *l = Logger::get(),
		Context const *c = 0) const
	throw(UnknownFormatException, IOException, UnexpectedException)
      {
	if(!r) ARCHON_THROW1(StateException, "Null image");
	_save(w, targetName, formatSpecifier, t, l, c);
      }

      /**
       * If an empty format specifier is passed the format is
       * determined by the suffix of the file name.
       *
       * \param t Pass an instance of ProgressTracker if you need
       * progress indications. This is needed if eg. you want to
       * display a progress bar.
       *
       * \param l Pass 0 to disable logging. Otherwise this logger
       * will be used to report warnings and non-fatal errors during
       * serialization of the image.
       *
       * \param c Pass 0 if you want to use the default context.
       *
       * \throw UnknownFormatException If an empty formatSpecifier is
       * passed and the format could not be detected from the suffix
       * of the file name.
       */
      void save(std::string filePath, std::string formatSpecifier = "",
		ProgressTracker *t = 0, Logger *l = Logger::get(),
		Context const *c = 0) const
	throw(UnknownFormatException, IOException, UnexpectedException)
      {
	if(!r) ARCHON_THROW1(StateException, "Null image");
	_save(Stream::makeFileWriter(filePath),
	      File::nameOf(filePath), formatSpecifier, t, l, c);
      }

    private:
      struct Rep: MutaRefObjectBase
      {
	Rep(unsigned width, unsigned height,
	    unsigned bitsPerComponent,
	    unsigned charsPerPixel,
	    unsigned charsPerRow,
	    unsigned maxComponentValue,
	    ComponentSpecifier components,
	    std::string comment):
	  width(width), height(height),
	  bitsPerComponent(bitsPerComponent),
	  charsPerPixel(charsPerPixel),
	  charsPerRow(charsPerRow),
	  maxComponentValue(maxComponentValue),
	  components(components),
	  comment(comment) {}

	unsigned width;
	unsigned height;
	unsigned bitsPerComponent;       // 1, 2, 4, 8, 16
	unsigned charsPerPixel;
	unsigned charsPerRow;
	unsigned maxComponentValue;
	ComponentSpecifier components;
	std::string comment; // UTF-8

	Rep *refClone() const;
	void refDelete() throw();

	char const *getPixelBuffer() const
	{
	  return reinterpret_cast<char const *>(this+1);
	}

	char *getPixelBuffer()
	{
	  return reinterpret_cast<char *>(this+1);
	}

	char *getPixelPtr(int x, int y)
	{
	  return getPixelBuffer() + y*charsPerRow + x*charsPerPixel;
	}

	unsigned getComponent(char *pixel, unsigned component);
	void setComponent(char *pixel, unsigned component, unsigned v);

	void getPixel(int x, int y, unsigned &l);
	void setPixel(int x, int y, unsigned l);
	void getPixel(int x, int y, unsigned &l, unsigned &a);
	void setPixel(int x, int y, unsigned l, unsigned a);
	void getPixel(int x, int y, unsigned &r, unsigned &g, unsigned &b);
	void setPixel(int x, int y, unsigned r, unsigned g, unsigned b);
	void getPixel(int x, int y, unsigned &r, unsigned &g,
		      unsigned &b, unsigned &a);
	void setPixel(int x, int y, unsigned r, unsigned g,
		      unsigned b, unsigned a);

	double toFloat(unsigned v)
	{
	  return double(v)/maxComponentValue;
	}

	unsigned fromFloat(double v)
	{
	  if(v<0) v=0;
	  else if(v>1) v=1;
	  return unsigned(v*maxComponentValue+0.5);
	}

	void getPixel(int x, int y, double &l)
	{
	  unsigned _l;
	  getPixel(x, y, _l);
	  l = toFloat(_l);
	}

	void setPixel(int x, int y, double l)
	{
	  setPixel(x, y, fromFloat(l));
	}

	void getPixel(int x, int y, double &l, double &a)
	{
	  unsigned _l, _a;
	  getPixel(x, y, _l, _a);
	  l = toFloat(_l);
	  a = toFloat(_a);
	}

	void setPixel(int x, int y, double l, double a)
	{
	  setPixel(x, y, fromFloat(l), fromFloat(a));
	}

	void getPixel(int x, int y, double &r, double &g, double &b)
	{
	  unsigned _r, _g, _b;
	  getPixel(x, y, _r, _g, _b);
	  r = toFloat(_r);
	  g = toFloat(_g);
	  b = toFloat(_b);
	}

	void setPixel(int x, int y, double r, double g, double b)
	{
	  setPixel(x, y, fromFloat(r), fromFloat(g), fromFloat(b));
	}

	void getPixel(int x, int y, double &r, double &g,
		      double &b, double &a)
	{
	  unsigned _r, _g, _b, _a;
	  getPixel(x, y, _r, _g, _b, _a);
	  r = toFloat(_r);
	  g = toFloat(_g);
	  b = toFloat(_b);
	  a = toFloat(_a);
	}

	void setPixel(int x, int y, double r, double g, double b, double a)
	{
	  setPixel(x, y, fromFloat(r), fromFloat(g), fromFloat(b), fromFloat(a));
	}
      };

      MutaRef<Rep> r;

      void _load(Ref<Stream::Reader>,
		 std::string sourceName, std::string formatSpecifier,
		 ProgressTracker *, Logger *, Context const *)
	throw(UnknownFormatException, InvalidFormatException,
	      IOException, UnexpectedException);

      void _save(Ref<Stream::Writer>,
		 std::string targetName, std::string formatSpecifier,
		 ProgressTracker *, Logger *, Context const *) const
	throw(UnknownFormatException, IOException, UnexpectedException);
    };
  }
}

#endif // ARCHON_IMAGE_IMAGE_HPP
