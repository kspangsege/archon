import sys
import os

import fontTools.fontBuilder
import fontTools.pens.ttGlyphPen
import fontTools.ttLib
import fontTools.colorLib.builder


self_path = sys.argv[0]
assert os.path.basename(self_path) == "create_test_fonts.py"
self_dir_path = os.path.dirname(self_path)


def int_div_round_half_up(dividend, divisor):
    quotient = dividend // divisor
    remainder = dividend % divisor
    doubled_remainder = remainder * 2
    if doubled_remainder >= divisor:
        return quotient + 1
    else:
        return quotient

def int_div_round_half_even(dividend, divisor):
    quotient = dividend // divisor
    remainder = dividend % divisor
    doubled_remainder = remainder * 2
    if doubled_remainder > divisor:
        return quotient + 1
    elif doubled_remainder < divisor:
        return quotient
    else:
        if quotient % 2 == 0:
            return quotient
        else:
            return quotient + 1


def create_scalable_uncolored_truetype_test_font(output_path):
    fb = fontTools.fontBuilder.FontBuilder(unitsPerEm = 1024, isTTF = True)

    # Every font requires a ".notdef" (fallback) glyph at index 0
    fb.setupGlyphOrder([".notdef", "space", "A", "B", "C"])

    fb.setupCharacterMap({
        0x0020: "space",
        0x0041: "A",
        0x0042: "B",
        0x0043: "C",
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

    # First test glyph is a filled triangle half of which sits to the left of the Y-axis
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((-462, 0))
    pen.lineTo((0, 800))
    pen.lineTo((462, 0))
    pen.closePath()
    test_glyph_1 = pen.glyph()

    # Second test glyph is a very wide and low triangle that can produce a perfect rising
    # coverage gradient
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((256, 1))
    pen.lineTo((256, 0))
    pen.closePath()
    test_glyph_2 = pen.glyph()

    # Third test glyph is a very wide and low triangle that can produce a perfect falling
    # coverage gradient
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((0, 1))
    pen.lineTo((256, 0))
    pen.closePath()
    test_glyph_3 = pen.glyph()

    fb.setupGlyf({
        ".notdef": notdef_glyph,
        "space": blank_glyph,
        "A": test_glyph_1,
        "B": test_glyph_2,
        "C": test_glyph_3,
    })

    # Format: {glyphName: (advanceWidth, leftSideBearing)} where leftSideBearing (LSB) is
    # the displacement of the left side of the bounding box relative to the origin
    fb.setupHorizontalMetrics({
        ".notdef": (600, 50),
        "space": (256, 0),
        "A": (948, -462),
        "B": (512, 128),
        "C": (512, 128),
    })

    ascent = 800
    descent = -200
    line_gap = 24
    fb.setupHorizontalHeader(ascent = ascent, descent = descent, lineGap = line_gap)
    fb.setupNameTable({"familyName": "Test", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender = ascent, sTypoDescender = descent, sTypoLineGap = line_gap)
    fb.setupPost()

    # Hardcode the timestamps to avoid introducing unnecessary diffs
    fb.font["head"].created = 0
    fb.font["head"].modified = 0

    fb.save(output_path)
    print(f"Created scalable uncolored TrueType test font: {output_path}")


def create_scalable_colored_truetype_test_font(output_path):
    fb = fontTools.fontBuilder.FontBuilder(unitsPerEm = 1024, isTTF = True)

    # Define the base glyphs (mapped to characters) and the layer glyphs
    fb.setupGlyphOrder([
        ".notdef", "space", "A", "B",
        "A_layer_1", "A_layer_2",
        "B_layer_1", "B_layer_2"
    ])

    fb.setupCharacterMap({
        0x0020: "space",
        0x0041: "A",
        0x0042: "B",
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

    # Glyph A, Layer 1: Outer Red Square (500x500)
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((0, 500))
    pen.lineTo((500, 500))
    pen.lineTo((500, 0))
    pen.closePath()
    a_layer_1 = pen.glyph()

    # Glyph A, Layer 2: Inner Blue Square (300x300, offset 100)
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((100, 100))
    pen.lineTo((100, 400))
    pen.lineTo((400, 400))
    pen.lineTo((400, 100))
    pen.closePath()
    a_layer_2 = pen.glyph()

    # Glyph B, Layer 1: Left Green Rectangle (250x500)
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((0, 0))
    pen.lineTo((0, 500))
    pen.lineTo((250, 500))
    pen.lineTo((250, 0))
    pen.closePath()
    b_layer_1 = pen.glyph()

    # Glyph B, Layer 2: Right Yellow Rectangle (250x500, offset 250)
    pen = fontTools.pens.ttGlyphPen.TTGlyphPen(None)
    pen.moveTo((250, 0))
    pen.lineTo((250, 500))
    pen.lineTo((500, 500))
    pen.lineTo((500, 0))
    pen.closePath()
    b_layer_2 = pen.glyph()

    fb.setupGlyf({
        ".notdef": notdef_glyph,
        "space": blank_glyph,
        "A": blank_glyph, # Base glyph is blank; COLR overlays the layers
        "B": blank_glyph,
        "A_layer_1": a_layer_1,
        "A_layer_2": a_layer_2,
        "B_layer_1": b_layer_1,
        "B_layer_2": b_layer_2,
    })

    # Format: {glyphName: (advanceWidth, leftSideBearing)} where leftSideBearing (LSB) is
    # the displacement of the left side of the bounding box relative to the origin
    fb.setupHorizontalMetrics({
        ".notdef": (600, 50),
        "space": (256, 0),
        "A": (512, 6),
        "B": (512, 6),
        "A_layer_1": (512, 6),
        "A_layer_2": (512, 106),
        "B_layer_1": (512, 6),
        "B_layer_2": (512, 256),
    })

    # Setup the CPAL table. Each palette is a list of RGBA tuples (Red, Green, Blue, Alpha)
    # encoded as floats ranging from 0.0 to 1.0.
    palettes = [
        [
            (1.0, 0.0, 0.0, 1.0),  # Palette Index 0: Red
            (0.0, 0.0, 1.0, 1.0),  # Palette Index 1: Blue
            (0.0, 1.0, 0.0, 1.0),  # Palette Index 2: Green
            (1.0, 1.0, 0.0, 1.0),  # Palette Index 3: Yellow
        ]
    ]
    fb.font["CPAL"] = fontTools.colorLib.builder.buildCPAL(palettes)

    # Setup the COLR table. Maps base glyphs to a list of (layer_glyph_name, palette_index)
    colr_layers = {
        "A": [("A_layer_1", 0), ("A_layer_2", 1)],
        "B": [("B_layer_1", 2), ("B_layer_2", 3)],
    }
    fb.font["COLR"] = fontTools.colorLib.builder.buildCOLR(colr_layers)

    ascent = 800
    descent = -200
    line_gap = 24
    fb.setupHorizontalHeader(ascent = ascent, descent = descent, lineGap = line_gap)
    fb.setupNameTable({"familyName": "Test", "styleName": "Regular"})
    fb.setupOS2(sTypoAscender = ascent, sTypoDescender = descent, sTypoLineGap = line_gap)
    fb.setupPost()

    # Hardcode timestamps to avoid introducing unnecessary diffs
    fb.font["head"].created = 0
    fb.font["head"].modified = 0

    fb.save(output_path)
    print(f"Created scalable colored TrueType test font (COLRv0): {output_path}")


def create_bdf_test_font(output_path):
    font_ascent = 12
    font_descent = 4
    ppem = font_ascent + font_descent
    dpi = 72
    point_size = ppem * 10

    class Glyph:
        def __init__(self, char_name, encoding, width, height, advance, x, y, ink):
            self.char_name = char_name
            self.encoding = encoding
            self.width = width
            self.height = height
            self.advance = advance
            self.x = x # Horizontal position of left-edge of pixel block (increases to the right)
            self.y = y # Vertical position of bottom-edge of pixel block (increases upwards)
            self.ink = ink

    glyphs = []
    def add_glyph(char_name, encoding, width, height, advance, x, y, ink):
        glyphs.append(Glyph(char_name, encoding, width, height, advance, x, y, ink))

    add_glyph("replacement", 65533, 8, 10, 12, 2, 0, [ "++++++++",
                                                       "+      +",
                                                       "+      +",
                                                       "+      +",
                                                       "+      +",
                                                       "+      +",
                                                       "+      +",
                                                       "+      +",
                                                       "+      +",
                                                       "++++++++" ])

    add_glyph("space", 32, 0, 0, 6, 0, 0, [])

    add_glyph("A", 65, 10, 10, 14, 2, 0, [ "    ++    ",
                                           "   +  +   ",
                                           "  +    +  ",
                                           " +      + ",
                                           "+        +",
                                           "++++++++++",
                                           "+        +",
                                           "+        +",
                                           "+        +",
                                           "+        +" ])

    add_glyph("B", 66, 8, 13, 12, 2, -2, [ "+++++   ",
                                           "+    ++ ",
                                           "+      +",
                                           "+      +",
                                           "+      +",
                                           "+    ++ ",
                                           "+++++   ",
                                           "+    ++ ",
                                           "+      +",
                                           "+      +",
                                           "+      +",
                                           "+    ++ ",
                                           "+++++   " ])

    assert glyphs
    avg_advance = sum(g.advance for g in glyphs) / len(glyphs)
    avg_width = int(avg_advance * 10)
    min_x = min(g.x for g in glyphs)
    min_y = min(g.y for g in glyphs)
    max_x = max(g.x + g.width for g in glyphs)
    max_y = max(g.y + g.height for g in glyphs)

    # Construct the BDF global header strings
    bdf = ""
    bdf += f"STARTFONT 2.1\n"
    # XLFD: -foundry-family-weight-slant-setwidth-addstyle-pixel-point-resx-resy-spacing-avgwidth-registry-encoding
    bdf += f"FONT -py-Test--r-normal--{ppem}-{point_size}-{dpi}-{dpi}-p-{avg_width}-iso10646-1\n"
    bdf += f"SIZE {point_size // 10} {dpi} {dpi}\n"
    bdf += f"FONTBOUNDINGBOX {max_x - min_x} {max_y - min_y} {min_x} {min_y}\n"
    bdf += f"STARTPROPERTIES 4\n"
    bdf += f"FAMILY_NAME \"Test\"\n"
    bdf += f"FONT_ASCENT {font_ascent}\n"
    bdf += f"FONT_DESCENT {font_descent}\n"
    bdf += f"DEFAULT_CHAR {glyphs[0].encoding}\n"
    bdf += f"ENDPROPERTIES\n"
    bdf += f"CHARS {len(glyphs)}\n"

    for g in glyphs:
        # SWIDTH is the scalable width (Design Units)
        swidth = int((g.advance / ppem) * 1000) if ppem > 0 else 0

        # Build the character block
        bdf += f"STARTCHAR {g.char_name}\n"
        bdf += f"ENCODING {g.encoding}\n"
        bdf += f"SWIDTH {swidth} 0\n"
        bdf += f"DWIDTH {g.advance} 0\n"
        bdf += f"BBX {g.width} {g.height} {g.x} {g.y}\n"
        bdf += f"BITMAP\n"

        byte_width = (g.width + 7) // 8
        assert len(g.ink) == g.height
        for line in g.ink:
            assert len(line) == g.width
            bit_string = ''.join('1' if char != ' ' else '0' for char in line)
            bit_string += '0' * (byte_width * 8 - g.width)
            row_hex = format(int(bit_string, 2), f'0{byte_width * 2}X')
            bdf += row_hex + "\n"

        bdf += "ENDCHAR\n"

    bdf += "ENDFONT\n"

    with open(output_path, "w") as f:
        f.write(bdf)

    print(f"Created non-scalable uncolored BDF test font: {output_path}")


def create_nonscalable_uncolored_bitmap_truetype_test_font(output_path):
    # Font metrics context
    ascent_px = 12
    descent_px = 4
    strike_size = ascent_px + descent_px # 16 (Pixels Per EM)
    upem = 1000                          # Units Per EM

    # Calculate global metrics in Design Units (proportional to UPEM)
    ascent_du = int(round((ascent_px / strike_size) * upem))
    descent_du = int(round((-descent_px / strike_size) * upem))

    class Glyph:
        def __init__(self, char_name, encoding, width, height, advance, x, y, ink):
            self.char_name = char_name
            self.encoding = encoding
            self.width = width
            self.height = height
            self.advance = advance
            self.x = x # Horizontal position of left-edge (BearingX)
            self.y = y # Vertical position of bottom-edge (increases upwards)
            self.ink = ink

    glyphs = []

    def add_glyph(char_name, encoding, width, height, advance, x, y, ink):
        glyphs.append(Glyph(char_name, encoding, width, height, advance, x, y, ink))

    # NOTE: The first glyph in a TrueType font MUST always be '.notdef'
    add_glyph(".notdef", 0, 8, 10, 12, 2, 0, [
        "77777777",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "77777777",
    ])

    add_glyph("space", 32, 0, 0, 6, 0, 0, [])

    add_glyph("A", 65, 10, 10, 14, 2, 0, [
        "    55    ",
        "   7227   ",
        "  7    7  ",
        " 7      7 ",
        "61      16",
        "7777777777",
        "7        7",
        "7        7",
        "7        7",
        "7        7",
    ])

    add_glyph("B", 66, 8, 13, 12, 2, -2, [
        "777764  ",
        "7    47 ",
        "7     27",
        "7      7",
        "7     27",
        "7    47 ",
        "777764  ",
        "7    47 ",
        "7     27",
        "7      7",
        "7     27",
        "7    47 ",
        "777764  ",
    ])

    assert glyphs
    assert glyphs[0].char_name == ".notdef"

    # Extract dynamic properties
    glyph_order = [g.char_name for g in glyphs]
    cmap = {g.encoding: g.char_name for g in glyphs if g.encoding > 0}
    max_width = max((g.width for g in glyphs), default = strike_size)

    # 1. Initialize FontBuilder
    fb = fontTools.fontBuilder.FontBuilder(upem, isTTF = True)
    fb.setupGlyphOrder(glyph_order)
    fb.setupCharacterMap(cmap)

    # Calculate proportional horizontal metrics based on UPEM
    hmtx = {}
    for g in glyphs:
        advance_design_units = int(round((g.advance / strike_size) * upem))
        hmtx[g.char_name] = (advance_design_units, 0)
    fb.setupHorizontalMetrics(hmtx)

    # Synchronized Vertical Metrics
    fb.setupHorizontalHeader(ascent = ascent_du, descent = descent_du)
    fb.setupNameTable({"familyName": "Test", "styleName": "Regular", "psName": "Test-Regular"})
    fb.setupOS2(sTypoAscender = ascent_du, sTypoDescender = descent_du, usWinAscent = ascent_du,
                usWinDescent = abs(descent_du))
    fb.setupPost()

    font = fb.font

    # 2. Zero out standard TrueType contours to ensure non-scalable designation
    maxp = fontTools.ttLib.newTable('maxp')
    maxp.tableVersion = 0x00010000
    maxp.numGlyphs = len(glyph_order)
    maxp.maxPoints = 0
    maxp.maxContours = 0
    maxp.maxCompositePoints = 0
    maxp.maxCompositeContours = 0
    maxp.maxZones = 1
    maxp.maxTwilightPoints = 0
    maxp.maxStorage = 0
    maxp.maxFunctionDefs = 0
    maxp.maxInstructionDefs = 0
    maxp.maxStackElements = 0
    maxp.maxSizeOfInstructions = 0
    maxp.maxComponentElements = 0
    maxp.maxComponentDepth = 0
    font['maxp'] = maxp

    # 3. Build Standard OpenType EBLC and EBDT
    eblc = fontTools.ttLib.newTable('EBLC')
    ebdt = fontTools.ttLib.newTable('EBDT')
    eblc.version = 2.0
    ebdt.version = 2.0

    strike_data = {}
    for glyph in glyphs:
        # Define the Pixel Metrics
        metrics = fontTools.ttLib.tables.BitmapGlyphMetrics.SmallGlyphMetrics()
        metrics.height = glyph.height
        metrics.width = glyph.width
        metrics.BearingX = glyph.x

        # TrueType BearingY is measured from the baseline to the TOP of the bounding box.
        metrics.BearingY = glyph.y + glyph.height
        metrics.Advance = glyph.advance

        assert len(glyph.ink) == glyph.height
        raw_pixels = bytearray()
        for row in glyph.ink:
            assert len(row) == glyph.width
            for char in row:
                assert char == ' ' or '1' <= char <= '7'
                val = 0 if char == ' ' else int(char)
                alpha = int_div_round_half_even(val * 255, 7)
                raw_pixels.append(alpha)

        bitmap_glyph = fontTools.ttLib.tables.E_B_D_T_.ebdt_bitmap_format_1(data = b'', ttFont = font)
        bitmap_glyph.metrics = metrics
        bitmap_glyph.imageData = bytes(raw_pixels)
        strike_data[glyph.char_name] = bitmap_glyph

    ebdt.strikeData = [strike_data]

    # Create the Size Table in EBLC
    size_table = fontTools.ttLib.tables.E_B_L_C_.BitmapSizeTable()
    size_table.ppemX = strike_size
    size_table.ppemY = strike_size
    size_table.bitDepth = 8
    size_table.flags = 0x01
    size_table.colorRef = 0

    # Synchronized Line Metrics
    line_metrics = fontTools.ttLib.tables.E_B_L_C_.SbitLineMetrics()
    line_metrics.ascender = ascent_px
    line_metrics.descender = -descent_px
    line_metrics.widthMax = max_width
    line_metrics.minOriginSB = 0
    line_metrics.minAdvanceSB = 0
    line_metrics.maxBeforeBL = 0
    line_metrics.minAfterBL = 0
    line_metrics.caretSlopeNumerator = 1
    line_metrics.caretSlopeDenominator = 0
    line_metrics.caretOffset = 0
    line_metrics.pad1 = 0
    line_metrics.pad2 = 0

    size_table.hori = line_metrics
    size_table.vert = line_metrics

    # Create the Index Subtable
    sub_table = fontTools.ttLib.tables.E_B_L_C_.eblc_index_sub_table_1(data = b'', ttFont = font)
    sub_table.indexFormat = 1
    sub_table.firstGlyphIndex = 0
    sub_table.lastGlyphIndex = len(glyph_order) - 1
    sub_table.imageFormat = 1
    sub_table.names = glyph_order

    strike = fontTools.ttLib.tables.E_B_L_C_.Strike()
    strike.bitmapSizeTable = size_table
    strike.indexSubTables = [sub_table]

    eblc.strikes = [strike]

    font["EBLC"] = eblc
    font["EBDT"] = ebdt

    # Freeze timestamps to ensure deterministic output
    font["head"].created = 0
    font["head"].modified = 0

    font.recalcTimestamp = False
    font.save(output_path)

    print(f"Created non-scalable uncolored TrueType test font (EBLC/EBDT): {output_path}")


def create_nonscalable_colored_bitmap_truetype_test_font(output_path):
    # Font metrics context
    ascent_px = 12
    descent_px = 4
    strike_size = ascent_px + descent_px # 16 (Pixels Per EM)
    upem = 1000                          # Units Per EM

    # Calculate global metrics in Design Units (proportional to UPEM)
    ascent_du = int(round((ascent_px / strike_size) * upem))
    descent_du = int(round((-descent_px / strike_size) * upem))

    class Glyph:
        def __init__(self, char_name, encoding, width, height, advance, x, y, color, ink):
            self.char_name = char_name
            self.encoding = encoding
            self.width = width
            self.height = height
            self.advance = advance
            self.x = x # Horizontal position of left-edge (BearingX)
            self.y = y # Vertical position of bottom-edge (increases upwards)
            self.color = color
            self.ink = ink

    glyphs = []

    def add_glyph(char_name, encoding, width, height, advance, x, y, color, ink):
        glyphs.append(Glyph(char_name, encoding, width, height, advance, x, y, color, ink))

    black  = (0, 0, 0)
    gray   = (150, 150, 150)
    blue   = (0, 150, 255)
    orange = (255, 100, 0)

    # NOTE: The first glyph in a TrueType font MUST always be '.notdef'
    add_glyph(".notdef", 0, 8, 10, 12, 2, 0, gray, [
        "77777777",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "7      7",
        "77777777",
    ])

    add_glyph("space", 32, 0, 0, 6, 0, 0, black, [])

    add_glyph("A", 65, 10, 10, 14, 2, 0, blue, [
        "    55    ",
        "   7227   ",
        "  7    7  ",
        " 7      7 ",
        "61      16",
        "7777777777",
        "7        7",
        "7        7",
        "7        7",
        "7        7",
    ])

    add_glyph("B", 66, 8, 13, 12, 2, -2, orange, [
        "777764  ",
        "7    47 ",
        "7     27",
        "7      7",
        "7     27",
        "7    47 ",
        "777764  ",
        "7    47 ",
        "7     27",
        "7      7",
        "7     27",
        "7    47 ",
        "777764  ",
    ])

    assert glyphs
    assert glyphs[0].char_name == ".notdef"

    # Extract dynamic properties
    glyph_order = [g.char_name for g in glyphs]
    cmap = {g.encoding: g.char_name for g in glyphs if g.encoding > 0}
    max_width = max((g.width for g in glyphs), default = strike_size)

    # 1. Initialize FontBuilder
    fb = fontTools.fontBuilder.FontBuilder(upem, isTTF = True)
    fb.setupGlyphOrder(glyph_order)
    fb.setupCharacterMap(cmap)

    # Calculate proportional horizontal metrics based on UPEM
    hmtx = {}
    for g in glyphs:
        advance_design_units = int(round((g.advance / strike_size) * upem))
        hmtx[g.char_name] = (advance_design_units, 0)
    fb.setupHorizontalMetrics(hmtx)

    # Synchronized Vertical Metrics
    fb.setupHorizontalHeader(ascent = ascent_du, descent = descent_du)
    fb.setupNameTable({"familyName": "Test", "styleName": "Regular", "psName": "Test-Regular"})
    fb.setupOS2(sTypoAscender = ascent_du, sTypoDescender = descent_du, usWinAscent = ascent_du,
                usWinDescent = abs(descent_du))
    fb.setupPost()

    font = fb.font

    # 2. Zero out standard TrueType contours to ensure non-scalable designation
    maxp = fontTools.ttLib.newTable('maxp')
    maxp.tableVersion = 0x00010000
    maxp.numGlyphs = len(glyph_order)
    maxp.maxPoints = 0
    maxp.maxContours = 0
    maxp.maxCompositePoints = 0
    maxp.maxCompositeContours = 0
    maxp.maxZones = 1
    maxp.maxTwilightPoints = 0
    maxp.maxStorage = 0
    maxp.maxFunctionDefs = 0
    maxp.maxInstructionDefs = 0
    maxp.maxStackElements = 0
    maxp.maxSizeOfInstructions = 0
    maxp.maxComponentElements = 0
    maxp.maxComponentDepth = 0
    font['maxp'] = maxp

    # 3. Build Standard OpenType CBLC and CBDT
    cblc = fontTools.ttLib.newTable('CBLC')
    cbdt = fontTools.ttLib.newTable('CBDT')
    cblc.version = 3.0
    cbdt.version = 3.0

    strike_data = {}
    for glyph in glyphs:
        # Define the Pixel Metrics
        metrics = fontTools.ttLib.tables.BitmapGlyphMetrics.SmallGlyphMetrics()
        metrics.height = glyph.height
        metrics.width = glyph.width
        metrics.BearingX = glyph.x

        # TrueType BearingY is measured from the baseline to the TOP of the bounding box.
        metrics.BearingY = glyph.y + glyph.height
        metrics.Advance = glyph.advance

        assert len(glyph.ink) == glyph.height
        raw_pixels = bytearray()
        for row in glyph.ink:
            assert len(row) == glyph.width
            for char in row:
                assert char == ' ' or '1' <= char <= '7'
                val = 0 if char == ' ' else int(char)
                alpha = int_div_round_half_even(val * 255, 7)

                # This emulates how FreeType does alpha pre-multiplication internally
                r, g, b = glyph.color
                r_2 = int_div_round_half_up(r * alpha, 255)
                g_2 = int_div_round_half_up(g * alpha, 255)
                b_2 = int_div_round_half_up(b * alpha, 255)

                raw_pixels.extend([b_2, g_2, r_2, alpha])

        bitmap_glyph = fontTools.ttLib.tables.E_B_D_T_.ebdt_bitmap_format_1(data = b'', ttFont = font)
        bitmap_glyph.metrics = metrics
        bitmap_glyph.imageData = bytes(raw_pixels)
        strike_data[glyph.char_name] = bitmap_glyph

    cbdt.strikeData = [strike_data]

    # Create the Size Table in CBLC
    size_table = fontTools.ttLib.tables.E_B_L_C_.BitmapSizeTable()
    size_table.ppemX = strike_size
    size_table.ppemY = strike_size
    size_table.bitDepth = 32
    size_table.flags = 0x01
    size_table.colorRef = 0

    # Synchronized Line Metrics
    line_metrics = fontTools.ttLib.tables.E_B_L_C_.SbitLineMetrics()
    line_metrics.ascender = ascent_px
    line_metrics.descender = -descent_px
    line_metrics.widthMax = max_width
    line_metrics.minOriginSB = 0
    line_metrics.minAdvanceSB = 0
    line_metrics.maxBeforeBL = 0
    line_metrics.minAfterBL = 0
    line_metrics.caretSlopeNumerator = 1
    line_metrics.caretSlopeDenominator = 0
    line_metrics.caretOffset = 0
    line_metrics.pad1 = 0
    line_metrics.pad2 = 0

    size_table.hori = line_metrics
    size_table.vert = line_metrics

    # Create the Index Subtable
    sub_table = fontTools.ttLib.tables.E_B_L_C_.eblc_index_sub_table_1(data = b'', ttFont = font)
    sub_table.indexFormat = 1
    sub_table.firstGlyphIndex = 0
    sub_table.lastGlyphIndex = len(glyph_order) - 1
    sub_table.imageFormat = 1
    sub_table.names = glyph_order

    strike = fontTools.ttLib.tables.E_B_L_C_.Strike()
    strike.bitmapSizeTable = size_table
    strike.indexSubTables = [sub_table]

    cblc.strikes = [strike]

    font["CBLC"] = cblc
    font["CBDT"] = cbdt

    # Freeze timestamps to ensure deterministic output
    font["head"].created = 0
    font["head"].modified = 0

    font.recalcTimestamp = False
    font.save(output_path)

    print(f"Created non-scalable colored TrueType test font (CBLC/CBDT): {output_path}")


create_scalable_uncolored_truetype_test_font(os.path.join(self_dir_path, "test_font_scalable_uncolored.ttf"))
create_scalable_colored_truetype_test_font(os.path.join(self_dir_path, "test_font_scalable_colored.ttf"))
create_bdf_test_font(os.path.join(self_dir_path, "test_font_bitmap_mono.bdf"))
create_nonscalable_uncolored_bitmap_truetype_test_font(os.path.join(self_dir_path, "test_font_bitmap_gray.ttf"))
create_nonscalable_colored_bitmap_truetype_test_font(os.path.join(self_dir_path, "test_font_bitmap_color.ttf"))
