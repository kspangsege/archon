import fontTools.fontBuilder
import fontTools.pens.ttGlyphPen

def generate_test_font(output_filename="test_font.ttf"):
    fb = fontTools.fontBuilder.FontBuilder(unitsPerEm=1024, isTTF=True)

    # Every font requires a ".notdef" (fallback) glyph at index 0
    fb.setupGlyphOrder([".notdef", "space", "A"])

    # Map the Unicode value for 'A' (U+0041) to our glyph name
    fb.setupCharacterMap({
        0x0020: "space",
        0x0041: "A",
    })

    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    # Outer contour (clockwise)
    pen.moveTo((50, 0))
    pen.lineTo((50, 700))
    pen.lineTo((550, 700))
    pen.lineTo((550, 0))
    pen.closePath()
    # Cut-out (counter-clockwise)
    pen.moveTo((100, 50))
    pen.lineTo((500, 50))
    pen.lineTo((500, 650))
    pen.lineTo((100, 650))
    pen.closePath()
    notdef_glyph = pen.glyph()

    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    blank_glyph = pen.glyph()

    # Test glyph is a filled triangle
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((-462, 0))
    pen.lineTo((0, 800))
    pen.lineTo((462, 0))
    pen.closePath()
    test_glyph = pen.glyph()

    fb.setupGlyf({
        ".notdef": notdef_glyph,
        "space": blank_glyph,
        "A": test_glyph,
    })

    # Format: {glyphName: (advanceWidth, leftSideBearing)} where leftSideBearing (LSB) is
    # the displacement relative to the origin of the left side of the bounding box
    fb.setupHorizontalMetrics({
        ".notdef": (600, 50),
        "space": (256, 0),
        "A": (948, -462)
    })

    ascent = 800
    descent = -200
    line_gap = 24
    fb.setupHorizontalHeader(ascent=ascent, descent=descent, lineGap=line_gap)
    fb.setupNameTable({"familyName": "Test", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender=ascent, sTypoDescender=descent, sTypoLineGap=line_gap)
    fb.setupPost()
    fb.save(output_filename)

if __name__ == "__main__":
    generate_test_font()
