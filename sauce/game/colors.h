#ifndef COLORS_H_
#define COLORS_H_

#include <PalmOS.h>

typedef enum {
    ALIZARIN,
    BELIZEHOLE,
    CARROT,
    EMERALD,
    CLOUDS,
    ASBESTOS,
    DIRT
} AppColor;

IndexedColorType colors_reference[5];

void colors_setupReferenceColors(Boolean colorSupport, UInt32 depth);

#endif