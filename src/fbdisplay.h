#include <packed.h>
#include <defines.h>
#include <stdbool.h>
#include <colour.h>

typedef PACKED_STRUCT(
    fb_info_t {
        u16 x;
        u16 y;
        u8 bpp;
    }
) fb_info_t;

extern colour_t* white;
extern colour_t* black;

extern bool fb_init();

extern fb_info_t fb_get_info();

extern void fb_clearscreen(colour_t* colour);

extern void fb_drawpixel(int x, int y, colour_t* colour);

extern void fb_exit();