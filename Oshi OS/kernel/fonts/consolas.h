// Header (.h) for font: Consolas Italic 30

#ifndef CONSOLAS_ITALIC_30_FONT_H
#define CONSOLAS_ITALIC_30_FONT_H

#include <stdint.h>

extern const int TALLEST_CHAR_PIXELS;
extern const uint8_t consolas_italic_30_font_pixels[];

struct font_char {
    int offset;
    int w;
    int h;
    int left;
    int top;
    int advance;
};

extern const struct font_char consolas_italic_30_font_lookup[];

#endif
