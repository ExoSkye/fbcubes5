#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <string.h>

#define xstr(s) str(s)
#define str(s) #s

int main(int argc, char** argv) {
    if (argc > 1) {
        for (int i = 0; i < strlen(argv[1]); i++) {
            switch (argv[1][i]) {
                case 'R':
                    printf("-DR%i ", i);
                    break;
                case 'G':
                    printf("-DG%i ", i);
                    break;
                case 'B':
                    printf("-DB%i ", i);
                    break;
                case 'A':
                    printf("-DA%i ", i);
                    break;
            }
        }
        return 0;
    }

#ifndef FB_NAME
#define FB_NAME fb0
#endif

#define FB_PATH "/dev/" xstr(FB_NAME)

    if(access(FB_PATH, R_OK) != 0) {
        printf("ERROR: You don't have sufficient permissions to write to the framebuffer (file: %s), exiting...\n", FB_PATH);
        exit(1);
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    int fb_file = open(FB_PATH, O_RDWR);

    if (ioctl(fb_file, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit(2);
    }

    if (ioctl(fb_file, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit(3);
    }

    if (vinfo.bits_per_pixel != 32) {
        printf("Error: Currently only 32bit colour modes are supported, you have %i, exiting...", vinfo.bits_per_pixel);
        exit(4);
    }

    bool alpha_used = false;
    int free_i = 0;

    for (int i = 0; i < 4; i++) {
        if (vinfo.red.offset == i * 8) {
            printf("-DR%i ", i);
        } else if (vinfo.green.offset == i * 8) {
            printf("-DG%i ", i);
        } else if (vinfo.blue.offset == i * 8) {
            printf("-DB%i ", i);
        } else if (vinfo.transp.offset == i * 8) {
            printf("-DA%i ", i);
            alpha_used = true;
        } else {
            free_i = i;
        }
    }

    if (!alpha_used) {
        printf("-DA%i ", free_i);
    }

    printf("\n");

    close(fb_file);

    return 0;
}