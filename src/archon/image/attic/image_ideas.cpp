


/*


Let the pixel codec be the accessor of the buffered image. Then there is no longer a need for the special release method of the accessor.


Circumvention of write aliasing:

Enforce that there can be at most one accessor with write access per image. If a second is attempted, an exception is thrown. Such an exception will then occur as soon as an accessor with write access is acquired for the image that contains aliases.

Allow multiple readers.






Degign principals:

An image object represents a rectangular pixel array of a specific size. The notion of a pixel is in this case completely abstract, and the representation is arbitrary. Even the color space is arbitrary.

A buffered image shall be provided as a specialization of the abstract image. It must provide for a very flexible specification of the buffer format, i.e. how pixels are encoded in the buffer. This shall provide for interoperability and compatibility with many other image libraries and API's.

The image API must provide for a way to copy one image into another. This shall serve as a primary means of converting image data from one buffer format to another. This is expected to be the highest level function provided by the API. In general, all public image functions must be safe, that is, without any risk of memory corruption due to arguments being out of range. In general this means that target regions must be clipped to the image area.

The space required by a a buffered image object must not exceed significantly that which is required by the actual pixel buffer. A little axtra space is required to define the size and format of the buffer. This allows the buffered image to be used as an efficient way of storing image data.

Accessing image data must be reasonably fast, for example it shall be fast enough that it is reasonable to use for texture lookups in a raytracer. Because of the high level of abstraction and generallity, this is hard to achieve. The general solution is to base everything around block operations (or BLIT's). Thus, when "pasting" one image into another, the general process starts by identifying the most appropriate and efficient BLIT function and then use it iteratively to transfer sub-blocks. The BLIT function is in general a combination of a numer of concrete functions. A number of buffers will be required too.

The establishment of the BLIT function and the allocation of buffers represent a significant amount of work and space. Storing the information in the image is not an option. Producing it for every image access is bad when we want to do a number of small operations such as sampling a texture for a raytracer. The solution is to provide an accessor object that can be requested from the image. The accessor will then store all the buffers and the composition of the BLIT function.




Vocabulary:

Image object
  An instance of the archon::Image::Image class.

Image operator:
  A reinterpretation of an image. That is, an image
  object that presents a "live" view of another image, but in a
  different way. For example, a mirrored view. Some operators will be
  "read only", others will allow both read and write operations. Some
  operators will present only a subsection of the original image, and
  other will combine all of, or sections of multiple original images.




There shall be a single image method responsible for updating a rectangular region of the image using an arbitrary manipulator object. In general it does this by iterating over smaller sub-blocks. First it must aquire an encoder object, and allocate the necessary buffers, then for each sub-block it must first call the manipulator to fill in the block, then optionally convert the data to the word type and color space required by the encoder object, and then finally calling the encoder.

The updator function object takes the coordinates and the block size as arguments.

Pasting one image into another can then be done by preparing an updator function object that simply reads each block from the source image.

void Image::manipRegion(Manipulator &manip, int width, int height, int left, int bottom, int horizontalRepeat, int verticalRepeat)

Things that might need to be configurable:

1) Is there a need to read original data, or is it a "clean" overwrite. Alpha blending would need to read original data.

The concept of repetition compound shall cease to be general, and instead be a feature that is offered specially by specific image view operators. Maybe not - this change will be postponed.






PROBLEM: How to offer the expected repetition options for interpolating sampling function for texture lookup? We don't want to do this by wrapping with an image operator since that would incur too must overhead, or would it? It would if each access requires allocation of a decoder, but this is maybe not necessary (see suggested solution for next problem)

PROBLEM: How to provide a fast interpolating sampling function for texture lookup. Problem right now is that such small reads will take disproportionally long time, which is bad for e.g. a raytracer.

Maybe note - idea:

Prepare the texture in a float buffer.
Work directly on the image object (no wrapping operators)
Hold on to the Decoder and the DecodeHelper objects (maybe by providing a high-level Accessor object that the application can construct and use to access the image)





What happens when reading from a region that lies wholly or partly outside the image borders?




What happens when writing to a region that lies wholly of partly outside the image borders?

Easy, we just ignore anything from the tray buffer that falls outside the image.


An image operator shall be provided that produces an M x N repetition of the original image. Reading is easy, writing needs consideration due to aliasing.

PROBLEM: In the current implementation an omptimization is in effect when copying a repetition of an image into another image, here only one module is read then being repeatedly writtin. This provides for efficient filling with a pattern. With the change suggested above, this optimization will be hard to preserve.



Use a fixed maximum block size (32x32*4*4) (4 channels) (4 bytes per channel).

Use fixed buffer sizes of 8192 chars (8KB) then determine the actual maximum block size as follows


Max block size is determined as follows

pixel size
less than
or equal to

 4 bytes       32 x 32


In general the aliasing problem when writing must be solved by the operator that introduces the posibility of aliasing.

Converting writer must have two buffers since it is not allowed to overwrite the incoming buffer.

Image::write(data, wordType, colorSpace):
  WriteHelper writer;
  writer.write(this, data, wordType, colorSpace);

WriteHelper::write(image, data, wordType, colorSpace):
  Image::Writer writer = image->aquireWriter();
  For each sub-block:
    writer->write(sub-block);




How to achieve good efficiency for small operations on simple buffered images?



Let's solve the problem of pasting a block with alpha blending:

The block is represented as an abstract data source

Acquire a writer from the image.
If no blending and no conversion is needed, just call the writer with the full incoming block.
We need to divide the block into sub-blocks
Allocate buffers
For each sub-block:
  read sub-block from image
  fetch sub-block from incoming block
  blend sub-blocks
  write sub-block to image




Whole new idea:

When pating one image into another with blending:

Aquire reader from source image and writer from target image
Negotiate word type and color space
Prep reader and writer for chosen word type and color space.
Devide region into sub-blocks
For each block:
  read from reader
  read from writer
  blend
  write to writer

BOGUS!

The advantage with this approach is that any kind of aliasing can be handled by the operator that introduces it, by having it subdivide the callers block.

The problem with this approach is that even the buffered image needs to dynamically allocate an accessor object because it may need to do color space and word type translation.

Also, consider how to handle the reintepret color space operator


"Node based compositing"


Whole new approach to handling "merge one image into another with arbitrary compositing operator" allowing for arbitrary read/write image wrappers with potential aliasing, and maintaining good efficiency for the simple cases:

Acquire a writer from the target image.
Construct a manipulator object on stack, that is prepped to work on the word type and color space of the writer.
Divide target region into sub-regions
For each sub-region:
  Ask the manipulator to prep itself for the current sub-region
  Call manip method of writer with manipulator, and a grid and location that corresponds the the current sub-region

In a buffered image the manip method of the writer translates into:
  Call low-level read method of image to read the specified region into the grid
  Call manip method of manipulator with the grid as argument
  Call low-level write method of image to write the grid to the specified region

In a horizontally flipping image operator the manip method of the writer translates into:
  Call sub-writer with same manipulator, and horizontally flipped grid  and location

Also, consider how to handle the reintepret color space operator


NOT GOOD: RESULT WILL IN GENERAL DEPEND ON THE ORDER IN WHICH SUB_BLOCKS ARE ITERATED, AND THIS IS AFFECTED IN COMPLEX WAYS BY SOME IMAGE OPERATORS

BETTER TO SAY THIS: WHEN merging one image into another both source and target can be composits. If a specific sub image 


When copying/merging one image into another and a sub-image occurs both as a part of the source and the target, the aliasing is handled by first copying the source into a temporary image, then using the temporary as source. Detection of such aliasing is done in a simplisitc way by first asking the source for the ultimate source. If this is not unique, null is returned to indicate possible aliasing. If a unique source was found, the target is asked for the ultimate target. If this is null or equal to the ultimate source, there is potential aliasing. This will give many false positives, but also prevent the detection process from becoming too expensive. All simple cases should be detected correctly.

Blending/merging could also be handled by on the fly constructing a blending image operator, although it will likely be less efficient. Needs to be considered further.

We should probably never do blending writes if target has potential internal aliasing. In that case we must fall back to the method mentioned above.

When an image operator can introduce aliasing and is writable it must at construction time make the simplistic alias detection (unless the result is obvious). If potential aliasing is detected ???

Ideally the effect should be as if each pixel was written individually starting with the lower left one, and then proceeding in row major order. However, this is hard to implement. One way that might work is if we write one row at a time rather than a block, and then when a node needs to plit it up, it must observe its direction of growth, and write the parts in that order.

Maybe add a \c flattern method to \c Grid that if possible will join the rows to one long row. This is possible if width*abs(pitch) = abs(stride) || height*abs(stride) = abs(width)


Plan for first steps:

Postpone any handling of aliasing

High level putBlock(block, wordType, colorSpace, width, height, left, bottom):
  Establish max pixels per block
  If incoming block is small enough:
    If no conversion is needed:
      acquireWriter(width*height)->writeSafe(block, width, height, left, bottom);
      return;
    WriteBuffers buffers;
    writeSmall(block, wordType, colorSpace, width, height, left, bottom, buffers);
    return;
  AutoReleaser<Writer> writer = acquireWriter();
  WriteBuffers buffers; // Has fixed number of fixed size buffers
  BlockWriteOp op(block);
  manip(op, width, height, buffers, false);

High level putImage(image, left, bottom):
  Clip target region to target image
  Image::WriterPtr writer = acquireWriter();
  See if we can choose a block size such that we only need to read once from source image
  WriteBuffers buffers; // Has fixed number of fixed size buffers
  ImageWriteOp op(buffers, block, wordType, colorSpace);
  manip(op, width, height, buffers);

void manip(Op &op, wordType, colorSpace, Writer *writer, WriteBuffers &buffers)
{
  bool read = op.needRead();
  for(each sub-block)
  {
    if(read)
    {
      writer->readSafe(readBuffer); // Must be a grid
      if(rdToFloat)   rdToFloat->convert(rdToFloat1,      rdToFloat2,   n);
      if(rdToRgba)    rdToRgba->convert(rdToRgba1,        rdToRgba2,    n);
      if(rdFromRgb)   rdFromRgb->convert(rdFromRgba1,     rdFromRgba2,  n);
      if(rdFromFloat) rdFromFloat->convert(rdFromFloat1,  rdFromFloat2, n);
    }
    op.manip(manipBuffer, x, y, w, h);
    if(wrFromFloat) wrFromFloat->convert(wrFromFloat1, wrFromFloat2, n);
    if(wrFromRgb)   wrFromRgb->convert(wrFromRgba1,    wrFromRgba2,  n);
    if(wrToRgba)    wrToRgba->convert(wrToRgba1,       wrToRgba2,    n);
    if(wrToFloat)   wrToFloat->convert(wrToFloat1,     wrToFloat2,   n);
    writer->writeSafe(writeBuffer); Must be a grid
  }
}


struct ImageReader
{
  static const int maxPixelsPerBlock = 1024; // = 32 * 32
  ImageReader(Image::ConstRefArg image):
    image(image), accessor(image->acquireAccessor(maxPixelsPerBlock).release()), x(0), y(0) {}

  void setCursorPos(int x, int y) { this->x = x; this->y = y; }
  void setBlendFunc();
  void putImage(source);
private:
  Image::ConstRef const image;
  Image::AccessorPtr const accessor;
  int x, y;
};


Introduce the Reader class and the acquisition method




The meain idea is that a "graphical context" is created and all access to image data goes through it. The context holds the buffers that are needed to convert to and from the color space and word type required by the image, such that these need not be allocated repeatedly for each access. In general the context object will subdivide any block operation into subblocks of a certain maximum number of pixels.

static const int Image::Reader::maxPixelsPerBlock = 1024; // = 32 * 32
static const int ColorSpace::maxNumberOfChannels = 256; // = maxPixelsPerBlock * 4 / sizeof(long double)


Two buffers are needed in Image::Context.

The size of each of these buffers is calculated as follows:

There must be enough space for a block of maxPixelsPerBlock pixels of the type and color space required by the image, such that when the callers buffer uses same word type and color space, we can work with maximum block size. Since we don't know what the color space and word type is of the callers buffer, we must be able to also store at least one pixel of the widest possible type (maximum number of channels and widest word type).

bytesPerPixel = image->getNumberOfChannels() * getBytesPerWord(reader->getWordType());
bufferSize = max(maxPixelsPerBlock * bytesPerPixel, ColorSpace::maxNumberOfChannels * getMaxBytesPerWord());

*/


struct Image
{
protected:
  struct Reader;
  struct Writer;
  struct Manipulator;

  /**
   * This call enables the image to allocate extra resources (buffers)
   * needed for reading. In the case of the standard buffered image
   * implementation, no extra resources are needed, and the method
   * will simply return the image object itself, leading to minimal
   * overhead.
   *
   * The returned reader must be released when reading is
   * complete. This is done by calling its \c release method.
   */
  Reader *acquireReader() const;

  /**
   * This call enables the image to allocate extra resources (buffers)
   * needed for writing. In the case of the standard buffered image
   * implementation, no extra resources are needed, and the method
   * will simply return the image object itself, leading to minimal
   * overhead.
   *
   * The returned writer must be released when writing is
   * complete. This is done by calling its \c release method.
   */
  Writer *acquireWriter();
};


struct Image::Reader
{
  virtual void release() = 0;

  // Lowest level read method - region is assumed to be withing image borders
  virtual void read(Core::Grid<char *> const &tray, int left, int bottom) = 0;

  virtual ~Reader() {}
};


struct Image::Writer: Image::Reader
{
  void manip(Manipulator &manip, int width, int height, int left, int bottom);

  // Lowest level write method - region is assumed to be withing image borders
  virtual void write(Core::Grid<char const *> const &tray, int left, int bottom) = 0;
};


struct Image::Manipulator
{
  /**
   * \param n Number of pixels.
   */
  virtual void manip(void *pixels, size_t n);
};


void Image::Encoder::manip(Manipulator &manip, int width, int height, int left, int bottom)
{
  // Divide target region into sub-blocks
  // For each sub-block:
  //   
}



void Image::putImage(Image::ConstRefArg source)
{
  // We will ask the reader to provide data using the word type and color space of the target image
  WriteHelper writer(this);
  // writer.writeImage(source); BAD IDEA
}



struct PixelEncoder
{
  virtual void release() = 0;
  virtual ~PixelEncoder() {}
};


struct Image
{

protected:

  /**
   * A decoder gives read access to the image data.
   */
  struct Decoder
  {
    virtual void decode() const = 0;
  };

  struct Encoder: Decoder
  {
    virtual void encode(Grid<void const *> const &tray, int left, int bottom) const = 0;
    void encodeSafe(Grid<void const *> const &tray, int left, int bottom,
                    int horizontalRepeat, int verticalRepeat) const;
  };

  auto_ptr<Decoder> acquireDecoder(size_t maxPixelsPerBlock) const;

  /**
   * May allocate buffers. Might also want to cache intermediate results. Maybe this can even be used to make a copy when reading and writing overlaps (aliasing). We might want to specify the target source region, such that preprocessing does not need to process more that is needed. Maybe this feature can be used for dithering too.
   *
   * THE LIFETIME OF THE ENCODER MUST NOT EXTEND BEYOND THE END OF THE LIFETIME OF THE IMAGE
   */
  auto_ptr<Encoder> acquireEncoder(size_t maxPixelsPerBlock);
};


// Also consider alpha blending (read, modify, write)


// Allocate at most two buffers, then use them alternatingly. Each buffer must be as large as the largets among the logical ones merged to one.


struct DecodeHelper
{
  void decode(primaryBuffer, secondaryBuffer)
  {
    decode(decodeBuffer); // Virtual
    if(toFloat) toFloat->convert(decodeBuffer, floatBuffer);
    if(toRgba) toRgba->convert(floatBuffer, rgbaBuffer);
    if(fromRgba) fromRgba->convert(rgbaBuffer, lastBuffer);
  }

  void decodeSafe(Grid<char *> const &tray, int width, int height, int left, int bottom,
                  int horizontalRepeat, int verticalRepeat);

  void decodeSafe(void *tray, int width, int height, int left, int bottom,
                  int horizontalRepeat, int verticalRepeat)
  {
    decodeSafe(Grid<char *>(tray, width, height), left, bottom,
               horizontalRepeat, verticalRepeat);
  }

  WordType preferredWordType;
  ColorSpace *preferredColorSpace;

  DecodeHelper(Image const *source, BufferManager &, WordType t, ColorSpace *c);
  decode()
  pixelSizes[2];
};

void Image::putBlock(void const *tray, WordType wordType, ColorSpace::ConstRefArg colorSpace,
                     int width, int height, int left = 0, int bottom = 0,
                     int horizontalRepeat = 0, int verticalRepeat = 0)
{
  BufferManager buffers;
  EncodeHelper encoder(this, buffers, wordType, colorSpace);
  encoder->encodeSafe(tray, width, height, left, bottom, horizontalRepeat, verticalRepeat);
}

/**
 * CAUTION: The size of the specified tray buffer must be at least
 * <tt>height * width * 4</tt>.
 *
 * Each color component occupies one <tt>char</tt>. Please note that the
 * maximum intensity corresponds to <tt>std::numeric_limits<unsigned
 * char>::max()</tt> which is not cessessarily equal to 255.
 */
void Image::putBlockRgba(unsigned char const *tray,
                         int width, int height, int left = 0, int bottom = 0,
                         int horizontalRepeat = 0, int verticalRepeat = 0)
{
  putBlock(tray, wordType_uchar, ColorSpace::getRgba(),
           width, height, left, bottom, horizontalRepeat, verticalRepeat);
}

struct Manipulator
{
  virtual WordType getWordType() const = 0;
  virtual ColorSpace::ConstRef getColorSpace() const = 0;
  /**
   * <param n Number of pixels.
   */
  virtual void manip(void *pixels, size_t n) = 0;
  virtual ~Manipulator() {}
};

void Image::manipBlock(Manip const &manip,
                       int width, int height, int left = 0, int bottom = 0,
                       int horizontalRepeat = 0, int verticalRepeat = 0)
{
  WordType wordType = manip.getWordType();
  ColorSpace::ConsrRef colorSpace = manip.getColorSpace();
  BufferManager buffers;
  DecodeHelper decoder(this, buffers, wordType, colorSpace);
  EncodeHelper encoder(this, buffers, wordType, colorSpace);
}


void blit(Image const *source, Image *target)
{
  // Best case:
  //   decode: sourceImage -> buffer
  //   encode: buffer -> targetImage
  //
  // Worst case:
  //   decode: sourceImage -> decodeBuffer
  //   typeConvert:  decodeBuffer -> sourceNativeBuffer
  //   colorConvert: sourceNativeBuffer -> rgbaBuffer
  //   colorConvert: rgbaBuffer -> targetNativeBuffer
  //   typeConvert:  targetNativeBuffer -> encodeBuffer
  //   encode: encodeBuffer -> targetImage

  size_t maxPixelsPerBlock = 24*24;

  auto_ptr<Image::Decoder> decoder(source->acquireDecoder(maxPixelsPerBlock));
  WordType decodeWordType = decoder->getWordType();

  auto_ptr<Image::Encoder> encoder(target->prepareEncoder(maxPixelsPerBlock));
  WordType encodeWordType = encoder->getWordType();

  bool needColorConv;

  // Get the smallest floating point type with enough bits. Adding 4
  // to allow for a bit of numeric instability in color space
  // conversion
  WordType colorConvWordType =
    min(isFloatingPoint(decodeWordType) ? decodeWordType :
        getBestFloatTypeByMantissaBits(getBitsPerWord(decodeWordType) + 4),
        isFloatingPoint(encodeWordType) ? encodeWordType :
        getBestFloatTypeByMantissaBits(getBitsPerWord(encodeWordType) + 4));

  if(decodeWordType != colorConvWordType) decodeTypeConverter = ...;
  if(encodeWordType != colorConvWordType) encodeTypeConverter = ...;
  
  int pixelSizes[2] = { 0, 0 };
  {
    pixelSizes[0] = numberOfSourceChannels * getBytesPerWord(decodeWordType);
    int i = 1;
    if(decodeTypeConverter)
    {
      int s = numberOfSourceChannels * getBytesPerWord(colorConvWordType);
      if(pixelSizes[1] < s) pixelSizes[1] = s;
      i ^= 1;
    }
    if(decodeColorConverter)
    {
      int s = 4 * getBytesPerWord(colorConvWordType);
      if(pixelSizes[i] < s) pixelSizes[i] = s;
      i ^= 1;
    }
    if(encodeColorConverter)
    {
      int s = numberOfTargetChannels * getBytesPerWord(colorConvWordType);
      if(pixelSizes[i] < s) pixelSizes[i] = s;
      i ^= 1;
    }
    if(encodeTypeConverter)
    {
      int s = numberOfTargetChannels * getBytesPerWord(encodeWordType);
      if(pixelSizes[i] < s) pixelSizes[i] = s;
    }
  }

  MemoryBuffer buffers[2] = { maxPixelsPerBlock * pixelSizes[0],
                              maxPixelsPerBlock * pixelSizes[1] };
}


Image::Encoder *BufferedImage::prepareEncoder(maxPixelsPerBlock) const
{
}





      /**
       * Get the type of words used in decoded image data where each
       * channel occupies one word. For integer words of type
       * <tt>T</tt>, <tt>std::numeric_limits<T>::min()</tt>
       * corresponds to no intensity, and
       * <tt>std::numeric_limits<T>::max()</tt> corresponds to full
       * intensity. For floating point types 0 corresponds to no
       * intensity, and 1 corresponds to full intensity, however,
       * actual values may exceed this limit.
       */
virtual WordType PixelCodec::getWordType() const = 0;

