#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        printf("\n");
        return 0;
    }

    return 0;
}