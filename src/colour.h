#include <packed.h>

typedef PACKED_STRUCT(
        colour_t {
#ifdef R0
            u8 r;
#elif B0
            u8 b;
#elif G0
            u8 g;
#elif A0
            u8 a;
#else
            u8 r;
#endif

#ifdef R1
            u8 r;
#elif B1
            u8 b;
#elif G1
            u8 g;
#elif A1
            u8 a;
#else
            u8 g;
#endif

#ifdef R2
            u8 r;
#elif B2
            u8 b;
#elif G2
            u8 g;
#elif A2
            u8 a;
#else
            u8 b;
#endif

#ifdef R3
            u8 r;
#elif B3
            u8 b;
#elif G3
            u8 g;
#elif A3
            u8 a;
#else
            u8 a;
#endif
        }
) colour_t;