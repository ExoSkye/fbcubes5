#include <fbdisplay.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <colour.h>
#include <dlfcn.h>

static char* fb = NULL;

static fb_info_t fb_info;

colour_t* white = NULL;
colour_t* black = NULL;

static void* colour_lib;

char* fb_make_format_str(struct fb_var_screeninfo vinfo) {
    char* format = (char*)malloc(5 * sizeof(char));
    bool alpha_used = false;
    int free_i;

    for (int i = 0; i < 4; i++) {
        if (vinfo.red.offset == i * 8) {
            format[i] = 'R';
        } else if (vinfo.green.offset == i * 8) {
            format[i] = 'G';
        } else if (vinfo.blue.offset == i * 8) {
            format[i] = 'B';
        } else if (vinfo.transp.offset == i * 8) {
            format[i] = 'A';
            alpha_used = true;
        } else {
            free_i = i;
        }
    }

    if (!alpha_used) {
        format[free_i] = 'A';
    }

    return format;
}

struct colour_vtable {
    colour_t* (*fb_make_colour)(u8 r, u8 g, u8 b, u8 a);
    void (*fb_delete_colour)(colour_t* colour_ptr);
};

static struct colour_vtable* imports;

colour_t* fb_make_colour(u8 r, u8 g, u8 b, u8 a) {
    return imports->fb_make_colour(r, g, b, a);
}

void fb_delete_colour(colour_t* colour) {
    imports->fb_delete_colour(colour);
}

bool fb_init(char* fb_name) {
    char* fb_path = (char*)malloc(sizeof(char) * 10);

    snprintf(fb_path, 10, "/dev/%s", fb_name);

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

    char* format = fb_make_format_str(vinfo);

    char* colour_lib_path = (char*)malloc(24 * sizeof(char));
    snprintf(colour_lib_path, 24, "./colours/fc5-colour-%s.so", format);

    colour_lib = dlopen(colour_lib_path, RTLD_NOW);

    if (colour_lib == NULL) {
        printf("ERROR: Can't open colour library (path: %s)", colour_lib_path);
        return false;
    }

    imports = dlsym(colour_lib, "exports");

    if (imports == NULL) {
        printf("ERROR: Can't get exported functions from the colour lib");
        return false;
    }

    printf("Using framebuffer provided by: %s\n"
           "Resolution: %ix%i at %i bpp (%s)\n",
           finfo.id,
           vinfo.xres_virtual, vinfo.yres_virtual, vinfo.bits_per_pixel, format
    );


    fb_info.x = vinfo.xres_virtual;
    fb_info.y = vinfo.yres_virtual;
    fb_info.bpp = vinfo.bits_per_pixel;

    black = fb_make_colour(0, 0, 0, 255);
    white = fb_make_colour(255, 255, 255, 255);

    fb = (char *)mmap(0, fb_info.x * fb_info.y * (fb_info.bpp / 8), PROT_READ | PROT_WRITE, MAP_SHARED, fb_file, 0);

    if ((long)fb == -1) {
        printf("ERROR: Failed to map framebuffer to memory");
        return false;
    }

    close(fb_file);
    free(fb_path);

    fb_clearscreen(black);

    return true;
}

fb_info_t fb_get_info() {
    return fb_info;
}

void fb_clearscreen(colour_t* colour) {
    for (u16 y = 0; y < fb_info.y; y++) {
        for (u16 x = 0; x < fb_info.x; x++) {
            fb_drawpixel(x, y, colour);
        }
    }
}

void fb_drawpixel(int x, int y, const colour_t* colour) {
    ((u32*)fb)[y * fb_info.x + x] = *colour;
}

void fb_exit() {
    if (fb != NULL) {
        munmap(fb, fb_info.x * fb_info.y * (fb_info.bpp / 8));
    }

    if (white != NULL) {
        fb_delete_colour(white);
    }

    if (black != NULL) {
        fb_delete_colour(black);
    }

    if (colour_lib != NULL) {
        dlclose(colour_lib);
    }
}