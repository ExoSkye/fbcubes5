#include <packed.h>
#include <defines.h>

typedef PACKED_STRUCT(
    colour_t {
        u8 r;
        u8 g;
        u8 b;
    }
) colour_t;

extern char* fb_path;

extern void clearscreen();

extern void drawpixel(int x, int y, colour_t colour);