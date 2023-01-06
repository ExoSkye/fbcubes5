#include <unistd.h>
#include <stdio.h>

#include "fbdisplay.h"

int main(int argc, char** argv) {
    if (argc == 1) {
        fb_path = "/dev/fb0";
    } else {
        fb_path = argv[1];
    }

    if(access(fb_path, W_OK) != 0) {
        printf("ERROR: I don't have sufficient permissions to write to the framebuffer, exiting...\n");
        return -1;
    }
}