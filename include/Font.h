// ============================================================================
// Font.h - description of the embedded bitmap fonts (data lives in FontData.cpp)
// ============================================================================
#pragma once

// One glyph of an embedded font. Coordinates are in pixels.
struct GlyphInfo {
    short w, h;        // bitmap size
    short xoff, yoff;  // offset of the bitmap from the pen position / line top
    short adv16;       // horizontal advance in 1/16 pixel units
};

// One embedded font size. Glyphs cover printable ASCII (32..126).
struct FontDesc {
    int pixelSize;
    int ascent;
    int lineHeight;
    bool bold;
    const GlyphInfo* info;     // 95 entries
    const char* const* bits;   // 95 hex strings, one hex digit (0..F) per pixel
};

constexpr int kNumEmbeddedFonts = 4;
extern const FontDesc kEmbeddedFonts[kNumEmbeddedFonts];
