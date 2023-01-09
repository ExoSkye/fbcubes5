#include <fbdisplay.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

static char* fb = NULL;

static fb_info_t fb_info;

colour_t white;
colour_t black;

#define xstr(s) str(s)
#define str(s) #s

bool fb_init() {
#ifndef FB_NAME
#define FB_NAME "fb0"
#endif
    char* fb_path = "/dev/" xstr(FB_NAME) ;

    if(access(fb_path, W_OK) != 0) {
        printf("ERROR: You don't have sufficient permissions to write to the framebuffer (file: %s), exiting...\n", fb_path);
        return false;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    int fb_file = open(fb_path, O_RDWR);

    if (ioctl(fb_file, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit(2);
    }

    if (ioctl(fb_file, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit(3);
    }

    fb_info.x = vinfo.xres_virtual;
    fb_info.y = vinfo.yres_virtual;
    fb_info.bpp = vinfo.bits_per_pixel;

    black = fb_make_colour(0, 0, 0, 255);
    white = fb_make_colour(0, 0, 255, 255);

    fb = (char *)mmap(0, fb_info.x * fb_info.y * (fb_info.bpp / 8), PROT_READ | PROT_WRITE, MAP_SHARED, fb_file, 0);

    if ((long)fb == -1) {
        printf("ERROR: Failed to map framebuffer to memory");
        return false;
    }

    close(fb_file);

    fb_clearscreen(black);

    return true;
}

fb_info_t fb_get_info() {
    return fb_info;
}

void fb_clearscreen(colour_t colour) {
    for (u16 y = 0; y < fb_info.y; y++) {
        for (u16 x = 0; x < fb_info.x; x++) {
            fb_drawpixel(x, y, colour);
        }
    }
}

void fb_drawpixel(int x, int y, colour_t colour) {

    ((u32*)fb)[y * fb_info.x + x] = *(u32*)&colour;
}

void fb_exit() {
    if (fb != NULL) {
        munmap(fb, fb_info.x * fb_info.y * (fb_info.bpp / 8));
    }
}

colour_t fb_make_colour(u8 r, u8 g, u8 b, u8 a) {
    colour_t colour_val = (colour_t) {
            .r = r,
            .g = g,
            .b = b,
            .a = a
    };

    return colour_val;
}