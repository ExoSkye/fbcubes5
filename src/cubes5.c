#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "fbdisplay.h"

int main(int argc, char** argv) {
    char* fb = NULL;
    if (getenv("FB") != NULL) {
        fb = getenv("FB");
    } else {
        fb = "/dev/fb0";
    }

    if (!fb_init(fb)) {
        fb_exit();
        return -1;
    }

    fb_info_t fb_info = fb_get_info();

    struct timeval curTime;
    gettimeofday(&curTime, NULL);
    unsigned long start_micro = curTime.tv_sec*(u32)1000000+curTime.tv_usec;

    for (int i = 0; i < 20000; i += 2) {
        fb_clearscreen(white);
        fb_clearscreen(black);
    }
    gettimeofday(&curTime, NULL);
    unsigned long end_micro = curTime.tv_sec*(u32)1000000+curTime.tv_usec;

    printf("20000 Full screen clears took: %lu microseconds\n", end_micro - start_micro);

    fb_exit();
}
